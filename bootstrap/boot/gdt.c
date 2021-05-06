#include <bootstrap/debug/serial.h>
#include <bootstrap/debug/debug.h>
#include <stdbool.h>
#include "gdt.h"

uint64_t encodeGdtEntry(uint32_t base, uint32_t limit, uint8_t type) {
    uint64_t descriptor = 0;
    uint8_t *target = &descriptor;

    // Check the limit to make sure that it can be encoded
    if ((limit > 65536) && ((limit & 0xFFF) != 0xFFF)) {
        dbg_logf(LOG_ERROR, "GDT Error\n");
    }

    if (limit > 65536) {
        // Adjust granularity if required
        limit = limit >> 12;
        target[6] = 0xC0;
    } else {
        target[6] = 0x40;
    }

    // Encode the limit
    target[0] = limit & 0xFF;
    target[1] = (limit >> 8) & 0xFF;
    target[6] |= (limit >> 16) & 0xF;

    // Encode the base
    target[2] = base & 0xFF;
    target[3] = (base >> 8) & 0xFF;
    target[4] = (base >> 16) & 0xFF;
    target[7] = (base >> 24) & 0xFF;

    // And... Type
    target[5] = type;

    return descriptor;
}

#define GDT_SIZE 0xffff
static uint64_t GDT[GDT_SIZE];
static uint16_t GDT_Entries = 0;

extern void gdt_load(uint64_t *gdt, uint16_t size);
void gdt_init(void) {
    GDT[GDT_Entries++] = encodeGdtEntry(0, 0, 0);
    GDT[GDT_Entries++] = encodeGdtEntry(0, 0xffffffff, 0x9A);
    GDT[GDT_Entries++] = encodeGdtEntry(0, 0xffffffff, 0x92);
    gdt_load(GDT, GDT_Entries * 8);
}

uint32_t gdt_add_entry(uint32_t base, uint32_t limit, uint8_t flag) {
    uint64_t value = encodeGdtEntry(base, limit, flag);
    dbg_logf(LOG_DEBUG, "Adding GDT Entry for {0x%x - 0x%x} type 0x%x = 0x%llx\n", base, limit, flag, value);

    uint32_t entry = GDT_Entries++;
    GDT[entry] = value;
    asm volatile ("cli\n");
    gdt_load(GDT, GDT_Entries * 8);
    asm volatile ("sti\n");
    return entry << 3;
}
