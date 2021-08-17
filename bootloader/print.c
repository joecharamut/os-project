#include "print.h"

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

const char hex_alphabet[] = "0123456789ABCDEF";
void print_hex(int i) {
    if (i >= 16) {
        print_hex(i / 16);
    }
    print_chr(hex_alphabet[i % 16]);
}

void print_chr(char c) {
    __asm__ volatile (
    "movb $0x0E, %%ah   \n" // teletype output
    "movb $0x00, %%bh   \n" // codepage 0
    "movb $0x00, %%bl   \n" // foreground color 0
    "movb %0, %%al      \n" // char to write
    "int $0x10          \n"
    :
    : "r" (c)
    : "ax", "bx");
}
