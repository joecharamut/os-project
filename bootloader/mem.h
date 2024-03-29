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

extern uint32_t get_system_memory_map(bios_mmap_entry_t *entry_buffer);
extern void memcpy(void *dst, void *src, uint32_t count);
extern uint32_t strncmp(const char *str1, const char *str2, uint32_t count);

#endif //LOADER_MEM_H
