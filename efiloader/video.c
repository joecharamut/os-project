#include "video.h"
#include <efilib.h>

#define _STDINT_H
#define SSFN_CONSOLEBITMAP_TRUECOLOR
#include <ssfn.h>

EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP = NULL;

static UINT32 background_color;
static UINT32 foreground_color;

EFI_STATUS video_init() {
    if (GOP) return EFI_SUCCESS;
    EFI_STATUS status;

    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = BS->LocateProtocol(&gopGuid, NULL, (void **) &GOP);
    if (EFI_ERROR(status)) {
        return status;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *modeInfo;
    UINTN infoSize;
    UINTN numModes;
    UINTN nativeMode;

    UINT32 currentMode = 0;
    if (GOP->Mode) {
        currentMode = GOP->Mode->Mode;
    }

    status = GOP->QueryMode(GOP, currentMode, &infoSize, &modeInfo);

    if (status == EFI_NOT_STARTED) {
        GOP->SetMode(GOP, 0);
    }

    if (EFI_ERROR(status)) {
        Print(L"Unable to get native graphics mode\n");
        return status;
    } else {
        nativeMode = GOP->Mode->Mode;
        numModes = GOP->Mode->MaxMode;
    }

    for (UINTN i = 0; i < numModes; ++i) {
        status = GOP->QueryMode(GOP, i, &infoSize, &modeInfo);
        Print(L"Mode %03d {Width %d, Height %d, Format %x}%s\n",
              i,
              modeInfo->HorizontalResolution,
              modeInfo->VerticalResolution,
              modeInfo->PixelFormat,
              i == nativeMode ? L" (current)" : L""
              );
    }

    ssfn_dst.ptr = (void *) GOP->Mode->FrameBufferBase;
    ssfn_dst.w = (INT16) GOP->Mode->Info->HorizontalResolution;
    ssfn_dst.h = (INT16) GOP->Mode->Info->VerticalResolution;
    ssfn_dst.p = 4 * GOP->Mode->Info->PixelsPerScanLine;
    ssfn_dst.x = ssfn_dst.y = 0;
    ssfn_dst.fg = 0xFFFFFF;

    background_color = make_color(0x00, 0x00, 0x00);
    foreground_color = make_color(0xff, 0xff, 0xff);

    return EFI_SUCCESS;
}

void set_font(void *font_buf) {
    ssfn_src = font_buf;
}

void write_string(const char *str) {
    while (*str) {
        ssfn_putc(*str++);
    }
}

void write_char(char c) {
    ssfn_putc(c);
}

void clear_screen() {
    UINT32 *framebuffer = (UINT32 *) GOP->Mode->FrameBufferBase;
    UINT32 pixels = GOP->Mode->Info->PixelsPerScanLine * GOP->Mode->Info->VerticalResolution;
    for (UINT32 i = 0; i < pixels; ++i) {
        framebuffer[i] = background_color;
    }
//    for (UINT32 y = 0; y < GOP->Mode->Info->VerticalResolution; ++y) {
//        for (UINT32 x = 0; x < GOP->Mode->Info->PixelsPerScanLine; ++x) {
//            *(framebuffer + (y * GOP->Mode->Info->PixelsPerScanLine) + x) = background_color;
//        }
//    }
}

void set_background_color(UINT32 color) {
    background_color = color;
}

void set_foreground_color(UINT32 color) {
    foreground_color = color;
}

UINT32 make_color(UINT8 r, UINT8 g, UINT8 b) {
    if (!GOP) return 0;

    UINT32 format = GOP->Mode->Info->PixelFormat;
    if (format == PixelRedGreenBlueReserved8BitPerColor) {
        // going off of \/ this must be ABGR or something
        return (((UINT32) b) << 16) | (((UINT32) g) << 8) | (((UINT32) r) << 0);
    } else if (format == PixelBlueGreenRedReserved8BitPerColor) {
        // ACTUALLY THIS IS ARGB FOR SOME REASON ??????????
        return (((UINT32) r) << 16) | (((UINT32) g) << 8) | (((UINT32) b) << 0);
    } else {
        return 0;
    }
}

void plot_pixel(UINT32 x, UINT32 y, UINT32 color) {
    UINT32 pitch = 4 * GOP->Mode->Info->PixelsPerScanLine;
    *((UINT32 *) (GOP->Mode->FrameBufferBase + (y * pitch) + (x * 4))) = color;
}
