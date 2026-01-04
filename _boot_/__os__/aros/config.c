/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/emu/hostdisk.h>
#include <holy/emu/exec.h>
#include <holy/emu/config.h>
#include <holy/util/install.h>
#include <holy/util/misc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>

const char *
holy_util_get_config_filename (void)
{
  static char *value = NULL;
  if (!value)
    value = holy_util_path_concat (3, holy_SYSCONFDIR,
				   "default", "holy");
  return value;
}

const char *
holy_util_get_pkgdatadir (void)
{
  const char *ret = getenv ("pkgdatadir");
  if (ret)
    return ret;
  return holy_DATADIR "/" PACKAGE;
}

const char *
holy_util_get_pkglibdir (void)
{
  return holy_LIBDIR "/" PACKAGE;
}

const char *
holy_util_get_localedir (void)
{
  return LOCALEDIR;
}

void
holy_util_load_config (struct holy_util_config *cfg)
{
  const char *cfgfile;
  FILE *f = NULL;
  const char *v;

  memset (cfg, 0, sizeof (*cfg));

  v = getenv ("holy_ENABLE_CRYPTODISK");
  if (v && v[0] == 'y' && v[1] == '\0')
    cfg->is_cryptodisk_enabled = 1;

  v = getenv ("holy_DISTRIBUTOR");
  if (v)
    cfg->holy_distributor = xstrdup (v);

  cfgfile = holy_util_get_config_filename ();
  if (!holy_util_is_regular (cfgfile))
    return;

  f = holy_util_fopen (cfgfile, "r");
  if (f)
    {
      holy_util_parse_config (f, cfg, 0);
      fclose (f);
    }
  else
    holy_util_warn (_("cannot open configuration file `%s': %s"),
		    cfgfile, strerror (errno));
}
