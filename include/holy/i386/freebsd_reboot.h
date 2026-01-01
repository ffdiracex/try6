/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FREEBSD_REBOOT_CPU_HEADER
#define holy_FREEBSD_REBOOT_CPU_HEADER	1

#define FREEBSD_RB_ASKNAME	(1 << 0)  /* ask for file name to reboot from */
#define FREEBSD_RB_SINGLE       (1 << 1)  /* reboot to single user only */
#define FREEBSD_RB_NOSYNC       (1 << 2)  /* dont sync before reboot */
#define FREEBSD_RB_HALT         (1 << 3)  /* don't reboot, just halt */
#define FREEBSD_RB_INITNAME     (1 << 4)  /* name given for /etc/init (unused) */
#define FREEBSD_RB_DFLTROOT     (1 << 5)  /* use compiled-in rootdev */
#define FREEBSD_RB_KDB          (1 << 6)  /* give control to kernel debugger */
#define FREEBSD_RB_RDONLY       (1 << 7)  /* mount root fs read-only */
#define FREEBSD_RB_DUMP         (1 << 8)  /* dump kernel memory before reboot */
#define FREEBSD_RB_MINIROOT     (1 << 9)  /* mini-root present in memory at boot time */
#define FREEBSD_RB_CONFIG       (1 << 10) /* invoke user configuration routing */
#define FREEBSD_RB_VERBOSE      (1 << 11) /* print all potentially useful info */
#define FREEBSD_RB_SERIAL       (1 << 12) /* user serial port as console */
#define FREEBSD_RB_CDROM        (1 << 13) /* use cdrom as root */
#define FREEBSD_RB_GDB		(1 << 15) /* use GDB remote debugger instead of DDB */
#define FREEBSD_RB_MUTE		(1 << 16) /* Come up with the console muted */
#define FREEBSD_RB_PAUSE	(1 << 20)
#define FREEBSD_RB_QUIET	(1 << 21)
#define FREEBSD_RB_NOINTR	(1 << 28)
#define FREENSD_RB_MULTIPLE	(1 << 29)  /* Use multiple consoles */
#define FREEBSD_RB_DUAL		FREENSD_RB_MULTIPLE
#define FREEBSD_RB_BOOTINFO     (1 << 31) /* have `struct bootinfo *' arg */

#endif
