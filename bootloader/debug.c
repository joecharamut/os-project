#include "debug.h"

__attribute__((noreturn)) void abort() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}
