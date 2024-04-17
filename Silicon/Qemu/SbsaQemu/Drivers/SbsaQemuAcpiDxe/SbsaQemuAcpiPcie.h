/** @file
*  This file is an ACPI driver for the Qemu SBSA platform.
*
*  Copyright (c) 2024, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/AcpiAml.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <IndustryStandard/SbsaQemuAcpi.h>
#include <IndustryStandard/SbsaQemuPlatformVersion.h>

#include <Library/AcpiHelperLib.h>
#include <Library/AcpiLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PciLib.h>
#include <Library/PrintLib.h>
#include <Library/HardwareInfoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/PciRootBridgeIo.h>

#include "SbsaQemuAcpiDxe.h"

#pragma pack(1)

/* AML bytecode generated from XsdtTemplate.asl */
extern CHAR8  xsdttemplate_aml_code[];

/** Adds the _OSC method to the PCIe node.

  @param PciNode  PCIe device node.

  @return EFI_SUCCESS on success, or an error code.

**/
EFI_STATUS
AddOscMethod (
  IN  OUT   AML_OBJECT_NODE_HANDLE  PciNode
  );

/** Creates a PCI Interrupt Link Device.

  @param PciDeviceHandle  PCIe device node.
  @param Uid              UID of the Link Device.
  @param LinkName         Name of the Device.
  @param Irq              IRQ number.

**/
VOID
GenPciLinkDevice (
  AML_OBJECT_NODE_HANDLE  PciDeviceHandle,
  UINT32                  Uid,
  CONST CHAR8             *LinkName,
  UINT32                  Irq
  );

/** Creates a _PRT package.

  @param PciDeviceHandle  PCIe device node.

**/
VOID
GenPrtEntries (
  AML_OBJECT_NODE_HANDLE  PciDeviceHandle
  );

/** Creates a PCIe device.

  @param ScopeNode   _SB scope node.
  @param Segment     Segment index.
  @param SegmentCfg  PCIe segment information.

  @return EFI_SUCCESS on success, or an error code.

**/
EFI_STATUS
GenPciDevice (
  AML_OBJECT_NODE_HANDLE  ScopeNode,
  PCI_ROOT_BRIDGE        *Bridge
  );

EFI_STATUS
PciGetProtocolAndResource (
  IN  EFI_HANDLE                         Handle,
  OUT EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    **IoDev,
  OUT EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  **Descriptors
  );

EFI_STATUS
AddPcieHostBridges (
  AML_OBJECT_NODE_HANDLE  ScopeNode
  );
