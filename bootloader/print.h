#ifndef LOADER_PRINT_H
#define LOADER_PRINT_H

#include <stdint.h>

#define print_hex(i) print_num((i), 16)
#define print_dec(i) print_num((i), 10)

extern void print_chr(char c);
void print_str(const char *str);
void print_num(uint32_t num, uint32_t base);

#endif //LOADER_PRINT_H
