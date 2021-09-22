#ifndef LOADER_DEBUG_H
#define LOADER_DEBUG_H

#include <stdint.h>
#include <stdbool.h>

#define static_assert _Static_assert

#define assert(condition) \
do { \
    if (!(condition)) { \
        write_str("ASSERTION FAILED: "#condition" ("__FILE__")"); \
        abort(); \
    } \
} while (0)

#define print_hex(i) write_num((i), 16)
#define print_dec(i) write_num((i), 10)
#define print_hexs(p, i, s) \
do {                        \
    write_str((p));         \
    print_hex((i));         \
    write_str((s));         \
} while (0)
#define print_decs(p, i, s) \
do {                        \
    write_str((p));         \
    print_dec((i));         \
    write_str((s));         \
} while (0)


extern bool serial_init();

__attribute__((noreturn)) void abort();
void write_chr(char c);
void write_str(const char *str);
void write_num(uint64_t num, uint64_t base);
void draw_box(int x, int y, int w, int h, int style, const char *title);

#endif //LOADER_DEBUG_H
