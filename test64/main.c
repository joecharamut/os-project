#include <stdint.h>

#define DISPLAY_WIDTH 80
#define DISPLAY_HEIGHT 25
#define DISPLAY_ENTRY(chr, color) ((uint16_t) (chr) | ((uint16_t) (color) << 8))
#define DISPLAY_COLOR 0x17

void _start() {
    const char *str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam mattis ante eu elit pretium mollis. Quisque dapibus, risus at tempor tempor, velit ligula porttitor ex, eleifend laoreet ante libero nec felis. Morbi convallis quam quis est semper molestie. Suspendisse tempus sit amet quam ut aliquam. Donec nec metus fermentum libero aliquam molestie ut nec est. Curabitur suscipit ex pretium enim imperdiet, eget aliquet diam cursus. Nullam vel diam mattis dolor tincidunt aliquet. Etiam pellentesque ante et justo cursus pretium. Maecenas et elementum felis.";

    uint16_t *display_buffer = (uint16_t *) 0xB8000;
    for (int x = 0; x < DISPLAY_WIDTH; ++x) {
        for (int y = 0; y < DISPLAY_HEIGHT; ++y) {
            display_buffer[y * DISPLAY_WIDTH + x] = DISPLAY_ENTRY(' ', DISPLAY_COLOR);
        }
    }

    int x = 0;
    int y = 0;
    for (int i = 0; i < 512; ++i) {
        display_buffer[y * DISPLAY_WIDTH + x] = DISPLAY_ENTRY(str[i], DISPLAY_COLOR);

        ++x;
        if (x > DISPLAY_WIDTH) {
            x = 0;
            ++y;
        }
    }
}
