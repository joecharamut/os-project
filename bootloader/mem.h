#ifndef LOADER_MEM_H
#define LOADER_MEM_H

#include <stdint.h>
#include "debug.h"

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) bios_mmap_entry_t;
static_assert(sizeof(bios_mmap_entry_t) == 20, "Invalid Size");

extern uint16_t get_memory_size();
extern uint16_t get_extended_memory_size();
extern uint32_t get_system_memory_map(bios_mmap_entry_t *entry_buffer);

#endif //LOADER_MEM_H
