#ifndef LOADER_MAIN_H
#define LOADER_MAIN_H

#include <efi.h>

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
EFI_MEMORY_DESCRIPTOR *efi_get_mem_map(UINTN *MapKey, UINTN *Entries);
void efi_dump_mem_map(EFI_MEMORY_DESCRIPTOR *mmap, UINTN size);
void efi_print_mem_map(EFI_MEMORY_DESCRIPTOR *mmap, UINTN size);
EFI_STATUS efi_exit();

#endif //LOADER_MAIN_H
