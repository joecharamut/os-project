#include <debug/debug.h>
#include <io/port.h>
#include <debug/panic.h>
#include <dev/pci.h>
#include <std/string.h>
#include <debug/assert.h>
#include "ide.h"

typedef enum {
    ATA_OFFSET_DATA         = 0,
    ATA_OFFSET_ERROR        = 1,
    ATA_OFFSET_SECTOR_COUNT = 2,
    ATA_OFFSET_LBA_LO       = 3,
    ATA_OFFSET_LBA_MID      = 4,
    ATA_OFFSET_LBA_HI       = 5,
    ATA_OFFSET_DRIVE_SELECT = 6,
    ATA_OFFSET_COMMAND      = 7,
} ata_bus_offset_t;

typedef union {
    struct {
        u8 lba1 : 8;
        u8 lba2 : 8;
        u8 lba3 : 8;
        u8 lba4 : 8;
        u8 lba5 : 8;
        u8 lba6 : 8;
        u16 _unused : 16;
    } bytes;
    u64 value;
} ata_lba_t;

void ata_send_command(ata_bus_t bus, ata_command_t command) {
    outb(bus + ATA_OFFSET_COMMAND, command);
}

ata_status_t ata_read_status(ata_bus_t bus) {
    return (ata_status_t) { .value = inb(bus + ATA_OFFSET_COMMAND) };
}

ata_status_t ata_poll_device(ata_bus_t bus) {
    ata_status_t status;

    do {
        status = ata_read_status(bus);
        asm volatile ("pause");
    } while (status.fields.busy && !(status.fields.error || status.fields.drive_request || status.fields.drive_fault));

    return status;
}

void ata_read_sectors(ata_bus_t bus, ata_drive_t drive, u64 lba_value, u16 count, u16 *buffer) {
    dbg_logf(LOG_DEBUG, "ATA: Reading %d sectors, starting at %lld\n", count, lba_value);
    ata_lba_t lba = (ata_lba_t) { .value = lba_value };

    // use lba instead of chs
    outb(bus + ATA_OFFSET_DRIVE_SELECT, drive == ATA_PRIMARY_DRIVE ? 0x40 : 0x50);

    outb(bus + ATA_OFFSET_SECTOR_COUNT, (count >> 8) & 0xFF);
    outb(bus + ATA_OFFSET_LBA_LO, lba.bytes.lba4);
    outb(bus + ATA_OFFSET_LBA_MID, lba.bytes.lba5);
    outb(bus + ATA_OFFSET_LBA_HI, lba.bytes.lba6);

    outb(bus + ATA_OFFSET_SECTOR_COUNT, (count >> 0) & 0xFF);
    outb(bus + ATA_OFFSET_LBA_LO, lba.bytes.lba1);
    outb(bus + ATA_OFFSET_LBA_MID, lba.bytes.lba2);
    outb(bus + ATA_OFFSET_LBA_HI, lba.bytes.lba3);

    outb(bus + ATA_OFFSET_COMMAND, ATA_READ_SECTORS_EXT);

    int sectors_read = 0;
    do {
        ata_poll_device(bus);
        for (int i = 0; i < 256; i++) {
            buffer[i + (sectors_read * 256)] = inw(bus + ATA_OFFSET_DATA);
        }
        ++sectors_read;
    } while (sectors_read < count);
}

int ata_identify(ata_bus_t bus, ata_drive_t drive, u16 *buffer) {
    outb(bus + ATA_OFFSET_DRIVE_SELECT, drive);
    outb(bus + ATA_OFFSET_SECTOR_COUNT, 0);
    outb(bus + ATA_OFFSET_LBA_LO, 0);
    outb(bus + ATA_OFFSET_LBA_MID, 0);
    outb(bus + ATA_OFFSET_LBA_HI, 0);
    outb(bus + ATA_OFFSET_COMMAND, ATA_IDENTIFY_DEVICE);

    if (inb(bus + ATA_OFFSET_COMMAND) == 0) {
        return -1;
    }

    while (ata_read_status(bus).fields.busy); // wait until not busy

    u8 lba_mid = inb(bus + ATA_OFFSET_LBA_MID);
    u8 lba_hi = inb(bus + ATA_OFFSET_LBA_HI);
    if (lba_hi || lba_mid) {
        if (lba_mid == 0x14 && lba_hi == 0xEB) {
            dbg_logf(LOG_WARN, "TODO: ATAPI Device\n");
        } else {
            dbg_logf(LOG_ERROR, "Not ATA: 0x%x 0x%x\n", lba_hi, lba_mid);
        }
        return -2;
    }

    if (ata_poll_device(ATA_PRIMARY_BUS).fields.error) {
        dbg_logf(LOG_ERROR, "Drive Reported Error\n");
        return -3;
    }

    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(bus + ATA_OFFSET_DATA);
    }

    if ((buffer[83] & (1 << 10)) == 0) {
        dbg_logf(LOG_ERROR, "Drive does not support LBA48\n");
        return -4;
    }

    return 0;
}

ide_drives_t ide_enumerate_drives() {
    ide_drives_t drives = { 0 };
    u16 ata_buffer[256];

    assert(sizeof(mbr_partition_entry_t) == 16);

    if (ata_identify(ATA_PRIMARY_BUS, ATA_PRIMARY_DRIVE, ata_buffer) == 0) {
        ata_read_sectors(ATA_PRIMARY_BUS, ATA_PRIMARY_DRIVE, 0, 1, ata_buffer);
        if (ata_buffer[255] == 0xAA55) {
            drives.drives[0] = (mbr_drive_t) {
                    .valid = true,
                    .ata_bus = ATA_PRIMARY_BUS,
                    .ata_drive = ATA_PRIMARY_DRIVE
            };
            memcpy(&drives.drives[0].partition_info, &ata_buffer[223], 64);
        }
    }

    if (ata_identify(ATA_PRIMARY_BUS, ATA_SECONDARY_DRIVE, ata_buffer) == 0) {
        ata_read_sectors(ATA_PRIMARY_BUS, ATA_SECONDARY_DRIVE, 0, 1, ata_buffer);
        if (ata_buffer[255] == 0xAA55) {
            drives.drives[1] = (mbr_drive_t) {
                    .valid = true,
                    .ata_bus = ATA_PRIMARY_BUS,
                    .ata_drive = ATA_SECONDARY_DRIVE
            };
            memcpy(&drives.drives[1].partition_info, &ata_buffer[223], 64);
        }
    }

    if (ata_identify(ATA_SECONDARY_BUS, ATA_PRIMARY_DRIVE, ata_buffer) == 0) {
        ata_read_sectors(ATA_SECONDARY_BUS, ATA_PRIMARY_DRIVE, 0, 1, ata_buffer);
        if (ata_buffer[255] == 0xAA55) {
            drives.drives[2] = (mbr_drive_t) {
                    .valid = true,
                    .ata_bus = ATA_SECONDARY_BUS,
                    .ata_drive = ATA_PRIMARY_DRIVE
            };
            memcpy(&drives.drives[2].partition_info, &ata_buffer[223], 64);
        }
    }

    if (ata_identify(ATA_SECONDARY_BUS, ATA_SECONDARY_DRIVE, ata_buffer) == 0) {
        ata_read_sectors(ATA_SECONDARY_BUS, ATA_SECONDARY_DRIVE, 0, 1, ata_buffer);
        if (ata_buffer[255] == 0xAA55) {
            drives.drives[3] = (mbr_drive_t) {
                    .valid = true,
                    .ata_bus = ATA_SECONDARY_BUS,
                    .ata_drive = ATA_SECONDARY_DRIVE
            };
            memcpy(&drives.drives[3].partition_info, &ata_buffer[223], 64);
        }
    }

    return drives;
}

bool ide_init() {
    bool usable_controller = false;
    for (size_t i = 0; i < pci_num_devs(); i++) {
        PCI_Header hdr = pci_dev_list()[i];
        if (hdr.classCode == PCI_MASS_STORAGE_CONTROLLER && hdr.subclass == 0x01) {
            if ((hdr.programmingInterface & 1) == 0) {
                usable_controller = true;
            }
        }
    }

    if (!usable_controller) {
        dbg_logf(LOG_FATAL, "No Supported IDE Controllers\n");
        return false;
    }

    return true;
}
