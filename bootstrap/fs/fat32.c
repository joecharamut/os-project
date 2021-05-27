#include <std/types.h>
#include <debug/debug.h>
#include <std/string.h>
#include <debug/assert.h>
#include <std/list.h>
#include "ide.h"
#include "fat32.h"

typedef struct fat32_boot_record {
    // BPB
    u8 _jmp[3];
    char oem_id[8];
    u16 sector_size;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    u8 number_of_fats;
    u16 number_of_directory_entries;
    u16 total_sectors;
    u8 media_descriptor_type;
    u16 fat16_sectors_per_fat;
    u16 sectors_per_track;
    u16 number_of_heads;
    u32 hidden_sectors;
    u32 large_sector_count;

    // EBPB
    u32 sectors_per_fat;
    u16 flags;
    u16 fat_version;
    u32 root_directory_cluster;
    u16 fsinfo_sector;
    u16 backup_boot_sector;
    u8 reserved[12];
    u8 drive_number;
    u8 windows_flags;
    u8 signature;
    u32 volume_id;
    char volume_label[11];
    char system_identifier[8];
    u8 boot_code[420];
    u16 boot_signature;
} __attribute__((packed)) fat32_boot_record_t;

typedef struct fat32_fsinfo {
    u32 lead_signature;
    u8 _reserved[480];
    u32 signature_two;
    u32 last_free_cluster;
    u32 available_cluster_hint;
    u8 _reserved_2[12];
    u32 tail_signature;
} __attribute__((packed)) fat32_fsinfo_t;

typedef struct fat32_cluster_entry {
    char filename[8];
    char extension[3];
    union {
        struct {
            bool readonly : 1;
            bool hidden : 1;
            bool system : 1;
            bool volume_id : 1;
            bool directory : 1;
            bool archive : 1;
            u8 _padding : 2;
        } __attribute__((packed)) flags;
        u8 value;
    } attributes;
    u8 reserved;
    u8 creation_tenths_seconds;
    struct {
        u16 hour : 5;
        u16 minutes : 6;
        u16 half_seconds : 5;
    } __attribute__((packed)) creation_time;
    struct {
        u16 year : 7;
        u16 month : 4;
        u16 day : 5;
    } __attribute__((packed)) creation_date;
    struct {
        u16 year : 7;
        u16 month : 4;
        u16 day : 5;
    } __attribute__((packed)) access_date;
    u16 first_cluster_hi;
    struct {
        u16 hour : 5;
        u16 minutes : 6;
        u16 half_seconds : 5;
    } __attribute__((packed)) modification_time;
    struct {
        u16 year : 7;
        u16 month : 4;
        u16 day : 5;
    } __attribute__((packed)) modification_date;
    u16 first_cluster_lo;
    u32 file_size;
} __attribute__((packed)) fat32_cluster_entry_t;

typedef enum fat32_entry_type {
    FAT32_DIRECTORY,
    FAT32_FILE,
} fat32_entry_type_t;

typedef struct fat32_entry {
    fat32_entry_type_t type;

    char filename[8];
    char extension[3];
    u8 attributes;
    time_t created_time;
    time_t accessed_time;
    time_t modified_time;

    u16 *long_filename;

    u32 first_cluster;
    u32 file_size;

    list_t *directory_contents;
} fat32_entry_t;

typedef struct fat32_volume {
    u32 sector_size;
    u32 first_partition_sector;
    u32 first_data_sector;
    u32 first_fat_sector;
    u32 reserved_sectors;
    u16 ata_bus;
    u8 ata_drive;
    fat32_entry_t *root_directory;
} fat32_volume_t;

u32 fat32_read_cluster(fat32_volume_t *volume, u32 cluster, u8 *buffer) {
    u32 fat_offset = cluster * 4;
    u32 fat_sector = volume->reserved_sectors + (fat_offset / volume->sector_size);
    u32 ent_offset = fat_offset % volume->sector_size;
    ata_read_sectors(volume->ata_bus, volume->ata_drive, volume->first_partition_sector + fat_sector, 1, (u16 *) buffer);
    return (*(u32 *) &buffer[ent_offset]) & 0x0FFFFFFF;
}

bool fat32_find_partitions() {
    assert(sizeof(fat32_boot_record_t) == 512);
    assert(sizeof(fat32_cluster_entry_t) == 32);

    ide_drives_t drives = ide_enumerate_drives();
    mbr_drive_t fat_drive;
    int fat_partition = -1;

    for (int i = 0; i < 4; ++i) {
        mbr_drive_t drive = drives.drives[i];
        if (drive.valid) {
            for (int j = 0; j < 4; ++j) {
                dbg_logf(LOG_DEBUG, "drive %d part %d type 0x%x\n", i, j, drive.partition_info[j].partition_type);
                if (drive.partition_info[j].partition_type == 0xC) {
                    fat_drive = drive;
                    fat_partition = j;
                    goto end;
                }
            }
        }
    }
    end:
    if (fat_partition == -1) {
        return false;
    }

    u32 first_sector = fat_drive.partition_info[fat_partition].lba_first_sector;
    u32 part_size = fat_drive.partition_info[fat_partition].sector_count;

    dbg_logf(LOG_DEBUG, "first sector: %d, size: %d\n", first_sector, part_size);

    u16 buffer[256];
    ata_read_sectors(fat_drive.ata_bus, fat_drive.ata_drive, first_sector, 1, buffer);

    fat32_boot_record_t boot_record = { 0 };
    memcpy(&boot_record, buffer, 512);

    assert(boot_record.signature == 0x28 || boot_record.signature == 0x29);

    dbg_logf(LOG_DEBUG, "found volume (id %04x-%04x, label '%c%c%c%c%c%c%c%c%c%c%c', sector size %d)\n",
             (boot_record.volume_id >> 16) & 0xFFFF,
             boot_record.volume_id & 0xFFFF,
             boot_record.volume_label[0],
             boot_record.volume_label[1],
             boot_record.volume_label[2],
             boot_record.volume_label[3],
             boot_record.volume_label[4],
             boot_record.volume_label[5],
             boot_record.volume_label[6],
             boot_record.volume_label[7],
             boot_record.volume_label[8],
             boot_record.volume_label[9],
             boot_record.volume_label[10],
             boot_record.sector_size);

    u8 fat_table[boot_record.sector_size];
    fat32_volume_t volume = (fat32_volume_t) {
        .sector_size = boot_record.sector_size,
        .first_partition_sector = first_sector,
        .first_data_sector = boot_record.reserved_sectors + (boot_record.number_of_fats * boot_record.sectors_per_fat),
        .reserved_sectors = boot_record.reserved_sectors,
        .ata_drive = fat_drive.ata_drive,
        .ata_bus = fat_drive.ata_bus,
    };
    fat32_read_cluster(&volume, 0, fat_table);

    u8 disk_buffer[512];
    fat32_read_cluster(&volume, 2, disk_buffer);
    fat32_cluster_entry_t *entries = (fat32_cluster_entry_t *) &disk_buffer;

    for (int i = 0; i < 16; ++i) {
        dbg_logf(LOG_DEBUG, "%c%c%c%c%c%c%c%c.%c%c%c\n",
                 entries[i].filename[0],
                 entries[i].filename[1],
                 entries[i].filename[2],
                 entries[i].filename[3],
                 entries[i].filename[4],
                 entries[i].filename[5],
                 entries[i].filename[6],
                 entries[i].filename[7],
                 entries[i].extension[0],
                 entries[i].extension[1],
                 entries[i].extension[2]
                 );
    }

    return true;
}
