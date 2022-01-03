#ifndef LOADER_MAIN_H
#define LOADER_MAIN_H

#include <efi.h>

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
EFI_MEMORY_DESCRIPTOR *efi_get_mem_map(UINTN *MapKey, UINTN *Entries, UINTN *DescriptorSize);
void efi_dump_mem_map(unsigned char *mmap_buf, UINTN size, UINTN descriptorSize);

#endif //LOADER_MAIN_H
