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
void print_hex(uint32_t i) {
    if (i >= 16) {
        print_hex(i / 16);
    }
    print_chr(hex_alphabet[i % 16]);
}
