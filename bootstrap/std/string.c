#include <stdbool.h>
#include <debug/debug.h>
#include "string.h"

size_t strlen(const char *str) {
    size_t count = 0;
    while (str[count++]);
    return count - 1;
}

void memset(void *ptr, u8 value, size_t count) {
    for (size_t i = 0; i < count; i++) {
        ((u8 *) ptr)[i] = value;
    }
}

void *memcpy(void *dest, const void *src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        ((u8 *) dest)[i] = ((u8 *) src)[i];
    }
}
