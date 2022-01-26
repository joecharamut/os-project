#include "video.h"
#include "debug.h"
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
    if (EFI_ERROR(status)) {
        return status;
    }

    nativeMode = GOP->Mode->Mode;
    numModes = GOP->Mode->MaxMode;

    uint64_t mode = 0;
    for (UINTN i = 0; i < numModes; ++i) {
        status = GOP->QueryMode(GOP, i, &infoSize, &modeInfo);
        if (EFI_ERROR(status)) {
            return status;
        }

        if (modeInfo->HorizontalResolution == 800 && modeInfo->VerticalResolution == 600) {
            mode = i;
        }
    }

    status = GOP->SetMode(GOP, mode);
    if (EFI_ERROR(status)) {
        return status;
    }

    background_color = make_color(0x00, 0x00, 0x00);
    foreground_color = make_color(0xff, 0xff, 0xff);

    ssfn_dst.ptr = (void *) GOP->Mode->FrameBufferBase;
    ssfn_dst.w = (INT16) GOP->Mode->Info->HorizontalResolution;
    ssfn_dst.h = (INT16) GOP->Mode->Info->VerticalResolution;
    ssfn_dst.p = 4 * GOP->Mode->Info->PixelsPerScanLine;
    ssfn_dst.x = ssfn_dst.y = 0;
    ssfn_dst.fg = foreground_color;

    return EFI_SUCCESS;
}

void set_font(void *font_buf) {
    ssfn_src = font_buf;
}

void write_char(char c) {
    if ((ssfn_dst.y + ssfn_src->height) > ssfn_dst.h) {
        uint8_t *src = (uint8_t *) ssfn_dst.ptr + (ssfn_dst.p * ssfn_src->height);
        uint8_t *dst = (uint8_t *) ssfn_dst.ptr;

        // copy buffer up by one line
//        for (int i = 0; i < ssfn_dst.h * ssfn_dst.p; ++i) {
//            dst[i] = src[i];
//        }
        __asm__ volatile (
                "cld;"
                "rep movsq;"
                :
                : "S" (src), "D" (dst), "c" (ssfn_dst.h * ssfn_dst.p / 8)
                : "memory"
        );

        // blank out the last line
//        for (int i = (ssfn_dst.y - ssfn_src->height) * ssfn_dst.p / 4; i < ssfn_dst.h * ssfn_dst.p / 4; ++i) {
//            ((uint32_t *) dst)[i] = background_color;
//        }
        __asm__ volatile (
                "cld;"
                "rep stosq;"
                :
                : "a" (((uint64_t) background_color << 32) | background_color), "D" (dst + ((ssfn_dst.y - ssfn_src->height) * ssfn_dst.p)), "c" (ssfn_dst.h * ssfn_dst.p / 8)
                : "memory"
        );

        ssfn_dst.y -= ssfn_src->height;
        ssfn_dst.x = 0;
    }

    if (c == '\n') {
        ssfn_dst.y += ssfn_src->height;
        ssfn_dst.x = 0;
    } else {
        ssfn_putc(c);
    }
}

void clear_screen() {
    UINT32 *framebuffer = (UINT32 *) GOP->Mode->FrameBufferBase;
    UINT32 pixels = GOP->Mode->Info->PixelsPerScanLine * GOP->Mode->Info->VerticalResolution;
//    for (UINT32 i = 0; i < pixels; ++i) {
//        framebuffer[i] = background_color;
//    }
    __asm__ volatile (
        "cld;"
        "rep stosq;"
        :
        : "a" (((uint64_t) background_color << 32) | background_color), "D" (framebuffer), "c" (pixels/2)
        : "memory"
    );
}

void set_background_color(UINT32 color) {
    background_color = color;
}

void set_foreground_color(UINT32 color) {
    foreground_color = color;
    ssfn_dst.fg = color;
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

void copy_video_info(boot_data_t *bootData) {
    bootData->video_info.bufferAddress = GOP->Mode->FrameBufferBase;
    bootData->video_info.bufferSize = GOP->Mode->FrameBufferSize;
    bootData->video_info.horizontalResolution = GOP->Mode->Info->HorizontalResolution;
    bootData->video_info.verticalResolution = GOP->Mode->Info->VerticalResolution;
    bootData->video_info.pixelsPerScanLine = GOP->Mode->Info->PixelsPerScanLine;
}
