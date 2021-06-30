#ifndef OS_TIMER_H
#define OS_TIMER_H

#include <std/types.h>

extern u64 global_timer;
void init_timer(u32 frequency);

#endif //OS_TIMER_H
