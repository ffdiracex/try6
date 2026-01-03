/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include <holy/kernel.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/cache.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/time.h>
#include <holy/i18n.h>
#include <holy/script_sh.h>
#include <holy/emu/hostfile.h>

#define ENABLE_RELOCATABLE 0
#ifdef holy_BUILD
const char *program_name = holy_BUILD_PROGRAM_NAME;
#else
#include "progname.h"
#endif

#ifdef holy_UTIL
int
holy_err_printf (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = vfprintf (stderr, fmt, ap);
  va_end (ap);

  return ret;
}
#endif

char *
holy_util_get_path (const char *dir, const char *file)
{
  char *path;

  path = (char *) xmalloc (strlen (dir) + 1 + strlen (file) + 1);
  sprintf (path, "%s/%s", dir, file);
  return path;
}

char *
holy_util_read_image (const char *path)
{
  char *img;
  FILE *fp;
  size_t size;

  holy_util_info ("reading %s", path);

  size = holy_util_get_image_size (path);
  img = (char *) xmalloc (size);

  fp = holy_util_fopen (path, "rb");
  if (! fp)
    holy_util_error (_("cannot open `%s': %s"), path,
		     strerror (errno));

  if (fread (img, 1, size, fp) != size)
    holy_util_error (_("cannot read `%s': %s"), path,
		     strerror (errno));

  fclose (fp);

  return img;
}

void
holy_util_write_image_at (const void *img, size_t size, off_t offset, FILE *out,
			  const char *name)
{
  holy_util_info ("writing 0x%" holy_HOST_PRIxLONG_LONG " bytes at offset 0x%"
		  holy_HOST_PRIxLONG_LONG,
		  (unsigned long long) size, (unsigned long long) offset);
  if (fseeko (out, offset, SEEK_SET) == -1)
    holy_util_error (_("cannot seek `%s': %s"),
		     name, strerror (errno));
  if (fwrite (img, 1, size, out) != size)
    holy_util_error (_("cannot write to `%s': %s"),
		     name, strerror (errno));
}

void
holy_util_write_image (const char *img, size_t size, FILE *out,
		       const char *name)
{
  holy_util_info ("writing 0x%" holy_HOST_PRIxLONG_LONG " bytes", (unsigned long long) size);
  if (fwrite (img, 1, size, out) != size)
    {
      if (!name)
	holy_util_error (_("cannot write to the stdout: %s"),
			 strerror (errno));
      else
	holy_util_error (_("cannot write to `%s': %s"),
			 name, strerror (errno));
    }
}

holy_err_t
holy_script_execute_cmdline (struct holy_script_cmd *cmd __attribute__ ((unused)))
{
  return 0;
}

holy_err_t
holy_script_execute_cmdlist (struct holy_script_cmd *cmd __attribute__ ((unused)))
{
  return 0;
}

holy_err_t
holy_script_execute_cmdif (struct holy_script_cmd *cmd __attribute__ ((unused)))
{
  return 0;
}

holy_err_t
holy_script_execute_cmdfor (struct holy_script_cmd *cmd __attribute__ ((unused)))
{
  return 0;
}

holy_err_t
holy_script_execute_cmdwhile (struct holy_script_cmd *cmd __attribute__ ((unused)))
{
  return 0;
}

holy_err_t
holy_script_execute (struct holy_script *script)
{
  if (script == 0 || script->cmd == 0)
    return 0;

  return script->cmd->exec (script->cmd);
}

int
holy_getkey (void)
{
  return -1;
}

void
holy_refresh (void)
{
  fflush (stdout);
}

static void
holy_xputs_real (const char *str)
{
  fputs (str, stdout);
}

void (*holy_xputs) (const char *str) = holy_xputs_real;

int
holy_dl_ref (holy_dl_t mod)
{
  (void) mod;
  return 0;
}

int
holy_dl_unref (holy_dl_t mod)
{
  (void) mod;
  return 0;
}

/* Some functions that we don't use.  */
void
holy_mm_init_region (void *addr __attribute__ ((unused)),
		     holy_size_t size __attribute__ ((unused)))
{
}

void
holy_register_exported_symbols (void)
{
}

/* Used in comparison of arrays of strings with qsort */
int
holy_qsort_strcmp (const void *p1, const void *p2)
{
  return strcmp(*(char *const *)p1, *(char *const *)p2);
}

