/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/efiemu/efiemu.h>
#include <holy/crypto.h>

holy_err_t
SUFFIX (holy_efiemu_prepare) (struct holy_efiemu_prepare_hook *prepare_hooks,
			      struct holy_efiemu_configuration_table
			      *config_tables)
{
  holy_err_t err;
  int conftable_handle;
  struct holy_efiemu_configuration_table *cur;
  struct holy_efiemu_prepare_hook *curhook;

  int cntconftables = 0;
  struct SUFFIX (holy_efiemu_configuration_table) *conftables = 0;
  int i;
  int handle;
  holy_off_t off;

  holy_dprintf ("efiemu", "Preparing EfiEmu\n");

  /* Request space for the list of configuration tables */
  for (cur = config_tables; cur; cur = cur->next)
    cntconftables++;
  conftable_handle
    = holy_efiemu_request_memalign (holy_EFIEMU_PAGESIZE,
				    cntconftables * sizeof (*conftables),
				    holy_EFI_RUNTIME_SERVICES_DATA);

  /* Switch from phase 1 (counting) to phase 2 (real job) */
  holy_efiemu_alloc_syms ();
  holy_efiemu_mm_do_alloc ();
  holy_efiemu_write_sym_markers ();

  holy_efiemu_system_table32 = 0;
  holy_efiemu_system_table64 = 0;

  /* Execute hooks */
  for (curhook = prepare_hooks; curhook; curhook = curhook->next)
    curhook->hook (curhook->data);

  /* Move runtime to its due place */
  err = holy_efiemu_loadcore_load ();
  if (err)
    {
      holy_efiemu_unload ();
      return err;
    }

  err = holy_efiemu_resolve_symbol ("efiemu_system_table", &handle, &off);
  if (err)
    {
      holy_efiemu_unload ();
      return err;
    }

  SUFFIX (holy_efiemu_system_table)
    = (struct SUFFIX (holy_efi_system_table) *)
    ((holy_uint8_t *) holy_efiemu_mm_obtain_request (handle) + off);

  /* Put pointer to the list of configuration tables in system table */
  err = holy_efiemu_write_value
	(&(SUFFIX (holy_efiemu_system_table)->configuration_table), 0,
	 conftable_handle, 0, 1,
	 sizeof (SUFFIX (holy_efiemu_system_table)->configuration_table));
  if (err)
    {
      holy_efiemu_unload ();
      return err;
    }

  SUFFIX(holy_efiemu_system_table)->num_table_entries = cntconftables;

  /* Fill the list of configuration tables */
  conftables = (struct SUFFIX (holy_efiemu_configuration_table) *)
    holy_efiemu_mm_obtain_request (conftable_handle);
  i = 0;
  for (cur = config_tables; cur; cur = cur->next, i++)
    {
      holy_memcpy (&(conftables[i].vendor_guid), &(cur->guid),
		       sizeof (cur->guid));
      if (cur->get_table)
	conftables[i].vendor_table = (holy_addr_t) cur->get_table (cur->data);
      else
	conftables[i].vendor_table = (holy_addr_t) cur->data;
    }

  err = SUFFIX (holy_efiemu_crc) ();
  if (err)
    {
      holy_efiemu_unload ();
      return err;
    }

  holy_dprintf ("efiemu","system_table = %p, conftables = %p (%d entries)\n",
		SUFFIX (holy_efiemu_system_table), conftables, cntconftables);

  return holy_ERR_NONE;
}

holy_err_t
SUFFIX (holy_efiemu_crc) (void)
{
  holy_err_t err;
  int handle;
  holy_off_t off;
  struct SUFFIX (holy_efiemu_runtime_services) *runtime_services;
  holy_uint32_t crc32_val;

  if (holy_MD_CRC32->mdlen != 4)
    return holy_error (holy_ERR_BUG, "incorrect mdlen");

  /* compute CRC32 of runtime_services */
  err = holy_efiemu_resolve_symbol ("efiemu_runtime_services",
				    &handle, &off);
  if (err)
    return err;

  runtime_services = (struct SUFFIX (holy_efiemu_runtime_services) *)
	((holy_uint8_t *) holy_efiemu_mm_obtain_request (handle) + off);

  runtime_services->hdr.crc32 = 0;

  holy_crypto_hash (holy_MD_CRC32, &crc32_val,
		    runtime_services, runtime_services->hdr.header_size);
  runtime_services->hdr.crc32 =
      holy_be_to_cpu32(crc32_val);

  err = holy_efiemu_resolve_symbol ("efiemu_system_table", &handle, &off);
  if (err)
    return err;

  /* compute CRC32 of system table */
  SUFFIX (holy_efiemu_system_table)->hdr.crc32 = 0;
  holy_crypto_hash (holy_MD_CRC32, &crc32_val,
		    SUFFIX (holy_efiemu_system_table),
		    SUFFIX (holy_efiemu_system_table)->hdr.header_size);
  SUFFIX (holy_efiemu_system_table)->hdr.crc32 =
      holy_be_to_cpu32(crc32_val);

  holy_dprintf ("efiemu","system_table = %p, runtime_services = %p\n",
		SUFFIX (holy_efiemu_system_table), runtime_services);

  return holy_ERR_NONE;
}
