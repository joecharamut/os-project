#include <debug/serial.h>
#include <debug/term.h>
#include <debug/debug.h>
#include <io/port.h>
#include <io/timer.h>
#include <mm/paging.h>
#include <kernel/kernel.h>
#include <std/attributes.h>
#include <mm/kmem.h>
#include <io/ps2.h>
#include <std/string.h>
#include "interrupts.h"
#include "multiboot.h"
#include "gdt.h"

void irq0_handler(registers_t regs) { }

void int3_handler(registers_t regs) {
    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
    dbg_printf("BREAKPOINT HIT: \"%s\"\n", regs.edx);
    dump_registers(&regs);
}

#define __MEMSET(ptr, value, count) do { \
    for (u32 i = 0; i < count; i++) { \
        ((u8 *) ptr)[i] = value; \
    } \
} while (0)

#define PAGE_ROUNDED(x) (((x) & 0x00000FFF) ? ((x) & 0xFFFFF000) + 0x1000 : ((x) & 0xFFFFF000))

page_directory_t *kernel_page_dir SECTION(".boot_data");

static void SECTION(".boot_text") _early_page_map(u32 physical_addr, u32 virtual_addr, bool writeable) {
    int dir_index = virtual_addr >> 22;
    int table_index = (virtual_addr >> 12) & 0x3FF;

    // allocate if not there
    if (!kernel_page_dir->tables[dir_index].present) {
        page_table_t *table = PAGE_TABLE_BASE + sizeof(page_directory_t) + (dir_index * sizeof(page_table_t));
        __MEMSET(table, 0, sizeof(page_table_t));

        kernel_page_dir->tables[dir_index].address = (u32) table >> 12;
        kernel_page_dir->tables[dir_index].present = true;
        kernel_page_dir->tables[dir_index].rw = true;
    }

    page_table_t *table = (page_table_t *) ((u32) kernel_page_dir->tables[dir_index].address << 12);
    table->pages[table_index].address = physical_addr >> 12;
    table->pages[table_index].present = true;
    table->pages[table_index].rw = writeable;
}

void USED NORETURN SECTION(".boot_text") _early_boot(u32 multiboot_magic, multiboot_info_t *multiboot_info) {
    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        asm volatile ("hlt");
    }

    __MEMSET(PAGE_TABLE_BASE, 0, 0x400000);

    kernel_page_dir = (page_directory_t *) PAGE_TABLE_BASE;
    __MEMSET(kernel_page_dir, 0, sizeof(page_directory_t));

    // set last page directory entry to the page directory itself
    // this lets the kernel paging code change the entries while paging is enabled
    kernel_page_dir->tables[1023].address = (u32) kernel_page_dir >> 12;
    kernel_page_dir->tables[1023].present = true;
    kernel_page_dir->tables[1023].rw = true;

    extern u32 _boot_base;
    extern u32 _boot_end;
    extern u32 _kernel_code_base;
    extern u32 _kernel_code_end;
    extern u32 _kernel_data_base;
    extern u32 _kernel_data_end;

    u32 boot_base = (u32) &_boot_base;
    u32 boot_end = (u32) &_boot_end;
    u32 kernel_code_start = (u32) &_kernel_code_base;
    u32 kernel_code_end = (u32) &_kernel_code_end;
    u32 kernel_data_start = (u32) &_kernel_data_base;
    u32 kernel_data_end = (u32) &_kernel_data_end;

    // identity map first mb (for display and stuff)
    for (int i = 0; i < 0x100000; i += 0x1000) {
        _early_page_map(i, i, true);
    }

    // identity map the bootcode
    for (u32 i = boot_base; i < boot_end; i += 0x1000) {
        _early_page_map(i, i, true);
    }

    // map kernel .text and .rodata as read-only
    for (u32 i = kernel_code_start; i < kernel_code_end; i += 0x1000) {
        _early_page_map(i - KERNEL_ADDR_BASE, i, false);
    }

    // map .data and .bss as read-write
    for (u32 i = kernel_data_start; i < kernel_data_end; i += 0x1000) {
        _early_page_map(i - KERNEL_ADDR_BASE, i, true);
    }

    // load cr3 with the page table
    asm volatile ("mov %0, %%cr3" :: "r" (kernel_page_dir));

    // enable paging bit in cr0
    volatile u32 cr0;
    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 1 << 31;
    asm volatile ("mov %0, %%cr0" :: "r" (cr0));

    // jmp to higher half code
    asm volatile (
    "lea _boot, %%eax  \n"
    "jmp *%%eax        \n"
    :
    :
    : "%eax"
    );

    __builtin_unreachable();
}

static void USED NORETURN _boot() {
    serial_init();
    term_init();

    u32 eip;
    asm volatile ("mov $., %0" : "=r" (eip));
    dbg_logf(LOG_INFO, "Hello higher-half, we're at 0x%08x now!\n", eip);

    init_gdt();
    init_interrupts();
    set_interrupt_handler(IRQ0, irq0_handler);
    set_interrupt_handler(3, int3_handler);
    init_paging();
    init_ps2();

    // todo setup kernel stack
    void *new_stack = kmalloc(0x1000);

    extern u32 boot_stack_top;
    void *old_stack = &boot_stack_top;

    volatile void *current_esp;
    asm volatile ("mov %%esp, %0" : "=r" (current_esp));

    u32 stack_offset = (u32) current_esp - (u32) old_stack;

    dbg_logf(LOG_DEBUG, "old stack: 0x%08x, new stack: 0x%08x, stack offset: 0x%08x\n", old_stack, new_stack, stack_offset);
    memcpy(new_stack, old_stack, 0x1000);
    new_stack = (void *) ((u32) new_stack + stack_offset);

    asm volatile (
    "mov %0, %%esp"
    : // no output
    : "r" (new_stack) // new stack ptr
    : "memory", "%esp"
    );

    kernel_main();

    dbg_printf("System Halted.");
    asm volatile ("1: cli; hlt; jmp 1");
    __builtin_unreachable();
}
