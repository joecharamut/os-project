#ifndef OS_KMEM_H
#define OS_KMEM_H

#include <std/types.h>

void *kmalloc(u32 size);
void *kmalloc_aligned(u32 size);
void *kmalloc_physical(u32 size, u32 *phys);
void *kmalloc_physical_aligned(u32 size, u32 *phys);

#endif //OS_KMEM_H
