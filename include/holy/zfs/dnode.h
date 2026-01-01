/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	_SYS_DNODE_H
#define	_SYS_DNODE_H

#include <holy/zfs/spa.h>

/*
 * Fixed constants.
 */
#define	DNODE_SHIFT		9	/* 512 bytes */
#define	DNODE_BLOCK_SHIFT	14	/* 16k */
#define	DNODE_CORE_SIZE		64	/* 64 bytes for dnode sans blkptrs */

/*
 * Derived constants.
 */
#define	DNODE_SIZE	(1 << DNODE_SHIFT)
#define	DN_MAX_NBLKPTR	((DNODE_SIZE - DNODE_CORE_SIZE) >> SPA_BLKPTRSHIFT)
#define	DN_MAX_BONUSLEN	(DNODE_SIZE - DNODE_CORE_SIZE - (1 << SPA_BLKPTRSHIFT))

#define	DNODES_PER_BLOCK_SHIFT	(DNODE_BLOCK_SHIFT - DNODE_SHIFT)
#define	DNODES_PER_BLOCK	(1ULL << DNODES_PER_BLOCK_SHIFT)

#define	DNODE_FLAG_SPILL_BLKPTR (1<<2)

#define	DN_BONUS(dnp)	((void*)((dnp)->dn_bonus + \
	(((dnp)->dn_nblkptr - 1) * sizeof (blkptr_t))))

typedef struct dnode_phys {
	holy_uint8_t dn_type;		/* dmu_object_type_t */
	holy_uint8_t dn_indblkshift;		/* ln2(indirect block size) */
	holy_uint8_t dn_nlevels;		/* 1=dn_blkptr->data blocks */
	holy_uint8_t dn_nblkptr;		/* length of dn_blkptr */
	holy_uint8_t dn_bonustype;		/* type of data in bonus buffer */
	holy_uint8_t	dn_checksum;		/* ZIO_CHECKSUM type */
	holy_uint8_t	dn_compress;		/* ZIO_COMPRESS type */
	holy_uint8_t dn_flags;		/* DNODE_FLAG_* */
	holy_uint16_t dn_datablkszsec;	/* data block size in 512b sectors */
	holy_uint16_t dn_bonuslen;		/* length of dn_bonus */
	holy_uint8_t dn_pad2[4];

	/* accounting is protected by dn_dirty_mtx */
	holy_uint64_t dn_maxblkid;		/* largest allocated block ID */
	holy_uint64_t dn_used;		/* bytes (or sectors) of disk space */

	holy_uint64_t dn_pad3[4];

	blkptr_t dn_blkptr[1];
	holy_uint8_t dn_bonus[DN_MAX_BONUSLEN - sizeof (blkptr_t)];
	blkptr_t dn_spill;
} dnode_phys_t;

#endif	/* _SYS_DNODE_H */
