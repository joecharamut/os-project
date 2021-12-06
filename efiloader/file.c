#include "file.h"
#include <efilib.h>

EFI_FILE_HANDLE GetVolume(EFI_HANDLE Image) {
    EFI_LOADED_IMAGE *LoadedImage = NULL;
    EFI_GUID ImageGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    EFI_FILE_IO_INTERFACE *IOVolume;
    EFI_GUID FsGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_STATUS Status;
    EFI_FILE_HANDLE Volume;

    Status = BS->HandleProtocol(Image, &ImageGuid, (void **) &LoadedImage);
    if (EFI_ERROR(Status)) {
        Print(L"GetVolume Error: HandleProtocol(Image)\n");
        return NULL;
    }

    Status = BS->HandleProtocol(LoadedImage->DeviceHandle, &FsGuid, (void **) &IOVolume);
    if (EFI_ERROR(Status)) {
        Print(L"GetVolume Error: HandleProtocol(DeviceHandle)\n");
        return NULL;
    }

    Status = IOVolume->OpenVolume(IOVolume, &Volume);
    if (EFI_ERROR(Status)) {
        Print(L"GetVolume Error: OpenVolume()\n");
        return NULL;
    }

    return Volume;
}

EFI_FILE_HANDLE OpenFile(EFI_FILE_HANDLE Volume, CHAR16 *Path) {
    EFI_FILE_HANDLE File;
    EFI_STATUS Status;

    Status = Volume->Open(Volume, &File, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    if (EFI_ERROR(Status)) {
        Print(L"Error opening file: '%s'\n", Path);
        return NULL;
    }

    return File;
}

UINT64 FileSize(EFI_FILE_HANDLE Handle) {
    UINT64 ret;
    EFI_FILE_INFO *FileInfo;

    FileInfo = LibFileInfo(Handle);
    ret = FileInfo->FileSize;
    FreePool(FileInfo);

    return ret;
}
