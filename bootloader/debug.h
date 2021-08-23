#ifndef LOADER_DEBUG_H
#define LOADER_DEBUG_H

#include <stdint.h>

#define static_assert _Static_assert

#define assert(condition) \
do { \
    if (!(condition)) { \
        print_str("ASSERTION FAILED: "#condition" ("__FILE__")"); \
        abort(); \
    } \
} while (0)

__attribute__((noreturn)) void abort();

#define print_hex(i) print_num((i), 16)
#define print_dec(i) print_num((i), 10)
#define print_hexs(p, i, s) \
do {                        \
    print_str((p));         \
    print_hex((i));         \
    print_str((s));         \
} while (0)
#define print_decs(p, i, s) \
do {                        \
    print_str((p));         \
    print_dec((i));         \
    print_str((s));         \
} while (0)


void print_chr(char c);
void print_str(const char *str);
void print_num(uint32_t num, uint32_t base);

#endif //LOADER_DEBUG_H
