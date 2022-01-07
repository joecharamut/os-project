#include "debug.h"

#include <stdarg.h>
#include "video.h"

void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" :: "a" (value), "dN" (port));
}

uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a" (value) : "dN" (port));
    return value;
}

#define COM0_PORT 0x3f8

int serial_init() {
    outb(COM0_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM0_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM0_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM0_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM0_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM0_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM0_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(COM0_PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(COM0_PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if(inb(COM0_PORT + 0) != 0xAE) {
        return 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(COM0_PORT + 4, 0x0F);
    return 0;
}

void serial_write(char c) {
    outb(COM0_PORT, c);
}

static void emit_char(char c) {
    write_char(c);
    serial_write(c);
}

static void emit_str(char *str) {
    while (*str) {
        emit_char(*str++);
    }
}

static void emit_wstr(CHAR16 *wstr) {
    while (*wstr) {
        emit_char((char) *wstr++);
    }
}

static const char *alphabet_lower = "0123456789abcdef";
static const char *alphabet_upper = "0123456789ABCDEF";

static uint32_t num_digits(uint64_t num, uint32_t base) {
    uint32_t digits = 1;
    while (num >= base) {
        num /= base;
        digits++;
    }
    return digits;
}

static void print_num(uint64_t num, uint32_t base, bool uppercase) {
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

static uint64_t get_number(va_list *arg, int width) {
    switch (width) {
        case 1:
            return (uint64_t) va_arg(*arg, long);
        case 2:
            return (uint64_t) va_arg(*arg, long long);
        default:
            return (uint64_t) va_arg(*arg, int);
    }
}

static void vprintf(const char *fmt, va_list args) {
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
