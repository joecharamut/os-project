#include "debug.h"

#include <efi.h>
#include <efilib.h>
#include <stdarg.h>
#include "video.h"

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" :: "a" (value), "dN" (port));
}

uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a" (value) : "dN" (port));
    return value;
}

const uint16_t KBC_DATA = 0x60;
const uint16_t KBC_STATUS = 0x64;
const uint16_t KBC_COMMAND = 0x64;

void play_sound(uint32_t freq) {
    uint32_t divisor = 1193180 / freq;
    outb(0x43, 0xB6);
    outb(0x42, ((uint8_t) (divisor)));
    outb(0x42, ((uint8_t) (divisor >> 8)));

    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}

void stop_sound() {
    uint8_t state = inb(0x61);
    outb(0x61, state & 0xFC);
}

void set_keyboard_leds(uint8_t state) {
    do {
        outb(KBC_COMMAND, 0xED);
        while (inb(KBC_STATUS) & 2); // wait for input buffer clear
        outb(KBC_DATA, state);
        while (inb(KBC_STATUS) & 1); // wait for output buffer clear
    } while (0 && inb(KBC_DATA) == 0xFE);
}


const uint64_t MS = 1000;
void beep(uint64_t count) {

    for (uint64_t i = 0; i < count; ++i) {
        play_sound(1000);
        set_keyboard_leds(3);

//        for (int i = 0; i < 0x1000; ++i) __asm__ volatile ("pause");
        BS->Stall(150*MS);

//        RT->GetNextHighMonotonicCount(&time, NULL);
//        time.

        set_keyboard_leds(0);
        stop_sound();

        BS->Stall(200*MS);
//        for (int i = 0; i < 0x1000; ++i) __asm__ volatile ("pause");
    }
}

static uint16_t SERIAL_PORT = 0;
static bool serial_works = false;

int serial_init(uint16_t port) {
    SERIAL_PORT = port;
    outb(SERIAL_PORT + 1, 0x00);    // Disable all interrupts
    outb(SERIAL_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_PORT + 0, 0x01);    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_PORT + 1, 0x00);    //                  (hi byte)
    outb(SERIAL_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    outb(SERIAL_PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip
    outb(SERIAL_PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if(inb(SERIAL_PORT + 0) != 0xAE) {
        return 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(SERIAL_PORT + 4, 0x0F);
    serial_works = true;
    return 0;
}

static void serial_write(char c) {
    outb(SERIAL_PORT, c);
}

static void emit_char(char c) {
    write_char(c);
    if (serial_works) serial_write(c);
}

static void print_str(char *str) {
    while (*str) {
        emit_char(*str++);
    }
}

static void print_wstr(CHAR16 *wstr) {
    while (*wstr) {
        emit_char((char) *wstr++);
    }
}

static uint32_t num_digits(uint64_t num, uint32_t base) {
    uint32_t digits = 1;
    while (num >= base) {
        num /= base;
        digits++;
    }
    return digits;
}

static const char *print_num_alphabet = "0123456789abcdefghijklmnopqrstuvwxyz";
static const int max_number_length = 32;

static void print_num(va_list *args_ptr, uint8_t width, uint32_t base, bool use_sign, char pad_char, uint8_t min_length) {
    bool sign = false;
    uint64_t num = 0;

    if (width == 0) {
        int32_t x = va_arg(*args_ptr, int32_t);

        if (use_sign && x < 0) {
            sign = true;
            num = (uint64_t) (-x);
        } else {
            num = (uint64_t) x;
        }

        num &= 0xFFFFFFFF;
    } else {
        int64_t x = va_arg(*args_ptr, int64_t);

        if (use_sign && x < 0) {
            sign = true;
            num = (uint64_t) (-x);
        } else {
            num = (uint64_t) x;
        }

        num &= 0xFFFFFFFFFFFFFFFF;
    }

    uint32_t digits = num_digits(num, base);
    if (digits < min_length && min_length < max_number_length-1) {
        digits = min_length;
    }

    char buf[max_number_length];
    for (int i = 0; i < max_number_length; ++i) {
        buf[i] = pad_char;
    }
    buf[digits] = 0;


    uint32_t i = digits - 1;
    while (1) {
        buf[i] = print_num_alphabet[num % base];
        num /= base;

        if (i == 0) break;
        --i;
    }

    if (sign) emit_char('-');
    print_str(buf);
}

void dbg_vprint(const char *fmt, va_list args) {
    bool in_format = false;
    bool hex_prefix;
    uint8_t width;
    char pad_char;
    uint8_t min_length;

    for (char c; (c = *fmt); fmt++) {
        if (in_format) {
            switch (c) {
                case '%':
                    emit_char('%');
                    in_format = false;
                    break;

                case 's':
                    if (width == 0) {
                        print_str(va_arg(args, char *));
                    } else {
                        print_wstr(va_arg(args, CHAR16 *));
                    }
                    in_format = false;
                    break;

                case 'c':
                    emit_char((char) va_arg(args, int));
                    in_format = false;
                    break;

                case 'd':
                case 'i':
                    print_num(&args, width, 10, true, pad_char, min_length);
                    in_format = false;
                    break;

                case 'u':
                    print_num(&args, width, 10, false, pad_char, min_length);
                    in_format = false;
                    break;

                case 'x':
                    if (hex_prefix) print_str("0x");
                    print_num(&args, width, 16, false, pad_char, min_length);
                    in_format = false;
                    break;

                case 'l':
                    width++;
                    break;

                case '#':
                    hex_prefix = true;
                    break;

                case ' ':
                    pad_char = c;
                    break;

                case '0':
                    if (!pad_char) {
                        pad_char = c;
                    } else {
                        min_length *= 10;
                    }
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
                    min_length *= 10;
                    min_length += c - '0';
                    break;

                default:
                    emit_char('?');
                    in_format = false;
                    break;
            }
        } else if (c == '%') {
            in_format = true;
            width = 0;
            hex_prefix = false;
            pad_char = 0;
            min_length = 0;
        } else {
            emit_char(c);
        }
    }
}

void dbg_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    dbg_vprint(fmt, args);

    va_end(args);
}
