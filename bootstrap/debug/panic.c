#include <stdbool.h>
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

void panic(const char *msg, const registers_t *registers) {
    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
    dbg_printf("KERNEL PANIC: %s\n", msg);

    if (registers) {
        term_setcolor(VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        dbg_printf("EAX=%0*x EBX=%0*x ECX=%0*x EDX=%0*x\nESI=%0*x EDI=%0*x EBP=%0*x ESP=%0*x\nEIP=%0*x EFL=%0*x ",
                8, registers->eax, 8, registers->ebx, 8, registers->ecx, 8, registers->edx,
                8, registers->esi, 8, registers->edi, 8, registers->ebp, 8, registers->esp,
                8, registers->eip, 8, registers->eflags);

        bool carry = registers->eflags & 1 << 0;
        bool parity = registers->eflags & 1 << 2;
        bool adjust = registers->eflags & 1 << 4;
        bool zero = registers->eflags & 1 << 6;
        bool sign = registers->eflags & 1 << 7;
        bool trap = registers->eflags & 1 << 8;
        bool interrupt = registers->eflags & 1 << 9;
        bool direction = registers->eflags & 1 << 10;
        bool overflow = registers->eflags & 1 << 11;
        dbg_printf("[%c%c%c%c%c]\n",
                   (overflow ? 'O' : '-'),
                   (interrupt ? 'I' : '-'),
                   (trap ? 'T' : '-'),
                   (zero ? 'Z' : '-'),
                   (carry ? 'C' : '-')
                );
    }

    halt();
}

void panic_file(const char *msg, const char *file, int line) {
    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
    dbg_printf("KERNEL PANIC: %s (in %s line %d)\n", msg, file, line);

    halt();
}
