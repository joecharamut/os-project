#include "main.h"

#include <efi.h>
#include <efilib.h>
#include <stdnoreturn.h>

#include "strings.h"
#include "elf.h"
#include "file.h"
#include "video.h"
#include "serial.h"
#include "printf.h"

static noreturn void halt() {
    __asm__ ("cli; hlt; jmp .");
    __builtin_unreachable();
}

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

    EFI_FILE_HANDLE kernelHandle = OpenFile(volume, L"KERNEL.BIN");
    if (!kernelHandle) {
        return EFI_NOT_FOUND;
    }

    printf("Starting EFILoader!\n");
    printf("UEFI Version %d.%d [Vendor: %ls, Revision: 0x%08X]\n",
           ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xFFFF, ST->FirmwareVendor, ST->FirmwareRevision);

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

    header = kernelBuf;
    printf("Entrypoint is at 0x%08llx\n", header->entry);
    printf("Parsing program headers...\n");
    UINTN pht_size = header->pht_entry_count * header->pht_entry_size;
    elf64_pht_entry_t *pht = kernelBuf + header->pht_offset;

    for (uint16_t i = 0; i < header->pht_entry_count; ++i) {
        printf("Header %d:\n Type: %d\n Flags: 0x%x\n Offset: 0x%llx\n Virtual: 0x%llx\n Physical: 0x%llx\n",
               i,
               pht[i].type,
               pht[i].flags,
               pht[i].offset,
               pht[i].vaddr,
               pht[i].paddr
        );
    }

    UINTN mapKey = 0;
    UINTN mapSize = 0;
    UINTN descriptorSize = 0;
    UINT8 *mmap = (void *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);
    efi_dump_mem_map(mmap, mapSize, descriptorSize);

    while (BS->ExitBootServices(ImageHandle, mapKey) == EFI_INVALID_PARAMETER) {
        FreePool(mmap);
        mmap = (void *) efi_get_mem_map(&mapKey, &mapSize, &descriptorSize);
    }

    UINTN convMemory = 0;
    UINTN reclaimMemory = 0;
    UINTN otherMemory = 0;
    for (UINTN i = 0; i < mapSize; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *) (mmap + (i * descriptorSize));

        if (entry->Type == EfiConventionalMemory) {
            convMemory += entry->NumberOfPages * 4;
        } else if (entry->Type == EfiBootServicesCode || entry->Type == EfiBootServicesData) {
            reclaimMemory += entry->NumberOfPages * 4;
        } else {
            otherMemory += entry->NumberOfPages * 4;
        }
    }
    UINTN totalMemory = convMemory + reclaimMemory + otherMemory;

    printf("mmap reports %lld KiB of memory:\n", totalMemory);
    printf(" Free: %lld KiB (~%lld%%)\n", convMemory, convMemory * 100 / totalMemory);
    printf(" Reclaimable: %lld KiB (~%lld%%)\n", reclaimMemory, reclaimMemory * 100 / totalMemory);
    printf(" Reserved: %lld KiB (~%lld%%)\n", otherMemory, otherMemory * 100 / totalMemory);
    printf("Boot services terminated\n");

//    __asm__ volatile ("call *%0\n\t" :: "r" (header->entry));
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
        EFI_MEMORY_DESCRIPTOR *entry = mmap + (i * descriptorSize);
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
