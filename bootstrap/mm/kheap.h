#ifndef OS_KHEAP_H
#define OS_KHEAP_H

#include <std/types.h>
#include <std/ordered_array.h>

#define HEAP_MAGIC          0x00C0FFEE
#define HEAP_MIN_CHUNK      0x00001000 // 4 KiB chunks

typedef struct {
    ordered_array_t index;
    u32 start_address;
    u32 end_address;
    u32 max_address;
    bool supervisor;
    bool readonly;
} heap_t;

heap_t *heap_create(heap_t *heap, u32 start, u32 end, u32 max, bool supervisor, bool readonly);
void *heap_alloc(heap_t *heap, u32 size, bool aligned);
void heap_free(heap_t *heap, void *ptr);

#endif //OS_KHEAP_H
