#include "debug.h"

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

const char print_num_alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
void write_num(uint32_t num, uint32_t base) {
    if (num >= base) {
        write_num(num / base, base);
    }
    write_chr(print_num_alphabet[num % base]);
}
