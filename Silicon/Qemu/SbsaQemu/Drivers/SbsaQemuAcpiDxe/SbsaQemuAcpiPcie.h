/** @file
*  This file is an ACPI driver for the Qemu SBSA platform.
*
*  Copyright (c) 2024, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef SBSAQEMU_ACPI_PCIE_H
#define SBSAQEMU_ACPI_PCIE_H

#pragma pack(1)

/* AML bytecode generated from SsdtTemplate.asl */
extern CHAR8  ssdttemplate_aml_code[];

EFI_STATUS
AddPcieHostBridges (
  AML_OBJECT_NODE_HANDLE  ScopeNode
  );

#endif
