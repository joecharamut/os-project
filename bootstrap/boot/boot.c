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

void _boot(u32 multiboot_magic, u32 *multiboot_info) {
    serial_init();
    term_init();

    init_gdt();
    init_interrupts();
    init_paging();

    set_interrupt_handler(IRQ0, irq0_handler);
    set_interrupt_handler(IRQ1, irq1_handler);
    set_interrupt_handler(3, int3_handler);

    kernel_main();
}
