OUTPUT_FORMAT("binary")
OUTPUT_ARCH("i8086")
ENTRY(main)

MEMORY
{
    CS (rwx) : ORIGIN = 0x1000, LENGTH = 60K
    DS (rwx) : ORIGIN = 0x10000, LENGTH = 400K
}

SECTIONS
{
    .code : {
        *(.text)
    } > CS

    .data : {
        *(.rodata)
        *(.data)
        *(.bss)
        *(COMMON)
    } > DS
}
