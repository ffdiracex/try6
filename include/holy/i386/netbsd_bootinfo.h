/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NETBSD_BOOTINFO_CPU_HEADER
#define holy_NETBSD_BOOTINFO_CPU_HEADER	1

#include <holy/types.h>

#define NETBSD_BTINFO_BOOTPATH		0
#define NETBSD_BTINFO_ROOTDEVICE	1
#define NETBSD_BTINFO_BOOTDISK		3
#define NETBSD_BTINFO_CONSOLE		6
#define NETBSD_BTINFO_SYMTAB		8
#define NETBSD_BTINFO_MEMMAP		9
#define NETBSD_BTINFO_BOOTWEDGE		10
#define NETBSD_BTINFO_MODULES		11
#define NETBSD_BTINFO_FRAMEBUF		12
#define NETBSD_BTINFO_USERCONFCOMMANDS  13
#define NETBSD_BTINFO_EFI	        14

struct holy_netbsd_bootinfo
{
  holy_uint32_t bi_count;
  holy_uint32_t bi_data[0];
};

struct holy_netbsd_btinfo_common
{
  holy_uint32_t len;
  holy_uint32_t type;
};

#define holy_NETBSD_MAX_BOOTPATH_LEN 80

struct holy_netbsd_btinfo_bootdisk
{
  holy_uint32_t labelsector;  /* label valid if != 0xffffffff */
  struct
    {
      holy_uint16_t type, checksum;
      char packname[16];
    } label;
  holy_uint32_t biosdev;
  holy_uint32_t partition;
};

struct holy_netbsd_btinfo_bootwedge {
  holy_uint32_t biosdev;
  holy_disk_addr_t startblk;
  holy_uint64_t nblks;
  holy_disk_addr_t matchblk;
  holy_uint64_t matchnblks;
  holy_uint8_t matchhash[16];  /* MD5 hash */
} holy_PACKED;

struct holy_netbsd_btinfo_symtab
{
  holy_uint32_t nsyms;
  holy_uint32_t ssyms;
  holy_uint32_t esyms;
};


struct holy_netbsd_btinfo_serial
{
  char devname[16];
  holy_uint32_t addr;
  holy_uint32_t speed;
};

struct holy_netbsd_btinfo_modules
{
  holy_uint32_t num;
  holy_uint32_t last_addr;
  struct holy_netbsd_btinfo_module
  {
    char name[80];
#define holy_NETBSD_MODULE_RAW 0
#define holy_NETBSD_MODULE_ELF 1
    holy_uint32_t type;
    holy_uint32_t size;
    holy_uint32_t addr;
  } mods[0];
};

struct holy_netbsd_btinfo_framebuf
{
  holy_uint64_t fbaddr;
  holy_uint32_t flags;
  holy_uint32_t width;
  holy_uint32_t height;
  holy_uint16_t pitch;
  holy_uint8_t bpp;

  holy_uint8_t red_mask_size;
  holy_uint8_t green_mask_size;
  holy_uint8_t blue_mask_size;

  holy_uint8_t red_field_pos;
  holy_uint8_t green_field_pos;
  holy_uint8_t blue_field_pos;

  holy_uint8_t reserved[16];
};

#define holy_NETBSD_MAX_ROOTDEVICE_LEN 16

struct holy_netbsd_btinfo_efi
{
  void *pa_systbl;  /* Physical address of the EFI System Table */
};

#endif
