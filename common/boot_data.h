#ifndef LOADER_BOOT_DATA_H
#define LOADER_BOOT_DATA_H

#include <stdint.h>

#define BOOT_DATA_SIGNATURE 0x534B52554E4B4C59

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

    uint64_t page_map_base;

    uint64_t kernel_base;
} allocation_info_t;

typedef struct {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address;

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t _reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

typedef struct {
    char anchor[4];
    uint8_t checksum;
    uint8_t length;
    uint8_t major_version;
    uint8_t minor_version;
    uint16_t max_structure_size;
    uint8_t entry_point_revision;
    uint8_t _reserved[5];

    char intermediate_anchor[5];
    uint16_t structure_table_length;
    uint32_t structure_table_address;
    uint16_t number_of_structures;
    uint8_t bcd_revision;
} __attribute__((packed)) smbios_entrypoint_t;

typedef struct {
    uint64_t signature;
    video_info_t video_info;
    memory_map_t memory_map;
    allocation_info_t allocation_info;
    acpi_rsdp_t rsdp;
    smbios_entrypoint_t smbios_entry;
} boot_data_t;

typedef void (__attribute__((sysv_abi)) *bootstrap_fn_ptr_t)(boot_data_t *);

#endif //LOADER_BOOT_DATA_H
