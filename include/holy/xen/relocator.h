/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_RELOCATOR_XEN_HEADER
#define holy_RELOCATOR_XEN_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/relocator.h>

#define XEN_MAX_MAPPINGS 3

struct holy_relocator_xen_state
{
  holy_addr_t start_info;
  holy_addr_t paging_start[XEN_MAX_MAPPINGS];
  holy_addr_t paging_size[XEN_MAX_MAPPINGS];
  holy_addr_t mfn_list;
  holy_addr_t stack;
  holy_addr_t entry_point;
};

holy_err_t
holy_relocator_xen_boot (struct holy_relocator *rel,
			 struct holy_relocator_xen_state state,
			 holy_uint64_t remapper_pfn,
			 holy_addr_t remapper_virt,
			 holy_uint64_t trampoline_pfn,
			 holy_addr_t trampoline_virt);

#endif
