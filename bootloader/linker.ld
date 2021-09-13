OUTPUT_FORMAT("binary")
OUTPUT_ARCH("i8086")
ENTRY(_start)

MEMORY
{
    /* Interrupt Vector Table [0x0000 - 0x03FF] */
    IVT (rw) : ORIGIN = 0x0000, LENGTH = 1024

    /* BIOS Data Area [0x0400 - 0x04FF] */
    BDA (r) : ORIGIN = 0x0400, LENGTH = 256

    /* MBR Boot Code [0x0500 - 0x06FF] */
    MBR (rx) : ORIGIN = 0x0500, LENGTH = 512

    /* Code Segment [0x0700 - 0xEFFF] */
    CS (rx) : ORIGIN = 0x0700, LENGTH = 61K + 768

    /* Variables Segment [0xF000 - 0xFFFF] */
    VAR (rw) : ORIGIN = 0xF000, LENGTH = 1K

    /* Page Map Segment [0x10000 - 0x1FFFF] */
    PM (rw) : ORIGIN = 0x10000, LENGTH = 64K

    /* Data Segment [0x20000 - 0x5FFFF] */
    DS (rw) : ORIGIN = 0x20000, LENGTH = 256K

    /* Stack Segment [0x60000 - 0x6FFFF] */
    SS (rw) : ORIGIN = 0x60000, LENGTH = 64K

    /* Scratch Space [0x70000 - 0x7FFFF] */
    SCR (rw) : ORIGIN = 0x70000, LENGTH = 64K
}

SECTIONS
{
    .code : {
        *(.start_text)
        *(.text)
        . = ALIGN(4);
        *(.text64)
    } > CS

    .data : {
        *(.data)
        *(.bss)
        *(COMMON)
    } > VAR

    .pm : {
        _page_map_base = .;
    } > PM

    .rodata : {
        *(.rodata)
    } > DS

    .scratch : {
        _boot_disk_ptr = .;
    } > SCR
}
