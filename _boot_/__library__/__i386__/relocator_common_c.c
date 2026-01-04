/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>

#include <holy/types.h>
#include <holy/err.h>
#include <holy/term.h>

#include <holy/relocator.h>
#include <holy/relocator_private.h>

extern holy_uint8_t holy_relocator_forward_start;
extern holy_uint8_t holy_relocator_forward_end;
extern holy_uint8_t holy_relocator_backward_start;
extern holy_uint8_t holy_relocator_backward_end;

extern void *holy_relocator_backward_dest;
extern void *holy_relocator_backward_src;
extern holy_size_t holy_relocator_backward_chunk_size;

extern void *holy_relocator_forward_dest;
extern void *holy_relocator_forward_src;
extern holy_size_t holy_relocator_forward_chunk_size;

#define RELOCATOR_SIZEOF(x)	(&holy_relocator##x##_end - &holy_relocator##x##_start)

holy_size_t holy_relocator_align = 1;
holy_size_t holy_relocator_forward_size;
holy_size_t holy_relocator_backward_size;
#ifdef __x86_64__
holy_size_t holy_relocator_jumper_size = 12;
#else
holy_size_t holy_relocator_jumper_size = 7;
#endif

void
holy_cpu_relocator_init (void)
{
  holy_relocator_forward_size = RELOCATOR_SIZEOF (_forward);
  holy_relocator_backward_size = RELOCATOR_SIZEOF (_backward);
}

void
holy_cpu_relocator_jumper (void *rels, holy_addr_t addr)
{
  holy_uint8_t *ptr;
  ptr = rels;
#ifdef __x86_64__
  /* movq imm64, %rax (for relocator) */
  *(holy_uint8_t *) ptr = 0x48;
  ptr++;
  *(holy_uint8_t *) ptr = 0xb8;
  ptr++;
  *(holy_uint64_t *) ptr = addr;
  ptr += sizeof (holy_uint64_t);
#else
  /* movl imm32, %eax (for relocator) */
  *(holy_uint8_t *) ptr = 0xb8;
  ptr++;
  *(holy_uint32_t *) ptr = addr;
  ptr += sizeof (holy_uint32_t);
#endif
  /* jmp $eax/$rax */
  *(holy_uint8_t *) ptr = 0xff;
  ptr++;
  *(holy_uint8_t *) ptr = 0xe0;
  ptr++;
}

void
holy_cpu_relocator_backward (void *ptr, void *src, void *dest,
			     holy_size_t size)
{
  holy_relocator_backward_dest = dest;
  holy_relocator_backward_src = src;
  holy_relocator_backward_chunk_size = size;

  holy_memmove (ptr,
		&holy_relocator_backward_start, RELOCATOR_SIZEOF (_backward));
}

void
holy_cpu_relocator_forward (void *ptr, void *src, void *dest,
			    holy_size_t size)
{
  holy_relocator_forward_dest = dest;
  holy_relocator_forward_src = src;
  holy_relocator_forward_chunk_size = size;

  holy_memmove (ptr,
		&holy_relocator_forward_start, RELOCATOR_SIZEOF (_forward));
}
