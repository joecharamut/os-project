#ifndef LOADER_DEBUG_H
#define LOADER_DEBUG_H

#include <stdint.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

int serial_init();

void dbg_print(const char *fmt, ...);

#endif //LOADER_DEBUG_H
