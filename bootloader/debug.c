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
