/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LINUX_CPU_HEADER
#define holy_LINUX_CPU_HEADER 1

#include <holy/efi/efi.h>

#define holy_ARM64_LINUX_MAGIC 0x644d5241 /* 'ARM\x64' */

#define holy_EFI_PE_MAGIC	0x5A4D

/* From linux/Documentation/arm64/booting.txt */
struct holy_arm64_linux_kernel_header
{
  holy_uint32_t code0;		/* Executable code */
  holy_uint32_t code1;		/* Executable code */
  holy_uint64_t text_offset;    /* Image load offset */
  holy_uint64_t res0;		/* reserved */
  holy_uint64_t res1;		/* reserved */
  holy_uint64_t res2;		/* reserved */
  holy_uint64_t res3;		/* reserved */
  holy_uint64_t res4;		/* reserved */
  holy_uint32_t magic;		/* Magic number, little endian, "ARM\x64" */
  holy_uint32_t hdr_offset;	/* Offset of PE/COFF header */
};

holy_err_t holy_arm64_uefi_check_image (struct holy_arm64_linux_kernel_header
                                        *lh);
holy_err_t holy_arm64_uefi_boot_image (holy_addr_t addr, holy_size_t size,
                                       char *args);

#endif /* ! holy_LINUX_CPU_HEADER */
