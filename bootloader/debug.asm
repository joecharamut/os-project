bits 16
section .text

global print_chr:function
print_chr:
    push ebp
    mov ebp, esp
    push bx

    mov ah, 0x0E
    mov al, byte [ebp+8]
    xor bx, bx
    int 10h

    pop bx
    mov esp, ebp
    pop ebp
    ret
