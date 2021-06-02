#ifndef OS_LIST_H
#define OS_LIST_H

#include "types.h"

typedef void* list_item_t;

typedef struct list_object {
    u32 size;
    u32 backing_size;
    list_item_t *items;
} list_t;

list_t *list_create();
list_t *list_create_size(u32 initial_size);
void list_destroy(list_t *list);

list_item_t list_get(list_t *this, u32 index);
void list_remove(list_t *this, u32 index);
void list_append(list_t *this, list_item_t item);
void list_insert(list_t *this, list_item_t item, u32 index);

#endif //OS_LIST_H
