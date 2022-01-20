#ifndef LOADER_VIDEO_H
#define LOADER_VIDEO_H

#include <stdbool.h>
#include "efi.h"
#include "../common/boot_data.h"

extern EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;

EFI_STATUS video_init();

UINT32 make_color(UINT8 r, UINT8 g, UINT8 b);
void set_font(void *font_buf);
void write_char(char c);
void clear_screen();
void set_background_color(UINT32 color);
void set_foreground_color(UINT32 color);
void copy_video_info(boot_data_t *bootData);

#endif //LOADER_VIDEO_H
