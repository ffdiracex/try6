/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/normal.h>
#include <holy/charset.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

static void
disp_sal (void *table)
{
  struct holy_efi_sal_system_table *t = table;
  void *desc;
  holy_uint32_t len, l, i;

  holy_printf ("SAL rev: %02x, signature: %x, len:%x\n",
	       t->sal_rev, t->signature, t->total_table_len);
  holy_printf ("nbr entry: %d, chksum: %02x, SAL version A: %02x B: %02x\n",
	       t->entry_count, t->checksum,
	       t->sal_a_version, t->sal_b_version);
  holy_printf ("OEM-ID: %-32s\n", t->oem_id);
  holy_printf ("Product-ID: %-32s\n", t->product_id);

  desc = t->entries;
  len = t->total_table_len - sizeof (struct holy_efi_sal_system_table);
  if (t->total_table_len <= sizeof (struct holy_efi_sal_system_table))
    return;
  for (i = 0; i < t->entry_count; i++)
    {
      switch (*(holy_uint8_t *) desc)
	{
	case holy_EFI_SAL_SYSTEM_TABLE_TYPE_ENTRYPOINT_DESCRIPTOR:
	  {
	    struct holy_efi_sal_system_table_entrypoint_descriptor *c = desc;
	    l = sizeof (*c);
	    holy_printf (" Entry point: PAL=%016" PRIxholy_UINT64_T
			 " SAL=%016" PRIxholy_UINT64_T " GP=%016"
			 PRIxholy_UINT64_T "\n",
			 c->pal_proc_addr, c->sal_proc_addr,
			 c->global_data_ptr);
	  }
	  break;
	case holy_EFI_SAL_SYSTEM_TABLE_TYPE_MEMORY_DESCRIPTOR:
	  {
	    struct holy_efi_sal_system_table_memory_descriptor *c = desc;
	    l = sizeof (*c);
	    holy_printf (" Memory descriptor entry addr=%016" PRIxholy_UINT64_T
			 " len=%" PRIuholy_UINT64_T "KB\n",
			 c->addr, c->len * 4);
	    holy_printf ("     sal_used=%d attr=%x AR=%x attr_mask=%x "
			 "type=%x usage=%x\n",
			 c->sal_used, c->attr, c->ar, c->attr_mask, c->mem_type,
			 c->usage);
	  }
	  break;
	case holy_EFI_SAL_SYSTEM_TABLE_TYPE_PLATFORM_FEATURES:
	  {
	    struct holy_efi_sal_system_table_platform_features *c = desc;
	    l = sizeof (*c);
	    holy_printf (" Platform features: %02x", c->flags);
	    if (c->flags & holy_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_BUSLOCK)
	      holy_printf (" BusLock");
	    if (c->flags & holy_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IRQREDIRECT)
	      holy_printf (" IrqRedirect");
	    if (c->flags & holy_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IPIREDIRECT)

	      holy_printf (" IPIRedirect");
	    if (c->flags & holy_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_ITCDRIFT)

	      holy_printf (" ITCDrift");
	    holy_printf ("\n");
	  }
	  break;
	case holy_EFI_SAL_SYSTEM_TABLE_TYPE_TRANSLATION_REGISTER_DESCRIPTOR:
	  {
	    struct holy_efi_sal_system_table_translation_register_descriptor *c
	      = desc;
	    l = sizeof (*c);
	    holy_printf (" TR type=%d num=%d va=%016" PRIxholy_UINT64_T
			 " pte=%016" PRIxholy_UINT64_T "\n",
			 c->register_type, c->register_number,
			 c->addr, c->page_size);
	  }
	  break;
	case holy_EFI_SAL_SYSTEM_TABLE_TYPE_PURGE_TRANSLATION_COHERENCE:
	  {
	    struct holy_efi_sal_system_table_purge_translation_coherence *c
	      = desc;
	    l = sizeof (*c);
	    holy_printf (" PTC coherence nbr=%d addr=%016" PRIxholy_UINT64_T "\n",
			 c->ndomains, c->coherence);
	  }
	  break;
	case holy_EFI_SAL_SYSTEM_TABLE_TYPE_AP_WAKEUP:
	  {
	    struct holy_efi_sal_system_table_ap_wakeup *c = desc;
	    l = sizeof (*c);
	    holy_printf (" AP wake-up: mec=%d vect=%" PRIxholy_UINT64_T "\n",
			 c->mechanism, c->vector);
	  }
	  break;
	default:
	  holy_printf (" unknown entry 0x%x\n", *(holy_uint8_t *)desc);
	  return;
	}
      desc = (holy_uint8_t *)desc + l;
      if (len <= l)
	return;
      len -= l;
    }
}

static holy_err_t
holy_cmd_lssal (struct holy_command *cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  const holy_efi_system_table_t *st = holy_efi_system_table;
  holy_efi_configuration_table_t *t = st->configuration_table;
  unsigned int i;
  holy_efi_packed_guid_t guid = holy_EFI_SAL_TABLE_GUID;

  for (i = 0; i < st->num_table_entries; i++)
    {
      if (holy_memcmp (&guid, &t->vendor_guid,
		       sizeof (holy_efi_packed_guid_t)) == 0)
	{
	  disp_sal (t->vendor_table);
	  return holy_ERR_NONE;
	}
      t++;
    }
  holy_printf ("SAL not found\n");
  return holy_ERR_NONE;
}

static holy_command_t cmd;

holy_MOD_INIT(lssal)
{
  cmd = holy_register_command ("lssal", holy_cmd_lssal, "",
			       "Display SAL system table.");
}

holy_MOD_FINI(lssal)
{
  holy_unregister_command (cmd);
}
