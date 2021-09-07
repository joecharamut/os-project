OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)

PHDRS
{
    phony PT_NULL;
    main PT_LOAD;
}

SECTIONS
{
    . = 0xC0000000;

    .phony : {
        . += 0x100000;
    } : phony

    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000) {
        *(.text)
    } : main

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000) {
        *(.rodata)
    } : main

    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000) {
        *(.data)
    } : main

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000) {
        *(.bss)
        *(COMMON)
    } : main
}
