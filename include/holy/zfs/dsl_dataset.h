/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	_SYS_DSL_DATASET_H
#define	_SYS_DSL_DATASET_H

typedef struct dsl_dataset_phys {
	holy_uint64_t ds_dir_obj;
	holy_uint64_t ds_prev_snap_obj;
	holy_uint64_t ds_prev_snap_txg;
	holy_uint64_t ds_next_snap_obj;
	holy_uint64_t ds_snapnames_zapobj;	/* zap obj of snaps; ==0 for snaps */
	holy_uint64_t ds_num_children;	/* clone/snap children; ==0 for head */
	holy_uint64_t ds_creation_time;	/* seconds since 1970 */
	holy_uint64_t ds_creation_txg;
	holy_uint64_t ds_deadlist_obj;
	holy_uint64_t ds_used_bytes;
	holy_uint64_t ds_compressed_bytes;
	holy_uint64_t ds_uncompressed_bytes;
	holy_uint64_t ds_unique_bytes;	/* only relevant to snapshots */
	/*
	 * The ds_fsid_guid is a 56-bit ID that can change to avoid
	 * collisions.  The ds_guid is a 64-bit ID that will never
	 * change, so there is a small probability that it will collide.
	 */
	holy_uint64_t ds_fsid_guid;
	holy_uint64_t ds_guid;
	holy_uint64_t ds_flags;
	blkptr_t ds_bp;
	holy_uint64_t ds_pad[8]; /* pad out to 320 bytes for good measure */
} dsl_dataset_phys_t;

#endif /* _SYS_DSL_DATASET_H */
