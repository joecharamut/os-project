#include <debug/serial.h>
#include <debug/term.h>
#include <debug/debug.h>
#include <io/port.h>
#include <io/timer.h>
#include <dev/pci.h>
#include <dev/ide.h>
#include <mm/paging.h>
#include <std/string.h>
#include <std/stdlib.h>
#include <debug/panic.h>

void kernel_main() {
    dbg_logf(LOG_INFO, "Welcome to {OS_NAME} Bootstrap Loader\n");

    dbg_logf(LOG_INFO, "Initializing PCI...\n");
    if (!pci_init()) {
        dbg_logf(LOG_FATAL, "BOOT FAILURE: PCI Initialization Failed\n");
        return;
    }
    dbg_logf(LOG_INFO, "Bus Scan found %d device(s).\n", pci_num_devs());

    bool found_storage = false;
    for (size_t i = 0; i < pci_num_devs(); i++) {
        PCI_Header hdr = pci_dev_list()[i];
        if (hdr.classCode == PCI_MASS_STORAGE_CONTROLLER) {
            found_storage = true;
        }
        dbg_logf(LOG_DEBUG, "PCI Dev %d: %x:%x, %x:%x:%x\n", i, hdr.vendorId, hdr.deviceId, hdr.classCode, hdr.subclass, hdr.programmingInterface);
    }

    if (!found_storage) {
        dbg_logf(LOG_FATAL, "BOOT FAILURE: No Storage Devices Detected\n");
        return;
    }

    dbg_logf(LOG_INFO, "Initializing Storage Controller(s)...\n");
    if (!ide_init()) {
        dbg_logf(LOG_FATAL, "IDE Controller Initialization Failed\n");
        return;
    }

    dbg_printf("Hello World!\n");
}
