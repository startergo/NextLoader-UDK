/** @file
  SetImage instance to update system firmware.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  FmpSetImage() will receive untrusted input and do basic validation.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SystemFirmwareDxe.h"

//
// SystemFmp driver private data
//
SYSTEM_FMP_PRIVATE_DATA *mSystemFmpPrivate = NULL;

EFI_GUID mCurrentImageTypeId;

BOOLEAN  mNvRamUpdated = FALSE;

/**
  Parse Config data file to get the updated data array.

  @param[in]      DataBuffer      Config raw file buffer.
  @param[in]      BufferSize      Size of raw buffer.
  @param[in, out] ConfigHeader    Pointer to the config header.
  @param[in, out] UpdateArray     Pointer to the config of update data.

  @retval EFI_NOT_FOUND         No config data is found.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Parse the config file successfully.

**/
EFI_STATUS
ParseUpdateDataFile (
  IN      UINT8                         *DataBuffer,
  IN      UINTN                         BufferSize,
  IN OUT  CONFIG_HEADER                 *ConfigHeader,
  IN OUT  UPDATE_CONFIG_DATA            **UpdateArray
  );

/**
  Update System Firmware image component.

  @param[in]  SystemFirmwareImage     Points to the System Firmware image.
  @param[in]  SystemFirmwareImageSize The length of the System Firmware image in bytes.
  @param[in]  ConfigData              Points to the component configuration structure.
  @param[out] LastAttemptVersion      The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus       The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @retval EFI_SUCCESS             The System Firmware image is updated.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
PerformUpdate (
  IN VOID                         *SystemFirmwareImage,
  IN UINTN                        SystemFirmwareImageSize,
  IN UPDATE_CONFIG_DATA           *ConfigData,
  OUT UINT32                      *LastAttemptVersion,
  OUT UINT32                      *LastAttemptStatus
  )
{
  EFI_STATUS                   Status;

  DEBUG((DEBUG_INFO, "PlatformUpdate:"));
  DEBUG((DEBUG_INFO, "  BaseAddress - 0x%lx,", ConfigData->BaseAddress));
  DEBUG((DEBUG_INFO, "  ImageOffset - 0x%x,", ConfigData->ImageOffset));
  DEBUG((DEBUG_INFO, "  Legnth - 0x%x\n", ConfigData->Length));
  Status = PerformFlashWrite (
             ConfigData->FirmwareType,
             ConfigData->BaseAddress,
             ConfigData->AddressType,
             (VOID *)((UINTN)SystemFirmwareImage + (UINTN)ConfigData->ImageOffset),
             ConfigData->Length
             );
  if (!EFI_ERROR(Status)) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
    if (ConfigData->FirmwareType == PlatformFirmwareTypeNvRam) {
      mNvRamUpdated = TRUE;
    }
  } else {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
  }
  return Status;
}

/**
  Update System Firmware image.

  @param[in]  SystemFirmwareImage     Points to the System Firmware image.
  @param[in]  SystemFirmwareImageSize The length of the System Firmware image in bytes.
  @param[in]  ConfigImage             Points to the config file image.
  @param[in]  ConfigImageSize         The length of the config file image in bytes.
  @param[out] LastAttemptVersion      The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus       The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @retval EFI_SUCCESS             The System Firmware image is updated.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
UpdateImage (
  IN VOID                         *SystemFirmwareImage,
  IN UINTN                        SystemFirmwareImageSize,
  IN VOID                         *ConfigImage,
  IN UINTN                        ConfigImageSize,
  OUT UINT32                      *LastAttemptVersion,
  OUT UINT32                      *LastAttemptStatus
  )
{
  EFI_STATUS                            Status;
  UPDATE_CONFIG_DATA                    *ConfigData;
  UPDATE_CONFIG_DATA                    *UpdateConfigData;
  CONFIG_HEADER                         ConfigHeader;
  UINTN                                 Index;

  if (ConfigImage == NULL) {
    DEBUG((DEBUG_INFO, "PlatformUpdate (NoConfig):"));
    DEBUG((DEBUG_INFO, "  BaseAddress - 0x%x,", 0));
    DEBUG((DEBUG_INFO, "  Length - 0x%x\n", SystemFirmwareImageSize));
    // ASSUME the whole System Firmware include NVRAM region.
    Status = PerformFlashWrite (
               PlatformFirmwareTypeNvRam,
               0,
               FlashAddressTypeRelativeAddress,
               SystemFirmwareImage,
               SystemFirmwareImageSize
               );
    if (!EFI_ERROR(Status)) {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;
      mNvRamUpdated = TRUE;
    } else {
      *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
    }
    return Status;
  }

  DEBUG((DEBUG_INFO, "PlatformUpdate (With Config):\n"));
  ConfigData        = NULL;
  ZeroMem (&ConfigHeader, sizeof(ConfigHeader));
  Status            = ParseUpdateDataFile (
                        ConfigImage,
                        ConfigImageSize,
                        &ConfigHeader,
                        &ConfigData
                        );
  DEBUG((DEBUG_INFO, "ParseUpdateDataFile - %r\n", Status));
  if (EFI_ERROR(Status)) {
    *LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;
    return EFI_INVALID_PARAMETER;
  }
  DEBUG((DEBUG_INFO, "ConfigHeader.NumOfUpdates - 0x%x\n", ConfigHeader.NumOfUpdates));
  DEBUG((DEBUG_INFO, "PcdEdkiiSystemFirmwareFileGuid - %g\n", PcdGetPtr(PcdEdkiiSystemFirmwareFileGuid)));

  Index = 0;
  UpdateConfigData = ConfigData;
  while (Index < ConfigHeader.NumOfUpdates) {
    if (CompareGuid(&UpdateConfigData->FileGuid, PcdGetPtr(PcdEdkiiSystemFirmwareFileGuid))) {
      DEBUG((DEBUG_INFO, "FileGuid - %g (processing)\n", &UpdateConfigData->FileGuid));
      Status = PerformUpdate (
                 SystemFirmwareImage,
                 SystemFirmwareImageSize,
                 UpdateConfigData,
                 LastAttemptVersion,
                 LastAttemptStatus
                 );
      //
      // Shall updates be serialized so that if an update is not successfully completed,
      // the remaining updates won't be performed.
      //
      if (EFI_ERROR (Status)) {
        break;
      }
    } else {
      DEBUG((DEBUG_INFO, "FileGuid - %g (ignored)\n", &UpdateConfigData->FileGuid));
    }

    Index++;
    UpdateConfigData++;
  }

  return Status;
}

/**
  Authenticate and update System Firmware image.

  Caution: This function may receive untrusted input.

  @param[in]  Image              The EDKII system FMP capsule image.
  @param[in]  ImageSize          The size of the EDKII system FMP capsule image in bytes.
  @param[out] LastAttemptVersion The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus  The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @retval EFI_SUCCESS             EDKII system FMP capsule passes authentication and the System Firmware image is updated.
  @retval EFI_SECURITY_VIOLATION  EDKII system FMP capsule fails authentication and the System Firmware image is not updated.
  @retval EFI_WRITE_PROTECTED     The flash device is read only.
**/
EFI_STATUS
SystemFirmwareAuthenticatedUpdate (
  IN VOID                         *Image,
  IN UINTN                        ImageSize,
  OUT UINT32                      *LastAttemptVersion,
  OUT UINT32                      *LastAttemptStatus
  )
{
  EFI_STATUS                  Status;
  VOID                        *SystemFirmwareImage;
  UINTN                       SystemFirmwareImageSize;
  VOID                        *ConfigImage;
  UINTN                       ConfigImageSize;
  VOID                        *AuthenticatedImage;
  UINTN                       AuthenticatedImageSize;

  AuthenticatedImage     = NULL;
  AuthenticatedImageSize = 0;

  DEBUG((DEBUG_INFO, "SystemFirmwareAuthenticatedUpdate...\n"));

  Status = CapsuleAuthenticateSystemFirmware(Image, ImageSize, FALSE, LastAttemptVersion, LastAttemptStatus, &AuthenticatedImage, &AuthenticatedImageSize);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "SystemFirmwareAuthenticateImage - %r\n", Status));
    return Status;
  }

  DEBUG((DEBUG_INFO, "ExtractSystemFirmwareImage ...\n"));
  ExtractSystemFirmwareImage(AuthenticatedImage, AuthenticatedImageSize, &SystemFirmwareImage, &SystemFirmwareImageSize);
  DEBUG((DEBUG_INFO, "ExtractConfigImage ...\n"));
  ExtractConfigImage(AuthenticatedImage, AuthenticatedImageSize, &ConfigImage, &ConfigImageSize);

  DEBUG((DEBUG_INFO, "UpdateImage ...\n"));
  Status = UpdateImage(SystemFirmwareImage, SystemFirmwareImageSize, ConfigImage, ConfigImageSize, LastAttemptVersion, LastAttemptStatus);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "UpdateImage - %r\n", Status));
    return Status;
  }

  DEBUG((DEBUG_INFO, "SystemFirmwareAuthenticatedUpdate Done\n"));

  return EFI_SUCCESS;
}

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  @param[in]      VariableName               Name of Variable to be found.
  @param[in]      VendorGuid                 Variable vendor GUID.
  @param[out]     Attributes                 Attribute value of the variable found.
  @param[in, out] DataSize                   Size of Data found. If size is less than the
                                             data, this value contains the required size.
  @param[out]     Data                       Data pointer.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
GetVariableHook (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data
  )
{
  DEBUG((DEBUG_INFO, "GetVariableHook - %S, %g\n", VariableName, VendorGuid));
  return EFI_NOT_AVAILABLE_YET;
}

/**

  This code Finds the Next available variable.

  @param[in, out] VariableNameSize           Size of the variable name.
  @param[in, out] VariableName               Pointer to variable name.
  @param[in, out] VendorGuid                 Variable Vendor Guid.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
GetNextVariableNameHook (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid
  )
{
  DEBUG((DEBUG_INFO, "GetNextVariableNameHook - %S, %g\n", VariableName, VendorGuid));
  return EFI_NOT_AVAILABLE_YET;
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param[in] VariableName                     Name of Variable to be found.
  @param[in] VendorGuid                       Variable vendor GUID.
  @param[in] Attributes                       Attribute value of the variable found
  @param[in] DataSize                         Size of Data found. If size is less than the
                                              data, this value contains the required size.
  @param[in] Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
SetVariableHook (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data
  )
{
  DEBUG((DEBUG_INFO, "SetVariableHook - %S, %g, 0x%x (0x%x)\n", VariableName, VendorGuid, Attributes, DataSize));
  return EFI_NOT_AVAILABLE_YET;
}

/**

  This code returns information about the EFI variables.

  @param[in]  Attributes                     Attributes bitmask to specify the type of variables
                                             on which to return information.
  @param[out] MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                             for the EFI variables associated with the attributes specified.
  @param[out] RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                             for EFI variables associated with the attributes specified.
  @param[out] MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                             associated with the attributes specified.

  @return EFI_SUCCESS                   Query successfully.

**/
EFI_STATUS
EFIAPI
QueryVariableInfoHook (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  )
{
  DEBUG((DEBUG_INFO, "QueryVariableInfoHook - 0x%x\n", Attributes));
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Updates the firmware image of the device.

  This function updates the hardware with the new firmware image.
  This function returns EFI_UNSUPPORTED if the firmware image is not updatable.
  If the firmware image is updatable, the function should perform the following minimal validations
  before proceeding to do the firmware image update.
  - Validate the image authentication if image has attribute
    IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED. The function returns
    EFI_SECURITY_VIOLATION if the validation fails.
  - Validate the image is a supported image for this device. The function returns EFI_ABORTED if
    the image is unsupported. The function can optionally provide more detailed information on
    why the image is not a supported image.
  - Validate the data from VendorCode if not null. Image validation must be performed before
    VendorCode data validation. VendorCode data is ignored or considered invalid if image
    validation failed. The function returns EFI_ABORTED if the data is invalid.

  VendorCode enables vendor to implement vendor-specific firmware image update policy. Null if
  the caller did not specify the policy or use the default policy. As an example, vendor can implement
  a policy to allow an option to force a firmware image update when the abort reason is due to the new
  firmware image version is older than the current firmware image version or bad image checksum.
  Sensitive operations such as those wiping the entire firmware image and render the device to be
  non-functional should be encoded in the image itself rather than passed with the VendorCode.
  AbortReason enables vendor to have the option to provide a more detailed description of the abort
  reason to the caller.

  @param[in]  This               A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in]  ImageIndex         A unique number identifying the firmware image(s) within the device.
                                 The number is between 1 and DescriptorCount.
  @param[in]  Image              Points to the new image.
  @param[in]  ImageSize          Size of the new image in bytes.
  @param[in]  VendorCode         This enables vendor to implement vendor-specific firmware image update policy.
                                 Null indicates the caller did not specify the policy or use the default policy.
  @param[in]  Progress           A function used by the driver to report the progress of the firmware update.
  @param[out] AbortReason        A pointer to a pointer to a null-terminated string providing more
                                 details for the aborted operation. The buffer is allocated by this function
                                 with AllocatePool(), and it is the caller's responsibility to free it with a
                                 call to FreePool().

  @retval EFI_SUCCESS            The device was successfully updated with the new image.
  @retval EFI_ABORTED            The operation is aborted.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.
  @retval EFI_SECURITY_VIOLATIO  The operation could not be performed due to an authentication failure.

**/
EFI_STATUS
EFIAPI
FmpSetImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL                 *This,
  IN  UINT8                                            ImageIndex,
  IN  CONST VOID                                       *Image,
  IN  UINTN                                            ImageSize,
  IN  CONST VOID                                       *VendorCode,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS    Progress,
  OUT CHAR16                                           **AbortReason
  )
{
  EFI_STATUS              Status;
  EFI_STATUS              VarStatus;
  SYSTEM_FMP_PRIVATE_DATA *SystemFmpPrivate;

  if (Image == NULL || ImageSize == 0 || AbortReason == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SystemFmpPrivate = SYSTEM_FMP_PRIVATE_DATA_FROM_FMP(This);
  *AbortReason     = NULL;

  if (ImageIndex == 0 || ImageIndex > SystemFmpPrivate->DescriptorCount) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SystemFirmwareAuthenticatedUpdate((VOID *)Image, ImageSize, &SystemFmpPrivate->LastAttempt.LastAttemptVersion, &SystemFmpPrivate->LastAttempt.LastAttemptStatus);
  DEBUG((DEBUG_INFO, "SetImage - LastAttemp Version - 0x%x, State - 0x%x\n", SystemFmpPrivate->LastAttempt.LastAttemptVersion, SystemFmpPrivate->LastAttempt.LastAttemptStatus));

  //
  // If NVRAM is updated, we should no longer touch variable services, because
  // the current variable driver may not manage the new NVRAM region.
  //
  if (mNvRamUpdated) {
    DEBUG ((DEBUG_INFO, "NvRamUpdated, Update Variable Serivces\n"));
    gRT->GetVariable         = GetVariableHook;
    gRT->GetNextVariableName = GetNextVariableNameHook;
    gRT->SetVariable         = SetVariableHook;
    gRT->QueryVariableInfo   = QueryVariableInfoHook;

    gRT->Hdr.CRC32 = 0;
    gBS->CalculateCrc32 (
          (UINT8 *) &gRT->Hdr,
          gRT->Hdr.HeaderSize,
          &gRT->Hdr.CRC32
          );
  }

  VarStatus = gRT->SetVariable(
                     SYSTEM_FMP_LAST_ATTEMPT_VARIABLE_NAME,
                     &gSystemFmpLastAttemptVariableGuid,
                     EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                     sizeof(SystemFmpPrivate->LastAttempt),
                     &SystemFmpPrivate->LastAttempt
                     );
  DEBUG((DEBUG_INFO, "SetLastAttemp - %r\n", VarStatus));

  return Status;
}

/**
  System FMP module entrypoint

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @return EFI_SUCCESS System FMP module is initialized.
**/
EFI_STATUS
EFIAPI
SystemFirmwareUpdateMainDxe (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS                                      Status;

  //
  // Initialize SystemFmpPrivateData
  //
  mSystemFmpPrivate = AllocateZeroPool (sizeof(SYSTEM_FMP_PRIVATE_DATA));
  if (mSystemFmpPrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InitializePrivateData(mSystemFmpPrivate);
  if (EFI_ERROR(Status)) {
    FreePool(mSystemFmpPrivate);
    mSystemFmpPrivate = NULL;
    return Status;
  }

  //
  // Install FMP protocol.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSystemFmpPrivate->Handle,
                  &gEfiFirmwareManagementProtocolGuid,
                  &mSystemFmpPrivate->Fmp,
                  &gSystemFmpProtocolGuid,
                  &mSystemFmpPrivate->Fmp,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool(mSystemFmpPrivate);
    mSystemFmpPrivate = NULL;
    return Status;
  }

  return Status;
}
