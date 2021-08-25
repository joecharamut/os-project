#ifndef LOADER_CPU_H
#define LOADER_CPU_H

#include <stdint.h>

extern uint32_t supports_cpuid();
extern uint32_t supports_long_mode();
extern uint32_t a20_line_enabled();

#endif //LOADER_CPU_H
