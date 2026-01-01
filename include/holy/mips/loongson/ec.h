/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EC_MACHINE_HEADER
#define holy_EC_MACHINE_HEADER	1

#include <holy/types.h>
#include <holy/cpu/io.h>
#include <holy/pci.h>

#define holy_MACHINE_EC_MAGIC_PORT1 0x381
#define holy_MACHINE_EC_MAGIC_PORT2 0x382
#define holy_MACHINE_EC_DATA_PORT 0x383

#define holy_MACHINE_EC_MAGIC_VAL1 0xf4
#define holy_MACHINE_EC_MAGIC_VAL2 0xec

#define holy_MACHINE_EC_COMMAND_REBOOT 1

static inline void
holy_write_ec (holy_uint8_t value)
{
  holy_outb (holy_MACHINE_EC_MAGIC_VAL1,
	     holy_MACHINE_PCI_IO_BASE + holy_MACHINE_EC_MAGIC_PORT1);
  holy_outb (holy_MACHINE_EC_MAGIC_VAL2,
	     holy_MACHINE_PCI_IO_BASE + holy_MACHINE_EC_MAGIC_PORT2);
  holy_outb (value, holy_MACHINE_PCI_IO_BASE + holy_MACHINE_EC_DATA_PORT);
}

#endif
