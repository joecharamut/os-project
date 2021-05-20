#ifndef OS_STRING_H
#define OS_STRING_H

#include <stddef.h>
#include "types.h"

size_t strlen(const char *str);

void memset(void *ptr, u8 value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);

#endif //OS_STRING_H
