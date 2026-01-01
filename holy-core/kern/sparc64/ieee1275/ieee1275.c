/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/ieee1275/ieee1275.h>
#include <holy/types.h>

/* Sun specific ieee1275 interfaces used by holy.  */

int
holy_ieee1275_claim_vaddr (holy_addr_t vaddr, holy_size_t size)
{
  struct claim_vaddr_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t method;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t align;
    holy_ieee1275_cell_t size;
    holy_ieee1275_cell_t virt;
    holy_ieee1275_cell_t catch_result;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 5, 2);
  args.method = (holy_ieee1275_cell_t) "claim";
  args.ihandle = holy_ieee1275_mmu;
  args.align = 0;
  args.size = size;
  args.virt = vaddr;
  args.catch_result = (holy_ieee1275_cell_t) -1;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  return args.catch_result;
}

int
holy_ieee1275_alloc_physmem (holy_addr_t *paddr, holy_size_t size,
			     holy_uint32_t align)
{
  holy_uint32_t memory_ihandle;
  struct alloc_physmem_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t method;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t align;
    holy_ieee1275_cell_t size;
    holy_ieee1275_cell_t catch_result;
    holy_ieee1275_cell_t phys_high;
    holy_ieee1275_cell_t phys_low;
  }
  args;
  holy_ssize_t actual = 0;

  holy_ieee1275_get_property (holy_ieee1275_chosen, "memory",
			      &memory_ihandle, sizeof (memory_ihandle),
			      &actual);
  if (actual != sizeof (memory_ihandle))
    return -1;

  if (!align)
    align = 1;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 4, 3);
  args.method = (holy_ieee1275_cell_t) "claim";
  args.ihandle = memory_ihandle;
  args.align = (align ? align : 1);
  args.size = size;
  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  *paddr = args.phys_low;

  return args.catch_result;
}
