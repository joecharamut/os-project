#include <stdint.h>
#include "debug.h"
#include "cpuid.h"
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
        print_decs("Boot Failure: Not enough Low Memory (expected: 640k, actual: ", low_ram + 1, "k)\n");
        abort();
    }

    print_str("hello world from pretty broken c!\n");
    print_hexs("boot drive was: 0x", boot_disk, "\n");
    print_decs("low ram available: ", low_ram + 1, "kb\n");

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
        print_hexs("Boot Failure: Error reading disk: 0x", status, "");
        abort();
    }

    print_hexs("disk signature: ", mbr->disk_signature, "\n");

    if (mbr->signature != 0xAA55) {
        print_hexs("Boot Failure: Invalid MBR Signature: 0x", mbr->signature, "");
        abort();
    }

    print_hexs("part 1 type: 0x", mbr->partitions[0].type, "\n");
    print_hexs("part 2 type: 0x", mbr->partitions[1].type, "\n");
    print_hexs("part 3 type: 0x", mbr->partitions[2].type, "\n");
    print_hexs("part 4 type: 0x", mbr->partitions[3].type, "\n");
}
