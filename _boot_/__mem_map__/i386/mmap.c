/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/i386/memory.h>
#include <holy/memory.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/mm.h>


#ifndef holy_MMAP_REGISTER_BY_FIRMWARE

/* Context for holy_mmap_malign_and_register.  */
struct holy_mmap_malign_and_register_ctx
{
  holy_uint64_t align, size, highestlow;
};

/* Helper for holy_mmap_malign_and_register.  */
static int
find_hook (holy_uint64_t start, holy_uint64_t rangesize,
	   holy_memory_type_t memtype, void *data)
{
  struct holy_mmap_malign_and_register_ctx *ctx = data;
  holy_uint64_t end = start + rangesize;

  if (memtype != holy_MEMORY_AVAILABLE)
    return 0;
  if (end > 0x100000)
    end = 0x100000;
  if (end > start + ctx->size
      && ctx->highestlow < ((end - ctx->size)
			    - ((end - ctx->size) & (ctx->align - 1))))
    ctx->highestlow = (end - ctx->size)
		      - ((end - ctx->size) & (ctx->align - 1));
  return 0;
}

void *
holy_mmap_malign_and_register (holy_uint64_t align, holy_uint64_t size,
			       int *handle, int type, int flags)
{
  struct holy_mmap_malign_and_register_ctx ctx = {
    .align = align,
    .size = size,
    .highestlow = 0
  };

  void *ret;
  if (flags & holy_MMAP_MALLOC_LOW)
    {
      /* FIXME: use low-memory mm allocation once it's available. */
      holy_mmap_iterate (find_hook, &ctx);
      ret = (void *) (holy_addr_t) ctx.highestlow;
    }
  else
    ret = holy_memalign (align, size);

  if (! ret)
    {
      *handle = 0;
      return 0;
    }

  *handle = holy_mmap_register ((holy_addr_t) ret, size, type);
  if (! *handle)
    {
      holy_free (ret);
      return 0;
    }

  return ret;
}

void
holy_mmap_free_and_unregister (int handle)
{
  struct holy_mmap_region *cur;
  holy_uint64_t addr;

  for (cur = holy_mmap_overlays; cur; cur = cur->next)
    if (cur->handle == handle)
      break;

  if (! cur)
    return;

  addr = cur->start;

  holy_mmap_unregister (handle);

  if (addr >= 0x100000)
    holy_free ((void *) (holy_addr_t) addr);
}

#endif
