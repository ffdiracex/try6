/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>
#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include <holy/mm.h>
#include <holy/err.h>
#include <holy/env.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/i18n.h>
#include <holy/time.h>
#include <holy/emu/misc.h>

int verbosity;

void
holy_util_warn (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, _("%s: warning:"), program_name);
  fprintf (stderr, " ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, ".\n");
  fflush (stderr);
}

void
holy_util_info (const char *fmt, ...)
{
  if (verbosity > 0)
    {
      va_list ap;

      fprintf (stderr, _("%s: info:"), program_name);
      fprintf (stderr, " ");
      va_start (ap, fmt);
      vfprintf (stderr, fmt, ap);
      va_end (ap);
      fprintf (stderr, ".\n");
      fflush (stderr);
    }
}

void
holy_util_error (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, _("%s: error:"), program_name);
  fprintf (stderr, " ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fprintf (stderr, ".\n");
  exit (1);
}

void *
xmalloc (holy_size_t size)
{
  void *p;

  p = malloc (size);
  if (! p)
    holy_util_error ("%s", _("out of memory"));

  return p;
}

void *
xrealloc (void *ptr, holy_size_t size)
{
  ptr = realloc (ptr, size);
  if (! ptr)
    holy_util_error ("%s", _("out of memory"));

  return ptr;
}

char *
xstrdup (const char *str)
{
  size_t len;
  char *newstr;

  len = strlen (str);
  newstr = (char *) xmalloc (len + 1);
  memcpy (newstr, str, len + 1);

  return newstr;
}

#if !defined (holy_MKFONT) && !defined (holy_BUILD)
char *
xasprintf (const char *fmt, ...)
{ 
  va_list ap;
  char *result;
  
  va_start (ap, fmt);
  result = holy_xvasprintf (fmt, ap);
  va_end (ap);
  if (!result)
    holy_util_error ("%s", _("out of memory"));
  
  return result;
}
#endif

#if !defined (holy_MACHINE_EMU) || defined (holy_UTIL)
void
holy_exit (void)
{
  exit (1);
}
#endif

holy_uint64_t
holy_get_time_ms (void)
{
  struct timeval tv;

  gettimeofday (&tv, 0);

  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

size_t
holy_util_get_image_size (const char *path)
{
  FILE *f;
  size_t ret;
  off_t sz;

  f = holy_util_fopen (path, "rb");

  if (!f)
    holy_util_error (_("cannot open `%s': %s"), path, strerror (errno));

  fseeko (f, 0, SEEK_END);
  
  sz = ftello (f);
  if (sz < 0)
    holy_util_error (_("cannot open `%s': %s"), path, strerror (errno));
  if (sz != (size_t) sz)
    holy_util_error (_("file `%s' is too big"), path);
  ret = (size_t) sz;

  fclose (f);

  return ret;
}

void
holy_util_load_image (const char *path, char *buf)
{
  FILE *fp;
  size_t size;

  holy_util_info ("reading %s", path);

  size = holy_util_get_image_size (path);

  fp = holy_util_fopen (path, "rb");
  if (! fp)
    holy_util_error (_("cannot open `%s': %s"), path,
		     strerror (errno));

  if (fread (buf, 1, size, fp) != size)
    holy_util_error (_("cannot read `%s': %s"), path,
		     strerror (errno));

  fclose (fp);
}
