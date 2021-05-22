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
} __attribute__ ((packed)) page_table_entry_t;
typedef page_table_entry_t page_directory_entry_t;

typedef struct {
    page_table_entry_t pages[1024];
} page_table_t;

typedef struct {
    page_directory_entry_t tables[1024];
} page_directory_t;

void mmap(void *paddr, void *vaddr, bool readonly, bool user);
void munmap(void *vaddr);
bool is_paging_enabled();
void page_fault_handler(registers_t registers);

void flush_page(u32 addr);
void flush_tlb();

#endif //OS_PAGING_H
