#include "serial.h"

#include "efilib.h"

EFI_SERIAL_IO_PROTOCOL *SIO = NULL;

EFI_STATUS serial_init() {
    if (SIO) return EFI_SUCCESS;
    EFI_STATUS status;

    EFI_GUID sioGuid = EFI_SERIAL_IO_PROTOCOL_GUID;
    status = BS->LocateProtocol(&sioGuid, NULL, (void **) &SIO);
    if (EFI_ERROR(status)) {
        return status;
    }

    return EFI_SUCCESS;
}

void serial_write(int count, void *buf) {
    UINTN num = count;
    SIO->Write(SIO, &num, buf);
}
