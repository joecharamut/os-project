#ifndef OS_STRING_H
#define OS_STRING_H

#include <stddef.h>
#include "types.h"

size_t strlen(const char *str);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t count);
const char *strchr(const char *str, char character);
char *strtok(char *str, const char *delims);
char *strdup(const char *str);

void memset(void *ptr, u8 value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);

#endif //OS_STRING_H
