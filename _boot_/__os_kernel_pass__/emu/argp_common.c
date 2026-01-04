/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"

#define _GNU_SOURCE	1
#include <stdlib.h>
#include <unistd.h>
#include <progname.h>
#include <argp.h>

/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state)
{
  fprintf (stream, "%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
}
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

/* Set the bug report address */
const char *argp_program_bug_address = "<"PACKAGE_BUGREPORT">";
