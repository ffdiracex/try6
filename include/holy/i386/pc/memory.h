/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_MACHINE_HEADER
#define holy_MEMORY_MACHINE_HEADER	1

#include <holy/symbol.h>
#ifndef ASM_FILE
#include <holy/types.h>
#include <holy/err.h>
#include <holy/memory.h>
#endif

#include <holy/i386/memory.h>
#include <holy/i386/memory_raw.h>

#include <holy/offsets.h>

/* The area where holy is decompressed at early startup.  */
#define holy_MEMORY_MACHINE_DECOMPRESSION_ADDR	0x100000

/* The address of a partition table passed to another boot loader.  */
#define holy_MEMORY_MACHINE_PART_TABLE_ADDR	0x7be

/* The address where another boot loader is loaded.  */
#define holy_MEMORY_MACHINE_BOOT_LOADER_ADDR	0x7c00

#define holy_MEMORY_MACHINE_BIOS_DATA_AREA_ADDR	0x400

#ifndef ASM_FILE

/* See http://heim.ifi.uio.no/~stanisls/helppc/bios_data_area.html for a
   description of the BIOS Data Area layout.  */
struct holy_machine_bios_data_area
{
  holy_uint8_t unused1[0x17];
  holy_uint8_t keyboard_flag_lower; /* 0x17 */
  holy_uint8_t unused2[0xf0 - 0x18];
};

holy_err_t holy_machine_mmap_register (holy_uint64_t start, holy_uint64_t size,
				       int type, int handle);
holy_err_t holy_machine_mmap_unregister (int handle);

#endif

#endif /* ! holy_MEMORY_MACHINE_HEADER */
