/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/cache.h>
#include <holy/misc.h>

static holy_int64_t dlinesz;
static holy_int64_t ilinesz;

/* Prototypes for asm functions. */
void holy_arch_clean_dcache_range (holy_addr_t beg, holy_addr_t end,
				   holy_uint64_t line_size);
void holy_arch_invalidate_icache_range (holy_addr_t beg, holy_addr_t end,
					holy_uint64_t line_size);

static void
probe_caches (void)
{
  holy_uint64_t cache_type;

  /* Read Cache Type Register */
  asm volatile ("mrs 	%0, ctr_el0": "=r"(cache_type));

  dlinesz = 4 << ((cache_type >> 16) & 0xf);
  ilinesz = 4 << (cache_type & 0xf);

  holy_dprintf("cache", "D$ line size: %lld\n", (long long) dlinesz);
  holy_dprintf("cache", "I$ line size: %lld\n", (long long) ilinesz);
}

void
holy_arch_sync_caches (void *address, holy_size_t len)
{
  holy_uint64_t start, end, max_align;

  if (dlinesz == 0)
    probe_caches();
  if (dlinesz == 0)
    holy_fatal ("Unknown cache line size!");

  max_align = dlinesz > ilinesz ? dlinesz : ilinesz;

  start = ALIGN_DOWN ((holy_uint64_t) address, max_align);
  end = ALIGN_UP ((holy_uint64_t) address + len, max_align);

  holy_arch_clean_dcache_range (start, end, dlinesz);
  holy_arch_invalidate_icache_range (start, end, ilinesz);
}
