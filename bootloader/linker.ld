OUTPUT_FORMAT("binary")
OUTPUT_ARCH("i8086")
ENTRY(main)

SECTIONS
{
    . = 0x1000;
    .text : {
        *(.text)
    }
    .data : {
        *(.rodata)
        *(.data)
        *(.bss)
        *(COMMON)
    }
}
