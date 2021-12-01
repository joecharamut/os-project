#include <efi.h>
#include <efilib.h>

EFI_FILE_HANDLE GetVolume(EFI_HANDLE Image) {
    EFI_LOADED_IMAGE *LoadedImage = NULL;
    EFI_GUID ImageGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    EFI_FILE_IO_INTERFACE *IOVolume;
    EFI_GUID FsGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_FILE_HANDLE Volume;

    BS->HandleProtocol(Image, &ImageGuid, (void **) &LoadedImage);
    BS->HandleProtocol(LoadedImage->DeviceHandle, &FsGuid, (void **) &IOVolume);
    IOVolume->OpenVolume(IOVolume, &Volume);

    return Volume;
}

UINT64 FileSize(EFI_FILE_HANDLE Handle) {
    UINT64 ret;
    EFI_FILE_INFO *FileInfo;

    FileInfo = LibFileInfo(Handle);
    ret = FileInfo->FileSize;
    FreePool(FileInfo);

    return ret;
}

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // setup global vars
    InitializeLib(ImageHandle, SystemTable);

    // disable watchdog
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    ST->ConOut->SetAttribute(ST->ConOut, EFI_BACKGROUND_BLUE | EFI_WHITE);
    ST->ConOut->ClearScreen(ST->ConOut);
    Print(L"Hello UEFI World!\n");

    EFI_STATUS status;
    EFI_FILE_HANDLE volume = GetVolume(ImageHandle);
    EFI_FILE_HANDLE handle;
    status = volume->Open(volume, &handle, L"KERNEL.BIN", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);

    if (EFI_ERROR(status)) {
        ST->ConOut->OutputString(ST->ConOut, L"Error opening kernel.bin\r\n");
        return status;
    }
    Print(L"Filesize of kernel.bin: %llu bytes\n", FileSize(handle));

    EFI_INPUT_KEY key;

    status = ST->ConOut->OutputString(ST->ConOut, L"Hello World\r\n");
    if (EFI_ERROR(status)) {
        return status;
    }

    status = ST->ConIn->Reset(ST->ConIn, FALSE);
    if (EFI_ERROR(status)) {
        return status;
    }
    while ((status = ST->ConIn->ReadKeyStroke(ST->ConIn, &key)) == EFI_NOT_READY) {
        // do nothing
    }

    return status;
}
