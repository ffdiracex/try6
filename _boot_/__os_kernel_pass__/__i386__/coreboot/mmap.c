/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/machine/lbio.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/misc.h>

/* Context for holy_machine_mmap_iterate.  */
struct holy_machine_mmap_iterate_ctx
{
  holy_memory_hook_t hook;
  void *hook_data;
};

#define holy_MACHINE_MEMORY_BADRAM 	5

/* Helper for holy_machine_mmap_iterate.  */
static int
iterate_linuxbios_table (holy_linuxbios_table_item_t table_item, void *data)
{
  struct holy_machine_mmap_iterate_ctx *ctx = data;
  mem_region_t mem_region;

  if (table_item->tag != holy_LINUXBIOS_MEMBER_MEMORY)
    return 0;

  mem_region =
    (mem_region_t) ((long) table_item +
			       sizeof (struct holy_linuxbios_table_item));
  for (; (long) mem_region < (long) table_item + (long) table_item->size;
       mem_region++)
    {
      holy_uint64_t start = mem_region->addr;
      holy_uint64_t end = mem_region->addr + mem_region->size;
      /* Mark region 0xa0000 - 0x100000 as reserved.  */
      if (start < 0x100000 && end >= 0xa0000
	  && mem_region->type == holy_MACHINE_MEMORY_AVAILABLE)
	{
	  if (start < 0xa0000
	      && ctx->hook (start, 0xa0000 - start,
			    /* Multiboot mmaps match with the coreboot mmap
			       definition.  Therefore, we can just pass type
			       through.  */
			    mem_region->type,
			    ctx->hook_data))
	    return 1;
	  if (start < 0xa0000)
	    start = 0xa0000;
	  if (start >= end)
	    continue;

	  if (ctx->hook (start, (end > 0x100000 ? 0x100000 : end) - start,
			 holy_MEMORY_RESERVED,
			 ctx->hook_data))
	    return 1;
	  start = 0x100000;

	  if (end <= start)
	    continue;
	}
      if (ctx->hook (start, end - start,
		     /* Multiboot mmaps match with the coreboot mmap
		        definition.  Therefore, we can just pass type
		        through.  */
		     mem_region->type,
		     ctx->hook_data))
	return 1;
    }

  return 0;
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  struct holy_machine_mmap_iterate_ctx ctx = { hook, hook_data };

  holy_linuxbios_table_iterate (iterate_linuxbios_table, &ctx);

  return 0;
}
