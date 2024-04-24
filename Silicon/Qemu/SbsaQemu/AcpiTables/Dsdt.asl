/** @file
*  Differentiated System Description Table Fields (DSDT).
*
*  Copyright (c) 2020, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi60.h>
#include <IndustryStandard/SbsaQemuAcpi.h>

#define LINK_DEVICE(Uid, LinkName, Irq)                                        \
        Device (LinkName) {                                                    \
            Name (_HID, EISAID("PNP0C0F"))                                     \
            Name (_UID, Uid)                                                   \
            Name (_PRS, ResourceTemplate() {                                   \
                Interrupt (ResourceProducer, Level, ActiveHigh, Exclusive) { Irq } \
            })                                                                 \
            Method (_STA) {                                                    \
              Return (0xF)                                                     \
            }                                                                  \
            Method (_CRS, 0) { Return (_PRS) }                                 \
            Method (_SRS, 1) { }                                               \
            Method (_DIS) { }                                                  \
        }

#define PRT_ENTRY(Address, Pin, Link)                                          \
        Package (4) {                                                          \
            Address, Pin, Link, Zero                                           \
          }

DefinitionBlock ("DsdtTable.aml", "DSDT",
                 EFI_ACPI_6_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_REVISION,
                 "LINARO", "SBSAQEMU", FixedPcdGet32 (PcdAcpiDefaultOemRevision)) {
  Scope (_SB) {
    // UART PL011
    Device (COM0) {
      Name (_HID, "ARMH0011")
      Name (_UID, Zero)
      Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite,
                       FixedPcdGet32 (PcdSerialRegisterBase),
                       0x00001000)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 33 }
      })
      Method (_STA) {
        Return (0xF)
      }
    }

    // AHCI Host Controller
    Device (AHC0) {
      Name (_HID, "LNRO001E")
      Name (_CLS, Package (3) {
        0x01,
        0x06,
        0x01,
      })
      Name (_CCA, 1)
      Name (_CRS, ResourceTemplate() {
        Memory32Fixed (ReadWrite,
                       FixedPcdGet32 (PcdPlatformAhciBase),
                       FixedPcdGet32 (PcdPlatformAhciSize))
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 42 }
      })
      Method (_STA) {
        Return (0xF)
      }
    }

    // USB XHCI Host Controller
    Device (USB0) {
        Name (_HID, "PNP0D10")      // _HID: Hardware ID
        Name (_UID, 0x00)            // _UID: Unique ID
        Name (_CCA, 0x01)            // _CCA: Cache Coherency Attribute
        Name (XHCI, 0xF)            // will be set using AcpiLib
        Method (_STA) {
          Return (XHCI)
        }
        Name (_CRS, ResourceTemplate() {
            Memory32Fixed (ReadWrite,
                           FixedPcdGet32 (PcdPlatformXhciBase),
                           FixedPcdGet32 (PcdPlatformXhciSize))
            Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) { 43 }
        })

        // Root Hub
        Device (RHUB) {
            Name (_ADR, 0x00000000)  // Address of Root Hub should be 0 as per ACPI 5.0 spec
            Method (_STA) {
              Return (0xF)
            }

            // Ports connected to Root Hub
            Device (HUB1) {
                Name (_ADR, 0x00000001)
                Name (_UPC, Package() {
                    0x00,       // Port is NOT connectable
                    0xFF,       // Don't care
                    0x00000000, // Reserved 0 must be zero
                    0x00000000  // Reserved 1 must be zero
                })
                Method (_STA) {
                  Return (0xF)
                }

                Device (PRT1) {
                    Name (_ADR, 0x00000001)
                    Name (_UPC, Package() {
                        0xFF,        // Port is connectable
                        0x00,        // Port connector is A
                        0x00000000,
                        0x00000000
                    })
                    Name (_PLD, Package() {
                        Buffer(0x10) {
                            0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                        }
                    })
                    Method (_STA) {
                      Return (0xF)
                    }
                } // USB0_RHUB_HUB1_PRT1
                Device (PRT2) {
                    Name (_ADR, 0x00000002)
                    Name (_UPC, Package() {
                        0xFF,        // Port is connectable
                        0x00,        // Port connector is A
                        0x00000000,
                        0x00000000
                    })
                    Name (_PLD, Package() {
                        Buffer(0x10) {
                            0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                        }
                    })
                    Method (_STA) {
                      Return (0xF)
                    }
                } // USB0_RHUB_HUB1_PRT2

                Device (PRT3) {
                    Name (_ADR, 0x00000003)
                    Name (_UPC, Package() {
                        0xFF,        // Port is connectable
                        0x09,        // Type C connector - USB2 and SS with Switch
                        0x00000000,
                        0x00000000
                    })
                    Name (_PLD, Package() {
                        Buffer (0x10) {
                            0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                        }
                    })
                    Method (_STA) {
                      Return (0xF)
                    }
                } // USB0_RHUB_HUB1_PRT3

                Device (PRT4) {
                    Name (_ADR, 0x00000004)
                    Name (_UPC, Package() {
                        0xFF,        // Port is connectable
                        0x09,        // Type C connector - USB2 and SS with Switch
                        0x00000000,
                        0x00000000
                    })
                    Name (_PLD, Package() {
                        Buffer (0x10){
                            0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x31, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
                        }
                    })
                    Method (_STA) {
                      Return (0xF)
                    }
                } // USB0_RHUB_HUB1_PRT4
            } // USB0_RHUB_HUB1
        } // USB0_RHUB
    } // USB0
  } // Scope (_SB)
}
