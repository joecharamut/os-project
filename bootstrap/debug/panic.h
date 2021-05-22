#ifndef OS_PANIC_H
#define OS_PANIC_H

#include <boot/interrupts.h>

#define PANIC(msg, ...) panic(msg, 0, ##__VA_ARGS__)

void __attribute__ ((noreturn)) panic(const char *msg, const registers_t *registers, ...);

#endif //OS_PANIC_H
