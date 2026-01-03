/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/types.h>
#include <holy/mm.h>
#include <holy/cpu/linux.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/lib/cmdline.h>
#include <holy/efi/efi.h>
#include <holy/tpm.h>

#include <holy/verity-hash.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;
static int loaded;
static void *kernel_mem;
static holy_uint64_t kernel_size;
static holy_uint8_t *initrd_mem;
static holy_uint32_t handover_offset;
struct linux_kernel_params *params;
static char *linux_cmdline;

#define BYTES_TO_PAGES(bytes)   (((bytes) + 0xfff) >> 12)

#define SHIM_LOCK_GUID \
  { 0x605dab50, 0xe046, 0x4300, {0xab, 0xb6, 0x3d, 0xd8, 0x10, 0xdd, 0x8b, 0x23} }

struct holy_efi_shim_lock
{
  holy_efi_status_t (*verify) (void *buffer, holy_uint32_t size);
};
typedef struct holy_efi_shim_lock holy_efi_shim_lock_t;

static holy_efi_boolean_t
holy_linuxefi_secure_validate (void *data, holy_uint32_t size)
{
  holy_efi_guid_t guid = SHIM_LOCK_GUID;
  holy_efi_shim_lock_t *shim_lock;

  shim_lock = holy_efi_locate_protocol(&guid, NULL);

  if (!shim_lock) {
    if (holy_efi_secure_boot())
      return 0;
    else
      return 1;
  }

  if (shim_lock->verify(data, size) == holy_EFI_SUCCESS)
    return 1;

  return 0;
}

typedef void(*handover_func)(void *, holy_efi_system_table_t *, struct linux_kernel_params *);

static holy_err_t
holy_linuxefi_boot (void)
{
  handover_func hf;
  int offset = 0;

#ifdef __x86_64__
  offset = 512;
#endif

  hf = (handover_func)((char *)kernel_mem + handover_offset + offset);

  asm volatile ("cli");

  hf (holy_efi_image_handle, holy_efi_system_table, params);

  /* Not reached */
  return holy_ERR_NONE;
}

static holy_err_t
holy_linuxefi_unload (void)
{
  holy_dl_unref (my_mod);
  loaded = 0;
  if (initrd_mem)
    holy_efi_free_pages((holy_efi_physical_address_t)initrd_mem, BYTES_TO_PAGES(params->ramdisk_size));
  if (linux_cmdline)
    holy_efi_free_pages((holy_efi_physical_address_t)linux_cmdline, BYTES_TO_PAGES(params->cmdline_size + 1));
  if (kernel_mem)
    holy_efi_free_pages((holy_efi_physical_address_t)kernel_mem, BYTES_TO_PAGES(kernel_size));
  if (params)
    holy_efi_free_pages((holy_efi_physical_address_t)params, BYTES_TO_PAGES(16384));
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
                 int argc, char *argv[])
{
  holy_file_t *files = 0;
  int i, nfiles = 0;
  holy_size_t size = 0;
  holy_uint8_t *ptr;

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (!loaded)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("you need to load the kernel first"));
      goto fail;
    }

  files = holy_zalloc (argc * sizeof (files[0]));
  if (!files)
    goto fail;

  for (i = 0; i < argc; i++)
    {
      holy_file_filter_disable_compression ();
      files[i] = holy_file_open (argv[i]);
      if (! files[i])
        goto fail;
      nfiles++;
      size += ALIGN_UP (holy_file_size (files[i]), 4);
    }

  initrd_mem = holy_efi_allocate_pages_max (0x3fffffff, BYTES_TO_PAGES(size));

  if (!initrd_mem)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("can't allocate initrd"));
      goto fail;
    }

  params->ramdisk_size = size;
  params->ramdisk_image = (holy_uint32_t)(holy_uint64_t) initrd_mem;

  ptr = initrd_mem;

  for (i = 0; i < nfiles; i++)
    {
      holy_ssize_t cursize = holy_file_size (files[i]);
      if (holy_file_read (files[i], ptr, cursize) != cursize)
        {
          if (!holy_errno)
            holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
                        argv[i]);
          goto fail;
        }
      holy_tpm_measure (ptr, cursize, holy_BINARY_PCR, "holy_linuxefi", "Initrd");
      holy_print_error();
      ptr += cursize;
      holy_memset (ptr, 0, ALIGN_UP_OVERHEAD (cursize, 4));
      ptr += ALIGN_UP_OVERHEAD (cursize, 4);
    }

  params->ramdisk_size = size;

 fail:
  for (i = 0; i < nfiles; i++)
    holy_file_close (files[i]);
  holy_free (files);

  if (initrd_mem && holy_errno)
    holy_efi_free_pages((holy_efi_physical_address_t)initrd_mem, BYTES_TO_PAGES(size));

  return holy_errno;
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  struct linux_kernel_header lh;
  holy_ssize_t len, start, filelen;
  void *kernel = NULL;

  holy_dl_ref (my_mod);

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  file = holy_file_open (argv[0]);
  if (! file)
    goto fail;

  filelen = holy_file_size (file);

  kernel = holy_malloc(filelen);

  if (!kernel)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("cannot allocate kernel buffer"));
      goto fail;
    }

  if (holy_file_read (file, kernel, filelen) != filelen)
    {
      holy_error (holy_ERR_FILE_READ_ERROR, N_("Can't read kernel %s"), argv[0]);
      goto fail;
    }

  holy_tpm_measure (kernel, filelen, holy_BINARY_PCR, "holy_linuxefi", "Kernel");
  holy_print_error();

  if (! holy_linuxefi_secure_validate (kernel, filelen))
    {
      holy_error (holy_ERR_INVALID_COMMAND, N_("%s has invalid signature"), argv[0]);
      holy_free (kernel);
      goto fail;
    }

  params = holy_efi_allocate_pages_max (0x3fffffff, BYTES_TO_PAGES(16384));

  if (! params)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, "cannot allocate kernel parameters");
      goto fail;
    }

  holy_memset (params, 0, 16384);

  holy_memcpy (&lh, kernel, sizeof (lh));

  if (lh.boot_flag != holy_cpu_to_le16 (0xaa55))
    {
      holy_error (holy_ERR_BAD_OS, N_("invalid magic number"));
      goto fail;
    }

  if (lh.setup_sects > holy_LINUX_MAX_SETUP_SECTS)
    {
      holy_error (holy_ERR_BAD_OS, N_("too many setup sectors"));
      goto fail;
    }

  if (lh.version < holy_cpu_to_le16 (0x020b))
    {
      holy_error (holy_ERR_BAD_OS, N_("kernel too old"));
      goto fail;
    }

  if (!lh.handover_offset)
    {
      holy_error (holy_ERR_BAD_OS, N_("kernel doesn't support EFI handover"));
      goto fail;
    }

  linux_cmdline = holy_efi_allocate_pages_max(0x3fffffff,
					 BYTES_TO_PAGES(lh.cmdline_size + 1));

  if (!linux_cmdline)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("can't allocate cmdline"));
      goto fail;
    }

  holy_memcpy (linux_cmdline, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  holy_create_loader_cmdline (argc, argv,
                              linux_cmdline + sizeof (LINUX_IMAGE) - 1,
			      lh.cmdline_size - (sizeof (LINUX_IMAGE) - 1));

  holy_pass_verity_hash(&lh, linux_cmdline, lh.cmdline_size);
  lh.cmd_line_ptr = (holy_uint32_t)(holy_uint64_t)linux_cmdline;

  handover_offset = lh.handover_offset;

  start = (lh.setup_sects + 1) * 512;
  len = holy_file_size(file) - start;

  kernel_mem = holy_efi_allocate_pages(lh.pref_address,
				       BYTES_TO_PAGES(lh.init_size));

  if (!kernel_mem)
    kernel_mem = holy_efi_allocate_pages_max(0x3fffffff,
					     BYTES_TO_PAGES(lh.init_size));

  if (!kernel_mem)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("can't allocate kernel"));
      goto fail;
    }

  holy_memcpy (kernel_mem, (char *)kernel + start, len);
  holy_loader_set (holy_linuxefi_boot, holy_linuxefi_unload, 0);
  loaded=1;

  lh.code32_start = (holy_uint32_t)(holy_uint64_t) kernel_mem;
  holy_memcpy (params, &lh, 2 * 512);

  params->type_of_loader = 0x21;

 fail:

  if (file)
    holy_file_close (file);

  holy_free (kernel);

  if (holy_errno != holy_ERR_NONE)
    {
      holy_dl_unref (my_mod);
      loaded = 0;
    }

  if (linux_cmdline && !loaded)
    holy_efi_free_pages((holy_efi_physical_address_t)linux_cmdline, BYTES_TO_PAGES(lh.cmdline_size + 1));

  if (kernel_mem && !loaded)
    holy_efi_free_pages((holy_efi_physical_address_t)kernel_mem, BYTES_TO_PAGES(kernel_size));

  if (params && !loaded)
    holy_efi_free_pages((holy_efi_physical_address_t)params, BYTES_TO_PAGES(16384));

  return holy_errno;
}

static holy_command_t cmd_linux, cmd_initrd;

holy_MOD_INIT(linuxefi)
{
  cmd_linux =
    holy_register_command ("linuxefi", holy_cmd_linux,
                           0, N_("Load Linux."));
  cmd_initrd =
    holy_register_command ("initrdefi", holy_cmd_initrd,
                           0, N_("Load initrd."));
  my_mod = mod;
}

holy_MOD_FINI(linuxefi)
{
  holy_unregister_command (cmd_linux);
  holy_unregister_command (cmd_initrd);
}
