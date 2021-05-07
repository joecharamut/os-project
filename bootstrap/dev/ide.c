#include <debug/debug.h>
#include <io/port.h>
#include <debug/serial.h>
#include "ide.h"
#include "pci.h"

#define IDE_PCI_NATIVE_MODE_PRIMARY 1 << 0
#define IDE_PCI_NATIVE_MODE_PRIMARY_WRITEABLE 1 << 1
#define IDE_PCI_NATIVE_MODE_SECONDARY 1 << 2
#define IDE_PCI_NATIVE_MODE_SECONDARY_WRITEABLE 1 << 3
#define IDE_PCI_BUS_MASTER_CONTROLLER 1 << 7

#define IDE_PRIMARY_CONTROL_REGISTER    0x3F6
#define IDE_PRIMARY_DATA_PORT           0x1F0
#define IDE_PRIMARY_ERROR_REGISTER      0x1F1
#define IDE_PRIMARY_SECTORCOUNT         0x1F2
#define IDE_PRIMARY_LBA_LO              0x1F3
#define IDE_PRIMARY_LBA_MID             0x1F4
#define IDE_PRIMARY_LBA_HI              0x1F5
#define IDE_PRIMARY_DRIVE_SELECT        0x1F6
#define IDE_PRIMARY_COMMAND_IO          0x1F7

#define ATA_COMMAND_READ_SECTORS_EXT    0x24
#define ATA_COMMAND_IDENTIFY_DEVICE     0xEC

#define ATA_STATUS_ERR                  1 << 0
#define ATA_STATUS_DRQ                  1 << 3
#define ATA_STATUS_BSY                  1 << 7

static volatile bool irq_flag = false;
void ide_irq14_handler() {
    dbg_logf(LOG_TRACE, "irq14\n");
    irq_flag = false;
}
void wait_irq() {
    irq_flag = true;
    while(irq_flag);
}

static uint16_t transfer_buffer[256];
void ata_read_sectors(uint16_t sectorCount, uint64_t lba) {
    uint8_t sectorCount_hi = (sectorCount >> 8) & 0xff;
    uint8_t sectorCount_lo = (sectorCount >> 0) & 0xff;
    uint8_t lba1 = (lba >> 0) & 0xff;
    uint8_t lba2 = (lba >> 8) & 0xff;
    uint8_t lba3 = (lba >> 16) & 0xff;
    uint8_t lba4 = (lba >> 24) & 0xff;
    uint8_t lba5 = (lba >> 32) & 0xff;
    uint8_t lba6 = (lba >> 40) & 0xff;

    outb(IDE_PRIMARY_DRIVE_SELECT, 0x40);
    outb(IDE_PRIMARY_SECTORCOUNT, sectorCount_hi);
    outb(IDE_PRIMARY_LBA_LO, lba4);
    outb(IDE_PRIMARY_LBA_MID, lba5);
    outb(IDE_PRIMARY_LBA_HI, lba6);
    outb(IDE_PRIMARY_SECTORCOUNT, sectorCount_lo);
    outb(IDE_PRIMARY_LBA_LO, lba1);
    outb(IDE_PRIMARY_LBA_MID, lba2);
    outb(IDE_PRIMARY_LBA_HI, lba3);
    outb(IDE_PRIMARY_COMMAND_IO, ATA_COMMAND_READ_SECTORS_EXT);
    wait_irq();

    for (int i = 0; i < 256; i++) {
        transfer_buffer[i] = inw(IDE_PRIMARY_DATA_PORT);
    }
}

bool ata_identify_device() {
    outb(IDE_PRIMARY_DRIVE_SELECT, 0xA0);
    outb(IDE_PRIMARY_SECTORCOUNT, 0x00);
    outb(IDE_PRIMARY_LBA_LO, 0x00);
    outb(IDE_PRIMARY_LBA_MID, 0x00);
    outb(IDE_PRIMARY_LBA_HI, 0x00);
    outb(IDE_PRIMARY_COMMAND_IO, ATA_COMMAND_IDENTIFY_DEVICE);

    if (inb(IDE_PRIMARY_COMMAND_IO) == 0) {
        return false;
    } else {
        while ((inb(IDE_PRIMARY_COMMAND_IO) & ATA_STATUS_BSY) != 0); // wait for BSY clear
    }

    uint8_t lbamid = inb(IDE_PRIMARY_LBA_MID);
    uint8_t lbahi = inb(IDE_PRIMARY_LBA_HI);
    if (lbamid != 0 || lbahi != 0) {
        dbg_logf(LOG_ERROR, "Not ATA: 0x%x 0x%x\n", lbahi, lbamid);
        return false;
    }

    uint8_t status = 0;
    while (((status = inb(IDE_PRIMARY_COMMAND_IO)) & (ATA_STATUS_DRQ | ATA_STATUS_ERR)) == 0); // wait for DRQ or ERR to set

    if (status & ATA_STATUS_ERR) {
        dbg_logf(LOG_ERROR, "Drive Reported Error\n");
        return false;
    }

    for (int i = 0; i < 256; i++) {
        transfer_buffer[i] = inw(IDE_PRIMARY_DATA_PORT);
    }

    return true;
}

bool ide_init() {
    bool usable_controller = false;
    for (size_t i = 0; i < pci_num_devs(); i++) {
        PCI_Header hdr = pci_dev_list()[i];
        if (hdr.classCode == PCI_MASS_STORAGE_CONTROLLER && hdr.subclass == 0x01) {
            if (!(hdr.programmingInterface & IDE_PCI_NATIVE_MODE_PRIMARY)) {
                usable_controller = true;
            }
        }
    }

    if (!usable_controller) {
        dbg_logf(LOG_FATAL, "No IDE Controllers in Compatibility mode, bailing out\n");
        return false;
    }

    if (ata_identify_device()) {
        if (transfer_buffer[83] & (1 << 10)) {
            dbg_logf(LOG_DEBUG, "ATA Identify Success\n");
            uint64_t word0 = transfer_buffer[100];
            uint64_t word1 = transfer_buffer[101];
            uint64_t word2 = transfer_buffer[102];
            uint64_t word3 = transfer_buffer[103];

            uint64_t sectors = (word0 << 0) | (word1 << 16) | (word2 << 32) | (word3 << 48);
            dbg_logf(LOG_DEBUG, "drive supports LBA48, sectors: %lld\n", sectors);
            ata_read_sectors(1, 0);

            dbg_printf("Buffer data: ");
            for (int i = 0; i < 256; i++) {
                dbg_printf("%x ", transfer_buffer[i]);
            }
            dbg_printf("\n");
        } else {
            dbg_logf(LOG_DEBUG, "drive does not support LBA48\n");
            return false;
        }
    } else {
        dbg_logf(LOG_FATAL, "ATA Identify Failed\n");
        return false;
    }
//
//    if (inb(IDE_PRIMARY_COMMAND_IO) == 0) {
//
//        return false;
//    } else {
//        dbg_logf(LOG_DEBUG, "perhaps ide here\n");
//        while ((inb(IDE_PRIMARY_COMMAND_IO) & ATA_STATUS_BSY) != 0); // wait for BSY clear
//
//        uint8_t lbamid = inb(IDE_PRIMARY_LBA_MID);
//        uint8_t lbahi = inb(IDE_PRIMARY_LBA_HI);
//        if (lbamid != 0 || lbahi != 0) {
//            dbg_logf(LOG_FATAL, "not ata: 0x%x 0x%x\n", lbahi, lbamid);
//            return false;
//        }
//
//        uint8_t status = 0;
//        while (((status = inb(IDE_PRIMARY_COMMAND_IO)) & (ATA_STATUS_DRQ | ATA_STATUS_ERR)) == 0); // wait for DRQ or ERR to set
//
//        if (status & ATA_STATUS_ERR) {
//            dbg_logf(LOG_FATAL, "Drive Reported Error\n");
//            return false;
//        }
//
//
//
//
//
//        dbg_logf(LOG_DEBUG, "check end\n");
//    }


    return false;
}
