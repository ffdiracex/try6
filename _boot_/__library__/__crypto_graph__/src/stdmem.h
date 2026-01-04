/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef G10_STDMEM_H
#define G10_STDMEM_H 1

void _gcry_private_enable_m_guard(void);

void *_gcry_private_malloc (size_t n) _GCRY_GCC_ATTR_MALLOC;
void *_gcry_private_malloc_secure (size_t n) _GCRY_GCC_ATTR_MALLOC;
void *_gcry_private_realloc (void *a, size_t n);
void _gcry_private_check_heap (const void *a);
void _gcry_private_free (void *a);

#endif /* G10_STDMEM_H */
