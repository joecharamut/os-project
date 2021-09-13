#ifndef LOADER_DISK_H
#define LOADER_DISK_H

#include <stdint.h>
#include <stdbool.h>
#include "debug.h"

typedef struct {
    uint8_t status;
    uint32_t chs_first_sector : 24;
    uint8_t type;
    uint32_t chs_last_sector : 24;
    uint32_t lba_first_sector;
    uint32_t lba_num_sectors;
} __attribute__((packed)) disk_partition_entry_t;
static_assert(sizeof(disk_partition_entry_t) == 16, "Invalid Size");

typedef struct {
    uint8_t bootcode[440];
    uint32_t disk_signature;
    uint16_t write_protected;
    disk_partition_entry_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) disk_mbr_t;
static_assert(sizeof(disk_mbr_t) == 512, "Invalid Size");

typedef struct {
    // BPB
    uint8_t _jmp[3];
    char oem_id[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t number_of_fats;
    uint16_t number_of_directory_entries;
    uint16_t total_sectors;
    uint8_t media_type;
    uint16_t sectors_per_fat_16; // FAT12/FAT16 Number of sectors per FAT
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;

    // EBR
    uint32_t sectors_per_fat_32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_directory_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t _reserved2[12]; // Always zero
    uint8_t drive_number;
    uint8_t _reserved3; // Flags in Windows NT
    uint8_t fat_signature;
    uint32_t serial_number;
    char label[11];
    char system_id[8]; // Always "FAT32   "
    uint8_t boot_code[420];
    uint16_t partition_signature;
} __attribute__((packed)) fat32_vbr_t;
static_assert(sizeof(fat32_vbr_t) == 512, "Invalid Size");

typedef struct {
    uint32_t lead_signature;
    uint8_t _reserved1[480];
    uint32_t mid_signature;
    uint32_t last_known_free_cluster;
    uint32_t available_clusters_hint;
    uint8_t _reserved2[12];
    uint32_t trail_signature;
} __attribute__((packed)) fat32_fsinfo_t;
static_assert(sizeof(fat32_fsinfo_t) == 512, "Invalid Size");

typedef struct {
    char name[8];
    char ext[3];
    uint8_t attributes;
    uint8_t _reserved;
    uint8_t creation_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_hi;
    uint16_t modification_time;
    uint16_t modification_date;
    uint16_t cluster_lo;
    uint32_t filesize;
} __attribute__((packed)) fat32_directory_entry_t;
static_assert(sizeof(fat32_directory_entry_t) == 32, "Invalid Size");

typedef struct {
    uint8_t order;
    uint16_t group_1[5];
    uint8_t attributes;
    uint8_t entry_type;
    uint8_t short_name_checksum;
    uint16_t group_2[6];
    uint16_t _reserved;
    uint16_t group_3[2];
} __attribute__((packed)) fat32_lfn_entry_t;
static_assert(sizeof(fat32_lfn_entry_t) == 32, "Invalid Size");

#define FAT32_ATTR_READONLY  0x01
#define FAT32_ATTR_HIDDEN    0x02
#define FAT32_ATTR_SYSTEM    0x04
#define FAT32_ATTR_VOLUMEID  0x08
#define FAT32_ATTR_LFN       0x0F
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE   0x20
#define FAT32_ATTR_DEVICE    0x40
#define FAT32_ATTR_RESERVED  0x80

typedef struct {
    uint8_t disk;
    uint32_t first_sector;

    char volume_id[12];
    uint32_t serial_number;
    uint32_t cluster_size;
    uint32_t fat_size;
    uint32_t first_data_sector;
    uint32_t first_fat_sector;
    uint32_t root_cluster;
} fat32_volume_t;

typedef struct {
    fat32_volume_t *volume;

    uint32_t starting_cluster;
    uint32_t current_cluster;
    uint32_t file_offset;
    uint32_t file_size;
} fat32_file_t;

extern uint32_t disk_read_sectors(uint8_t disk, uint8_t *buffer, uint32_t sector, uint32_t count);

bool fat32_open_volume(fat32_volume_t *this, uint8_t disk, uint32_t first_sector);
int fat32_read_directory(fat32_volume_t *this, fat32_directory_entry_t *entry_buf, uint32_t cluster);

#define FAT32_SEEK_SET 0
#define FAT32_SEEK_CUR 1

bool fat32_file_open(fat32_volume_t *this, fat32_file_t *file_ptr, fat32_directory_entry_t *dir_entry);
uint32_t fat32_file_read(fat32_file_t *this, uint8_t *buf, uint32_t bytes);
int fat32_file_seek(fat32_file_t *this, int offset, int origin);

#endif //LOADER_DISK_H
