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
    } PACKED fields;
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

bool ide_init();

#endif //OS_IDE_H
