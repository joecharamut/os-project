#ifndef LOADER_VIDEO_H
#define LOADER_VIDEO_H

#include "efi.h"

EFI_STATUS video_init();

UINT32 make_color(UINT8 r, UINT8 g, UINT8 b);
void plot_pixel(UINT32 x, UINT32 y, UINT32 color);

#endif //LOADER_VIDEO_H
