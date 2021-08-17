__attribute__((noreturn)) void abort();

#include "cpuid.h"
#include "print.h"

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
