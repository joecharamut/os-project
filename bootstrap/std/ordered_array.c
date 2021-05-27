#include <debug/panic.h>
#include <debug/debug.h>
#include <mm/kmem.h>
#include <debug/assert.h>
#include "ordered_array.h"
#include "string.h"

int default_compare_function(type_t a, type_t b) {
    return a < b ? 1 : 0;
}

ordered_array_t ordered_array_create(u32 max_size, compare_func_t compare_func) {
    PANIC("todo");
}

ordered_array_t ordered_array_create_at(void *addr, u32 max_size, compare_func_t compare_func) {
    ordered_array_t array = {
            .array = (type_t *) addr,
            .size = 0,
            .max_size = max_size,
            .was_mallocated = false,
            .compare_function = compare_func
    };
    memset(array.array, 0, sizeof(type_t) * max_size);

    return array;
}

void ordered_array_destroy(ordered_array_t *array) {
    if (array->was_mallocated) {
//        kfree(array);
    }
}

void ordered_array_insert(ordered_array_t *array, type_t item) {
    assert(array->compare_function);

    u32 i = 0;
    while (i < array->size && array->compare_function(array->array[i], item)) {
        i++;
    }

    if (i == array->size) {
        array->array[array->size++] = item;
    } else {
        type_t tmp = array->array[i];
        array->array[i] = item;
        while (i < array->size) {
            i++;
            type_t tmp2 = array->array[i];
            array->array[i] = tmp;
            tmp = tmp2;
        }
        array->size++;
    }
}

type_t ordered_array_get(ordered_array_t *array, u32 index) {
    assert(index < array->size);
    return array->array[index];
}

void ordered_array_remove(ordered_array_t *array, u32 index) {
    while (index < array->size) {
        array->array[index] = array->array[index + 1];
        index++;
    }

    array->size--;
}

