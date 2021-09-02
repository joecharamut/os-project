bits 16
section .text

; uint16_t get_memory_size()
global get_memory_size:function
get_memory_size:
    clc
    int 12h
    jnc .exit ; carry flag set on error

    ; on error set ax to -1
    mov ax, -1
.exit:
    ret

; uint16_t get_extended_memory_size()
global get_extended_memory_size:function
get_extended_memory_size:
    clc
    xor eax, eax
    mov ah, 88h
    int 15h      ; int 15h,88h [Get Extended Memory Size]
    jnc .exit    ; cf set on error

    ; return -1 on error
    mov ax, -1
.exit:
    ret

; uint32_t get_system_memory_map(bios_mmap_entry_t *entry_buffer)
global get_system_memory_map:function
get_system_memory_map:
    ; enter stack frame
    push ebp
    mov ebp, esp

    ; save important registers
    push esi
    push edi
    push ebx
    push es

    mov ax, 0x7000
    mov es, ax
    mov di, 0x0000      ; buffer [es:di] = 0x80000
    xor ebx, ebx        ; index = 0
    xor esi, esi        ; counter = 0

.read_loop:
    mov eax, 0x0000E820 ; function E820h
    mov edx, 0x534D4150 ; 'SMAP'
    mov ecx, 20         ; buffer size
    int 15h
    jc .loop_done       ; carry set on error, or if finished on some bioses

    add edi, 20         ; move buffer to next entry
    inc esi             ; inc counter

    test ebx, ebx       ; ebx = next entry or 0 if done
    jnz .read_loop
.loop_done:

    pop es              ; restore original es
    push esi            ; save the num of entries

    test esi, esi
    jz .skip_copy       ; if 0 mmap entries, we're all done

    ; copy to buffer
    mov ecx, 20         ; 20 bytes per entry
    mov eax, esi
    mul ecx
    mov ecx, eax        ; count = entries * size
    mov esi, 0x70000    ; src = tmp buffer
    mov edi, [ebp+8]    ; dst = real buffer
    a32 rep movsb       ; do the copy

.skip_copy:
    pop eax             ; pop entry count into return value

.exit:
    ; restore important registers
    pop ebx
    pop edi
    pop esi

    ; leave stack frame
    mov esp, ebp
    pop ebp
    ret

; void *memcpy(void *dst, void *src, uint32_t count)
global memcpy:function
memcpy:
    ; enter stack frame
    push ebp
    mov ebp, esp

    push esi
    push edi

    mov ecx, [ebp+16] ; count
    mov esi, [ebp+12] ; src
    mov edi, [ebp+8] ; dst
    a32 rep movsb

    pop edi
    pop esi
    mov eax, [ebp+8] ; dst

    ; leave stack frame
    mov esp, ebp
    pop ebp
    ret

; uint32_t strncmp(const char *str1, const char *str2, uint32_t count)
global strncmp:function
strncmp:
    ; enter stack frame
        push ebp
        mov ebp, esp

        push esi
        push edi

        mov ecx, [ebp+16] ; count
        mov edi, [ebp+12] ; str2
        mov esi, [ebp+8] ; str1
        a32 repe cmpsb

        jg .greater
        jl .lesser

        ; equal
        xor eax, eax
        jmp .return

.greater:
        mov eax, 1
        jmp .return

.lesser:
        mov eax, -1
        jmp .return

.return:
        pop edi
        pop esi

        ; leave stack frame
        mov esp, ebp
        pop ebp
        ret


