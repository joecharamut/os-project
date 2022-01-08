#ifndef LOADER_MAIN_H
#define LOADER_MAIN_H

#include <efi.h>

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);

#endif //LOADER_MAIN_H
