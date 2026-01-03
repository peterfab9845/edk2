#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE imgHandle, IN EFI_SYSTEM_TABLE *sysTable)
{
    gST = sysTable;
    gBS = sysTable->BootServices;
    gImageHandle = imgHandle;

    Print(L"Hello, world!\r\n");

    return EFI_SUCCESS;
}
