#include <stdbool.h>
#include "kmem.h"

u32 heap_ptr = 0;

static void *kmalloc_internal(u32 size, bool align, u32 *phys) {
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
    return (void *) tmp;
}

void *kmalloc(u32 size) {
    return kmalloc_internal(size, false, 0);
}

void set_heap_address(void *addr) {
    heap_ptr = (u32) addr;
}
