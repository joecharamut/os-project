MBALIGN equ 1 << 0
MEMINFO equ 1 << 1
MBFLAGS equ MBALIGN | MEMINFO
MBMAGIC equ 0x1BADB002
MBCHECK equ -(MBMAGIC + MBFLAGS)

section .boot_magic
align 4
dd MBMAGIC
dd MBFLAGS
dd MBCHECK

section .boot_stack nobits
boot_stack_bottom:
resb 0x1000
boot_stack_top:

section .boot_text
global _start:function (_start.end - _start)
_start:
    ; make extra sure interrupts are disabled
    cli

    ; setup paging in c (like a boss)
    mov esp, $boot_stack_top
    extern _boot_paging_init
    call _boot_paging_init

    ; off we go
    jmp _high_start
_start.end:

section .bss
stack_bottom:
resb 0x4000
stack_top:

align 4096
boot_page_directory:
    resb 0x1000
boot_page_table:
    resb 0x1000
ident_page_table:
    resb 0x1000

section .text
global _high_start:function (_high_start.end - _high_start)
_high_start:
;    ; unmap identity kernel page
;    mov long [boot_page_directory], 0
;
;    ; remap just the first mb
;    mov long [boot_page_directory], PHYS_ADDR(ident_page_table) + 3
;    mov edi, ident_page_table
;    mov esi, 0
;    mov ecx, 0
;.id_loop:
;    mov edx, esi
;    or  edx, 3
;    mov [edi], edx
;    add esi, 4096
;    add edi, 4
;
;    inc ecx
;    cmp ecx, 256
;    jne .id_loop
;
;    ; flush tlb
;    mov ecx, cr3
;    mov cr3, ecx

    ; setup the stack
    mov esp, stack_top

    ; setup gdt
    extern gdt_init
    call gdt_init

    ; setup idt
    extern idt_init
    call idt_init

    ; enter kernel
    sti
    extern _init
    call _init

    ; if we return, disable interrupts and halt
    cli
.h: hlt
    jmp .h
_high_start.end:
