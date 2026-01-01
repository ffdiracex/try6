/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/i386/coreboot/lbio.h>
#include <holy/i386/tsc.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_uint32_t
tsc2ms (holy_uint64_t tsc)
{
  holy_uint64_t ah = tsc >> 32;
  holy_uint64_t al = tsc & 0xffffffff;

  return ((al * holy_tsc_rate) >> 32) + ah * holy_tsc_rate;
}

static const char *descs[] = {
  [1] = "romstage",
  [2] = "before RAM init",
  [3] = "after RAM init",
  [4] = "end of romstage",
  [5] = "start of verified boot",
  [6] = "end of verified boot",
  [8] = "start of RAM copy",
  [9] = "end of RAM copy",
  [10] = "start of ramstage",
  [11] = "start of bootblock",
  [12] = "end of bootblock",
  [13] = "starting to load romstage",
  [14] = "finished loading romstage",
  [15] = "starting LZMA decompress (ignore for x86)",
  [16] = "finished LZMA decompress (ignore for x86)",
  [30] = "device enumerate",
  [40] = "device configure",
  [50] = "device enable",
  [60] = "device initialize",
  [70] = "device done",
  [75] = "CBMEM POST",
  [80] = "writing tables",
  [90] = "loading payload",
  [98] = "wake jump",
  [99] = "selfboot jump",
};

static int
iterate_linuxbios_table (holy_linuxbios_table_item_t table_item,
			 void *data)
{
  int *available = data;
  holy_uint64_t last_tsc = 0;
  struct holy_linuxbios_timestamp_table *ts_table;
  unsigned i;

  if (table_item->tag != holy_LINUXBIOS_MEMBER_TIMESTAMPS)
    return 0;

  *available = 1;
  ts_table = (struct holy_linuxbios_timestamp_table *) (holy_addr_t)
    *(holy_uint64_t *) (table_item + 1);

  for (i = 0; i < ts_table->used; i++)
    {
      holy_uint32_t tmabs = tsc2ms (ts_table->entries[i].tsc);
      holy_uint32_t tmrel = tsc2ms (ts_table->entries[i].tsc - last_tsc);
      last_tsc = ts_table->entries[i].tsc;

      holy_printf ("%3d.%03ds %2d.%03ds %02d %s\n",
		   tmabs / 1000, tmabs % 1000, tmrel / 1000, tmrel % 1000,
		   ts_table->entries[i].id,
		   (ts_table->entries[i].id < ARRAY_SIZE (descs)
		    && descs[ts_table->entries[i].id])
		   ? descs[ts_table->entries[i].id] : "");
    }
  return 1;
}


static holy_err_t
holy_cmd_coreboot_boottime (struct holy_command *cmd __attribute__ ((unused)),
			    int argc __attribute__ ((unused)),
			    char *argv[] __attribute__ ((unused)))
{
  int available = 0;

  holy_linuxbios_table_iterate (iterate_linuxbios_table, &available);
  if (!available)
    {
      holy_puts_ (N_("No boot time statistics is available\n"));
      return 0;
    }
 return 0;
}

static holy_command_t cmd_boottime;

holy_MOD_INIT(cbtime)
{
  cmd_boottime =
    holy_register_command ("coreboot_boottime", holy_cmd_coreboot_boottime,
			   0, N_("Show coreboot boot time statistics."));
}

holy_MOD_FINI(cbtime)
{
  holy_unregister_command (cmd_boottime);
}
