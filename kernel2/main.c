#include <stdint.h>

#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 600

#define section(s) __attribute__((section(s)))
#define attribute(a) __attribute__((a))

#include "../common/boot_data.h"

void kernel_main();

section(".bootstrap_data") const char *str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam mattis ante eu elit pretium mollis. Quisque dapibus, risus at tempor tempor, velit ligula porttitor ex, eleifend laoreet ante libero nec felis. Morbi convallis quam quis est semper molestie. Suspendisse tempus sit amet quam ut aliquam. Donec nec metus fermentum libero aliquam molestie ut nec est. Curabitur suscipit ex pretium enim imperdiet, eget aliquet diam cursus. Nullam vel diam mattis dolor tincidunt aliquet. Etiam pellentesque ante et justo cursus pretium. Maecenas et elementum felis.";
section(".bootstrap") attribute(unused) void bootstrap(boot_data_t *bootData) {
    // todo: setup pagetables

    if (bootData->signature != BOOT_DATA_SIGNATURE) {
        return;
    }

    uint32_t *display_buffer = (uint32_t *) bootData->video_info.bufferAddress;
    for (int x = 0; x < DISPLAY_WIDTH; ++x) {
        for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
            display_buffer[y * DISPLAY_WIDTH + x] = 0xFFFF00FF;
        }
    }

//    int x = 0;
//    int y = 0;
//    for (int i = 0; i < 512; ++i) {
//        display_buffer[y * DISPLAY_WIDTH + x] = DISPLAY_ENTRY(str[i], DISPLAY_COLOR);
//
//        ++x;
//        if (x > DISPLAY_WIDTH) {
//            x = 0;
//            ++y;
//        }
//    }

    __asm__ ("cli; hlt; jmp .");
}

void kernel_main() {

}
