#include "kmem.h"
#include "kheap.h"
#include "paging.h"
#include <std/string.h>
#include <debug/panic.h>
#include <debug/debug.h>

heap_t *kernel_heap = NULL;
u32 placement_phys = 0;
u32 placement_addr = 0;

void init_kmem() {
    for (u32 i = 0; i < HEAP_SIZE; i += 0x1000) {
        mmap((void *) (MEMORY_BASE_PHYS + i), (void *) (HEAP_BASE_ADDR + i), false, false);
    }
    placement_phys = MEMORY_BASE_PHYS;
    placement_addr = HEAP_BASE_ADDR;

    u32 start = kmalloc(HEAP_SIZE - 0x1000);
    heap_t *new_heap = kmalloc(sizeof(heap_t));
    heap_create(new_heap, start, start + 0x1000, start + HEAP_SIZE, true, false);
    kernel_heap = new_heap;
}

static void *internal_kmalloc(size_t size, bool aligned, int *physical_addr) {
    if (kernel_heap) {
        return heap_alloc(kernel_heap, size, false);
    } else {
        if (aligned && placement_addr % 1024 != 0) {
            placement_addr &= 0xFFFFF000;
            placement_addr += 0x1000;
        }

        if (physical_addr) {
            TODO();
        }

        u32 tmp = placement_addr;
        placement_addr += size;
        return (void *) tmp;
    }
}

void *kmalloc(size_t size) {
    return internal_kmalloc(size, false, NULL);
}

void *kcalloc(size_t elements, size_t element_size) {
    void *ptr = kmalloc(elements * element_size);
    memset(ptr, 0, elements * element_size);
    return ptr;
}

void *kmalloc_a(size_t size) {
    return internal_kmalloc(size, true, NULL);
}

void kfree(void *ptr) {
    if (kernel_heap) {
        heap_free(kernel_heap, ptr);
    } else {
        PANIC("kmalloc: no heap");
    }
}
