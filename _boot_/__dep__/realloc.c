/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define _GL_USE_STDLIB_ALLOC 1
#include <config.h>

/* Only the AC_FUNC_REALLOC macro defines 'realloc' already in config.h.  */
#ifdef realloc
# define NEED_REALLOC_GNU 1
/* Whereas the gnulib module 'realloc-gnu' defines HAVE_REALLOC_GNU.  */
#elif GNULIB_REALLOC_GNU && !HAVE_REALLOC_GNU
# define NEED_REALLOC_GNU 1
#endif

/* Infer the properties of the system's malloc function.
   The gnulib module 'malloc-gnu' defines HAVE_MALLOC_GNU.  */
#if GNULIB_MALLOC_GNU && HAVE_MALLOC_GNU
# define SYSTEM_MALLOC_GLIBC_COMPATIBLE 1
#endif

#include <stdlib.h>

#include <errno.h>

/* Change the size of an allocated block of memory P to N bytes,
   with error checking.  If N is zero, change it to 1.  If P is NULL,
   use malloc.  */

void *
rpl_realloc (void *p, size_t n)
{
  void *result;

#if NEED_REALLOC_GNU
  if (n == 0)
    {
      n = 1;

      /* In theory realloc might fail, so don't rely on it to free.  */
      free (p);
      p = NULL;
    }
#endif

  if (p == NULL)
    {
#if GNULIB_REALLOC_GNU && !NEED_REALLOC_GNU && !SYSTEM_MALLOC_GLIBC_COMPATIBLE
      if (n == 0)
        n = 1;
#endif
      result = malloc (n);
    }
  else
    result = realloc (p, n);

#if !HAVE_REALLOC_POSIX
  if (result == NULL)
    errno = ENOMEM;
#endif

  return result;
}
