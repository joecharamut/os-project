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
        return status;
    }

    EFI_FILE_HANDLE volume;
    status = GetVolume(ImageHandle, &volume);
    if (EFI_ERROR(status)) {
        return status;
    }

    // setup console
    status = video_init();
    if (EFI_ERROR(status)) {
        return status;
    }

    // init serial
    if (serial_init()) {
        return EFI_DEVICE_ERROR;
    }

    EFI_FILE_HANDLE consoleFont;
    status = volume->Open(volume, &consoleFont, L"\\QOS\\FONT.SFN", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        return status;
    }

    uint64_t fontSize = FileSize(consoleFont);
    void *fontBuf = AllocatePool(fontSize);
    status = consoleFont->Read(consoleFont, &fontSize, fontBuf);
    if (EFI_ERROR(status)) {
        return status;
    }

    set_background_color(make_color(0x20, 0x10, 0x20));
    set_foreground_color(make_color(0xee, 0xee, 0xee));
    clear_screen();
    set_font(fontBuf);

    printf("Starting EFILoader!\n");
    printf("UEFI Version %d.%d [Vendor: %ls, Revision: 0x%x]\n",
           ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xFFFF, ST->FirmwareVendor, ST->FirmwareRevision);

    EFI_FILE_HANDLE kernelHandle;
    status = volume->Open(volume, &kernelHandle, L"RKERNEL.BIN", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        return status;
    }

    printf("Loading Kernel Header...");
    uint64_t bufsz = 64;
    void *buf = AllocatePool(bufsz);
    status = kernelHandle->Read(kernelHandle, &bufsz, buf);
    if (EFI_ERROR(status)) {
        printf("Error\n");
        return status;
    }
    printf("OK!\n");

    if (!elf_is_header_valid(buf)) {
        printf("Invalid ELF Header\n");
        return EFI_UNSUPPORTED;
    }

    if (!elf_is_64_bit(buf)) {
        printf("Unsupported ELF Format\n");
        return EFI_UNSUPPORTED;
    }

    elf64_header_t *header = buf;

    if (header->type != ELF_TYPE_EXEC) {
        printf("Unsupported ELF Object Type\n");
        return EFI_UNSUPPORTED;
    }

    if (header->instruction_set != 0x3E) {
        printf("Unsupported ELF Instruction Set\n");
        return EFI_UNSUPPORTED;
    }

    FreePool(buf);
    printf("ELF header looks okay... Loading the whole kernel!\n");

    uint64_t kernelSize = FileSize(kernelHandle);
    printf("Kernel size is %ld bytes\n", kernelSize);

    uint64_t readSize = kernelSize;
    void *kernelBuf = AllocatePool(kernelSize);
    if (!kernelBuf) {
        printf("Unable to allocate memory\n");
        return EFI_OUT_OF_RESOURCES;
    }

    kernelHandle->SetPosition(kernelHandle, 0);
    kernelHandle->Read(kernelHandle, &readSize, kernelBuf);
    if (readSize != kernelSize) {
        printf("Kernel was not fully loaded\n");
        return EFI_DEVICE_ERROR;
    }

    uint64_t mapKey = 0;
    uint64_t mapSize = 0;
    uint64_t descriptorSize = 0;
    uint8_t *mmap = (uint8_t *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);

    while (BS->ExitBootServices(ImageHandle, mapKey) == EFI_INVALID_PARAMETER) {
        FreePool(mmap);
        mmap = (void *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);
    }
    printf("Boot services terminated successfully\n");

//    efi_dump_mem_map(mmap, mapSize, descriptorSize);

    printf("int: %x, long: %x, long long: %x\n", sizeof(int), sizeof(long), sizeof(long long));
    printf("-1: %#x, -1: %d, -1: %u\n", -1, -1, -1);

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

    printf("mmap reports %ld KiB of memory:\n", totalMemory);
    printf(" Free: %ld KiB (~%ld%%)\n", convMemory, convMemory * 100 / totalMemory);
    printf(" Reclaimable: %ld KiB (~%ld%%)\n", reclaimMemory, reclaimMemory * 100 / totalMemory);
    printf(" Runtime Services: %ld KiB (~%ld%%)\n", runtimeMemory, runtimeMemory * 100 / totalMemory);
    printf(" Other: %ld KiB (~%ld%%)\n", otherMemory, otherMemory * 100 / totalMemory);

    uint64_t rip, rsp, rbp;
    __asm__ volatile ("1: lea 1b(%%rip), %0\n\t"
                      "movq %%rsp, %1\n\t"
                      "movq %%rbp, %2\n\t"
                      : "=a" (rip), "=g" (rsp), "=g" (rbp)
                      );
    printf("rip is currently: 0x%016lx\n", rip);
    printf("rsp is currently: 0x%016lx\n", rsp);
    printf("rbp is currently: 0x%016lx\n", rbp);

    printf("boot_data_t is %lld bytes long\n", sizeof(boot_data_t));

    for (uint64_t i = 0; i < mapSize; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) (mmap + (i * descriptorSize));
        uint64_t start = entry->PhysicalStart;
        uint64_t end = start + (entry->NumberOfPages * 4096);

        if (rip > start && rip < end) {
            printf("rip is in mmap descriptor %ld\n", i);
        }

        if (rsp > start && rsp < end) {
            printf("rsp is in mmap descriptor %ld\n", i);
        }

        if (rbp > start && rbp < end) {
            printf("rbp is in mmap descriptor %ld\n", i);
        }

        if (((uint64_t) kernelBuf) > start && ((uint64_t) kernelBuf) < end) {
            printf("kernel image is in mmap descriptor %ld\n", i);
        }
    }

    printf("Identity mapping first 4GiB of address space\n");
    for (uint64_t i = 0; i < 0x100000000; i += 0x40000000) {
        map_page((physical_address_t) { .value=i }, (virtual_address_t){ .value=i }, PageSize1GiB);
    }
    printf("Mapping last 4GiB of address space\n");
    for (uint64_t i = 0; i < 0x100000000; i += 0x40000000) {
        map_page((physical_address_t) { .value=i }, (virtual_address_t){ .value=i + 0xFFFFFFF800000000 }, PageSize1GiB);
    }
    printf("Enabling the new page map\n");
    load_page_map();

    header = kernelBuf;
    printf("Parsing kernel phdrs...\n");

    for (uint16_t i = 0; i < header->pht_entry_count; ++i) {
        elf64_pht_entry_t *entry = (void *) (((char *) kernelBuf) + header->pht_offset + (i * header->pht_entry_size));
        if (entry->type != ELF_PTYPE_LOAD) continue;

        printf("Header %d:\n Type: %s (%d)\n Flags: [%c%c%c]\n Offset: 0x%lx\n Virtual: 0x%lx\n Physical: 0x%lx\n",
               i,
               elf_ptype_strings[entry->type], entry->type,
               (entry->flags & ELF_PFLAG_R ? 'R' : '-'),
               (entry->flags & ELF_PFLAG_W ? 'W' : '-'),
               (entry->flags & ELF_PFLAG_X ? 'X' : '-'),
               entry->offset,
               entry->vaddr,
               entry->paddr
        );

        if (entry->type == ELF_PTYPE_LOAD) {
            printf("Trying to load segment %d (0x%lx bytes) to phys address 0x%016lx...", i, entry->filesize, entry->paddr);
            lmemcpy((void *) entry->paddr, ((char *) kernelBuf) + entry->offset, entry->filesize);
            printf("OK!\n");
        }
    }

    printf("Setup complete, calling the kernel!\n");

    boot_data_t *bootData = allocate(sizeof(boot_data_t));
    bootData->signature = BOOT_DATA_SIGNATURE;
    copy_video_info(bootData);

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

    bootData->allocation_info.image_base = 0x900000;
    bootData->allocation_info.image_size = kernelSize;

    bootData->allocation_info.stack_base = 0x200000;
    bootData->allocation_info.stack_size = 0;

    ((bootstrap_fn_ptr_t) header->entry)(bootData);
    halt();
}
