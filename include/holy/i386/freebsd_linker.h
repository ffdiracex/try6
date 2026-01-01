/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FREEBSD_LINKER_CPU_HEADER
#define holy_FREEBSD_LINKER_CPU_HEADER	1

#define FREEBSD_MODINFO_END		0x0000	/* End of list */
#define FREEBSD_MODINFO_NAME		0x0001	/* Name of module (string) */
#define FREEBSD_MODINFO_TYPE		0x0002	/* Type of module (string) */
#define FREEBSD_MODINFO_ADDR		0x0003	/* Loaded address */
#define FREEBSD_MODINFO_SIZE		0x0004	/* Size of module */
#define FREEBSD_MODINFO_EMPTY		0x0005	/* Has been deleted */
#define FREEBSD_MODINFO_ARGS		0x0006	/* Parameters string */
#define FREEBSD_MODINFO_METADATA	0x8000	/* Module-specfic */

#define FREEBSD_MODINFOMD_AOUTEXEC	0x0001	/* a.out exec header */
#define FREEBSD_MODINFOMD_ELFHDR	0x0002	/* ELF header */
#define FREEBSD_MODINFOMD_SSYM		0x0003	/* start of symbols */
#define FREEBSD_MODINFOMD_ESYM		0x0004	/* end of symbols */
#define FREEBSD_MODINFOMD_DYNAMIC	0x0005	/* _DYNAMIC pointer */
#define FREEBSD_MODINFOMD_ENVP		0x0006	/* envp[] */
#define FREEBSD_MODINFOMD_HOWTO		0x0007	/* boothowto */
#define FREEBSD_MODINFOMD_KERNEND	0x0008	/* kernend */
#define FREEBSD_MODINFOMD_SHDR		0x0009	/* section header table */
#define FREEBSD_MODINFOMD_NOCOPY	0x8000	/* don't copy this metadata to the kernel */

#define FREEBSD_MODINFOMD_SMAP		0x1001

#define FREEBSD_MODINFOMD_DEPLIST	(0x4001 | FREEBSD_MODINFOMD_NOCOPY)  /* depends on */

#endif
