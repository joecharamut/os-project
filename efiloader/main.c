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

noreturn void halt() {
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

    status = serial_init();
    if (EFI_ERROR(status)) {
        return status;
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
    printf("UEFI Version %d.%d [Vendor: %ls, Revision: 0x%08X]\n", ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xFFFF, ST->FirmwareVendor, ST->FirmwareRevision);

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

    printf("Entrypoint is at 0x%08llx\n", header->entry);
    printf("Loading program headers...\n");
    UINTN pht_size = header->pht_entry_count * header->pht_entry_size;
    elf64_pht_entry_t *pht = AllocatePool(pht_size);
    kernelHandle->SetPosition(kernelHandle, header->pht_offset);
    kernelHandle->Read(kernelHandle, &pht_size, pht);

    for (uint16_t i = 0; i < header->pht_entry_count; ++i) {
        printf("Header %d:\n Offset: 0x%llx\n Virtual: 0x%llx\n Physical: 0x%llx",
               i
        );
    }

    UINTN mapKey = 0;
    UINTN mapSize = 0;
    EFI_MEMORY_DESCRIPTOR *mmap = efi_get_mem_map(&mapKey, &mapSize);
//    efi_dump_mem_map(mmap, mapSize);

    while (BS->ExitBootServices(ImageHandle, mapKey) == EFI_INVALID_PARAMETER) {
        FreePool(mmap);
        mmap = efi_get_mem_map(&mapKey, &mapSize);
    }

//    UINTN descriptorSize = 0;
//    UINT32 descriptorVersion = 0;
//    BS->GetMemoryMap(&mapSize, mmap, &mapKey, &descriptorSize, &descriptorVersion);
//
//    UINT64 convMem = 0;
//    for (UINTN i = 0; i < mapSize; ++i) {
//        if (mmap[i].Type == EfiConventionalMemory) {
//            if (mmap[i].NumberOfPages > convMem) {
//                convMem = mmap[i].NumberOfPages;
//            }
//        }
//    }

    halt();
    return status;
}


const CHAR16 *efi_mem_type_string(UINT32 type) {
    if (type < 15) {
        return EFI_MEMORY_TYPE_STRINGS[type];
    } else if (type < 0x6FFFFFFF) {
        return L"Reserved";
    } else if (type < 0x7FFFFFFF) {
        return L"Reserved (OEM)";
    } else {
        return L"Reserved (OSV)";
    }
}

EFI_MEMORY_DESCRIPTOR *efi_get_mem_map(UINTN *MapKey, UINTN *Entries) {
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

    return memoryMap;
}

void efi_dump_mem_map(EFI_MEMORY_DESCRIPTOR *mmap, UINTN size) {
    Print(L"Index,Type,Physical Address,Virtual Address,Pages,Attributes\n");
    for (UINTN i = 0; i < size; ++i) {
        Print(L"%lld,%s (%d),0x%08llx,0x%08llx,%lld,[%c%c%c] (0x%llx)\n",
              i, efi_mem_type_string(mmap[i].Type), mmap[i].Type,
              mmap[i].PhysicalStart, mmap[i].VirtualStart,
              mmap[i].NumberOfPages,

              (mmap[i].Attribute & EFI_MEMORY_RP) ? '-' : 'R',
              (mmap[i].Attribute & EFI_MEMORY_WP) ? '-' : 'W',
              (mmap[i].Attribute & EFI_MEMORY_XP) ? '-' : 'X',
              mmap[i].Attribute);
    }
}

void efi_print_mem_map(EFI_MEMORY_DESCRIPTOR *mmap, UINTN size) {
    for (UINTN i = 0; i < size; ++i) {
        Print(L" [%lld]: Type: %s, PhysAddr: 0x%llx, VirtAddr: 0x%llx, Pages: %lld, Attr: [%c%c%c] (0x%llx)\n",
              i,efi_mem_type_string(mmap[i].Type),
              mmap[i].PhysicalStart, mmap[i].VirtualStart,
              mmap[i].NumberOfPages,

              (mmap[i].Attribute & EFI_MEMORY_RP) ? '-' : 'R',
              (mmap[i].Attribute & EFI_MEMORY_WP) ? '-' : 'W',
              (mmap[i].Attribute & EFI_MEMORY_XP) ? '-' : 'X',
              mmap[i].Attribute);
    }
}

EFI_STATUS efi_exit() {
    return EFI_SUCCESS;
}
