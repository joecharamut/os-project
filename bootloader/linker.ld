OUTPUT_FORMAT("binary")
OUTPUT_ARCH("i8086")
ENTRY(_start)

MEMORY
{
    /* Interrupt Vector Table [0x0000 - 0x03FF] */
    IVT (rw) : ORIGIN = 0x0000, LENGTH = 1024

    /* BIOS Data Area [0x0400 - 0x04FF] */
    BDA (r)  : ORIGIN = 0x0400, LENGTH = 256

    /* MBR Boot Code [0x0500 - 0x06FF] */
    MBR (rx) : ORIGIN = 0x0500, LENGTH = 512

    /* Code Segment [0x0700 - 0xFFFF] */
    CS (rwx) : ORIGIN = 0x0700, LENGTH = 63744

    /* Data Segment [0x10000 - 0x5FFFF] */
    DS (rwx) : ORIGIN = 0x10000, LENGTH = 327680

    /* Stack Segment [0x60000 - 0x6FFFF] */
    SS (rw)  : ORIGIN = 0x60000, LENGTH = 65536

    /* Interrupt Copy Buffers [0x70000 - 0x7FFFF] */
    ICB (rw) : ORIGIN = 0x70000, LENGTH = 65536
}

SECTIONS
{
    .code : {
        *(.start_text)
        *(.text)
    } > CS

    .data : {
        *(.rodata)
        *(.data)
        *(.bss)
        *(COMMON)
    } > DS
}
