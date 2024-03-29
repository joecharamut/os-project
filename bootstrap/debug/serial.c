#include "serial.h"
#include <stdint.h>
#include "../io/port.h"

#define COM0_PORT 0x3f8

int serial_received() {
    return inb(COM0_PORT + 5) & 1;
}

int is_transmit_empty() {
    return inb(COM0_PORT + 5) & 0x20;
}

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

char serial_read() {
    while (serial_received() == 0);
    return inb(COM0_PORT);
}

void serial_putchar(char c) {
    while (is_transmit_empty() == 0);
    outb(COM0_PORT, c);
}

void serial_write(const char *str) {
    for (int i = 0; str[i]; i++) {
        serial_putchar(str[i]);
    }
}
