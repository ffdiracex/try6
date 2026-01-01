/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MACHINE_EMU
#error "This source is only meant for holy-emu platform"
#endif

#include <holy/cache.h>

#if defined(__ia64__)
#include "../ia64/cache.c"
#elif defined (__arm__) || defined (__aarch64__)

void __clear_cache (void *beg, void *end);

void
holy_arch_sync_caches (void *address, holy_size_t len)
{
  __clear_cache (address, (char *) address + len);
}

#elif defined (__mips__)
void _flush_cache (void *address, holy_size_t len, int type);

void
holy_arch_sync_caches (void *address, holy_size_t len)
{
  return _flush_cache (address, len, 0);
}

#endif

