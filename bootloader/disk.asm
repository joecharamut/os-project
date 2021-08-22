bits 16
section .text

struc disk_packet
    .size: resb 1
    .padding: resb 1
    .count: resw 1
    .buffer: resd 1
    .start_lo: resd 1
    .start_hi: resd 1
endstruc

global __disk_read_sectors:function
; uint32_t disk_read_sectors(uint8_t disk, uint8_t *buffer, uint32_t sector, uint32_t count)
__disk_read_sectors:
    %define p_count ebp+20
    %define p_sector ebp+16
    %define p_buffer ebp+12
    %define p_disk ebp+8

    push ebp
    mov ebp, esp
    push esi
    push edi
    sub esp, 0x10

    mov esi, esp
    mov byte [si+disk_packet.size], 0x10
    mov byte [si+disk_packet.padding], 0
    mov eax, [p_count]
    mov word [si+disk_packet.count], ax
    mov eax, [p_buffer]
    mov dword [si+disk_packet.buffer], eax
    mov eax, [p_sector]
    mov dword [si+disk_packet.start_lo], eax
    mov dword [si+disk_packet.start_hi], 0

    mov ah, 0x42
    mov di, [p_disk]
    int 13h

    pop edi
    pop esi
    mov esp, ebp
    pop ebp
    ret