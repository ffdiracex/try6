/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>

#include <holy/types.h>
#include <holy/err.h>
#include <holy/term.h>
#include <holy/xen.h>

#include <holy/xen/relocator.h>
#include <holy/relocator_private.h>

typedef holy_addr_t holy_xen_reg_t;

struct holy_relocator_xen_paging_area {
  holy_xen_reg_t start;
  holy_xen_reg_t size;
} holy_PACKED;

extern holy_uint8_t holy_relocator_xen_start;
extern holy_uint8_t holy_relocator_xen_end;
extern holy_uint8_t holy_relocator_xen_remap_start;
extern holy_uint8_t holy_relocator_xen_remap_end;
extern holy_xen_reg_t holy_relocator_xen_stack;
extern holy_xen_reg_t holy_relocator_xen_start_info;
extern holy_xen_reg_t holy_relocator_xen_entry_point;
extern holy_xen_reg_t holy_relocator_xen_remapper_virt;
extern holy_xen_reg_t holy_relocator_xen_remapper_virt2;
extern holy_xen_reg_t holy_relocator_xen_remapper_map;
extern holy_xen_reg_t holy_relocator_xen_mfn_list;
extern struct holy_relocator_xen_paging_area
  holy_relocator_xen_paging_areas[XEN_MAX_MAPPINGS];
extern holy_xen_reg_t holy_relocator_xen_remap_continue;
#ifdef __i386__
extern holy_xen_reg_t holy_relocator_xen_mmu_op_addr;
extern holy_xen_reg_t holy_relocator_xen_paging_areas_addr;
extern holy_xen_reg_t holy_relocator_xen_remapper_map_high;
#endif
extern mmuext_op_t holy_relocator_xen_mmu_op[3];

#define RELOCATOR_SIZEOF(x)	(&holy_relocator##x##_end - &holy_relocator##x##_start)

holy_err_t
holy_relocator_xen_boot (struct holy_relocator *rel,
			 struct holy_relocator_xen_state state,
			 holy_uint64_t remapper_pfn,
			 holy_addr_t remapper_virt,
			 holy_uint64_t trampoline_pfn,
			 holy_addr_t trampoline_virt)
{
  holy_err_t err;
  void *relst;
  int i;
  holy_relocator_chunk_t ch, ch_tramp;
  holy_xen_mfn_t *mfn_list =
    (holy_xen_mfn_t *) holy_xen_start_page_addr->mfn_list;

  err = holy_relocator_alloc_chunk_addr (rel, &ch, remapper_pfn << 12,
					 RELOCATOR_SIZEOF (_xen_remap));
  if (err)
    return err;
  err = holy_relocator_alloc_chunk_addr (rel, &ch_tramp, trampoline_pfn << 12,
					 RELOCATOR_SIZEOF (_xen));
  if (err)
    return err;

  holy_relocator_xen_stack = state.stack;
  holy_relocator_xen_start_info = state.start_info;
  holy_relocator_xen_entry_point = state.entry_point;
  for (i = 0; i < XEN_MAX_MAPPINGS; i++)
    {
      holy_relocator_xen_paging_areas[i].start = state.paging_start[i];
      holy_relocator_xen_paging_areas[i].size = state.paging_size[i];
    }
  holy_relocator_xen_remapper_virt = remapper_virt;
  holy_relocator_xen_remapper_virt2 = remapper_virt;
  holy_relocator_xen_remap_continue = trampoline_virt;

  holy_relocator_xen_remapper_map = (mfn_list[remapper_pfn] << 12) | 5;
#ifdef __i386__
  holy_relocator_xen_remapper_map_high = (mfn_list[remapper_pfn] >> 20);
  holy_relocator_xen_mmu_op_addr = (char *) &holy_relocator_xen_mmu_op
    - (char *) &holy_relocator_xen_remap_start + remapper_virt;
  holy_relocator_xen_paging_areas_addr =
    (char *) &holy_relocator_xen_paging_areas
    - (char *) &holy_relocator_xen_remap_start + remapper_virt;
#endif

  holy_relocator_xen_mfn_list = state.mfn_list;

  holy_memset (holy_relocator_xen_mmu_op, 0,
	       sizeof (holy_relocator_xen_mmu_op));
#ifdef __i386__
  holy_relocator_xen_mmu_op[0].cmd = MMUEXT_PIN_L3_TABLE;
#else
  holy_relocator_xen_mmu_op[0].cmd = MMUEXT_PIN_L4_TABLE;
#endif
  holy_relocator_xen_mmu_op[0].arg1.mfn = mfn_list[state.paging_start[0]];
  holy_relocator_xen_mmu_op[1].cmd = MMUEXT_NEW_BASEPTR;
  holy_relocator_xen_mmu_op[1].arg1.mfn = mfn_list[state.paging_start[0]];
  holy_relocator_xen_mmu_op[2].cmd = MMUEXT_UNPIN_TABLE;
  holy_relocator_xen_mmu_op[2].arg1.mfn =
    mfn_list[holy_xen_start_page_addr->pt_base >> 12];

  holy_memmove (get_virtual_current_address (ch),
		&holy_relocator_xen_remap_start,
		RELOCATOR_SIZEOF (_xen_remap));
  holy_memmove (get_virtual_current_address (ch_tramp),
		&holy_relocator_xen_start, RELOCATOR_SIZEOF (_xen));

  err = holy_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  ((void (*)(void)) relst) ();

  /* Not reached.  */
  return holy_ERR_NONE;
}
