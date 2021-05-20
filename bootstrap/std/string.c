#include <stdbool.h>
#include <debug/debug.h>
#include "string.h"

size_t strlen(const char *str) {
    size_t count = 0;
    while (str[count++]);
    return count - 1;
}

void memset(void *ptr, u8 value, size_t count) {
    // todo align then rep stosl for all sets
    if ((u32) ptr & 0x3 || count % 4) {
        dbg_logf(LOG_WARN, "memset: not 32 bit aligned\n");
        for (size_t i = 0; i < count; i++) {
            ((u8 *) ptr)[i] = value;
        }
    } else {
        asm volatile (
        "cld    \n"
        "rep    \n"
        "stosl  \n"
        :
        : "a" (value), "D" (ptr), "c" (count / 4)
        : "memory"
        );
    }
}

void *memcpy(void *dest, const void *src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        ((u8 *) dest)[i] = ((u8 *) src)[i];
    }
}
