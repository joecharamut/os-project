#include "disk.h"

bool fat32_load_vbr(fat32_volume_t *this) {
    uint8_t vbr_buffer[512];
    fat32_vbr_t *vbr = (fat32_vbr_t *) &vbr_buffer;

    if (disk_read_sectors(this->disk, vbr_buffer, this->first_sector, 1)) {
        print_str("Read Error\n");
        return false;
    }

    uint8_t fsinfo_buffer[512];
    fat32_fsinfo_t *fsinfo = (fat32_fsinfo_t *) &fsinfo_buffer;

    if (disk_read_sectors(this->disk, fsinfo_buffer, this->first_sector + vbr->fsinfo_sector, 1)) {
        print_str("Read Error\n");
        return false;
    }

    if (fsinfo->lead_signature != 0x41615252 || fsinfo->mid_signature  != 0x61417272 || fsinfo->trail_signature != 0xAA550000) {
        print_str("Invalid FSInfo Struct\n");
        return false;
    }

    // should be good here, copy necessary data

    return true;
}

bool fat32_open_volume(fat32_volume_t *this, uint8_t disk, uint32_t first_sector) {
    this->disk = disk;
    this->first_sector = first_sector;

    if (!fat32_load_vbr(this)) {
        return false;
    }

    return true;
}
