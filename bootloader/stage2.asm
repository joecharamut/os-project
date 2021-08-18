bits 16
section .start_text

extern main
global _start
_start:
    call main

    cli
    hlt
    jmp $
