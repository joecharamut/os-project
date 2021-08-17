bits 16
org 0x0E00

_boot:
    mov byte [boot_disk], dl ; save disk id

    ; move stack to 0x0DFF
    cli
    xor ax, ax
    mov ss, ax
    mov si, 0x0DFF
    sti

    ; copy mbr to 0x0E00
    mov si, 0x7C00
    mov di, 0x0E00
    mov cx, 512
    rep movsb

    ; jump to relocated mbr
    jmp 0:_boot_reloc

_boot_reloc:
    ; INT13h function 41h - Check Extensions Present
    mov ah, 0x41
    mov bx, 0x55AA
    int 13h
    mov bp, no_int13_msg
    jc err_print ; CF set if not present

    ; check if partition 1 is stage2 code
    cmp byte [partition_1.type], 0x7F
    mov bp, stage2_err_msg
    jne err_print

    mov ebx, [partition_1.first_sector]
    mov ecx, 0x1000
.read_loop:
    mov word [disk_packet.count], 1
    mov dword [disk_packet.buffer], ecx
    mov dword [disk_packet.start_lo], ebx

    ; INT13h function 42h - Extended Read Sectors
    mov ah, 0x42
    mov si, disk_packet
    int 13h
    mov bp, read_error
    jc err_print ; CF set on error

    inc ebx
    add ecx, 512
    cmp ebx, 0x78 ; loads 0x1000 - 0xFFFF (60 KiB)
    jle .read_loop

    mov dl, [boot_disk] ; reload disk id
    jmp 0:0x1000 ; jump to stage2

err_print:
    mov ah, 0Eh ; teletype output
    mov bh, 0 ; codepage
    mov bl, 00001001b ; attributes
    mov al, [bp] ; char to display
.loop:
    int 10h
    inc bp
    mov al, [bp]
    test al, al
    jnz .loop
.exit:
    cli
    hlt

no_int13_msg: db "Error: No INT13h Extensions", 0
read_error: db "Read Error", 0
stage2_err_msg: db "Error loading stage2", 0

boot_disk: db 0
disk_packet:
    db 0x10
    db 0
    .count: dw 0
    .buffer: dd 0
    .start_lo: dd 0
    .start_hi: dd 0

; pad rest of boot code area with zeros
times 440 - ($-$$) db 0

; partition table
%macro partition_entry 4
    %if %1 == 0
        .status: db 0x00 ; status=inactive
    %else
        .status: db 0x80 ; status=active
    %endif
    db 0, 0, 0 ; CHS addr of first sector
    .type: db %2 ; partition type
    db 0, 0, 0 ; CHS addr of last sector
    .first_sector: dd %3 ; LBA of first sector
    .num_sectors: dd %4 ; number of sectors
%endmacro

; disk identifier
dd 0x1234ABCD
; 0x0000 = read-write, 0x5A5A = read only
dw 0x0000

partition_1: partition_entry 0, 0x7F, 1, 128 ; type=EXPERIMENTAL, start=512B, size=64KiB
partition_2: partition_entry 0, 0x83, 2048, 2048 ; type=LINUX, start=1MiB, size=1MiB
partition_3: partition_entry 0, 0x00, 0x00, 0x00 ; type=EMPTY
partition_4: partition_entry 0, 0x00, 0x00, 0x00 ; type=EMPTY

; boot signature
dw 0xAA55
