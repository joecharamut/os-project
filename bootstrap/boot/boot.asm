PAGE_ALIGNED equ 1 << 0
MEMORY_INFO equ 1 << 1

MB_FLAGS equ PAGE_ALIGNED | MEMORY_INFO
MB_MAGIC equ 0x1BADB002
MB_CHECK equ -(MB_MAGIC + MB_FLAGS)

section .boot
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECK

section .bss
stack_bottom:
resb 0x4000
stack_top:

section .text
extern _boot
global _start:function (_start.end - _start)
_start:
    ; disable interrupts
    cli

    ; set the stack
    mov esp, $stack_top

    ; push multiboot info
    push ebx
    push eax
    ; call the kernel
    call _boot

    ; halt if we return
    cli
.h: hlt
    jmp .h
.end:
