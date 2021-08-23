__attribute__((noreturn)) void abort();

#include <stdint.h>
#include "cpuid.h"
#include "print.h"
#include "disk.h"
#include "mem.h"

void main() {
    volatile uint8_t boot_disk;
    __asm__ volatile ("movb %%dl, %0\t\n" : "=al" (boot_disk));

    uint16_t low_ram = detect_low_ram();
    if (low_ram == 0xFFFF) {
        print_str("Boot Failure: BIOS does not support detecting Low Memory");
        abort();
    }
    if (low_ram != 639) {
        print_str("Boot Failure: Not enough Low Memory (expected: 640k, actual: ");
        print_dec(low_ram + 1);
        print_str("k)\n");
        abort();
    }
    assert(1 == 2);

    print_str("hello world from pretty broken c!\n");
    print_str("boot drive was: 0x"); print_hex(boot_disk); print_str("\n");
    print_str("low ram available: "); print_dec(low_ram + 1); print_str("kb\n");

    if (!supports_cpuid()) {
        print_str("Boot Failure: Processor does not support CPUID");
        abort();
    }

    if (!supports_long_mode()) {
        print_str("Boot Failure: Processor does not support Long Mode");
        abort();
    }

    uint8_t mbr_buffer[512];
    disk_mbr_t *mbr = (void *) &mbr_buffer;
    uint32_t status = disk_read_sectors(boot_disk, mbr_buffer, 0, 1);
    if (status) {
        print_str("Boot Failure: Error reading disk: 0x"); print_hex(status);
        abort();
    }

    print_str("disk signature: ");
    print_hex(mbr->disk_signature);
    print_str("\n");

    if (mbr->signature != 0xAA55) {
        print_str("Boot Failure: Invalid MBR Signature: 0x");
        print_hex(mbr->signature);
        abort();
    }

    print_str("part 1 type: 0x"); print_hex(mbr->partitions[0].type); print_str("\n");
    print_str("part 2 type: 0x"); print_hex(mbr->partitions[1].type); print_str("\n");
    print_str("part 3 type: 0x"); print_hex(mbr->partitions[2].type); print_str("\n");
    print_str("part 4 type: 0x"); print_hex(mbr->partitions[3].type); print_str("\n");
}

__attribute__((noreturn)) void abort() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}
