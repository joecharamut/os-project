#include <bootstrap/debug/debug.h>
#include "paging.h"

extern u32 _kernel_base;
extern u32 _kernel_end;
extern u32 _page_table_base;
extern u32 _page_table_end;

static page_directory_entry_t _page_directory[1024] __attribute__ ((aligned(4096))) __attribute__ ((section(".page_tables")));
static page_table_entry_t _page_tables[1024 * 1024] __attribute__ ((aligned(4096))) __attribute__ ((section(".page_tables")));

static page_directory_entry_t *page_directory;
static page_table_entry_t *page_tables;

void _boot_paging_init(void) __attribute__ ((section(".boot_text")));
void _boot_paging_init(void) {
//    page_directory_entry_t *boot_page_directory = (void *) ((u32) &_page_directory - (u32) &_kernel_base);
    page_directory_entry_t *boot_page_directory = (void *) &_page_directory;
//    page_table_entry_t *boot_page_tables = (void *) ((u32) &_page_tables - (u32) &_kernel_base);
    page_table_entry_t *boot_page_tables = (void *) &_page_tables;
//    u32 kernel_size = (u32) &_kernel_end - (u32) &_kernel_base;
//    u32 kernel_start_page = (u32) &_kernel_base >> 22;
//    u32 kernel_pages = (kernel_size / 0x400000) + 1;

    asm volatile (".b: jmp .b\n" :: "a" ( kernel_size ), "b" ( 'B' ));

    for (u32 pg = 0; pg < kernel_pages; pg++) {
        for (int i = 0; i < 1024; i++) {
            boot_page_tables[((pg + kernel_start_page) * 1024) + i] = (page_table_entry_t) {
                    .fields.address = ((pg * 1024 + i) * 4096) >> 12,
                    .fields.present = true,
                    .fields.rw      = true,
            };
        }

        boot_page_directory[pg].fields.address = (u32) &boot_page_tables[((pg + kernel_start_page) * 1024)] >> 12;
        boot_page_directory[pg].fields.present = true;
        boot_page_directory[pg].fields.rw = true;
        boot_page_directory[pg + 768].fields.address = (u32) &boot_page_tables[((pg + kernel_start_page) * 1024)] >> 12;
        boot_page_directory[pg + 768].fields.present = true;
        boot_page_directory[pg + 768].fields.rw = true;
    }

    // map kernel
    u32 kernel_size = (u32) &_kernel_end - (u32) &_kernel_base;
    u32 kernel_start_page = (u32) &_kernel_base >> 22;
    u32 kernel_pages = (kernel_size / 0x400000) + 1;

    for (int i = 0; ) {}


    // 1mb (misc io) + 5mb (page tables) ident map
    for (int i = 0; i < 256*6; ++i) {
        _page_tables[i] = (page_table_entry_t) {
                .fields.address = i,
                .fields.supervisor = true,
                .fields.present = true,
                .fields.rw = true,
        };

        if (i % 1024 == 0) {
            _page_directory[i / 1024] = (page_directory_entry_t) {
                    .fields.address = (u32) &_page_tables[i] >> 12,
                    .fields.supervisor = true,
                    .fields.present = true,
                    .fields.rw = true,
            };
        }
    }

    asm volatile (
    "\n mov %0, %%cr3"
    "\n mov %%cr0, %%eax"
    "\n or %1, %%eax"
    "\n mov %%eax, %%cr0"
    :
    : "r" (boot_page_directory), "r" ((1 << 31) | (1 << 16))
    : "eax"
    );

//    asm volatile (".a: jmp .a\n");
}

void flush_tlb() {
    asm volatile (
    "\n mov %%cr3, %%eax"
    "\n mov %%eax, %%cr3"
    : // no output
    : // no input
    : "eax"
    );
    dbg_logf(LOG_DEBUG, "Flushed the toilet\n");
}

void flush_page(virt_addr_t address) {
    asm volatile (
    "invlpg (%0)"
    : // no output
    : "r" (address)
    : "memory"
    );
}

void paging_main_init(void) {
    page_directory = (void *)&_page_directory;
    page_tables = (void *) &_page_tables;

    u32 kernel_size = (u32) &_kernel_end - (u32) &_kernel_base;
    u32 kernel_pages = (kernel_size / 0x400000) + 1;
    for (u32 i = 1; i < kernel_pages; i++) {
        for (int j = 0; j < 1024; j++) {
            page_tables[(i * 1024) + j] = (page_table_entry_t) { .value=0 };
        }
        page_directory[i] = (page_directory_entry_t) { .value=0 };
    }

    for (int k = 0; k < 256; k++) {
        page_tables[k] = (page_table_entry_t) {
                .fields.address = (k * 4096) >> 12,
                .fields.present = true,
                .fields.rw      = true,
        };
    }
    page_directory[0] = (page_directory_entry_t) {
            .fields.address = (u32) &page_tables[0] >> 12,
            .fields.present = true,
            .fields.rw = true,
    };


    flush_tlb();
}

void map_page(phys_addr_t phys_addr, virt_addr_t vaddr, bool kernel, bool writeable) {
    vaddr &= ~(0xfff);
    dbg_logf(LOG_DEBUG, "Mapping paddr 0x%x to vaddr 0x%x\n", phys_addr, vaddr);

    u32 directoryIndex = vaddr / (4 * 1024 * 1024);
    u32 pageIndex = (vaddr / (4 * 1024));
    bool dirty = false;

    dbg_logf(LOG_DEBUG, "Directory entry; %d, Page: %d (%% 1024: %d)\n", directoryIndex, pageIndex, pageIndex % 1024);


    // if the table isnt mapped, map it
    if (!page_tables[pageIndex].fields.present) {
        page_tables[pageIndex] = (page_table_entry_t) {
                .fields.address = phys_addr >> 12,
                .fields.supervisor = kernel,
                .fields.present = true,
                .fields.rw = writeable,
        };

        dirty = true;
    }

    // if the directory does not have this table mapped yet, map it
    if (!page_directory[directoryIndex].fields.present) {
        page_directory[directoryIndex] = (page_directory_entry_t) {
                .fields.address = (u32) (page_tables + (directoryIndex * 1024)) >> 12,
                .fields.supervisor = kernel,
                .fields.present = true,
                .fields.rw = writeable,
        };

        dirty = true;
    }

    if (dirty) {
        flush_tlb();
    }

    asm volatile ("a: jmp a\n" :: "a" ( &_page_directory ));
}

void unmap_page(virt_addr_t vaddr) {
    vaddr &= ~(0xfff);
    dbg_logf(LOG_DEBUG, "Unmapping page for vaddr 0x%x\n", vaddr);

    u32 directoryIndex = vaddr / (4 * 1024 * 1024);
    u32 pageIndex = (vaddr / (4 * 1024));
    bool dirty = false;

    dbg_logf(LOG_DEBUG, "Directory entry; %d, Page: %d (%% 1024: %d)\n", directoryIndex, pageIndex, pageIndex % 1024);


    // if the table isnt mapped, map it
    if (page_tables[pageIndex].fields.present) {
        page_tables[pageIndex] = (page_table_entry_t) { .value = 0 };

        dirty = true;
    }

    if (dirty) {
        flush_tlb();
    }
}
