/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_VESA_MODE_TABLE_HEADER
#define holy_VESA_MODE_TABLE_HEADER 1

#include <holy/types.h>

#define holy_VESA_MODE_TABLE_START 0x300
#define holy_VESA_MODE_TABLE_END 0x373

struct holy_vesa_mode_table_entry {
  holy_uint16_t width;
  holy_uint16_t height;
  holy_uint8_t depth;
};

extern struct holy_vesa_mode_table_entry
holy_vesa_mode_table[holy_VESA_MODE_TABLE_END
		     - holy_VESA_MODE_TABLE_START + 1];

#endif
