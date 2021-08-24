bits 16
org 0x0500

KiB equ 1024
MiB equ (KiB*1024)
GiB equ (MiB*1024)

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
    mov byte [boot_disk], dl

    ; INT10h function 00h - Set Video Mode
    mov ah, 0x00
    mov al, byte [0x0449] ; current video mode byte
    int 10h

    ; INT10h function 0Bh/00h - Set Background Color
    mov ah, 0x0B
    mov bh, 0x00
    mov bl, 0x01 ; color blue
    int 10h

    ; check a20 line
    call check_a20
    test ax, ax
    mov bp, str_no_a20
    jz err_print ; no a20

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
    cmp ebx, 1024 ; load 1024 sectors (512 KiB)
    jle .read_loop

    mov dl, [boot_disk] ; reload disk id
    jmp 0:0x0700 ; jump to stage2

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

str_no_a20: db "No A20 Line", 0
no_int13_msg: db "No INT13 Extensions", 0
read_error: db "Read Error", 0

boot_disk: db 0
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

partition_1: partition_entry 0, 0x7F, 1, (512*KiB)/512 ; type=EXPERIMENTAL, start=512B, size=512KiB [stage2.bin]
partition_2: partition_entry 0, 0x0C, (1*MiB)/512, (64*MiB)/512 ; type=FAT32+LBA, start=1MiB, size=64MiB [boot_fs.bin]
partition_3: partition_entry 0, 0x83, (65*MiB)/512, (64*MiB)/512 ; type=LINUX, start=64MiB, size=1MiB [fs.bin]
partition_4: partition_entry 0, 0x00, 0x00, 0x00 ; type=EMPTY

; boot signature
dw 0xAA55
