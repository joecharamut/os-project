#include <bootstrap/debug/debug.h>
#include <bootstrap/debug/term.h>
#include <bootstrap/debug/serial.h>
#include <bootstrap/dev/pci.h>
#include <bootstrap/io/port.h>
#include <stdbool.h>
#include <bootstrap/dev/ide.h>
#include <bootstrap/types.h>
#include <bootstrap/mm/paging.h>
#include "gdt.h"

uint32_t kernel_size() {
    extern u32 _kernel_base;
    extern u32 _kernel_end;
    return (u32) &_kernel_end - (u32) &_kernel_base;
}

void _init(void) {
    serial_init();
    serial_write("Hello from early init!\n");

    // fix paging
    paging_main_init();

    term_init();

    map_page(0x4000000, 0x41414141, false, true);
    while(1);
    unmap_page(0x41414141);

    term_enable_cursor(0, 14);
    term_update_cursor(0, 0);

    dbg_logf(LOG_INFO, "Welcome to {OS_NAME} Bootstrap Loader\n");

    dbg_logf(LOG_DEBUG, "Kernel size: 0x%x\n", kernel_size());

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
