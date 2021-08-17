void term_write(const char *);
__attribute__((noreturn)) void abort();

extern unsigned int supports_cpuid();
extern unsigned int supports_long_mode();

__attribute__((noreturn, used)) void main() {
    term_write("hello stage2 c world!\r\n");

    if (!supports_cpuid()) {
        term_write("Boot Failure: Processor does not support CPUID");
        abort();
    }

    if (!supports_long_mode()) {
        term_write("Boot Failure: Processor does not support Long Mode");
        abort();
    }

    term_write("good?");

    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}

__attribute__((noreturn)) void abort() {
    __asm__ volatile ("cli; hlt; jmp .");
    __builtin_unreachable();
}

void term_write(const char *str) {
    while (*str) {
        __asm__ volatile (
        "movb $0x0E, %%ah   \n" // teletype output
        "movb $0x00, %%bh   \n" // codepage 0
        "movb $0x00, %%bl   \n" // foreground color 0
        "movb %0, %%al      \n" // char to write
        "int $0x10          \n"
        :
        : "r" (*str)
        : "ax", "bx");

        ++str;
    }
}
