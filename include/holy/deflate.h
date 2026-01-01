/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_DEFLATE_HEADER
#define holy_DEFLATE_HEADER 1

holy_ssize_t
holy_zlib_decompress (char *inbuf, holy_size_t insize, holy_off_t off,
		      char *outbuf, holy_size_t outsize);

holy_ssize_t
holy_deflate_decompress (char *inbuf, holy_size_t insize, holy_off_t off,
			 char *outbuf, holy_size_t outsize);

#endif
