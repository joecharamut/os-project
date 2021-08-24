bits 16
section .text

struc disk_packet
    .size: resb 1
    .padding: resb 1
    .count: resw 1
    .buffer_offset: resw 1
    .buffer_segment: resw 1
    .start_lo: resd 1
    .start_hi: resd 1
endstruc

global disk_read_sectors:function
; uint32_t disk_read_sectors(uint8_t disk, uint8_t *buffer, uint32_t sector, uint32_t count)
disk_read_sectors:
    ; offsets for function arguments
    %define p_count ebp+20
    %define p_sector ebp+16
    %define p_buffer ebp+12
    %define p_disk ebp+8

    ; address for the tmp buffer at 0x70000
    %define tmp_buf 0x70000
    %define tmp_buf_seg 0x7000
    %define tmp_buf_off 0x0000

    ; enter a new stack frame
    push ebp
    mov ebp, esp

    ; preserve esi and edi
    push esi
    push edi
    ; allocate 0x10 bytes on the stack for the disk packet
    sub esp, 0x10

    ; setup disk packet to copy to a temp buffer at 0x80000 (512KiB)
    mov esi, esp
    mov byte [si+disk_packet.size], 0x10
    mov byte [si+disk_packet.padding], 0x00
    mov eax, [p_count]
    mov word [si+disk_packet.count], ax
    mov word [si+disk_packet.buffer_offset], tmp_buf_off
    mov word [si+disk_packet.buffer_segment], tmp_buf_seg
    mov eax, [p_sector]
    mov dword [si+disk_packet.start_lo], eax
    mov dword [si+disk_packet.start_hi], 0

    ; do the disk read
    mov ah, 0x42
    mov dl, [p_disk]
    int 13h
    jc .error ; cf should be set on error
    test ah, ah
    jnz .error ; but sometimes it isnt because sure

    ; setup to copy the data from the temp buffer to the real buffer requested by the caller
    cld
    mov esi, tmp_buf
    mov edi, [p_buffer]

    ; count * 512 bytes per sector
    mov eax, [p_count]
    mov ecx, 512
    mul ecx
    mov ecx, eax

    ; do the copy
    a32 rep movsb

    ; return 0 on success
    xor eax, eax
    jmp .exit

.error:
    mov eax, -1

.exit:
    ; restore edi and esi
    pop edi
    pop esi

    ; leave stack frame
    mov esp, ebp
    pop ebp
    ret

