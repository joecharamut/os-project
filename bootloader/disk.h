#ifndef LOADER_DISK_H
#define LOADER_DISK_H

#include <stdint.h>
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
    uint16_t copy_protected;
    disk_partition_entry_t partitions[4];
    uint16_t signature;
} __attribute__((packed)) disk_mbr_t;
static_assert(sizeof(disk_mbr_t) == 512, "Invalid Size");

extern uint32_t disk_read_sectors(uint8_t disk, uint8_t *buffer, uint32_t sector, uint32_t count);

#endif //LOADER_DISK_H
