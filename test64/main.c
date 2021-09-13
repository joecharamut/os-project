char bss[512];

void _start() {
    const char *str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam mattis ante eu elit pretium mollis. Quisque dapibus, risus at tempor tempor, velit ligula porttitor ex, eleifend laoreet ante libero nec felis. Morbi convallis quam quis est semper molestie. Suspendisse tempus sit amet quam ut aliquam. Donec nec metus fermentum libero aliquam molestie ut nec est. Curabitur suscipit ex pretium enim imperdiet, eget aliquet diam cursus. Nullam vel diam mattis dolor tincidunt aliquet. Etiam pellentesque ante et justo cursus pretium. Maecenas et elementum felis.";
    __asm__ (
    "mov $0xB8000, %edi\t\n"
    "mov $500, %rcx\t\n"
    "mov $0x1F201F201F201F20, %rax\t\n"
    "rep stosq\t\n"
    "mov $0xB8000, %edi\t\n"
    "mov $0x1F6C1F6C1F651F48, %rax\t\n"
    "mov %rax, (%edi)\t\n"
    "mov $0x1F6F1F571F201F6F, %rax\t\n"
    "mov %rax, 8(%edi)\t\n"
    "mov $0x1F211F641F6C1F72, %rax\t\n"
    "mov %rax, 16(%edi)\t\n"
    );

    __asm__ (
    "cli; hlt; jmp ."
    );
}
