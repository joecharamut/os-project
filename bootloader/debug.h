#ifndef LOADER_DEBUG_H
#define LOADER_DEBUG_H

#include "print.h"

#define static_assert _Static_assert

#define assert(condition) \
do { \
    if (!(condition)) { \
        print_str("ASSERTION FAILED: "#condition" (in "__FILE__")"); \
    } \
} while (0)

__attribute__((noreturn)) void abort();

#endif //LOADER_DEBUG_H
