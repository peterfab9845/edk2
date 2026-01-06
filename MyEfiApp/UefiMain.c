#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/PartitionInfo.h>

EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE imgHandle, IN EFI_SYSTEM_TABLE *sysTable)
{
    gST = sysTable;
    gBS = sysTable->BootServices;
    gImageHandle = imgHandle;

    EFI_STATUS Status;

    // get the parent shell
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_SHELL_PROTOCOL *Shell;

    Status = gBS->OpenProtocol(
            gImageHandle,
            &gEfiLoadedImageProtocolGuid,
            (VOID **) &LoadedImage,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
    if (EFI_ERROR (Status)) {
        Print(L"Failed to get loaded image protocol\r\n");
        return Status;
    }
    if (LoadedImage->ParentHandle != NULL) {
        Status = gBS->OpenProtocol(
                LoadedImage->ParentHandle,
                &gEfiShellProtocolGuid,
                (VOID **) &Shell,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );
        if (EFI_ERROR (Status)) {
            // ignore it, will search for the protocol
        }
    }

    if (Shell == NULL) {
        // didn't get it from the parent, search everywhere
        Status = gBS->LocateProtocol(
                &gEfiShellProtocolGuid,
                NULL,
                (VOID **) &Shell
                );
        if (EFI_ERROR (Status)) {
            Print(L"Failed to get shell protocol\r\n");
            return Status;
        }
    }

    // get the shell parameters
    EFI_SHELL_PARAMETERS_PROTOCOL *ShellParameters;
    CONST CHAR16 *TargetGuidString;//[] = L"10FA6F54-79A5-4185-BC0A-618692606103";
    EFI_GUID TargetGuid;

    Status = gBS->OpenProtocol(
            gImageHandle,
            &gEfiShellParametersProtocolGuid,
            (VOID **) &ShellParameters,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
    if (EFI_ERROR (Status)) {
        Print(L"Failed to get shell parameters\r\n");
        return Status;
    }

    if (ShellParameters->Argc != 2) {
        Print(L"Takes 1 argument of target GUID\r\n");
        return EFI_SUCCESS;
    }
    TargetGuidString = ShellParameters->Argv[1];

    Status = StrToGuid(TargetGuidString, &TargetGuid);
    if (EFI_ERROR (Status)) {
        Print(L"Failed to convert GUID from string\r\n");
        return Status;
    }


    // search handles for the desired partition guid
    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleIndex;
    EFI_PARTITION_INFO_PROTOCOL *PartitionInfo;
    EFI_GUID PartitionGuid;

    Status = gBS->LocateHandleBuffer(
            ByProtocol,
            &gEfiPartitionInfoProtocolGuid,
            NULL,
            &HandleCount,
            &HandleBuffer
            );
    if (EFI_ERROR (Status)) {
        Print(L"Failed to locate any handles with partition info\r\n");
        return Status;
    }

    for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
        Status = gBS->OpenProtocol(
                HandleBuffer[HandleIndex],
                &gEfiPartitionInfoProtocolGuid,
                (VOID **) &PartitionInfo,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );
        if (EFI_ERROR (Status)) {
            Print(L"Failed to open partition info protocol on handle 0x%x\r\n", HandleBuffer[HandleIndex]);
            return Status;
        }

        if (PartitionInfo->Type == PARTITION_TYPE_GPT) {
            PartitionGuid = PartitionInfo->Info.Gpt.UniquePartitionGUID;
            //Print(L"Looking at GUID: %g\r\n", &PartitionGuid);

            if (CompareGuid(&PartitionGuid, &TargetGuid)) {
                //Print(L"Found it!\r\n");

                // cd to the located partition
                EFI_DEVICE_PATH_PROTOCOL *DevicePath;
                CONST CHAR16 *DeviceMapping;
                CONST CHAR16 *LastMap;

                Status = gBS->OpenProtocol(
                        HandleBuffer[HandleIndex],
                        &gEfiDevicePathProtocolGuid,
                        (VOID **) &DevicePath,
                        gImageHandle,
                        NULL,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
                if (EFI_ERROR (Status)) {
                    Print(L"Failed to get device path\r\n");
                    return Status;
                }

                DeviceMapping = Shell->GetMapFromDevicePath(
                        &DevicePath
                        );
                if (DeviceMapping == NULL) {
                    Print(L"Failed to get device mapping\r\n");
                    return EFI_UNSUPPORTED;
                }
                Print(L"Found device mapping: %s\r\n", DeviceMapping);

                LastMap = DeviceMapping;
                while (*DeviceMapping) {
                    if (*DeviceMapping++ == L';') {
                        LastMap = DeviceMapping;
                    }
                }
                Print(L"Last component: %s\r\n", LastMap);

                Status = Shell->SetCurDir(
                        NULL,
                        LastMap
                        );
                if (EFI_ERROR (Status)) {
                    Print(L"Failed to cd to %s\r\n", LastMap);
                    return Status;
                }

                // don't do anything else
                return EFI_SUCCESS;
            }
        }
    }

    // got outside the loop, didn't find anything
    Print(L"Did not find any matches\r\n");
    return EFI_SUCCESS;
}
