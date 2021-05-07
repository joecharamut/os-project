#ifndef OS_PAGING_H
#define OS_PAGING_H

#include <std/types.h>
#include <stdbool.h>
#include <boot/interrupts.h>

typedef struct {
    bool present: 1;
    bool rw: 1;
    bool user: 1;
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

void init_paging();
void set_page_directory(page_directory_t *dir);
page_t *get_page(u32 address, bool create, page_directory_t *dir);
void page_fault_handler(registers_t registers);

#endif //OS_PAGING_H
