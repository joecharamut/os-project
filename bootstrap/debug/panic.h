#ifndef OS_PANIC_H
#define OS_PANIC_H

#include <boot/interrupts.h>
#include <stdnoreturn.h>

#define PANIC(msg, ...) panic(msg, 0, ##__VA_ARGS__)

noreturn __attribute__((format(printf, 1, 3))) void panic(const char *msg, const registers_t *registers, ...);

#endif //OS_PANIC_H
