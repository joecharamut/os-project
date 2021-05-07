#ifndef OS_TERM_H
#define OS_TERM_H

#include <stddef.h>
#include <stdint.h>
#include <std/types.h>

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};

#define VGA_COLOR(fg, bg) (fg | bg << 4)
#define VGA_ENTRY(ch, color) ((uint16_t) ch | (uint16_t) color << 8)

void term_init();
void term_clear();
void term_setcolor(uint8_t);
void term_write(const char *);
void term_putchar(char);

void term_enable_cursor(u8 start, u8 end);
void term_disable_cursor();
void term_update_cursor(int x, int y);

#endif //OS_TERM_H
