/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	_SYS_FS_ZFS_ACL_H
#define	_SYS_FS_ZFS_ACL_H

#ifndef _UID_T
#define	_UID_T
typedef	unsigned int uid_t;			/* UID type */
#endif /* _UID_T */

typedef struct zfs_oldace {
	holy_uint32_t	z_fuid;		/* "who" */
	holy_uint32_t	z_access_mask;  /* access mask */
	holy_uint16_t	z_flags;	/* flags, i.e inheritance */
	holy_uint16_t	z_type;		/* type of entry allow/deny */
} zfs_oldace_t;

#define	ACE_SLOT_CNT	6

typedef struct zfs_znode_acl_v0 {
	holy_uint64_t	z_acl_extern_obj;	  /* ext acl pieces */
	holy_uint32_t	z_acl_count;		  /* Number of ACEs */
	holy_uint16_t	z_acl_version;		  /* acl version */
	holy_uint16_t	z_acl_pad;		  /* pad */
	zfs_oldace_t	z_ace_data[ACE_SLOT_CNT]; /* 6 standard ACEs */
} zfs_znode_acl_v0_t;

#define	ZFS_ACE_SPACE	(sizeof (zfs_oldace_t) * ACE_SLOT_CNT)

typedef struct zfs_znode_acl {
	holy_uint64_t	z_acl_extern_obj;	  /* ext acl pieces */
	holy_uint32_t	z_acl_size;		  /* Number of bytes in ACL */
	holy_uint16_t	z_acl_version;		  /* acl version */
	holy_uint16_t	z_acl_count;		  /* ace count */
	holy_uint8_t	z_ace_data[ZFS_ACE_SPACE]; /* space for embedded ACEs */
} zfs_znode_acl_t;


#endif	/* _SYS_FS_ZFS_ACL_H */
