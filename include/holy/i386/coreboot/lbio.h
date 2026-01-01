/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _holy_MACHINE_LBIO_HEADER
#define _holy_MACHINE_LBIO_HEADER      1

struct holy_linuxbios_table_header
{
  holy_uint8_t signature[4];
  holy_uint32_t header_size;
  holy_uint32_t header_checksum;
  holy_uint32_t table_size;
  holy_uint32_t table_checksum;
  holy_uint32_t table_entries;
};
typedef struct holy_linuxbios_table_header *holy_linuxbios_table_header_t;

struct holy_linuxbios_timestamp_entry
{
  holy_uint32_t id;
  holy_uint64_t tsc;
} holy_PACKED;

struct holy_linuxbios_timestamp_table
{
  holy_uint64_t base_tsc;
  holy_uint32_t capacity;
  holy_uint32_t used;
  struct holy_linuxbios_timestamp_entry entries[0];
} holy_PACKED;

struct holy_linuxbios_mainboard
{
  holy_uint8_t vendor;
  holy_uint8_t part_number;
  char strings[0];
};

struct holy_linuxbios_table_item
{
  holy_uint32_t tag;
  holy_uint32_t size;
};
typedef struct holy_linuxbios_table_item *holy_linuxbios_table_item_t;

enum
  {
    holy_LINUXBIOS_MEMBER_UNUSED      = 0x00,
    holy_LINUXBIOS_MEMBER_MEMORY      = 0x01,
    holy_LINUXBIOS_MEMBER_MAINBOARD   = 0x03,
    holy_LINUXBIOS_MEMBER_CONSOLE     = 0x10,
    holy_LINUXBIOS_MEMBER_LINK        = 0x11,
    holy_LINUXBIOS_MEMBER_FRAMEBUFFER = 0x12,
    holy_LINUXBIOS_MEMBER_TIMESTAMPS  = 0x16,
    holy_LINUXBIOS_MEMBER_CBMEMC      = 0x17
  };

struct holy_linuxbios_table_framebuffer {
  holy_uint64_t lfb;
  holy_uint32_t width;
  holy_uint32_t height;
  holy_uint32_t pitch;
  holy_uint8_t bpp;

  holy_uint8_t red_field_pos;
  holy_uint8_t red_mask_size;
  holy_uint8_t green_field_pos;
  holy_uint8_t green_mask_size;
  holy_uint8_t blue_field_pos;
  holy_uint8_t blue_mask_size;
  holy_uint8_t reserved_field_pos;
  holy_uint8_t reserved_mask_size;
} holy_PACKED;

struct holy_linuxbios_mem_region
{
  holy_uint64_t addr;
  holy_uint64_t size;
#define holy_MACHINE_MEMORY_AVAILABLE		1
  holy_uint32_t type;
} holy_PACKED;
typedef struct holy_linuxbios_mem_region *mem_region_t;

holy_err_t
EXPORT_FUNC(holy_linuxbios_table_iterate) (int (*hook) (holy_linuxbios_table_item_t,
					   void *),
					   void *hook_data);

#endif
