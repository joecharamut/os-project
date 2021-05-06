#ifndef OS_DEBUG_H
#define OS_DEBUG_H

typedef enum LOG_LEVEL { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL } LOG_LEVEL;

void dbg_logf(LOG_LEVEL level, const char *fmt, ...);
void dbg_printf(const char *fmt, ...);

#endif //OS_DEBUG_H
