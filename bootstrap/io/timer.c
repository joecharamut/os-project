#include "timer.h"
#include <boot/interrupts.h>
#include "port.h"

u64 global_timer = 0;

void timer_handler(interrupt_registers_t regs) {
    global_timer++;
}

void init_timer(u32 frequency) {
    u32 divisor = 1193180 / frequency;

    outb(0x43, 0x36);

    u8 loByte = (u8) (divisor & 0xFF);
    u8 hiByte = (u8) ((divisor >> 8) & 0xFF);

    outb(0x40, loByte);
    outb(0x40, hiByte);

    global_timer = 0;
    set_interrupt_handler(IRQ0, timer_handler);
}
