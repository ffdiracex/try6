/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	_SYS_SA_IMPL_H
#define	_SYS_SA_IMPL_H

typedef struct sa_hdr_phys {
	holy_uint32_t sa_magic;
	holy_uint16_t sa_layout_info;
	holy_uint16_t sa_lengths[1];
} sa_hdr_phys_t;

#define	SA_HDR_SIZE(hdr)	BF32_GET_SB(hdr->sa_layout_info, 10, 16, 3, 0)
#define	SA_TYPE_OFFSET	0x0
#define	SA_SIZE_OFFSET	0x8
#define	SA_MTIME_OFFSET	0x38
#define SA_SYMLINK_OFFSET 0xa0

#endif	/* _SYS_SA_IMPL_H */
