#include "pci.h"
#include <bootstrap/debug/debug.h>
#include <stdint.h>
#include <bootstrap/io/port.h>
#include <stdbool.h>
#include <bootstrap/types.h>

#define PCI_ADDRESS(bus, device, function, offset) ((0x80000000) | (bus << 16) | (device << 11) | (function << 8) | (offset))
#define PCI_ADDRESS_PORT 0xCF8
#define PCI_DATA_PORT 0xCFC

u32 pciConfigReadLong(u8 bus, u8 device, u8 func, u8 offset) {
    outl(PCI_ADDRESS_PORT, PCI_ADDRESS(bus, device, func, offset));
    return inl(PCI_DATA_PORT);
}

void pciConfigWriteLong(u8 bus, u8 device, u8 func, u8 offset, u32 data) {
    outl(PCI_ADDRESS_PORT, PCI_ADDRESS(bus, device, func, offset));
    outl(PCI_DATA_PORT, data);
}

u16 pciConfigReadWord(u8 bus, u8 device, u8 func, u8 offset) {
    outl(PCI_ADDRESS_PORT, PCI_ADDRESS(bus, device, func, offset));
    return (u16) inw(PCI_DATA_PORT + (offset & 2));
}

void pciConfigWriteWord(u8 bus, u8 device, u8 func, u8 offset, u16 data) {
    outl(PCI_ADDRESS_PORT, PCI_ADDRESS(bus, device, func, offset));
    outw(PCI_DATA_PORT + (offset & 2), data);
}

uint8_t pciConfigReadByte(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    outl(PCI_ADDRESS_PORT, PCI_ADDRESS(bus, device, func, offset));
    return (u8) inb(PCI_DATA_PORT + (offset & 3));
}

void pciConfigWriteByte(u8 bus, u8 device, u8 func, u8 offset, u8 data) {
    outl(PCI_ADDRESS_PORT, PCI_ADDRESS(bus, device, func, offset));
    outb(PCI_DATA_PORT + (offset & 3), data);
}

uint16_t pciReadVendorId(uint8_t bus, uint8_t dev, uint8_t func) {
    return pciConfigReadWord(bus, dev, func, 0);
}

void pciGetHeader(uint8_t bus, uint8_t device, uint8_t func, PCI_Header *header) {
    header->vendorId = pciConfigReadWord(bus, device, func, 0x00);
    header->deviceId = pciConfigReadWord(bus, device, func, 0x02);

    header->command = pciConfigReadWord(bus, device, func, 0x04);
    header->status = pciConfigReadWord(bus, device, func, 0x06);

    header->revision = pciConfigReadByte(bus, device, func, 0x08);
    header->programmingInterface = pciConfigReadByte(bus, device, func, 0x09);
    header->subclass = pciConfigReadByte(bus, device, func, 0x0A);
    header->classCode = pciConfigReadByte(bus, device, func, 0x0B);

    header->cacheLineSize = pciConfigReadByte(bus, device, func, 0x0C);
    header->latencyTimer = pciConfigReadByte(bus, device, func, 0x0D);
    header->headerType = pciConfigReadByte(bus, device, func, 0x0E);
    header->selfTest = pciConfigReadByte(bus, device, func, 0x0F);

    if (header->headerType == 0) {
        // type 0 header
        header->baseAddress0 = pciConfigReadLong(bus, device, func, 0x10);
        header->baseAddress1 = pciConfigReadLong(bus, device, func, 0x14);
        header->baseAddress2 = pciConfigReadLong(bus, device, func, 0x18);
        header->baseAddress3 = pciConfigReadLong(bus, device, func, 0x1C);
        header->baseAddress4 = pciConfigReadLong(bus, device, func, 0x20);
        header->baseAddress5 = pciConfigReadLong(bus, device, func, 0x24);
        header->cardbusInfoPointer = pciConfigReadLong(bus, device, func, 0x28);

        header->subsystemVendorId = pciConfigReadWord(bus, device, func, 0x2C);
        header->subsystemId = pciConfigReadWord(bus, device, func, 0x2E);

        header->expansionRomAddress = pciConfigReadLong(bus, device, func, 0x30);

        header->capabilitiesPointer = pciConfigReadByte(bus, device, func, 0x34);
    }
}

static size_t dev_count = 0;
static PCI_Header devices[255]; // todo: malloc?
bool pci_init() {
    uint8_t device;
    uint16_t bus;

    for(bus = 0; bus < 256; bus++) {
        for(device = 0; device < 32; device++) {
            if (pciReadVendorId(bus, device, 0) == 0xffff) continue;

            for (uint8_t func = 0; func < 8; func++) {
                if (pciReadVendorId(bus, device, func) == 0xffff) break;
                PCI_Header *header = &devices[dev_count];
                pciGetHeader(bus, device, func, header);

                dbg_logf(LOG_TRACE, "PCI: bus %d, dev: %x, func: %d\n", bus, device, func);
                dbg_logf(LOG_TRACE, "  vnd id: %x\n", header->vendorId);
                dbg_logf(LOG_TRACE, "  dev id: %x\n", header->deviceId);
                dbg_logf(LOG_TRACE, "  class: %x:%x:%x\n", header->classCode, header->subclass, header->programmingInterface);
                dbg_logf(LOG_TRACE, "  status: %x\n", header->status);

                if (header->status & PCI_STATUS_HAS_CAPABILITIES_LIST) {
                    u8 next = header->capabilitiesPointer;
                    u8 id = 0;

                    while (next != 0) {
                        id = pciConfigReadByte(bus, device, func, next);
                        next = pciConfigReadByte(bus, device, func, next+1);
                        dbg_logf(LOG_DEBUG, "  cap id: %x\n", id);
                    }
                }

                dev_count++;

                // if not multi-function device, break
                if (func == 0 && !(header->headerType & 0x80)) break;
            }
        }
    }

    return true;
}

size_t pci_num_devs() {
    return dev_count;
}

PCI_Header *pci_dev_list() {
    return devices;
}
