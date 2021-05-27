#include <debug/debug.h>
#include <io/port.h>
#include "interrupts.h"

#define IDT_SIZE 256
isr_t interrupt_handlers[IDT_SIZE];
idt_entry_t idt_entries[IDT_SIZE];
idt_ptr_t idt_ptr;

void __attribute__((used)) isr_handler(registers_t registers) {
    if (interrupt_handlers[registers.interrupt_num]) {
        interrupt_handlers[registers.interrupt_num](registers);
    } else {
        dbg_logf(LOG_ERROR, "Unhandled Exception: int%xh (Err Code: 0x%x)\n", registers.interrupt_num, registers.error_code);
    }
}

void __attribute__((used)) irq_handler(registers_t registers) {
    if (registers.interrupt_num >= 40) {
        // send reset to PIC2
        outb(0xA0, 0x20);
    }
    // send reset to PIC1
    outb(0x20, 0x20);

    if (interrupt_handlers[registers.interrupt_num]) {
        interrupt_handlers[registers.interrupt_num](registers);
    } else {
        dbg_logf(LOG_TRACE, "Unhandled IRQ: %d\n", registers.interrupt_num - 32);
    }
}

void set_interrupt_handler(u8 number, isr_t handler) {
    interrupt_handlers[number] = handler;
}

extern void load_idt_ptr(u32);

static void init_idt();
static void init_pic();
static void set_idt_entry(int index, u32 addr, u16 sel, u8 flags);

void init_interrupts() {
    // load the pointer
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_SIZE - 1;
    idt_ptr.base = (u32) &idt_entries;

    // clear the table
    for (int i = 0; i < IDT_SIZE; i++) {
        idt_entries[i] = (idt_entry_t) { 0 };
    }

    // load int0 - int31
    init_idt();
    // load irq0-irq15 (int32-int47)
    init_pic();

    load_idt_ptr((u32) &idt_ptr);

    asm volatile ("sti\n");
}

static void init_idt() {
    extern void isr0();
    extern void isr1();
    extern void isr2();
    extern void isr3();
    extern void isr4();
    extern void isr5();
    extern void isr6();
    extern void isr7();
    extern void isr8();
    extern void isr9();
    extern void isr10();
    extern void isr11();
    extern void isr12();
    extern void isr13();
    extern void isr14();
    extern void isr15();
    extern void isr16();
    extern void isr17();
    extern void isr18();
    extern void isr19();
    extern void isr20();
    extern void isr21();
    extern void isr22();
    extern void isr23();
    extern void isr24();
    extern void isr25();
    extern void isr26();
    extern void isr27();
    extern void isr28();
    extern void isr29();
    extern void isr30();
    extern void isr31();
    set_idt_entry(0, (u32) isr0, 0x08, 0x8E);
    set_idt_entry(1, (u32) isr1, 0x08, 0x8E);
    set_idt_entry(2, (u32) isr2, 0x08, 0x8E);
    set_idt_entry(3, (u32) isr3, 0x08, 0x8E);
    set_idt_entry(4, (u32) isr4, 0x08, 0x8E);
    set_idt_entry(5, (u32) isr5, 0x08, 0x8E);
    set_idt_entry(6, (u32) isr6, 0x08, 0x8E);
    set_idt_entry(7, (u32) isr7, 0x08, 0x8E);
    set_idt_entry(8, (u32) isr8, 0x08, 0x8E);
    set_idt_entry(9, (u32) isr9, 0x08, 0x8E);
    set_idt_entry(10, (u32) isr10, 0x08, 0x8E);
    set_idt_entry(11, (u32) isr11, 0x08, 0x8E);
    set_idt_entry(12, (u32) isr12, 0x08, 0x8E);
    set_idt_entry(13, (u32) isr13, 0x08, 0x8E);
    set_idt_entry(14, (u32) isr14, 0x08, 0x8E);
    set_idt_entry(15, (u32) isr15, 0x08, 0x8E);
    set_idt_entry(16, (u32) isr16, 0x08, 0x8E);
    set_idt_entry(17, (u32) isr17, 0x08, 0x8E);
    set_idt_entry(18, (u32) isr18, 0x08, 0x8E);
    set_idt_entry(19, (u32) isr19, 0x08, 0x8E);
    set_idt_entry(20, (u32) isr20, 0x08, 0x8E);
    set_idt_entry(21, (u32) isr21, 0x08, 0x8E);
    set_idt_entry(22, (u32) isr22, 0x08, 0x8E);
    set_idt_entry(23, (u32) isr23, 0x08, 0x8E);
    set_idt_entry(24, (u32) isr24, 0x08, 0x8E);
    set_idt_entry(25, (u32) isr25, 0x08, 0x8E);
    set_idt_entry(26, (u32) isr26, 0x08, 0x8E);
    set_idt_entry(27, (u32) isr27, 0x08, 0x8E);
    set_idt_entry(28, (u32) isr28, 0x08, 0x8E);
    set_idt_entry(29, (u32) isr29, 0x08, 0x8E);
    set_idt_entry(30, (u32) isr30, 0x08, 0x8E);
    set_idt_entry(31, (u32) isr31, 0x08, 0x8E);
}

static void set_idt_entry(int index, u32 addr, u16 sel, u8 flags) {
    idt_entries[index] = (idt_entry_t) {
            .base_lo = (addr & 0xFFFF),
            .base_hi = (addr >> 16) & 0xFFFF,
            .selector = sel,
            .flags = flags,
    };
}

static void init_pic() {
    // remap irq table
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    outb(0x21, 0x20);
    outb(0xA1, 0x28);

    outb(0x21, 0x04);
    outb(0xA1, 0x02);

    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    extern void irq0();
    extern void irq1();
    extern void irq2();
    extern void irq3();
    extern void irq4();
    extern void irq5();
    extern void irq6();
    extern void irq7();
    extern void irq8();
    extern void irq9();
    extern void irq10();
    extern void irq11();
    extern void irq12();
    extern void irq13();
    extern void irq14();
    extern void irq15();
    set_idt_entry(32, (u32) irq0, 0x08, 0x8E);
    set_idt_entry(33, (u32) irq1, 0x08, 0x8E);
    set_idt_entry(34, (u32) irq2, 0x08, 0x8E);
    set_idt_entry(35, (u32) irq3, 0x08, 0x8E);
    set_idt_entry(36, (u32) irq4, 0x08, 0x8E);
    set_idt_entry(37, (u32) irq5, 0x08, 0x8E);
    set_idt_entry(38, (u32) irq6, 0x08, 0x8E);
    set_idt_entry(39, (u32) irq7, 0x08, 0x8E);
    set_idt_entry(40, (u32) irq8, 0x08, 0x8E);
    set_idt_entry(41, (u32) irq9, 0x08, 0x8E);
    set_idt_entry(42, (u32) irq10, 0x08, 0x8E);
    set_idt_entry(43, (u32) irq11, 0x08, 0x8E);
    set_idt_entry(44, (u32) irq12, 0x08, 0x8E);
    set_idt_entry(45, (u32) irq13, 0x08, 0x8E);
    set_idt_entry(46, (u32) irq14, 0x08, 0x8E);
    set_idt_entry(47, (u32) irq15, 0x08, 0x8E);
}
