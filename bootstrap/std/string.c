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
    asm volatile (
    "cld       \n"
    "rep stosb \n"
    :
    : "a" (value), "D" (ptr), "c" (count)
    : "memory"
    );
}

void *memcpy(void *dest, const void *src, size_t count) {
    asm volatile (
    "cld        \n"
    "rep movsb  \n"
    :
    : "S" (src), "D" (dest), "c" (count)
    : "memory"
    );

    return dest;
}
