#ifndef LOADER_FILE_H
#define LOADER_FILE_H

#include <efi.h>

EFI_STATUS GetVolume(EFI_HANDLE Image, EFI_FILE_HANDLE *Volume);
uint64_t FileSize(EFI_FILE_HANDLE Handle);

#endif //LOADER_FILE_H
