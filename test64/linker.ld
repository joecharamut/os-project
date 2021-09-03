OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)

SECTIONS
{
    .code : {
        *(.text)
    }

    .data : {
        *(.rodata)
        *(.data)
        *(.bss)
        *(COMMON)
    }
}
