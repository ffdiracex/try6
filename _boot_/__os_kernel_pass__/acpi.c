/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/time.h>
#include <holy/misc.h>
#include <holy/acpi.h>

/* Simple checksum by summing all bytes. Used by ACPI and SMBIOS. */
holy_uint8_t
holy_byte_checksum (void *base, holy_size_t size)
{
  holy_uint8_t *ptr;
  holy_uint8_t ret = 0;
  for (ptr = (holy_uint8_t *) base; ptr < ((holy_uint8_t *) base) + size;
       ptr++)
    ret += *ptr;
  return ret;
}

static void *
holy_acpi_rsdt_find_table (struct holy_acpi_table_header *rsdt, const char *sig)
{
  holy_size_t s;
  holy_unaligned_uint32_t *ptr;

  if (!rsdt)
    return 0;

  if (holy_memcmp (rsdt->signature, "RSDT", 4) != 0)
    return 0;

  ptr = (holy_unaligned_uint32_t *) (rsdt + 1);
  s = (rsdt->length - sizeof (*rsdt)) / sizeof (holy_uint32_t);
  for (; s; s--, ptr++)
    {
      struct holy_acpi_table_header *tbl;
      tbl = (struct holy_acpi_table_header *) (holy_addr_t) ptr->val;
      if (holy_memcmp (tbl->signature, sig, 4) == 0)
	return tbl;
    }
  return 0;
}

static void *
holy_acpi_xsdt_find_table (struct holy_acpi_table_header *xsdt, const char *sig)
{
  holy_size_t s;
  holy_unaligned_uint64_t *ptr;

  if (!xsdt)
    return 0;

  if (holy_memcmp (xsdt->signature, "XSDT", 4) != 0)
    return 0;

  ptr = (holy_unaligned_uint64_t *) (xsdt + 1);
  s = (xsdt->length - sizeof (*xsdt)) / sizeof (holy_uint32_t);
  for (; s; s--, ptr++)
    {
      struct holy_acpi_table_header *tbl;
#if holy_CPU_SIZEOF_VOID_P != 8
      if (ptr->val >> 32)
	continue;
#endif
      tbl = (struct holy_acpi_table_header *) (holy_addr_t) ptr->val;
      if (holy_memcmp (tbl->signature, sig, 4) == 0)
	return tbl;
    }
  return 0;
}

struct holy_acpi_fadt *
holy_acpi_find_fadt (void)
{
  struct holy_acpi_fadt *fadt = 0;
  struct holy_acpi_rsdp_v10 *rsdpv1;
  struct holy_acpi_rsdp_v20 *rsdpv2;
  rsdpv1 = holy_machine_acpi_get_rsdpv1 ();
  if (rsdpv1)
    fadt = holy_acpi_rsdt_find_table ((struct holy_acpi_table_header *)
				      (holy_addr_t) rsdpv1->rsdt_addr,
				      holy_ACPI_FADT_SIGNATURE);
  if (fadt)
    return fadt;
  rsdpv2 = holy_machine_acpi_get_rsdpv2 ();
  if (rsdpv2)
    fadt = holy_acpi_rsdt_find_table ((struct holy_acpi_table_header *)
				      (holy_addr_t) rsdpv2->rsdpv1.rsdt_addr,
				      holy_ACPI_FADT_SIGNATURE);
  if (fadt)
    return fadt;
  if (rsdpv2
#if holy_CPU_SIZEOF_VOID_P != 8
      && !(rsdpv2->xsdt_addr >> 32)
#endif
      )
    fadt = holy_acpi_xsdt_find_table ((struct holy_acpi_table_header *)
				      (holy_addr_t) rsdpv2->xsdt_addr,
				      holy_ACPI_FADT_SIGNATURE);
  if (fadt)
    return fadt;
  return 0;
}
