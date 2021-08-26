#ifndef LOADER_CPU_H
#define LOADER_CPU_H

#include <stdint.h>
#include <stdbool.h>

extern bool supports_cpuid();
extern bool supports_long_mode();
extern bool get_a20_line_state();

#endif //LOADER_CPU_H
