#include <debug/assert.h>
#include <mm/kmem.h>
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

const char *strchr(const char *str, char character) {
    size_t len = strlen(str);

    for (size_t i = 0; i < len; ++i) {
        if (str[i] == character) {
            return &str[i];
        }
    }

    return NULL;
}

static char *strtok_current_str = NULL;
char *strtok(char *str, const char *delims) {
    if (str != NULL) {
        strtok_current_str = str;
    }

    if (str == NULL && strtok_current_str == NULL) {
        return NULL;
    }

    assert(strtok_current_str != NULL);
    assert(delims != NULL);

    char *start = strtok_current_str;
    while (strchr(delims, *start) != NULL) {
        start++;
    }

    size_t len = strlen(start);
    size_t end = 0;
    while (end < len && *(start + end) && strchr(delims, *(start + end)) == NULL) {
        end++;
    }

    if (end == 0 || end == len) {
        strtok_current_str = NULL;
    } else {
        *(start + end) = 0;
        strtok_current_str = start + end + 1;
    }

    if (end == 0) {
        return NULL;
    }

    return start;
}

char *strdup(const char *str) {
    size_t len = strlen(str) + 1;
    char *new_str = kcalloc(len, sizeof(char));
    memcpy(new_str, str, len);
    return new_str;
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
