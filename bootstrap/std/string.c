#include <debug/debug.h>
#include "string.h"

size_t strlen(const char *str) {
    size_t count = 0;
    while (str[count++]);
    return count - 1;
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }

    return *(u8 *) str1 - *(u8 *) str2;
}

int strncmp(const char *str1, const char *str2, size_t count) {
    while (count > 0 && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
        count--;
    }

    if (count == 0) {
        return 0;
    } else {
        return *(u8 *) str1 - *(u8 *) str2;
    }
}

void memset(void *ptr, u8 value, size_t count) {
    dbg_logf(LOG_DEBUG, "memset stosb ptr: 0x%08x, count: 0x%x\n", ptr, count);

    asm volatile (
    "cld       \n"
    "rep stosb \n"
    :
    : "a" (value), "D" (ptr), "c" (count)
    : "memory"
    );
}

void *memcpy(void *dest, const void *src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        ((u8 *) dest)[i] = ((u8 *) src)[i];
    }

    return dest;
}
