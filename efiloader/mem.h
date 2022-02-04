#ifndef LOADER_MEM_H
#define LOADER_MEM_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <efi.h>

typedef union {
    uint64_t value;
    struct {
        uint64_t offset: 12;
        uint64_t pt_index: 9;
        uint64_t pd_index: 9;
        uint64_t pdpt_index: 9;
        uint64_t pml4_index: 9;
        uint64_t reserved: 16;
    };
} address_t;
typedef address_t virtual_address_t;
typedef address_t physical_address_t;

typedef struct {
    union {
        uint64_t value;
        struct {
            uint64_t present: 1;
            uint64_t writeable: 1;
            uint64_t user_access: 1;
            uint64_t write_through: 1;
            uint64_t cache_disabled: 1;
            uint64_t accessed: 1;
            uint64_t ignored_3: 1;
            uint64_t size: 1; // must be 0
            uint64_t ignored_2: 4;
            uint64_t page_ppn: 40;
            uint64_t ignored_1: 11;
            uint64_t execution_disabled: 1;
        } __attribute__((packed));
    };
} pml4_entry_t;
static_assert(sizeof(pml4_entry_t) == 8, "sizeof(pml4_entry_t) != 8");

typedef struct {
    union {
        struct {
            uint64_t present            :1;
            uint64_t writeable          :1;
            uint64_t user_access        :1;
            uint64_t write_through      :1;
            uint64_t cache_disabled     :1;
            uint64_t accessed           :1;
            uint64_t ignored_3          :1;
            uint64_t size               :1;
            uint64_t ignored_2          :4;
            uint64_t page_ppn           :40;
            uint64_t ignored_1          :11;
            uint64_t execution_disabled :1;
        } __attribute__ ((packed));
        uint64_t value;
    };
} pdpt_entry_t;
static_assert(sizeof(pdpt_entry_t) == 8, "Invalid Size");

typedef struct {
    union {
        struct {
            uint64_t present             :1;
            uint64_t writeable           :1;
            uint64_t user_access         :1;
            uint64_t write_through       :1;
            uint64_t cache_disabled      :1;
            uint64_t accessed            :1;
            uint64_t ignored_3           :1;
            uint64_t size                :1;
            uint64_t ignored_2           :4;
            uint64_t page_ppn            :28;
            uint64_t reserved_1          :12; // must be 0
            uint64_t ignored_1           :11;
            uint64_t execution_disabled  :1;
        } __attribute__ ((packed));
        uint64_t value;
    };
} pd_entry_t;
static_assert(sizeof(pd_entry_t) == 8, "Invalid Size");

typedef struct {
    union {
        struct {
            uint64_t present             :1;
            uint64_t writeable           :1;
            uint64_t user_access         :1;
            uint64_t write_through       :1;
            uint64_t cache_disabled      :1;
            uint64_t accessed            :1;
            uint64_t dirty               :1;
            uint64_t size                :1;
            uint64_t global              :1;
            uint64_t ignored_2           :3;
            uint64_t page_ppn            :28;
            uint64_t reserved_1          :12; // must be 0
            uint64_t ignored_1           :11;
            uint64_t execution_disabled  :1;
        } __attribute__ ((packed));
        uint64_t value;
    };
} pt_entry_t;
static_assert(sizeof(pt_entry_t) == 8, "Invalid Size");

typedef enum {
    PageSize4KiB,
    PageSize2MiB,
    PageSize1GiB,
} page_size_t;

extern const char * const EFI_MEMORY_TYPE_STRINGS[];
extern pml4_entry_t *pml4_pointer;

const char *efi_mem_type_string(UINT32 type);
const CHAR16 *efi_mem_type_wstring(UINT32 type);
EFI_MEMORY_DESCRIPTOR *efi_get_mem_map(UINTN *MapKey, UINTN *Entries, UINTN *DescriptorSize);
void efi_dump_mem_map_to_file(void *mmap, UINTN size, UINTN descriptorSize, EFI_FILE_HANDLE volume);
void efi_dump_mem_map(void *mmap, UINTN size, UINTN descriptorSize);

int lmemcmp(const void *ptr1, const void *ptr2, uint64_t num);
void *lmemcpy(void *dst, void *src, uint64_t num);
void *lmemset(void *ptr, unsigned char value, uint64_t num);
void *allocate(uint64_t size);

void poke_pml4();
void map_page(physical_address_t paddr, virtual_address_t vaddr, page_size_t size);

#endif //LOADER_MEM_H
