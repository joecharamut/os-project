#include "file.h"
#include <efilib.h>

EFI_STATUS GetVolume(EFI_HANDLE Image, EFI_FILE_HANDLE *Volume) {
    EFI_LOADED_IMAGE *LoadedImage = NULL;
    EFI_GUID ImageGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    EFI_FILE_IO_INTERFACE *IOVolume;
    EFI_GUID FsGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_STATUS Status;

    Status = BS->HandleProtocol(Image, &ImageGuid, (void **) &LoadedImage);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = BS->HandleProtocol(LoadedImage->DeviceHandle, &FsGuid, (void **) &IOVolume);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = IOVolume->OpenVolume(IOVolume, Volume);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    return EFI_SUCCESS;
}

uint64_t FileSize(EFI_FILE_HANDLE Handle) {
    uint64_t size;
    EFI_FILE_INFO *FileInfo;

    FileInfo = LibFileInfo(Handle);
    size = FileInfo->FileSize;
    FreePool(FileInfo);

    return size;
}
