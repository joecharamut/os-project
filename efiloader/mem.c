#include "mem.h"
#include "debug.h"

#include <efilib.h>

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

const char * const memory_type_t_strings[] = {
        "MemoryTypeNull",
        "MemoryTypeFree",
        "MemoryTypeUsed",
        "MemoryTypeReserved",
};

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
        printf("%s%lld,%s (%lld),0x%08llx,0x%08llx,%lld (%lld KiB),[%c%c%c] (0x%llx)\n",
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

void *kmemcpy(void *dst, void *src, uint64_t num) {
    for (uint64_t i = 0; i < num; ++i) {
        ((char*) dst)[i] = ((char*) src)[i];
    }
    return dst;
}

void *kmemset(void *ptr, unsigned char value, uint64_t num) {
    for (uint64_t i = 0; i < num; ++i) {
        *((unsigned char *) ptr + i) = value;
    }
    return ptr;
}

