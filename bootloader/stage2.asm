bits 16

section .bss
align 16
stack_bottom:
resb 16384 ; 16 KiB
stack_top:

section .start_text
extern main
global _start
_start:
    mov esp, stack_top
    call main

    cli
    hlt
    jmp $
