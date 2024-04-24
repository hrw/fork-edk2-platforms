/** @file
*  This file is an ACPI driver for the Qemu SBSA platform.
*
*  Copyright (c) 2024, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/PciRootBridgeIo.h>

#include "SbsaQemuAcpiPcie.h"

#pragma pack(1)

/** Adds the _OSC method to the PCIe node.

  @param PciNode  PCIe device node.

  @return EFI_SUCCESS on success, or an error code.

**/
STATIC
EFI_STATUS
AddOscMethod (
  IN  OUT   AML_OBJECT_NODE_HANDLE  PciNode
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_DESCRIPTION_HEADER  *SsdtPcieOscTemplate;
  AML_ROOT_NODE_HANDLE         OscTemplateRoot;
  AML_OBJECT_NODE_HANDLE       OscNode;
  AML_OBJECT_NODE_HANDLE       ClonedOscNode;

  ASSERT (PciNode != NULL);

  /* Parse the Ssdt Pci Osc Template. */
  SsdtPcieOscTemplate = (EFI_ACPI_DESCRIPTION_HEADER *)
                        ssdttemplate_aml_code;

  Status = AmlParseDefinitionBlock (
             SsdtPcieOscTemplate,
             &OscTemplateRoot
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PCI-OSC: Failed to parse SSDT PCI OSC Template."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AmlFindNode (OscTemplateRoot, "\\_OSC", &OscNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AmlFindNode: %r\n", Status));
    return Status;
  }

  Status = AmlCloneTree (OscNode, &ClonedOscNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AmlCloneTree: %r\n", Status));
    return Status;
  }

  Status = AmlAttachNode (PciNode, ClonedOscNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AmlAttachNode: %r\n", Status));
    // Free the cloned node.
    AmlDeleteTree (ClonedOscNode);
    return Status;
  }

  return Status;
}

/** Creates a PCI Interrupt Link Device.

  @param PciDeviceHandle  PCIe device node.
  @param Uid              UID of the Link Device.
  @param LinkName         Name of the Device.
  @param Irq              IRQ number.

**/
STATIC
VOID
GenPciLinkDevice (
  AML_OBJECT_NODE_HANDLE  PciDeviceHandle,
  UINT32                  Uid,
  CONST CHAR8             *LinkName,
  UINT32                  Irq
  )
{
  EFI_STATUS              Status;
  UINT32                  EisaId;
  AML_OBJECT_NODE_HANDLE  GsiNode;

  AmlCodeGenDevice (LinkName, PciDeviceHandle, &GsiNode);

  Status = AmlGetEisaIdFromString ("PNP0C0F", &EisaId);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return;
  }

  AmlCodeGenNameInteger ("_HID", EisaId, GsiNode, NULL);
  AmlCodeGenNameInteger ("_UID", Uid, GsiNode, NULL);

  AML_OBJECT_NODE_HANDLE  Prs;

  AmlCodeGenNameResourceTemplate ("_PRS", GsiNode, &Prs);
  AmlCodeGenRdInterrupt (FALSE, FALSE, FALSE, FALSE, &Irq, 1, Prs, NULL);
  AmlCodeGenMethodRetNameString ("_CRS", "_PRS", 0, TRUE, 0, GsiNode, NULL);
  AmlCodeGenMethodRetNameString ("_SRS", NULL, 1, FALSE, 0, GsiNode, NULL);
  AmlCodeGenMethodRetNameString ("_DIS", NULL, 0, FALSE, 0, GsiNode, NULL);
}

/** Creates a _PRT package.

  @param PciDeviceHandle  PCIe device node.

**/
STATIC
VOID
GenPrtEntries (
  AML_OBJECT_NODE_HANDLE  PciDeviceHandle
  )
{
  AML_OBJECT_NODE_HANDLE  PrtNode;

  AmlCodeGenNamePackage ("_PRT", PciDeviceHandle, &PrtNode);

  AmlAddPrtEntry (0x0000FFFF, 0, "GSI0", 0, PrtNode);
  AmlAddPrtEntry (0x0000FFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x0000FFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x0000FFFF, 0, "GSI3", 0, PrtNode);

  AmlAddPrtEntry (0x0001FFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x0001FFFF, 0, "GSI2", 1, PrtNode);
  AmlAddPrtEntry (0x0001FFFF, 0, "GSI3", 2, PrtNode);
  AmlAddPrtEntry (0x0001FFFF, 0, "GSI0", 3, PrtNode);

  AmlAddPrtEntry (0x0002FFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x0002FFFF, 0, "GSI3", 1, PrtNode);
  AmlAddPrtEntry (0x0002FFFF, 0, "GSI0", 2, PrtNode);
  AmlAddPrtEntry (0x0002FFFF, 0, "GSI1", 3, PrtNode);

  AmlAddPrtEntry (0x0003FFFF, 0, "GSI3", 0, PrtNode);
  AmlAddPrtEntry (0x0003FFFF, 0, "GSI0", 1, PrtNode);
  AmlAddPrtEntry (0x0003FFFF, 0, "GSI1", 2, PrtNode);
  AmlAddPrtEntry (0x0003FFFF, 0, "GSI2", 3, PrtNode);

  AmlAddPrtEntry (0x0004FFFF, 0, "GSI0", 0, PrtNode);
  AmlAddPrtEntry (0x0004FFFF, 0, "GSI1", 1, PrtNode);

  AmlAddPrtEntry (0x0004FFFF, 0, "GSI2", 2, PrtNode);
  AmlAddPrtEntry (0x0004FFFF, 0, "GSI3", 3, PrtNode);

  AmlAddPrtEntry (0x0005FFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x0005FFFF, 0, "GSI2", 1, PrtNode);
  AmlAddPrtEntry (0x0005FFFF, 0, "GSI3", 2, PrtNode);
  AmlAddPrtEntry (0x0005FFFF, 0, "GSI0", 3, PrtNode);

  AmlAddPrtEntry (0x0006FFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x0006FFFF, 0, "GSI3", 1, PrtNode);
  AmlAddPrtEntry (0x0006FFFF, 0, "GSI0", 2, PrtNode);
  AmlAddPrtEntry (0x0006FFFF, 0, "GSI1", 3, PrtNode);

  AmlAddPrtEntry (0x0007FFFF, 0, "GSI3", 0, PrtNode);
  AmlAddPrtEntry (0x0007FFFF, 0, "GSI0", 1, PrtNode);
  AmlAddPrtEntry (0x0007FFFF, 0, "GSI1", 2, PrtNode);
  AmlAddPrtEntry (0x0007FFFF, 0, "GSI2", 3, PrtNode);

  AmlAddPrtEntry (0x0008FFFF, 0, "GSI0", 0, PrtNode);
  AmlAddPrtEntry (0x0008FFFF, 0, "GSI1", 1, PrtNode);
  AmlAddPrtEntry (0x0008FFFF, 0, "GSI2", 2, PrtNode);
  AmlAddPrtEntry (0x0008FFFF, 0, "GSI3", 3, PrtNode);

  AmlAddPrtEntry (0x0009FFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x0009FFFF, 0, "GSI2", 1, PrtNode);
  AmlAddPrtEntry (0x0009FFFF, 0, "GSI3", 2, PrtNode);
  AmlAddPrtEntry (0x0009FFFF, 0, "GSI0", 3, PrtNode);

  AmlAddPrtEntry (0x000AFFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x000AFFFF, 0, "GSI3", 1, PrtNode);
  AmlAddPrtEntry (0x000AFFFF, 0, "GSI0", 2, PrtNode);
  AmlAddPrtEntry (0x000AFFFF, 0, "GSI1", 3, PrtNode);

  AmlAddPrtEntry (0x000BFFFF, 0, "GSI3", 0, PrtNode);
  AmlAddPrtEntry (0x000BFFFF, 0, "GSI0", 1, PrtNode);
  AmlAddPrtEntry (0x000BFFFF, 0, "GSI1", 2, PrtNode);
  AmlAddPrtEntry (0x000BFFFF, 0, "GSI2", 3, PrtNode);

  AmlAddPrtEntry (0x000CFFFF, 0, "GSI0", 0, PrtNode);
  AmlAddPrtEntry (0x000CFFFF, 0, "GSI1", 1, PrtNode);
  AmlAddPrtEntry (0x000CFFFF, 0, "GSI2", 2, PrtNode);
  AmlAddPrtEntry (0x000CFFFF, 0, "GSI3", 3, PrtNode);

  AmlAddPrtEntry (0x000DFFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x000DFFFF, 0, "GSI2", 1, PrtNode);
  AmlAddPrtEntry (0x000DFFFF, 0, "GSI3", 2, PrtNode);
  AmlAddPrtEntry (0x000DFFFF, 0, "GSI0", 3, PrtNode);

  AmlAddPrtEntry (0x000EFFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x000EFFFF, 0, "GSI3", 1, PrtNode);
  AmlAddPrtEntry (0x000EFFFF, 0, "GSI0", 2, PrtNode);
  AmlAddPrtEntry (0x000EFFFF, 0, "GSI1", 3, PrtNode);

  AmlAddPrtEntry (0x000FFFFF, 0, "GSI3", 0, PrtNode);
  AmlAddPrtEntry (0x000FFFFF, 0, "GSI0", 1, PrtNode);
  AmlAddPrtEntry (0x000FFFFF, 0, "GSI1", 2, PrtNode);
  AmlAddPrtEntry (0x000FFFFF, 0, "GSI2", 3, PrtNode);

  AmlAddPrtEntry (0x0010FFFF, 0, "GSI0", 0, PrtNode);
  AmlAddPrtEntry (0x0010FFFF, 0, "GSI1", 1, PrtNode);
  AmlAddPrtEntry (0x0010FFFF, 0, "GSI2", 2, PrtNode);
  AmlAddPrtEntry (0x0010FFFF, 0, "GSI3", 3, PrtNode);

  AmlAddPrtEntry (0x0011FFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x0011FFFF, 0, "GSI2", 1, PrtNode);
  AmlAddPrtEntry (0x0011FFFF, 0, "GSI3", 2, PrtNode);
  AmlAddPrtEntry (0x0011FFFF, 0, "GSI0", 3, PrtNode);

  AmlAddPrtEntry (0x0012FFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x0012FFFF, 0, "GSI3", 1, PrtNode);
  AmlAddPrtEntry (0x0012FFFF, 0, "GSI0", 2, PrtNode);
  AmlAddPrtEntry (0x0012FFFF, 0, "GSI1", 3, PrtNode);

  AmlAddPrtEntry (0x0013FFFF, 0, "GSI3", 0, PrtNode);
  AmlAddPrtEntry (0x0013FFFF, 0, "GSI0", 1, PrtNode);
  AmlAddPrtEntry (0x0013FFFF, 0, "GSI1", 2, PrtNode);
  AmlAddPrtEntry (0x0013FFFF, 0, "GSI2", 3, PrtNode);

  AmlAddPrtEntry (0x0014FFFF, 0, "GSI0", 0, PrtNode);
  AmlAddPrtEntry (0x0014FFFF, 0, "GSI1", 1, PrtNode);
  AmlAddPrtEntry (0x0014FFFF, 0, "GSI2", 2, PrtNode);
  AmlAddPrtEntry (0x0014FFFF, 0, "GSI3", 3, PrtNode);

  AmlAddPrtEntry (0x0015FFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x0015FFFF, 0, "GSI2", 1, PrtNode);
  AmlAddPrtEntry (0x0015FFFF, 0, "GSI3", 2, PrtNode);
  AmlAddPrtEntry (0x0015FFFF, 0, "GSI0", 3, PrtNode);

  AmlAddPrtEntry (0x0016FFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x0016FFFF, 0, "GSI3", 1, PrtNode);
  AmlAddPrtEntry (0x0016FFFF, 0, "GSI0", 2, PrtNode);
  AmlAddPrtEntry (0x0016FFFF, 0, "GSI1", 3, PrtNode);

  AmlAddPrtEntry (0x0017FFFF, 0, "GSI3", 0, PrtNode);
  AmlAddPrtEntry (0x0017FFFF, 0, "GSI0", 1, PrtNode);
  AmlAddPrtEntry (0x0017FFFF, 0, "GSI1", 2, PrtNode);
  AmlAddPrtEntry (0x0017FFFF, 0, "GSI2", 3, PrtNode);

  AmlAddPrtEntry (0x0018FFFF, 0, "GSI0", 0, PrtNode);
  AmlAddPrtEntry (0x0018FFFF, 0, "GSI1", 1, PrtNode);
  AmlAddPrtEntry (0x0018FFFF, 0, "GSI2", 2, PrtNode);
  AmlAddPrtEntry (0x0018FFFF, 0, "GSI3", 3, PrtNode);

  AmlAddPrtEntry (0x0019FFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x0019FFFF, 0, "GSI2", 1, PrtNode);
  AmlAddPrtEntry (0x0019FFFF, 0, "GSI3", 2, PrtNode);
  AmlAddPrtEntry (0x0019FFFF, 0, "GSI0", 3, PrtNode);

  AmlAddPrtEntry (0x001AFFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x001AFFFF, 0, "GSI3", 1, PrtNode);
  AmlAddPrtEntry (0x001AFFFF, 0, "GSI0", 2, PrtNode);
  AmlAddPrtEntry (0x001AFFFF, 0, "GSI1", 3, PrtNode);

  AmlAddPrtEntry (0x001BFFFF, 0, "GSI3", 0, PrtNode);
  AmlAddPrtEntry (0x001BFFFF, 0, "GSI0", 1, PrtNode);
  AmlAddPrtEntry (0x001BFFFF, 0, "GSI1", 2, PrtNode);
  AmlAddPrtEntry (0x001BFFFF, 0, "GSI2", 3, PrtNode);

  AmlAddPrtEntry (0x001CFFFF, 0, "GSI0", 0, PrtNode);
  AmlAddPrtEntry (0x001CFFFF, 0, "GSI1", 1, PrtNode);
  AmlAddPrtEntry (0x001CFFFF, 0, "GSI2", 2, PrtNode);
  AmlAddPrtEntry (0x001CFFFF, 0, "GSI3", 3, PrtNode);

  AmlAddPrtEntry (0x001DFFFF, 0, "GSI1", 0, PrtNode);
  AmlAddPrtEntry (0x001DFFFF, 0, "GSI2", 1, PrtNode);
  AmlAddPrtEntry (0x001DFFFF, 0, "GSI3", 2, PrtNode);
  AmlAddPrtEntry (0x001DFFFF, 0, "GSI0", 3, PrtNode);

  AmlAddPrtEntry (0x001EFFFF, 0, "GSI2", 0, PrtNode);
  AmlAddPrtEntry (0x001EFFFF, 0, "GSI3", 1, PrtNode);
  AmlAddPrtEntry (0x001EFFFF, 0, "GSI0", 2, PrtNode);
  AmlAddPrtEntry (0x001EFFFF, 0, "GSI1", 3, PrtNode);

  AmlAddPrtEntry (0x001FFFFF, 0, "GSI3", 0, PrtNode);
  AmlAddPrtEntry (0x001FFFFF, 0, "GSI0", 1, PrtNode);
  AmlAddPrtEntry (0x001FFFFF, 0, "GSI1", 2, PrtNode);
  AmlAddPrtEntry (0x001FFFFF, 0, "GSI2", 3, PrtNode);
}

EFI_STATUS
PciGetProtocolAndResource (
  IN  EFI_HANDLE                         Handle,
  OUT EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    **IoDev,
  OUT EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors
  )
{
  EFI_STATUS  Status;

  //
  // Get inferface from protocol
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **)IoDev
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Call Configuration() to get address space descriptors
  //
  Status = (*IoDev)->Configuration (*IoDev, (VOID **)Descriptors);
  if (Status == EFI_UNSUPPORTED) {
    *Descriptors = NULL;
    return EFI_SUCCESS;
  } else {
    return Status;
  }
}

EFI_STATUS
AddPcieHostBridges (
  AML_OBJECT_NODE_HANDLE  ScopeNode
  )
{
  UINTN       HandleBufSize;
  EFI_HANDLE  *HandleBuf;
  UINTN       HandleCount;
  EFI_STATUS  Status;
  UINTN       Index;

  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    *IoDev;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptors;

  AML_OBJECT_NODE_HANDLE  PciNode;
  AML_OBJECT_NODE_HANDLE  ResNode;
  AML_OBJECT_NODE_HANDLE  RbufRt;
  AML_OBJECT_NODE_HANDLE  ResRt;
  UINT64                  PcieDeviceStatus;
  UINT32                  EisaId;
  CHAR8                   DeviceName[5];

  HandleBufSize = sizeof (EFI_HANDLE);
  HandleBuf     = (EFI_HANDLE *)AllocateZeroPool (HandleBufSize);
  if (HandleBuf == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateHandle (
                  ByProtocol,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  &HandleBufSize,
                  HandleBuf
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    HandleBuf = ReallocatePool (sizeof (EFI_HANDLE), HandleBufSize, HandleBuf);
    if (HandleBuf == NULL) {
      DEBUG ((DEBUG_ERROR, "Failed to allocate memory\n"));
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gBS->LocateHandle (
                    ByProtocol,
                    &gEfiPciRootBridgeIoProtocolGuid,
                    NULL,
                    &HandleBufSize,
                    HandleBuf
                    );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate PciRootBridge\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  HandleCount = HandleBufSize / sizeof (EFI_HANDLE);
  DEBUG ((DEBUG_INFO, "HRW: %a %d handles \n", __FUNCTION__, HandleCount));

  for (Index = 0; Index < HandleCount; Index++) {
    AsciiSPrint (DeviceName, sizeof (DeviceName), "PCI%x", Index);
    AmlCodeGenDevice (DeviceName, ScopeNode, &PciNode);
    Status = AmlGetEisaIdFromString ("PNP0A08", &EisaId);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    AmlCodeGenNameInteger ("_HID", EisaId, PciNode, NULL);
    Status = AmlGetEisaIdFromString ("PNP0A03", &EisaId);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    AmlCodeGenNameInteger ("_CID", EisaId, PciNode, NULL);
    AmlCodeGenNameInteger ("_SEG", 0, PciNode, NULL);
    AmlCodeGenNameInteger ("_CCA", 1, PciNode, NULL);
    AmlCodeGenNameString ("_UID", DeviceName, PciNode, NULL);

    GenPciLinkDevice (PciNode, 0, "GSI0", 0x23);
    GenPciLinkDevice (PciNode, 0, "GSI1", 0x24);
    GenPciLinkDevice (PciNode, 0, "GSI2", 0x25);
    GenPciLinkDevice (PciNode, 0, "GSI3", 0x26);

    GenPrtEntries (PciNode);
    AmlCodeGenNameResourceTemplate ("RBUF", PciNode, &RbufRt);
    AmlCodeGenMethodRetInteger ("_CBA", PcdGet64 (PcdPciExpressBaseAddress), 0, FALSE, 0, PciNode, NULL);

    Status = PciGetProtocolAndResource (
               HandleBuf[Index],
               &IoDev,
               &Descriptors
               );

    while ((*Descriptors).Desc != ACPI_END_TAG_DESCRIPTOR) {
      DEBUG ((
        DEBUG_INFO,
        "HRW: \n"
        "HRW:0 ResType: %d Len: %d Desc: 0x%02x GenFlag: 0x%02x SpecFlag: 0x%02x\n"
        "HRW:1 Granularity: 0x%08x RangeMin-Max: 0x%08x-0x%08x\n"
        "HRW:2 Offset: 0x%08x Len: 0x%08x\n",
        (UINT8)(*Descriptors).ResType,
        (UINT16)(*Descriptors).Len,
        (UINT16)(*Descriptors).Desc,
        (UINT8)(*Descriptors).GenFlag,
        (UINT8)(*Descriptors).SpecificFlag,
        (UINT64)(*Descriptors).AddrSpaceGranularity,
        (UINT64)(*Descriptors).AddrRangeMin,
        (UINT64)(*Descriptors).AddrRangeMax,
        (UINT64)(*Descriptors).AddrTranslationOffset,
        (UINT64)(*Descriptors).AddrLen
        ));

      if (((*Descriptors).ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) &&
          ((*Descriptors).AddrSpaceGranularity == 0x20))
      {
        DEBUG ((DEBUG_INFO, "HRW: Mem\n"));
        AmlCodeGenRdDWordMemory (
          FALSE,
          TRUE,
          TRUE,
          TRUE,
          1,
          TRUE,
          0,
          (*Descriptors).AddrRangeMin,
          (*Descriptors).AddrRangeMax,
          0,
          (*Descriptors).AddrRangeMax - (*Descriptors).AddrRangeMin + 1,
          0,
          NULL,
          0,
          TRUE,
          RbufRt,
          NULL
          );
      }

      if (((*Descriptors).ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) &&
          ((*Descriptors).AddrSpaceGranularity == 0x40))
      {
        DEBUG ((DEBUG_INFO, "HRW: MemAbove4G\n"));
        AmlCodeGenRdQWordMemory (
          FALSE,
          TRUE,
          TRUE,
          TRUE,
          TRUE,
          TRUE,
          0,
          (*Descriptors).AddrRangeMin,
          (*Descriptors).AddrRangeMax,
          0,
          (*Descriptors).AddrRangeMax - (*Descriptors).AddrRangeMin + 1,
          0,
          NULL,
          0,
          TRUE,
          RbufRt,
          NULL
          );
      }

      if ((*Descriptors).ResType == ACPI_ADDRESS_SPACE_TYPE_IO) {
        DEBUG ((DEBUG_INFO, "HRW: Io\n"));
        AmlCodeGenRdDWordIo (
          FALSE,
          TRUE,
          TRUE,
          TRUE,
          3,
          0,
          (*Descriptors).AddrRangeMin,
          (*Descriptors).AddrRangeMax,
          0,
          (*Descriptors).AddrRangeMax - (*Descriptors).AddrRangeMin + 1,
          0,
          NULL,
          FALSE,
          TRUE,
          RbufRt,
          NULL
          );
      }

      if ((*Descriptors).ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) {
        DEBUG ((DEBUG_INFO, "HRW: Bus\n"));
        AmlCodeGenNameInteger ("_BBN", (*Descriptors).AddrRangeMin, PciNode, NULL);
        AmlCodeGenRdWordBusNumber (
          FALSE,
          TRUE,
          TRUE,
          TRUE,
          0,
          (*Descriptors).AddrRangeMin,
          (*Descriptors).AddrRangeMax,
          0,
          (*Descriptors).AddrRangeMax - (*Descriptors).AddrRangeMin + 1,
          0,
          NULL,
          RbufRt,
          NULL
          );
      }

      (Descriptors)++;
    }

    DEBUG ((DEBUG_INFO, "\n"));

    AmlCodeGenMethodRetNameString ("_CRS", "RBUF", 0, TRUE, 0, PciNode, NULL);
    PcieDeviceStatus = 0xF; // STATUS_PRESENT | STATUS_ENABLED | STATUS_SHOWN_IN_UI | STATUS_FUNCTIONING;
    AmlCodeGenMethodRetInteger ("_STA", PcieDeviceStatus, 0, TRUE, 0, PciNode, NULL);

    AmlCodeGenNameInteger ("SUPP", 0, PciNode, NULL);
    AmlCodeGenNameInteger ("CTRL", 0, PciNode, NULL);

    AddOscMethod (PciNode);

    if (Index == 0) {
      // first node
      AmlCodeGenDevice ("RES0", PciNode, &ResNode);
      Status = AmlGetEisaIdFromString ("PNP0C02", &EisaId);
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        return Status;
      }

      AmlCodeGenNameInteger ("_HID", EisaId, ResNode, NULL);

      AmlCodeGenNameResourceTemplate ("_CRS", ResNode, &ResRt);
      AmlCodeGenRdQWordMemory (
        FALSE,
        TRUE,
        TRUE,
        TRUE,
        FALSE,
        TRUE,
        0,
        PcdGet64 (PcdPciExpressBaseAddress), // Range Minimum
        PcdGet64 (PcdPciExpressBarLimit),    // Range Maximum
        0x0000000000000000,                       // Translation Offset
        PcdGet64 (PcdPciExpressBarSize),     // Length
        //   Bridge->Mem.Base,
        //   Bridge->Mem.Limit,
        //   FixedPcdGet32 (PcdPciMmio32Translation),
        //   Bridge->Mem.Limit - Bridge->Mem.Base + 1,
        0,
        NULL /* ResourceSource */,
        0 /* MemoryRangeType */,
        TRUE /* IsTypeStatic */,
        ResRt,
        NULL
        );
    }
  }

  return EFI_SUCCESS;
}
