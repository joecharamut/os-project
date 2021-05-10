#ifndef OS_PANIC_H
#define OS_PANIC_H

#include <boot/interrupts.h>

#define PANIC(msg) panic("%s (in %s line %d)", 0, msg, __FILE__, __LINE__)

void __attribute__ ((noreturn)) panic(const char *msg, const registers_t *registers, ...);

#endif //OS_PANIC_H
