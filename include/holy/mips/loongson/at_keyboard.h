/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MACHINE_AT_KEYBOARD_HEADER
#define holy_MACHINE_AT_KEYBOARD_HEADER	1

#include <holy/pci.h>

#define KEYBOARD_REG_DATA	(holy_MACHINE_PCI_IO_BASE | 0x60)
#define KEYBOARD_REG_STATUS	(holy_MACHINE_PCI_IO_BASE | 0x64)

#endif
