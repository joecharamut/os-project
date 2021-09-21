OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)

PHDRS
{
    load PT_LOAD;
}

KERNEL_VMA = 0xFFFFFFF800000000;
SECTIONS
{
    . = 1M;
    _page_table = .;
    . += KERNEL_VMA + 1M;

    .text : AT(ADDR(.text) - KERNEL_VMA) {
        _code = .;
        *(.text)
        *(.rodata*)
        . = ALIGN(4K);
    } :load

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA) {
        _data = .;
        *(.data)
    } :load

    .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA)
    {
        _eh_frame = .;
        *(.eh_frame)
         . = ALIGN(4096);
    } :load

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA) {
        _bss = .;
        *(.bss)
        *(COMMON)
        . = ALIGN(4096);
    } :load

    _end = .;
}
