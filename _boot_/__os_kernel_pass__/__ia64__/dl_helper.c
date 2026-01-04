/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/elf.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <holy/i18n.h>
#include <holy/ia64/reloc.h>

#pragma GCC diagnostic ignored "-Wcast-align"

#define MASK20 ((1 << 20) - 1)
#define MASK3 (~(holy_addr_t) 3)

void
holy_ia64_set_immu64 (holy_addr_t addr, holy_uint64_t val)
{
  /* Copied from binutils.  */
  holy_uint64_t *ptr = ((holy_uint64_t *) (addr & MASK3));
  holy_uint64_t t0, t1;

  t0 = holy_le_to_cpu64 (ptr[0]);
  t1 = holy_le_to_cpu64 (ptr[1]);

  /* tmpl/s: bits  0.. 5 in t0
     slot 0: bits  5..45 in t0
     slot 1: bits 46..63 in t0, bits 0..22 in t1
     slot 2: bits 23..63 in t1 */

  /* First, clear the bits that form the 64 bit constant.  */
  t0 &= ~(0x3ffffLL << 46);
  t1 &= ~(0x7fffffLL
	  | ((  (0x07fLL << 13) | (0x1ffLL << 27)
		| (0x01fLL << 22) | (0x001LL << 21)
		| (0x001LL << 36)) << 23));

  t0 |= ((val >> 22) & 0x03ffffLL) << 46;		/* 18 lsbs of imm41 */
  t1 |= ((val >> 40) & 0x7fffffLL) <<  0;		/* 23 msbs of imm41 */
  t1 |= (  (((val >>  0) & 0x07f) << 13)		/* imm7b */
	   | (((val >>  7) & 0x1ff) << 27)		/* imm9d */
	   | (((val >> 16) & 0x01f) << 22)		/* imm5c */
	   | (((val >> 21) & 0x001) << 21)		/* ic */
	   | (((val >> 63) & 0x001) << 36)) << 23;	/* i */

  ptr[0] = t0;
  ptr[1] = t1;
}

void
holy_ia64_add_value_to_slot_20b (holy_addr_t addr, holy_uint32_t value)
{
  holy_uint32_t val;
  switch (addr & 3)
    {
    case 0:
      val = holy_le_to_cpu32 (holy_get_unaligned32 (((holy_uint8_t *)
						     (addr & MASK3) + 2)));
      val = (((((val & MASK20) + value) & MASK20) << 2) 
	    | (val & ~(MASK20 << 2)));
      holy_set_unaligned32 (((holy_uint8_t *) (addr & MASK3) + 2),
			    holy_cpu_to_le32 (val));
      break;
    case 1:
      val = holy_le_to_cpu32 (holy_get_unaligned32 (((holy_uint8_t *)
						     (addr & MASK3) + 7)));
      val = ((((((val >> 3) & MASK20) + value) & MASK20) << 3)
	    | (val & ~(MASK20 << 3)));
      holy_set_unaligned32 (((holy_uint8_t *) (addr & MASK3) + 7),
			    holy_cpu_to_le32 (val));
      break;
    case 2:
      val = holy_le_to_cpu32 (holy_get_unaligned32 (((holy_uint8_t *)
						     (addr & MASK3) + 12)));
      val = ((((((val >> 4) & MASK20) + value) & MASK20) << 4)
	    | (val & ~(MASK20 << 4)));
      holy_set_unaligned32 (((holy_uint8_t *) (addr & MASK3) + 12),
			    holy_cpu_to_le32 (val));
      break;
    }
}

#define MASKF21 ( ((1 << 23) - 1) & ~((1 << 7) | (1 << 8)) )

static holy_uint32_t
add_value_to_slot_21_real (holy_uint32_t a, holy_uint32_t value)
{
  holy_uint32_t high, mid, low, c;
  low  = (a & 0x00007f);
  mid  = (a & 0x7fc000) >> 7;
  high = (a & 0x003e00) << 7;
  c = (low | mid | high) + value;
  return (c & 0x7f) | ((c << 7) & 0x7fc000) | ((c >> 7) & 0x0003e00); //0x003e00
}

void
holy_ia64_add_value_to_slot_21 (holy_addr_t addr, holy_uint32_t value)
{
  holy_uint32_t val;
  switch (addr & 3)
    {
    case 0:
      val = holy_le_to_cpu32 (holy_get_unaligned32 (((holy_uint8_t *)
						     (addr & MASK3) + 2)));
      val = ((add_value_to_slot_21_real (((val >> 2) & MASKF21), value)
	      & MASKF21) << 2) | (val & ~(MASKF21 << 2));
      holy_set_unaligned32 (((holy_uint8_t *) (addr & MASK3) + 2),
			    holy_cpu_to_le32 (val));
      break;
    case 1:
      val = holy_le_to_cpu32 (holy_get_unaligned32 (((holy_uint8_t *)
						     (addr & MASK3) + 7)));
      val = ((add_value_to_slot_21_real (((val >> 3) & MASKF21), value)
	      & MASKF21) << 3) | (val & ~(MASKF21 << 3));
      holy_set_unaligned32 (((holy_uint8_t *) (addr & MASK3) + 7),
			    holy_cpu_to_le32 (val));
      break;
    case 2:
      val = holy_le_to_cpu32 (holy_get_unaligned32 (((holy_uint8_t *)
						     (addr & MASK3) + 12)));
      val = ((add_value_to_slot_21_real (((val >> 4) & MASKF21), value)
	      & MASKF21) << 4) | (val & ~(MASKF21 << 4));
      holy_set_unaligned32 (((holy_uint8_t *) (addr & MASK3) + 12),
			    holy_cpu_to_le32 (val));
      break;
    }
}

static const holy_uint8_t nopm[5] =
  {
    /* [MLX]       nop.m 0x0 */
    0x05, 0x00, 0x00, 0x00, 0x01
  };

#ifdef holy_UTIL
static holy_uint8_t jump[0x20] =
  {
    /* [MMI]       add r15=r15,r1;; */
    0x0b, 0x78, 0x3c, 0x02, 0x00, 0x20,
    /* ld8 r16=[r15],8 */
    0x00, 0x41, 0x3c, 0x30, 0x28, 0xc0,
    /* mov r14=r1;; */
    0x01, 0x08, 0x00, 0x84,
    /* 	[MIB]       ld8 r1=[r15] */
    0x11, 0x08, 0x00, 0x1e, 0x18, 0x10,
    /* mov b6=r16 */
    0x60, 0x80, 0x04, 0x80, 0x03, 0x00, 
    /* br.few b6;; */
    0x60, 0x00, 0x80, 0x00       	            
  };
#else
static const holy_uint8_t jump[0x20] =
  {
    /* ld8 r16=[r15],8 */
    0x02, 0x80, 0x20, 0x1e, 0x18, 0x14,
    /* mov r14=r1;; */
    0xe0, 0x00, 0x04, 0x00, 0x42, 0x00,
    /* nop.i 0x0 */
    0x00, 0x00, 0x04, 0x00,
    /* ld8 r1=[r15] */
    0x11, 0x08, 0x00, 0x1e, 0x18, 0x10,
    /* mov b6=r16 */
    0x60, 0x80, 0x04, 0x80, 0x03, 0x00,
    /* br.few b6;; */
    0x60, 0x00, 0x80, 0x00
  };
#endif

void
holy_ia64_make_trampoline (struct holy_ia64_trampoline *tr, holy_uint64_t addr)
{
  holy_memcpy (tr->nop, nopm, sizeof (tr->nop));
  tr->addr_hi[0] = ((addr & 0xc00000) >> 16);
  tr->addr_hi[1] = (addr >> 24) & 0xff;
  tr->addr_hi[2] = (addr >> 32) & 0xff;
  tr->addr_hi[3] = (addr >> 40) & 0xff;
  tr->addr_hi[4] = (addr >> 48) & 0xff;
  tr->addr_hi[5] = (addr >> 56) & 0xff;
  tr->e0 = 0xe0;
  tr->addr_lo[0] = ((addr & 0x000f) << 4) | 0x01;
  tr->addr_lo[1] = (((addr & 0x0070) >> 4) | ((addr & 0x070000) >> 11)
		    | ((addr & 0x200000) >> 17));
  tr->addr_lo[2] = ((addr & 0x1f80) >> 5) | ((addr & 0x180000) >> 19);
  tr->addr_lo[3] = ((addr & 0xe000) >> 13) | 0x60;
  holy_memcpy (tr->jump, jump, sizeof (tr->jump));
}

holy_err_t
holy_ia64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
				 holy_size_t *got)
{
  const Elf64_Ehdr *e = ehdr;
  holy_size_t cntt = 0, cntg = 0;
  const Elf64_Shdr *s;
  unsigned i;

  for (i = 0, s = (Elf64_Shdr *) ((char *) e + holy_le_to_cpu64 (e->e_shoff));
       i < holy_le_to_cpu16 (e->e_shnum);
       i++, s = (Elf64_Shdr *) ((char *) s + holy_le_to_cpu16 (e->e_shentsize)))
    if (s->sh_type == holy_cpu_to_le32_compile_time (SHT_RELA))
      {
	const Elf64_Rela *rel, *max;

	for (rel = (Elf64_Rela *) ((char *) e + holy_le_to_cpu64 (s->sh_offset)),
	       max = (const Elf64_Rela *) ((char *) rel + holy_le_to_cpu64 (s->sh_size));
	     rel < max; rel = (const Elf64_Rela *) ((char *) rel + holy_le_to_cpu64 (s->sh_entsize)))
	  switch (ELF64_R_TYPE (holy_le_to_cpu64 (rel->r_info)))
	    {
	    case R_IA64_PCREL21B:
	      cntt++;
	      break;
	    case R_IA64_LTOFF_FPTR22:
	    case R_IA64_LTOFF22X:
	    case R_IA64_LTOFF22:
	      cntg++;
	      break;
	    }
      }
  *tramp = cntt * sizeof (struct holy_ia64_trampoline);
  *got = cntg * sizeof (holy_uint64_t);

  return holy_ERR_NONE;
}

