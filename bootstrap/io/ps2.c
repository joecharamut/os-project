#include <boot/interrupts.h>
#include <debug/debug.h>
#include "ps2.h"
#include "port.h"

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64

u8 scancode = 0;
void irq1_handler(registers_t regs) {
    scancode = inb(PS2_DATA);
    dbg_logf(LOG_DEBUG, "scancode: %x\n", scancode);
}

u8 poll_scancode() {
    while (!scancode) asm volatile ("pause");

    u8 tmp = scancode;
    scancode = 0;
    return tmp;
}

void send_command_data(u8 command, u8 data) {
    do {
        outb(PS2_DATA, command);
        outb(PS2_DATA, data);
    } while (poll_scancode() == 0xFE);
}

void send_command(u8 command) {
    do {
        outb(PS2_DATA, command);
    } while (poll_scancode() == 0xFE);
}

void init_ps2() {
    set_interrupt_handler(IRQ1, irq1_handler);

}
