/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/types.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/util/install.h>
#include <holy/lib/envblk.h>
#include <holy/i18n.h>
#include <holy/emu/hostfile.h>

#include <errno.h>
#include <string.h>

#define DEFAULT_ENVBLK_SIZE	1024

void
holy_util_create_envblk_file (const char *name)
{
  FILE *fp;
  char *buf;
  char *namenew;

  buf = xmalloc (DEFAULT_ENVBLK_SIZE);

  namenew = xasprintf ("%s.new", name);
  fp = holy_util_fopen (namenew, "wb");
  if (! fp)
    holy_util_error (_("cannot open `%s': %s"), namenew,
		     strerror (errno));

  memcpy (buf, holy_ENVBLK_SIGNATURE, sizeof (holy_ENVBLK_SIGNATURE) - 1);
  memset (buf + sizeof (holy_ENVBLK_SIGNATURE) - 1, '#',
          DEFAULT_ENVBLK_SIZE - sizeof (holy_ENVBLK_SIGNATURE) + 1);

  if (fwrite (buf, 1, DEFAULT_ENVBLK_SIZE, fp) != DEFAULT_ENVBLK_SIZE)
    holy_util_error (_("cannot write to `%s': %s"), namenew,
		     strerror (errno));


  holy_util_file_sync (fp);
  free (buf);
  fclose (fp);

  if (holy_util_rename (namenew, name) < 0)
    holy_util_error (_("cannot rename the file %s to %s"), namenew, name);
  free (namenew);
}
