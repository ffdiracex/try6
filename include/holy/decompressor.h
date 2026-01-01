/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_DECOMPRESSOR_HEADER
#define holy_DECOMPRESSOR_HEADER	1

void
holy_decompress_core (void *src, void *dst, unsigned long srcsize,
		      unsigned long dstsize);

void
find_scratch (void *src, void *dst, unsigned long srcsize,
	      unsigned long dstsize);

#define holy_DECOMPRESSOR_DICT_SIZE (1 << 16)

extern void *holy_decompressor_scratch;

#endif
