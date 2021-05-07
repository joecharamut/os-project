#include <stdbool.h>
#include "kmem.h"

u32 placement_address;

static u32 kmalloc_internal(u32 size, bool align, u32 *phys) {
    if (!placement_address) {
        extern u32 heap_top;
        placement_address = (u32) &heap_top;
    }

    if (align && (placement_address & 0xFFFFF000)) {
        // if we need to be aligned and are not already, align the placement address
        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }

    if (phys) {
        *phys = placement_address;
    }

    u32 tmp = placement_address;
    placement_address += size;
    return tmp;
}

u32 kmalloc(u32 size) {
    return kmalloc_internal(size, false, 0);
}

u32 kmalloc_aligned(u32 size) {
    return kmalloc_internal(size, true, 0);
}

u32 kmalloc_physical(u32 size, u32 *phys) {
    return kmalloc_internal(size, false, phys);
}

u32 kmalloc_physical_aligned(u32 size, u32 *phys) {
    return kmalloc_internal(size, true, phys);
}
