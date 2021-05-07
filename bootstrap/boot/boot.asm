PAGE_ALIGNED equ 1 << 0
MEMORY_INFO equ 1 << 1

MBFLAGS equ PAGE_ALIGNED | MEMORY_INFO
MBMAGIC equ 0x1BADB002
MBCHECK equ -(MBMAGIC + MBFLAGS)

KiB equ 1024
MiB equ KiB * 1024

section .boot_data
multiboot_info:
    dd MBMAGIC
    dd MBFLAGS
    dd MBCHECK

section .bss
stack_bottom:
resb 16*KiB
stack_top:

global heap_top
heap_top:
resb 16*KiB
heap_bottom:

section .text
global _start:function (_start.end - _start)
_start:
    ; disable interrupts
    cli

    mov esp, $stack_top
    push eax
    push ebx
    extern _boot
    call _boot

    ; halt if we return
    cli
.halt:
    hlt
    jmp .halt
_start.end:

