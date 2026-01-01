/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __holy_UBOOT_IMAGE_H__
#define __holy_UBOOT_IMAGE_H__

/*
 * Operating System Codes
 */
#define holy_UBOOT_IH_OS_INVALID		0	/* Invalid OS	*/
#define holy_UBOOT_IH_OS_OPENBSD		1	/* OpenBSD	*/
#define holy_UBOOT_IH_OS_NETBSD		2	/* NetBSD	*/
#define holy_UBOOT_IH_OS_FREEBSD		3	/* FreeBSD	*/
#define holy_UBOOT_IH_OS_4_4BSD		4	/* 4.4BSD	*/
#define holy_UBOOT_IH_OS_LINUX		5	/* Linux	*/
#define holy_UBOOT_IH_OS_SVR4		6	/* SVR4		*/
#define holy_UBOOT_IH_OS_ESIX		7	/* Esix		*/
#define holy_UBOOT_IH_OS_SOLARIS		8	/* Solaris	*/
#define holy_UBOOT_IH_OS_IRIX		9	/* Irix		*/
#define holy_UBOOT_IH_OS_SCO		10	/* SCO		*/
#define holy_UBOOT_IH_OS_DELL		11	/* Dell		*/
#define holy_UBOOT_IH_OS_NCR		12	/* NCR		*/
#define holy_UBOOT_IH_OS_LYNXOS		13	/* LynxOS	*/
#define holy_UBOOT_IH_OS_VXWORKS		14	/* VxWorks	*/
#define holy_UBOOT_IH_OS_PSOS		15	/* pSOS		*/
#define holy_UBOOT_IH_OS_QNX		16	/* QNX		*/
#define holy_UBOOT_IH_OS_U_BOOT		17	/* Firmware	*/
#define holy_UBOOT_IH_OS_RTEMS		18	/* RTEMS	*/
#define holy_UBOOT_IH_OS_ARTOS		19	/* ARTOS	*/
#define holy_UBOOT_IH_OS_UNITY		20	/* Unity OS	*/
#define holy_UBOOT_IH_OS_INTEGRITY		21	/* INTEGRITY	*/
#define holy_UBOOT_IH_OS_OSE		22	/* OSE		*/

/*
 * CPU Architecture Codes (supported by Linux)
 */
#define holy_UBOOT_IH_ARCH_INVALID		0	/* Invalid CPU	*/
#define holy_UBOOT_IH_ARCH_ALPHA		1	/* Alpha	*/
#define holy_UBOOT_IH_ARCH_ARM		2	/* ARM		*/
#define holy_UBOOT_IH_ARCH_I386		3	/* Intel x86	*/
#define holy_UBOOT_IH_ARCH_IA64		4	/* IA64		*/
#define holy_UBOOT_IH_ARCH_MIPS		5	/* MIPS		*/
#define holy_UBOOT_IH_ARCH_MIPS64		6	/* MIPS	 64 Bit */
#define holy_UBOOT_IH_ARCH_PPC		7	/* PowerPC	*/
#define holy_UBOOT_IH_ARCH_S390		8	/* IBM S390	*/
#define holy_UBOOT_IH_ARCH_SH		9	/* SuperH	*/
#define holy_UBOOT_IH_ARCH_SPARC		10	/* Sparc	*/
#define holy_UBOOT_IH_ARCH_SPARC64		11	/* Sparc 64 Bit */
#define holy_UBOOT_IH_ARCH_M68K		12	/* M68K		*/
#define holy_UBOOT_IH_ARCH_MICROBLAZE	14	/* MicroBlaze   */
#define holy_UBOOT_IH_ARCH_NIOS2		15	/* Nios-II	*/
#define holy_UBOOT_IH_ARCH_BLACKFIN	16	/* Blackfin	*/
#define holy_UBOOT_IH_ARCH_AVR32		17	/* AVR32	*/
#define holy_UBOOT_IH_ARCH_ST200	        18	/* STMicroelectronics ST200  */
#define holy_UBOOT_IH_ARCH_SANDBOX		19	/* Sandbox architecture (test only) */
#define holy_UBOOT_IH_ARCH_NDS32	        20	/* ANDES Technology - NDS32  */
#define holy_UBOOT_IH_ARCH_OPENRISC        21	/* OpenRISC 1000  */

/*
 * Image Types
 *
 * "Standalone Programs" are directly runnable in the environment
 *	provided by U-Boot; it is expected that (if they behave
 *	well) you can continue to work in U-Boot after return from
 *	the Standalone Program.
 * "OS Kernel Images" are usually images of some Embedded OS which
 *	will take over control completely. Usually these programs
 *	will install their own set of exception handlers, device
 *	drivers, set up the MMU, etc. - this means, that you cannot
 *	expect to re-enter U-Boot except by resetting the CPU.
 * "RAMDisk Images" are more or less just data blocks, and their
 *	parameters (address, size) are passed to an OS kernel that is
 *	being started.
 * "Multi-File Images" contain several images, typically an OS
 *	(Linux) kernel image and one or more data images like
 *	RAMDisks. This construct is useful for instance when you want
 *	to boot over the network using BOOTP etc., where the boot
 *	server provides just a single image file, but you want to get
 *	for instance an OS kernel and a RAMDisk image.
 *
 *	"Multi-File Images" start with a list of image sizes, each
 *	image size (in bytes) specified by an "uint32_t" in network
 *	byte order. This list is terminated by an "(uint32_t)0".
 *	Immediately after the terminating 0 follow the images, one by
 *	one, all aligned on "uint32_t" boundaries (size rounded up to
 *	a multiple of 4 bytes - except for the last file).
 *
 * "Firmware Images" are binary images containing firmware (like
 *	U-Boot or FPGA images) which usually will be programmed to
 *	flash memory.
 *
 * "Script files" are command sequences that will be executed by
 *	U-Boot's command interpreter; this feature is especially
 *	useful when you configure U-Boot to use a real shell (hush)
 *	as command interpreter (=> Shell Scripts).
 */

#define holy_UBOOT_IH_TYPE_INVALID		0	/* Invalid Image		*/
#define holy_UBOOT_IH_TYPE_STANDALONE	1	/* Standalone Program		*/
#define holy_UBOOT_IH_TYPE_KERNEL		2	/* OS Kernel Image		*/
#define holy_UBOOT_IH_TYPE_RAMDISK		3	/* RAMDisk Image		*/
#define holy_UBOOT_IH_TYPE_MULTI		4	/* Multi-File Image		*/
#define holy_UBOOT_IH_TYPE_FIRMWARE	5	/* Firmware Image		*/
#define holy_UBOOT_IH_TYPE_SCRIPT		6	/* Script file			*/
#define holy_UBOOT_IH_TYPE_FILESYSTEM	7	/* Filesystem Image (any type)	*/
#define holy_UBOOT_IH_TYPE_FLATDT		8	/* Binary Flat Device Tree Blob	*/
#define holy_UBOOT_IH_TYPE_KWBIMAGE	9	/* Kirkwood Boot Image		*/
#define holy_UBOOT_IH_TYPE_IMXIMAGE	10	/* Freescale IMXBoot Image	*/
#define holy_UBOOT_IH_TYPE_UBLIMAGE	11	/* Davinci UBL Image		*/
#define holy_UBOOT_IH_TYPE_OMAPIMAGE	12	/* TI OMAP Config Header Image	*/
#define holy_UBOOT_IH_TYPE_AISIMAGE	13	/* TI Davinci AIS Image		*/
#define holy_UBOOT_IH_TYPE_KERNEL_NOLOAD	14	/* OS Kernel Image, can run from any load address */
#define holy_UBOOT_IH_TYPE_PBLIMAGE	15	/* Freescale PBL Boot Image	*/

/*
 * Compression Types
 */
#define holy_UBOOT_IH_COMP_NONE		0	/*  No	 Compression Used	*/
#define holy_UBOOT_IH_COMP_GZIP		1	/* gzip	 Compression Used	*/
#define holy_UBOOT_IH_COMP_BZIP2		2	/* bzip2 Compression Used	*/
#define holy_UBOOT_IH_COMP_LZMA		3	/* lzma  Compression Used	*/
#define holy_UBOOT_IH_COMP_LZO		4	/* lzo   Compression Used	*/

#define holy_UBOOT_IH_MAGIC	0x27051956	/* Image Magic Number		*/
#define holy_UBOOT_IH_NMLEN		32	/* Image Name Length		*/

/*
 * Legacy format image header,
 * all data in network byte order (aka natural aka bigendian).
 */
struct holy_uboot_image_header {
	holy_uint32_t	ih_magic;	/* Image Header Magic Number	*/
	holy_uint32_t	ih_hcrc;	/* Image Header CRC Checksum	*/
	holy_uint32_t	ih_time;	/* Image Creation Timestamp	*/
	holy_uint32_t	ih_size;	/* Image Data Size		*/
	holy_uint32_t	ih_load;	/* Data	 Load  Address		*/
	holy_uint32_t	ih_ep;		/* Entry Point Address		*/
	holy_uint32_t	ih_dcrc;	/* Image Data CRC Checksum	*/
	holy_uint8_t	ih_os;		/* Operating System		*/
	holy_uint8_t	ih_arch;	/* CPU architecture		*/
	holy_uint8_t	ih_type;	/* Image Type			*/
	holy_uint8_t	ih_comp;	/* Compression Type		*/
	holy_uint8_t	ih_name[holy_UBOOT_IH_NMLEN];	/* Image Name		*/
};

#endif	/* __IMAGE_H__ */
