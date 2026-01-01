/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	_SYS_ZAP_IMPL_H
#define	_SYS_ZAP_IMPL_H

#define	ZAP_MAGIC 0x2F52AB2ABULL

#define	ZAP_HASHBITS		28
#define	MZAP_ENT_LEN		64
#define	MZAP_NAME_LEN		(MZAP_ENT_LEN - 8 - 4 - 2)

typedef struct mzap_ent_phys {
	holy_uint64_t mze_value;
	holy_uint32_t mze_cd;
	holy_uint16_t mze_pad;	/* in case we want to chain them someday */
	char mze_name[MZAP_NAME_LEN];
} mzap_ent_phys_t;

typedef struct mzap_phys {
	holy_uint64_t mz_block_type;	/* ZBT_MICRO */
	holy_uint64_t mz_salt;
	holy_uint64_t mz_pad[6];
	mzap_ent_phys_t mz_chunk[1];
	/* actually variable size depending on block size */
} mzap_phys_t;

/*
 * The (fat) zap is stored in one object. It is an array of
 * 1<<FZAP_BLOCK_SHIFT byte blocks. The layout looks like one of:
 *
 * ptrtbl fits in first block:
 * 	[zap_phys_t zap_ptrtbl_shift < 6] [zap_leaf_t] ...
 *
 * ptrtbl too big for first block:
 * 	[zap_phys_t zap_ptrtbl_shift >= 6] [zap_leaf_t] [ptrtbl] ...
 *
 */

#define	ZBT_LEAF		((1ULL << 63) + 0)
#define	ZBT_HEADER		((1ULL << 63) + 1)
#define	ZBT_MICRO		((1ULL << 63) + 3)
/* any other values are ptrtbl blocks */

/*
 * the embedded pointer table takes up half a block:
 * block size / entry size (2^3) / 2
 */
#define	ZAP_EMBEDDED_PTRTBL_SHIFT(zap) (FZAP_BLOCK_SHIFT(zap) - 3 - 1)

/*
 * The embedded pointer table starts half-way through the block.  Since
 * the pointer table itself is half the block, it starts at (64-bit)
 * word number (1<<ZAP_EMBEDDED_PTRTBL_SHIFT(zap)).
 */
#define	ZAP_EMBEDDED_PTRTBL_ENT(zap, idx) \
	((holy_uint64_t *)(zap)->zap_f.zap_phys) \
	[(idx) + (1<<ZAP_EMBEDDED_PTRTBL_SHIFT(zap))]

/*
 * TAKE NOTE:
 * If zap_phys_t is modified, zap_byteswap() must be modified.
 */
typedef struct zap_phys {
	holy_uint64_t zap_block_type;	/* ZBT_HEADER */
	holy_uint64_t zap_magic;		/* ZAP_MAGIC */

	struct zap_table_phys {
		holy_uint64_t zt_blk;	/* starting block number */
		holy_uint64_t zt_numblks;	/* number of blocks */
		holy_uint64_t zt_shift;	/* bits to index it */
		holy_uint64_t zt_nextblk;	/* next (larger) copy start block */
		holy_uint64_t zt_blks_copied; /* number source blocks copied */
	} zap_ptrtbl;

	holy_uint64_t zap_freeblk;		/* the next free block */
	holy_uint64_t zap_num_leafs;		/* number of leafs */
	holy_uint64_t zap_num_entries;	/* number of entries */
	holy_uint64_t zap_salt;		/* salt to stir into hash function */
	holy_uint64_t zap_normflags;		/* flags for u8_textprep_str() */
	holy_uint64_t zap_flags;		/* zap_flag_t */
	/*
	 * This structure is followed by padding, and then the embedded
	 * pointer table.  The embedded pointer table takes up second
	 * half of the block.  It is accessed using the
	 * ZAP_EMBEDDED_PTRTBL_ENT() macro.
	 */
} zap_phys_t;

#endif /* _SYS_ZAP_IMPL_H */
