/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/acpi.h>
#include <holy/misc.h>

struct holy_acpi_rsdp_v10 *
holy_machine_acpi_get_rsdpv1 (void)
{
  int ebda_len;
  holy_uint8_t *ebda, *ptr;

  holy_dprintf ("acpi", "Looking for RSDP. Scanning EBDA\n");
  ebda = (holy_uint8_t *) ((* ((holy_uint16_t *) 0x40e)) << 4);
  ebda_len = * (holy_uint16_t *) ebda;
  if (! ebda_len) /* FIXME do we really need this check? */
    goto scan_bios;
  for (ptr = ebda; ptr < ebda + 0x400; ptr += 16)
    if (holy_memcmp (ptr, holy_RSDP_SIGNATURE, holy_RSDP_SIGNATURE_SIZE) == 0
	&& holy_byte_checksum (ptr, sizeof (struct holy_acpi_rsdp_v10)) == 0
	&& ((struct holy_acpi_rsdp_v10 *) ptr)->revision == 0)
      return (struct holy_acpi_rsdp_v10 *) ptr;

scan_bios:
  holy_dprintf ("acpi", "Looking for RSDP. Scanning BIOS\n");
  for (ptr = (holy_uint8_t *) 0xe0000; ptr < (holy_uint8_t *) 0x100000;
       ptr += 16)
    if (holy_memcmp (ptr, holy_RSDP_SIGNATURE, holy_RSDP_SIGNATURE_SIZE) == 0
	&& holy_byte_checksum (ptr, sizeof (struct holy_acpi_rsdp_v10)) == 0
	&& ((struct holy_acpi_rsdp_v10 *) ptr)->revision == 0)
      return (struct holy_acpi_rsdp_v10 *) ptr;
  return 0;
}

struct holy_acpi_rsdp_v20 *
holy_machine_acpi_get_rsdpv2 (void)
{
  int ebda_len;
  holy_uint8_t *ebda, *ptr;

  holy_dprintf ("acpi", "Looking for RSDP. Scanning EBDA\n");
  ebda = (holy_uint8_t *) ((* ((holy_uint16_t *) 0x40e)) << 4);
  ebda_len = * (holy_uint16_t *) ebda;
  if (! ebda_len) /* FIXME do we really need this check? */
    goto scan_bios;
  for (ptr = ebda; ptr < ebda + 0x400; ptr += 16)
    if (holy_memcmp (ptr, holy_RSDP_SIGNATURE, holy_RSDP_SIGNATURE_SIZE) == 0
	&& holy_byte_checksum (ptr, sizeof (struct holy_acpi_rsdp_v10)) == 0
	&& ((struct holy_acpi_rsdp_v10 *) ptr)->revision != 0
	&& ((struct holy_acpi_rsdp_v20 *) ptr)->length < 1024
	&& holy_byte_checksum (ptr, ((struct holy_acpi_rsdp_v20 *) ptr)->length)
	== 0)
      return (struct holy_acpi_rsdp_v20 *) ptr;

scan_bios:
  holy_dprintf ("acpi", "Looking for RSDP. Scanning BIOS\n");
  for (ptr = (holy_uint8_t *) 0xe0000; ptr < (holy_uint8_t *) 0x100000;
       ptr += 16)
    if (holy_memcmp (ptr, holy_RSDP_SIGNATURE, holy_RSDP_SIGNATURE_SIZE) == 0
	&& holy_byte_checksum (ptr, sizeof (struct holy_acpi_rsdp_v10)) == 0
	&& ((struct holy_acpi_rsdp_v10 *) ptr)->revision != 0
	&& ((struct holy_acpi_rsdp_v20 *) ptr)->length < 1024
	&& holy_byte_checksum (ptr, ((struct holy_acpi_rsdp_v20 *) ptr)->length)
	== 0)
      return (struct holy_acpi_rsdp_v20 *) ptr;
  return 0;
}
