bits 16
section .text

global print_chr:function
; void print_chr(char)
print_chr:
    push bx

    mov ax, [esp+6] ; Char to print [Param 0]
    mov ah, 0x0E    ; Function 0Eh [Teletype Output]
    xor bx, bx      ; Codepage 0, Foreground Color 0
    int 10h

    pop bx
    ret

