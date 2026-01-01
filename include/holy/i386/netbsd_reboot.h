/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NETBSD_REBOOT_CPU_HEADER
#define holy_NETBSD_REBOOT_CPU_HEADER	1

#define NETBSD_RB_AUTOBOOT	0  /* flags for system auto-booting itself */

#define NETBSD_RB_ASKNAME	(1 << 0)  /* ask for file name to reboot from */
#define NETBSD_RB_SINGLE	(1 << 1)  /* reboot to single user only */
#define NETBSD_RB_NOSYNC	(1 << 2)  /* dont sync before reboot */
#define NETBSD_RB_HALT		(1 << 3)  /* don't reboot, just halt */
#define NETBSD_RB_INITNAME	(1 << 4)  /* name given for /etc/init (unused) */
#define NETBSD_RB_UNUSED1	(1 << 5)  /* was RB_DFLTROOT, obsolete */
#define NETBSD_RB_KDB		(1 << 6)  /* give control to kernel debugger */
#define NETBSD_RB_RDONLY	(1 << 7)  /* mount root fs read-only */
#define NETBSD_RB_DUMP		(1 << 8)  /* dump kernel memory before reboot */
#define NETBSD_RB_MINIROOT	(1 << 9)  /* mini-root present in memory at boot time */
#define NETBSD_RB_STRING	(1 << 10) /* use provided bootstr */
#define NETBSD_RB_POWERDOWN     ((1 << 11) | RB_HALT) /* turn power off (or at least halt) */
#define NETBSD_RB_USERCONFIG	(1 << 12) /* change configured devices */

#define NETBSD_AB_NORMAL	0  /* boot normally (default) */

#define NETBSD_AB_QUIET		(1 << 16) /* boot quietly */
#define NETBSD_AB_VERBOSE	(1 << 17) /* boot verbosely */
#define NETBSD_AB_SILENT	(1 << 18) /* boot silently */
#define NETBSD_AB_DEBUG		(1 << 19) /* boot with debug messages */
#define NETBSD_AB_NOSMP		(1 << 28) /* Boot without SMP support.  */
#define NETBSD_AB_NOACPI        (1 << 29) /* Boot without ACPI support.  */


#endif
