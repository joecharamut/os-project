#include <debug/serial.h>
#include <debug/debug.h>
#include <stdbool.h>
#include "gdt.h"

extern void load_gdt_ptr(u32);
static void set_gdt_entry(int index, u32 base, u32 limit, u8 privilege, bool data, bool executable, bool rw, bool kbGranularity);

#define GDT_SIZE 5
gdt_entry_t gdt_entries[GDT_SIZE];
gdt_ptr_t gdt_ptr;

void init_gdt() {
    gdt_ptr.limit = sizeof(gdt_entry_t) * GDT_SIZE - 1;
    gdt_ptr.base = (u32) &gdt_entries;

    gdt_entries[0] = (gdt_entry_t) { 0 }; // null segment
    set_gdt_entry(1, 0, 0xFFFFFFFF, 0, true, true, true, true); // kernel code
    set_gdt_entry(2, 0, 0xFFFFFFFF, 0, true, false, true, true); // kernel data
    set_gdt_entry(3, 0, 0xFFFFFFFF, 3, true, true, true, true); // user code
    set_gdt_entry(4, 0, 0xFFFFFFFF, 3, true, false, true, true); // user data

    load_gdt_ptr((u32) &gdt_ptr);
}

static void set_gdt_entry(int index, u32 base, u32 limit, u8 privilege, bool data, bool executable, bool rw, bool kbGranularity) {
    gdt_entries[index] = (gdt_entry_t) {
        .base_low = (base & 0xFFFF),
        .base_mid = (base >> 16) & 0xFF,
        .base_high = (base >> 24) & 0xFF,

        .limit_low = (limit & 0xFFFF),
        .granularity.limit_high = (limit >> 16) & 0x0F,

        .granularity.is32bit = true,
        .granularity.pageGranularity = kbGranularity,

        .access.present = true,
        .access.data = data,
        .access.direction = false,
        .access.executable = executable,
        .access.rw = rw,
        .access.privilege = privilege,
    };
}
