#include <debug/debug.h>
#include <debug/panic.h>
#include <std/types.h>
#include <std/stdlib.h>
#include <debug/term.h>
#include <std/bitset.h>
#include <debug/assert.h>
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

static void internal_map_page(void *physaddr, void *virtualaddr, bool readonly, bool user) {
    // Make sure that both addresses are page-aligned.
    assert((u32) physaddr % 0x1000 == 0);
    assert((u32) virtualaddr % 0x1000 == 0);

    u32 pdindex = (u32) virtualaddr >> 22;
    u32 ptindex = (u32) virtualaddr >> 12 & 0x03FF;

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

    page_table_t *pt = (page_table_t *) ((u32 *) 0xFFC00000 + (0x400 * pdindex));
    // Here you need to check whether the PT entry is present.
    // When it is, then there is already a mapping present. What do you do now?

    if (pt->pages[ptindex].present) {
        PANIC("Page already mapped!");
    }

    pt->pages[ptindex].address = (u32) physaddr >> 12;
    pt->pages[ptindex].present = true;
    pt->pages[ptindex].rw = !readonly;
    pt->pages[ptindex].user = user;

    flush_page((u32) virtualaddr);
}

static void internal_unmap_page(void *vaddr) {
    // Make sure that both addresses are page-aligned.
    assert((u32) vaddr % 0x1000 == 0);

    u32 pdindex = (u32) vaddr >> 22;
    u32 ptindex = (u32) vaddr >> 12 & 0x03FF;

    page_directory_t *pd = (page_directory_t *) 0xFFFFF000;
    // if page dir entry does not exist, exit
    if (!pd->tables[pdindex].present) {
        PANIC("munmap: Page isn't mapped!");
    }

    page_table_t *pt = (page_table_t *) ((u32 *) 0xFFC00000 + (0x400 * pdindex));
    // if the page isn't even mapped, exit
    if (!pt->pages[ptindex].present) {
        PANIC("munmap: Page isn't mapped!");
    }

    // mark the page as not there and flush its entry
    // todo: clear the PDE if it's empty
    pt->pages[ptindex] = (page_table_entry_t) { 0 };
    flush_page((u32) vaddr);
}

void mmap(void *paddr, void *vaddr, bool readonly, bool user) {
//    dbg_logf(LOG_DEBUG, "mapping 0x%08x to 0x%08x\n", paddr, vaddr);
    internal_map_page(paddr, vaddr, readonly, user);
}

void munmap(void *vaddr) {
    dbg_logf(LOG_DEBUG, "unmapping 0x%08x\n", vaddr);
    internal_unmap_page(vaddr);
}

bool is_paging_enabled() {
    u32 cr0;
    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    return cr0 & (1 << 31);
}

void page_fault_handler(registers_t registers) {
    u32 fault_addr;
    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    bool present = registers.error_code & 1 << 0;
    bool on_write = registers.error_code & 1 << 1;
    bool user = registers.error_code & 1 << 2;
    bool reserved = registers.error_code & 1 << 3;
    bool instruction_fetch = registers.error_code & 1 << 4;

    panic("Page Fault %s! (%s %s to %s page) at 0x%08x", &registers,
          (instruction_fetch ? "on instruction fetch" : "\b"),
          (user ? "user" : "supervisor"),
          (on_write ? "write" : "read"),
          (present ? "present" : "non-present"),
          fault_addr);
}

void flush_page(u32 addr) {
    asm volatile ("invlpg (%0)" :: "r" (addr) : "memory");
}

void flush_tlb() {
    asm volatile (
    "cli                \n"
    "mov %%cr3, %%eax   \n"
    "mov %%eax, %%cr3   \n"
    "sti                \n"
    :
    :
    : "%eax", "memory"
    );
}
