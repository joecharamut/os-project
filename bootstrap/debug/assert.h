#ifndef OS_ASSERT_H
#define OS_ASSERT_H

#include "panic.h"

#define static_assert _Static_assert

#ifndef NDEBUG
#define assert(condition) \
do { \
    if (!(condition)) { \
        panic("ASSERTION FAILED: %s (in %s:%d %s())", 0, #condition, __FILE__, __LINE__, __func__); \
    } \
} while (0)
#else
#define assert(condition) do {} while (0)
#endif

#endif //OS_ASSERT_H
