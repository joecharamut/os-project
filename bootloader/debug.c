#include "debug.h"
#include "cpu.h"

__attribute__((noreturn)) void abort() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}

void write_chr(char c) {
    extern void print_chr(char c);
    extern void serial_write(char c);

    print_chr(c);
    serial_write(c);
}

extern void set_chr(char c, uint8_t x, uint8_t y);

void write_str(const char *str) {
    char c;
    while ((c = *str)) {
        if (c == '\n') {
            write_chr('\r');
            write_chr('\n');
        } else {
            write_chr(c);
        }
        str++;
    }
}

// from http://lxr.linux.no/#linux+v2.6.22/lib/div64.c
uint64_t div_u64_u32(uint64_t *num, uint32_t base) {
    uint64_t rem = *num;
    uint64_t b = base;
    uint64_t res, d = 1;
    uint32_t high = rem >> 32;

    /* Reduce the thing a bit first */
    res = 0;
    if (high >= base) {
        high /= base;
        res = (uint64_t) high << 32;
        rem -= (uint64_t) (high*base) << 32;
    }

    while ((int64_t)b > 0 && b < rem) {
        b = b+b;
        d = d+d;
    }

    do {
        if (rem >= b) {
            rem -= b;
            res += d;
        }
        b >>= 1;
        d >>= 1;
    } while (d);

    *num = res;
    return rem;
}

uint64_t div64(uint64_t dividend, uint32_t divisor) {
    uint8_t buf[8];
    uint64_t *res = (uint64_t *) &buf;
    *res = dividend;

    div_u64_u32(res, divisor);
    return *res;
}

uint64_t mod64(uint64_t dividend, uint32_t divisor) {
    uint8_t buf[8];
    uint64_t *res = (uint64_t *) &buf;
    *res = dividend;

    return div_u64_u32(res, divisor);
}

const char print_num_alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
void write_num(uint64_t num, uint64_t base) {
    if (num >= base) {
        write_num(div64(num, base), base);
    }
    write_chr(print_num_alphabet[mod64(num, base)]);
}

int strlen(const char *str) {
    int len = 0;
    while (*str++) len++;
    return len;
}

const char *box_chars[] = {
        // ╔   ╗   ╚   ╝   ═   ║
        "\xC9\xBB\xC8\xBC\xCD\xBA",
        // ┌   ┐   └   ┘   ─   │
        "\xDA\xBF\xC0\xD9\xC4\xB3",
};
void draw_box(int x, int y, int w, int h, int style, const char *title) {
    assert(w > 0);
    w--;
    assert(h > 0);
    h--;
    assert(x + w < 80);
    assert(y + h < 25);

    const char *charset = box_chars[style];

    // corners
    set_chr(charset[0], x, y);
    set_chr(charset[1], x+w, y);
    set_chr(charset[2], x, y+h);
    set_chr(charset[3], x+w, y+h);

    // top side
    for (int i = 1; i < w; ++i) {
        set_chr(charset[4], x+i, y);
    }

    // bottom side
    for (int i = 1; i < w; ++i) {
        set_chr(charset[4], x+i, y+h);
    }

    // left side
    for (int i = 1; i < h; ++i) {
        set_chr(charset[5], x, y+i);
    }

    // right side
    for (int i = 1; i < h; ++i) {
        set_chr(charset[5], x+w, y+i);
    }

    if (title) {
        int len = strlen(title);
        int box_half = (w / 2);
        int str_half = (len / 2);
        set_chr('[', x + box_half - str_half - (w % 2), y);

        for (int i = 0; i < len; ++i) {
            set_chr(title[i], x + box_half - str_half + i, y);
        }

        set_chr(']', x + box_half + str_half + (len % 2), y);
    }
}

void write_status(int x, int y, int w, char status, const char *str) {
    set_chr('[', x+0, y);
    set_chr(status, x+1, y);
    set_chr(']', x+2, y);
    set_chr(' ', x+3, y);

    for (int i = x+4; i < w && *str; ++i) {
        set_chr(*str, i, y);
        str++;
    }
}
