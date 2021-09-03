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
