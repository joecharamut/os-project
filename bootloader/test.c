void term_write(const char *);
extern void test();

void main() {
    term_write("hello from c world");
    test();
}

void term_write(const char *str) {
    while (*str) {
        __asm__(
        "movb $0x0E, %%ah   \n" // teletype output
        "movb $0x00, %%bh   \n" // codepage 0
        "movb $0x00, %%bl   \n" // foreground color
        "movb %0, %%al      \n" // display char
        "int $0x10          \n"
        :
        : "r" (*str)
        : "ax", "bx");

        ++str;
    }
}
