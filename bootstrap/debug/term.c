#include "term.h"

#include <stdbool.h>
#include <stdarg.h>
#include <bootstrap/types.h>
#include <bootstrap/io/port.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
uint16_t *term_buffer = (uint16_t *) 0xB8000;
static bool term_ready = false;

size_t term_row;
size_t term_col;
uint8_t  term_default_color;
uint8_t term_color;

void term_init() {
    term_default_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE);
    term_color = term_default_color;

    term_clear();
    term_ready = true;
}

void term_clear() {
    term_row = 0;
    term_col = 0;

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            term_buffer[index] = vga_entry(' ', term_color);
        }
    }
}

void term_setcolor(uint8_t color) {
    term_color = color;
}

void term_putcharat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    term_buffer[index] = vga_entry(c, color);
}

void term_scroll() {
    for (size_t y = 1; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t src = y * VGA_WIDTH + x;
            const size_t dest = (y - 1) * VGA_WIDTH + x;
            term_buffer[dest] = term_buffer[src];
        }
    }

    for (size_t x = 0; x < VGA_WIDTH; x++) {
        term_buffer[(VGA_HEIGHT - 2) * VGA_WIDTH + x] = vga_entry(' ', term_color);
    }
}

void term_putchar(char c) {
    if (!term_ready) return;

    if (term_col == VGA_WIDTH) {
        term_col = 0;
        term_row++;
    }

    if (term_row == VGA_HEIGHT-1) {
        term_scroll();
        term_row--;
    }

    switch (c) {
        case '\n':
            term_col = 0;
            term_row++;
            break;

        case '\r':
            term_col = 0;
            break;

        default:
            term_putcharat(c, term_color, term_col, term_row);
            term_col++;
            break;
    }

    term_update_cursor(term_col, term_row);
}

void term_write(const char *str) {
    for (size_t i = 0; str[i]; i++) {
        term_putchar(str[i]);
    }
}

void term_enable_cursor(u8 start, u8 end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | end);
}

void term_disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void term_update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}
