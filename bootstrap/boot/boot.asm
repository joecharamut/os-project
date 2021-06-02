PAGE_ALIGNED equ 1 << 0
MEMORY_INFO equ 1 << 1

MB_MAGIC equ 0x1BADB002
MB_FLAGS equ PAGE_ALIGNED|MEMORY_INFO
MB_CHECK equ -(MB_MAGIC + MB_FLAGS)

section .boot_magic
align 4
multiboot_header:
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECK

section .boot_bss nobits
global boot_stack_top
global boot_stack_bottom
    boot_stack_top:
    resb 256
    boot_stack_bottom:

section .boot_text
extern boot_entrypoint
global _start
_start:
    ; disable interrupts
    cli
    ; set the stack
    mov esp, $boot_stack_bottom
    ; push multiboot vars
    push ebx
    push eax
    ; jump to c code as fast as possible
    call boot_entrypoint
