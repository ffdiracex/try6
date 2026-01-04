/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/acpi.h>
#include <holy/smbios.h>
#include <holy/misc.h>

struct holy_smbios_eps *
holy_machine_smbios_get_eps (void)
{
  holy_uint8_t *ptr;

  holy_dprintf ("smbios", "Looking for SMBIOS EPS. Scanning BIOS\n");
  for (ptr = (holy_uint8_t *) 0xf0000; ptr < (holy_uint8_t *) 0x100000;
       ptr += 16)
    if (holy_memcmp (ptr, "_SM_", 4) == 0
	&& holy_byte_checksum (ptr, sizeof (struct holy_smbios_eps)) == 0)
      return (struct holy_smbios_eps *) ptr;
  return 0;
}

struct holy_smbios_eps3 *
holy_machine_smbios_get_eps3 (void)
{
  holy_uint8_t *ptr;

  holy_dprintf ("smbios", "Looking for SMBIOS3 EPS. Scanning BIOS\n");
  for (ptr = (holy_uint8_t *) 0xf0000; ptr < (holy_uint8_t *) 0x100000;
       ptr += 16)
    if (holy_memcmp (ptr, "_SM3_", 5) == 0
	&& holy_byte_checksum (ptr, sizeof (struct holy_smbios_eps3)) == 0)
      return (struct holy_smbios_eps3 *) ptr;
  return 0;
}
