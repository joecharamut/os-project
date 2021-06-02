#include <debug/debug.h>
#include <debug/assert.h>
#include <mm/kmem.h>
#include <std/string.h>
#include <std/list.h>
#include "ext2.h"
#include "ide.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

typedef enum inode_type {
    EXT2_INODE_FIFO = 0x1,
    EXT2_INODE_CHARDEV = 0x2,
    EXT2_INODE_DIR = 0x4,
    EXT2_INODE_BLOCKDEV = 0x6,
    EXT2_INODE_FILE = 0x8,
    EXT2_INODE_LINK = 0xA,
    EXT2_INODE_SOCKET = 0xC,
} __attribute__((packed)) inode_type_t;
static_assert(sizeof(inode_type_t) == 1, "inode_type_t invalid size");

typedef enum file_type {
    EXT2_FT_UNKNOWN,
    EXT2_FT_REG_FILE,
    EXT2_FT_DIR,
    EXT2_FT_CHARDEV,
    EXT2_FT_BLOCKDEV,
    EXT2_FT_FIFO,
    EXT2_FT_SOCK,
    EXT2_FT_SYMLINK,
} __attribute__((packed)) file_type_t;
static_assert(sizeof(file_type_t) == 1, "file_type_t invalid size");

typedef struct ext2_inode {
    struct {
        u16 permissions : 12;
        inode_type_t type : 4;
    } __attribute__((packed)) type_and_permissions;
    u16 user_id;
    u32 filesize_lo;
    u32 access_time;
    u32 creation_time;
    u32 modification_time;
    u32 deletion_time;
    u16 group_id;
    u16 hard_links;
    u32 sectors_used;
    u32 flags;
    u32 os_specific_1;
    u32 direct_block_pointers[12];
    u32 indirect_block_pointer;
    u32 doubly_indirect_block_pointer;
    u32 triply_indirect_block_pointer;
    u32 generation_number;
    u32 extended_attribute_block;
    u32 filesize_hi;
    u32 fragment_block;
    u8 os_specific_2[12];
} __attribute__((packed)) ext2_inode_t;
static_assert(sizeof(ext2_inode_t) == 128, "ext2_inode_t invalid size");

typedef struct ext2_directory_entry {
    u32 inode;
    u8 type;
    char *name;
} ext2_directory_entry_t;


void ext2_read_block(ext2_volume_t *volume, u32 block, u8 *buffer) {
    assert(block > 0);
    u32 sector = (block * volume->block_size) / volume->sector_size;
    u32 count = (volume->block_size / volume->sector_size);
    ata_read_sectors(volume->ata_bus, volume->ata_drive, volume->partition_offset + sector, count, (u16 *) buffer);
}

ext2_inode_t ext2_read_inode(ext2_volume_t *volume, u32 inode) {
    assert(inode > 0);

    u32 inodes_per_block = volume->block_size / volume->superblock.inode_size;
    u32 group = (inode - 1) / volume->superblock.blocks_per_group;
    u32 index = ((inode - 1) % volume->superblock.blocks_per_group) % inodes_per_block;
    u32 block = (inode - 1) / inodes_per_block;

    dbg_logf(LOG_DEBUG, "inode %d is in group %d block %d offset %d\n", inode, group, block, index);

    u32 inode_block = volume->block_group_descriptor_table[group].inode_table_start_block + block;

    ext2_inode_t *inode_table = kmalloc(volume->block_size);
    ext2_read_block(volume, inode_block, (void *) inode_table);
    ext2_inode_t inode_data = inode_table[index];
    kfree(inode_table);

    return inode_data;
}

u32 ext2_read_directory_block(ext2_volume_t *volume, u32 block, list_t *entries) {
    typedef struct {
        u32 inode;
        u16 entry_length;
        u8 name_length;
        file_type_t file_type;
        char name[];
    } __attribute__((packed)) ext2_phys_dir_entry_t;

    u8 *buffer = kmalloc(volume->block_size);
    ext2_read_block(volume, block, buffer);

    int count = 0;
    int offset = 0;
    while (offset < 1024) {
        ext2_phys_dir_entry_t *entry = (ext2_phys_dir_entry_t *) (buffer + offset);
        offset += entry->entry_length;

        char *name = kcalloc(entry->name_length + 1, sizeof(char));
        memcpy(name, entry->name, entry->name_length);

        ext2_directory_entry_t *list_entry = kcalloc(1, sizeof(ext2_directory_entry_t));
        list_entry->type = entry->file_type;
        list_entry->inode = entry->inode;
        list_entry->name = name;

        list_append(entries, list_entry);
        count++;
    }

    kfree(buffer);
    return count;
}

void ext2_read_directory(ext2_volume_t *volume, u32 inode) {
    ext2_inode_t directory_node = ext2_read_inode(volume, inode);
    assert(directory_node.type_and_permissions.type == EXT2_INODE_DIR);

    list_t *dir = list_create();

    for (int i = 0; i < 12; i++) {
        if (directory_node.direct_block_pointers[i] == 0) continue;
        ext2_read_directory_block(volume, directory_node.direct_block_pointers[i], dir);
    }

    if (directory_node.indirect_block_pointer) {
        TODO();
    }

    if (directory_node.doubly_indirect_block_pointer) {
        TODO();
    }

    if (directory_node.triply_indirect_block_pointer) {
        TODO();
    }

    for (u32 i = 0; i < dir->size; i++) {
        ext2_directory_entry_t *entry = list_get(dir, i);
        dbg_logf(LOG_DEBUG, "dir entry %d: inode %d, type %d, name '%s'\n", i, entry->inode, entry->type, entry->name);
    }
}

ext2_volume_t *ext2_open_volume(mbr_drive_t drive, u8 partition) {
    ext2_volume_t *volume = kmalloc(sizeof(ext2_volume_t));

    volume->ata_bus = drive.ata_bus;
    volume->ata_drive = drive.ata_drive;
    volume->sector_size = 512; // todo fix this
    volume->partition_offset = drive.partition_info[partition].lba_first_sector;
    volume->partition_size = drive.partition_info[partition].sector_count;
    ata_read_sectors(drive.ata_bus, drive.ata_drive, volume->partition_offset + 2, 2, (u16 *) &volume->superblock);

    if (volume->superblock.ext2_signature != 0xEF53) {
        dbg_logf(LOG_ERROR, "ext2 Signature Invalid\n");
        return NULL;
    }

    if (volume->superblock.version_major < 1) {
        dbg_logf(LOG_ERROR, "Unsupported ext2 version\n");
        return NULL;
    }

    volume->block_size = 1024 << volume->superblock.log_block_size;

    u32 n_descriptors = (volume->superblock.total_blocks / volume->superblock.blocks_per_group) + 1;
    u32 descriptor_start = volume->block_size == 1024 ? 2 : 1;

    dbg_logf(LOG_DEBUG, "Block group descriptor table has %d entries\n", n_descriptors);

    ext2_block_group_descriptor_t *descriptor_table = kmalloc(volume->block_size);
    ext2_read_block(volume, descriptor_start, (u8 *) descriptor_table);
    volume->block_group_descriptor_count = n_descriptors;
    volume->block_group_descriptor_table = kcalloc(n_descriptors, sizeof(ext2_block_group_descriptor_t));
    memcpy(volume->block_group_descriptor_table, descriptor_table, n_descriptors * sizeof(ext2_block_group_descriptor_t));
    kfree(descriptor_table);

    // read root dir
    ext2_read_directory(volume, 2);

    return volume;
}

#pragma GCC diagnostic pop
