bits 16
section .text

global get_a20_line_state:function
get_a20_line_state:
    pushf
    push ds
    push es
    push di
    push si

    xor ax, ax
    mov es, ax ; es = 0x0000

    not ax
    mov ds, ax ; ds = 0xFFFF

    mov di, 0x0500
    mov si, 0x0510

    mov al, byte [es:di]
    push ax
    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
    cmp byte [es:di], 0xFF

    pop ax
    mov byte [ds:si], al
    pop ax
    mov byte [es:di], al

    mov eax, 0
    je .exit
    mov eax, 1
.exit:
    pop si
    pop di
    pop es
    pop ds
    popf
    ret

