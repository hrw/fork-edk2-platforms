/** @file
  PCI Host Bridge Library instance for pci-ecam-generic DT nodes

  Copyright (c) 2019, Linaro Ltd. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <limits.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PciLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <PiDxe.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mEfiPciRootBridgeDevicePath = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID(0x0A03),
    0
  },

  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16 *mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

EFI_STATUS
EFIAPI
PciHostBridgeUtilityInitRootBridge (
  IN UINTN             RootBusNumber,
  OUT PCI_ROOT_BRIDGE  *RootBus
  )
{
  EFI_PCI_ROOT_BRIDGE_DEVICE_PATH  *DevicePath;
  UINTN                            MaxSubBusNumber = 255;

  DevicePath = AllocateCopyPool (
                 sizeof mEfiPciRootBridgeDevicePath,
                 &mEfiPciRootBridgeDevicePath
                 );
  if (DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }

  DevicePath->AcpiDevicePath.UID = RootBusNumber;

  RootBus->Segment               = 0;
  RootBus->Supports              = 0;
  RootBus->Attributes            = 0;
  RootBus->DmaAbove4G            = TRUE;
  RootBus->AllocationAttributes  = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM | EFI_PCI_HOST_BRIDGE_MEM64_DECODE, /* as Mmio64Size > 0 */
  RootBus->Bus.Base              = RootBusNumber;
  RootBus->Bus.Limit             = MaxSubBusNumber;
  RootBus->Io.Base               = PcdGet64 (PcdPciIoBase);
  RootBus->Io.Limit              = PcdGet64 (PcdPciIoBase) + PcdGet64 (PcdPciIoSize) - 1;
  RootBus->Mem.Base              = PcdGet32 (PcdPciMmio32Base);
  RootBus->Mem.Limit             = PcdGet32 (PcdPciMmio32Base) + PcdGet32 (PcdPciMmio32Size) - 1;
  RootBus->MemAbove4G.Base       = PcdGet64 (PcdPciMmio64Base);
  RootBus->MemAbove4G.Limit      = PcdGet64 (PcdPciMmio64Base) + PcdGet64 (PcdPciMmio64Size) - 1;
  RootBus->PMem.Base             = MAX_UINT64;
  RootBus->PMem.Limit            = 0;
  RootBus->PMemAbove4G.Base      = MAX_UINT64;
  RootBus->PMemAbove4G.Limit     = 0;
  RootBus->NoExtendedConfigSpace = FALSE;
  RootBus->DevicePath            = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;

  return EFI_SUCCESS;
}

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN  *Count
  )
{
  PCI_ROOT_BRIDGE  *Bridges;
  int              BusId, BusMin = 0, BusMax = 255, Index = 0;
  int              AvailableBusses[255] = { INT_MAX }; // INT_MAX as "there is no bus"

  *Count = 0;

  //
  // Scan all root buses. If function 0 of any device on a bus returns a
  // VendorId register value different from all-bits-one, then that bus is
  // alive.
  //
  for (BusId = BusMin; BusId <= BusMax; ++BusId) {
    UINTN  Device;

    for (Device = 0; Device <= PCI_MAX_DEVICE; ++Device) {
      if (PciRead16 (
            PCI_LIB_ADDRESS (
              BusId,
              Device,
              0,
              PCI_VENDOR_ID_OFFSET
              )
            ) != MAX_UINT16)
      {
        break;
      }
    }

    if (Device <= PCI_MAX_DEVICE) {
      DEBUG ((DEBUG_ERROR, "%a: found bus: 0x%02x\n", __func__, BusId));
      AvailableBusses[Index++] = BusId;
      *Count                  += 1;
    }
  }

  //
  // Allocate the "main" root bridge, and any extra root bridges.
  //
  Bridges = AllocateZeroPool (*Count * sizeof *Bridges);
  if (Bridges == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, EFI_OUT_OF_RESOURCES));
    return NULL;
  }

  for (Index = 0; Index < *Count; Index++) {
    if (AvailableBusses[Index] == INT_MAX) {
      break;
    }

    PciHostBridgeUtilityInitRootBridge (AvailableBusses[Index], &Bridges[Index]);

    // limit previous RootBridge bus range
    if (Index > 0) {
      Bridges[Index - 1].Bus.Limit = AvailableBusses[Index] - 1;
    }
  }

  return Bridges;
}

/**
  Free the root bridge instances array returned from
  PciHostBridgeGetRootBridges().

  @param Bridges The root bridge instances array.
  @param Count   The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE  *Bridges,
  UINTN            Count
  )
{
  FreePool (Bridges);
}

/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE                        HostBridgeHandle,
  VOID                              *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  UINTN                             RootBridgeIndex;
  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
               ARRAY_SIZE(mPciHostBridgeLibAcpiAddressSpaceTypeStr));
      DEBUG ((DEBUG_ERROR, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
              mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
              Descriptor->AddrLen, Descriptor->AddrRangeMax
              ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((DEBUG_ERROR, "     Granularity/SpecificFlag = %ld / %02x%s\n",
                Descriptor->AddrSpaceGranularity, Descriptor->SpecificFlag,
                ((Descriptor->SpecificFlag &
                  EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE
                  ) != 0) ? L" (Prefetchable)" : L""
                ));
      }
    }
    //
    // Skip the END descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                   (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                   );
  }
}
