#ifndef LOADER_VIDEO_H
#define LOADER_VIDEO_H

#include <stdbool.h>
#include "efi.h"

EFI_STATUS video_init();

UINT32 make_color(UINT8 r, UINT8 g, UINT8 b);
void set_font(void *font_buf);
void write_string(const char *str);
void write_char(char c);
void clear_screen();
void set_background_color(UINT32 color);
void set_foreground_color(UINT32 color);
void *get_framebuffer();

void plot_pixel(UINT32 x, UINT32 y, UINT32 color);

#endif //LOADER_VIDEO_H
