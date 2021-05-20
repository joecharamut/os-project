#ifndef OS_KMEM_H
#define OS_KMEM_H

#include <std/types.h>
#include <stdbool.h>
#include <stddef.h>

#define KERNEL_BASE_ADDR    0xC0000000 // +3 GiB (virtual)
#define HEAP_BASE_ADDR      0xE0000000 // +3.5 GiB (virtual)
#define PAGE_TABLE_BASE     0x01000000 // +16 MiB (physical)
#define MEMORY_BASE_PHYS    0x01800000 // +24 MiB (physical)
#define HEAP_SIZE           0x00400000 // 4 MiB long

void init_kmem();

void *kmalloc(size_t size);
void *kcalloc(size_t elements, size_t element_size);
void kfree(void *ptr);

#endif //OS_KMEM_H
