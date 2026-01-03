/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>

#include <holy/types.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/cache.h>

#include <holy/mips/relocator.h>
#include <holy/relocator_private.h>

/* Do we need mips64? */

extern holy_uint8_t holy_relocator_forward_start;
extern holy_uint8_t holy_relocator_forward_end;
extern holy_uint8_t holy_relocator_backward_start;
extern holy_uint8_t holy_relocator_backward_end;

#define REGW_SIZEOF (2 * sizeof (holy_uint32_t))
#define JUMP_SIZEOF (2 * sizeof (holy_uint32_t))

#define RELOCATOR_SRC_SIZEOF(x) (&holy_relocator_##x##_end \
				 - &holy_relocator_##x##_start)
#define RELOCATOR_SIZEOF(x)	(RELOCATOR_SRC_SIZEOF(x) \
				 + REGW_SIZEOF * 3)
holy_size_t holy_relocator_align = sizeof (holy_uint32_t);
holy_size_t holy_relocator_forward_size;
holy_size_t holy_relocator_backward_size;
holy_size_t holy_relocator_jumper_size = JUMP_SIZEOF + REGW_SIZEOF;

void
holy_cpu_relocator_init (void)
{
  holy_relocator_forward_size = RELOCATOR_SIZEOF(forward);
  holy_relocator_backward_size = RELOCATOR_SIZEOF(backward);
}

static void
write_reg (int regn, holy_uint32_t val, void **target)
{
  /* lui $r, (val+0x8000).  */
  *(holy_uint32_t *) *target = ((0x3c00 | regn) << 16) | ((val + 0x8000) >> 16);
  *target = ((holy_uint32_t *) *target) + 1;
  /* addiu $r, $r, val. */
  *(holy_uint32_t *) *target = (((0x2400 | regn << 5 | regn) << 16)
				| (val & 0xffff));
  *target = ((holy_uint32_t *) *target) + 1;
}

static void
write_jump (int regn, void **target)
{
  /* j $r.  */
  *(holy_uint32_t *) *target = (regn<<21) | 0x8;
  *target = ((holy_uint32_t *) *target) + 1;
  /* nop.  */
  *(holy_uint32_t *) *target = 0;
  *target = ((holy_uint32_t *) *target) + 1;
}

void
holy_cpu_relocator_jumper (void *rels, holy_addr_t addr)
{
  write_reg (1, addr, &rels);
  write_jump (1, &rels);
}

void
holy_cpu_relocator_backward (void *ptr0, void *src, void *dest,
			     holy_size_t size)
{
  void *ptr = ptr0;
  write_reg (8, (holy_uint32_t) src, &ptr);
  write_reg (9, (holy_uint32_t) dest, &ptr);
  write_reg (10, (holy_uint32_t) size, &ptr);
  holy_memcpy (ptr, &holy_relocator_backward_start,
	       RELOCATOR_SRC_SIZEOF (backward));
}

void
holy_cpu_relocator_forward (void *ptr0, void *src, void *dest,
			     holy_size_t size)
{
  void *ptr = ptr0;
  write_reg (8, (holy_uint32_t) src, &ptr);
  write_reg (9, (holy_uint32_t) dest, &ptr);
  write_reg (10, (holy_uint32_t) size, &ptr);
  holy_memcpy (ptr, &holy_relocator_forward_start,
	       RELOCATOR_SRC_SIZEOF (forward));
}

holy_err_t
holy_relocator32_boot (struct holy_relocator *rel,
		       struct holy_relocator32_state state)
{
  holy_relocator_chunk_t ch;
  void *ptr;
  holy_err_t err;
  void *relst;
  holy_size_t relsize;
  holy_size_t stateset_size = 31 * REGW_SIZEOF + JUMP_SIZEOF;
  unsigned i;
  holy_addr_t vtarget;

  err = holy_relocator_alloc_chunk_align (rel, &ch, 0,
					  (0xffffffff - stateset_size)
					  + 1, stateset_size,
					  sizeof (holy_uint32_t),
					  holy_RELOCATOR_PREFERENCE_NONE, 0);
  if (err)
    return err;

  ptr = get_virtual_current_address (ch);
  for (i = 1; i < 32; i++)
    write_reg (i, state.gpr[i], &ptr);
  write_jump (state.jumpreg, &ptr);

  vtarget = (holy_addr_t) holy_map_memory (get_physical_target_address (ch),
					   stateset_size);

  err = holy_relocator_prepare_relocs (rel, vtarget, &relst, &relsize);
  if (err)
    return err;

  holy_arch_sync_caches ((void *) relst, relsize);

  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return holy_ERR_NONE;
}
