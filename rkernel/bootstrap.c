#include <stdint.h>
#include <stddef.h>

#define section(s) __attribute__((section(s)))
#define attribute(...) __attribute__((__VA_ARGS__))

#include "../common/boot_data.h"

extern void kernel_main(boot_data_t *bootData);

extern char kernel_base_addr;

const section(".bootstrap_data") attribute(used) uint64_t stack_offset =
        offsetof(boot_data_t, allocation_info) + offsetof(allocation_info_t, stack_base);

// sysv_abi calling convention
// arguments: RDI, RSI, RDX, RCX, R8, R9, stack
// return: RAX
// preserve: RBX, RSP, RBP, R12-R15

section(".bootstrap") attribute(naked, used) void bootstrap(attribute(unused) boot_data_t *bootData) {
    // %rdi = bootData
    __asm__ (
            // save bootData
            "push %rdi;"

            // get the intended stack address from the bootData pointer
            // %rax = bootData->allocation_info.stack_base
            "mov %rdi, %rbx;"
            "mov stack_offset, %rdi;"
            "mov (%rbx,%rdi), %rax;" // (%rbx,%rdi) => *(bootData+stack_offset)

            // restore bootData
            "pop %rdi;"

            // set the new stack and clear rbp
            // %rsp = %rax
            // %rbp = 0
            "mov %rax, %rsp;"
            "xor %rbp, %rbp;"

            // call the actual bootstrap
            // stage2_bootstrap(bootData)
            "call stage2_bootstrap;"

            // in case we return, halt
            "1: hlt;"
            "jmp 1b;"
    );
}

section(".bootstrap") attribute(used) void stage2_bootstrap(boot_data_t *bootData) {
    if (bootData->signature != BOOT_DATA_SIGNATURE) {
        return;
    }

    bootData->allocation_info.kernel_base = (uint64_t) &kernel_base_addr;

    // todo: setup pagetables properly
    kernel_main(bootData);
}
