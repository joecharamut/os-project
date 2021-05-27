#ifndef OS_IDE_H
#define OS_IDE_H

#include <stdbool.h>

typedef union {
    struct {
        bool error: 1;
        bool index: 1;
        bool corrected: 1;
        bool drive_request: 1;
        bool service_request: 1;
        bool drive_fault: 1;
        bool ready: 1;
        bool busy: 1;
    } __attribute__((packed)) fields;
    u8 value;
} ata_status_t;

typedef enum {
    ATA_PRIMARY_BUS = 0x1F0,
    ATA_SECONDARY_BUS = 0x170,
} ata_bus_t;

typedef enum {
    ATA_PRIMARY_DRIVE = 0xA0,
    ATA_SECONDARY_DRIVE = 0xB0,
} ata_drive_t;

typedef enum {
    ATA_READ_SECTORS_EXT = 0x24,
    ATA_IDENTIFY_DEVICE = 0xEC,
} ata_command_t;

typedef struct mbr_partition_entry {
    u8 status;
    struct {
        u8 head : 8;
        u8 sector : 6;
        u16 cylinder : 10;
    } __attribute__((packed)) chs_first_sector;
    u8 partition_type;
    struct {
        u8 head : 8;
        u8 sector : 6;
        u16 cylinder : 10;
    } __attribute__((packed)) chs_last_sector;
    u32 lba_first_sector;
    u32 sector_count;
} __attribute__((packed)) mbr_partition_entry_t;

typedef struct mbr_drive {
    bool valid;
    u16 ata_bus;
    u8 ata_drive;
    mbr_partition_entry_t partition_info[4];
} mbr_drive_t;

typedef struct ide_drives {
    mbr_drive_t drives[4];
} ide_drives_t;

void ata_send_command(ata_bus_t bus, ata_command_t command);
ata_status_t ata_read_status(ata_bus_t bus);
ata_status_t ata_poll_device(ata_bus_t bus);
void ata_read_sectors(ata_bus_t bus, ata_drive_t drive, u64 lba_value, u16 count, u16 *buffer);
int ata_identify(ata_bus_t bus, ata_drive_t drive, u16 *buffer);

ide_drives_t ide_enumerate_drives();
bool ide_init();

#endif //OS_IDE_H
