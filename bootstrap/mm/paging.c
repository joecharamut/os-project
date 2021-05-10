#include <debug/debug.h>
#include <debug/panic.h>
#include <std/types.h>
#include <std/stdlib.h>
#include <debug/term.h>
#include "paging.h"
#include "kmem.h"

u32 *frames;
u32 n_frames;

page_directory_t *kernel_directory;
page_directory_t *current_directory;

#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

#define FLAG_PRESENT    1 << 0
#define FLAG_RW         1 << 1
#define FLAG_USER       1 << 2


// Static function to set a bit in the frames bitset
static void set_frame(u32 frame_addr) {
    u32 frame = frame_addr / 0x1000;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
}

// Static function to clear a bit in the frames bitset
static void clear_frame(u32 frame_addr) {
    u32 frame = frame_addr / 0x1000;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off);
}

// Static function to test if a bit is set.
static u32 test_frame(u32 frame_addr) {
    u32 frame = frame_addr / 0x1000;
    u32 idx = INDEX_FROM_BIT(frame);
    u32 off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
}

// Static function to find the first free frame.
static u32 first_frame() {
    for (u32 i = 0; i < INDEX_FROM_BIT(n_frames); i++) {
        // nothing free, exit early.
        if (frames[i] != 0xFFFFFFFF) {
            // at least one bit is free here.
            for (u32 j = 0; j < 32; j++) {
                u32 test = 0x1 << j;
                if (!(frames[i] & test)) {
                    return i * 4 * 8 + j;
                }
            }
        }
    }

    PANIC("first_frame() end of func");
}

void allocate_frame(page_t *page, bool user_page, bool writeable) {
    if (page->address) {
        return;
    } else {
        u32 idx = first_frame();
        if (idx == 0xFFFFFFFF) {
            PANIC("No free frames!");
        }
        set_frame(idx * 0x1000);
        page->present = true;
        page->rw = writeable;
        page->user = user_page;
        page->address = idx;
    }
}

void free_frame(page_t *page) {
    u32 frame = page->address;
    if (frame) {
        clear_frame(frame);
        page->address = 0;
    }
}


page_t *get_page(u32 address, bool create, page_directory_t *dir) {
    // Turn the address into an index.
    address /= 0x1000;
    // Find the page table containing this address.
    u32 table_idx = address / 1024;
    if (dir->tables[table_idx]) {
        return &dir->tables[table_idx]->pages[address % 1024];
    } else if(create) {
        u32 tmp;
        dir->tables[table_idx] = (page_table_t*) kmalloc_physical_aligned(sizeof(page_table_t), &tmp);
        memset(dir->tables[table_idx], 0, 0x1000);
        dir->tablesPhysical[table_idx] = tmp | FLAG_PRESENT | FLAG_RW;
        return &dir->tables[table_idx]->pages[address % 1024];
    } else {
        return 0;
    }
}

void init_paging() {
    u32 mem_end_page = 0x1000000;
    n_frames = mem_end_page / 0x1000;
    frames = (u32*) kmalloc(INDEX_FROM_BIT(n_frames));
    memset(frames, 0, INDEX_FROM_BIT(n_frames));

    kernel_directory = (page_directory_t *) kmalloc_aligned(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));
    current_directory = kernel_directory;

    u32 i = 0;
    extern u32 heap_ptr;
    while (i < heap_ptr) {
        allocate_frame(get_page(i, 1, kernel_directory), false, false);
        i += 0x1000;
    }

    set_interrupt_handler(14, &page_fault_handler);
    set_page_directory(kernel_directory);
}

void page_fault_handler(registers_t registers) {
//    asm volatile ("cli;hlt");
    u32 fault_addr;
    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    bool present = !(registers.error_code & 1 << 0);
    bool on_write = registers.error_code & 1 << 1;
    bool user = registers.error_code & 1 << 2;
    bool reserved = registers.error_code & 1 << 3;
    bool instruction_fetch = registers.error_code & 1 << 4;

    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
    dbg_printf("Page Fault %s! ( %s%s%s) at 0x%0*x\n",
               (instruction_fetch ? "on instruction fetch" : (on_write ? "on write" : "on read")),
               (present ? "present " : ""),
               (user ? "user-mode " : ""),
               (reserved ? "reserved " : ""),
               8, fault_addr);

    panic("Page Fault", &registers);
}

void set_page_directory(page_directory_t *dir) {
    current_directory = dir;

    asm volatile ("mov %0, %%cr3" :: "r" (&dir->tablesPhysical));
    volatile u32 cr0;
    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 1 << 31; // enable paging
    asm volatile ("mov %0, %%cr0" :: "r" (cr0));
}
