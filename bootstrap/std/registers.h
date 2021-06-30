#ifndef OS_REGISTERS_H
#define OS_REGISTERS_H

#include "types.h"

typedef struct {
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 eflags;
    u32 eip;
} __attribute__((packed)) registers_t;

#define SAVE_REGISTERS(__registers) \
do { asm volatile ( \
    "pusha              \n" \
    "pop 0x00(%0)       \n" \
    "pop 0x04(%0)       \n" \
    "pop 0x08(%0)       \n" \
    "pop 0x0C(%0)       \n" \
    "pop 0x10(%0)       \n" \
    "pop 0x14(%0)       \n" \
    "pop 0x18(%0)       \n" \
    "pop 0x1C(%0)       \n" \
    "pushf              \n" \
    "pop 0x20(%0)       \n" \
    "movl $., 0x24(%0)  \n" \
    :: "r" (__registers) : "memory" \
); } while (0)

#endif //OS_REGISTERS_H
