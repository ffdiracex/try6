/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _SYS_ZIO_CHECKSUM_H
#define	_SYS_ZIO_CHECKSUM_H

extern void zio_checksum_SHA256 (const void *, holy_uint64_t,
				 holy_zfs_endian_t endian, zio_cksum_t *);
extern void fletcher_2 (const void *, holy_uint64_t, holy_zfs_endian_t endian,
			zio_cksum_t *);
extern void fletcher_4 (const void *, holy_uint64_t, holy_zfs_endian_t endian,
			zio_cksum_t *);

#endif	/* _SYS_ZIO_CHECKSUM_H */
