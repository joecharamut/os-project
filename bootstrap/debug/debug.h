#ifndef OS_DEBUG_H
#define OS_DEBUG_H

#include <stdarg.h>
#include <boot/interrupts.h>
#include "panic.h"

typedef enum LOG_LEVEL { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL } LOG_LEVEL;

void dbg_logf(LOG_LEVEL level, const char *fmt, ...);
void dbg_printf(const char *fmt, ...);
void dbg_vprintf(const char *fmt, va_list ap);

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
