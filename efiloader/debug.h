#ifndef LOADER_DEBUG_H
#define LOADER_DEBUG_H

#include <stdint.h>
#include <stdarg.h>

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

void play_sound(uint32_t freq);
void stop_sound();
void beep(uint64_t count);

int serial_init(uint16_t port);

void dbg_vprint(const char *fmt, va_list args);
void dbg_print(const char *fmt, ...);

#endif //LOADER_DEBUG_H
