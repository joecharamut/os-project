#include <stdint.h>

#define section(s) __attribute__((section(s)))
#define attribute(a) __attribute__((a))

#include "../common/boot_data.h"

extern void kernel_main();

section(".bootstrap") attribute(unused) void bootstrap(boot_data_t *bootData) {
    if (bootData->signature != BOOT_DATA_SIGNATURE) {
        return;
    }

    uint32_t *display_buffer = (uint32_t *) bootData->video_info.bufferAddress;
    for (uint32_t x = 0; x < bootData->video_info.horizontalResolution; ++x) {
        for (uint32_t y = 0; y < bootData->video_info.verticalResolution; ++y) {
            display_buffer[y * bootData->video_info.horizontalResolution + x] = 0xFFFF00FF;
        }
    }

    // todo: setup pagetables properly
    kernel_main();
    __asm__ ("cli; hlt; jmp .");
}

void *memcpy(void *dst, void *src, uint64_t n) {
    unsigned char *d = dst;
    unsigned char *s = src;

    while (n) {
        *d = *s;
        d++;
        s++;
        n--;
    }

    return dst;
}

void *memset(void *dst, uint8_t v, uint64_t n) {
    unsigned char * d = dst;

    while (n) {
        *d = v;
        d++;
        n--;
    }

    return dst;
}

int memcmp(const void *vleft, const void *vright, uint64_t n) {
    const unsigned char *l = vleft;
    const unsigned char *r = vright;

    while (n && *l == *r) {
        l++;
        r++;
        n--;
    }

    if (n) {
        return *l - *r;
    }
    return 0;
}
