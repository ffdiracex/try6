/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SMBIOS_HEADER
#define holy_SMBIOS_HEADER	1

#include <holy/types.h>

#define holy_SMBIOS_TYPE_END_OF_TABLE ((holy_uint8_t)127)

struct holy_smbios_ieps
{
  holy_uint8_t anchor[5]; /* "_DMI_" */
  holy_uint8_t checksum;
  holy_uint16_t table_length;
  holy_uint32_t table_address;
  holy_uint16_t structures;
  holy_uint8_t revision;
} holy_PACKED;

struct holy_smbios_eps
{
  holy_uint8_t anchor[4]; /* "_SM_" */
  holy_uint8_t checksum;
  holy_uint8_t length; /* 0x1f */
  holy_uint8_t version_major;
  holy_uint8_t version_minor;
  holy_uint16_t maximum_structure_size;
  holy_uint8_t revision;
  holy_uint8_t formatted[5];
  struct holy_smbios_ieps intermediate;
} holy_PACKED;

struct holy_smbios_eps3
{
  holy_uint8_t anchor[5]; /* "_SM3_" */
  holy_uint8_t checksum;
  holy_uint8_t length; /* 0x18 */
  holy_uint8_t version_major;
  holy_uint8_t version_minor;
  holy_uint8_t docrev;
  holy_uint8_t revision;
  holy_uint8_t reserved;
  holy_uint32_t maximum_table_length;
  holy_uint64_t table_address;
} holy_PACKED;

struct holy_smbios_eps *holy_smbios_get_eps (void);
struct holy_smbios_eps3 *holy_smbios_get_eps3 (void);
struct holy_smbios_eps *holy_machine_smbios_get_eps (void);
struct holy_smbios_eps3 *holy_machine_smbios_get_eps3 (void);

#endif /* ! holy_SMBIOS_HEADER */
