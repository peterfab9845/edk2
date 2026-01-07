#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/PartitionInfo.h>

EFI_STATUS EFIAPI UefiEntry(IN EFI_HANDLE imgHandle, IN EFI_SYSTEM_TABLE *sysTable)
{
    gST = sysTable;
    gBS = sysTable->BootServices;
    gImageHandle = imgHandle;

    EFI_STATUS Status;

    // get the shell parameters
    EFI_SHELL_PARAMETERS_PROTOCOL *ShellParameters;
    CONST CHAR16 *TargetGuidString;
    EFI_GUID TargetGuid;
    CHAR16 *TargetPathString;

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

    if (ShellParameters->Argc != 3) {
        Print(L"Usage: BootPartuuid.efi <target PARTUUID> <target path>\r\n");
        return EFI_SUCCESS;
    }
    TargetGuidString = ShellParameters->Argv[1];
    TargetPathString = ShellParameters->Argv[2];

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

                // load and execute the target image from the located device
                EFI_DEVICE_PATH_PROTOCOL *TargetDevicePath;
                TargetDevicePath = FileDevicePath(
                        HandleBuffer[HandleIndex],
                        TargetPathString
                        );

                EFI_HANDLE TargetImage;
                Status = gBS->LoadImage(
                        TRUE,
                        gImageHandle,
                        TargetDevicePath,
                        NULL,
                        0,
                        &TargetImage
                        );
                if (EFI_ERROR (Status)) {
                    Print(L"Failed to load image at %s\r\n", TargetPathString);
                    return Status;
                }

                Status = gBS->StartImage(
                        TargetImage,
                        0,
                        NULL
                        );
                if (EFI_ERROR (Status)) {
                    Print(L"Failed to start image\r\n");
                    return Status;
                }

                // shouldn't get here, either we start the image or failed
                return EFI_SUCCESS;
            }
        }
    }

    // got outside the loop, didn't find anything
    Print(L"Did not find any matches\r\n");
    return EFI_SUCCESS;
}
