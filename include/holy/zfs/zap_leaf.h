/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	_SYS_ZAP_LEAF_H
#define	_SYS_ZAP_LEAF_H

#define	ZAP_LEAF_MAGIC 0x2AB1EAF

/* chunk size = 24 bytes */
#define	ZAP_LEAF_CHUNKSIZE 24

/*
 * The amount of space within the chunk available for the array is:
 * chunk size - space for type (1) - space for next pointer (2)
 */
#define	ZAP_LEAF_ARRAY_BYTES (ZAP_LEAF_CHUNKSIZE - 3)

typedef enum zap_chunk_type {
	ZAP_CHUNK_FREE = 253,
	ZAP_CHUNK_ENTRY = 252,
	ZAP_CHUNK_ARRAY = 251,
	ZAP_CHUNK_TYPE_MAX = 250
} zap_chunk_type_t;

/*
 * TAKE NOTE:
 * If zap_leaf_phys_t is modified, zap_leaf_byteswap() must be modified.
 */
typedef struct zap_leaf_phys {
	struct zap_leaf_header {
		holy_uint64_t lh_block_type;		/* ZBT_LEAF */
		holy_uint64_t lh_pad1;
		holy_uint64_t lh_prefix;		/* hash prefix of this leaf */
		holy_uint32_t lh_magic;		/* ZAP_LEAF_MAGIC */
		holy_uint16_t lh_nfree;		/* number free chunks */
		holy_uint16_t lh_nentries;		/* number of entries */
		holy_uint16_t lh_prefix_len;		/* num bits used to id this */

/* above is accessable to zap, below is zap_leaf private */

		holy_uint16_t lh_freelist;		/* chunk head of free list */
		holy_uint8_t lh_pad2[12];
	} l_hdr; /* 2 24-byte chunks */

	/*
	 * The header is followed by a hash table with
	 * ZAP_LEAF_HASH_NUMENTRIES(zap) entries.  The hash table is
	 * followed by an array of ZAP_LEAF_NUMCHUNKS(zap)
	 * zap_leaf_chunk structures.  These structures are accessed
	 * with the ZAP_LEAF_CHUNK() macro.
	 */

	holy_uint16_t l_hash[0];
        holy_properly_aligned_t l_entries[0];
} zap_leaf_phys_t;

typedef union zap_leaf_chunk {
	struct zap_leaf_entry {
		holy_uint8_t le_type; 		/* always ZAP_CHUNK_ENTRY */
		holy_uint8_t le_int_size;		/* size of ints */
		holy_uint16_t le_next;		/* next entry in hash chain */
		holy_uint16_t le_name_chunk;		/* first chunk of the name */
		holy_uint16_t le_name_length;	/* bytes in name, incl null */
		holy_uint16_t le_value_chunk;	/* first chunk of the value */
		holy_uint16_t le_value_length;	/* value length in ints */
		holy_uint32_t le_cd;		/* collision differentiator */
		holy_uint64_t le_hash;		/* hash value of the name */
	} l_entry;
	struct zap_leaf_array {
		holy_uint8_t la_type;		/* always ZAP_CHUNK_ARRAY */
		union
		{
			holy_uint8_t la_array[ZAP_LEAF_ARRAY_BYTES];
			holy_uint64_t la_array64;
		} holy_PACKED;
		holy_uint16_t la_next;		/* next blk or CHAIN_END */
	} l_array;
	struct zap_leaf_free {
		holy_uint8_t lf_type;		/* always ZAP_CHUNK_FREE */
		holy_uint8_t lf_pad[ZAP_LEAF_ARRAY_BYTES];
		holy_uint16_t lf_next;	/* next in free list, or CHAIN_END */
	} l_free;
} zap_leaf_chunk_t;

#endif /* _SYS_ZAP_LEAF_H */
