OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(_start)

SECTIONS
{
    . = 0x00100000;
    _kernel_base = .;

    .boot_data : ALIGN(4) {
        *(.boot_data);
    }

    .text : ALIGN(4K) {
        *(.text)
    }

    .data : ALIGN(4K) {
        _kernel_data = .;
        *(.data)
        *(.rodata)
    }

    .bss : ALIGN(4K) {
        _kernel_bss = .;
        *(.bss)
        *(COMMON)
    }

    _kernel_end = .;
}