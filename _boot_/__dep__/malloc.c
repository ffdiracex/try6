/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define _GL_USE_STDLIB_ALLOC 1
#include <config.h>
/* Only the AC_FUNC_MALLOC macro defines 'malloc' already in config.h.  */
#ifdef malloc
# define NEED_MALLOC_GNU 1
# undef malloc
/* Whereas the gnulib module 'malloc-gnu' defines HAVE_MALLOC_GNU.  */
#elif GNULIB_MALLOC_GNU && !HAVE_MALLOC_GNU
# define NEED_MALLOC_GNU 1
#endif

#include <stdlib.h>

#include <errno.h>

/* Allocate an N-byte block of memory from the heap.
   If N is zero, allocate a 1-byte block.  */

void *
rpl_malloc (size_t n)
{
  void *result;

#if NEED_MALLOC_GNU
  if (n == 0)
    n = 1;
#endif

  result = malloc (n);

#if !HAVE_MALLOC_POSIX
  if (result == NULL)
    errno = ENOMEM;
#endif

  return result;
}
