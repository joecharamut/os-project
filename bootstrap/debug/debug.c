#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>

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
void print_num(uint64_t input, int base) {
    int size = 64;
    char buf[size];
    for (int j = 0; j < size; j++) {
        buf[j] = 0;
    }

    if (base < 2 || base > 36) return;

    int tmp = size;
    while (input > 0) {
        buf[--tmp] = alphabet[input % base];
        input = input / base;
    }

    for (int i = 0; i < size; i++) {
        if (buf[i]) {
            emit_char(buf[i]);
        }
    }
}

void dbg_vprintf(const char *fmt, va_list ap) {
    bool in_format = false;
    int width_count = 0;

    for (int i = 0; fmt[i]; i++) {
        char c = fmt[i];

        if (in_format) {
            if (c == '%') {
                emit_char('%');
                in_format = false;
            } else if (c == 's') {
                emit_str(va_arg(ap, const char *));
                in_format = false;
            } else if (c == 'c') {
                emit_char(va_arg(ap, int));
                in_format = false;
            } else if (c == 'd') {
                uint64_t number = 0;

                if (width_count == 0) {
                    int tmp = va_arg(ap, int);
                    if (tmp < 0) {
                        tmp = -tmp;
                        emit_char('-');
                    }
                    number = tmp;
                } else if (width_count == 2) {
                    long long tmp = va_arg(ap, long long);
                    if (tmp < 0) {
                        tmp = -tmp;
                        emit_char('-');
                    }
                    number = tmp;
                }

                if (number == 0) {
                    emit_char('0');
                } else {
                    print_num(number, 10);
                }
                in_format = false;
            } else if (c == 'x') {
                uint64_t number = 0;

                if (width_count == 0) {
                    number = va_arg(ap, int);
                } else if (width_count == 2) {
                    number = va_arg(ap, long long);
                }

                if (number == 0) {
                    emit_char('0');
                } else {
                    print_num(number, 16);
                }
                in_format = false;
            } else if (c == 'l') {
                width_count++;
            }
        } else if (c == '%') {
            in_format = true;
            width_count = 0;
        } else {
            emit_char(c);
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
            term_setcolor(vga_entry_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE));
            break;

        case LOG_ERROR:
            term_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLUE));
            break;

        case LOG_FATAL:
            term_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
            break;

        default:
            break;
    }

    dbg_printf("[%s] ", LOG_LEVEL_TO_STR[level]);
    dbg_vprintf(fmt, ap);

    term_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
    current_level = LOG_INFO;

    va_end(ap);
}
