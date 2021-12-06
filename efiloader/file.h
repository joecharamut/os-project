#ifndef LOADER_FILE_H
#define LOADER_FILE_H

#include <efi.h>

EFI_FILE_HANDLE GetVolume(EFI_HANDLE Image);
EFI_FILE_HANDLE OpenFile(EFI_FILE_HANDLE Volume, CHAR16 *Path);
UINT64 FileSize(EFI_FILE_HANDLE Handle);

#endif //LOADER_FILE_H
