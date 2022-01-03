#include "printf.h"

#include <stdarg.h>

#include "video.h"
#include "serial.h"

void emit_char(char c) {
    write_char(c);
    serial_write(c);
}

void emit_str(char *str) {
    while (*str) {
        emit_char(*str++);
    }
}

void emit_wstr(CHAR16 *wstr) {
    while (*wstr) {
        emit_char((char) *wstr++);
    }
}

static const char *alphabet_lower = "0123456789abcdef";
static const char *alphabet_upper = "0123456789ABCDEF";

uint32_t num_digits(uint64_t num, uint32_t base) {
    uint32_t digits = 1;
    while (num >= base) {
        num /= base;
        digits++;
    }
    return digits;
}

void print_num(uint64_t num, uint32_t base, bool uppercase) {
    uint32_t digits = num_digits(num, base);
    char buf[32] = { 0 };

    uint32_t i = digits - 1;
    while (1) {
        buf[i] = (uppercase ? alphabet_upper : alphabet_lower)[num % base];
        num /= base;

        if (i == 0) break;
        --i;
    }

    emit_str(buf);
}

uint64_t get_number(va_list *arg, int width) {
    switch (width) {
        case 1:
            return (uint64_t) va_arg(*arg, long);
        case 2:
            return (uint64_t) va_arg(*arg, long long);
        default:
            return (uint64_t) va_arg(*arg, int);
    }
}

void vprintf(const char *fmt, va_list args) {
    bool in_format = false;
    char c;

    uint64_t n;
    int width;
    bool is_signed;
    char pad_char;
    uint32_t pad_length;

    while ((c = *fmt)) {
        if (in_format) {
            switch (c) {
                case '%':
                    emit_char('%');
                    in_format = false;
                    break;

                case 's':
                    if (width == 0) {
                        emit_str(va_arg(args, char *));
                    } else {
                        emit_wstr(va_arg(args, CHAR16 *));
                    }
                    in_format = false;
                    break;

                case 'c':
                    emit_char((char) va_arg(args, int));
                    in_format = false;
                    break;

                case 'd':
                case 'i':
                    n = get_number(&args, width);
                    print_num(n, 10, false);
                    in_format = false;
                    break;

                case 'x':
                case 'X':
                    n = get_number(&args, width);
                    print_num(n, 16, (c == 'X'));
                    in_format = false;
                    break;

                case 'l':
                    width++;
                    break;

                case ' ':
                case '0':
                    pad_char = c;
                    pad_length = 0;
                    break;

                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    pad_length *= 10;
                    pad_length += c - '0';
                    break;

                default:
                    emit_char('%');
                    emit_char(c);
                    in_format = false;
                    break;
            }
        } else if (c == '%') {
            in_format = true;
            n = 0;
            width = 0;
            is_signed = true;
            pad_char = ' ';
            pad_length = 0;
        } else {
            emit_char(*fmt);
        }

        fmt++;
    }
}

void printf(const char *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);

    vprintf(fmt, argp);

    va_end(argp);
}
