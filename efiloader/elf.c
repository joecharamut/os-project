#include "elf.h"

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
