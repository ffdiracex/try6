/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef G10_SECMEM_H
#define G10_SECMEM_H 1

void _gcry_secmem_init (size_t npool);
void _gcry_secmem_term (void);
void *_gcry_secmem_malloc (size_t size) _GCRY_GCC_ATTR_MALLOC;
void *_gcry_secmem_realloc (void *a, size_t newsize);
void _gcry_secmem_free (void *a);
void _gcry_secmem_dump_stats (void);
void _gcry_secmem_set_flags (unsigned flags);
unsigned _gcry_secmem_get_flags(void);
int _gcry_private_is_secure (const void *p);

/* Flags for _gcry_secmem_{set,get}_flags.  */
#define GCRY_SECMEM_FLAG_NO_WARNING      (1 << 0)
#define GCRY_SECMEM_FLAG_SUSPEND_WARNING (1 << 1)
#define GCRY_SECMEM_FLAG_NOT_LOCKED      (1 << 2)

#endif /* G10_SECMEM_H */
