/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_IEEE1275_MACHINE_HEADER
#define holy_IEEE1275_MACHINE_HEADER	1

#include <holy/types.h>

#define holy_IEEE1275_CELL_SIZEOF 8
typedef holy_uint64_t holy_ieee1275_cell_t;

/* Encoding of 'mode' argument to holy_ieee1275_map_physical() */
#define IEEE1275_MAP_WRITE	0x0001 /* Writable */
#define IEEE1275_MAP_READ	0x0002 /* Readable */
#define IEEE1275_MAP_EXEC	0x0004 /* Executable */
#define IEEE1275_MAP_LOCKED	0x0010 /* Locked in TLB */
#define IEEE1275_MAP_CACHED	0x0020 /* Cacheable */
#define IEEE1275_MAP_SE		0x0040 /* Side-effects */
#define IEEE1275_MAP_GLOBAL	0x0080 /* Global */
#define IEEE1275_MAP_IE		0x0100 /* Invert Endianness */
#define IEEE1275_MAP_DEFAULT	(IEEE1275_MAP_WRITE | IEEE1275_MAP_READ | \
				 IEEE1275_MAP_EXEC | IEEE1275_MAP_CACHED)

extern int EXPORT_FUNC(holy_ieee1275_claim_vaddr) (holy_addr_t vaddr,
						   holy_size_t size);
extern int EXPORT_FUNC(holy_ieee1275_alloc_physmem) (holy_addr_t *paddr,
						     holy_size_t size,
						     holy_uint32_t align);

extern holy_addr_t EXPORT_VAR (holy_ieee1275_original_stack);

#endif /* ! holy_IEEE1275_MACHINE_HEADER */
