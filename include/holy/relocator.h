/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_RELOCATOR_HEADER
#define holy_RELOCATOR_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/memory.h>
#include <holy/cpu/memory.h>

struct holy_relocator;
struct holy_relocator_chunk;
typedef const struct holy_relocator_chunk *holy_relocator_chunk_t;

struct holy_relocator *holy_relocator_new (void);

holy_err_t
holy_relocator_alloc_chunk_addr (struct holy_relocator *rel,
				 holy_relocator_chunk_t *out,
				 holy_phys_addr_t target, holy_size_t size);

void *
get_virtual_current_address (holy_relocator_chunk_t in);
holy_phys_addr_t
get_physical_target_address (holy_relocator_chunk_t in);

holy_err_t
holy_relocator_alloc_chunk_align (struct holy_relocator *rel,
				  holy_relocator_chunk_t *out,
				  holy_phys_addr_t min_addr,
				  holy_phys_addr_t max_addr,
				  holy_size_t size, holy_size_t align,
				  int preference,
				  int avoid_efi_boot_services);

#define holy_RELOCATOR_PREFERENCE_NONE 0
#define holy_RELOCATOR_PREFERENCE_LOW 1
#define holy_RELOCATOR_PREFERENCE_HIGH 2

void
holy_relocator_unload (struct holy_relocator *rel);

#endif
