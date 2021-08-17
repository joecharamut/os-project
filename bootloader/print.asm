print_str:
    push bx
    push bp
    mov bp, ax

    mov ah, 0Eh ; teletype output
    mov bh, 0 ; codepage
    mov bl, 00001001b ; attributes (black background, light grey foreground)
    mov al, [bp] ; char to display
.loop:
    int 10h
    inc bp
    mov al, [bp]
    test al, al
    jnz .loop

    pop bp
    pop bx
    ret
