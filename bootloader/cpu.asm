enable_unreal_mode:
    ; save real mode segments
    push ds
    push ss
    push es

    ; load gtd
    lgdt [unreal_gdtinfo]

    ; set protected mode
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; set segments to protected mode descriptor 1
    mov bx, 0x08
    mov ds, bx
    mov ss, bx
    mov es, bx

    ; clear protected mode
    and al, 0xFE
    mov cr0, eax

    ; restore real mode segments
    pop es
    pop ss
    pop ds

    ret

unreal_gdtinfo:
    dw unreal_gdt_end - unreal_gdt - 1
    dd unreal_gdt

unreal_gdt:
    dd 0, 0 ; null descriptor
    db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0 ; flat data descriptor
unreal_gdt_end:

check_a20:
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

    mov ax, 0
    je .exit
    mov ax, 1
.exit:
    pop si
    pop di
    pop es
    pop ds
    popf
    ret

detect_cpuid:
    ; copy flags to eax
    pushfd
    pop eax

    ; store a copy in ecx
    mov ecx, eax

    ; flip the ID bit and pop new flags
    xor eax, 1<<21
    push eax
    popfd

    ; copy flags to eax
    pushfd
    pop eax

    ; restore old flags
    push ecx
    popfd

    ; compare flags, if equal (ZF set) -> no cpuid
    xor eax, ecx
    ret
