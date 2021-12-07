#include "main.h"

#include <efi.h>
#include <efilib.h>
#include "elf.h"
#include "file.h"

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // init gnu-efi
    InitializeLib(ImageHandle, SystemTable);

    // disable watchdog
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    ST->ConOut->ClearScreen(ST->ConOut);
    Print(L"Hello UEFI World!\n");
    Print(L"UEFI Version %d.%d [Vendor: %s, Revision: 0x%08X]\n", ST->Hdr.Revision >> 16, ST->Hdr.Revision & 0xFFFF, ST->FirmwareVendor, ST->FirmwareRevision);

    uint64_t rip = 0xCCCCCCCCCCCCCCCC;
    __asm__ ("1: lea 1b(%%rip), %0\n\t" : "=a" (rip));
    Print(L"rip = 0x%016x\n", rip);

    EFI_STATUS status;
    EFI_FILE_HANDLE volume = GetVolume(ImageHandle);
    if (!volume) {
        return EFI_DEVICE_ERROR;
    }

    EFI_FILE_HANDLE kernelHandle = OpenFile(volume, L"KERNEL.BIN");
    if (!kernelHandle) {
        return EFI_NOT_FOUND;
    }

    Print(L"Loading Kernel Header...");
    UINTN bufsz = 64;
    void *buf = AllocatePool(bufsz);
    status = kernelHandle->Read(kernelHandle, &bufsz, buf);
    if (EFI_ERROR(status)) {
        Print(L"Error\n");
        return status;
    }
    Print(L"OK!\n");

    if (!elf_is_header_valid(buf)) {
        Print(L"Invalid ELF Header\n");
        return EFI_UNSUPPORTED;
    }

    if (!elf_is_64_bit(buf)) {
        Print(L"Unsupported ELF Format\n");
        return EFI_UNSUPPORTED;
    }

    Print(L"Parsing Kernel Header...\n");
    elf64_header_t *header = buf;

    if (header->type != ELF_TYPE_EXEC) {
        Print(L"Unsupported ELF Object Type\n");
        return EFI_UNSUPPORTED;
    }

    if (header->instruction_set != 0x3E) {
        Print(L"Unsupported ELF Instruction Set\n");
        return EFI_UNSUPPORTED;
    }

    Print(L"Entrypoint is at 0x%08x\n", header->entry);

    EFI_INPUT_KEY key;
    Print(L"Press any key to continue...\n");
    status = ST->ConIn->Reset(ST->ConIn, FALSE);
    if (EFI_ERROR(status)) {
        return status;
    }
    while ((status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key)) == EFI_NOT_READY) {
        // do nothing
    }

    UINTN mapKey = 0;
    UINTN mapSize = 0;
    EFI_MEMORY_DESCRIPTOR *mmap = efi_get_mem_map(&mapKey, &mapSize);
    efi_dump_mem_map(mmap, mapSize);

    while (BS->ExitBootServices(ImageHandle, mapKey) == EFI_INVALID_PARAMETER) {
        FreePool(mmap);
        mmap = efi_get_mem_map(&mapKey, &mapSize);
    }

    UINTN descriptorSize = 0;
    UINT32 descriptorVersion = 0;
    BS->GetMemoryMap(&mapSize, mmap, &mapKey, &descriptorSize, &descriptorVersion);

    UINT64 convMem = 0;
    for (UINTN i = 0; i < mapSize; ++i) {
        if (mmap[i].Type == EfiConventionalMemory) {
            if (mmap[i].NumberOfPages > convMem) {
                convMem = mmap[i].NumberOfPages;
            }
        }
    }

    __asm__ ("mov %0, %%r12\t\n"
             "cli\t\n"
             "hlt\t\n"
             "jmp .\t\n"
             :: "a" (convMem));

    return status;
}

const CHAR16 *UEFI_MEMORY_TYPES[] = {
        L"EfiReservedMemoryType",
        L"EfiLoaderCode",
        L"EfiLoaderData",
        L"EfiBootServicesCode",
        L"EfiBootServicesData",
        L"EfiRuntimeServicesCode",
        L"EfiRuntimeServicesData",
        L"EfiConventionalMemory",
        L"EfiUnusableMemory",
        L"EfiACPIReclaimMemory",
        L"EfiACPIMemoryNVS",
        L"EfiMemoryMappedIO",
        L"EfiMemoryMappedIOPortSpace",
        L"EfiPalCode",
        L"EfiMaxMemoryType"
};
const CHAR16 *efi_mem_type_string(UINT32 type) {
    if (type < 15) {
        return UEFI_MEMORY_TYPES[type];
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
