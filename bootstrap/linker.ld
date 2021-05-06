OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

    .boot_magic : ALIGN(4) {
        *(.boot_magic);
    }

    .boot_text : ALIGN(4K) {
        *(.boot_text)
    }

    .boot_stack : ALIGN(4K) {
        *(.boot_stack)
    }

    _page_table_base = .;

    .page_tables : ALIGN(4K) {
        *(.page_tables)
    }

    _page_table_end = .;

    . += 0xC0000000;
    _kernel_base = .;

    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
    {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;
}