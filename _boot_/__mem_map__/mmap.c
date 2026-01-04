/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/memory.h>
#include <holy/machine/memory.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/command.h>
#include <holy/dl.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#ifndef holy_MMAP_REGISTER_BY_FIRMWARE

struct holy_mmap_region *holy_mmap_overlays = 0;
static int curhandle = 1;

#endif

static int current_priority = 1;

/* Scanline events. */
struct holy_mmap_scan
{
  /* At which memory address. */
  holy_uint64_t pos;
  /* 0 = region starts, 1 = region ends. */
  int type;
  /* Which type of memory region? */
  holy_memory_type_t memtype;
  /* Priority. 0 means coming from firmware.  */
  int priority;
};

/* Context for holy_mmap_iterate.  */
struct holy_mmap_iterate_ctx
{
  struct holy_mmap_scan *scanline_events;
  int i;
};

/* Helper for holy_mmap_iterate.  */
static int
count_hook (holy_uint64_t addr __attribute__ ((unused)),
	    holy_uint64_t size __attribute__ ((unused)),
	    holy_memory_type_t type __attribute__ ((unused)), void *data)
{
  int *mmap_num = data;

  (*mmap_num)++;
  return 0;
}

/* Helper for holy_mmap_iterate.  */
static int
fill_hook (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	   void *data)
{
  struct holy_mmap_iterate_ctx *ctx = data;

  if (type == holy_MEMORY_HOLE)
    {
      holy_dprintf ("mmap", "Unknown memory type %d. Assuming unusable\n",
		    type);
      type = holy_MEMORY_RESERVED;
    }

  ctx->scanline_events[ctx->i].pos = addr;
  ctx->scanline_events[ctx->i].type = 0;
  ctx->scanline_events[ctx->i].memtype = type;
  ctx->scanline_events[ctx->i].priority = 0;

  ctx->i++;

  ctx->scanline_events[ctx->i].pos = addr + size;
  ctx->scanline_events[ctx->i].type = 1;
  ctx->scanline_events[ctx->i].memtype = type;
  ctx->scanline_events[ctx->i].priority = 0;
  ctx->i++;

  return 0;
}

struct mm_list
{
  struct mm_list *next;
  holy_memory_type_t val;
  int present;
};

holy_err_t
holy_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  /* This function resolves overlapping regions and sorts the memory map.
     It uses scanline (sweeping) algorithm.
  */
  struct holy_mmap_iterate_ctx ctx;
  int i, done;

  struct holy_mmap_scan t;

  /* Previous scanline event. */
  holy_uint64_t lastaddr;
  int lasttype;
  /* Current scanline event. */
  int curtype;
  /* How many regions of given type/priority overlap at current location? */
  /* Normally there shouldn't be more than one region per priority but be robust.  */
  struct mm_list *present;
  /* Number of mmap chunks. */
  int mmap_num;

#ifndef holy_MMAP_REGISTER_BY_FIRMWARE
  struct holy_mmap_region *cur;
#endif

  mmap_num = 0;

#ifndef holy_MMAP_REGISTER_BY_FIRMWARE
  for (cur = holy_mmap_overlays; cur; cur = cur->next)
    mmap_num++;
#endif

  holy_machine_mmap_iterate (count_hook, &mmap_num);

  /* Initialize variables. */
  ctx.scanline_events = (struct holy_mmap_scan *)
    holy_malloc (sizeof (struct holy_mmap_scan) * 2 * mmap_num);

  present = holy_zalloc (sizeof (present[0]) * current_priority);

  if (! ctx.scanline_events || !present)
    {
      holy_free (ctx.scanline_events);
      holy_free (present);
      return holy_errno;
    }

  ctx.i = 0;
#ifndef holy_MMAP_REGISTER_BY_FIRMWARE
  /* Register scanline events. */
  for (cur = holy_mmap_overlays; cur; cur = cur->next)
    {
      ctx.scanline_events[ctx.i].pos = cur->start;
      ctx.scanline_events[ctx.i].type = 0;
      ctx.scanline_events[ctx.i].memtype = cur->type;
      ctx.scanline_events[ctx.i].priority = cur->priority;
      ctx.i++;

      ctx.scanline_events[ctx.i].pos = cur->end;
      ctx.scanline_events[ctx.i].type = 1;
      ctx.scanline_events[ctx.i].memtype = cur->type;
      ctx.scanline_events[ctx.i].priority = cur->priority;
      ctx.i++;
    }
#endif /* ! holy_MMAP_REGISTER_BY_FIRMWARE */

  holy_machine_mmap_iterate (fill_hook, &ctx);

  /* Primitive bubble sort. It has complexity O(n^2) but since we're
     unlikely to have more than 100 chunks it's probably one of the
     fastest for one purpose. */
  done = 1;
  while (done)
    {
      done = 0;
      for (i = 0; i < 2 * mmap_num - 1; i++)
	if (ctx.scanline_events[i + 1].pos < ctx.scanline_events[i].pos
	    || (ctx.scanline_events[i + 1].pos == ctx.scanline_events[i].pos
		&& ctx.scanline_events[i + 1].type == 0
		&& ctx.scanline_events[i].type == 1))
	  {
	    t = ctx.scanline_events[i + 1];
	    ctx.scanline_events[i + 1] = ctx.scanline_events[i];
	    ctx.scanline_events[i] = t;
	    done = 1;
	  }
    }

  lastaddr = ctx.scanline_events[0].pos;
  lasttype = ctx.scanline_events[0].memtype;
  for (i = 0; i < 2 * mmap_num; i++)
    {
      /* Process event. */
      if (ctx.scanline_events[i].type)
	{
	  if (present[ctx.scanline_events[i].priority].present)
	    {
	      if (present[ctx.scanline_events[i].priority].val == ctx.scanline_events[i].memtype)
		{
		  if (present[ctx.scanline_events[i].priority].next)
		    {
		      struct mm_list *p = present[ctx.scanline_events[i].priority].next;
		      present[ctx.scanline_events[i].priority] = *p;
		      holy_free (p);
		    }
		  else
		    {
		      present[ctx.scanline_events[i].priority].present = 0;
		    }
		}
	      else
		{
		  struct mm_list **q = &(present[ctx.scanline_events[i].priority].next), *p;
		  for (; *q; q = &((*q)->next))
		    if ((*q)->val == ctx.scanline_events[i].memtype)
		      {
			p = *q;
			*q = p->next;
			holy_free (p);
			break;
		      }
		}
	    }
	}
      else
	{
	  if (!present[ctx.scanline_events[i].priority].present)
	    {
	      present[ctx.scanline_events[i].priority].present = 1;
	      present[ctx.scanline_events[i].priority].val = ctx.scanline_events[i].memtype;
	    }
	  else
	    {
	      struct mm_list *n = holy_malloc (sizeof (*n));
	      n->val = ctx.scanline_events[i].memtype;
	      n->present = 1;
	      n->next = present[ctx.scanline_events[i].priority].next;
	      present[ctx.scanline_events[i].priority].next = n;
	    }
	}

      /* Determine current region type. */
      curtype = -1;
      {
	int k;
	for (k = current_priority - 1; k >= 0; k--)
	  if (present[k].present)
	    {
	      curtype = present[k].val;
	      break;
	    }
      }

      /* Announce region to the hook if necessary. */
      if ((curtype == -1 || curtype != lasttype)
	  && lastaddr != ctx.scanline_events[i].pos
	  && lasttype != -1
	  && lasttype != holy_MEMORY_HOLE
	  && hook (lastaddr, ctx.scanline_events[i].pos - lastaddr, lasttype,
		   hook_data))
	{
	  holy_free (ctx.scanline_events);
	  return holy_ERR_NONE;
	}

      /* Update last values if necessary. */
      if (curtype == -1 || curtype != lasttype)
	{
	  lasttype = curtype;
	  lastaddr = ctx.scanline_events[i].pos;
	}
    }

  holy_free (ctx.scanline_events);
  return holy_ERR_NONE;
}

#ifndef holy_MMAP_REGISTER_BY_FIRMWARE
int
holy_mmap_register (holy_uint64_t start, holy_uint64_t size, int type)
{
  struct holy_mmap_region *cur;

  holy_dprintf ("mmap", "registering\n");

  cur = (struct holy_mmap_region *)
    holy_malloc (sizeof (struct holy_mmap_region));
  if (! cur)
    return 0;

  cur->next = holy_mmap_overlays;
  cur->start = start;
  cur->end = start + size;
  cur->type = type;
  cur->handle = curhandle++;
  cur->priority = current_priority++;
  holy_mmap_overlays = cur;

  if (holy_machine_mmap_register (start, size, type, curhandle))
    {
      holy_mmap_overlays = cur->next;
      holy_free (cur);
      return 0;
    }

  return cur->handle;
}

holy_err_t
holy_mmap_unregister (int handle)
{
  struct holy_mmap_region *cur, *prev;

  for (cur = holy_mmap_overlays, prev = 0; cur; prev = cur, cur = cur->next)
    if (handle == cur->handle)
      {
	holy_err_t err;
	err = holy_machine_mmap_unregister (handle);
	if (err)
	  return err;

	if (prev)
	  prev->next = cur->next;
	else
	  holy_mmap_overlays = cur->next;
	holy_free (cur);
	return holy_ERR_NONE;
      }
  return holy_error (holy_ERR_BUG, "mmap overlay not found");
}

#endif /* ! holy_MMAP_REGISTER_BY_FIRMWARE */

#define CHUNK_SIZE	0x400

struct badram_entry {
  holy_uint64_t addr, mask;
};

static inline holy_uint64_t
fill_mask (struct badram_entry *entry, holy_uint64_t iterator)
{
  int i, j;
  holy_uint64_t ret = (entry->addr & entry->mask);

  /* Find first fixed bit. */
  for (i = 0; i < 64; i++)
    if ((entry->mask & (1ULL << i)) != 0)
      break;
  j = 0;
  for (; i < 64; i++)
    if ((entry->mask & (1ULL << i)) == 0)
      {
	if ((iterator & (1ULL << j)) != 0)
	  ret |= 1ULL << i;
	j++;
      }
  return ret;
}

/* Helper for holy_cmd_badram.  */
static int
badram_iter (holy_uint64_t addr, holy_uint64_t size,
	     holy_memory_type_t type __attribute__ ((unused)), void *data)
{
  struct badram_entry *entry = data;
  holy_uint64_t iterator, low, high, cur;
  int tail, var;
  int i;
  holy_dprintf ("badram", "hook %llx+%llx\n", (unsigned long long) addr,
		(unsigned long long) size);

  /* How many trailing zeros? */
  for (tail = 0; ! (entry->mask & (1ULL << tail)); tail++);

  /* How many zeros in mask? */
  var = 0;
  for (i = 0; i < 64; i++)
    if (! (entry->mask & (1ULL << i)))
      var++;

  if (fill_mask (entry, 0) >= addr)
    iterator = 0;
  else
    {
      low = 0;
      high = ~0ULL;
      /* Find starting value. Keep low and high such that
	 fill_mask (low) < addr and fill_mask (high) >= addr;
      */
      while (high - low > 1)
	{
	  cur = (low + high) / 2;
	  if (fill_mask (entry, cur) >= addr)
	    high = cur;
	  else
	    low = cur;
	}
      iterator = high;
    }

  for (; iterator < (1ULL << (var - tail))
	 && (cur = fill_mask (entry, iterator)) < addr + size;
       iterator++)
    {
      holy_dprintf ("badram", "%llx (size %llx) is a badram range\n",
		    (unsigned long long) cur, (1ULL << tail));
      holy_mmap_register (cur, (1ULL << tail), holy_MEMORY_HOLE);
    }
  return 0;
}

static holy_err_t
holy_cmd_badram (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char **args)
{
  char * str;
  struct badram_entry entry;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  holy_dprintf ("badram", "executing badram\n");

  str = args[0];

  while (1)
    {
      /* Parse address and mask.  */
      entry.addr = holy_strtoull (str, &str, 16);
      if (*str == ',')
	str++;
      entry.mask = holy_strtoull (str, &str, 16);
      if (*str == ',')
	str++;

      if (holy_errno == holy_ERR_BAD_NUMBER)
	{
	  holy_errno = 0;
	  return holy_ERR_NONE;
	}

      /* When part of a page is tainted, we discard the whole of it.  There's
	 no point in providing sub-page chunks.  */
      entry.mask &= ~(CHUNK_SIZE - 1);

      holy_dprintf ("badram", "badram %llx:%llx\n",
		    (unsigned long long) entry.addr,
		    (unsigned long long) entry.mask);

      holy_mmap_iterate (badram_iter, &entry);
    }
}

static holy_uint64_t
parsemem (const char *str)
{
  holy_uint64_t ret;
  char *ptr;

  ret = holy_strtoul (str, &ptr, 0);

  switch (*ptr)
    {
    case 'K':
      return ret << 10;
    case 'M':
      return ret << 20;
    case 'G':
      return ret << 30;
    case 'T':
      return ret << 40;
    }
  return ret;
}

struct cutmem_range {
  holy_uint64_t from, to;
};

/* Helper for holy_cmd_cutmem.  */
static int
cutmem_iter (holy_uint64_t addr, holy_uint64_t size,
	     holy_memory_type_t type __attribute__ ((unused)), void *data)
{
  struct cutmem_range *range = data;
  holy_uint64_t end = addr + size;

  if (addr <= range->from)
    addr = range->from;
  if (end >= range->to)
    end = range->to;

  if (end <= addr)
    return 0;

  holy_mmap_register (addr, end - addr, holy_MEMORY_HOLE);
  return 0;
}

static holy_err_t
holy_cmd_cutmem (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char **args)
{
  struct cutmem_range range;

  if (argc != 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  range.from = parsemem (args[0]);
  if (holy_errno)
    return holy_errno;

  range.to = parsemem (args[1]);
  if (holy_errno)
    return holy_errno;

  holy_mmap_iterate (cutmem_iter, &range);

  return holy_ERR_NONE;
}

static holy_command_t cmd, cmd_cut;


holy_MOD_INIT(mmap)
{
  cmd = holy_register_command ("badram", holy_cmd_badram,
			       N_("ADDR1,MASK1[,ADDR2,MASK2[,...]]"),
			       N_("Declare memory regions as faulty (badram)."));
  cmd_cut = holy_register_command ("cutmem", holy_cmd_cutmem,
				   N_("FROM[K|M|G] TO[K|M|G]"),
				   N_("Remove any memory regions in specified range."));

}

holy_MOD_FINI(mmap)
{
  holy_unregister_command (cmd);
  holy_unregister_command (cmd_cut);
}

