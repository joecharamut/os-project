#ifndef OS_EXT2_H
#define OS_EXT2_H

#include <stdbool.h>
#include <debug/assert.h>
#include "ide.h"

typedef struct {
    u32 total_inodes;
    u32 total_blocks;
    u32 reserved_blocks;
    u32 unallocated_blocks;
    u32 unallocated_inodes;
    u32 superblock_block_number;
    u32 log_block_size;
    u32 log_fragment_size;
    u32 blocks_per_group;
    u32 fragments_per_group;
    u32 inodes_per_group;
    u32 last_mount_time;
    u32 last_write_time;
    u16 mounts_since_last_fsck;
    u16 mounts_allowed_until_fsck;
    u16 ext2_signature;
    u16 filesystem_state;
    u16 error_handling_policy;
    u16 version_minor;
    u32 last_fsck_time;
    u32 time_between_fsck;
    u32 os_creation_id;
    u32 version_major;
    u16 reserved_uid;
    u16 reserved_gid;

    u32 first_unreserved_inode;
    u16 inode_size;
    u16 superblock_block_group;
    struct {
        bool preallocate_directory_blocks : 1;
        bool afs_server_inodes : 1;
        bool has_journal : 1;
        bool inodes_extended_attributes : 1;
        bool can_resize : 1;
        bool hash_indexed_directories : 1;
        u32 _padding : 26;
    } __attribute__((packed)) optional_features;
    struct {
        bool compression : 1;
        bool directories_have_type_field : 1;
        bool needs_journal_replay : 1;
        bool uses_journal_device : 1;
        u32 _padding : 28;
    } __attribute__((packed)) required_features;
    struct {
        bool sparse_superblocks_and_tables : 1;
        bool filesize_is_64_bits : 1;
        bool directory_contents_are_btree : 1;
        u32 _padding : 29;
    } __attribute__((packed)) write_required_features;
    u8 filesystem_id[16];
    char volume_name[16];
    char last_mount_path[64];
    u32 compression_algorithms;
    u8 preallocated_file_blocks;
    u8 preallocated_directory_blocks;
    u8 _unused[2];
    u8 journal_id[16];
    u32 journal_inode;
    u32 journal_device;
    u32 orphan_inode_head;
    u8 _unused_2[788];
} __attribute__((packed)) ext2_superblock_t;
static_assert(sizeof(ext2_superblock_t) == 1024, "ext2_superblock_t invalid size");

typedef struct {
    u32 usage_bitmap_block;
    u32 inode_bitmap_block;
    u32 inode_table_start_block;
    u16 unallocated_blocks;
    u16 unallocated_inodes;
    u16 directories;
    u8 _padding[14];
} __attribute__((packed)) ext2_block_group_descriptor_t;
static_assert(sizeof(ext2_block_group_descriptor_t) == 32, "ext2_block_group_descriptor_t invalid size");

typedef enum {
    EXT2_INODE_FIFO = 0x1,
    EXT2_INODE_CHARDEV = 0x2,
    EXT2_INODE_DIR = 0x4,
    EXT2_INODE_BLOCKDEV = 0x6,
    EXT2_INODE_FILE = 0x8,
    EXT2_INODE_LINK = 0xA,
    EXT2_INODE_SOCKET = 0xC,
} __attribute__((packed)) inode_type_t;
static_assert(sizeof(inode_type_t) == 1, "inode_type_t invalid size");

typedef enum {
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

typedef struct {
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

typedef struct {
    u16 ata_bus;
    u8 ata_drive;
    u32 sector_size;
    u32 partition_offset;
    u32 partition_size;

    ext2_superblock_t superblock;
    u32 block_size;

    u32 block_group_descriptor_count;
    ext2_block_group_descriptor_t *block_group_descriptor_table;
} ext2_volume_t;

typedef struct {
    ext2_inode_t inode;
    u32 inode_number;
    ext2_volume_t *volume;
    u32 position;
    u32 buffered_block;
    u8 *buffer;
} ext2_file_t;

ext2_volume_t *ext2_open_volume(mbr_drive_t drive, u8 partition);

ext2_file_t *ext2_fopen(ext2_volume_t *volume, const char *path);
size_t ext2_fread(u8 *ptr, size_t count, ext2_file_t *file);
void ext2_fclose(ext2_file_t *file);

#endif //OS_EXT2_H
