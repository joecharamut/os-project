#include "debug.h"

__attribute__((noreturn)) void abort() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}

void print_str(const char *str) {
    char c;
    while ((c = *str)) {
        if (c == '\n') {
            print_chr('\r');
            print_chr('\n');
        } else {
            print_chr(c);
        }
        str++;
    }
}

const char print_num_alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
void print_num(uint32_t num, uint32_t base) {
    if (num >= base) {
        print_num(num / base, base);
    }
    print_chr(print_num_alphabet[num % base]);
}
