#include <debug/debug.h>
#include <debug/assert.h>
#include <mm/kmem.h>
#include <std/string.h>
#include <std/list.h>
#include <std/math.h>
#include "ext2.h"
#include "ide.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

typedef struct ext2_directory_entry {
    u32 inode;
    u8 type;
    char *name;
} ext2_directory_entry_t;

void ext2_read_block(ext2_volume_t *volume, u32 block, u8 *buffer) {
    assert(block > 0);
    dbg_logf(LOG_DEBUG, "Reading block %d\n", block);
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

static u32 read_directory_block(ext2_volume_t *volume, u32 block, list_t *entries) {
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

void ext2_stat_directory(ext2_volume_t *volume, u32 inode, list_t *dir) {
    ext2_inode_t directory_node = ext2_read_inode(volume, inode);
    assert(directory_node.type_and_permissions.type == EXT2_INODE_DIR);

    for (int i = 0; i < 12; i++) {
        if (directory_node.direct_block_pointers[i] == 0) continue;
        read_directory_block(volume, directory_node.direct_block_pointers[i], dir);
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

static void destroy_directory_list(list_t *dir) {
    for (u32 i = 0; i < dir->size; ++i) {
        ext2_directory_entry_t *entry = list_get(dir, i);
        kfree(entry->name);
    }
    list_destroy(dir);
}

u32 ext2_resolve_path(ext2_volume_t *volume, const char *path) {
    // special case: path "/"
    if (strcmp("/", path) == 0) {
        return 2;
    }

    // read the root dir
    list_t *dir = list_create();
    ext2_stat_directory(volume, 2, dir);

    // tokenize the path
    char *path_copy = strdup(path);
    char *token = strtok(path_copy, "/");

    u32 return_val = 0;

    if (token != NULL) {
        start:
        for (u32 i = 0; i < dir->size; i++) {
            // if the entry matches the current token
            ext2_directory_entry_t *entry = list_get(dir, i);
            if (strcmp(token, entry->name) == 0) {
                // store its inode
                u32 inode = entry->inode;
                token = strtok(NULL, "/");

                // if no more tokens, exit
                if (token == NULL) {
                    return_val = inode;
                    goto end;
                }

                // otherwise if the current token was a directory, search it
                if (entry->type == EXT2_FT_DIR) {
                    // get that directory's contents
                    destroy_directory_list(dir);
                    dir = list_create();
                    ext2_stat_directory(volume, inode, dir);

                    // search the list again
                    goto start;
                } else {
                    // if it isn't, the path is invalid
                    return_val = 0;
                    goto end;
                }
            }
        }
    }

    end:
    destroy_directory_list(dir);
    return return_val;
}

ext2_file_t *ext2_open(ext2_volume_t *volume, const char *path) {
    u32 inode = ext2_resolve_path(volume, path);

    if (inode == 0) {
        return NULL;
    }

    ext2_inode_t inode_data = ext2_read_inode(volume, inode);
    if (inode_data.type_and_permissions.type != EXT2_INODE_FILE) {
        return NULL;
    }

    ext2_file_t *file = kcalloc(1, sizeof(ext2_file_t));

    file->inode = inode_data;
    file->volume = volume;
    file->buffer = kmalloc(volume->block_size);

    return file;
}

size_t ext2_fread(u8 *ptr, size_t count, ext2_file_t *file) {
    ext2_inode_t node = file->inode;
    assert(node.type_and_permissions.type == EXT2_INODE_FILE);

    u32 block_size = file->volume->block_size;
    u32 read = 0;

    u32 pointers_per_block = (block_size / sizeof(u32));
    u32 indirect = 12 + (pointers_per_block);
    u32 dbl_indirect = indirect + (u32) pow(pointers_per_block, 2);
    u32 tpl_indirect = dbl_indirect + (u32) pow(pointers_per_block, 3);

    do {
        if (file->position >= node.filesize_lo) {
            break;
        }

        u32 block_index = file->position / block_size;
        u32 offset = file->position % block_size;
        u32 real_block;

        if (block_index <= 12) {
            // direct block
            real_block = node.direct_block_pointers[block_index];
        } else if (block_index <= indirect) {
            // indirect block
            TODO();
        } else if (block_index <= dbl_indirect) {
            // doubly indirect block
            TODO();
        } else if (block_index <= tpl_indirect) {
            // triply indirect block
            TODO();
        } else {
            assert_not_reached();
        }

        if (real_block != file->buffered_block) {
            ext2_read_block(file->volume, real_block, file->buffer);
            file->buffered_block = real_block;
        }

        while (offset < block_size && read < count && read < node.filesize_lo) {
            *(ptr + read) = *(file->buffer + offset);

            offset++;
            read++;
        }
        file->position += read;
    } while (read < count);

    return read;
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

    ext2_file_t *fp = ext2_open(volume, "/HELLO.TXT");
    char *buf = kcalloc(16, sizeof(char));
    if (fp) {
        u32 read;
        while ((read = ext2_fread(buf, 16, fp)) > 0) {
            dbg_logf(LOG_DEBUG, "Read %d bytes: [", read);
            for (int i = 0; i < 16; ++i) {
                dbg_printf("%c", buf[i]);
            }
            dbg_printf("]\n");
        }
    }

    return volume;
}

#pragma GCC diagnostic pop
