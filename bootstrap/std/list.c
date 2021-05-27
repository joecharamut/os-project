#include <mm/kmem.h>
#include <debug/assert.h>
#include <debug/debug.h>
#include "list.h"
#include "string.h"

typedef struct list_object {
    u32 size;
    u32 backing_size;
    list_item_t *items;
} list_t;

list_t *list_create() {
    return list_create_size(4);
}

list_t *list_create_size(u32 initial_size) {
    list_t *list = kmalloc(sizeof(list_t));
    list->size = 0;
    list->backing_size = initial_size;

    assert(initial_size > 0);
    list->items = kcalloc(initial_size, sizeof(list_item_t));
    return list;
}

void list_destroy(list_t *list) {
    kfree(list->items);
    kfree(list);
}

list_item_t list_get(list_t *this, u32 index) {
    assert(index < this->size);
    return this->items[index];
}

void list_remove(list_t *this, u32 index) {
    TODO();
}

void list_append(list_t *this, list_item_t item) {
    if (this->size >= this->backing_size) {
        u32 new_size = this->backing_size * 2;
        assert(new_size > this->backing_size);

        list_item_t *new_array = kcalloc(new_size, sizeof(list_item_t));
        memcpy(new_array, this->items, this->size);

        kfree(this->items);
        this->items = new_array;
        this->backing_size = new_size;
    }

    this->items[this->size] = item;
    this->size++;
}

void list_insert(list_t *this, list_item_t item, u32 index) {
    TODO();
}
