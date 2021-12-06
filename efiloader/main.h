#ifndef LOADER_MAIN_H
#define LOADER_MAIN_H

#include <efi.h>

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
EFI_STATUS efi_print_mem_map();
EFI_STATUS efi_exit();

#endif //LOADER_MAIN_H
