/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/memory.h>
#include <holy/misc.h>
#include <holy/term.h>
#include <holy/loader.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))

static void *hooktarget = 0;

extern holy_uint8_t holy_machine_mmaphook_start;
extern holy_uint8_t holy_machine_mmaphook_end;
extern holy_uint8_t holy_machine_mmaphook_int12;
extern holy_uint8_t holy_machine_mmaphook_int15;

static holy_uint16_t holy_machine_mmaphook_int12offset = 0;
static holy_uint16_t holy_machine_mmaphook_int12segment = 0;
extern holy_uint16_t holy_machine_mmaphook_int15offset;
extern holy_uint16_t holy_machine_mmaphook_int15segment;

extern holy_uint16_t holy_machine_mmaphook_mmap_num;
extern holy_uint16_t holy_machine_mmaphook_kblow;
extern holy_uint16_t holy_machine_mmaphook_kbin16mb;
extern holy_uint16_t holy_machine_mmaphook_64kbin4gb;

struct holy_e820_mmap_entry
{
  holy_uint64_t addr;
  holy_uint64_t len;
  holy_uint32_t type;
} holy_PACKED;


/* Helper for preboot.  */
static int fill_hook (holy_uint64_t addr, holy_uint64_t size,
		      holy_memory_type_t type, void *data)
{
  struct holy_e820_mmap_entry **hookmmapcur = data;
  holy_dprintf ("mmap", "mmap chunk %llx-%llx:%x\n", addr, addr + size, type);
  (*hookmmapcur)->addr = addr;
  (*hookmmapcur)->len = size;
  (*hookmmapcur)->type = type;
  (*hookmmapcur)++;
  return 0;
}

static holy_err_t
preboot (int noreturn __attribute__ ((unused)))
{
  struct holy_e820_mmap_entry *hookmmap, *hookmmapcur;

  if (! hooktarget)
    return holy_error (holy_ERR_OUT_OF_MEMORY,
		       "no space is allocated for memory hook");

  holy_dprintf ("mmap", "installing preboot handlers\n");

  hookmmapcur = hookmmap = (struct holy_e820_mmap_entry *)
    ((holy_uint8_t *) hooktarget + (&holy_machine_mmaphook_end
				    - &holy_machine_mmaphook_start));

  holy_mmap_iterate (fill_hook, &hookmmapcur);
  holy_machine_mmaphook_mmap_num = hookmmapcur - hookmmap;

  holy_machine_mmaphook_kblow = holy_mmap_get_lower () >> 10;
  holy_machine_mmaphook_kbin16mb
    = min (holy_mmap_get_upper (),0x3f00000ULL) >> 10;
  holy_machine_mmaphook_64kbin4gb
    = min (holy_mmap_get_post64 (), 0xfc000000ULL) >> 16;

  /* Correct BDA. */
  *((holy_uint16_t *) 0x413) = holy_mmap_get_lower () >> 10;

  /* Save old interrupt handlers. */
  holy_machine_mmaphook_int12offset = *((holy_uint16_t *) 0x48);
  holy_machine_mmaphook_int12segment = *((holy_uint16_t *) 0x4a);
  holy_machine_mmaphook_int15offset = *((holy_uint16_t *) 0x54);
  holy_machine_mmaphook_int15segment = *((holy_uint16_t *) 0x56);

  holy_dprintf ("mmap", "hooktarget = %p\n", hooktarget);

  /* Install the interrupt handlers. */
  holy_memcpy (hooktarget, &holy_machine_mmaphook_start,
	       &holy_machine_mmaphook_end - &holy_machine_mmaphook_start);

  *((holy_uint16_t *) 0x4a) = ((holy_addr_t) hooktarget) >> 4;
  *((holy_uint16_t *) 0x56) = ((holy_addr_t) hooktarget) >> 4;
  *((holy_uint16_t *) 0x48) = &holy_machine_mmaphook_int12
    - &holy_machine_mmaphook_start;
  *((holy_uint16_t *) 0x54) = &holy_machine_mmaphook_int15
    - &holy_machine_mmaphook_start;

  return holy_ERR_NONE;
}

static holy_err_t
preboot_rest (void)
{
  /* Restore old interrupt handlers. */
  *((holy_uint16_t *) 0x48) = holy_machine_mmaphook_int12offset;
  *((holy_uint16_t *) 0x4a) = holy_machine_mmaphook_int12segment;
  *((holy_uint16_t *) 0x54) = holy_machine_mmaphook_int15offset;
  *((holy_uint16_t *) 0x56) = holy_machine_mmaphook_int15segment;

  return holy_ERR_NONE;
}

/* Helper for malloc_hook.  */
static int
count_hook (holy_uint64_t addr __attribute__ ((unused)),
	    holy_uint64_t size __attribute__ ((unused)),
	    holy_memory_type_t type __attribute__ ((unused)), void *data)
{
  int *regcount = data;
  (*regcount)++;
  return 0;
}

static holy_err_t
malloc_hook (void)
{
  static int reentry = 0;
  static int mmapregion = 0;
  static int slots_available = 0;
  int hooksize;
  int regcount = 0;

  if (reentry)
    return holy_ERR_NONE;

  holy_dprintf ("mmap", "registering\n");

  holy_mmap_iterate (count_hook, &regcount);

  /* Mapping hook itself may introduce up to 2 additional regions. */
  regcount += 2;

  if (regcount <= slots_available)
    return holy_ERR_NONE;

  if (mmapregion)
    {
      holy_mmap_free_and_unregister (mmapregion);
      mmapregion = 0;
      hooktarget = 0;
    }

  hooksize = &holy_machine_mmaphook_end - &holy_machine_mmaphook_start
    + regcount * sizeof (struct holy_e820_mmap_entry);
  /* Allocate an integer number of KiB. */
  hooksize = ((hooksize - 1) | 0x3ff) + 1;
  slots_available = (hooksize - (&holy_machine_mmaphook_end
				 - &holy_machine_mmaphook_start))
    / sizeof (struct holy_e820_mmap_entry);

  reentry = 1;
  hooktarget
    = holy_mmap_malign_and_register (16, ALIGN_UP (hooksize, 16), &mmapregion,
				     holy_MEMORY_RESERVED,
				     holy_MMAP_MALLOC_LOW);
  reentry = 0;

  if (! hooktarget)
    {
      slots_available = 0;
      return holy_error (holy_ERR_OUT_OF_MEMORY, "no space for mmap hook");
    }
  return holy_ERR_NONE;
}

holy_err_t
holy_machine_mmap_register (holy_uint64_t start __attribute__ ((unused)),
			    holy_uint64_t size __attribute__ ((unused)),
			    int type __attribute__ ((unused)),
			    int handle  __attribute__ ((unused)))
{
  holy_err_t err;
  static struct holy_preboot *preb_handle = 0;

  err = malloc_hook ();
  if (err)
    return err;

  if (! preb_handle)
    {
      holy_dprintf ("mmap", "adding preboot\n");
      preb_handle
	= holy_loader_register_preboot_hook (preboot, preboot_rest,
					     holy_LOADER_PREBOOT_HOOK_PRIO_MEMORY);
      if (! preb_handle)
	return holy_errno;
    }
  return holy_ERR_NONE;
}

holy_err_t
holy_machine_mmap_unregister (int handle __attribute__ ((unused)))
{
  return holy_ERR_NONE;
}
