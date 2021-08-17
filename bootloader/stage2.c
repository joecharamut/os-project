void print_str(const char *);
void print_chr(char);
void print_hex(int);
__attribute__((noreturn)) void abort();

#include "cpuid.h"

__attribute__((noreturn, used)) void main() {
    print_str("hello stage2 c world!\n");

    if (!supports_cpuid()) {
        print_str("Boot Failure: Processor does not support CPUID");
        abort();
    }

    if (!supports_long_mode()) {
        print_str("Boot Failure: Processor does not support Long Mode");
        abort();
    }

    print_str("good?\n");

    print_str("4096 in hex is: ");
    print_hex(4096);
    print_str("\n");

    abort();
}

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
