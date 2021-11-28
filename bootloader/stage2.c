#include <stdint.h>
#include <stddef.h>
#include <cpuid.h>

#include "debug.h"
#include "cpu.h"
#include "disk.h"
#include "mem.h"
#include "elf.h"

__attribute__((noreturn)) void fail(const char *msg) {
    write_str("Boot Failure: ");
    write_str(msg);
    abort();
}

#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr) (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr) (((addr) >> 12) & 0x1FF)

static uint32_t page_map_free = 0;
static uint32_t map_page(uint64_t paddr, uint64_t vaddr) {
    const uint32_t pml4_index = PML4_INDEX(vaddr);
    const uint32_t pdpt_index = PDPT_INDEX(vaddr);
    const uint32_t pd_index = PD_INDEX(vaddr);
    const uint32_t pt_index = PT_INDEX(vaddr);

//    print_decs("pml4: ", pml4_index, "\n");
//    print_decs("pdpt: ", pdpt_index, "\n");
//    print_decs("pd: ", pd_index, "\n");
//    print_decs("pt: ", pt_index, "\n");
//    print_hexs("0x", paddr, " -> ");
//    print_hexs("0x", vaddr, "\n");

    if (page_map_free == 0) {
        extern void *_page_map_base;
        page_map_free = (uint32_t) &_page_map_base;
    }

    uint64_t *pml4;
    __asm__ volatile ("mov %%cr3, %0" : "=r" (pml4));
    if (pml4 == 0) {
        pml4 = (uint64_t *) page_map_free;
        __asm__ volatile ("mov %0, %%cr3" :: "r" (pml4));
        page_map_free += 0x1000;
    }

    if (pml4[pml4_index] == 0) {
        pml4[pml4_index] = page_map_free | 3;
        page_map_free += 0x1000;
    }

    uint64_t *pdpt = (uint64_t *) (pml4[pml4_index] & 0xFFFFFFFFFFFFF000);
    if (pdpt[pdpt_index] == 0) {
        pdpt[pdpt_index] = page_map_free | 3;
        page_map_free += 0x1000;
    }

    uint64_t *pd = (uint64_t *) (pdpt[pdpt_index] & 0xFFFFFFFFFFFFF000);
    if (pd[pd_index] == 0) {
        pd[pd_index] = page_map_free | 3;
        page_map_free += 0x1000;
    }

    uint64_t *pt = (uint64_t *) (pd[pd_index] & 0xFFFFFFFFFFFFF000);
    pt[pt_index] = (paddr) | 3;
//    print_hexs("free: 0x", page_map_free, "\n");
    return page_map_free;
}

static const char *spinner = "|/-\\";
static int spin_index = 0;
static void tick_spinner() {
    write_chr('\b');
    write_chr(spinner[spin_index]);
    spin_index = ((spin_index + 1) % 4);
    delay(1);
}

void main() {
    // load boot disk from first byte of scratch space
    uint8_t boot_disk = *((uint8_t *) 0x70000);

    if (!serial_init()) {
        fail("Failed to initialize serial");
    }

//    write_str("\xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB");
//    write_str("\xBA                            bruh loader v1.0                                  \xBA");
//    write_str("\xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC");

//    write_str("Hello World!\n");
    // reload video mode (clear screen)
    __asm__ ("int $0x10\t\n" :: "ax" (0x00 | *((uint8_t *) 0x0449)));
    // set bg color
    __asm__ ("int $0x10\t\n" :: "ax" (0x0B00), "bx" (0x0001));
    // disable cursor
    __asm__ ("int $0x10\t\n" :: "ax" (0x0100), "cx" (0x1000));
    draw_box(0, 0, 80, 25, 0, " MISSINGNO. ");
    write_status(2, 2, 78, '?', "bruh");
    write_status(2, 3, 78, '?', "bruh 2");
    write_status(2, 4, 78, '?', "bruh continues");
    abort();

    if (!get_a20_line_state()) {
        // todo enable a20 if disabled
        fail("A20 Line Disabled");
    }

    write_str("Detecting Memory...\n");
    bios_mmap_entry_t memory_map[16];
    uint32_t mmap_entries = get_system_memory_map(memory_map);
    if (mmap_entries == 0) {
        fail("BIOS does not support System Memory Map");
    } else if (mmap_entries > 16) {
        fail("mmap_entries buffer overrun");
    }

    for (uint32_t i = 0; i < mmap_entries; ++i) {
        print_decs("mmap ", i, ": [");
        write_str(memory_map[i].type == 1 ? "\xFB, " : "-, ");
        print_hexs("Region: 0x", memory_map[i].base, "-");
        print_hexs("0x", ((uint64_t) memory_map[i].base) + memory_map[i].length, ", ");

        print_hexs("Size: 0x", memory_map[i].length, ", ");
        if (memory_map[i].length > 0x100000) {
            write_str("(");
            if ((uint32_t) memory_map[i].length % 0x100000 != 0) {
                write_str("~");
            }
            print_dec((uint32_t) memory_map[i].length / 0x100000);
            write_str(" MiB)");
        } else if (memory_map[i].length > 0x400) {
            write_str("(");
            if ((uint32_t) memory_map[i].length % 0x400 != 0) {
                write_str("~");
            }
            print_dec((uint32_t) memory_map[i].length / 0x400);
            write_str(" KiB)");
        } else {
            write_str("(");
            print_dec((uint32_t) memory_map[i].length);
            write_str(" B)");
        }
        write_str("]\n");
    }

    if (!__get_cpuid_max(0x80000001, NULL)) {
        fail("Processor does not support CPUID");
    }

    uint32_t eax, ebx, ecx, edx;
    __cpuid(0x80000001, eax, ebx, ecx, edx);

    if (!(edx & bit_LM)) {
        fail("Processor does not support Long Mode");
    }

    uint8_t mbr_buffer[512];
    disk_mbr_t *mbr = (disk_mbr_t *) &mbr_buffer;
    if (disk_read_sectors(boot_disk, mbr_buffer, 0, 1)) {
        fail("Error reading disk");
    }

    if (mbr->signature != 0xAA55) {
        fail("Invalid MBR Signature");
    }

    int fat_partition = -1;
    for (int i = 0; i < 4; i++) {
        if (mbr->partitions[i].type == 0x0C) {
            fat_partition = i;
        }
    }
    if (fat_partition == -1) {
        fail("Could not find system partition");
    }
    print_decs("Found potential FAT32 Volume (Partition ", fat_partition+1, ")\n");

    uint32_t first_sector = mbr->partitions[fat_partition].lba_first_sector;

    uint8_t volume_buf[sizeof(fat32_volume_t)];
    fat32_volume_t *volume = (fat32_volume_t *) &volume_buf;
    if (!fat32_open_volume(volume, boot_disk, first_sector)) {
        fail("Error opening FAT32 volume");
    }

    int entries = fat32_read_directory(volume, NULL, volume->root_cluster);
    if (entries <= 0) {
        fail("Error reading root directory");
    }
    fat32_directory_entry_t directory[entries];
    fat32_read_directory(volume, (fat32_directory_entry_t *) &directory, volume->root_cluster);

    print_hexs(" Volume in Drive 0x", boot_disk, " is ");
    write_str(volume->volume_id);
    write_str("\n");
    print_hexs(" Volume Serial Number is ", volume->serial_number >> 16, "");
    print_hexs("-", volume->serial_number & 0xFFFF, "\n");

    write_str("\n Directory of X:\\\n\n");

    for (int i = 0; i < entries; ++i) {
        write_str("    ");
        for (int j = 0; j < 8; ++j) {
            write_chr(directory[i].name[j]);
        }
        write_str("  ");
        for (int j = 0; j < 3; ++j) {
            write_chr(directory[i].ext[j]);
        }
        write_str("  ");
        if ((directory[i].attributes & FAT32_ATTR_DIRECTORY)) {
            write_str("<DIR>");
        } else {
            print_dec(directory[i].filesize);
        }
        write_str("  [");
        write_chr((directory[i].attributes & FAT32_ATTR_ARCHIVE) ? 'A' : '-');
        write_chr((directory[i].attributes & FAT32_ATTR_SYSTEM) ? 'S' : '-');
        write_chr((directory[i].attributes & FAT32_ATTR_HIDDEN) ? 'H' : '-');
        write_chr((directory[i].attributes & FAT32_ATTR_READONLY) ? 'R' : '-');
        write_str("]");

        print_hexs(" : cluster 0x", directory[i].cluster_hi << 16 | directory[i].cluster_lo, "\n");
    }

    write_str("\n");

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
        fail("Could not find TEST64.BIN");
    }

    uint8_t file_buf[sizeof(fat32_file_t)];
    fat32_file_t *file = (fat32_file_t *) &file_buf;
    if (!fat32_file_open(volume, file, &directory[file_index])) {
        fail("Error opening kernel file");
    }

    write_str("Loading Kernel File Header...\n");
    uint8_t header_buf[64];
    if (fat32_file_read(file, header_buf, 64) != 64) {
        fail("Error reading kernel file header");
    }

    elf_identifier_t *ident = (elf_identifier_t *) header_buf;

    if (ident->magic[0] != 0x7F
        || ident->magic[1] != 'E'
        || ident->magic[2] != 'L'
        || ident->magic[3] != 'F'
        || ident->version != ELF_IDENT_CURRENT_VERSION) {
        fail("Invalid ELF Header");
    }

    if (ident->bitness != ELF_IDENT_64BIT) {
        fail("Unsupported ELF Bitness");
    }

    if (ident->endianness != ELF_IDENT_LITTLE_ENDIAN) {
        fail("Unsupported ELF Endianness");
    }

    elf64_header_t *header = (elf64_header_t *) header_buf;

    if (header->type != ELF_TYPE_EXEC) {
        fail("Unsupported ELF Object Type");
    }

    if (header->instruction_set != 0x3E) {
        fail("Unsupported ELF Instruction Set");
    }
    print_hexs("Entrypoint is at 0x", header->entry, "\n");

    write_str("Loading Program Header Table...\n");

    elf64_pht_entry_t pht[header->pht_entry_count];
    fat32_file_seek(file, (int) header->pht_offset, FAT32_SEEK_SET);
    if (fat32_file_read(file, (void *) pht, header->pht_entry_count * sizeof(elf64_pht_entry_t)) != header->pht_entry_count * sizeof(elf64_pht_entry_t)) {
        fail("Error reading program header table");
    }

    // identity map first 1MiB
    write_str("Identity mapping first 1MiB of RAM...");
    for (uint64_t i = 0; i < 0x100000; i += 0x1000) {
        map_page(i, i);
    }
    write_str("OK!\n");

    // recursive map pml4
    write_str("Adding recursive mapping for PML4...");
    {
        uint64_t *pml4;
        __asm__ volatile ("mov %%cr3, %0" : "=r" (pml4));
        map_page((uint64_t) pml4, 0xfffffffffffff000);
    }
    write_str("OK!\n");

    for (int i = 0; i < header->pht_entry_count; ++i) {
        if (pht[i].type == ELF_PTYPE_LOAD) {
            print_decs("Segment ", i, ": Loading");
            print_decs(" ", pht[i].filesize, " bytes....");
            fat32_file_seek(file, (int) pht[i].offset, FAT32_SEEK_SET);

            uint32_t count = 0;
            uint32_t block_size = 512;
            while (count < pht[i].filesize) {
                tick_spinner();
                uint32_t loaded = fat32_file_read(file, ((uint8_t *) pht[i].paddr) + count, block_size);
                if (loaded == 0) {
                    fail("Error reading file");
                }
                count += loaded;
            }
            write_str("\bDone!\n");

            write_str("Mapping segment into virtual memory....");
            for (uint64_t j = 0; j <= pht[i].filesize; j += 0x1000) {
                tick_spinner();
                map_page(pht[i].paddr + j, pht[i].vaddr + j);
            }
            write_str("\bDone!\n");
        } else {
            print_decs("Segment ", i, ": [");
            print_hexs("type 0x", pht[i].type, ": ");

            print_hexs("flags 0x", pht[i].flags, " (");
            write_chr((pht[i].flags & ELF_PFLAG_R) ? 'R' : '-');
            write_chr((pht[i].flags & ELF_PFLAG_W) ? 'W' : '-');
            write_chr((pht[i].flags & ELF_PFLAG_X) ? 'X' : '-');
            write_str("): ");

            print_hexs("offset 0x", pht[i].offset, ": ");
            print_hexs("vaddr 0x", pht[i].vaddr, ": ");
            print_hexs("paddr 0x", pht[i].paddr, ": ");
            print_decs("filesize ", pht[i].filesize, ": ");
            print_decs("memsize ", pht[i].memsize, ": ");
            print_hexs("align 0x", pht[i].align, "]\n");
        }
    }

    write_str("All loaded up, good luck!\n");

    // mode 2 = long mode only
    uint8_t status = set_bios_target_mode(2);
    if (status) {
        print_hexs("Could not inform BIOS of intended operating mode (0x", status, ")\n");
    }

    write_str("Press any key to continue....\n");
    while (!peek_keystroke()) {
        tick_spinner();
        delay(2);
    }
    write_str("\b \n");

    enter_long_mode(header->entry);
}
