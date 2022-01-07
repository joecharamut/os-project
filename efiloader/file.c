#include "file.h"
#include <efilib.h>

const CHAR16 * const EFI_ERROR_TYPE_STRINGS[] = {
        L"EFI_SUCCESS",
        L"EFI_LOAD_ERROR",
        L"EFI_INVALID_PARAMETER",
        L"EFI_UNSUPPORTED",
        L"EFI_BAD_BUFFER_SIZE",
        L"EFI_BUFFER_TOO_SMALL",
        L"EFI_NOT_READY",
        L"EFI_DEVICE_ERROR",
        L"EFI_WRITE_PROTECTED",
        L"EFI_OUT_OF_RESOURCES",
        L"EFI_VOLUME_CORRUPTED",
        L"EFI_VOLUME_FULL",
        L"EFI_NO_MEDIA",
        L"EFI_MEDIA_CHANGED",
        L"EFI_NOT_FOUND",
        L"EFI_ACCESS_DENIED",
        L"EFI_NO_RESPONSE",
        L"EFI_NO_MAPPING",
        L"EFI_TIMEOUT",
        L"EFI_NOT_STARTED",
        L"EFI_ALREADY_STARTED",
        L"EFI_ABORTED",
        L"EFI_ICMP_ERROR",
        L"EFI_TFTP_ERROR",
        L"EFI_PROTOCOL_ERROR",
        L"EFI_INCOMPATIBLE_VERSION",
        L"EFI_SECURITY_VIOLATION",
        L"EFI_CRC_ERROR",
        L"EFI_END_OF_MEDIA",
        L"EFI_END_OF_FILE",
        L"EFI_INVALID_LANGUAGE",
        L"EFI_COMPROMISED_DATA",
};

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

    Status = Volume->Open(Volume, &File, Path, EFI_FILE_MODE_READ, 0);

    if (Status == EFI_INVALID_PARAMETER) {
        Print(L"Error opening file: '%s' (%s)\n", Path, EFI_ERROR_TYPE_STRINGS[Status & 255]);
        Print(L"(Make sure you use \\ instead of / because microsoft)\n");
        return NULL;
    }

    if (EFI_ERROR(Status)) {
        Print(L"Error opening file: '%s' (%s)\n", Path, EFI_ERROR_TYPE_STRINGS[Status & 255]);
        return NULL;
    }

    return File;
}

UINT64 FileSize(EFI_FILE_HANDLE Handle) {
    UINT64 size;
    EFI_FILE_INFO *FileInfo;

    FileInfo = LibFileInfo(Handle);
    size = FileInfo->FileSize;
    FreePool(FileInfo);

    return size;
}
