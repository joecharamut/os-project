#ifndef OS_PANIC_H
#define OS_PANIC_H

#include <boot/interrupts.h>

#define PANIC(msg) panic_file(msg, __FILE__, __LINE__)

void __attribute__ ((noreturn)) panic(const char *msg, const registers_t *registers);
void __attribute__ ((noreturn)) panic_file(const char *msg, const char *file, int line);

#endif //OS_PANIC_H
