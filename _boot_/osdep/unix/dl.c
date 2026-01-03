/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

void *
holy_osdep_dl_memalign (holy_size_t align, holy_size_t size)
{
  void *ret;
  if (align < 8192 * 16)
    align = 8192 * 16;
  size = ALIGN_UP (size, 8192 * 16);

#if defined(HAVE_POSIX_MEMALIGN)
  if (posix_memalign (&ret, align, size) != 0)
    ret = 0;
#elif defined(HAVE_MEMALIGN)
  ret = memalign (align, size);
#else
#error "Complete this"
#endif

  if (!ret)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      return NULL;
    }

  mprotect (ret, size, PROT_READ | PROT_WRITE | PROT_EXEC);
  return ret;
}

void
holy_dl_osdep_dl_free (void *ptr)
{
  if (ptr)
    free (ptr);
}
