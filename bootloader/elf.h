#ifndef LOADER_ELF_H
#define LOADER_ELF_H

#include <stdint.h>
#include "debug.h"

#define ELF_IDENT_32BIT 1
#define ELF_IDENT_64BIT 2

typedef struct {
    char magic[4];
    uint8_t bitness;
    uint8_t endianness;
    uint8_t version;
    uint8_t abi;
    uint8_t abi_version;
    uint8_t _padding[7];
} __attribute__((__packed__)) elf_identifier_t;
static_assert(sizeof(elf_identifier_t) == 16, "Invalid Size");

typedef struct {
    elf_identifier_t identifier;

    uint16_t type;
    uint16_t instruction_set;
    uint32_t version;
    uint64_t entry;
    uint64_t pht_offset;
    uint64_t sht_offset;
    uint32_t flags;
    uint16_t header_size;
    uint16_t pht_entry_size;
    uint16_t pht_entry_count;
    uint16_t sht_entry_size;
    uint16_t sht_entry_count;
    uint16_t sht_string_index;
} __attribute__((__packed__)) elf64_header_t;
static_assert(sizeof(elf64_header_t) == 64, "Invalid Size");


#endif //LOADER_ELF_H
