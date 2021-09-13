#ifndef LOADER_CPU_H
#define LOADER_CPU_H

#include <stdint.h>
#include <stdbool.h>

extern bool get_a20_line_state();
extern uint32_t get_system_time();
extern void delay(uint32_t ticks);
extern __attribute__((noreturn)) void enter_long_mode(uint64_t entry);

#endif //LOADER_CPU_H
