#include "timer.h"
#include "port.h"

void init_timer(u32 frequency) {
    u32 divisor = 1193180 / frequency;

    outb(0x43, 0x36);

    u8 loByte = (u8) (divisor & 0xFF);
    u8 hiByte = (u8) ((divisor >> 8) & 0xFF);

    outb(0x40, loByte);
    outb(0x40, hiByte);
}
