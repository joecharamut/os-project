#include <debug/term.h>
#include <debug/debug.h>
#include <dev/pci.h>
#include <io/acpi.h>
#include <fs/ide.h>
#include <debug/assert.h>
#include <fs/ext2.h>
#include <std/string.h>
#include <mm/kmem.h>
#include <cpuid.h>
#include <std/math.h>

void kernel_main() {
    dbg_logf(LOG_INFO, "Welcome to {OS_NAME} Bootstrap Loader\n");

    unsigned int eax, ebx, ecx, edx;
    if (__get_cpuid(0, &eax, &ebx, &ecx, &edx) == 0) {
        dbg_logf(LOG_FATAL, "BOOT FAILURE: No CPUID support\n");
        return;
    }

    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    if ((edx & 1) == 0) {
        dbg_logf(LOG_FATAL, "BOOT FAILURE: No x87 FPU\n");
        return;
    }

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

    ide_drives_t drives = ide_enumerate_drives();
    mbr_drive_t drive;
    int partition = -1;

    for (int i = 0; i < 4; ++i) {
        if (drives.drives[i].valid) {
            for (int j = 0; j < 4; ++j) {
                dbg_logf(LOG_DEBUG, "drive %d part %d type 0x%x\n", i, j, drives.drives[i].partition_info[j].partition_type);
                if (drives.drives[i].partition_info[j].partition_type == 0x83) {
                    drive = drives.drives[i];
                    partition = j;
                    goto found_drive;
                }
            }
        }
    }

found_drive:
    if (partition == -1) {
        dbg_logf(LOG_FATAL, "Could not find any EXT2 Partitions\n");
        return;
    }

    ext2_volume_t *volume = ext2_open_volume(drive, partition);
    dbg_logf(LOG_DEBUG, "Found EXT2 Volume, Name: '%s'\n", volume->superblock.volume_name);

    ext2_file_t *fp = ext2_fopen(volume, "/HELLO.TXT");
    u8 *buf = kcalloc(16, sizeof(char));
    if (fp) {
        u32 read;
        while ((read = ext2_fread(buf, 16, fp)) > 0) {
//            dbg_logf(LOG_DEBUG, "Read %d bytes: [", read);
//            for (int i = 0; i < 16; ++i) {
//                dbg_printf("%c", buf[i]);
//            }
//            dbg_printf("]\n");
        }
    }

    dbg_printf("Hello World!\n");
}
