/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/disk.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_rescue_cmd_info (struct holy_command *cmd __attribute__ ((unused)),
    int argc __attribute__ ((unused)),
    char *argv[] __attribute__ ((unused)))
{
  unsigned long hits, misses;

  holy_disk_cache_get_performance (&hits, &misses);
  if (hits + misses)
    {
      unsigned long ratio = hits * 10000 / (hits + misses);
      holy_printf ("(%lu.%lu%%)\n", ratio / 100, ratio % 100);
      holy_printf_ (N_("Disk cache statistics: hits = %lu (%lu.%02lu%%),"
		     " misses = %lu\n"), ratio / 100, ratio % 100,
		    hits, misses);
    }
  else
    holy_printf ("%s\n", _("No disk cache statistics available\n"));

 return 0;
}

static holy_command_t cmd_cacheinfo;

holy_MOD_INIT(cacheinfo)
{
  cmd_cacheinfo =
    holy_register_command ("cacheinfo", holy_rescue_cmd_info,
			   0, N_("Get disk cache info."));
}

holy_MOD_FINI(cacheinfo)
{
  holy_unregister_command (cmd_cacheinfo);
}
