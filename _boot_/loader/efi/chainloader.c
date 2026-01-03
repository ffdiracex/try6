/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/charset.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/efi/disk.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/net.h>
#if defined (__i386__) || defined (__x86_64__)
#include <holy/macho.h>
#include <holy/i386/macho.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;

static holy_efi_physical_address_t address;
static holy_efi_uintn_t pages;
static holy_efi_device_path_t *file_path;
static holy_efi_handle_t image_handle;
static holy_efi_char16_t *cmdline;

static holy_err_t
holy_chainloader_unload (void)
{
  holy_efi_boot_services_t *b;

  b = holy_efi_system_table->boot_services;
  efi_call_1 (b->unload_image, image_handle);
  efi_call_2 (b->free_pages, address, pages);

  holy_free (file_path);
  holy_free (cmdline);
  cmdline = 0;
  file_path = 0;

  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

static holy_err_t
holy_chainloader_boot (void)
{
  holy_efi_boot_services_t *b;
  holy_efi_status_t status;
  holy_efi_uintn_t exit_data_size;
  holy_efi_char16_t *exit_data = NULL;

  b = holy_efi_system_table->boot_services;
  status = efi_call_3 (b->start_image, image_handle, &exit_data_size, &exit_data);
  if (status != holy_EFI_SUCCESS)
    {
      if (exit_data)
	{
	  char *buf;

	  buf = holy_malloc (exit_data_size * 4 + 1);
	  if (buf)
	    {
	      *holy_utf16_to_utf8 ((holy_uint8_t *) buf,
				   exit_data, exit_data_size) = 0;

	      holy_error (holy_ERR_BAD_OS, buf);
	      holy_free (buf);
	    }
	}
      else
	holy_error (holy_ERR_BAD_OS, "unknown error");
    }

  if (exit_data)
    efi_call_1 (b->free_pool, exit_data);

  holy_loader_unset ();

  return holy_errno;
}

static void
copy_file_path (holy_efi_file_path_device_path_t *fp,
		const char *str, holy_efi_uint16_t len)
{
  holy_efi_char16_t *p;
  holy_efi_uint16_t size;

  fp->header.type = holy_EFI_MEDIA_DEVICE_PATH_TYPE;
  fp->header.subtype = holy_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE;

  size = holy_utf8_to_utf16 (fp->path_name, len * holy_MAX_UTF16_PER_UTF8,
			     (const holy_uint8_t *) str, len, 0);
  for (p = fp->path_name; p < fp->path_name + size; p++)
    if (*p == '/')
      *p = '\\';

  /* File Path is NULL terminated */
  fp->path_name[size++] = '\0';
  fp->header.length = size * sizeof (holy_efi_char16_t) + sizeof (*fp);
}

static holy_efi_device_path_t *
make_file_path (holy_efi_device_path_t *dp, const char *filename)
{
  char *dir_start;
  char *dir_end;
  holy_size_t size;
  holy_efi_device_path_t *d;

  dir_start = holy_strchr (filename, ')');
  if (! dir_start)
    dir_start = (char *) filename;
  else
    dir_start++;

  dir_end = holy_strrchr (dir_start, '/');
  if (! dir_end)
    {
      holy_error (holy_ERR_BAD_FILENAME, "invalid EFI file path");
      return 0;
    }

  size = 0;
  d = dp;
  while (1)
    {
      size += holy_EFI_DEVICE_PATH_LENGTH (d);
      if ((holy_EFI_END_ENTIRE_DEVICE_PATH (d)))
	break;
      d = holy_EFI_NEXT_DEVICE_PATH (d);
    }

  /* File Path is NULL terminated. Allocate space for 2 extra characters */
  /* FIXME why we split path in two components? */
  file_path = holy_malloc (size
			   + ((holy_strlen (dir_start) + 2)
			      * holy_MAX_UTF16_PER_UTF8
			      * sizeof (holy_efi_char16_t))
			   + sizeof (holy_efi_file_path_device_path_t) * 2);
  if (! file_path)
    return 0;

  holy_memcpy (file_path, dp, size);

  /* Fill the file path for the directory.  */
  d = (holy_efi_device_path_t *) ((char *) file_path
				  + ((char *) d - (char *) dp));
  holy_efi_print_device_path (d);
  copy_file_path ((holy_efi_file_path_device_path_t *) d,
		  dir_start, dir_end - dir_start);

  /* Fill the file path for the file.  */
  d = holy_EFI_NEXT_DEVICE_PATH (d);
  copy_file_path ((holy_efi_file_path_device_path_t *) d,
		  dir_end + 1, holy_strlen (dir_end + 1));

  /* Fill the end of device path nodes.  */
  d = holy_EFI_NEXT_DEVICE_PATH (d);
  d->type = holy_EFI_END_DEVICE_PATH_TYPE;
  d->subtype = holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
  d->length = sizeof (*d);

  return file_path;
}

static holy_err_t
holy_cmd_chainloader (holy_command_t cmd __attribute__ ((unused)),
		      int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_ssize_t size;
  holy_efi_status_t status;
  holy_efi_boot_services_t *b;
  holy_device_t dev = 0;
  holy_efi_device_path_t *dp = 0;
  holy_efi_loaded_image_t *loaded_image;
  char *filename;
  void *boot_image = 0;
  holy_efi_handle_t dev_handle = 0;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
  filename = argv[0];

  holy_dl_ref (my_mod);

  /* Initialize some global variables.  */
  address = 0;
  image_handle = 0;
  file_path = 0;

  b = holy_efi_system_table->boot_services;

  file = holy_file_open (filename);
  if (! file)
    goto fail;

  /* Get the root device's device path.  */
  dev = holy_device_open (0);
  if (! dev)
    goto fail;

  if (dev->disk)
    dev_handle = holy_efidisk_get_device_handle (dev->disk);
  else if (dev->net && dev->net->server)
    {
      holy_net_network_level_address_t addr;
      struct holy_net_network_level_interface *inf;
      holy_net_network_level_address_t gateway;
      holy_err_t err;

      err = holy_net_resolve_address (dev->net->server, &addr);
      if (err)
	goto fail;

      err = holy_net_route_address (addr, &gateway, &inf);
      if (err)
	goto fail;

      dev_handle = holy_efinet_get_device_handle (inf->card);
    }

  if (dev_handle)
    dp = holy_efi_get_device_path (dev_handle);

  if (! dp)
    {
      holy_error (holy_ERR_BAD_DEVICE, "not a valid root device");
      goto fail;
    }

  file_path = make_file_path (dp, filename);
  if (! file_path)
    goto fail;

  holy_printf ("file path: ");
  holy_efi_print_device_path (file_path);

  size = holy_file_size (file);
  if (!size)
    {
      holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		  filename);
      goto fail;
    }
  pages = (((holy_efi_uintn_t) size + ((1 << 12) - 1)) >> 12);

  status = efi_call_4 (b->allocate_pages, holy_EFI_ALLOCATE_ANY_PAGES,
			      holy_EFI_LOADER_CODE,
			      pages, &address);
  if (status != holy_EFI_SUCCESS)
    {
      holy_dprintf ("chain", "Failed to allocate %u pages\n",
		    (unsigned int) pages);
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }

  boot_image = (void *) ((holy_addr_t) address);
  if (holy_file_read (file, boot_image, size) != size)
    {
      if (holy_errno == holy_ERR_NONE)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);

      goto fail;
    }

#if defined (__i386__) || defined (__x86_64__)
  if (size >= (holy_ssize_t) sizeof (struct holy_macho_fat_header))
    {
      struct holy_macho_fat_header *head = boot_image;
      if (head->magic
	  == holy_cpu_to_le32_compile_time (holy_MACHO_FAT_EFI_MAGIC))
	{
	  holy_uint32_t i;
	  struct holy_macho_fat_arch *archs
	    = (struct holy_macho_fat_arch *) (head + 1);
	  for (i = 0; i < holy_cpu_to_le32 (head->nfat_arch); i++)
	    {
	      if (holy_MACHO_CPUTYPE_IS_HOST_CURRENT (archs[i].cputype))
		break;
	    }
	  if (i == holy_cpu_to_le32 (head->nfat_arch))
	    {
	      holy_error (holy_ERR_BAD_OS, "no compatible arch found");
	      goto fail;
	    }
	  if (holy_cpu_to_le32 (archs[i].offset)
	      > ~holy_cpu_to_le32 (archs[i].size)
	      || holy_cpu_to_le32 (archs[i].offset)
	      + holy_cpu_to_le32 (archs[i].size)
	      > (holy_size_t) size)
	    {
	      holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			  filename);
	      goto fail;
	    }
	  boot_image = (char *) boot_image + holy_cpu_to_le32 (archs[i].offset);
	  size = holy_cpu_to_le32 (archs[i].size);
	}
    }
#endif

  status = efi_call_6 (b->load_image, 0, holy_efi_image_handle, file_path,
		       boot_image, size,
		       &image_handle);
  if (status != holy_EFI_SUCCESS)
    {
      if (status == holy_EFI_OUT_OF_RESOURCES)
	holy_error (holy_ERR_OUT_OF_MEMORY, "out of resources");
      else
	holy_error (holy_ERR_BAD_OS, "cannot load image");

      goto fail;
    }

  /* LoadImage does not set a device handler when the image is
     loaded from memory, so it is necessary to set it explicitly here.
     This is a mess.  */
  loaded_image = holy_efi_get_loaded_image (image_handle);
  if (! loaded_image)
    {
      holy_error (holy_ERR_BAD_OS, "no loaded image available");
      goto fail;
    }
  loaded_image->device_handle = dev_handle;

  if (argc > 1)
    {
      int i, len;
      holy_efi_char16_t *p16;

      for (i = 1, len = 0; i < argc; i++)
        len += holy_strlen (argv[i]) + 1;

      len *= sizeof (holy_efi_char16_t);
      cmdline = p16 = holy_malloc (len);
      if (! cmdline)
        goto fail;

      for (i = 1; i < argc; i++)
        {
          char *p8;

          p8 = argv[i];
          while (*p8)
            *(p16++) = *(p8++);

          *(p16++) = ' ';
        }
      *(--p16) = 0;

      loaded_image->load_options = cmdline;
      loaded_image->load_options_size = len;
    }

  holy_file_close (file);
  holy_device_close (dev);

  holy_loader_set (holy_chainloader_boot, holy_chainloader_unload, 0);
  return 0;

 fail:

  if (dev)
    holy_device_close (dev);

  if (file)
    holy_file_close (file);

  holy_free (file_path);

  if (address)
    efi_call_2 (b->free_pages, address, pages);

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(chainloader)
{
  cmd = holy_register_command ("chainloader", holy_cmd_chainloader,
			       0, N_("Load another boot loader."));
  my_mod = mod;
}

holy_MOD_FINI(chainloader)
{
  holy_unregister_command (cmd);
}
