#include <stdint.h>
#include <stddef.h>
#include <cpuid.h>

#include "debug.h"
#include "cpu.h"
#include "disk.h"
#include "mem.h"
#include "elf.h"

void main() {
    // load boot disk from first byte of scratch space
    uint8_t boot_disk = *((uint8_t *) 0x70000);

    print_str("\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB");
    print_str("\xBA                            bruh loader v1.0                                  \xBA");
    print_str("\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC");

    bios_mmap_entry_t memory_map[16];
    uint32_t mmap_entries = get_system_memory_map(memory_map);
    if (mmap_entries == 0) {
        print_str("Boot Failure: BIOS does not support System Memory Map");
        abort();
    } else if (mmap_entries > 16) {
        print_str("Boot Failure: mmap_entries buffer overrun");
        abort();
    }

    const int KiB = 1024;
    const int MiB = KiB * 1024;

    for (uint32_t i = 0; i < mmap_entries; ++i) {
        print_decs("mmap ", i, ": [");
        print_str(memory_map[i].type == 1 ? "\xFB, " : "-, ");
        print_hexs("Region: 0x", memory_map[i].base, "-");
        print_hexs("0x", memory_map[i].base + memory_map[i].length, ", ");

        print_hexs("Size: 0x", memory_map[i].length, ", ");
        if (memory_map[i].length > MiB) {
            print_str("(");
            if ((uint32_t) memory_map[i].length % MiB != 0) {
                print_str("~");
            }
            print_dec((uint32_t) memory_map[i].length / MiB);
            print_str(" MiB)");
        } else if (memory_map[i].length > KiB) {
            print_str("(");
            if ((uint32_t) memory_map[i].length % KiB != 0) {
                print_str("~");
            }
            print_dec((uint32_t) memory_map[i].length / KiB);
            print_str(" KiB)");
        } else {
            print_str("(");
            print_dec((uint32_t) memory_map[i].length);
            print_str(" B)");
        }
        print_str("]\n");
    }

    if (!get_a20_line_state()) {
        // todo enable a20 if disabled
        print_str("Boot Failure: A20 Line Disabled (todo: enable it)");
        abort();
    }

    if (!__get_cpuid_max(0x80000001, NULL)) {
        print_str("Boot Failure: Processor does not support CPUID");
        abort();
    }

    uint32_t eax, ebx, ecx, edx;
    __cpuid(0x80000001, eax, ebx, ecx, edx);

    // long mode bit
    if (!(edx & (1 << 29))) {
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

    const char *load_name = "TEST64  ";
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

    print_str("Loading Kernel File Header...\n");
    uint8_t header_buf[64];
    if (fat32_file_read(file, header_buf, 64) != 64) {
        print_str("Boot Failure: Error reading kernel file header");
        abort();
    }

    elf_identifier_t *ident = (elf_identifier_t *) header_buf;

    if (ident->magic[0] != 0x7F
        || ident->magic[1] != 'E'
        || ident->magic[2] != 'L'
        || ident->magic[3] != 'F'
        || ident->version != ELF_IDENT_CURRENT_VERSION) {
        print_str("Boot Failure: Invalid ELF Header");
        abort();
    }

    if (ident->bitness != ELF_IDENT_64BIT) {
        print_str("Boot Failure: Unsupported ELF Bitness");
        abort();
    }

    if (ident->endianness != ELF_IDENT_LITTLE_ENDIAN) {
        print_str("Boot Failure: Unsupported ELF Endianness");
        abort();
    }

    elf64_header_t *header = (elf64_header_t *) header_buf;

    if (header->type != ELF_TYPE_EXEC) {
        print_str("Boot Failure: Unsupported ELF Object Type");
        abort();
    }

    if (header->instruction_set != 0x3E) {
        print_str("Boot Failure: Unsupported ELF Instruction Set");
        abort();
    }
    print_hexs("Entrypoint is at 0x", header->entry, "\n");

    print_str("Loading Program Header Table...\n");

    elf64_pht_entry_t pht[header->pht_entry_count];
    fat32_file_seek(file, (int) header->pht_offset, FAT32_SEEK_SET);
    if (fat32_file_read(file, (void *) pht, header->pht_entry_count * sizeof(elf64_pht_entry_t)) != header->pht_entry_count * sizeof(elf64_pht_entry_t)) {
        print_str("Boot Failure: Error reading program header table");
        abort();
    }

    for (int i = 0; i < header->pht_entry_count; ++i) {
        if (pht[i].type == ELF_PTYPE_LOAD) {
            print_decs("Segment ", i, ": Loading");
            print_decs(" ", pht[i].filesize, " bytes");
            print_hexs(" to 0x", pht[i].paddr, "....");
            const char *spinner = "|/-\\";
            int spin_index = 0;
            fat32_file_seek(file, (int) pht[i].offset, FAT32_SEEK_SET);

            uint32_t count = 0;
            uint32_t block_size = 512;
            while (count < pht[i].filesize) {
                print_chr('\b');
                print_chr(spinner[spin_index]);
                spin_index = ((spin_index + 1) % 4);

                uint32_t loaded = fat32_file_read(file, (void *) (pht[i].paddr + count), block_size);
                if (loaded == 0) {
                    print_str("Boot Failure: Error reading kernel file");
                    abort();
                }
                count += loaded;
            }
            print_str("\bDone!\n");
        } else {
            print_decs("Segment ", i, ": [");
            print_hexs("type 0x", pht[i].type, ": ");

            print_hexs("flags 0x", pht[i].flags, " (");
            print_chr((pht[i].flags & ELF_PFLAG_R) ? 'R' : '-');
            print_chr((pht[i].flags & ELF_PFLAG_W) ? 'W' : '-');
            print_chr((pht[i].flags & ELF_PFLAG_X) ? 'X' : '-');
            print_str("): ");

            print_hexs("offset 0x", pht[i].offset, ": ");
            print_hexs("vaddr 0x", pht[i].vaddr, ": ");
            print_hexs("paddr 0x", pht[i].paddr, ": ");
            print_decs("filesize ", pht[i].filesize, ": ");
            print_decs("memsize ", pht[i].memsize, ": ");
            print_hexs("align 0x", pht[i].align, "]\n");
        }
    }

    print_str("Success?\n");
    assert(false);
}
