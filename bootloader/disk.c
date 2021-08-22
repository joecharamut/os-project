#include "disk.h"

uint32_t disk_read_sectors(uint8_t disk, uint8_t *buffer, uint32_t sector, uint32_t count) {
    struct {
        uint8_t size;
        uint8_t _padding;
        uint16_t count;
        uint16_t buffer_offset;
        uint16_t buffer_segment;
//        uint32_t buffer;
        uint32_t sector_lo;
        uint32_t sector_hi;
    } __attribute__((packed)) disk_packet = {
            .size = 0x10,
            ._padding = 0,
            .count = 1,
            .buffer_offset = ((uint32_t) buffer) & 0xFFFF,
            .buffer_segment = (((uint32_t) buffer) >> 16) << 8,
//            .buffer = (uint32_t) buffer,
            .sector_lo = 1,
            .sector_hi = 0,
    };
    static_assert(sizeof(disk_packet) == 0x10, "Invalid Size");
    uint16_t ret;
    __asm__ volatile (
            "movb $0x42, %%ah\t\n"
            "movb %1, %%dl\t\n"
            "mov %2, %%esi\t\n"
            "int $0x13\t\n"
            "movw %%ax, %0\t\n"
            : "=r" (ret)
            : "r" (disk), "r" (&disk_packet)
            : "esi", "eax", "edx", "memory", "cc"
    );
    return ret;
}
