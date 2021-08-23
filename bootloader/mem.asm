bits 16
section .text

global detect_low_ram:function
detect_low_ram:
    clc
    int 12h
    jnc .exit ; carry flag set on error

    ; on error set ax to -1
    mov ax, -1
.exit:
    ret


