#include <stddef.h>
#include <std/types.h>
#include <stdbool.h>
#include <debug/debug.h>
#include <std/string.h>
#include <mm/paging.h>
#include "acpi.h"

typedef struct {
    char signature[4];
    u32 length;
    u8 revision;
    u8 checksum;
    char oem_id[6];
    char oem_table_id[8];
    u32 oem_revision;
    u32 creator_id;
    u32 creator_revision;
} __attribute__((packed)) sdt_header_t;

typedef struct {
    sdt_header_t header;
    sdt_header_t *other_headers[];
} rsdt_t;

typedef struct {
    char signature[8];
    u8 checksum;
    char oem_id[6];
    u8 revision;
    u32 rsdt_address;
} __attribute__((packed)) rsdp_t;

static rsdp_t *find_rsdp() {
    const char *signature = "RSD PTR ";

    // search main bios area
    for (u32 i = 0x000E0000; i < 0x000FFFFF; i += 16) {
        if (strncmp((char *) i, signature, 8) == 0) {
            return (void *) i;
        }
    }

    return NULL;
}

void init_acpi() {
    rsdp_t *desc = find_rsdp();
    if (!desc) {
        PANIC("ACPI RSDP Not Found!");
    }

    u8 sum = 0;
    for (u32 i = 0; i < sizeof(rsdp_t); i++) {
        sum += ((u8 *) desc)[i];
    }
    if (sum != 0) {
        PANIC("RSDP Checksum Invalid!");
    }

    dbg_logf(LOG_DEBUG, "RSDT address: 0x%08x\n", desc->rsdt_address);
    void *rsdt_page = (void *) (desc->rsdt_address & 0xFFFFF000);
    dbg_logf(LOG_DEBUG, "mapping page 0x%08x\n", rsdt_page);
    mmap(rsdt_page, rsdt_page, true, false);

    rsdt_t *table = (rsdt_t *) desc->rsdt_address;
    u32 entries = (table->header.length - sizeof(sdt_header_t)) / 4;
    for (u32 i = 0; i < entries; i++) {
        dbg_logf(LOG_DEBUG, "RSDT Entry: %c%c%c%c\n",
                 table->other_headers[i]->signature[0],
                 table->other_headers[i]->signature[1],
                 table->other_headers[i]->signature[2],
                 table->other_headers[i]->signature[3]);
    }
}
