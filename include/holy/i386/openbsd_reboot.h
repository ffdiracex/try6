/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_OPENBSD_REBOOT_CPU_HEADER
#define holy_OPENBSD_REBOOT_CPU_HEADER	1

#define OPENBSD_RB_ASKNAME	(1 << 0)  /* ask for file name to reboot from */
#define OPENBSD_RB_SINGLE	(1 << 1)  /* reboot to single user only */
#define OPENBSD_RB_NOSYNC	(1 << 2)  /* dont sync before reboot */
#define OPENBSD_RB_HALT		(1 << 3)  /* don't reboot, just halt */
#define OPENBSD_RB_INITNAME	(1 << 4)  /* name given for /etc/init (unused) */
#define OPENBSD_RB_DFLTROOT	(1 << 5)  /* use compiled-in rootdev */
#define OPENBSD_RB_KDB		(1 << 6)  /* give control to kernel debugger */
#define OPENBSD_RB_RDONLY	(1 << 7)  /* mount root fs read-only */
#define OPENBSD_RB_DUMP		(1 << 8)  /* dump kernel memory before reboot */
#define OPENBSD_RB_MINIROOT	(1 << 9)  /* mini-root present in memory at boot time */
#define OPENBSD_RB_CONFIG	(1 << 10) /* change configured devices */
#define OPENBSD_RB_TIMEBAD	(1 << 11) /* don't call resettodr() in boot() */
#define OPENBSD_RB_POWERDOWN	(1 << 12) /* attempt to power down machine */
#define OPENBSD_RB_SERCONS	(1 << 13) /* use serial console if available */
#define OPENBSD_RB_USERREQ	(1 << 14) /* boot() called at user request (e.g. ddb) */

#define OPENBSD_B_DEVMAGIC	0xa0000000
#define OPENBSD_B_ADAPTORSHIFT	24
#define OPENBSD_B_CTRLSHIFT	20
#define OPENBSD_B_UNITSHIFT	16
#define OPENBSD_B_PARTSHIFT	8
#define OPENBSD_B_TYPESHIFT	0

#endif
