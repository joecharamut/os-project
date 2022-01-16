#include <stdint.h>

#define section(s) __attribute__((section(s)))
#define attribute(a) __attribute__((a))

#include "../common/boot_data.h"

extern void kernel_main(boot_data_t *bootData);

extern void* kernel_base_addr;

section(".bootstrap") attribute(unused) void bootstrap(boot_data_t *bootData) {
    if (bootData->signature != BOOT_DATA_SIGNATURE) {
        return;
    }

    bootData->allocation_info.kernel_base = (uint64_t) &kernel_base_addr;

    // todo: setup pagetables properly
    kernel_main(bootData);
    __asm__ ("cli; hlt; jmp .");
}
