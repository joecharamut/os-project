#ifndef LOADER_SERIAL_H
#define LOADER_SERIAL_H

#include "efi.h"

EFI_STATUS serial_init();
void serial_write(int count, void *buf);

#endif //LOADER_SERIAL_H
