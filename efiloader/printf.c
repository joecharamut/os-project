#include "printf.h"

#include <stdarg.h>

#include "efi.h"
#include "efilib.h"
#include "video.h"
#include "serial.h"

void emit_char(char c) {
    write_char(c);
    serial_write(1, &c);
}

void emit_str(char *str) {
    while (*str) {
        emit_char(*str++);
    }
}

static const char *alphabet_lower = "0123456789abcdef";
static const char *alphabet_upper = "0123456789ABCDEF";
void print_num(uint64_t num, int base, bool uppercase) {

}

void vprintf(const char *fmt, va_list args) {
    bool in_format = false;
    char c;
    uint64_t n;

    while ((c = *fmt)) {
        if (in_format) {
            switch (c) {
                case '%':
                    emit_char('%');
                    in_format = false;
                    break;

                case 's':
                    emit_str(va_arg(args, char *));
                    in_format = false;
                    break;

                case 'd':
                    n = va_arg(args, uint64_t);
                    print_num(n, 10, false);
                    break;

                default:
                    emit_char('%');
                    emit_char(c);
                    in_format = false;
                    break;
            }
        } else if (c == '%') {
            in_format = true;
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
