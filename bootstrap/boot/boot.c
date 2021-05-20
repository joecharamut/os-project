#include <debug/serial.h>
#include <debug/term.h>
#include <debug/debug.h>
#include <mm/paging.h>
#include <kernel/kernel.h>
#include <std/attributes.h>
#include <mm/kmem.h>
#include <io/ps2.h>
#include <std/string.h>
#include "interrupts.h"
#include "multiboot.h"
#include "gdt.h"

void irq0_handler(registers_t regs) { }

void int3_handler(registers_t regs) {
    term_setcolor(VGA_COLOR(VGA_COLOR_WHITE, VGA_COLOR_LIGHT_RED));
    dbg_printf("BREAKPOINT HIT: \"%s\"\n", regs.edx);
    dump_registers(&regs);
}

#define BOOT_MEMSET(ptr, value, count) \
do { \
    for (u32 i = 0; i < count; i++) { \
        ((u8 *) ptr)[i] = value; \
    } \
} while (0)

#define PAGE_CEIL(addr) ((addr) & 0x00000FFF ? (((addr) & 0xFFFFF000) + 0x1000) : (addr))
#define PAGE_FLOOR(addr) ((addr) & 0x00000FFF ? (((addr) & 0xFFFFF000)) : (addr))

static void SECTION(".boot_text") _early_page_map(page_directory_t *page_dir, u32 physical_addr, u32 virtual_addr, bool writeable) {
    int dir_index = virtual_addr >> 22;
    int table_index = (virtual_addr >> 12) & 0x3FF;

    // allocate if not there
    if (!page_dir->tables[dir_index].present) {
        page_table_t *table = (void *) (PAGE_TABLE_BASE + sizeof(page_directory_t) + (dir_index * sizeof(page_table_t)));
        BOOT_MEMSET(table, 0, sizeof(page_table_t));

        page_dir->tables[dir_index].address = (u32) table >> 12;
        page_dir->tables[dir_index].present = true;
        page_dir->tables[dir_index].rw = true;
    }

    page_table_t *table = (page_table_t *) ((u32) page_dir->tables[dir_index].address << 12);
    table->pages[table_index].address = physical_addr >> 12;
    table->pages[table_index].present = true;
    table->pages[table_index].rw = writeable;
}

void USED NORETURN SECTION(".boot_text") _early_boot(u32 multiboot_magic, multiboot_info_t *multiboot_info) {
    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        asm volatile ("hlt");
    }

    u32 page_table_base = PAGE_TABLE_BASE;

    page_directory_t *kernel_page_dir = (page_directory_t *) page_table_base;
    BOOT_MEMSET(kernel_page_dir, 0, sizeof(page_directory_t));
    page_table_base += sizeof(page_directory_t);

    asm volatile (
        "cld              \n" // clear direction flag
        "xor %%eax, %%eax \n" // set eax to 0
        "rep stosl        \n" // store long words
        :
        : "D" (page_table_base), "c" (sizeof(page_table_t) * 1024 / 4)
        : "eax", "memory", "cc"
    );
    page_table_base += sizeof(page_table_t) * 1024;

    // set last page directory entry to the page directory itself
    // this lets the kernel paging code change the entries while paging is enabled
    kernel_page_dir->tables[1023].address = (u32) kernel_page_dir >> 12;
    kernel_page_dir->tables[1023].present = true;
    kernel_page_dir->tables[1023].rw = true;

    extern u32 _boot_base;
    extern u32 _boot_end;
    extern u32 _kernel_code_base;
    extern u32 _kernel_code_end;
    extern u32 _kernel_data_base;
    extern u32 _kernel_data_end;

    u32 boot_base = (u32) &_boot_base;
    u32 boot_end = (u32) &_boot_end;
    u32 kernel_code_start = (u32) &_kernel_code_base;
    u32 kernel_code_end = (u32) &_kernel_code_end;
    u32 kernel_data_start = (u32) &_kernel_data_base;
    u32 kernel_data_end = (u32) &_kernel_data_end;

    // identity map first mb (for display and stuff)
    for (int i = 0; i < 0x100000; i += 0x1000) {
        _early_page_map(kernel_page_dir, i, i, true);
    }

    // identity map the bootcode
    for (u32 i = PAGE_FLOOR(boot_base); i < PAGE_CEIL(boot_end); i += 0x1000) {
        _early_page_map(kernel_page_dir, i, i, true);
    }

    // map .text and .rodata as read-only
    for (u32 i = PAGE_FLOOR(kernel_code_start); i < PAGE_CEIL(kernel_code_end); i += 0x1000) {
        _early_page_map(kernel_page_dir, i - KERNEL_BASE_ADDR, i, false);
    }

    // map .data and .bss as read-write
    for (u32 i = PAGE_FLOOR(kernel_data_start); i < PAGE_CEIL(kernel_data_end); i += 0x1000) {
        _early_page_map(kernel_page_dir, i - KERNEL_BASE_ADDR, i, true);
    }

    // load cr3 with the page table
    asm volatile ("mov %0, %%cr3" :: "r" (kernel_page_dir));

    // enable paging bit in cr0
    volatile u32 cr0;
    asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 1 << 31;
    asm volatile ("mov %0, %%cr0" :: "r" (cr0));

    // jmp to higher half code
    asm volatile (
    "push %0          \n"
    "lea _boot, %%eax \n"
    "call *%%eax      \n"
    :
    : "r" (multiboot_info)
    : "%eax"
    );

    __builtin_unreachable();
}

static void move_kernel_stack() {
    extern u32 boot_stack_top;
    extern u32 boot_stack_bottom;

    u32 old_stack_size = (u32) &boot_stack_bottom - (u32) &boot_stack_top;
    u32 new_stack_size = 0x1000;

    void *new_stack = kmalloc(new_stack_size);
    void *old_stack = &boot_stack_top;

    void *current_esp;
    void *current_ebp;
    asm volatile ("mov %%esp, %0" : "=r" (current_esp));
    asm volatile ("mov %%ebp, %0" : "=r" (current_ebp));

    u32 esp_offset = (u32) current_esp - (u32) old_stack;
    u32 ebp_offset = (u32) current_ebp - (u32) old_stack;

    memcpy(new_stack, old_stack, old_stack_size);

    asm volatile ("mov %0, %%esp" :: "r" ((u32) new_stack + esp_offset) : "memory");
    asm volatile ("mov %0, %%ebp" :: "r" ((u32) new_stack + ebp_offset) : "memory");
}

static void unmap_bootcode() {
    extern u32 _boot_base;
    extern u32 _boot_end;
    u32 boot_base = (u32) &_boot_base;
    u32 boot_end = (u32) &_boot_end;
    for (u32 i = PAGE_FLOOR(boot_base); i < PAGE_CEIL(boot_end); i += 0x1000) {
        munmap((void *) i);
    }
}

static void USED NORETURN _boot(multiboot_info_t *multiboot_info) {
    serial_init();
    term_init();

    dbg_logf(LOG_DEBUG, "bootloader: %s\n", multiboot_info->boot_loader_name, multiboot_info->cmdline);

    u32 eip;
    asm volatile ("mov $., %0" : "=r" (eip));
    dbg_logf(LOG_INFO, "Hello higher-half, we're at 0x%08x now!\n", eip);

    init_gdt();
    init_interrupts();
    set_interrupt_handler(IRQ0, irq0_handler);
    set_interrupt_handler(3, int3_handler);
    set_interrupt_handler(14, page_fault_handler);
    init_kmem();
    init_ps2();

    move_kernel_stack();
    unmap_bootcode();

    kernel_main();

    dbg_printf("System Halted.");
    asm volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}
