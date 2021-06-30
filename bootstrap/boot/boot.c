#include <debug/serial.h>
#include <debug/term.h>
#include <debug/debug.h>
#include <mm/paging.h>
#include <kernel/kernel.h>
#include <mm/kmem.h>
#include <std/string.h>
#include "interrupts.h"
#include "multiboot.h"
#include "gdt.h"
#include <stdnoreturn.h>
#include <debug/assert.h>
#include <io/timer.h>
#include <std/registers.h>

void int3_handler(interrupt_registers_t regs) {
    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
    dbg_printf("BREAKPOINT HIT: \"%s\"\n", regs.edx);
    dump_registers(&regs);
}

#define MEMSET(ptr, val, count) asm volatile ("cld ; rep stosb" :: "D" (ptr), "a" (val), "c" (count) : "memory")
#define PAGE_CEIL(addr) ((addr) & 0x00000FFF ? (((addr) & 0xFFFFF000) + 0x1000) : (addr))
#define PAGE_FLOOR(addr) ((addr) & 0x00000FFF ? (((addr) & 0xFFFFF000)) : (addr))

noreturn void boot_entrypoint(u32 multiboot_magic, multiboot_info_t *multiboot_info);
noreturn static void boot_main(multiboot_info_t *multiboot_info);
noreturn static void boot_final();

extern u32 boot_base_addr;
extern u32 boot_end_addr;
extern u32 kernel_code_base;
extern u32 kernel_code_end;
extern u32 kernel_data_base;
extern u32 kernel_data_end;

static void __attribute__((section(".boot_text"))) early_page_map(page_directory_t *page_dir, u32 physical_addr, u32 virtual_addr, bool writeable) {
    u32 dir_index = virtual_addr >> 22;
    u32 table_index = (virtual_addr >> 12) & 0x3FF;

    // allocate if not there
    if (!page_dir->tables[dir_index].present) {
        page_table_t *table = (void *) (PAGE_TABLE_BASE + sizeof(page_directory_t) + (dir_index * sizeof(page_table_t)));
        MEMSET(table, 0, sizeof(page_table_t));

        page_dir->tables[dir_index].address = (u32) table >> 12;
        page_dir->tables[dir_index].present = true;
        page_dir->tables[dir_index].rw = true;
    }

    page_table_t *table = (page_table_t *) ((u32) page_dir->tables[dir_index].address << 12);
    table->pages[table_index].address = physical_addr >> 12;
    table->pages[table_index].present = true;
    table->pages[table_index].rw = writeable;
}

const char multiboot_error[] __attribute__((section(".boot_data"))) = "BOOT ERROR: Not loaded with a Multiboot 1 compatible bootloader.";
noreturn void __attribute__((used, section(".boot_text"))) boot_entrypoint(u32 multiboot_magic, multiboot_info_t *multiboot_info) {
    uint16_t *term_buffer = (uint16_t *) 0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            term_buffer[y * VGA_WIDTH + x] = VGA_ENTRY(' ', VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        }
    }

    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        int i = 0;
        while (multiboot_error[i]) {
            term_buffer[i] = VGA_ENTRY(multiboot_error[i], VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            ++i;
        }

        asm volatile ("hlt; jmp .");
    }

    u32 page_table_base = PAGE_TABLE_BASE;

    page_directory_t *kernel_page_dir = (page_directory_t *) page_table_base;
    MEMSET(kernel_page_dir, 0, sizeof(page_directory_t));
    page_table_base += sizeof(page_directory_t);
    MEMSET(page_table_base, 0, sizeof(page_table_t) * 1024);
    page_table_base += sizeof(page_table_t) * 1024;

    // set last page directory entry to the page directory itself
    // this lets the kernel paging code change the entries while paging is enabled
    kernel_page_dir->tables[1023].address = (u32) kernel_page_dir >> 12;
    kernel_page_dir->tables[1023].present = true;
    kernel_page_dir->tables[1023].rw = true;

    // identity map first mb (for display and stuff)
    for (int i = 0; i < 0x100000; i += 0x1000) {
        early_page_map(kernel_page_dir, i, i, true);
    }

    // identity map the bootcode
    for (u32 i = PAGE_FLOOR((u32) &boot_base_addr); i < PAGE_CEIL((u32) &boot_end_addr); i += 0x1000) {
        early_page_map(kernel_page_dir, i, i, true);
    }

    // map .text and .rodata as read-only
    for (u32 i = PAGE_FLOOR((u32) &kernel_code_base); i < PAGE_CEIL((u32) &kernel_code_end); i += 0x1000) {
        early_page_map(kernel_page_dir, i - KERNEL_BASE_ADDR, i, false);
    }

    // map .data and .bss as read-write
    for (u32 i = PAGE_FLOOR((u32) &kernel_data_base); i < PAGE_CEIL((u32) &kernel_data_end); i += 0x1000) {
        early_page_map(kernel_page_dir, i - KERNEL_BASE_ADDR, i, true);
    }

    // load cr3 with the page table
    asm volatile ("mov %0, %%cr3" :: "r" (kernel_page_dir));

    // enable paging bit in cr0
    volatile u32 cr0;
    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 1 << 31;
    asm volatile ("mov %0, %%cr0" :: "r" (cr0));

    asm volatile (
    "push %0    \n" // push the multiboot info
    "call *%1   \n" // absolute jump to higher half
    :
    : "r" (multiboot_info), "r" (&boot_main)
    : "%eax"
    );
    __builtin_unreachable();
}

noreturn static void boot_main(multiboot_info_t *multiboot_info) {
    serial_init();
    term_init();

    // init timer for 1ms
    init_timer(1000);

    dbg_logf(LOG_INFO, "Hello World!\n");

    u32 eip;
    asm volatile ("mov $., %0" : "=r" (eip));
    dbg_logf(LOG_DEBUG, "Hello higher-half, we're at 0x%08x now!\n", eip);

    init_gdt();
    init_interrupts();
    set_interrupt_handler(3, int3_handler);
    set_interrupt_handler(14, page_fault_handler);
    init_kmem();

    extern u32 boot_stack_top;
    extern u32 boot_stack_bottom;
    u32 old_stack_size = (u32) &boot_stack_bottom - (u32) &boot_stack_top;

    void *new_stack = kmalloc(0x1000);
    void *old_stack = &boot_stack_top;
    memcpy(new_stack, old_stack, old_stack_size);

    asm volatile (
    "mov %%esp, %%eax       \n" // copy stack pointer to eax
    "sub %0, %%eax          \n" // get offset from old_stack
    "add %1, %%eax          \n" // add offset to new_stack
    "mov %%eax, %%esp       \n" // save new stack pointer
    ""
    "mov %%ebp, %%eax       \n" // copy base pointer to eax
    "sub %0, %%eax          \n" // get offset from old_stack
    "add %1, %%eax          \n" // add offset to new_stack
    "mov %%eax, %%ebp       \n" // save new base pointer
    ""
    "jmp *%2                \n" // absolute jump to boot_final
    :
    : "r" (old_stack), "r" (new_stack), "r" (&boot_final)
    : "memory", "%eax"
    );
    __builtin_unreachable();
}

static void unmap_bootcode() {
    for (u32 i = PAGE_FLOOR((u32) &boot_base_addr); i < PAGE_CEIL((u32) &boot_end_addr); i += 0x1000) {
        munmap((void *) i);
    }
}

noreturn static void boot_final() {
    unmap_bootcode();
    kernel_main();
    PANIC("Kernel Returned");
}
