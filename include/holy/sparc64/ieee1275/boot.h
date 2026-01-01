/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BOOT_MACHINE_HEADER
#define holy_BOOT_MACHINE_HEADER	1

#define CIF_REG				%l0
#define CHOSEN_NODE_REG			%l4
#define STDOUT_NODE_REG			%l5
#define BOOTDEV_REG			%l6
#define PIC_REG				%l7

#define	SCRATCH_PAD_BOOT		0x5000
#define	SCRATCH_PAD_DISKBOOT		0x4000
#define	SCRATCH_PAD_BOOT_SIZE		0x110

#define GET_ABS(symbol, reg)	\
	add	PIC_REG, (symbol - pic_base), reg
#define LDUW_ABS(symbol, offset, reg)	\
	lduw	[PIC_REG + (symbol - pic_base) + (offset)], reg
#define LDX_ABS(symbol, offset, reg)	\
	ldx	[PIC_REG + (symbol - pic_base) + (offset)], reg

#define holy_BOOT_AOUT_HEADER_SIZE	32

#define holy_BOOT_MACHINE_SIGNATURE	0xbb44aa55

#define holy_BOOT_MACHINE_BOOT_DEVPATH	0x08

#define holy_BOOT_MACHINE_BOOT_DEVPATH_END 0x80

#define holy_BOOT_MACHINE_KERNEL_BYTE 0x80

#define holy_BOOT_MACHINE_CODE_END \
	(0x1fc - holy_BOOT_AOUT_HEADER_SIZE)

#define holy_BOOT_MACHINE_KERNEL_ADDR 0x4200

#ifndef ASM_FILE
/* This is the blocklist used in the diskboot image.  */
struct holy_boot_blocklist
{
  holy_uint64_t start;
  holy_uint32_t len;
} holy_PACKED;
#endif

#endif /* ! BOOT_MACHINE_HEADER */
