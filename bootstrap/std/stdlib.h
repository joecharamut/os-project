#include <stdbool.h>
#include "types.h"

#ifndef OS_STDIO_H
#define OS_STDIO_H

int atoi(const char *str);

bool isalnum(char c);
bool isalpha(char c);
bool iscntrl(char c);
bool isdigit(char c);
bool islower(char c);
bool isprint(char c);
bool ispunct(char c);
bool isspace(char c);
bool isupper(char c);

char tolower(char c);
char toupper(char c);

#endif //OS_STDIO_H
