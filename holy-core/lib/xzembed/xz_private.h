/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef XZ_PRIVATE_H
#define XZ_PRIVATE_H

/*
 * For userspace builds, use a separate header to define the required
 * macros and functions. This makes it easier to adapt the code into
 * different environments and avoids clutter in the Linux kernel tree.
 */
#include "xz_config.h"

/*
 * If any of the BCJ filter decoders are wanted, define XZ_DEC_BCJ.
 * XZ_DEC_BCJ is used to enable generic support for BCJ decoders.
 */
#ifndef XZ_DEC_BCJ
#	if defined(XZ_DEC_X86) || defined(XZ_DEC_POWERPC) \
			|| defined(XZ_DEC_IA64) || defined(XZ_DEC_ARM) \
			|| defined(XZ_DEC_ARM) || defined(XZ_DEC_ARMTHUMB) \
			|| defined(XZ_DEC_SPARC)
#		define XZ_DEC_BCJ
#	endif
#endif

/*
 * Allocate memory for LZMA2 decoder. xz_dec_lzma2_reset() must be used
 * before calling xz_dec_lzma2_run().
 */
struct xz_dec_lzma2 * xz_dec_lzma2_create(
		uint32_t dict_max);

/*
 * Decode the LZMA2 properties (one byte) and reset the decoder. Return
 * XZ_OK on success, XZ_MEMLIMIT_ERROR if the preallocated dictionary is not
 * big enough, and XZ_OPTIONS_ERROR if props indicates something that this
 * decoder doesn't support.
 */
enum xz_ret xz_dec_lzma2_reset(
		struct xz_dec_lzma2 *s, uint8_t props);

/* Decode raw LZMA2 stream from b->in to b->out. */
enum xz_ret xz_dec_lzma2_run(
		struct xz_dec_lzma2 *s, struct xz_buf *b);

/* Free the memory allocated for the LZMA2 decoder. */
void xz_dec_lzma2_end(struct xz_dec_lzma2 *s);

/*
 * Allocate memory for BCJ decoders. xz_dec_bcj_reset() must be used before
 * calling xz_dec_bcj_run().
 */
struct xz_dec_bcj * xz_dec_bcj_create(bool single_call);

/*
 * Decode the Filter ID of a BCJ filter. This implementation doesn't
 * support custom start offsets, so no decoding of Filter Properties
 * is needed. Returns XZ_OK if the given Filter ID is supported.
 * Otherwise XZ_OPTIONS_ERROR is returned.
 */
enum xz_ret xz_dec_bcj_reset(
		struct xz_dec_bcj *s, uint8_t id);

/*
 * Decode raw BCJ + LZMA2 stream. This must be used only if there actually is
 * a BCJ filter in the chain. If the chain has only LZMA2, xz_dec_lzma2_run()
 * must be called directly.
 */
enum xz_ret xz_dec_bcj_run(struct xz_dec_bcj *s,
		struct xz_dec_lzma2 *lzma2, struct xz_buf *b);

/* Free the memory allocated for the BCJ filters. */
#define xz_dec_bcj_end(s) kfree(s)

#endif
