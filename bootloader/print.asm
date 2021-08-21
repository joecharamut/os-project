bits 16
section .text

global print_chr
; void print_chr(char)
print_chr:
    mov ecx, [esp+4]
    push ebx

    mov ah, 0x0E ; Function 0Eh [Teletype Output]
    mov al, cl   ; Char to print
    mov bh, 0    ; Codepage 0
    mov bl, 0    ; Foreground Color 0
    int 10h

    pop ebx
    ret

