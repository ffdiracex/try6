/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;

static holy_efi_handle_t image_handle;
static holy_efi_char16_t *cmdline;

static holy_err_t
holy_appleloader_unload (void)
{
  holy_efi_boot_services_t *b;

  b = holy_efi_system_table->boot_services;
  efi_call_1 (b->unload_image, image_handle);

  holy_free (cmdline);
  cmdline = 0;

  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

static holy_err_t
holy_appleloader_boot (void)
{
  holy_efi_boot_services_t *b;

  b = holy_efi_system_table->boot_services;
  efi_call_3 (b->start_image, image_handle, 0, 0);

  holy_appleloader_unload ();

  return holy_errno;
}

struct piwg_full_device_path
{
  struct holy_efi_memory_mapped_device_path comp1;
  struct holy_efi_piwg_device_path comp2;
  struct holy_efi_device_path end;
};

#define MAKE_PIWG_PATH(st, en)						\
  {									\
  .comp1 =								\
    {									\
      .header = {							\
	.type = holy_EFI_HARDWARE_DEVICE_PATH_TYPE,			\
	.subtype = holy_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE,		\
	.length = sizeof (struct holy_efi_memory_mapped_device_path)	\
      },								\
      .memory_type = holy_EFI_MEMORY_MAPPED_IO,				\
      .start_address = st,						\
      .end_address = en							\
    },									\
    .comp2 =								\
      {									\
	.header = {							\
	  .type = holy_EFI_MEDIA_DEVICE_PATH_TYPE,			\
	  .subtype = holy_EFI_PIWG_DEVICE_PATH_SUBTYPE,			\
	  .length = sizeof (struct holy_efi_piwg_device_path)		\
	},								\
	.guid = holy_EFI_VENDOR_APPLE_GUID				\
      },								\
       .end =								\
	  {								\
	    .type = holy_EFI_END_DEVICE_PATH_TYPE,			\
	    .subtype = holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE,		\
	    .length = sizeof (struct holy_efi_device_path)		\
	  }								\
  }

/* early 2006 Core Duo / Core Solo models  */
static struct piwg_full_device_path devpath_1 = MAKE_PIWG_PATH (0xffe00000,
								0xfff9ffff);

/* mid-2006 Mac Pro (and probably other Core 2 models)  */
static struct piwg_full_device_path devpath_2 = MAKE_PIWG_PATH (0xffe00000,
								0xfff7ffff);

/* mid-2007 MBP ("Santa Rosa" based models)  */
static struct piwg_full_device_path devpath_3 = MAKE_PIWG_PATH (0xffe00000,
								0xfff8ffff);

/* early-2008 MBA  */
static struct piwg_full_device_path devpath_4 = MAKE_PIWG_PATH (0xffc00000,
								0xfff8ffff);

/* late-2008 MB/MBP (NVidia chipset)  */
static struct piwg_full_device_path devpath_5 = MAKE_PIWG_PATH (0xffcb4000,
								0xffffbfff);

/* mid-2010 MB/MBP (NVidia chipset)  */
static struct piwg_full_device_path devpath_6 = MAKE_PIWG_PATH (0xffcc4000,
								0xffffbfff);

static struct piwg_full_device_path devpath_7 = MAKE_PIWG_PATH (0xff981000,
								0xffc8ffff);

/* mid-2012 MBP retina (MacBookPro10,1) */ 
static struct piwg_full_device_path devpath_8 = MAKE_PIWG_PATH (0xff990000,
								0xffb2ffff);

struct devdata
{
  const char *model;
  holy_efi_device_path_t *devpath;
};

struct devdata devs[] =
{
  {"Core Duo/Solo", (holy_efi_device_path_t *) &devpath_1},
  {"Mac Pro", (holy_efi_device_path_t *) &devpath_2},
  {"MBP", (holy_efi_device_path_t *) &devpath_3},
  {"MBA", (holy_efi_device_path_t *) &devpath_4},
  {"MB NV", (holy_efi_device_path_t *) &devpath_5},
  {"MB NV2", (holy_efi_device_path_t *) &devpath_6},
  {"MBP2011", (holy_efi_device_path_t *) &devpath_7},
  {"MBP2012", (holy_efi_device_path_t *) &devpath_8},
  {NULL, NULL},
};

static holy_err_t
holy_cmd_appleloader (holy_command_t cmd __attribute__ ((unused)),
                      int argc, char *argv[])
{
  holy_efi_boot_services_t *b;
  holy_efi_loaded_image_t *loaded_image;
  struct devdata *pdev;

  holy_dl_ref (my_mod);

  /* Initialize some global variables.  */
  image_handle = 0;

  b = holy_efi_system_table->boot_services;

  for (pdev = devs ; pdev->devpath ; pdev++)
    if (efi_call_6 (b->load_image, 0, holy_efi_image_handle, pdev->devpath,
                    NULL, 0, &image_handle) == holy_EFI_SUCCESS)
      break;

  if (! pdev->devpath)
    {
      holy_error (holy_ERR_BAD_OS, "can't find model");
      goto fail;
    }

  holy_dprintf ("appleload", "Model: %s\n", pdev->model);

  loaded_image = holy_efi_get_loaded_image (image_handle);
  if (! loaded_image)
    {
      holy_error (holy_ERR_BAD_OS, "no loaded image available");
      goto fail;
    }

  if (argc > 0)
    {
      int i, len;
      holy_efi_char16_t *p16;

      for (i = 0, len = 0; i < argc; i++)
        len += holy_strlen (argv[i]) + 1;

      len *= sizeof (holy_efi_char16_t);
      cmdline = p16 = holy_malloc (len);
      if (! cmdline)
        goto fail;

      for (i = 0; i < argc; i++)
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

  holy_loader_set (holy_appleloader_boot, holy_appleloader_unload, 0);

  return 0;

 fail:

  holy_dl_unref (my_mod);
  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(appleloader)
{
  cmd = holy_register_command ("appleloader", holy_cmd_appleloader,
			       N_("[OPTS]"),
			       /* TRANSLATORS: This command is used on EFI to
				switch to BIOS mode and boot the OS requiring
				BIOS.  */
			       N_("Boot BIOS-based system."));
  my_mod = mod;
}

holy_MOD_FINI(appleloader)
{
  holy_unregister_command (cmd);
}
