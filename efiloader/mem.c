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

const CHAR16 * const EFI_MEMORY_TYPE_WSTRINGS[] = {
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

const CHAR16 *efi_mem_type_wstring(UINT32 type) {
    if (type < 15) {
        return EFI_MEMORY_TYPE_WSTRINGS[type];
    } else if (type < 0x6FFFFFFF) {
        return L"Reserved";
    } else if (type < 0x7FFFFFFF) {
        return L"Reserved (OEM)";
    } else {
        return L"Reserved (OSV)";
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
    dbg_print("Index,Type,Physical Address,Virtual Address,Pages,Attributes\n");
    for (UINTN i = 0; i < size; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (void *) (((char *) mmap) + (i * descriptorSize));
        dbg_print("%s%lld,%s (%lld),0x%08llx,0x%08llx,%lld (%lld KiB),[%c%c%c] (0x%llx)\n",
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

void efi_dump_mem_map_to_file(void *mmap, UINTN size, UINTN descriptorSize, EFI_FILE_HANDLE volume) {
    EFI_FILE_HANDLE dumpFile;
    EFI_STATUS status;

    status = volume->Open(volume, &dumpFile, L"\\MMAP.CSV", EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 0);
    if (status == EFI_SUCCESS) {
        dumpFile->Delete(dumpFile);
    }

    status = volume->Open(volume, &dumpFile, L"\\MMAP.CSV", EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 0);
    if (EFI_ERROR(status)) {
        return;
    }

    const CHAR16 header[] = L"Index,Type,Physical Address,Virtual Address,Pages,Attributes\n";
    uint64_t headerSize = sizeof(header) - 2;
    dumpFile->Write(dumpFile, &headerSize, (void *) header);

    for (UINTN i = 0; i < size; ++i) {
        EFI_MEMORY_DESCRIPTOR *entry = (void *) (((char *) mmap) + (i * descriptorSize));

        CHAR16 str[256];
        uint64_t len = UnicodeSPrint(str, 256, L"%lld,%s (%lld),0x%08llx,0x%08llx,%lld (%lld KiB),[%c%c%c] (0x%llx)\n",
                  i, efi_mem_type_wstring(entry->Type), entry->Type,
                  entry->PhysicalStart, entry->VirtualStart,
                  entry->NumberOfPages, entry->NumberOfPages * 4,

                  (entry->Attribute & EFI_MEMORY_RP) ? '-' : 'R',
                  (entry->Attribute & EFI_MEMORY_WP) ? '-' : 'W',
                  (entry->Attribute & EFI_MEMORY_XP) ? '-' : 'X',
                  entry->Attribute);
        len = (len - 1) * sizeof(CHAR16);

        dumpFile->Write(dumpFile, &len, str);
    }

    dumpFile->Close(dumpFile);
}

int lmemcmp(const void *ptr1, const void *ptr2, uint64_t num) {
    const unsigned char *p1 = ptr1;
    const unsigned char *p2 = ptr2;
    unsigned char tmp;

    for (uint64_t i = 0; i < num; ++i) {
        tmp = p1[i] - p2[i];
        if (tmp) return tmp;
    }

    return 0;
}

void *lmemcpy(void *dst, void *src, uint64_t num) {
    for (uint64_t i = 0; i < num; ++i) {
        ((char*) dst)[i] = ((char*) src)[i];
    }
    return dst;
}

void *lmemset(void *ptr, unsigned char value, uint64_t num) {
    for (uint64_t i = 0; i < num; ++i) {
        *((unsigned char *) ptr + i) = value;
    }
    return ptr;
}

static uint64_t lomem_allocate_ptr = 0x100000; // +1MiB
void *allocate(uint64_t size) {
    void *ret = (void *) lomem_allocate_ptr;
    lmemset(ret, 0, size);

    lomem_allocate_ptr += size;
    if (lomem_allocate_ptr & 0xFFF) {
        lomem_allocate_ptr &= ~0xFFF;
        lomem_allocate_ptr += 0x1000;
    }

    return ret;
}


pml4_entry_t *pml4_pointer = NULL;

void load_page_map() {
    __asm__ volatile (
            "mov %%rax, %%cr3\n\t"
            :
            : "a" (pml4_pointer)
            :
    );
}

void map_page(physical_address_t paddr, virtual_address_t vaddr, page_size_t size) {
    if (!pml4_pointer) {
        pml4_pointer = allocate(sizeof(pml4_entry_t) * 512);
        dbg_print("allocating new pml4 at 0x%016llx\n", pml4_pointer);
        lmemset(pml4_pointer, 0, sizeof(pml4_entry_t) * 512);
    }

    if (!pml4_pointer[vaddr.pml4_index].present) {
        pml4_pointer[vaddr.pml4_index] = (pml4_entry_t) {
                .present = true,
                .writeable = true,
                .user_access = false,
                .write_through = true,
                .cache_disabled = true,
                .accessed = false,
                .size = 0,
                .page_ppn = 0,
                .execution_disabled = false,
        };
    }

    pdpt_entry_t *pdpt = (pdpt_entry_t *) (pml4_pointer[vaddr.pml4_index].page_ppn * 0x1000);
    if (!pdpt) {
        pdpt = allocate(sizeof(pdpt_entry_t) * 512);
        dbg_print("allocating new pdpt at 0x%016llx\n", pdpt);
        lmemset(pdpt, 0, sizeof(pdpt_entry_t) * 512);
        pml4_pointer[vaddr.pml4_index].page_ppn = ((uint64_t) pdpt) / 0x1000;
    }

    if (!pdpt[vaddr.pdpt_index].present) {
        pdpt[vaddr.pdpt_index] = (pdpt_entry_t) {
                .present = true,
                .writeable = true,
                .user_access = false,
                .write_through = true,
                .cache_disabled = true,
                .accessed = false,
                .size = 0,
                .page_ppn = 0,
                .execution_disabled = false,
        };
    }

    if (size == PageSize1GiB) {
        // 1GiB = PML4->PDPT
        pdpt[vaddr.pdpt_index].size = 1;
        pdpt[vaddr.pdpt_index].page_ppn = paddr.value / 0x1000;
        return;
    }

    pd_entry_t *pd = (pd_entry_t *) (pdpt[vaddr.pdpt_index].page_ppn * 0x1000);
    if (!pd) {
        pd = allocate(sizeof(pd_entry_t) * 512);
        dbg_print("allocating new pd at 0x%016llx\n", pd);
        lmemset(pd, 0, sizeof(pd_entry_t) * 512);
        pdpt[vaddr.pdpt_index].page_ppn = ((uint64_t) pd) / 0x1000;
    }

    if (!pd[vaddr.pd_index].present) {
        pd[vaddr.pd_index] = (pd_entry_t) {
                .present = true,
                .writeable = true,
                .user_access = false,
                .write_through = true,
                .cache_disabled = true,
                .accessed = false,
                .size = 0,
                .page_ppn = 0,
                .execution_disabled = false,
        };
    }

    if (size == PageSize2MiB) {
        // 2MiB = PML4->PDPT->PD
        pd[vaddr.pd_index].size = 1;
        pd[vaddr.pd_index].page_ppn = paddr.value / 0x1000;
        return;
    }

    pt_entry_t *pt = (pt_entry_t *) (pd[vaddr.pd_index].page_ppn * 0x1000);
    if (!pt) {
        pt = allocate(sizeof(pt_entry_t) * 512);
        dbg_print("allocating new pt at 0x%016llx\n", pt);
        lmemset(pt, 0, sizeof(pt_entry_t) * 512);
        pd[vaddr.pd_index].page_ppn = ((uint64_t) pt) / 0x1000;
    }

    if (!pt[vaddr.pt_index].present) {
        pt[vaddr.pt_index] = (pt_entry_t) {
                .present = true,
                .writeable = true,
                .user_access = false,
                .write_through = true,
                .cache_disabled = true,
                .accessed = false,
                .size = 0,
                .page_ppn = paddr.value / 0x1000,
                .execution_disabled = false,
        };
    }

    if (size == PageSize4KiB) {
        // 4KiB = PML4->PDPT->PD->PT
    }
}

