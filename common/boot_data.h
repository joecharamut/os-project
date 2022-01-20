#ifndef LOADER_BOOT_DATA_H
#define LOADER_BOOT_DATA_H

#include <stdint.h>

//#define BOOT_DATA_SIGNATURE 0x534B52554E4B4C59

#define BOOT_DATA_SIGNATURE (((union { char s[9]; uint64_t i; }) { "SKRUNKLY" }).i)

typedef struct {
    uint64_t bufferAddress;
    uint64_t bufferSize;
    uint64_t horizontalResolution;
    uint64_t verticalResolution;
    uint64_t pixelsPerScanLine;
} video_info_t;

typedef struct {
    uint32_t type;
    uint32_t padding;
    uint64_t physical_address;
    uint64_t virtual_address;
    uint64_t number_of_pages;
    uint64_t flags;
} memory_descriptor_t;

typedef struct {
    uint64_t count;
    memory_descriptor_t *entries;
} memory_map_t;

typedef struct {
    uint64_t image_base;
    uint64_t image_size;

    uint64_t stack_base;
    uint64_t stack_size;

    uint64_t kernel_base;
} allocation_info_t;

typedef struct {
    uint64_t signature;
    video_info_t video_info;
    memory_map_t memory_map;
    allocation_info_t allocation_info;
} boot_data_t;

typedef void (__attribute__((sysv_abi)) *bootstrap_fn_ptr_t)(boot_data_t *);

#endif //LOADER_BOOT_DATA_H
