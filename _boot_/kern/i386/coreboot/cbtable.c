/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/i386/coreboot/memory.h>
#include <holy/i386/coreboot/lbio.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

/* Helper for holy_linuxbios_table_iterate.  */
static int
check_signature (holy_linuxbios_table_header_t tbl_header)
{
  if (! holy_memcmp (tbl_header->signature, "LBIO", 4))
    return 1;

  return 0;
}

holy_err_t
holy_linuxbios_table_iterate (int (*hook) (holy_linuxbios_table_item_t,
					   void *),
			      void *hook_data)
{
  holy_linuxbios_table_header_t table_header;
  holy_linuxbios_table_item_t table_item;

  /* Assuming table_header is aligned to its size (8 bytes).  */

  for (table_header = (holy_linuxbios_table_header_t) 0x500;
       table_header < (holy_linuxbios_table_header_t) 0x1000; table_header++)
    if (check_signature (table_header))
      goto signature_found;

  for (table_header = (holy_linuxbios_table_header_t) 0xf0000;
       table_header < (holy_linuxbios_table_header_t) 0x100000; table_header++)
    if (check_signature (table_header))
      goto signature_found;

  return 0;

signature_found:

  table_item =
    (holy_linuxbios_table_item_t) ((char *) table_header +
				   table_header->header_size);
  for (; table_item < (holy_linuxbios_table_item_t) ((char *) table_header
						     + table_header->header_size
						     + table_header->table_size);
       table_item = (holy_linuxbios_table_item_t) ((char *) table_item + table_item->size))
    {
      if (table_item->tag == holy_LINUXBIOS_MEMBER_LINK
         && check_signature ((holy_linuxbios_table_header_t) (holy_addr_t)
                             *(holy_uint64_t *) (table_item + 1)))
       {
         table_header = (holy_linuxbios_table_header_t) (holy_addr_t)
           *(holy_uint64_t *) (table_item + 1);
         goto signature_found;   
       }
      if (hook (table_item, hook_data))
       return 1;
    }

  return 0;
}
