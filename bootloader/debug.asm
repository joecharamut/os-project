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

global set_chr:function
set_chr:
    push ebp
    mov ebp, esp
    push bx

    ; save cursor position
    mov ah, 0x03
    xor bx, bx
    int 10h
    push dx

    ; set cursor position
    mov ah, 0x02
    xor bx, bx
    mov dh, byte [ebp+16] ; x
    mov dl, byte [ebp+12] ; y
    int 10h

    ; write character
    mov ah, 0x0A
    mov al, byte [ebp+8]
    xor bx, bx
    mov cx, 1
    int 10h

    ; restore cursor position
    mov ah, 0x02
    xor bx, bx
    pop dx
    int 10h

    pop bx
    mov esp, ebp
    pop ebp
    ret

global set_cursor_pos:function
set_cursor_pos:
    push ebp
    mov ebp, esp

    push ebx

    mov ah, 0x02
    xor bx, bx
    mov dh, byte [ebp+12] ; x
    mov dl, byte [ebp+8] ; y
    int 10h

    pop ebx

    mov esp, ebp
    pop ebp
    ret

global beep:function
beep:
    push ebp
    mov ebp, esp
    pushad

    ; control word
    mov al, 0xB6
    out 43h, al

    ; send frequency
    mov ax, [ebp+8]
    out 42h, al
    mov al, ah
    out 42h, al

    ; enable the pc speaker gate
    in al, 61h
    or al, 03h
    out 61h, al

    ; delay cx:dx microseconds
    mov cx, [ebp+14]
    mov dx, [ebp+12]
    mov ah, 86h
    int 15h

    ; disable pc speaker
    in al, 61h
    and al, 0xFC
    out 61h, al

    popad
    mov esp, ebp
    pop ebp
    ret


%define COM1 0x3F8
%macro outb 2
    mov dx, %1
    mov al, %2
    out dx, al
%endmacro
%macro inb 1
    mov dx, %1
    in al, dx
%endmacro
global serial_init:function
serial_init:
    push ebp
    mov ebp, esp

    outb COM1+1, 0x00 ; Disable all interrupts
    outb COM1+3, 0x80 ; Enable DLAB (set baud rate divisor)
    outb COM1+0, 0x03 ; Set divisor to 3 (lo byte) 38400 baud
    outb COM1+1, 0x00 ;                  (hi byte)
    outb COM1+3, 0x03 ; 8 bits, no parity, one stop bit
    outb COM1+2, 0xC7 ; Enable FIFO, clear them, with 14-byte threshold
    outb COM1+4, 0x0B ; IRQs enabled, RTS/DSR set
    outb COM1+4, 0x1E ; Set in loopback mode, test the serial chip
    outb COM1+0, 0xAE ; Test serial chip (send byte 0xAE and check if serial returns same byte)

    ; Check if serial is faulty (i.e: not same byte as sent)
    inb COM1+0
    cmp al, 0xAE
    je .works
    ; doesnt work
    mov eax, 0
    jmp .exit

.works:
    ; If serial is not faulty set it in normal operation mode
    ; (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb COM1+4, 0x0F
    mov eax, 1

.exit:
    mov esp, ebp
    pop ebp
    ret

global serial_write:function
serial_write:
    push ebp
    mov ebp, esp

    outb COM1+0, byte [ebp+8]

    mov esp, ebp
    pop ebp
    ret




