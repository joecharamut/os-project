#include <efi.h>
#include <efilib.h>

#define CHECK(s) do { if (EFI_ERROR((s))) return (s); } while (0)

__attribute__((used)) EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    EFI_STATUS Status;

    EFI_LOADED_IMAGE *ThisImage = NULL;
    Status = BS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void **) &ThisImage);
    CHECK(Status);

    EFI_DEVICE_PATH *LoaderPath = FileDevicePath(ThisImage->DeviceHandle, L"\\qOS\\loader.efi");

    EFI_HANDLE LoaderImage;
    Status = BS->LoadImage(FALSE, ImageHandle, LoaderPath, NULL, 0, &LoaderImage);
    CHECK(Status);

    Status = BS->StartImage(LoaderImage, NULL, NULL);
    CHECK(Status);

    return EFI_LOAD_ERROR;
}
