/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CPU_XNU_H
#define holy_CPU_XNU_H 1

#include <holy/err.h>
#include <holy/efi/api.h>
#include <holy/cpu/relocator.h>

#define XNU_RELOCATOR(x) (holy_relocator32_ ## x)

#define holy_XNU_PAGESIZE 4096
typedef holy_uint32_t holy_xnu_ptr_t;

struct holy_xnu_boot_params_common
{
  /* Command line passed to xnu. */
  holy_uint8_t cmdline[1024];

  /* Later are the same as EFI's get_memory_map (). */
  holy_xnu_ptr_t efi_mmap;
  holy_uint32_t efi_mmap_size;
  holy_uint32_t efi_mem_desc_size;
  holy_uint32_t efi_mem_desc_version;

  /* Later are video parameters. */
  holy_xnu_ptr_t lfb_base;
#define holy_XNU_VIDEO_SPLASH 1
#define holy_XNU_VIDEO_TEXT_IN_VIDEO 2
  holy_uint32_t lfb_mode;
  holy_uint32_t lfb_line_len;
  holy_uint32_t lfb_width;
  holy_uint32_t lfb_height;
  holy_uint32_t lfb_depth;

  /* Pointer to device tree and its len. */
  holy_xnu_ptr_t devtree;
  holy_uint32_t devtreelen;

  /* First used address by kernel or boot structures. */
  holy_xnu_ptr_t heap_start;
  /* Last used address by kernel or boot structures minus previous value. */
  holy_uint32_t heap_size;
  /* First memory page containing runtime code or data. */
  holy_uint32_t efi_runtime_first_page;
  /* First memory page containing runtime code or data minus previous value. */
  holy_uint32_t efi_runtime_npages;
} holy_PACKED;

struct holy_xnu_boot_params_v1
{
  holy_uint16_t verminor;
  holy_uint16_t vermajor;
  struct holy_xnu_boot_params_common common;

  holy_uint32_t efi_system_table;
  /* Size of holy_efi_uintn_t in bits. */
  holy_uint8_t efi_uintnbits;
} holy_PACKED;
#define holy_XNU_BOOTARGSV1_VERMINOR 5
#define holy_XNU_BOOTARGSV1_VERMAJOR 1

struct holy_xnu_boot_params_v2
{
  holy_uint16_t verminor;
  holy_uint16_t vermajor;

  /* Size of holy_efi_uintn_t in bits. */
  holy_uint8_t efi_uintnbits;
  holy_uint8_t unused[3];

  struct holy_xnu_boot_params_common common;

  holy_uint64_t efi_runtime_first_page_virtual;
  holy_uint32_t efi_system_table;
  holy_uint32_t unused2[9];
  holy_uint64_t ram_size;
  holy_uint64_t fsbfreq;
  holy_uint32_t unused3[734];
} holy_PACKED;
#define holy_XNU_BOOTARGSV2_VERMINOR 0
#define holy_XNU_BOOTARGSV2_VERMAJOR 2

union holy_xnu_boot_params_any
{
  struct holy_xnu_boot_params_v1 v1;
  struct holy_xnu_boot_params_v2 v2;
};

struct holy_xnu_devprop_header
{
  holy_uint32_t length;
  /* Always set to 1. Version?  */
  holy_uint32_t alwaysone;
  holy_uint32_t num_devices;
};

struct holy_xnu_devprop_device_header
{
  holy_uint32_t length;
  holy_uint32_t num_values;
};

void holy_cpu_xnu_unload (void);

struct holy_xnu_devprop_device_descriptor;

struct holy_xnu_devprop_device_descriptor *
holy_xnu_devprop_add_device (struct holy_efi_device_path *path, int length);
holy_err_t
holy_xnu_devprop_remove_device (struct holy_xnu_devprop_device_descriptor *dev);
holy_err_t
holy_xnu_devprop_remove_property (struct holy_xnu_devprop_device_descriptor *dev,
				  char *name);
holy_err_t
holy_xnu_devprop_add_property_utf8 (struct holy_xnu_devprop_device_descriptor *dev,
				    char *name, void *data, int datalen);
holy_err_t
holy_xnu_devprop_add_property_utf16 (struct holy_xnu_devprop_device_descriptor *dev,
				     holy_uint16_t *name, int namelen,
				     void *data, int datalen);
holy_err_t
holy_xnu_devprop_remove_property_utf8 (struct holy_xnu_devprop_device_descriptor *dev,
				       char *name);
void holy_cpu_xnu_init (void);
void holy_cpu_xnu_fini (void);

extern holy_uint32_t holy_xnu_entry_point;
extern holy_uint32_t holy_xnu_stack;
extern holy_uint32_t holy_xnu_arg1;
extern char holy_xnu_cmdline[1024];
holy_err_t holy_xnu_boot (void);
#endif
