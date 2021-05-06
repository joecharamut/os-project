#ifndef OS_GDT_H
#define OS_GDT_H

#include <stdint.h>
#include <stddef.h>

uint32_t gdt_add_entry(uint32_t base, uint32_t limit, uint8_t type);

#endif //OS_GDT_H
