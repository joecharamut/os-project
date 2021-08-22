__attribute__((noreturn)) void abort();

#include <stdint.h>
#include "cpuid.h"
#include "print.h"
#include "disk.h"

void main() {
    volatile uint8_t boot_disk;
    __asm__ volatile ("movb %%dl, %0\t\n" : "=al" (boot_disk));
    print_str("hello world from unreal mode in c\n");
    print_str("boot drive is: 0x");
    print_hex(boot_disk);
    print_str("\n");

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

    uint8_t test[512];
    print_str("orig: "); print_hex((uint32_t) test); print_str("\n");
    print_str("mask: "); print_hex(((uint32_t) test) & 0xFFFF); print_str("\n");
    print_str("mask: "); print_hex((((uint32_t) test) >> 16) << 8); print_str("\n");
    uint32_t ret = disk_read_sectors(boot_disk, test, 0, 1);

    print_str("ret: "); print_hex(ret); print_str("\n");
    for (int i = 0; i < 512; ++i) {
        print_hex(test[i]);
    }
}

__attribute__((noreturn)) void abort() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}
