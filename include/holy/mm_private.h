/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MM_PRIVATE_H
#define holy_MM_PRIVATE_H	1

#include <holy/mm.h>

/* Magic words.  */
#define holy_MM_FREE_MAGIC	0x2d3c2808
#define holy_MM_ALLOC_MAGIC	0x6db08fa4

typedef struct holy_mm_header
{
  struct holy_mm_header *next;
  holy_size_t size;
  holy_size_t magic;
#if holy_CPU_SIZEOF_VOID_P == 4
  char padding[4];
#elif holy_CPU_SIZEOF_VOID_P == 8
  char padding[8];
#else
# error "unknown word size"
#endif
}
*holy_mm_header_t;

#if holy_CPU_SIZEOF_VOID_P == 4
# define holy_MM_ALIGN_LOG2	4
#elif holy_CPU_SIZEOF_VOID_P == 8
# define holy_MM_ALIGN_LOG2	5
#endif

#define holy_MM_ALIGN	(1 << holy_MM_ALIGN_LOG2)

typedef struct holy_mm_region
{
  struct holy_mm_header *first;
  struct holy_mm_region *next;
  holy_size_t pre_size;
  holy_size_t size;
}
*holy_mm_region_t;

#ifndef holy_MACHINE_EMU
extern holy_mm_region_t EXPORT_VAR (holy_mm_base);
#endif

#endif
