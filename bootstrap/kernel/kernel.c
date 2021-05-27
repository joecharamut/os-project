#include <debug/term.h>
#include <debug/debug.h>
#include <dev/pci.h>
#include <io/acpi.h>
#include <fs/ide.h>
#include <fs/fat32.h>
#include <debug/assert.h>

void kernel_main() {
    dbg_logf(LOG_INFO, "Welcome to {OS_NAME} Bootstrap Loader\n");

    init_acpi();

    dbg_logf(LOG_INFO, "Initializing PCI...\n");
    if (!pci_init()) {
        dbg_logf(LOG_FATAL, "BOOT FAILURE: PCI Initialization Failed\n");
        return;
    }
    dbg_logf(LOG_INFO, "Bus Scan found %d device(s).\n", pci_num_devs());

    for (size_t i = 0; i < pci_num_devs(); i++) {
        PCI_Header hdr = pci_dev_list()[i];
        dbg_logf(LOG_DEBUG, "PCI Dev %d: %x:%x, %x:%x:%x\n", i, hdr.vendorId, hdr.deviceId, hdr.classCode, hdr.subclass, hdr.programmingInterface);
    }

    dbg_logf(LOG_INFO, "Initializing Storage...\n");
    if (!ide_init()) {
        dbg_logf(LOG_FATAL, "IDE Controller Initialization Failed\n");
        return;
    }
    if (!fat32_find_partitions()) {
        dbg_logf(LOG_FATAL, "Could not find any FAT32 Partitions\n");
        return;
    }

    dbg_printf("Hello World!\n");
}
