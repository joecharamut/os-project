#include "../common/boot_data.h"
#include "elf.h"
#include "file.h"
#include "video.h"
#include "debug.h"
#include "mem.h"

#include <efi.h>
#include <efilib.h>
#include <stdnoreturn.h>

static noreturn void halt() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;

    // init gnu-efi
    InitializeLib(ImageHandle, SystemTable);

    // disable watchdog
    status = BS->SetWatchdogTimer(0, 0, 0, NULL);
    if (EFI_ERROR(status)) {
        beep(1);
        return status;
    }

    EFI_FILE_HANDLE volume;
    status = GetVolume(ImageHandle, &volume);
    if (EFI_ERROR(status)) {
        beep(2);
        return status;
    }

    // setup console
    status = video_init();
    if (EFI_ERROR(status)) {
        beep(3);
        return status;
    }

    // init serial
    // dont fail on error because computer might not have serial
    if (serial_init(0x3f8)) {
        // todo: something
    }

    EFI_FILE_HANDLE consoleFont;
    status = volume->Open(volume, &consoleFont, L"\\QOS\\FONT.SFN", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        beep(5);
        return status;
    }

    uint64_t fontSize = FileSize(consoleFont);
    void *fontBuf = AllocatePool(fontSize);
    status = consoleFont->Read(consoleFont, &fontSize, fontBuf);
    if (EFI_ERROR(status)) {
        beep(6);
        return status;
    }

    set_background_color(make_color(0x20, 0x10, 0x20));
    set_foreground_color(make_color(0xee, 0xee, 0xee));
    clear_screen();
    set_font(fontBuf);

    dbg_print("Starting EFILoader!\n");
    dbg_print("UEFI Version %d.%d [Vendor: %ls, Revision: 0x%x]\n",
           ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xFFFF, ST->FirmwareVendor, ST->FirmwareRevision);

    EFI_LOADED_IMAGE *LoadedImage = NULL;
    status = BS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void **) &LoadedImage);
    if (EFI_ERROR(status)) {
        return status;
    }

    dbg_print("UEFI Image Base is %#016llx\n", LoadedImage->ImageBase);

    EFI_FILE_HANDLE kernelHandle;
    status = volume->Open(volume, &kernelHandle, L"RKERNEL.BIN", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        return status;
    }

    dbg_print("Loading Kernel Header...");
    uint64_t bufsz = 64;
    void *buf = AllocatePool(bufsz);
    status = kernelHandle->Read(kernelHandle, &bufsz, buf);
    if (EFI_ERROR(status)) {
        dbg_print("Error\n");
        return status;
    }
    dbg_print("OK!\n");

    if (!elf_is_header_valid(buf)) {
        dbg_print("Invalid ELF Header\n");
        return EFI_UNSUPPORTED;
    }

    if (!elf_is_64_bit(buf)) {
        dbg_print("Unsupported ELF Format\n");
        return EFI_UNSUPPORTED;
    }

    elf64_header_t *header = buf;

    if (header->type != ELF_TYPE_EXEC) {
        dbg_print("Unsupported ELF Object Type\n");
        return EFI_UNSUPPORTED;
    }

    if (header->instruction_set != 0x3E) {
        dbg_print("Unsupported ELF Instruction Set\n");
        return EFI_UNSUPPORTED;
    }

    FreePool(buf);
    dbg_print("ELF header looks okay... Loading the whole kernel!\n");

    uint64_t kernelSize = FileSize(kernelHandle);
    dbg_print("Kernel size is %ld bytes\n", kernelSize);

    uint64_t readSize = kernelSize;
    void *kernelBuf = AllocatePool(kernelSize);
    if (!kernelBuf) {
        dbg_print("Unable to allocate memory\n");
        return EFI_OUT_OF_RESOURCES;
    }

    kernelHandle->SetPosition(kernelHandle, 0);
    kernelHandle->Read(kernelHandle, &readSize, kernelBuf);
    if (readSize != kernelSize) {
        dbg_print("Kernel was not fully loaded\n");
        return EFI_DEVICE_ERROR;
    }

    uint64_t mapKey = 0;
    uint64_t mapSize = 0;
    uint64_t descriptorSize = 0;
    uint8_t *mmap = (uint8_t *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);

//    efi_dump_mem_map_to_file(mmap, mapSize, descriptorSize, volume);

    while (BS->ExitBootServices(ImageHandle, mapKey) == EFI_INVALID_PARAMETER) {
        FreePool(mmap);
        mmap = (void *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);
    }
    dbg_print("Boot services terminated successfully\n");

//    efi_dump_mem_map(mmap, mapSize, descriptorSize);

    uint64_t convMemory = 0;
    uint64_t reclaimMemory = 0;
    uint64_t runtimeMemory = 0;
    uint64_t otherMemory = 0;
    for (uint64_t i = 0; i < mapSize; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) (mmap + (i * descriptorSize));

        if (entry->Type == EfiConventionalMemory) {
            convMemory += entry->NumberOfPages * 4;
        } else if (entry->Type == EfiBootServicesCode || entry->Type == EfiBootServicesData || entry->Type == EfiLoaderCode || entry->Type == EfiLoaderData) {
            reclaimMemory += entry->NumberOfPages * 4;
        } else if (entry->Type == EfiRuntimeServicesCode || entry->Type == EfiRuntimeServicesData) {
            runtimeMemory += entry->NumberOfPages * 4;
        } else {
            otherMemory += entry->NumberOfPages * 4;
        }
    }
    uint64_t totalMemory = convMemory + reclaimMemory + runtimeMemory + otherMemory;

    dbg_print("mmap reports %ld KiB of memory:\n", totalMemory);
    dbg_print(" Free: %ld KiB (~%ld%%)\n", convMemory, convMemory * 100 / totalMemory);
    dbg_print(" Reclaimable: %ld KiB (~%ld%%)\n", reclaimMemory, reclaimMemory * 100 / totalMemory);
    dbg_print(" Runtime Services: %ld KiB (~%ld%%)\n", runtimeMemory, runtimeMemory * 100 / totalMemory);
    dbg_print(" Other: %ld KiB (~%ld%%)\n", otherMemory, otherMemory * 100 / totalMemory);

    uint64_t rip, rsp, rbp;
    __asm__ volatile ("1: lea 1b(%%rip), %0\n\t"
                      "movq %%rsp, %1\n\t"
                      "movq %%rbp, %2\n\t"
                      : "=a" (rip), "=g" (rsp), "=g" (rbp)
                      );
    dbg_print("rip is currently: 0x%016lx\n", rip);
    dbg_print("rsp is currently: 0x%016lx\n", rsp);
    dbg_print("rbp is currently: 0x%016lx\n", rbp);

    dbg_print("boot_data_t is %lld bytes long\n", sizeof(boot_data_t));

    for (uint64_t i = 0; i < mapSize; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) (mmap + (i * descriptorSize));
        uint64_t start = entry->PhysicalStart;
        uint64_t end = start + (entry->NumberOfPages * 4096);

        if (rip > start && rip < end) {
            dbg_print("rip is in mmap descriptor %ld\n", i);
        }

        if (rsp > start && rsp < end) {
            dbg_print("rsp is in mmap descriptor %ld\n", i);
        }

        if (rbp > start && rbp < end) {
            dbg_print("rbp is in mmap descriptor %ld\n", i);
        }

        if (((uint64_t) kernelBuf) > start && ((uint64_t) kernelBuf) < end) {
            dbg_print("kernel image is in mmap descriptor %ld\n", i);
        }
    }

    dbg_print("Setting up the pml4\n");
    poke_pml4();
    dbg_print("Identity mapping the heap\n");
    for (uint64_t i = 0x100000; i < 0x200000; i += 0x1000) {
        map_page((physical_address_t) { .value=i }, (virtual_address_t){ .value=i }, PageSize4KiB);
    }
//    dbg_print("Mapping last 4GiB of address space\n");
//    for (uint64_t i = 0; i < 0x100000000; i += 0x40000000) {
//        map_page((physical_address_t) { .value=i }, (virtual_address_t){ .value=i + 0xFFFFFFF800000000 }, PageSize1GiB);
//    }
//    dbg_print("Enabling the new page map\n");
//    load_page_map();

    header = kernelBuf;
    dbg_print("Parsing kernel phdrs...\n");

    for (uint16_t i = 0; i < header->pht_entry_count; ++i) {
        elf64_pht_entry_t *entry = (void *) (((char *) kernelBuf) + header->pht_offset + (i * header->pht_entry_size));

        dbg_print("Header %d:\n Type: %s (%#llx)\n Flags: [%c%c%c]\n Offset: 0x%lx\n Virtual: 0x%lx\n Physical: 0x%lx\n",
                  i,
                  elf_ptype_string(entry->type), entry->type,
                  (entry->flags & ELF_PFLAG_R ? 'R' : '-'),
                  (entry->flags & ELF_PFLAG_W ? 'W' : '-'),
                  (entry->flags & ELF_PFLAG_X ? 'X' : '-'),
                  entry->offset,
                  entry->vaddr,
                  entry->paddr
        );

        if (entry->type == ELF_PTYPE_LOAD) {
            dbg_print("Loading segment %d (0x%lx bytes) to physical address 0x%016lx...\n", i, entry->filesize, entry->paddr);
            lmemcpy((void *) entry->paddr, ((char *) kernelBuf) + entry->offset, entry->filesize);

            dbg_print("Mapping segment %d (0x%lx bytes) to virtual address 0x%016lx...\n", i, entry->filesize, entry->paddr);
            for (uint64_t i = 0; i < entry->memsize; i += 0x1000) {
                map_page(
                        (physical_address_t) { .value=entry->paddr + i },
                        (virtual_address_t){ .value=entry->vaddr + i },
                        PageSize4KiB
                );
            }
            dbg_print("Done!\n");
        }
    }

    dbg_print("Writing boot data...\n");

    boot_data_t *bootData = allocate(sizeof(boot_data_t));
    bootData->signature = BOOT_DATA_SIGNATURE;
    copy_video_info(bootData);

    dbg_print("Mapping the framebuffer\n");
    for (uint64_t i = 0; i < bootData->video_info.bufferSize; i += 0x1000) {
        map_page(
                (physical_address_t) { .value=bootData->video_info.bufferAddress+i },
                (virtual_address_t){ .value=bootData->video_info.bufferAddress+i },
                PageSize4KiB
        );
    }

    bootData->memory_map.count = mapSize;
    bootData->memory_map.entries = allocate(sizeof(memory_descriptor_t) * mapSize);
    for (uint64_t i = 0; i < mapSize; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) (mmap + (i * descriptorSize));
        bootData->memory_map.entries[i] = (memory_descriptor_t) {
                .type = entry->Type,
                .padding = 0,
                .physical_address = entry->PhysicalStart,
                .virtual_address = entry->VirtualStart,
                .number_of_pages = entry->NumberOfPages,
                .flags = entry->Attribute,
        };
    }

    EFI_GUID acpi20_guid = ACPI_20_TABLE_GUID;
    const char *rsdp_sig = "RSD PTR ";
    acpi_rsdp_t *the_rsdp = NULL;

    EFI_GUID smbios_guid = SMBIOS_TABLE_GUID;
    EFI_GUID smbios3_guid = SMBIOS3_TABLE_GUID;
    const char *smbios_sig = "_SM_";
    const char *smbios3_sig = "_SM3_";
    smbios_entrypoint_t *the_smbios = NULL;

    for (uint64_t i = 0x000F0000; i < 0x00100000; i += 16) {
        if (lmemcmp((void *) i, smbios3_sig, 5) == 0) {
            dbg_print("found smbios3 entry point: %#016x\n", i);
        }
    }

    for (uint64_t i = 0; i < SystemTable->NumberOfTableEntries; ++i) {
        EFI_CONFIGURATION_TABLE *entry = &SystemTable->ConfigurationTable[i];

        if (lmemcmp(&entry->VendorGuid, &acpi20_guid, 16) == 0) {
            dbg_print("Found ACPI 2.0 table @ %#016llx\n", entry->VendorTable);
            acpi_rsdp_t *rsdp = (acpi_rsdp_t *) entry->VendorTable;

            if (lmemcmp(rsdp->signature, rsdp_sig, 8) == 0) {
                uint8_t checksum = 0;
                for (uint32_t j = 0; j < rsdp->length; ++j) {
                    checksum += ((uint8_t *) rsdp)[j];
                }

                if ((checksum & 0xFF) == 0) {
                    the_rsdp = rsdp;
                    dbg_print("RSDP signature and checksum are valid, oemid='%c%c%c%c%c%c', rev=%d\n",
                              the_rsdp->oemid[0], the_rsdp->oemid[1], the_rsdp->oemid[2],
                              the_rsdp->oemid[3], the_rsdp->oemid[4], the_rsdp->oemid[5],
                              the_rsdp->revision
                    );
                } else {
                    dbg_print("Invalid RSDP checksum\n");
                }
            } else {
                dbg_print("Invalid RSDP signature\n");
            }

            continue;
        }

        if (lmemcmp(&entry->VendorGuid, &smbios_guid, 16) == 0) {
            dbg_print("SMBIOS entry @ %#016llx\n", entry->VendorTable);

            if (lmemcmp(entry->VendorTable, smbios_sig, 4) == 0) {
                the_smbios = entry->VendorTable;
                dbg_print("Valid SMBIOS signature [table: @ %#08x, %d bytes long, %d structs]\n",
                          the_smbios->structure_table_address, the_smbios->structure_table_length,
                          the_smbios->number_of_structures);
            } else {
                dbg_print("Invalid SMBIOS signature\n");
            }

            continue;
        }

        if (lmemcmp(&entry->VendorGuid, &smbios3_guid, 16) == 0) {
            dbg_print("SMBIOS 3 ptr->%#016llx\n", entry->VendorTable);

            if (lmemcmp(entry->VendorTable, &smbios_sig, 4) == 0) {
                dbg_print("Valid Signature!\n");
            } else {
                dbg_print("Invalid Signature\n");
            }

            continue;
        }
    }

    if (!the_rsdp) {
        dbg_print("ACPI RSDP was not found, cannot continue booting.\n");
        halt();
    }

    if (the_rsdp->revision < 2) {
        dbg_print("ACPI Revision < 2.0, cannot continue booting.\n");
        halt();
    }

    lmemcpy(&bootData->rsdp, the_rsdp, sizeof(acpi_rsdp_t));

    if (the_smbios) {
        lmemcpy(&bootData->smbios_entry, the_smbios, sizeof(smbios_entrypoint_t));
    }

    // basic mmap as far as i understand
    // 0x0000'0000 - 0x007F'FFFF: Available ~8 MiB
    // 0x0080'0000 - 0x008F'FFFF: Fragmented ACPIMemoryNVS & ConventionalMemory
    // 0x0090'0000 - 0x????'????: Available (Depending on RAM size)
    // 0x????'???? - 0x????'????: Misc EfiRuntimeServices & ACPIMemory Regions (upper ~8 MiB i think?)
    // 0xFFE0'0000 - 0xFFFF'FFFF: MemoryMappedIO ~2 MiB

    // todo: see if uefi has a way to see where the start of physical memory is / where it is safe to load to
    bootData->allocation_info.image_base = 0x200000;
    bootData->allocation_info.image_size = kernelSize;

    // todo: make sure this area is fine to use for stack
    // stack at 2 MiB growing down should be fine for setup
    // pairing with slab alloc at 1 MiB
    bootData->allocation_info.stack_base = 0x1FFFF0;
    bootData->allocation_info.stack_size = 0x100000;

    bootData->allocation_info.page_map_base = (uint64_t) pml4_pointer;

    dbg_print("Kernel entrypoint is at %#016llx\n", header->entry);
    dbg_print("Setup complete, calling the kernel!\n");
    dbg_print("=================================[END OF LOADER]================================\n");

    ((bootstrap_fn_ptr_t) header->entry)(bootData);

    halt();
}
