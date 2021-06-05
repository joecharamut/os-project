#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <std/stdlib.h>
#include <std/math.h>

#include "debug.h"
#include "term.h"
#include "serial.h"

static LOG_LEVEL current_level = LOG_INFO;
static LOG_LEVEL min_level = LOG_DEBUG;

void emit_char(char c) {
    if (current_level >= min_level) {
        term_putchar(c);
    }
    serial_putchar(c);
}

void emit_str(const char *str) {
    for (int i = 0; str[i]; i++) {
        emit_char(str[i]);
    }
}

const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void recursive_print_number(uint64_t input, u64 base) {
    if (input < base) {
        emit_char(alphabet[input]);
    } else {
        recursive_print_number(input / base, base);
        emit_char(alphabet[input % base]);
    }
}

int recursive_count_digits(uint64_t input, u64 base) {
    if (input < base) {
        return 1;
    } else {
        return recursive_count_digits(input / base, base) + 1;
    }
}

void print_number(va_list *ap, int width, bool signed_format, int base, int padding_count, char padding_char) {
    bool input_negative;
    u64 input;

    if (width == 0) {
        unsigned int arg = va_arg(*ap, int);
        input_negative = (signed int) arg < 0;
        input = (u64) arg;
    } else if (width == 1) {
        unsigned long arg = va_arg(*ap, long);
        input_negative = (signed long) arg < 0;
        input = (u64) arg;
    } else if (width == 2) {
        unsigned long long arg = va_arg(*ap, long long);
        input_negative = (signed long long) arg < 0;
        input = (u64) arg;
    } else {
        PANIC("Invalid width specifier");
    }

    if (signed_format && input_negative) {
        emit_char('-');
        input = -input;
    }

    int places = recursive_count_digits(input, base);
    if (padding_count > places) {
        for (int i = 0; i < padding_count - places; i++) {
            emit_char(padding_char);
        }
    }

    recursive_print_number(input, base);
}

void dbg_vprintf(const char *fmt, va_list ap) {
    bool in_format = false;
    bool in_length = false;
    char pad_char = ' ';
    int width_count = 0;
    int pad_count = 0;

    for (int i = 0; fmt[i]; i++) {
        char c = fmt[i];

        if (in_format) {
            in_format = false;
            switch (c) {
                default:
                    PANIC("Invalid format character: %c\n", c);

                case '%':
                    emit_char('%');
                    break;

                case 's':
                    emit_str(va_arg(ap, const char *));
                    break;

                case 'c':
                    emit_char(va_arg(ap, int));
                    break;

                case 'd':
                    print_number(&ap, width_count, true, 10, pad_count, pad_char);
                    break;

                case 'x':
                    print_number(&ap, width_count, false, 16, pad_count, pad_char);
                    break;

                case 'u':
                    print_number(&ap, width_count, false, 10, pad_count, pad_char);
                    break;

                case 'o':
                    print_number(&ap, width_count, false, 8, pad_count, pad_char);
                    break;

                case 'l':
                    width_count++;
                    in_format = true;
                    break;

                case '*':
                    pad_count = va_arg(ap, int);
                    in_length = false;
                    in_format = true;
                    break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if (!in_length && c == '0') {
                        pad_char = '0';
                        in_length = true;
                    } else {
                        int end;
                        for (end = i; isdigit(fmt[end]); end++);
                        end--;

                        for (int j = end; j >= i; j--) {
                            pad_count += (fmt[j] - '0') * (int) pow(10, abs(j - end));
                        }
                    }
                    in_format = true;
                    break;
            }
        } else {
            if (c == '%') {
                in_format = true;
                width_count = 0;
                pad_count = 0;
                in_length = false;
            } else {
                emit_char(c);
            }
        }
    }
}

void dbg_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    dbg_vprintf(fmt, ap);
    va_end(ap);
}

static const char *LOG_LEVEL_TO_STR[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
void dbg_logf(LOG_LEVEL level, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    current_level = level;
    switch (level) {
        case LOG_WARN:
            term_setcolor(VGA_COLOR(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
            break;

        case LOG_ERROR:
            term_setcolor(VGA_COLOR(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLUE));
            break;

        case LOG_FATAL:
            term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
            break;

        default:
            break;
    }

    dbg_printf("[%s] ", LOG_LEVEL_TO_STR[level]);
    dbg_vprintf(fmt, ap);

    term_setcolor(VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
    current_level = LOG_INFO;

    va_end(ap);
}

void dump_registers(const registers_t *registers) {
    term_setcolor(VGA_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    dbg_printf("EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx\nESI=%08lx EDI=%08lx EBP=%08lx ESP=%08lx\nEIP=%08lx EFL=%08lx ",
               registers->eax, registers->ebx, registers->ecx, registers->edx,
               registers->esi, registers->edi, registers->ebp, registers->esp,
               registers->eip, registers->eflags);

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
