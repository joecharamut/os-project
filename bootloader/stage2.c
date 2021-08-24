#include <stdint.h>
#include "debug.h"
#include "cpuid.h"
#include "disk.h"
#include "mem.h"

void main() {
    volatile uint8_t boot_disk;
    __asm__ volatile ("movb %%dl, %0\t\n" : "=al" (boot_disk));
    print_str("hello world from pretty broken c!\n");
    print_hexs("boot drive was: 0x", boot_disk, "\n");

    uint16_t low_ram = get_memory_size();
    if (low_ram == 0xFFFF) {
        print_str("Boot Failure: BIOS does not support detecting Low Memory");
        abort();
    }

    print_decs("low ram available: ", low_ram + 1, "kb\n");
    if (low_ram != 639) {
        print_str("Boot Failure: Not enough Low Memory");
        abort();
    }

    uint16_t extended_ram = get_extended_memory_size();
    if (extended_ram == 0xFFFF) {
        print_str("Boot Failure: BIOS does not support detecting Extended Memory");
        abort();
    }
    print_decs("extd ram available: ", extended_ram, "kb\n");

    bios_mmap_entry_t memory_map[16];
    uint32_t mmap_entries = get_system_memory_map(memory_map);
    if (mmap_entries == 0) {
        print_str("Boot Failure: BIOS does not support System Memory Map");
        abort();
    }
    print_decs("mmap entries: ", mmap_entries, "\n");
    for (uint32_t i = 0; i < mmap_entries; ++i) {
        print_decs("entry ", i, ": ");
        print_hexs("type ", memory_map[i].type, ", ");
        print_hexs("start: 0x", memory_map[i].base, ", ");
        print_hexs("length: 0x", memory_map[i].length, "\n");
    }


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
        print_str("Boot Failure: Error reading disk");
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
