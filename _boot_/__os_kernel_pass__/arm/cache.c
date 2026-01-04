/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/cache.h>
#include <holy/arm/system.h>
#ifdef holy_MACHINE_UBOOT
#include <holy/uboot/uboot.h>
#include <holy/uboot/api_public.h>
#include <holy/mm.h>
#endif

/* This is only about cache architecture. It doesn't imply
   the CPU architecture.  */
static enum
  {
    ARCH_UNKNOWN,
    ARCH_ARMV5_WRITE_THROUGH,
    ARCH_ARMV6,
    ARCH_ARMV6_UNIFIED,
    ARCH_ARMV7
  } type = ARCH_UNKNOWN;

static int is_v6_mmu;

static holy_uint32_t holy_arch_cache_dlinesz;
static holy_uint32_t holy_arch_cache_ilinesz;
static holy_uint32_t holy_arch_cache_max_linesz;

/* Prototypes for asm functions.  */
void holy_arm_clean_dcache_range_armv6 (holy_addr_t start, holy_addr_t end,
					holy_addr_t dlinesz);
void holy_arm_clean_dcache_range_armv7 (holy_addr_t start, holy_addr_t end,
					holy_addr_t dlinesz);
void holy_arm_invalidate_icache_range_armv6 (holy_addr_t start, holy_addr_t end,
					     holy_addr_t dlinesz);
void holy_arm_invalidate_icache_range_armv7 (holy_addr_t start, holy_addr_t end,
					     holy_addr_t dlinesz);
void holy_arm_disable_caches_mmu_armv6 (void);
void holy_arm_disable_caches_mmu_armv7 (void);
holy_uint32_t holy_arm_main_id (void);
holy_uint32_t holy_arm_cache_type (void);

static void
probe_caches (void)
{
  holy_uint32_t main_id, cache_type;

  /* Read main ID Register */
  main_id = holy_arm_main_id ();

  switch ((main_id >> 16) & 0xf)
    {
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
      is_v6_mmu = 0;
      break;
    case 0x7:
    case 0xf:
      is_v6_mmu = 1;
      break;
    default:
      holy_fatal ("Unsupported ARM ID 0x%x", main_id);
    }

  /* Read Cache Type Register */
  cache_type = holy_arm_cache_type ();

  switch (cache_type >> 24)
    {
    case 0x00:
    case 0x01:
      holy_arch_cache_dlinesz = 8 << ((cache_type >> 12) & 3);
      holy_arch_cache_ilinesz = 8 << (cache_type & 3);
      type = ARCH_ARMV5_WRITE_THROUGH;
      break;
    case 0x04:
    case 0x0a:
    case 0x0c:
    case 0x0e:
    case 0x1c:
      holy_arch_cache_dlinesz = 8 << ((cache_type >> 12) & 3);
      holy_arch_cache_ilinesz = 8 << (cache_type & 3);
      type = ARCH_ARMV6_UNIFIED;
      break;
    case 0x05:
    case 0x0b:
    case 0x0d:
    case 0x0f:
    case 0x1d:
      holy_arch_cache_dlinesz = 8 << ((cache_type >> 12) & 3);
      holy_arch_cache_ilinesz = 8 << (cache_type & 3);
      type = ARCH_ARMV6;
      break;
    case 0x80 ... 0x8f:
      holy_arch_cache_dlinesz = 4 << ((cache_type >> 16) & 0xf);
      holy_arch_cache_ilinesz = 4 << (cache_type & 0xf);
      type = ARCH_ARMV7;
      break;
    default:
      holy_fatal ("Unsupported cache type 0x%x", cache_type);
    }
  if (holy_arch_cache_dlinesz > holy_arch_cache_ilinesz)
    holy_arch_cache_max_linesz = holy_arch_cache_dlinesz;
  else
    holy_arch_cache_max_linesz = holy_arch_cache_ilinesz;
}

#ifdef holy_MACHINE_UBOOT

static void subdivide (holy_uint32_t *table, holy_uint32_t *subtable,
		       holy_uint32_t addr)
{
  holy_uint32_t j;
  addr = addr >> 20 << 20;
  table[addr >> 20] = (holy_addr_t) subtable | 1;
  for (j = 0; j < 256; j++)
    subtable[j] = addr | (j << 12)
      | (3 << 4) | (3 << 6) | (3 << 8) | (3 << 10)
      | (0 << 3) | (1 << 2) | 2;
}

void
holy_arm_enable_caches_mmu (void)
{
  holy_uint32_t *table;
  holy_uint32_t i;
  holy_uint32_t border_crossing = 0;
  holy_uint32_t *subtable;
  struct sys_info *si = holy_uboot_get_sys_info ();

  if (!si || (si->mr_no == 0))
    {
      holy_printf ("couldn't get memory map, not enabling caches");
      holy_errno = holy_ERR_NONE;
      return;
    }

  if (type == ARCH_UNKNOWN)
    probe_caches ();

  for (i = 0; (signed) i < si->mr_no; i++)
    {
      if (si->mr[i].start & ((1 << 20) - 1))
	border_crossing++;
      if ((si->mr[i].start + si->mr[i].size) & ((1 << 20) - 1))
	border_crossing++;
    }

  holy_printf ("%d crossers\n", border_crossing);

  table = holy_memalign (1 << 14, (1 << 14) + (border_crossing << 10));
  if (!table)
    {
      holy_printf ("couldn't allocate place for MMU table, not enabling caches");
      holy_errno = holy_ERR_NONE;
      return;
    }

  subtable = table + (1 << 12);
  /* Map all unknown as device.  */
  for (i = 0; i < (1 << 12); i++)
    table[i] = (i << 20) | (3 << 10) | (0 << 3) | (1 << 2) | 2;
  /*
    Device: TEX= 0, C=0, B=1
    normal: TEX= 0, C=1, B=1
    AP = 3
    IMP = 0
    Domain = 0
*/

  for (i = 0; (signed) i < si->mr_no; i++)
    {
      if (si->mr[i].start & ((1 << 20) - 1))
	{
	  subdivide (table, subtable, si->mr[i].start);
	  subtable += (1 << 8);
	}
      if ((si->mr[i].start + si->mr[i].size) & ((1 << 20) - 1))
	{
	  subdivide (table, subtable, si->mr[i].start + si->mr[i].size);
	  subtable += (1 << 8);
	}
    }

  for (i = 0; (signed) i < si->mr_no; i++)
    if ((si->mr[i].flags & MR_ATTR_MASK) == MR_ATTR_DRAM
	|| (si->mr[i].flags & MR_ATTR_MASK) == MR_ATTR_SRAM
	|| (si->mr[i].flags & MR_ATTR_MASK) == MR_ATTR_FLASH)
      {
	holy_uint32_t cur, end;
	cur = si->mr[i].start;
	end = si->mr[i].start + si->mr[i].size;
	while (cur < end)
	  {
	    holy_uint32_t *st;
	    if ((table[cur >> 20] & 3) == 2)
	      {
		cur = cur >> 20 << 20;
		table[cur >> 20] = cur | (3 << 10) | (1 << 3) | (1 << 2) | 2;
		cur += (1 << 20);
		continue;
	      }
	    cur = cur >> 12 << 12;
	    st = (holy_uint32_t *) (table[cur >> 20] & ~0x3ff);
	    st[(cur >> 12) & 0xff] = cur | (3 << 4) | (3 << 6)
	      | (3 << 8) | (3 << 10)
	      | (1 << 3) | (1 << 2) | 2;
	    cur += (1 << 12);
	  }
      }

  holy_printf ("MMU tables generated\n");
  if (is_v6_mmu)
    holy_arm_clear_mmu_v6 ();

  holy_printf ("enabling MMU\n");
  holy_arm_enable_mmu (table);
  holy_printf ("MMU enabled\n");
}

#endif

void
holy_arch_sync_caches (void *address, holy_size_t len)
{
  holy_addr_t start = (holy_addr_t) address;
  holy_addr_t end = start + len;

  if (type == ARCH_UNKNOWN)
    probe_caches ();
  start = ALIGN_DOWN (start, holy_arch_cache_max_linesz);
  end = ALIGN_UP (end, holy_arch_cache_max_linesz);
  switch (type)
    {
    case ARCH_ARMV6:
      holy_arm_clean_dcache_range_armv6 (start, end, holy_arch_cache_dlinesz);
      holy_arm_invalidate_icache_range_armv6 (start, end,
					      holy_arch_cache_ilinesz);
      break;
    case ARCH_ARMV7:
      holy_arm_clean_dcache_range_armv7 (start, end, holy_arch_cache_dlinesz);
      holy_arm_invalidate_icache_range_armv7 (start, end,
					      holy_arch_cache_ilinesz);
      break;
      /* Nothing to do.  */
    case ARCH_ARMV5_WRITE_THROUGH:
    case ARCH_ARMV6_UNIFIED:
      break;
      /* Pacify GCC.  */
    case ARCH_UNKNOWN:
      break;
    }
}

void
holy_arm_disable_caches_mmu (void)
{
  if (type == ARCH_UNKNOWN)
    probe_caches ();
  switch (type)
    {
    case ARCH_ARMV5_WRITE_THROUGH:
    case ARCH_ARMV6_UNIFIED:
    case ARCH_ARMV6:
      holy_arm_disable_caches_mmu_armv6 ();
      break;
    case ARCH_ARMV7:
      holy_arm_disable_caches_mmu_armv7 ();
      break;
      /* Pacify GCC.  */
    case ARCH_UNKNOWN:
      break;
    }
}
