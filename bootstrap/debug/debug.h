#ifndef OS_DEBUG_H
#define OS_DEBUG_H

#include "panic.h"
#include <std/registers.h>
#include <stdarg.h>

typedef enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL } LOG_LEVEL;

void dbg_set_level(LOG_LEVEL level);

__attribute__((format(printf, 2, 3))) void dbg_logf(LOG_LEVEL level, const char *fmt, ...);
__attribute__((format(printf, 1, 2))) void dbg_printf(const char *fmt, ...);
__attribute__((format(printf, 1, 0))) void dbg_vprintf(const char *fmt, va_list ap);

void dump_registers(const registers_t *registers);

#define TODO() \
do { \
    panic("TODO in %s:%d %s()", 0, __FILE__, __LINE__, __func__); \
} while (0)

#ifndef NDEBUG
#define BREAKPOINT(MSG) \
do { \
    asm volatile ("int $0x3" :: "d" (MSG)); \
    while (1) asm volatile ("pause"); \
} while (0)
#else
#define BREAKPOINT(MSG) do {} while (0)
#endif

#endif //OS_DEBUG_H
