bits 16
section .text

global supports_cpuid
supports_cpuid:
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

    ; compare flags, if equal -> no cpuid
    test eax, ecx
    je .ret_false

    ; has cpuid, check extd cpuid
    mov eax, 0x80000000
    push ebx
    cpuid
    pop ebx
    cmp eax, 0x80000001
    jb .ret_false

    ; ret true
    mov eax, 1
    ret

.ret_false:
    xor eax, eax
    ret

global supports_long_mode
supports_long_mode:
     ; check for long mode support
    mov eax, 0x80000001
    push ebx
    cpuid
    pop ebx

    xor eax, eax
    test edx, 1<<29 ; long mode bit
    jz .has_long_mode
    mov eax, 1
.has_long_mode:
    ret