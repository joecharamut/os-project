#include <stdbool.h>
#include <stdarg.h>
#include "panic.h"
#include "term.h"
#include "debug.h"

static __attribute__ ((noreturn)) void halt() {
    asm volatile (
    "\n cli"
    "\n panic_halt: hlt"
    "\n jmp panic_halt"
    );
}

void panic(const char *msg, const registers_t *registers, ...) {
    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));

    dbg_printf("KERNEL PANIC: ");

    va_list ap;
    va_start(ap, registers);
    dbg_vprintf(msg, ap);
    va_end(ap);

    dbg_printf("\n");

    if (registers) {
        dump_registers(registers);
    }

    halt();
}
