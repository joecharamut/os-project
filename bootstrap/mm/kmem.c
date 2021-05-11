#include <stdbool.h>
#include "kmem.h"

u32 heap_index = 0;
u8 heap[16384];

static void *kmalloc_internal(u32 size, bool align, u32 *phys) {
    if (heap_index == 0) heap_index = (u32) &heap;

    if (align && (heap_index & 0xFFFFF000)) {
        // if we need to be aligned and are not already, align the placement address
        heap_index &= 0xFFFFF000;
        heap_index += 0x1000;
    }

    if (phys) {
        *phys = heap_index;
    }

    u32 tmp = heap_index;
    heap_index += size;
    return (void *) tmp;
}

void *kmalloc(u32 size) {
    return kmalloc_internal(size, false, 0);
}

void *kmalloc_aligned(u32 size) {
    return kmalloc_internal(size, true, 0);
}

void *kmalloc_physical(u32 size, u32 *phys) {
    return kmalloc_internal(size, false, phys);
}

void *kmalloc_physical_aligned(u32 size, u32 *phys) {
    return kmalloc_internal(size, true, phys);
}
