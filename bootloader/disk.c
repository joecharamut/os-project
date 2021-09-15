#include "disk.h"
#include "mem.h"

static bool fat32_load_vbr(fat32_volume_t *this) {
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
    this->serial_number = vbr->serial_number;

    this->cluster_size = vbr->sectors_per_cluster;
    this->fat_size = (vbr->sectors_per_fat_16 == 0) ? vbr->sectors_per_fat_32 : vbr->sectors_per_fat_16;
    this->root_cluster = vbr->root_directory_cluster;

    this->first_fat_sector = vbr->reserved_sector_count;
    this->first_data_sector = vbr->reserved_sector_count + (vbr->number_of_fats * this->fat_size);

    return true;
}

static bool fat32_read_cluster(fat32_volume_t *this, uint8_t *buf, uint32_t cluster) {
    assert(cluster >= 2);
    return disk_read_sectors(this->disk, buf, this->first_sector + (((cluster - 2) * this->cluster_size) + this->first_data_sector), this->cluster_size);
}

static uint32_t fat32_next_cluster(fat32_volume_t *this, uint32_t current_cluster) {
    uint8_t fat_table[512];
    uint32_t cluster = current_cluster;
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = this->first_fat_sector + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;

    if (disk_read_sectors(this->disk, fat_table, this->first_sector + fat_sector, 1)) {
        print_hexs("Error reading sector 0x", this->first_sector + fat_sector, "\n");
        abort();
    }
    cluster = (*(uint32_t *) &fat_table[entry_offset]) & 0x0FFFFFFF;

    return cluster;
}

bool fat32_open_volume(fat32_volume_t *this, uint8_t disk, uint32_t first_sector) {
    this->disk = disk;
    this->first_sector = first_sector;

    if (!fat32_load_vbr(this)) {
        return false;
    }

    return true;
}

int fat32_read_directory(fat32_volume_t *this, fat32_directory_entry_t *entry_buf, uint32_t cluster) {
    uint8_t cluster_buf[this->cluster_size * 512];
    fat32_directory_entry_t *directory = (fat32_directory_entry_t *) &cluster_buf;

    if (fat32_read_cluster(this, cluster_buf, cluster)) {
        return -1;
    }

    int count = 0;
    for (uint32_t entry_idx = 0; entry_idx < (this->cluster_size * 512) / 32; ++entry_idx) {
        if (directory[entry_idx].name[0] == 0) {
            break;
        }

        switch (directory[entry_idx].attributes) {
            case FAT32_ATTR_LFN:
                break;

            case FAT32_ATTR_VOLUMEID:
                memcpy(this->volume_id, directory[entry_idx].name, 8);
                memcpy(this->volume_id+8, directory[entry_idx].ext, 3);
                break;

            default:
                if (entry_buf) {
                    entry_buf[count] = directory[entry_idx];
                }
                count++;
                break;
        }

        if (directory[entry_idx].attributes == FAT32_ATTR_LFN) {
//            fat32_lfn_entry_t *lfn = (fat32_lfn_entry_t *) &directory[entry_idx];
//
//            print_decs("lfn entry ", entry_idx, ": ");
//            for (int i = 0; i < 5; ++i) {
//                print_chr(lfn->group_1[i]);
//            }
//            for (int i = 0; i < 6; ++i) {
//                print_chr(lfn->group_2[i]);
//            }
//            for (int i = 0; i < 2; ++i) {
//                print_chr(lfn->group_3[i]);
//            }
//            print_str("\n");
        }
    }

    return count;
}

bool fat32_file_open(fat32_volume_t *this, fat32_file_t *file_ptr, fat32_directory_entry_t *dir_entry) {
    file_ptr->volume = this;
    file_ptr->starting_cluster = (dir_entry->cluster_hi << 16) | dir_entry->cluster_lo;
    file_ptr->current_cluster = file_ptr->starting_cluster;
    file_ptr->file_offset = 0;
    file_ptr->file_size = dir_entry->filesize;
    return true;
}

uint32_t fat32_file_read(fat32_file_t *this, uint8_t *buf, uint32_t bytes) {
    uint32_t cluster_size = this->volume->cluster_size * 512;
    uint8_t cluster_buf[cluster_size];
    uint32_t bytes_read = 0;

    if (fat32_read_cluster(this->volume, cluster_buf, this->current_cluster)) {
        print_hexs("Error reading cluster 0x", this->current_cluster, "\n");
        abort();
    }

    while (bytes_read < bytes && this->file_offset < this->file_size) {
        buf[bytes_read] = cluster_buf[this->file_offset % cluster_size];
        bytes_read++;
        this->file_offset++;

        if (this->file_offset % cluster_size == 0 && bytes_read != 0) {
            this->current_cluster = fat32_next_cluster(this->volume, this->current_cluster);
            if (fat32_read_cluster(this->volume, cluster_buf, this->current_cluster)) {
                print_hexs("Error reading cluster 0x", this->current_cluster, "\n");
                abort();
            }
        }
    }

    return bytes_read;
}

int fat32_file_seek(fat32_file_t *this, int offset, int origin) {
    switch (origin) {
        case FAT32_SEEK_SET:
            this->file_offset = offset;
            break;

        case FAT32_SEEK_CUR:
            this->file_offset += offset;
            break;

        default: return -1;
    }

    uint32_t cluster_index = this->file_offset / (this->volume->cluster_size * 512);
    this->current_cluster = this->starting_cluster;
    while (cluster_index-- > 0) {
        this->current_cluster = fat32_next_cluster(this->volume, this->current_cluster);
    }

    return 0;
}
