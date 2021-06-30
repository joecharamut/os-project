#ifndef OS_INTERRUPTS_H
#define OS_INTERRUPTS_H

#include <std/types.h>

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 46
#define IRQ14 46
#define IRQ15 47

typedef struct {
    u32 ds;
    u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;
    u32 interrupt_num, error_code;
    u32 eip, cs, eflags, user_esp, ss;
} interrupt_registers_t;

typedef struct {
    u16 base_lo;
    u16 selector;
    u8 _padding;
    u8 flags;
    u16 base_hi;
} __attribute__ ((packed)) idt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__ ((packed)) idt_ptr_t;

typedef void (*isr_t)(interrupt_registers_t);

void init_interrupts();
void set_interrupt_handler(u8 number, isr_t handler);

#endif //OS_INTERRUPTS_H
