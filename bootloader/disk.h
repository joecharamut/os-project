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
    uint16_t reserved_sectors;
    uint8_t number_of_fats;
    uint16_t number_of_directory_entries;
    uint16_t total_sectors;
    uint8_t media_type;
    uint8_t _reserved1[2]; // FAT12/FAT16 Number of sectors per FAT
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;

    // EBR
    uint32_t sectors_per_fat;
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
    uint8_t disk;
    uint32_t first_sector;

} fat32_volume_t;

extern uint32_t disk_read_sectors(uint8_t disk, uint8_t *buffer, uint32_t sector, uint32_t count);
bool fat32_open_volume(fat32_volume_t *this, uint8_t disk, uint32_t first_sector);

#endif //LOADER_DISK_H
