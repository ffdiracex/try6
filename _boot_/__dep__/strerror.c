/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include <string.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intprops.h"
#include "strerror-override.h"
#include "verify.h"

/* Use the system functions, not the gnulib overrides in this file.  */
#undef sprintf

char *
strerror (int n)
#undef strerror
{
  static char buf[STACKBUF_LEN];
  size_t len;

  /* Cast away const, due to the historical signature of strerror;
     callers should not be modifying the string.  */
  const char *msg = strerror_override (n);
  if (msg)
    return (char *) msg;

  msg = strerror (n);

  /* Our strerror_r implementation might use the system's strerror
     buffer, so all other clients of strerror have to see the error
     copied into a buffer that we manage.  This is not thread-safe,
     even if the system strerror is, but portable programs shouldn't
     be using strerror if they care about thread-safety.  */
  if (!msg || !*msg)
    {
      static char const fmt[] = "Unknown error %d";
      verify (sizeof buf >= sizeof (fmt) + INT_STRLEN_BOUND (n));
      sprintf (buf, fmt, n);
      errno = EINVAL;
      return buf;
    }

  /* Fix STACKBUF_LEN if this ever aborts.  */
  len = strlen (msg);
  if (sizeof buf <= len)
    abort ();

  return memcpy (buf, msg, len + 1);
}
