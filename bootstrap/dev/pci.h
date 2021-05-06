#ifndef OS_PCI_H
#define OS_PCI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PCI_COMMAND_INTERRUPT_DISABLE           1 << 10
#define PCI_COMMAND_FAST_BACK_TO_BACK           1 << 09
#define PCI_COMMAND_SERR_ENABLE                 1 << 08
#define PCI_COMMAND_PARITY_ERROR_RESPONSE       1 << 06
#define PCI_COMMAND_VGA_PALETTE_SNOOP           1 << 05
#define PCI_COMMAND_MEMORY_WRITE_AND_INVALIDATE 1 << 04
#define PCI_COMMAND_SPECIAL_CYCLES              1 << 03
#define PCI_COMMAND_BUS_MASTER                  1 << 02
#define PCI_COMMAND_MEMORY_SPACE                1 << 01
#define PCI_COMMAND_IO_SPACE                    1 << 00

#define PCI_STATUS_PARITY_ERROR_DETECTED        1 << 15
#define PCI_STATUS_SYSTEM_ERROR_SIGNALED        1 << 14
#define PCI_STATUS_MASTER_ABORT_RECEIVED        1 << 13
#define PCI_STATUS_TARGET_ABORT_RECEIVED        1 << 12
#define PCI_STATUS_TARGET_ABORT_SIGNAL          1 << 11
#define PCI_STATUS_DEVSEL_HI                    1 << 10
#define PCI_STATUS_DEVSEL_LO                    1 << 09
#define PCI_STATUS_MASTER_DATA_PARITY_ERROR     1 << 08
#define PCI_STATUS_FAST_BACK_TO_BACK_CAPABLE    1 << 07
#define PCI_STATUS_66_MHZ_CAPABLE               1 << 05
#define PCI_STATUS_HAS_CAPABILITIES_LIST        1 << 04
#define PCI_STATUS_INTERRUPT_STATUS             1 << 03

typedef enum PCI_CLASS_CODE {
    PCI_UNDEFINED_CLASS                     = 0x00,
    PCI_MASS_STORAGE_CONTROLLER             = 0x01,
    PCI_NETWORK_CONTROLLER                  = 0x02,
    PCI_DISPLAY_CONTROLLER                  = 0x03,
    PCI_MULTIMEDIA_DEVICE                   = 0x04,
    PCI_MEMORY_CONTROLLER                   = 0x05,
    PCI_BRIDGE_DEVICE                       = 0x06,
    PCI_SIMPLE_COMMUNICATION_CONTROLLER     = 0x07,
    PCI_BASE_SYSTEM_PERIPHERAL              = 0x08,
    PCI_INPUT_DEVICE                        = 0x09,
    PCI_DOCKING_STATION                     = 0x0a,
    PCI_PROCESSOR                           = 0x0b,
    PCI_SERIAL_BUS_CONTROLLER               = 0x0c,
    PCI_WIRELESS_CONTROLLER                 = 0x0d,
    PCI_INTELLIGENT_I_O_CONTROLLER          = 0x0e,
    PCI_SATELLITE_COMMUNICATION_CONTROLLER  = 0x0f,
    PCI_CRYPTOGRAPHIC_CONTROLLER            = 0x10,
    PCI_DSP_CONTROLLER                      = 0x11,
    PCI_PROCESSING_ACCELERATOR              = 0x12,
    PCI_NON_ESSENTIAL_INSTRUMENTATION       = 0x13,
    /* 0x14 - 0x3f Reserved */
    PCI_COPROCESSOR                         = 0x40,
    /* 0x41 - 0xfe Reserved */
    PCI_OTHER_DEVICE                        = 0xff,
} PCI_CLASS_CODE;

typedef struct PCI_Header {
    uint16_t vendorId;
    uint16_t deviceId;
    uint16_t command;
    uint16_t status;
    uint8_t revision;
    uint8_t programmingInterface;
    uint8_t subclass;
    uint8_t classCode;
    uint8_t cacheLineSize;
    uint8_t latencyTimer;
    uint8_t headerType;
    uint8_t selfTest;

    // only valid on header type 0
    uint32_t baseAddress0;
    uint32_t baseAddress1;
    uint32_t baseAddress2;
    uint32_t baseAddress3;
    uint32_t baseAddress4;
    uint32_t baseAddress5;
    uint32_t cardbusInfoPointer;
    uint16_t subsystemVendorId;
    uint16_t subsystemId;
    uint32_t expansionRomAddress;
    uint8_t capabilitiesPointer;
    uint8_t interruptLine;
    uint8_t interruptPin;
    uint8_t minGrant;
    uint8_t maxLatency;
} PCI_Header;

bool pci_init();
size_t pci_num_devs();
PCI_Header *pci_dev_list();

#endif //OS_PCI_H
