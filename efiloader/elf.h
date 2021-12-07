#ifndef LOADER_ELF_H
#define LOADER_ELF_H

#include <stdint.h>
#include <stdbool.h>

#define static_assert _Static_assert

#define ELF_IDENT_32BIT             1
#define ELF_IDENT_64BIT             2
#define ELF_IDENT_LITTLE_ENDIAN     1
#define ELF_IDENT_BIG_ENDIAN        2
#define ELF_IDENT_CURRENT_VERSION   1

typedef struct {
    char magic[4];
    uint8_t bitness;
    uint8_t endianness;
    uint8_t version;
    uint8_t abi;
    uint8_t abi_version;
    uint8_t _padding[7];
} __attribute__((packed)) elf_identifier_t;
static_assert(sizeof(elf_identifier_t) == 16, "Invalid Size");

#define ELF_TYPE_NONE   0
#define ELF_TYPE_REL    1
#define ELF_TYPE_EXEC   2
#define ELF_TYPE_DYN    3
#define ELF_TYPE_CORE   4

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
} __attribute__((packed)) elf64_header_t;
static_assert(sizeof(elf64_header_t) == 64, "Invalid Size");

#define ELF_PFLAG_R 1
#define ELF_PFLAG_W 2
#define ELF_PFLAG_X 4

#define ELF_PTYPE_NULL      0x00000000
#define ELF_PTYPE_LOAD      0x00000001
#define ELF_PTYPE_DYNAMIC   0x00000002
#define ELF_PTYPE_INTERP    0x00000003
#define ELF_PTYPE_NOTE      0x00000004
#define ELF_PTYPE_SHLIB     0x00000005
#define ELF_PTYPE_PHDR      0x00000006
#define ELF_PTYPE_TLS       0x00000007

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesize;
    uint64_t memsize;
    uint64_t align;
} __attribute__((packed)) elf64_pht_entry_t;
static_assert(sizeof(elf64_pht_entry_t) == 56, "Invalid Size");

typedef struct {
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    uint64_t vaddr;
    uint64_t offset;
    uint64_t filesize;
    uint32_t link_section;
    uint32_t info_section;
    uint64_t alignment;
    uint64_t entry_size;
} __attribute__((packed)) elf64_sht_entry_t;
static_assert(sizeof(elf64_sht_entry_t) == 64, "Invalid Size");

bool elf_is_header_valid(void *header_buf);
bool elf_is_64_bit(void *header_buf);

#endif //LOADER_ELF_H
