#ifndef LOADER_ASSERT_H
#define LOADER_ASSERT_H

#include "print.h"

#define static_assert _Static_assert

#define assert(condition) \
do { \
    if (!(condition)) { \
        print_str("ASSERTION FAILED: "#condition" (in "__FILE__")"); \
    } \
} while (0)

#endif //LOADER_ASSERT_H
