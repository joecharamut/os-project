#include <debug/debug.h>
#include <debug/panic.h>
#include <std/types.h>
#include <std/stdlib.h>
#include <debug/term.h>
#include <std/bitset.h>
#include "paging.h"
#include "kmem.h"

bitset_t *page_set;

void *get_physaddr(void *virtualaddr) {
    u32 pdindex = (u32) virtualaddr >> 22;
    u32 ptindex = (u32) virtualaddr >> 12 & 0x03FF;

    u32 *pd = (u32 *) 0xFFFFF000;

    // if page dir entry does not exist, return -1
    if (!(pd[pdindex] & 1)) {
        return (void *) 0xFFFFFFFF;
    }

    u32 *pt = ((u32 *) 0xFFC00000) + (0x400 * pdindex);
    // Here you need to check whether the PT entry is present.

    return (void *)((pt[ptindex] & ~0xFFF) + ((unsigned long)virtualaddr & 0xFFF));
}

void map_page(void *physaddr, void *virtualaddr) {
    // Make sure that both addresses are page-aligned.

    u32 pdindex = (u32) virtualaddr >> 22;
    u32 ptindex = (u32) virtualaddr >> 12 & 0x03FF;

    dbg_logf(LOG_DEBUG, "mapping 0x%08x to 0x%08x (dir %d table %d)\n", physaddr, virtualaddr, pdindex, ptindex);

    page_directory_t *pd = (page_directory_t *) 0xFFFFF000;
    // Here you need to check whether the PD entry is present.
    // When it is not present, you need to create a new empty PT and
    // adjust the PDE accordingly.
    // if page dir entry does not exist, create one
    if (!pd->tables[pdindex].present) {
        pd->tables[pdindex].address = (u32) (0x01000000 + pdindex*0x1000) >> 12;
        pd->tables[pdindex].present = true;
        pd->tables[pdindex].rw = true;
    }

    page_table_t *pt = ((u32 *) 0xFFC00000 + (0x400 * pdindex));
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    if (pt->pages[ptindex].present) {
        PANIC("Page already mapped!");
    }

    pt->pages[ptindex].address = (u32) physaddr >> 12;
    pt->pages[ptindex].present = true;
    pt->pages[ptindex].rw = true;
//    flush_tlb();
}

void init_paging() {
    set_interrupt_handler(14, &page_fault_handler);

    u32 heap_vaddr_base = (1023 - (HEAP_SIZE >> 22)) << 22;
    for (u32 offset = 0; offset < HEAP_SIZE; offset += 0x1000) {
        map_page(HEAP_BASE + offset, heap_vaddr_base + offset);
    }
    set_heap_address(heap_vaddr_base);

    flush_tlb();
}

void page_fault_handler(registers_t registers) {
    u32 fault_addr;
    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    bool present = !(registers.error_code & 1 << 0);
    bool on_write = registers.error_code & 1 << 1;
    bool user = registers.error_code & 1 << 2;
    bool reserved = registers.error_code & 1 << 3;
    bool instruction_fetch = registers.error_code & 1 << 4;

    panic("Page Fault %s! ( %s%s%s) at 0x%08x", &registers,
               (instruction_fetch ? "on instruction fetch" : (on_write ? "on write" : "on read")),
               (present ? "present " : ""),
               (user ? "user-mode " : ""),
               (reserved ? "reserved " : ""),
               fault_addr);
}

void flush_tlb() {
    asm volatile (
    "cli                \n"
    "mov %%cr3, %%eax   \n"
    "mov %%eax, %%cr3   \n"
    "sti                \n"
    :
    :
    : "%eax"
    );
}
