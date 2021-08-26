bits 16
org 0x0500

_boot:
    ; relocate mbr to 0x0500 - 0x06FF
    mov si, 0x7C00
    mov di, 0x0500
    mov cx, 512
    rep movsb

    ; jump to relocated mbr and reload CS
    jmp 0x0000:_boot_reloc

_boot_reloc:
    ; save boot disk id
    push es
    mov ax, 0x7000
    mov es, ax
    mov [es:0x0000], dl
    pop es

    ; INT10h function 00h - Set Video Mode
    mov ah, 0x00
    mov al, byte [0x0449] ; current video mode byte
    int 10h

    ; INT10h function 0Bh/00h - Set Background Color
    mov ah, 0x0B
    mov bh, 0x00
    mov bl, 0x01 ; color blue
    int 10h

    ; enable unreal mode
    call enable_unreal_mode

    ; switch to the real stack
    mov ax, 0x6000
    mov ss, ax
    mov sp, 0xFF00

    ; INT13h function 41h - Check Extensions Present
    mov ah, 0x41
    mov bx, 0x55AA
    int 13h
    mov bp, no_int13_msg
    jc err_print ; CF set if not present

    mov ebx, [partition_1.first_sector]
    mov cx, 0x0070
.read_loop:
    mov word [disk_packet.count], 1
    mov word [disk_packet.buffer_segment], cx
    mov dword [disk_packet.start_lo], ebx

    ; INT13h function 42h - Extended Read Sectors
    mov ah, 0x42
    mov si, disk_packet
    int 13h
    mov bp, read_error
    jc err_print ; CF set on error

    inc ebx
    add cx, 32
    cmp ebx, 768 ; load 768 sectors (384 KiB)
    jle .read_loop

    jmp 0x0000:0x0700 ; jump to stage2

; ======== functions ========

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

no_int13_msg: db "No INT13 Extensions", 0
read_error: db "Read Error", 0

disk_packet:
    db 0x10
    db 0
    .count: dw 0
    .buffer_offset: dw 0
    .buffer_segment: dw 0
    .start_lo: dd 0
    .start_hi: dd 0

unreal_gdtinfo:
    dw unreal_gdt_end - unreal_gdt - 1
    dd unreal_gdt

unreal_gdt:
    dd 0, 0 ; null descriptor
    db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0 ; flat data descriptor
unreal_gdt_end:

; pad rest of boot code area with zeros
times 440 - ($-$$) db 0

; disk identifier
dd 0x1234ABCD
; 0x0000 = read-write, 0x5A5A = read only
dw 0x0000

%macro partition_entry 4
    .status: db %1 ; 0x80 for active, 0x00 for inactive
    db 0, 0, 0 ; CHS addr of first sector
    .type: db %2 ; partition type
    db 0, 0, 0 ; CHS addr of last sector
    .first_sector: dd %3 ; LBA of first sector
    .num_sectors: dd %4 ; number of sectors
%endmacro
%define SECTOR(x) ((x)/512)

KiB equ 1024
MiB equ (KiB*1024)
GiB equ (MiB*1024)

; partition table
partition_1: partition_entry 0x80, 0xDA, SECTOR(    512), SECTOR(512*KiB) ; boot=Y, type=DATA, start=512B, size=512KiB [stage2.bin]
partition_2: partition_entry 0x00, 0x0C, SECTOR(  1*MiB), SECTOR( 64*MiB) ; boot=N, type=FAT32+LBA, start=1MiB, size=64MiB [boot_fs.bin]
partition_3: partition_entry 0x00, 0x83, SECTOR( 65*MiB), SECTOR( 64*MiB) ; boot=N, type=LINUX, start=64MiB, size=1MiB [fs.bin]
partition_4: partition_entry 0x00, 0x00, 0x00, 0x00                       ; boot=N, type=EMPTY

; boot signature
dw 0xAA55
