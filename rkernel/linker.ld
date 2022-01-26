OUTPUT_FORMAT("elf64-x86-64")
ENTRY(bootstrap)

KERNEL_VMA = 0xFFFFFFF800000000;
KERNEL_LMA = 0x200000;

SECTIONS
{
    . = KERNEL_LMA;

    init_base_addr = .;

    .bootstrap : ALIGN(4K) {
        *(.bootstrap)
        *(.bootstrap_data)
    }

    init_end_addr = .;

    . += KERNEL_VMA;

    _kernel_base_addr = .;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA) {
        kernel_code_base = .;
        *(.text .text.*)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA) {
        *(.rodata .rodata.*)
        kernel_code_end = .;
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA) {
        kernel_data_base = .;
        *(.data .data.*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VMA) {
        *(.bss)
        *(COMMON)
        kernel_data_end = .;
    }

    kernel_end_addr = .;
}
