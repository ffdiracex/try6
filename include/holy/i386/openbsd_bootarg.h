/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_OPENBSD_BOOTARG_CPU_HEADER
#define holy_OPENBSD_BOOTARG_CPU_HEADER	1

#define OPENBSD_BOOTARG_APIVER	(OPENBSD_BAPIV_VECTOR | \
                                 OPENBSD_BAPIV_ENV | \
                                 OPENBSD_BAPIV_BMEMMAP)

#define OPENBSD_BAPIV_ANCIENT	0x0  /* MD old i386 bootblocks */
#define OPENBSD_BAPIV_VARS	0x1  /* MD structure w/ add info passed */
#define OPENBSD_BAPIV_VECTOR	0x2  /* MI vector of MD structures passed */
#define OPENBSD_BAPIV_ENV	0x4  /* MI environment vars vector */
#define OPENBSD_BAPIV_BMEMMAP	0x8  /* MI memory map passed is in bytes */

#define OPENBSD_BOOTARG_ENV	0x1000
#define OPENBSD_BOOTARG_END	-1

#define	OPENBSD_BOOTARG_MMAP	0
#define	OPENBSD_BOOTARG_PCIBIOS 4
#define	OPENBSD_BOOTARG_CONSOLE 5

struct holy_openbsd_bootargs
{
  holy_uint32_t ba_type;
  holy_uint32_t ba_size;
  holy_uint32_t ba_next;
} holy_PACKED;

struct holy_openbsd_bootarg_console
{
  holy_uint32_t device;
  holy_uint32_t speed;
  holy_uint32_t addr;
  holy_uint32_t frequency;
};

struct holy_openbsd_bootarg_pcibios
{
  holy_uint32_t characteristics;
  holy_uint32_t revision;
  holy_uint32_t pm_entry;
  holy_uint32_t last_bus;
};

#define holy_OPENBSD_COM_MAJOR 8
#define holy_OPENBSD_VGA_MAJOR 12

#endif
