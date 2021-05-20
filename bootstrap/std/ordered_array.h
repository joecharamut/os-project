#ifndef OS_ORDERED_ARRAY_H
#define OS_ORDERED_ARRAY_H

#include <stdbool.h>
#include "types.h"

typedef void * type_t;
typedef int (*compare_func_t)(type_t, type_t);

typedef struct {
    type_t *array;
    u32 size;
    u32 max_size;
    bool was_mallocated;
    compare_func_t compare_function;
} ordered_array_t;

int default_compare_function(type_t a, type_t b);

ordered_array_t ordered_array_create(u32 max_size, compare_func_t compare_func);
ordered_array_t ordered_array_create_at(void *addr, u32 max_size, compare_func_t compare_func);

void ordered_array_destroy(ordered_array_t *array);
void ordered_array_insert(ordered_array_t *array, type_t item);
type_t ordered_array_get(ordered_array_t *array, u32 index);
void ordered_array_remove(ordered_array_t *array, u32 index);


#endif //OS_ORDERED_ARRAY_H
