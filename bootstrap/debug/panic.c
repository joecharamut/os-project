#include <stdarg.h>
#include <stdnoreturn.h>
#include <io/timer.h>
#include <std/registers.h>
#include "panic.h"
#include "term.h"
#include "debug.h"

noreturn void halt() {
    asm volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}

noreturn void panic(const char *msg, const registers_t *registers, ...) {
    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
    dbg_printf("Kernel Panic: ");

    va_list ap;
    va_start(ap, registers);
    dbg_vprintf(msg, ap);
    va_end(ap);

    term_setcolor(VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    dbg_printf("\n");

    if (registers) {
        dump_registers(registers);
    } else {
        registers_t regs = {0};
        SAVE_REGISTERS(&regs);
        dump_registers(&regs);
    }

    dbg_printf("Uptime was %llu ms\n", global_timer);
    dbg_printf("End Kernel Panic");

    halt();
}
