#ifndef OS_KMEM_H
#define OS_KMEM_H

#include <std/types.h>

typedef struct {
    u32 magic;
    bool is_hole;
    u32 size;
} heap_header_t;

typedef struct {
    u32 magic;
    heap_header_t *header;
} heap_footer_t;

void *kmalloc(u32 size);
void *kmalloc_aligned(u32 size);
void *kmalloc_physical(u32 size, u32 *phys);
void *kmalloc_physical_aligned(u32 size, u32 *phys);

#endif //OS_KMEM_H
