#ifndef OS_KMEM_H
#define OS_KMEM_H

#include <std/types.h>
#include <stdbool.h>

#define KERNEL_ADDR_BASE    0xC0000000 // +3 GiB (virtual)
#define PAGE_TABLE_BASE     0x01000000 // +16 MiB (physical)
#define HEAP_BASE           0x01800000 // +24 MiB (physical)
#define HEAP_SIZE           0x00400000 // 4 MiB long

#define HEAP_MAGIC          0x00C0FFEE

typedef struct heap_header {
    u32 magic;
    bool is_hole;
    u32 size;
} heap_header_t;

typedef struct heap_footer {
    u32 magic;
    heap_header_t *header;
} heap_footer_t;

void set_heap_address(void *addr);
void *kmalloc(u32 size);

#endif //OS_KMEM_H
