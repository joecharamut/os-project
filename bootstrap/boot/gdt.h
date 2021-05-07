#ifndef OS_GDT_H
#define OS_GDT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <std/types.h>

typedef struct {
    u16 limit_low;
    u16 base_low;
    u8 base_mid;
    struct {
        bool accessed: 1;
        bool rw: 1;
        bool direction: 1;
        bool executable: 1;
        bool data: 1;
        u8 privilege: 2;
        bool present: 1;
    } __attribute__ ((packed)) access;
    struct {
        u8 limit_high: 4;
        u8 _padding: 2;
        bool is32bit: 1;
        bool pageGranularity: 1;
    } __attribute__ ((packed)) granularity;
    u8 base_high;
} __attribute__ ((packed)) gdt_entry_t;

typedef struct {
    u16 limit;
    u32 base;
} __attribute__ ((packed)) gdt_ptr_t;

void init_gdt();

#endif //OS_GDT_H
