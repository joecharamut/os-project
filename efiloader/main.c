#include "main.h"

#include <efi.h>
#include <efilib.h>
#include <stdnoreturn.h>

#include "elf.h"
#include "file.h"
#include "video.h"
#include "debug.h"
#include "mem.h"

static noreturn void halt() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}

typedef enum {
    MemoryTypeNull,
    MemoryTypeFree,
    MemoryTypeUsed,
    MemoryTypeReserved,
} memory_type_t;
const char *memory_type_t_strings[] = {
    "MemoryTypeNull",
    "MemoryTypeFree",
    "MemoryTypeUsed",
    "MemoryTypeReserved",
};

typedef struct {
    memory_type_t type;
    UINT64 paddr;
    UINT64 vaddr;
    UINT64 pages;
    UINT64 flags;
} memory_map_entry_t;

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // init gnu-efi
    InitializeLib(ImageHandle, SystemTable);

    // disable watchdog
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    EFI_STATUS status;
    EFI_FILE_HANDLE volume = GetVolume(ImageHandle);
    if (!volume) {
        return EFI_DEVICE_ERROR;
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

    EFI_FILE_HANDLE consoleFont = OpenFile(volume, L"\\qOS\\unifont.sfn");
    if (!consoleFont) {
        return EFI_NOT_FOUND;
    }

    UINT64 fontSize = FileSize(consoleFont);
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
    printf("UEFI Version %d.%d [Vendor: %ls, Revision: 0x%08X]\n",
           ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xFFFF, ST->FirmwareVendor, ST->FirmwareRevision);
    printf("GOP Framebuffer is at 0x%016llx\n", (UINT64) get_framebuffer());

    EFI_FILE_HANDLE kernelHandle = OpenFile(volume, L"KERNEL2.BIN");
    if (!kernelHandle) {
        return EFI_NOT_FOUND;
    }

    printf("Loading Kernel Header...");
    UINTN bufsz = 64;
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

    printf("Parsing Kernel Header...\n");
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

    UINT64 kernelSize = FileSize(kernelHandle);
    printf("Kernel size is %lld bytes\n", kernelSize);

    UINT64 readSize = kernelSize;
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

    UINTN mapKey = 0;
    UINTN mapSize = 0;
    UINTN descriptorSize = 0;
    UINT8 *mmap = (void *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);

    while (BS->ExitBootServices(ImageHandle, mapKey) == EFI_INVALID_PARAMETER) {
        FreePool(mmap);
        mmap = (void *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);
    }
    printf("Boot services terminated successfully\n");

//    mmap = (void *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);
    efi_dump_mem_map(mmap, mapSize, descriptorSize);

    UINT64 convMemory = 0;
    UINT64 reclaimMemory = 0;
    UINT64 runtimeMemory = 0;
    UINT64 otherMemory = 0;
    for (UINT64 i = 0; i < mapSize; ++i) {
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
    UINT64 totalMemory = convMemory + reclaimMemory + runtimeMemory + otherMemory;

    printf("mmap reports %lld KiB of memory:\n", totalMemory);
    printf(" Free: %lld KiB (~%lld%%)\n", convMemory, convMemory * 100 / totalMemory);
    printf(" Reclaimable: %lld KiB (~%lld%%)\n", reclaimMemory, reclaimMemory * 100 / totalMemory);
    printf(" Runtime Services: %lld KiB (~%lld%%)\n", runtimeMemory, runtimeMemory * 100 / totalMemory);
    printf(" Other: %lld KiB (~%lld%%)\n", otherMemory, otherMemory * 100 / totalMemory);

    UINTN rip, rsp, rbp;
    __asm__ volatile ("1: lea 1b(%%rip), %0\n\t"
                      "movq %%rsp, %1\n\t"
                      "movq %%rbp, %2\n\t"
                      : "=a" (rip), "=g" (rsp), "=g" (rbp)
                      );
    printf("rip is currently: 0x%016llx\n", rip);
    printf("rsp is currently: 0x%016llx\n", rsp);
    printf("rbp is currently: 0x%016llx\n", rbp);

    for (UINTN i = 0; i < mapSize; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) (mmap + (i * descriptorSize));
        UINT64 start = entry->PhysicalStart;
        UINT64 end = start + (entry->NumberOfPages * 4096);

        if (rip > start && rip < end) {
            printf("rip is in mmap descriptor %lld\n", i);
        }

        if (rsp > start && rsp < end) {
            printf("rsp is in mmap descriptor %lld\n", i);
        }

        if (rbp > start && rbp < end) {
            printf("rbp is in mmap descriptor %lld\n", i);
        }

        if (((UINT64) kernelBuf) > start && ((UINT64) kernelBuf) < end) {
            printf("kernel image is in mmap descriptor %lld\n", i);
        }
    }

    printf("Condensing mmap...\n");
    UINT64 condensedSize = 0;
    memory_map_entry_t condensedMmap[mapSize];
    memset(condensedMmap, 0, mapSize * sizeof(memory_map_entry_t));

    for (UINTN i = 0; i < mapSize; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) (mmap + (i * descriptorSize));

        memory_type_t entryType = MemoryTypeNull;
        if (entry->Type == EfiConventionalMemory || entry->Type == EfiBootServicesCode || entry->Type == EfiBootServicesData) {
            entryType = MemoryTypeFree;
        } else if (entry->Type == EfiLoaderCode || entry->Type == EfiLoaderData) {
            entryType = MemoryTypeUsed;
        } else {
            entryType = MemoryTypeReserved;
        }

        if (condensedMmap[condensedSize].type == MemoryTypeNull) {
            condensedMmap[condensedSize].type = entryType;
            condensedMmap[condensedSize].paddr = entry->PhysicalStart;
            condensedMmap[condensedSize].vaddr = entry->VirtualStart;
            condensedMmap[condensedSize].pages = entry->NumberOfPages;
            condensedMmap[condensedSize].flags = entry->Attribute;
        }

        if (condensedMmap[condensedSize].type != entryType) {
            ++condensedSize;
            --i;
            continue;
        }

        condensedMmap[condensedSize].pages += entry->NumberOfPages;
    }

    printf("New mmap:\n");
    for (UINT64 i = 0; i < condensedSize; ++i) {
        printf("%lld,%s (%d),0x%08llx,0x%08llx,%lld (%lld KiB),[%c%c%c] (0x%llx)\n",
               i, memory_type_t_strings[condensedMmap[i].type], condensedMmap[i].type,
               condensedMmap[i].paddr, condensedMmap[i].vaddr,
               condensedMmap[i].pages, condensedMmap[i].pages * 4,

               (condensedMmap[i].flags & EFI_MEMORY_RP) ? '-' : 'R',
               (condensedMmap[i].flags & EFI_MEMORY_WP) ? '-' : 'W',
               (condensedMmap[i].flags & EFI_MEMORY_XP) ? '-' : 'X',
               condensedMmap[i].flags);
    }

    UINT64 cr3;
    __asm__ volatile ("mov %%cr3, %0\n\t" : "=g" (cr3));
    printf("cr3 is 0x%016llx\n", cr3);

    for (int i = 0; i < 512; ++i) {
        pml4_entry_t pml4Entry = ((pml4_entry_t *) cr3)[i];
        if (pml4Entry.present) {
//            printf("pml4 entry %lld -> 0x%016llx @@ 0x%016llx\n", i, pml4Entry.value, pml4Entry.page_ppn);
            printf("pml4[%lld] -> P:%llx W:%llx U:%llx WT:%llx CD:%llx A:%llx I3:%llx S:%llx I2:%llx PP:%llx I1:%llx NX:%llx\n",
                   i,
                   pml4Entry.present,
                   pml4Entry.writeable,
                   pml4Entry.user_access,
                   pml4Entry.write_through,
                   pml4Entry.cache_disabled,
                   pml4Entry.accessed,
                   pml4Entry.ignored_3,
                   pml4Entry.size,
                   pml4Entry.ignored_2,
                   pml4Entry.page_ppn,
                   pml4Entry.ignored_1,
                   pml4Entry.execution_disabled
            );

            for (int j = 0; j < 512; ++j) {
                pdpt_entry_t pdptEntry = ((pdpt_entry_t *) (pml4Entry.page_ppn * 0x1000))[j];

                if (pdptEntry.present) {
//                    printf("pdpt entry %lld:%lld is present\n", i, j);
                    printf("pml4[%lld][%lld] -> P:%lld W:%lld U:%lld WT:%lld CD:%lld A:%lld I3:%lld S:%lld I2:%lld PP:%lld I1:%lld NX:%lld\n",
                           i, j,
                           pdptEntry.present,
                           pdptEntry.writeable,
                           pdptEntry.user_access,
                           pdptEntry.write_through,
                           pdptEntry.cache_disabled,
                           pdptEntry.accessed,
                           pdptEntry.ignored_3,
                           pdptEntry.size,
                           pdptEntry.ignored_2,
                           pdptEntry.page_ppn,
                           pdptEntry.ignored_1,
                           pdptEntry.execution_disabled
                    );
                }
            }
        }
    }

    halt();

    header = kernelBuf;
    printf("Parsing kernel phdrs...\n");

    for (uint16_t i = 0; i < header->pht_entry_count; ++i) {
        elf64_pht_entry_t *entry = (void *) (((char *) kernelBuf) + header->pht_offset + (i * header->pht_entry_size));
        printf("Header %d:\n Type: %s (%d)\n Flags: [%c%c%c]\n Offset: 0x%llx\n Virtual: 0x%llx\n Physical: 0x%llx\n",
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
            printf("Trying to load segment %d (0x%llx bytes) to phys address 0x%016llx...", i, entry->filesize, entry->paddr);
            memcpy((void *) entry->paddr, ((char *) kernelBuf) + entry->offset, entry->filesize);
            printf("OK!\n");
        }
    }

    printf("Setup complete, calling the kernel!\n\n\n\n\n");
    ((void (*)()) header->entry)();

    printf("Halted.");
    halt();
}
