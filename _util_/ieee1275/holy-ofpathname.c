/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/misc.h>
#include <holy/util/ofpath.h>

#include <holy/i18n.h>

#include "progname.h"

#include <string.h>

int main(int argc, char **argv)
{
  char *of_path;

  holy_util_host_init (&argc, &argv);

  if (argc != 2 || strcmp (argv[1], "--help") == 0)
    {
      printf(_("Usage: %s DEVICE\n"), program_name);
      return 1;
    }
  if (strcmp (argv[1], "--version") == 0)
    {
      printf ("%s\n", PACKAGE_STRING);
      return 1;
    }

  of_path = holy_util_devname_to_ofpath (argv[1]);
  printf("%s\n", of_path);

  free (of_path);

  return 0;
}
