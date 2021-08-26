#include <stdint.h>
#include <stddef.h>
#include "debug.h"
#include "cpu.h"
#include "disk.h"
#include "mem.h"

void main() {
    // load boot disk from first byte of scratch space
    uint8_t boot_disk = *((uint8_t *) 0x70000);

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

    if (!get_a20_line_state()) {
        print_str("Boot Failure: A20 Line Disabled (todo: enable it)");
        abort();
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
    disk_mbr_t *mbr = (disk_mbr_t *) &mbr_buffer;
    if (disk_read_sectors(boot_disk, mbr_buffer, 0, 1)) {
        print_str("Boot Failure: Error reading disk");
        abort();
    }

    if (mbr->signature != 0xAA55) {
        print_str("Boot Failure: Invalid MBR Signature");
        abort();
    }

    int fat_partition = -1;
    for (int i = 0; i < 4; i++) {
        print_decs("partition ", i+1, " ");
        print_hexs("is type 0x", mbr->partitions[i].type, "\n");
        if (mbr->partitions[i].type == 0x0C) {
            fat_partition = i;
        }
    }
    if (fat_partition == -1) {
        print_str("Boot Failure: Could not find system partition");
        abort();
    }
    print_decs("Trying to load partition ", fat_partition+1, "\n");

    uint32_t first_sector = mbr->partitions[fat_partition].lba_first_sector;
    uint8_t vbr_buffer[512];
    fat32_vbr_t *vbr = (fat32_vbr_t *) &vbr_buffer;
    if (disk_read_sectors(boot_disk, vbr_buffer, first_sector, 1)) {
        print_str("Boot Failure: Error reading disk");
        abort();
    }

    print_str("oem id: '");
    for (int i = 0; i < 8; ++i) {
        print_chr(vbr->oem_id[i]);
    }
    print_str("'\n");

    print_str("label: '");
    for (int i = 0; i < 11; ++i) {
        print_chr(vbr->label[i]);
    }
    print_str("'\n");

    print_hexs("vol id: 0x", vbr->serial_number, "\n");
    print_hexs("fat ver: 0x", vbr->version, "\n");
    print_hexs("vbr sig: 0x", vbr->partition_signature, "\n");

    fat32_fsinfo_t *fsinfo = (void *) 0x70000;
    if (disk_read_sectors(boot_disk, NULL, first_sector + vbr->fsinfo_sector, 1)) {
        print_str("Boot Failure: Error reading disk");
        abort();
    }

    if (fsinfo->lead_signature != 0x41615252 ||
        fsinfo->mid_signature  != 0x61417272 ||
        fsinfo->trail_signature != 0xAA550000) {
        print_str("Boot Failure: Invalid FSInfo Struct");
        abort();
    }

    assert(1 == 2);
}
