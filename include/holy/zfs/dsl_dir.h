/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	_SYS_DSL_DIR_H
#define	_SYS_DSL_DIR_H

typedef struct dsl_dir_phys {
	holy_uint64_t dd_creation_time; /* not actually used */
	holy_uint64_t dd_head_dataset_obj;
	holy_uint64_t dd_parent_obj;
	holy_uint64_t dd_clone_parent_obj;
	holy_uint64_t dd_child_dir_zapobj;
	/*
	 * how much space our children are accounting for; for leaf
	 * datasets, == physical space used by fs + snaps
	 */
	holy_uint64_t dd_used_bytes;
	holy_uint64_t dd_compressed_bytes;
	holy_uint64_t dd_uncompressed_bytes;
	/* Administrative quota setting */
	holy_uint64_t dd_quota;
	/* Administrative reservation setting */
	holy_uint64_t dd_reserved;
	holy_uint64_t dd_props_zapobj;
	holy_uint64_t dd_deleg_zapobj;	/* dataset permissions */
	holy_uint64_t unused[7];
        holy_uint64_t keychain;
	holy_uint64_t unused2[12];
} dsl_dir_phys_t;

#endif /* _SYS_DSL_DIR_H */
