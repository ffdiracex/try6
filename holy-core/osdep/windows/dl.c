/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

void *
holy_osdep_dl_memalign (holy_size_t align, holy_size_t size)
{
  void *ret;
  if (align > 4096)
    {
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET, "too large alignment");
      return NULL;
    }

  size = ALIGN_UP (size, 4096);

  ret = VirtualAlloc (NULL, size, MEM_COMMIT | MEM_RESERVE,
		      PAGE_EXECUTE_READWRITE);

  if (!ret)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      return NULL;
    }

  return ret;
}

void
holy_dl_osdep_dl_free (void *ptr)
{
  if (!ptr)
    return;
  VirtualFree (ptr, 0,  MEM_RELEASE);
}
