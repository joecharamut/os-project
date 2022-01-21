#include "elf.h"

static const char * const elf_ptype_strings[] = {
        "PT_NULL",
        "PT_LOAD",
        "PT_DYNAMIC",
        "PT_INTERP",
        "PT_NOTE",
        "PT_SHLIB",
        "PT_PHDR",
        "PT_TLS",
};

const char *elf_ptype_string(uint64_t type) {
    if (type < sizeof(elf_ptype_strings)) {
        return elf_ptype_strings[type];
    }

    if (type == 0x6474e550) return "PT_GNU_EH_FRAME";
    if (type == 0x6474e551) return "PT_GNU_STACK";
    if (type == 0x6474e552) return "PT_GNU_RELRO";

    return "PT_UNKNOWN";
}

bool elf_is_header_valid(void *header_buf) {
    elf_identifier_t *ident = header_buf;
    if (ident->magic[0] != 0x7F
        || ident->magic[1] != 'E'
        || ident->magic[2] != 'L'
        || ident->magic[3] != 'F'
        || ident->version != ELF_IDENT_CURRENT_VERSION) {
        return false;
    }
    return true;
}

bool elf_is_64_bit(void *header_buf) {
    elf_identifier_t *ident = header_buf;
    if (ident->bitness != ELF_IDENT_64BIT || ident->endianness != ELF_IDENT_LITTLE_ENDIAN) {
        return false;
    }
    return true;
}
