#include <debug/debug.h>
#include <debug/assert.h>
#include "kheap.h"
#include "kmem.h"

#define HEAP_MIN_SIZE 0
#define HEAP_INDEX_SIZE 0x1000

typedef struct {
    u32 magic;
    bool is_hole;
    u32 size;
} header_t;

typedef struct {
    u32 magic;
    header_t *header;
} footer_t;

static int heap_header_compare(void *a, void *b) {
    assert(((header_t *) a)->magic == HEAP_MAGIC);
    assert(((header_t *) b)->magic == HEAP_MAGIC);
    return (((header_t *) a)->size < ((header_t *) b)->size) ? 1 : 0;
}

static int find_hole(heap_t *heap, u32 size, bool aligned) {
    u32 i = 0;
    while (i < heap->index.size) {
        header_t *block = (header_t *) ordered_array_get(&heap->index, i);

        if (aligned) {
            u32 loc = (u32) block;
            int offset = 0;
            if ((loc + sizeof(header_t)) & 0xFFFFF000) {
                offset = 0x1000 - (loc + sizeof(header_t)) % 0x1000;
            }
            int hole_size = (int) block->size - offset;
            if (hole_size >= (int) size) {
                break;
            }
        } else if (block->size >= size) {
            break;
        }

        i++;
    }

    return i == heap->index.size ? -1 : i;
}

static void expand_heap(heap_t *heap, u32 new_size) {
    assert(new_size > heap->end_address - heap->start_address);

    if (new_size & 0xFFFFF000) {
        new_size &= 0xFFFFF000;
        new_size += 0x1000;
    }

    assert(heap->start_address+new_size <= heap->max_address);

    u32 old_size = heap->end_address-heap->start_address;
    u32 i = old_size;
    while (i < new_size) {
//        alloc_frame(get_page(heap->start_address + i, 1, kernel_directory), heap->supervisor, !heap->readonly);
        i += 0x1000;
    }
    heap->end_address = heap->start_address+new_size;
}

static u32 contract_heap(heap_t *heap, u32 new_size) {
    assert(new_size < heap->end_address - heap->start_address);

    if (new_size & 0x1000) {
        new_size &= 0x1000;
        new_size += 0x1000;
    }

    if (new_size < HEAP_MIN_SIZE) {
        new_size = HEAP_MIN_SIZE;
    }

    u32 old_size = heap->end_address - heap->start_address;
    u32 i = old_size - 0x1000;
    while (new_size < i) {
//        free_frame(get_page(heap->start_address + i, 0, kernel_directory));

        i -= 0x1000;
    }
    heap->end_address = heap->start_address + new_size;
    return new_size;
}

heap_t *heap_create(heap_t *heap, u32 start, u32 end, u32 max, bool supervisor, bool readonly) {
    assert(start % 0x1000 == 0);
    assert(end % 0x1000 == 0);

    heap->index = ordered_array_create_at((void *) start, HEAP_INDEX_SIZE, &heap_header_compare);
    start += HEAP_INDEX_SIZE * sizeof(type_t);

    if (start & 0xFFFFF000) {
        start &= 0xFFFFF000;
        start += 0x1000;
    }

    heap->start_address = start;
    heap->end_address = end;
    heap->max_address = max;
    heap->supervisor = supervisor;
    heap->readonly = readonly;

    header_t *hole = (header_t *) start;
    hole->size = (end - start);
    hole->magic = HEAP_MAGIC;
    hole->is_hole = true;

    ordered_array_insert(&heap->index, hole);

    return heap;
}

void *heap_alloc(heap_t *heap, u32 size, bool aligned) {
    u32 real_size = size + sizeof(header_t) + sizeof(footer_t);

    int i = find_hole(heap, size, aligned);
    if (i == -1) {
        TODO();
    }

    header_t *original_hole = (header_t *) ordered_array_get(&heap->index, i);
    u32 original_hole_addr = (u32) original_hole;
    u32 original_hole_size = original_hole->size;

    // check if it's worth splitting the hole
    if (original_hole_size - real_size < sizeof(header_t) + sizeof(footer_t)) {
        size += original_hole_size - real_size;
        real_size = original_hole_size;
    }

    // handle page-alignment
    if (aligned && (original_hole_addr & 0xFFFFF000)) {
        TODO();
    } else {
        // otherwise remove it from the index
        ordered_array_remove(&heap->index, i);
    }

    header_t *block = (header_t *) original_hole_addr;
    block->magic = HEAP_MAGIC;
    block->is_hole = false;
    block->size = real_size;

    footer_t *block_footer = (footer_t *) (original_hole_addr + sizeof(header_t) + size);
    block_footer->magic = HEAP_MAGIC;
    block_footer->header = block;

    if (original_hole_size - real_size > 0) {
        header_t *hole_header = (header_t *) (original_hole_addr + sizeof(header_t) + size + sizeof(footer_t));
        hole_header->magic = HEAP_MAGIC;
        hole_header->is_hole = true;
        hole_header->size = original_hole_size - real_size;
        footer_t *hole_footer = (footer_t *) ((u32) hole_header + original_hole_size - real_size - sizeof(footer_t));
        if ((u32) hole_footer < heap->end_address) {
            hole_footer->magic = HEAP_MAGIC;
            hole_footer->header = hole_header;
        }
        // Put the new hole in the index;
        ordered_array_insert(&heap->index, hole_header);
    }

    return (void *) ((u32) block + sizeof(header_t));
}

void heap_free(heap_t *heap, void *ptr) {
    if (ptr == NULL) {
        return;
    }

    header_t *header = (header_t *) ((u32) ptr - sizeof(header_t));
    footer_t *footer = (footer_t *) ((u32) header + header->size - sizeof(footer_t));

    assert(header->magic == HEAP_MAGIC);
    assert(footer->magic == HEAP_MAGIC);

    header->is_hole = true;
    bool add_to_index = true;

    footer_t *check_left = (footer_t *) ((u32) header - sizeof(footer_t));
    // left unification
    if (check_left->magic == HEAP_MAGIC && check_left->header->is_hole) {
        u32 this_size = header->size;
        header = check_left->header;
        footer->header = header;
        header->size += this_size;
        add_to_index = false;
    }

    header_t *check_right = (header_t *) ((u32) footer + sizeof(footer_t));
    if (check_right->magic == HEAP_MAGIC && check_right->is_hole) {
        header->size += check_right->size;

        footer = (footer_t *) ((u32) check_right + check_right->size - sizeof(footer_t));
        u32 i = 0;
        while ((i < heap->index.size) && (ordered_array_get(&heap->index, i) != check_right)) {
            i++;
        }
        assert(i < heap->index.size);
        ordered_array_remove(&heap->index, i);
    }

    if (add_to_index) {
        ordered_array_insert(&heap->index, header);
    }
}


