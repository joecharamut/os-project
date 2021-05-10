#include <stdbool.h>
#include "kmem.h"

static u32 kmalloc_internal(u32 size, bool align, u32 *phys) {
    extern u32 heap_ptr;

    if (align && (heap_ptr & 0xFFFFF000)) {
        // if we need to be aligned and are not already, align the placement address
        heap_ptr &= 0xFFFFF000;
        heap_ptr += 0x1000;
    }

    if (phys) {
        *phys = heap_ptr;
    }

    u32 tmp = heap_ptr;
    heap_ptr += size;
    return tmp;
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
