PAGE_ALIGNED equ 1 << 0
MEMORY_INFO equ 1 << 1

MBFLAGS equ PAGE_ALIGNED | MEMORY_INFO
MBMAGIC equ 0x1BADB002
MBCHECK equ -(MBMAGIC + MBFLAGS)

KiB equ 1024
MiB equ KiB * 1024

section .boot_data
multiboot_info:
align 4
    dd MBMAGIC
    dd MBFLAGS
    dd MBCHECK

section .bss
stack_bottom:
resb 16*KiB
stack_top:

global heap_ptr
heap_ptr: resb 4

heap_top:
resb 16*KiB
heap_bottom:

section .text
extern _boot
global _start:function (_start.end - _start)
_start:
    ; disable interrupts
    cli

    ; set our own stack
    mov esp, $stack_top

    ; set the heap ptr
    mov dword [$heap_ptr], $heap_top

    ; push multiboot info
    push ebx
    push eax
    ; call the kernel
    call _boot

    ; halt if we return
    cli
.h: hlt
    jmp .h
_start.end:

