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

global get_system_time:function
get_system_time:
    xor eax, eax
    xor edx, edx
    int 1Ah
    mov ax, cx
    shr eax, 16
    or eax, edx
    ret

global delay:function
delay:
    push ebp
    mov ebp, esp
    sub esp, 4

    call get_system_time
    add eax, [ebp+8] ; u32 ticks
    mov [ebp-4], eax ; u32 end_time

.loop:
    nop
    call get_system_time
    cmp eax, [ebp-4] ; u32 end_time
    jb .loop

    mov esp, ebp
    pop ebp
    ret

global peek_keystroke:function
peek_keystroke:
    mov ah, 0x01
    int 16h
    jnz .exit
    xor ax, ax
.exit:
    ret

global pop_keystroke:function
pop_keystroke:
    mov ah, 0x00
    int 16h
    ret

global set_bios_target_mode:function
set_bios_target_mode:
    push ebp
    mov ebp, esp
    push ebx

    mov ax, 0xEC00
    mov bl, [ebp+8]
    int 15h ; INT15h,EC00 [Detect Target Operating Mode Callback]
    jnc .success

    shr ax, 8 ; mov val in ah to al
    test al, al
    jnz .exit
    mov al, 1
    jmp .exit

.success:
    xor eax, eax
.exit:
    pop ebx
    mov esp, ebp
    pop ebp
    ret

global enter_long_mode:function
enter_long_mode:
    xor ax, ax
    mov ss, ax
    add esp, 0x60000 ; prepare the stack for transition out of segmented memory

    push ebp
    mov ebp, esp

    mov al, 0xFF
    out 0xA1, al
    out 0x21, al ; disable all IRQs
    cli          ; disable interrupts
    lidt [IDT]   ; load the null interrupt table to force NMIs to triple fault

    mov eax, cr4
    or eax, 1<<5 | 1<<7 ; set PAE and PGE
    mov cr4, eax

    mov ecx, 0xC0000080 ; EFER MSR
    rdmsr

    or eax, 0x00000100 ; set the LME bit
    wrmsr

    mov eax, cr0
    or eax, 1<<31 | 1<<0 ; set Protected Mode Enable and Paging
    mov cr0, eax

    lgdt [GDT.Pointer]

    push dword [ebp+0xC] ; push high and low dwords of kernel entrypoint
    push dword [ebp+0x8]
    jmp dword 08h:_entry_long_mode ; do it

bits 64
section .text64
_entry_long_mode:
    mov eax, 0x10
    mov ds, eax
    mov ss, eax
    mov es, eax
    mov fs, eax
    mov gs, eax ; set all the data segments

    pop rax ; kernel entrypoint
    jmp rax

bits 16
section .data
GDT:
    .Null: dq 0x0000000000000000
    .Code: dq 0x00209A0000000000
    .Data: dq 0x0000920000000000
align 4
    .Pointer:
        dw $ - GDT - 1
        dd GDT
align 4
IDT:
    .length: dw 0
    .base: dd 0