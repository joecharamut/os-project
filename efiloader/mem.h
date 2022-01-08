#ifndef LOADER_MEM_H
#define LOADER_MEM_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <efi.h>

typedef struct {
    union {
        struct {
            bool present            :1;
            bool writeable          :1;
            bool user_access        :1;
            bool write_through      :1;
            bool cache_disabled     :1;
            bool accessed           :1;
            uint8_t ignored_3       :1;
            bool size               :1; // must be 0
            uint8_t ignored_2       :4;
            uint64_t page_ppn       :40;
            uint16_t ignored_1      :11;
            bool execution_disabled :1;
        } __attribute__ ((packed));
        uint64_t value;
    };
} pml4_entry_t;
static_assert(sizeof(pml4_entry_t) == 8, "Invalid Size");

typedef struct {
    union {
        struct {
            bool present            :1;
            bool writeable          :1;
            bool user_access        :1;
            bool write_through      :1;
            bool cache_disabled     :1;
            bool accessed           :1;
            uint8_t ignored_3       :1;
            bool size               :1;
            uint8_t ignored_2       :4;
            uint64_t page_ppn       :40;
            uint16_t ignored_1      :11;
            bool execution_disabled :1;
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
    MemoryTypeNull,
    MemoryTypeFree,
    MemoryTypeUsed,
    MemoryTypeReserved,
} memory_type_t;

typedef struct {
    memory_type_t type;
    UINT64 paddr;
    UINT64 vaddr;
    UINT64 pages;
    UINT64 flags;
} memory_map_entry_t;

extern const char * const EFI_MEMORY_TYPE_STRINGS[];
extern const char * const memory_type_t_strings[];

const char *efi_mem_type_string(UINT32 type);
EFI_MEMORY_DESCRIPTOR *efi_get_mem_map(UINTN *MapKey, UINTN *Entries, UINTN *DescriptorSize);
void efi_dump_mem_map(void *mmap, UINTN size, UINTN descriptorSize);
void *kmemcpy(void *dst, void *src, uint64_t num);
void *kmemset(void *ptr, unsigned char value, uint64_t num);

#endif //LOADER_MEM_H
