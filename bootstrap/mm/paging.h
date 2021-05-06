#ifndef OS_PAGING_H
#define OS_PAGING_H

#include <bootstrap/types.h>
#include <stdbool.h>

typedef struct {
    bool present: 1;
    bool rw: 1;
    bool supervisor: 1;
    bool write_through: 1;
    bool cache_disabled: 1;
    bool accessed: 1;
    bool dirty: 1;
    u8 _padding: 1;
    bool global: 1;
    u8 os: 3;
    u32 address: 20;
} __attribute__ ((packed)) page_t;

typedef struct {
    page_t pages[1024];
} page_table_t;

typedef struct {
    page_table_t *tables[1024];
    phys_addr_t tablesPhysical[1024];
    phys_addr_t physicalAddr;
} page_directory_t;

typedef union {
    page_t page;
    u32 value;
} page_table_entry_t;
typedef page_table_entry_t page_directory_entry_t;

void paging_main_init(void);
void map_page(phys_addr_t phys_addr, virt_addr_t vaddr, bool kernel, bool writeable);
void unmap_page(virt_addr_t vaddr);

#endif //OS_PAGING_H
