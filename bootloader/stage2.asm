bits 16
org 0x1000
jmp _start

KiB equ 1024
MiB equ KiB * 1024
GiB equ MiB * 1024

_start:
    cli

    ; hello, world!
    mov ax, str_hello_world
    call print_str

    ; check for cpuid
    call detect_cpuid
    jnz .has_cpuid

    ; no cpuid
    mov ax, str_no_cpuid
    call print_str
    hlt

.has_cpuid:
    ; check for extended cpuid
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jae .has_extd_cpuid

    ; no extended cpuid
    mov ax, str_no_extd_cpuid
    call print_str
    hlt

.has_extd_cpuid:
    ; check for long mode support
    mov eax, 0x80000001
    cpuid
    test edx, 1<<29 ; long mode bit
    jnz .has_long_mode

    ; no long mode
    mov ax, str_no_long_mode
    call print_str
    hlt

.has_long_mode:
    ; check a20 line
    call check_a20
    test ax, ax
    jnz .a20_enabled

    ; no a20
    mov ax, str_no_a20
    call print_str
    hlt

.a20_enabled:
    ; switch to unreal mode
    call enable_unreal_mode

    mov ax, str_unreal
    call print_str
    hlt

; functions

%include "print.asm"
%include "cpu.asm"
%include "ext2.asm"

%define CHAR_CR 0x0D
%define CHAR_LF 0x0A
%define CRLF_STR(s) db s, CHAR_CR, CHAR_LF, 0

str_hello_world: CRLF_STR("stage2.bin says: Hello, World!")
str_no_cpuid: CRLF_STR("Error: Processor does not support CPUID")
str_no_extd_cpuid: CRLF_STR("Error: Processor does not support Extended CPUID")
str_no_long_mode: CRLF_STR("Error: Processor does not support Long Mode")
str_no_a20: CRLF_STR("Error: A20 Line is disabled")
str_unreal: CRLF_STR("Things are about to get (un)real :3")

times (64 * KiB) - ($-$$) db 0
