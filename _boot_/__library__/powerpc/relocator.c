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

#include <holy/powerpc/relocator.h>
#include <holy/relocator_private.h>

extern holy_uint8_t holy_relocator_forward_start;
extern holy_uint8_t holy_relocator_forward_end;
extern holy_uint8_t holy_relocator_backward_start;
extern holy_uint8_t holy_relocator_backward_end;

#define REGW_SIZEOF (2 * sizeof (holy_uint32_t))
#define JUMP_SIZEOF (sizeof (holy_uint32_t))

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
  /* lis r, val >> 16  */
  *(holy_uint32_t *) *target =
    ((0x3c00 | (regn << 5)) << 16) | (val >> 16);
  *target = ((holy_uint32_t *) *target) + 1;
  /* ori r, r, val & 0xffff. */
  *(holy_uint32_t *) *target = (((0x6000 | regn << 5 | regn) << 16)
				| (val & 0xffff));
  *target = ((holy_uint32_t *) *target) + 1;
}

static void
write_jump (void **target)
{
  /* blr.  */
  *(holy_uint32_t *) *target = 0x4e800020;
  *target = ((holy_uint32_t *) *target) + 1;
}

void
holy_cpu_relocator_jumper (void *rels, holy_addr_t addr)
{
  write_reg (holy_PPC_JUMP_REGISTER, addr, &rels);
  write_jump (&rels);
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
  void *ptr;
  holy_err_t err;
  void *relst;
  holy_size_t relsize;
  holy_size_t stateset_size = 32 * REGW_SIZEOF + JUMP_SIZEOF;
  unsigned i;
  holy_relocator_chunk_t ch;

  err = holy_relocator_alloc_chunk_align (rel, &ch, 0,
					  (0xffffffff - stateset_size)
					  + 1, stateset_size,
					  sizeof (holy_uint32_t),
					  holy_RELOCATOR_PREFERENCE_NONE, 0);
  if (err)
    return err;

  ptr = get_virtual_current_address (ch);
  for (i = 0; i < 32; i++)
    write_reg (i, state.gpr[i], &ptr);
  write_jump (&ptr);

  err = holy_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, &relsize);
  if (err)
    return err;

  holy_arch_sync_caches ((void *) relst, relsize);

  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return holy_ERR_NONE;
}
