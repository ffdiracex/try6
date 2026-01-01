/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CACHE_H
#define holy_CACHE_H	1

#include <holy/symbol.h>
#include <holy/types.h>

#if defined (__i386__) || defined (__x86_64__)
static inline void
holy_arch_sync_caches (void *address __attribute__ ((unused)),
		       holy_size_t len __attribute__ ((unused)))
{
}
#else
void EXPORT_FUNC(holy_arch_sync_caches) (void *address, holy_size_t len);
#endif

#ifndef holy_MACHINE_EMU
#ifdef _mips
void EXPORT_FUNC(holy_arch_sync_dma_caches) (volatile void *address,
					     holy_size_t len);
#else
static inline void
holy_arch_sync_dma_caches (volatile void *address __attribute__ ((unused)),
			   holy_size_t len __attribute__ ((unused)))
{
}
#endif
#endif

#ifdef __arm__
void
holy_arm_cache_enable (void);
#endif

#endif /* ! holy_CACHE_HEADER */
