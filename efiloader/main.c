#include "main.h"

#include <efi.h>
#include <efilib.h>
#include <stdnoreturn.h>

#include "elf.h"
#include "file.h"
#include "video.h"
#include "debug.h"

static noreturn void halt() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}

static void *memcpy(void *dst, void *src, UINT64 num) {
    for (UINT64 i = 0; i < num; ++i) {
        ((char*) dst)[i] = ((char*) src)[i];
    }
    return dst;
}

static void *memset(void *ptr, unsigned char value, UINT64 num) {
    for (UINT64 i = 0; i < num; ++i) {
        *((unsigned char *) ptr + i) = value;
    }
    return ptr;
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

const char * const EFI_MEMORY_TYPE_STRINGS[] = {
        "EfiReservedMemoryType",
        "EfiLoaderCode",
        "EfiLoaderData",
        "EfiBootServicesCode",
        "EfiBootServicesData",
        "EfiRuntimeServicesCode",
        "EfiRuntimeServicesData",
        "EfiConventionalMemory",
        "EfiUnusableMemory",
        "EfiACPIReclaimMemory",
        "EfiACPIMemoryNVS",
        "EfiMemoryMappedIO",
        "EfiMemoryMappedIOPortSpace",
        "EfiPalCode",
        "EfiMaxMemoryType"
};

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


const char *efi_mem_type_string(UINT32 type) {
    if (type < 15) {
        return EFI_MEMORY_TYPE_STRINGS[type];
    } else if (type < 0x6FFFFFFF) {
        return "Reserved";
    } else if (type < 0x7FFFFFFF) {
        return "Reserved (OEM)";
    } else {
        return "Reserved (OSV)";
    }
}

EFI_MEMORY_DESCRIPTOR *efi_get_mem_map(UINTN *MapKey, UINTN *Entries, UINTN *DescriptorSize) {
    EFI_STATUS status;

    UINTN mmapSize = 0;
    EFI_MEMORY_DESCRIPTOR *memoryMap = NULL;
    UINTN mmapKey = 0;
    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;

    while ((status = BS->GetMemoryMap(&mmapSize, memoryMap, &mmapKey, &descriptorSize, &descriptorVersion)) == EFI_BUFFER_TOO_SMALL) {
        mmapSize += descriptorSize * 8;

        if (memoryMap) {
            FreePool(memoryMap);
        }
        memoryMap = AllocatePool(mmapSize);
    }

    if (EFI_ERROR(status)) {
        Print(L"Failed to get memory map: 0x%llx\n", status);
        return NULL;
    }

    if (MapKey) {
        *MapKey = mmapKey;
    }

    if (Entries) {
        *Entries = mmapSize / descriptorSize;
    }

    if (DescriptorSize) {
        *DescriptorSize = descriptorSize;
    }

    return memoryMap;
}

void efi_dump_mem_map(void *mmap, UINTN size, UINTN descriptorSize) {
    printf("Index,Type,Physical Address,Virtual Address,Pages,Attributes\n");
    for (UINTN i = 0; i < size; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (void *) (((char *) mmap) + (i * descriptorSize));
        printf("%s%lld,%s (%d),0x%08llx,0x%08llx,%lld (%lld KiB),[%c%c%c] (0x%llx)\n",
              entry->NumberOfPages == 0 ? "!!! " : "",
              i, efi_mem_type_string(entry->Type), entry->Type,
              entry->PhysicalStart, entry->VirtualStart,
              entry->NumberOfPages, entry->NumberOfPages * 4,

              (entry->Attribute & EFI_MEMORY_RP) ? '-' : 'R',
              (entry->Attribute & EFI_MEMORY_WP) ? '-' : 'W',
              (entry->Attribute & EFI_MEMORY_XP) ? '-' : 'X',
              entry->Attribute);
    }
}
