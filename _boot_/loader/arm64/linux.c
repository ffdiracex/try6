/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/charset.h>
#include <holy/command.h>
#include <holy/err.h>
#include <holy/file.h>
#include <holy/fdt.h>
#include <holy/linux.h>
#include <holy/loader.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/cpu/linux.h>
#include <holy/cpu/fdtload.h>
#include <holy/efi/efi.h>
#include <holy/efi/pe32.h>
#include <holy/i18n.h>
#include <holy/lib/cmdline.h>

#include <holy/verity-hash.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;
static int loaded;

static void *kernel_addr;
static holy_uint64_t kernel_size;

static char *linux_args;
static holy_uint32_t cmdline_size;

static holy_addr_t initrd_start;
static holy_addr_t initrd_end;

holy_err_t
holy_arm64_uefi_check_image (struct holy_arm64_linux_kernel_header * lh)
{
  if (lh->magic != holy_ARM64_LINUX_MAGIC)
    return holy_error(holy_ERR_BAD_OS, "invalid magic number");

  if ((lh->code0 & 0xffff) != holy_EFI_PE_MAGIC)
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       N_("plain image kernel not supported - rebuild with CONFIG_(U)EFI_STUB enabled"));

  holy_dprintf ("linux", "UEFI stub kernel:\n");
  holy_dprintf ("linux", "text_offset = 0x%012llx\n",
		(long long unsigned) lh->text_offset);
  holy_dprintf ("linux", "PE/COFF header @ %08x\n", lh->hdr_offset);

  return holy_ERR_NONE;
}

static holy_err_t
finalize_params_linux (void)
{
  int node, retval;

  void *fdt;

  fdt = holy_fdt_load (0x400);

  if (!fdt)
    goto failure;

  node = holy_fdt_find_subnode (fdt, 0, "chosen");
  if (node < 0)
    node = holy_fdt_add_subnode (fdt, 0, "chosen");

  if (node < 1)
    goto failure;

  /* Set initrd info */
  if (initrd_start && initrd_end > initrd_start)
    {
      holy_dprintf ("linux", "Initrd @ 0x%012lx-0x%012lx\n",
		    initrd_start, initrd_end);

      retval = holy_fdt_set_prop64 (fdt, node, "linux,initrd-start",
				    initrd_start);
      if (retval)
	goto failure;
      retval = holy_fdt_set_prop64 (fdt, node, "linux,initrd-end",
				    initrd_end);
      if (retval)
	goto failure;
    }

  if (holy_fdt_install() != holy_ERR_NONE)
    goto failure;

  return holy_ERR_NONE;

failure:
  holy_fdt_unload();
  return holy_error(holy_ERR_BAD_OS, "failed to install/update FDT");
}

holy_err_t
holy_arm64_uefi_boot_image (holy_addr_t addr, holy_size_t size, char *args)
{
  holy_efi_memory_mapped_device_path_t *mempath;
  holy_efi_handle_t image_handle;
  holy_efi_boot_services_t *b;
  holy_efi_status_t status;
  holy_efi_loaded_image_t *loaded_image;
  int len;

  mempath = holy_malloc (2 * sizeof (holy_efi_memory_mapped_device_path_t));
  if (!mempath)
    return holy_errno;

  mempath[0].header.type = holy_EFI_HARDWARE_DEVICE_PATH_TYPE;
  mempath[0].header.subtype = holy_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE;
  mempath[0].header.length = holy_cpu_to_le16_compile_time (sizeof (*mempath));
  mempath[0].memory_type = holy_EFI_LOADER_DATA;
  mempath[0].start_address = addr;
  mempath[0].end_address = addr + size;

  mempath[1].header.type = holy_EFI_END_DEVICE_PATH_TYPE;
  mempath[1].header.subtype = holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
  mempath[1].header.length = sizeof (holy_efi_device_path_t);

  b = holy_efi_system_table->boot_services;
  status = b->load_image (0, holy_efi_image_handle,
			  (holy_efi_device_path_t *) mempath,
			  (void *) addr, size, &image_handle);
  if (status != holy_EFI_SUCCESS)
    return holy_error (holy_ERR_BAD_OS, "cannot load image");

  holy_dprintf ("linux", "linux command line: '%s'\n", args);

  /* Convert command line to UCS-2 */
  loaded_image = holy_efi_get_loaded_image (image_handle);
  loaded_image->load_options_size = len =
    (holy_strlen (args) + 1) * sizeof (holy_efi_char16_t);
  loaded_image->load_options =
    holy_efi_allocate_pages (0,
			     holy_EFI_BYTES_TO_PAGES (loaded_image->load_options_size));
  if (!loaded_image->load_options)
    return holy_errno;

  loaded_image->load_options_size =
    2 * holy_utf8_to_utf16 (loaded_image->load_options, len,
			    (holy_uint8_t *) args, len, NULL);

  holy_dprintf ("linux", "starting image %p\n", image_handle);
  status = b->start_image (image_handle, 0, NULL);

  /* When successful, not reached */
  b->unload_image (image_handle);
  holy_efi_free_pages ((holy_efi_physical_address_t) loaded_image->load_options,
		       holy_EFI_BYTES_TO_PAGES (loaded_image->load_options_size));

  return holy_errno;
}

static holy_err_t
holy_linux_boot (void)
{
  if (finalize_params_linux () != holy_ERR_NONE)
    return holy_errno;

  return (holy_arm64_uefi_boot_image((holy_addr_t)kernel_addr,
                                     kernel_size, linux_args));
}

static holy_err_t
holy_linux_unload (void)
{
  holy_dl_unref (my_mod);
  loaded = 0;
  if (initrd_start)
    holy_efi_free_pages ((holy_efi_physical_address_t) initrd_start,
			 holy_EFI_BYTES_TO_PAGES (initrd_end - initrd_start));
  initrd_start = initrd_end = 0;
  holy_free (linux_args);
  if (kernel_addr)
    holy_efi_free_pages ((holy_efi_physical_address_t) kernel_addr,
			 holy_EFI_BYTES_TO_PAGES (kernel_size));
  holy_fdt_unload ();
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  struct holy_linux_initrd_context initrd_ctx = { 0, 0, 0 };
  int initrd_size, initrd_pages;
  void *initrd_mem = NULL;

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (!loaded)
    {
      holy_error (holy_ERR_BAD_ARGUMENT,
		  N_("you need to load the kernel first"));
      goto fail;
    }

  if (holy_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  initrd_size = holy_get_initrd_size (&initrd_ctx);
  holy_dprintf ("linux", "Loading initrd\n");

  initrd_pages = (holy_EFI_BYTES_TO_PAGES (initrd_size));
  initrd_mem = holy_efi_allocate_pages (0, initrd_pages);
  if (!initrd_mem)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }

  if (holy_initrd_load (&initrd_ctx, argv, initrd_mem))
    goto fail;

  initrd_start = (holy_addr_t) initrd_mem;
  initrd_end = initrd_start + initrd_size;
  holy_dprintf ("linux", "[addr=%p, size=0x%x]\n",
		(void *) initrd_start, initrd_size);

 fail:
  holy_initrd_close (&initrd_ctx);
  if (initrd_mem && !initrd_start)
    holy_efi_free_pages ((holy_efi_physical_address_t) initrd_mem,
			 initrd_pages);

  return holy_errno;
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  struct holy_arm64_linux_kernel_header lh;

  holy_dl_ref (my_mod);

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  file = holy_file_open (argv[0]);
  if (!file)
    goto fail;

  kernel_size = holy_file_size (file);

  if (holy_file_read (file, &lh, sizeof (lh)) < (long) sizeof (lh))
    return holy_errno;

  if (holy_arm64_uefi_check_image (&lh) != holy_ERR_NONE)
    goto fail;

  holy_loader_unset();

  holy_dprintf ("linux", "kernel file size: %lld\n", (long long) kernel_size);
  kernel_addr = holy_efi_allocate_pages (0, holy_EFI_BYTES_TO_PAGES (kernel_size));
  holy_dprintf ("linux", "kernel numpages: %lld\n",
		(long long) holy_EFI_BYTES_TO_PAGES (kernel_size));
  if (!kernel_addr)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }

  holy_file_seek (file, 0);
  if (holy_file_read (file, kernel_addr, kernel_size)
      < (holy_int64_t) kernel_size)
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), argv[0]);
      goto fail;
    }

  holy_dprintf ("linux", "kernel @ %p\n", kernel_addr);

  cmdline_size = holy_loader_cmdline_size (argc, argv) + sizeof (LINUX_IMAGE)
	+ VERITY_CMDLINE_LENGTH;
  linux_args = holy_malloc (cmdline_size);
  if (!linux_args)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }
  holy_memcpy (linux_args, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  holy_create_loader_cmdline (argc, argv,
			      linux_args + sizeof (LINUX_IMAGE) - 1,
			      cmdline_size);

  if (holy_errno == holy_ERR_NONE)
    {
      holy_pass_verity_hash (kernel_addr, linux_args, cmdline_size);
      holy_loader_set (holy_linux_boot, holy_linux_unload, 0);
      loaded = 1;
    }

fail:
  if (file)
    holy_file_close (file);

  if (holy_errno != holy_ERR_NONE)
    {
      holy_dl_unref (my_mod);
      loaded = 0;
    }

  if (linux_args && !loaded)
    holy_free (linux_args);

  if (kernel_addr && !loaded)
    holy_efi_free_pages ((holy_efi_physical_address_t) kernel_addr,
			 holy_EFI_BYTES_TO_PAGES (kernel_size));

  return holy_errno;
}


static holy_command_t cmd_linux, cmd_initrd;

holy_MOD_INIT (linux)
{
  cmd_linux = holy_register_command ("linux", holy_cmd_linux, 0,
				     N_("Load Linux."));
  cmd_initrd = holy_register_command ("initrd", holy_cmd_initrd, 0,
				      N_("Load initrd."));
  my_mod = mod;
}

holy_MOD_FINI (linux)
{
  holy_unregister_command (cmd_linux);
  holy_unregister_command (cmd_initrd);
}
