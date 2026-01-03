/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/util/misc.h>
#include <holy/i18n.h>

#include "progname.h"

void
holy_util_host_init (int *argc __attribute__ ((unused)),
		     char ***argv)
{
  set_program_name ((*argv)[0]);

#if (defined (holy_UTIL) && defined(ENABLE_NLS) && ENABLE_NLS)
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif /* (defined(ENABLE_NLS) && ENABLE_NLS) */
}
