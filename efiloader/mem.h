#ifndef LOADER_MEM_H
#define LOADER_MEM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool present: 1;
    bool rw: 1;
    bool user: 1;
    bool write_through: 1;
    bool cache_disabled: 1;
    bool accessed: 1;
    bool dirty: 1;
    uint8_t _padding: 1;
    bool global: 1;
    uint8_t os: 3;
    uint32_t address: 20;
} __attribute__ ((packed)) page_table_entry_t;

#endif //LOADER_MEM_H
