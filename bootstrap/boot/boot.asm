PAGE_ALIGNED equ 1 << 0
MEMORY_INFO equ 1 << 1

MB_FLAGS equ PAGE_ALIGNED | MEMORY_INFO
MB_MAGIC equ 0x1BADB002
MB_CHECK equ -(MB_MAGIC + MB_FLAGS)

section .boot_data
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECK

section .boot_bss nobits
    resb 0x1000
    boot_stack:

section .boot_text
extern _early_boot
global _start:function (_start.end - _start)
_start:
    ; disable interrupts
    cli
    ; set the stack
    mov esp, $boot_stack
    ; jump to c code as fast as possible
    jmp _early_boot
.end:
