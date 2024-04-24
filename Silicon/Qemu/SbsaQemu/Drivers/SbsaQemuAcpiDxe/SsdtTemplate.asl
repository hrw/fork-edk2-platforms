/** @file
*  Differentiated System Description Table Fields (DSDT).
*
*  Copyright (c) 2024, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi63.h>
#include <IndustryStandard/SbsaQemuAcpi.h>

#define LINK_DEVICE(Uid, LinkName, Irq)                                        \
        Device (LinkName) {                                                    \
            Name (_HID, EISAID("PNP0C0F"))                                     \
            Name (_UID, Uid)                                                   \
            Name (_PRS, ResourceTemplate() {                                   \
                Interrupt (ResourceProducer, Level, ActiveHigh, Exclusive) { Irq } \
            })                                                                 \
            Method (_CRS, 0) { Return (_PRS) }                                 \
            Method (_SRS, 1) { }                                               \
            Method (_DIS) { }                                                  \
        }

#define PRT_ENTRY(Address, Pin, Link)                                          \
        Package (4) {                                                          \
            Address, Pin, Link, Zero                                           \
          }

DefinitionBlock ("Dsdt.aml", "DSDT",
                 EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_REVISION,
                 "LINARO", "SBSAQEMU", FixedPcdGet32 (PcdAcpiDefaultOemRevision)) {
      /*
       * See [1] 6.2.10, [2] 4.5
       */
      Name (SUPP, Zero) // PCI _OSC Support Field value
      Name (CTRL, Zero) // PCI _OSC Control Field value
      Method (_OSC, 4, Serialized) {
        //
        // OS Control Handoff
        //

        // Check for proper UUID
        If (Arg0 == ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")) {
          // Create DWord-adressable fields from the Capabilities Buffer
          CreateDWordField (Arg3,0,CDW1)
          CreateDWordField (Arg3,4,CDW2)
          CreateDWordField (Arg3,8,CDW3)

          // Save Capabilities DWord2 & 3
          Store (CDW2,SUPP)
          Store (CDW3,CTRL)

          // Only allow native hot plug control if OS supports:
          // * ASPM
          // * Clock PM
          // * MSI/MSI-X
          If ((SUPP & 0x16) != 0x16) {
            CTRL |= 0x1E // Mask bit 0 (and undefined bits)
          }

          // Always allow native PME, AER (no dependencies)

          // Never allow SHPC (no SHPC controller in this system)
          CTRL &= 0x1D

          If (Arg1 != One) {        // Unknown revision
            CDW1 |= 0x08
          }

          If (CDW3 != CTRL) {        // Capabilities bits were masked
            CDW1 |= 0x10
          }

          // Update DWORD3 in the buffer
          Store (CTRL,CDW3)
          Return (Arg3)
        } Else {
          CDW1 |= 4 // Unrecognized UUID
          Return (Arg3)
        }
      } // End _OSC
}
