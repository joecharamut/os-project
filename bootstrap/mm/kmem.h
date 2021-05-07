#ifndef OS_KMEM_H
#define OS_KMEM_H

#include <std/types.h>

u32 kmalloc(u32 size);
u32 kmalloc_aligned(u32 size);
u32 kmalloc_physical(u32 size, u32 *phys);
u32 kmalloc_physical_aligned(u32 size, u32 *phys);

#endif //OS_KMEM_H
