char bss[512];

void _start() {
    const char *str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam mattis ante eu elit pretium mollis. Quisque dapibus, risus at tempor tempor, velit ligula porttitor ex, eleifend laoreet ante libero nec felis. Morbi convallis quam quis est semper molestie. Suspendisse tempus sit amet quam ut aliquam. Donec nec metus fermentum libero aliquam molestie ut nec est. Curabitur suscipit ex pretium enim imperdiet, eget aliquet diam cursus. Nullam vel diam mattis dolor tincidunt aliquet. Etiam pellentesque ante et justo cursus pretium. Maecenas et elementum felis.";
    __asm__ (
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    "nop\t\n"
    );
}
