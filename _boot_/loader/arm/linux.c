/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/fdt.h>
#include <holy/file.h>
#include <holy/loader.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/cache.h>
#include <holy/cpu/linux.h>
#include <holy/lib/cmdline.h>
#include <holy/linux.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;

static holy_addr_t initrd_start;
static holy_addr_t initrd_end;

static holy_addr_t linux_addr;
static holy_size_t linux_size;

static char *linux_args;

static holy_uint32_t machine_type;
static void *fdt_addr;

typedef void (*kernel_entry_t) (int, unsigned long, void *);

#define LINUX_ZIMAGE_OFFSET	0x24
#define LINUX_ZIMAGE_MAGIC	0x016f2818

#define LINUX_PHYS_OFFSET        (0x00008000)
#define LINUX_INITRD_PHYS_OFFSET (LINUX_PHYS_OFFSET + 0x02000000)
#define LINUX_FDT_PHYS_OFFSET    (LINUX_INITRD_PHYS_OFFSET - 0x10000)

static holy_size_t
get_atag_size (holy_uint32_t *atag)
{
  holy_uint32_t *atag0 = atag;
  while (atag[0] && atag[1])
    atag += atag[0];
  return atag - atag0;
}

/*
 * linux_prepare_fdt():
 *   Prepares a loaded FDT for being passed to Linux.
 *   Merges in command line parameters and sets up initrd addresses.
 */
static holy_err_t
linux_prepare_atag (void)
{
  holy_uint32_t *atag_orig = (holy_uint32_t *) fdt_addr;
  holy_uint32_t *tmp_atag, *from, *to;
  holy_size_t tmp_size;
  holy_size_t arg_size = holy_strlen (linux_args);
  char *cmdline_orig = NULL;
  holy_size_t cmdline_orig_len = 0;

  /* some place for cmdline, initrd and terminator.  */
  tmp_size = get_atag_size (atag_orig) + 20 + (arg_size) / 4;
  tmp_atag = holy_malloc (tmp_size * sizeof (holy_uint32_t));
  if (!tmp_atag)
    return holy_errno;

  for (from = atag_orig, to = tmp_atag; from[0] && from[1];
       from += from[0])
    switch (from[1])
      {
      case 0x54410004:
      case 0x54410005:
      case 0x54420005:
	break;
      case 0x54410009:
	if (*(char *) (from + 2))
	  {
	    cmdline_orig = (char *) (from + 2);
	    cmdline_orig_len = holy_strlen (cmdline_orig) + 1;
	  }
	break;
      default:
	holy_memcpy (to, from, sizeof (holy_uint32_t) * from[0]);
	to += from[0];
	break;
      }

  holy_dprintf ("linux", "linux inherited args: '%s'\n",
		cmdline_orig ? : "");
  holy_dprintf ("linux", "linux_args: '%s'\n", linux_args);

  /* Generate and set command line */
  to[0] = 3 + (arg_size + cmdline_orig_len) / 4;
  to[1] = 0x54410009;
  if (cmdline_orig)
    {
      holy_memcpy ((char *) to + 8, cmdline_orig, cmdline_orig_len - 1);
      *((char *) to + 8 + cmdline_orig_len - 1) = ' ';
    }
  holy_memcpy ((char *) to + 8 + cmdline_orig_len, linux_args, arg_size);
  holy_memset ((char *) to + 8 + cmdline_orig_len + arg_size, 0,
	       4 - ((arg_size + cmdline_orig_len) & 3));
  to += to[0];

  if (initrd_start && initrd_end)
    {
      /*
       * We're using physical addresses, so even if we have LPAE, we're
       * restricted to a 32-bit address space.
       */
      holy_dprintf ("loader", "Initrd @ 0x%08x-0x%08x\n",
		    initrd_start, initrd_end);

      to[0] = 4;
      to[1] = 0x54420005;
      to[2] = initrd_start;
      to[3] = initrd_end - initrd_start;
      to += 4;
    }

  to[0] = 0;
  to[1] = 0;
  to += 2;

  /* Copy updated FDT to its launch location */
  holy_memcpy (atag_orig, tmp_atag, sizeof (holy_uint32_t) * (to - tmp_atag));
  holy_free (tmp_atag);

  holy_dprintf ("loader", "ATAG updated for Linux boot\n");

  return holy_ERR_NONE;
}

/*
 * linux_prepare_fdt():
 *   Prepares a loaded FDT for being passed to Linux.
 *   Merges in command line parameters and sets up initrd addresses.
 */
static holy_err_t
linux_prepare_fdt (void)
{
  int node;
  int retval;
  int tmp_size;
  void *tmp_fdt;

  tmp_size = holy_fdt_get_totalsize (fdt_addr) + 0x100 + holy_strlen (linux_args);
  tmp_fdt = holy_malloc (tmp_size);
  if (!tmp_fdt)
    return holy_errno;

  holy_memcpy (tmp_fdt, fdt_addr, holy_fdt_get_totalsize (fdt_addr));
  holy_fdt_set_totalsize (tmp_fdt, tmp_size);

  /* Find or create '/chosen' node */
  node = holy_fdt_find_subnode (tmp_fdt, 0, "chosen");
  if (node < 0)
    {
      holy_dprintf ("linux", "No 'chosen' node in FDT - creating.\n");
      node = holy_fdt_add_subnode (tmp_fdt, 0, "chosen");
      if (node < 0)
	goto failure;
    }

  holy_dprintf ("linux", "linux_args: '%s'\n", linux_args);

  /* Generate and set command line */
  retval = holy_fdt_set_prop (tmp_fdt, node, "bootargs", linux_args,
			      holy_strlen (linux_args) + 1);
  if (retval)
    goto failure;

  if (initrd_start && initrd_end)
    {
      /*
       * We're using physical addresses, so even if we have LPAE, we're
       * restricted to a 32-bit address space.
       */
      holy_dprintf ("loader", "Initrd @ 0x%08x-0x%08x\n",
		    initrd_start, initrd_end);

      retval = holy_fdt_set_prop32 (tmp_fdt, node, "linux,initrd-start",
				    initrd_start);
      if (retval)
	goto failure;
      retval = holy_fdt_set_prop32 (tmp_fdt, node, "linux,initrd-end",
				    initrd_end);
      if (retval)
	goto failure;
    }

  /* Copy updated FDT to its launch location */
  holy_memcpy (fdt_addr, tmp_fdt, tmp_size);
  holy_free (tmp_fdt);

  holy_dprintf ("loader", "FDT updated for Linux boot\n");

  return holy_ERR_NONE;

failure:
  holy_free (tmp_fdt);
  return holy_error (holy_ERR_BAD_ARGUMENT, "unable to prepare FDT");
}

static holy_err_t
linux_boot (void)
{
  kernel_entry_t linuxmain;
  int fdt_valid, atag_valid;

  fdt_valid = (fdt_addr && holy_fdt_check_header_nosize (fdt_addr) == 0);
  atag_valid = ((((holy_uint16_t *) fdt_addr)[3] & ~3) == 0x5440
		&& *((holy_uint32_t *) fdt_addr));
  holy_dprintf ("loader", "atag: %p, %x, %x, %s, %s\n",
		fdt_addr,
		((holy_uint16_t *) fdt_addr)[3],
		*((holy_uint32_t *) fdt_addr),
		(char *) fdt_addr,
		(char *) fdt_addr + 1);

  if (!fdt_valid && machine_type == holy_ARM_MACHINE_TYPE_FDT)
    return holy_error (holy_ERR_FILE_NOT_FOUND,
		       N_("device tree must be supplied (see `devicetree' command)"));

  holy_arch_sync_caches ((void *) linux_addr, linux_size);

  holy_dprintf ("loader", "Kernel at: 0x%x\n", linux_addr);

  if (fdt_valid)
    {
      holy_err_t err;

      err = linux_prepare_fdt ();
      if (err)
	return err;
      holy_dprintf ("loader", "FDT @ 0x%p\n", fdt_addr);
    }
  else if (atag_valid)
    {
      holy_err_t err;

      err = linux_prepare_atag ();
      if (err)
	return err;
      holy_dprintf ("loader", "ATAG @ 0x%p\n", fdt_addr);
    }

  holy_dprintf ("loader", "Jumping to Linux...\n");

  /* Boot the kernel.
   *   Arguments to kernel:
   *     r0 - 0
   *     r1 - machine type
   *     r2 - address of DTB
   */
  linuxmain = (kernel_entry_t) linux_addr;

#ifdef holy_MACHINE_EFI
  {
    holy_err_t err;
    err = holy_efi_prepare_platform();
    if (err != holy_ERR_NONE)
      return err;
  }
#endif

  holy_arm_disable_caches_mmu ();

  linuxmain (0, machine_type, fdt_addr);

  return holy_error (holy_ERR_BAD_OS, "Linux call returned");
}

/*
 * Only support zImage, so no relocations necessary
 */
static holy_err_t
linux_load (const char *filename, holy_file_t file)
{
  int size;

  size = holy_file_size (file);

#ifdef holy_MACHINE_EFI
  linux_addr = (holy_addr_t) holy_efi_allocate_loader_memory (LINUX_PHYS_OFFSET, size);
  if (!linux_addr)
    return holy_errno;
#else
  linux_addr = LINUX_ADDRESS;
#endif
  holy_dprintf ("loader", "Loading Linux to 0x%08x\n",
		(holy_addr_t) linux_addr);

  if (holy_file_read (file, (void *) linux_addr, size) != size)
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);
      return holy_errno;
    }

  if (size > LINUX_ZIMAGE_OFFSET + 4
      && *(holy_uint32_t *) (linux_addr + LINUX_ZIMAGE_OFFSET)
      == LINUX_ZIMAGE_MAGIC)
    ;
  else if (size > 0x8000 && *(holy_uint32_t *) (linux_addr) == 0xea000006
	   && machine_type == holy_ARM_MACHINE_TYPE_RASPBERRY_PI)
    holy_memmove ((void *) linux_addr, (void *) (linux_addr + 0x8000),
		  size - 0x8000);
  else
    return holy_error (holy_ERR_BAD_FILE_TYPE, N_("invalid zImage"));

  linux_size = size;

  return holy_ERR_NONE;
}

static holy_err_t
linux_unload (void)
{
  holy_dl_unref (my_mod);

  holy_free (linux_args);
  linux_args = NULL;

  initrd_start = initrd_end = 0;

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  int size;
  holy_err_t err;
  holy_file_t file;
  holy_dl_ref (my_mod);

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = holy_file_open (argv[0]);
  if (!file)
    goto fail;

  err = linux_load (argv[0], file);
  holy_file_close (file);
  if (err)
    goto fail;

  holy_loader_set (linux_boot, linux_unload, 0);

  size = holy_loader_cmdline_size (argc, argv);
  linux_args = holy_malloc (size + sizeof (LINUX_IMAGE));
  if (!linux_args)
    {
      holy_loader_unset();
      goto fail;
    }

  /* Create kernel command line.  */
  holy_memcpy (linux_args, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  holy_create_loader_cmdline (argc, argv,
			      linux_args + sizeof (LINUX_IMAGE) - 1, size);

  return holy_ERR_NONE;

fail:
  holy_dl_unref (my_mod);
  return holy_errno;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  holy_file_t file;
  holy_size_t size = 0;
  struct holy_linux_initrd_context initrd_ctx = { 0, 0, 0 };

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = holy_file_open (argv[0]);
  if (!file)
    return holy_errno;

  if (holy_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  size = holy_get_initrd_size (&initrd_ctx);

#ifdef holy_MACHINE_EFI
  if (initrd_start)
    holy_efi_free_pages (initrd_start,
			 (initrd_end - initrd_start + 0xfff) >> 12);
  initrd_start = (holy_addr_t) holy_efi_allocate_loader_memory (LINUX_INITRD_PHYS_OFFSET, size);

  if (!initrd_start)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }
#else
  initrd_start = LINUX_INITRD_ADDRESS;
#endif

  holy_dprintf ("loader", "Loading initrd to 0x%08x\n",
		(holy_addr_t) initrd_start);

  if (holy_initrd_load (&initrd_ctx, argv, (void *) initrd_start))
    goto fail;

  initrd_end = initrd_start + size;

  return holy_ERR_NONE;

fail:
  holy_file_close (file);

  return holy_errno;
}

static holy_err_t
load_dtb (holy_file_t dtb, int size)
{
  if ((holy_file_read (dtb, fdt_addr, size) != size)
      || (holy_fdt_check_header (fdt_addr, size) != 0))
    return holy_error (holy_ERR_BAD_OS, N_("invalid device tree"));

  holy_fdt_set_totalsize (fdt_addr, size);
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_devicetree (holy_command_t cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  holy_file_t dtb;
  int size;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  dtb = holy_file_open (argv[0]);
  if (!dtb)
    goto out;

  size = holy_file_size (dtb);
  if (size == 0)
    {
      holy_error (holy_ERR_BAD_OS, "empty file");
      goto out;
    }

#ifdef holy_MACHINE_EFI
  fdt_addr = holy_efi_allocate_loader_memory (LINUX_FDT_PHYS_OFFSET, size);
  if (!fdt_addr)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto out;
    }
#else
  fdt_addr = (void *) LINUX_FDT_ADDRESS;
#endif

  holy_dprintf ("loader", "Loading device tree to 0x%08x\n",
		(holy_addr_t) fdt_addr);
  load_dtb (dtb, size);
  if (holy_errno != holy_ERR_NONE)
    {
      fdt_addr = NULL;
      goto out;
    }

  /* 
   * We've successfully loaded an FDT, so any machine type passed
   * from firmware is now obsolete.
   */
  machine_type = holy_ARM_MACHINE_TYPE_FDT;

 out:
  holy_file_close (dtb);

  return holy_errno;
}

static holy_command_t cmd_linux, cmd_initrd, cmd_devicetree;

holy_MOD_INIT (linux)
{
  cmd_linux = holy_register_command ("linux", holy_cmd_linux,
				     0, N_("Load Linux."));
  cmd_initrd = holy_register_command ("initrd", holy_cmd_initrd,
				      0, N_("Load initrd."));
  cmd_devicetree = holy_register_command ("devicetree", holy_cmd_devicetree,
					  /* TRANSLATORS: DTB stands for device tree blob.  */
					  0, N_("Load DTB file."));
  my_mod = mod;
  fdt_addr = (void *) holy_arm_firmware_get_boot_data ();
  machine_type = holy_arm_firmware_get_machine_type ();
}

holy_MOD_FINI (linux)
{
  holy_unregister_command (cmd_linux);
  holy_unregister_command (cmd_initrd);
  holy_unregister_command (cmd_devicetree);
}
