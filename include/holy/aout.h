/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_AOUT_HEADER
#define holy_AOUT_HEADER 1

#include <holy/types.h>

struct holy_aout32_header
{
  holy_uint32_t a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
  holy_uint32_t a_text;		/* text segment size */
  holy_uint32_t a_data;		/* initialized data size */
  holy_uint32_t a_bss;		/* uninitialized data size */
  holy_uint32_t a_syms;		/* symbol table size */
  holy_uint32_t a_entry;	/* entry point */
  holy_uint32_t a_trsize;	/* text relocation size */
  holy_uint32_t a_drsize;	/* data relocation size */
};

struct holy_aout64_header
{
  holy_uint32_t a_midmag;	/* htonl(flags<<26 | mid<<16 | magic) */
  holy_uint64_t a_text;		/* text segment size */
  holy_uint64_t a_data;		/* initialized data size */
  holy_uint64_t a_bss;		/* uninitialized data size */
  holy_uint64_t a_syms;		/* symbol table size */
  holy_uint64_t a_entry;	/* entry point */
  holy_uint64_t a_trsize;	/* text relocation size */
  holy_uint64_t a_drsize;	/* data relocation size */
};

union holy_aout_header
{
  struct holy_aout32_header aout32;
  struct holy_aout64_header aout64;
};

#define AOUT_TYPE_NONE		0
#define AOUT_TYPE_AOUT32	1
#define AOUT_TYPE_AOUT64	6

#define	AOUT32_OMAGIC		0x107	/* 0407 old impure format */
#define	AOUT32_NMAGIC		0x108	/* 0410 read-only text */
#define	AOUT32_ZMAGIC		0x10b	/* 0413 demand load format */
#define AOUT32_QMAGIC		0xcc	/* 0314 "compact" demand load format */

#define AOUT64_OMAGIC		0x1001
#define AOUT64_ZMAGIC		0x1002
#define AOUT64_NMAGIC		0x1003

#define	AOUT_MID_ZERO		0	/* unknown - implementation dependent */
#define	AOUT_MID_SUN010		1	/* sun 68010/68020 binary */
#define	AOUT_MID_SUN020		2	/* sun 68020-only binary */
#define AOUT_MID_I386		134	/* i386 BSD binary */
#define AOUT_MID_SPARC		138	/* sparc */
#define	AOUT_MID_HP200		200	/* hp200 (68010) BSD binary */
#define	AOUT_MID_SUN            0x103
#define	AOUT_MID_HP300		300	/* hp300 (68020+68881) BSD binary */
#define	AOUT_MID_HPUX		0x20C	/* hp200/300 HP-UX binary */
#define	AOUT_MID_HPUX800	0x20B	/* hp800 HP-UX binary */

#define AOUT_FLAG_PIC		0x10	/* contains position independent code */
#define AOUT_FLAG_DYNAMIC	0x20	/* contains run-time link-edit info */
#define AOUT_FLAG_DPMASK	0x30	/* mask for the above */

#define AOUT_GETMAGIC(header) ((header).a_midmag & 0xffff)
#define AOUT_GETMID(header) ((header).a_midmag >> 16) & 0x03ff)
#define AOUT_GETFLAG(header) ((header).a_midmag >> 26) & 0x3f)

#ifndef holy_UTIL

int EXPORT_FUNC(holy_aout_get_type) (union holy_aout_header *header);

holy_err_t EXPORT_FUNC(holy_aout_load) (holy_file_t file, int offset,
                                        void *load_addr, int load_size,
                                        holy_size_t bss_size);

#endif

#endif /* ! holy_AOUT_HEADER */
