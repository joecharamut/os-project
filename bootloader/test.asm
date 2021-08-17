bits 16

section .text

global test
test:
    mov eax, 0x41414242
    cli
    hlt
