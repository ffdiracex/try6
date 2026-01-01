/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	_SYS_ZIL_H
#define	_SYS_ZIL_H

/*
 * Intent log format:
 *
 * Each objset has its own intent log.  The log header (zil_header_t)
 * for objset N's intent log is kept in the Nth object of the SPA's
 * intent_log objset.  The log header points to a chain of log blocks,
 * each of which contains log records (i.e., transactions) followed by
 * a log block trailer (zil_trailer_t).  The format of a log record
 * depends on the record (or transaction) type, but all records begin
 * with a common structure that defines the type, length, and txg.
 */

/*
 * Intent log header - this on disk structure holds fields to manage
 * the log.  All fields are 64 bit to easily handle cross architectures.
 */
typedef struct zil_header {
	holy_uint64_t zh_claim_txg;   /* txg in which log blocks were claimed */
	holy_uint64_t zh_replay_seq;  /* highest replayed sequence number */
	blkptr_t zh_log;	/* log chain */
	holy_uint64_t zh_claim_seq;	/* highest claimed sequence number */
	holy_uint64_t zh_flags;	/* header flags */
	holy_uint64_t zh_pad[4];
} zil_header_t;

/*
 * zh_flags bit settings
 */
#define	ZIL_REPLAY_NEEDED 0x1	/* replay needed - internal only */

#endif	/* _SYS_ZIL_H */
