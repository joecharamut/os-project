section .data
GDTR:
.size:   dw 0
.offset: dd 0

section .text
global gdt_load:function (gdt_load.end - gdt_load)
gdt_load:
    push ebp
    mov ebp, esp

    mov cx, [ebp + 12]
    mov [GDTR.size], cx
    mov ecx, [ebp + 8]
    mov [GDTR.offset], ecx
    lgdt [GDTR]
    jmp 0x08:.cs
.cs:
    mov cx, 0x10
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    mov ss, cx

    leave
    ret
gdt_load.end:
