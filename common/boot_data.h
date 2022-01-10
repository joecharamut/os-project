#ifndef LOADER_BOOT_DATA_H
#define LOADER_BOOT_DATA_H

#include <stdint.h>

#define BOOT_DATA_SIGNATURE 0x534B52554E4B4C59

typedef struct {
    uint64_t signature;

    struct {
        uint64_t bufferAddress;
        uint64_t bufferSize;
        uint64_t horizontalResolution;
        uint64_t verticalResolution;
        uint64_t pixelsPerScanLine;
    } video_info;

    struct {
        uint64_t count;
        void *entries;
    } memory_info;

} boot_data_t;

#endif //LOADER_BOOT_DATA_H
