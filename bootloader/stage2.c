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

    uint8_t volume_buf[sizeof(fat32_volume_t)];
    fat32_volume_t *volume = (fat32_volume_t *) &volume_buf;
    if (!fat32_open_volume(volume, boot_disk, first_sector)) {
        print_str("Boot Failure: Error opening FAT32 volume");
        abort();
    }

    int entries = fat32_read_directory(volume, NULL, volume->root_cluster);
    if (entries <= 0) {
        print_str("Boot Failure: Error reading root directory");
        abort();
    }
    fat32_directory_entry_t directory[entries];
    fat32_read_directory(volume, (fat32_directory_entry_t *) &directory, volume->root_cluster);

    for (int i = 0; i < entries; ++i) {
        print_decs("entry ", i, ": ");
        for (int j = 0; j < 8; ++j) {
            print_chr(directory[i].name[j]);
        }
        print_str(".");
        for (int j = 0; j < 3; ++j) {
            print_chr(directory[i].ext[j]);
        }
        print_hexs(" : type 0x", directory[i].attributes, "");
        print_decs(" : size ", directory[i].filesize, "");
        print_hexs(" : cluster 0x", directory[i].cluster_hi << 16 | directory[i].cluster_lo, "\n");
    }

    const char *load_name = "KERNEL  ";
    const char *load_ext = "BIN";
    int file_index = -1;
    for (int i = 0; i < entries; ++i) {
        if (strncmp(directory[i].name, load_name, 8) == 0 && strncmp(directory[i].ext, load_ext, 3) == 0) {
            file_index = i;
            break;
        }
    }
    if (file_index == -1) {
        print_str("Boot Failure: Could not find KERNEL.BIN");
        abort();
    }

    uint8_t file_buf[sizeof(fat32_file_t)];
    fat32_file_t *file = (fat32_file_t *) &file_buf;
    if (!fat32_file_open(volume, file, &directory[file_index])) {
        print_str("Boot Failure: Error opening kernel file");
        abort();
    }

    uint8_t *load_addr = (void *) 0x100000;
    uint32_t count = 0;
    uint32_t block_size = 512;
    while (count < file->file_size) {
        if (fat32_file_read(file, load_addr + count, block_size) <= 0) {
            print_str("Boot Failure: Error reading kernel file");
            abort();
        }
        count += block_size;
    }

    print_str("Success?");
    assert(false);
}
