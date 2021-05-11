OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH("i386")
ENTRY(_start)

SECTIONS
{
    . = 0x00100000;
    _kernel_base = .;
    _kernel_code_base = .;

    .boot : ALIGN(4K) {
        *(.boot);
    }

    .text : ALIGN(4K) {
        *(.text)
    }

    .rodata : ALIGN(4K) {
        *(.rodata)
    }

    _kernel_code_end = .;
    _kernel_data_base = .;

    .data : ALIGN(4K) {
        *(.data)
    }

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }

    _kernel_data_end = .;
    _kernel_end = .;
}