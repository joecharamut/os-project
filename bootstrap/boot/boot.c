#include <debug/serial.h>
#include <debug/term.h>
#include <debug/debug.h>
#include <io/port.h>
#include <io/timer.h>
#include <dev/pci.h>
#include <dev/ide.h>
#include <mm/paging.h>
#include <std/string.h>
#include <std/stdlib.h>
#include <debug/panic.h>
#include <kernel/kernel.h>
#include "gdt.h"
#include "interrupts.h"

#define SECTION(x) __attribute__((section(x)))
#define ALIGN(x) __attribute__((aligned(x)))
#define NORETURN __attribute__((noreturn))
#define UNUSED __attribute__((unused))
#define USED __attribute__((used))

volatile bool keypress_flag = false;

void irq0_handler(registers_t regs) { }

void irq1_handler(registers_t regs) {
    int scancode = inb(0x60);
    int i = inb(0x61);
    outb(0x61, i | 0x80);
    outb(0x61, i);
    dbg_logf(LOG_DEBUG, "scancode: %x\n", scancode);

    keypress_flag = true;
}

void int3_handler(registers_t regs) {
    u8 color = term_getcolor();
    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
    dbg_printf("BREAKPOINT HIT: \"%s\" (in %s line %d)\n", regs.edx, __FILE__, __LINE__);
    dump_registers(&regs);
    dbg_printf("Press any key to resume...");
    term_setcolor(color);

    keypress_flag = false;
}

u32 _early_malloc_ptr SECTION(".boot_data");
page_directory_t *kernel_page_dir SECTION(".boot_data");
#define KERNEL_OFFSET 0xC0000000

static void SECTION(".boot_text") *_early_malloc(u32 size, bool aligned) {
    if (aligned && (_early_malloc_ptr & 0xFFFFF000)) {
        // if we need to be aligned and are not already, align the placement address
        _early_malloc_ptr &= 0xFFFFF000;
        _early_malloc_ptr += 0x1000;
    }

    u32 tmp = _early_malloc_ptr;
    _early_malloc_ptr += size;
    return (void *) tmp;
}

static void SECTION(".boot_text") _early_memset(void *ptr, u8 value, u32 count) {
    for (u32 i = 0; i < count; i++) {
        ((u8 *) ptr)[i] = value;
    }
}

static void SECTION(".boot_text") _early_page_map(u32 physical_addr, u32 virtual_addr) {
    int dir_index = virtual_addr >> 22;
    int table_index = (virtual_addr >> 12) & 0x3FF;

    // allocate if not there
    if (!kernel_page_dir->tables[dir_index]) {
        page_table_t *table = _early_malloc(sizeof(page_table_t), true);
        _early_memset(table, 0, sizeof(page_table_t));

        kernel_page_dir->tables[dir_index] = table;
        kernel_page_dir->tablesPhysical[dir_index] = (u32) table | 3;
    }

    kernel_page_dir->tables[dir_index]->pages[table_index].address = physical_addr >> 12;
    kernel_page_dir->tables[dir_index]->pages[table_index].present = true;
    kernel_page_dir->tables[dir_index]->pages[table_index].rw = true;
}

void USED NORETURN SECTION(".boot_text") _early_boot() {
    extern u32 _kernel_end;
    _early_malloc_ptr = (u32) &_kernel_end;

    kernel_page_dir = _early_malloc(sizeof(page_directory_t), true);
    _early_memset(kernel_page_dir, 0, sizeof(page_directory_t));

    // identity map first mb (for display and stuff)
    for (int i = 0; i < 0x100000; i += 0x1000) {
        _early_page_map(i, i);
    }

    // todo map the last entry to the page tables

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

    // identity map the bootcode
    for (u32 i = boot_base; i < boot_end; i += 0x1000) {
        _early_page_map(i, i);
    }

    // map kernel .text and .rodata as read-only
    for (u32 i = kernel_code_start; i < kernel_code_end; i += 0x1000) {
        _early_page_map(i - KERNEL_OFFSET, i - KERNEL_OFFSET);
        _early_page_map(i - KERNEL_OFFSET, i);
    }

    // map .data and .bss as read-write
    for (u32 i = kernel_data_start; i < kernel_data_end; i += 0x1000) {
        _early_page_map(i - KERNEL_OFFSET, i - KERNEL_OFFSET);
        _early_page_map(i - KERNEL_OFFSET, i);
    }

    // load cr3 with the table
    asm volatile ("mov %0, %%cr3" :: "r" (&kernel_page_dir->tablesPhysical));

    // tell cr0 to enable paging
    volatile u32 cr0;
    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 1 << 31;
    asm volatile ("mov %0, %%cr0" :: "r" (cr0));

    // jmp to higher half boot
    asm volatile (
    "lea _boot, %%eax  \n"
    "jmp *%%eax        \n"
    ::: "%eax"
    );

    __builtin_unreachable();
}

static void USED NORETURN _boot() {
    serial_init();
    term_init();

    u32 eip;
    asm volatile ("mov $., %0" : "=r" (eip));
    dbg_logf(LOG_INFO, "Hello higher-half kernel world, we at 0x%08x now!\n", eip);

    init_gdt();
    init_interrupts();
    init_paging();

    set_interrupt_handler(IRQ0, irq0_handler);
    set_interrupt_handler(IRQ1, irq1_handler);
    set_interrupt_handler(3, int3_handler);

    kernel_main();

    dbg_printf("System Halted.");
    asm volatile ("1: cli; hlt; jmp 1");
    __builtin_unreachable();
}
