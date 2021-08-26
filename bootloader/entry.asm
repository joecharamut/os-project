bits 16
section .text

extern main
global _start:function
_start:
    jmp .real_start
    db "SWAG"

.real_start:
    call main
    cli
    hlt
    jmp $
