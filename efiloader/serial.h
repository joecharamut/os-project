#ifndef LOADER_SERIAL_H
#define LOADER_SERIAL_H

#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" :: "a" (value), "dN" (port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a" (value) : "dN" (port));
    return value;
}

int serial_init();
void serial_write(char c);

#endif //LOADER_SERIAL_H
