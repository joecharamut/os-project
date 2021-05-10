
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

    dbg_logf(LOG_INFO, "Welcome to {OS_NAME} Bootstrap Loader\n");

    dbg_logf(LOG_INFO, "Initializing PCI...\n");
    if (!pci_init()) {
        dbg_logf(LOG_FATAL, "BOOT FAILURE: PCI Initialization Failed\n");
        return;
    }
    dbg_logf(LOG_INFO, "Bus Scan found %d device(s).\n", pci_num_devs());

    bool found_storage = false;
    for (size_t i = 0; i < pci_num_devs(); i++) {
        PCI_Header hdr = pci_dev_list()[i];
        if (hdr.classCode == PCI_MASS_STORAGE_CONTROLLER) {
            found_storage = true;
        }
        dbg_logf(LOG_DEBUG, "PCI Dev %d: %x:%x, %x:%x:%x\n", i, hdr.vendorId, hdr.deviceId, hdr.classCode, hdr.subclass, hdr.programmingInterface);
    }

    if (!found_storage) {
        dbg_logf(LOG_FATAL, "BOOT FAILURE: No Storage Devices Detected\n");
        return;
    }

    dbg_logf(LOG_INFO, "Initializing Storage Controller(s)...\n");
    if (!ide_init()) {
        dbg_logf(LOG_FATAL, "IDE Controller Initialization Failed\n");
        return;
    }

    dbg_printf("Hello World!\n");
}
