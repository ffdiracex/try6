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
#include <holy/types.h>
#include <holy/partition.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/machine/biosnum.h>
#include <holy/i18n.h>
#include <holy/video.h>
#include <holy/mm.h>
#include <holy/cpu/relocator.h>
#include <holy/machine/chainloader.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;
static struct holy_relocator *rel;
static holy_uint32_t edx = 0xffffffff;

#define holy_NTLDR_SEGMENT         0x2000

static holy_err_t
holy_ntldr_boot (void)
{
  struct holy_relocator16_state state = {
    .cs = holy_NTLDR_SEGMENT,
    .ip = 0,
    .ds = 0,
    .es = 0,
    .fs = 0,
    .gs = 0,
    .ss = 0,
    .sp = 0x7c00,
    .edx = edx,
    .a20 = 1
  };
  holy_video_set_mode ("text", 0, 0);

  return holy_relocator16_boot (rel, state);
}

static holy_err_t
holy_ntldr_unload (void)
{
  holy_relocator_unload (rel);
  rel = NULL;
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_ntldr (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_err_t err;
  void *bs, *ntldr;
  holy_size_t ntldrsize;
  holy_device_t dev;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_dl_ref (my_mod);

  rel = holy_relocator_new ();
  if (!rel)
    goto fail;

  file = holy_file_open (argv[0]);
  if (! file)
    goto fail;

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (rel, &ch, 0x7C00,
					   holy_DISK_SECTOR_SIZE);
    if (err)
      goto fail;
    bs = get_virtual_current_address (ch);
  }

  edx = holy_get_root_biosnumber ();
  dev = holy_device_open (0);

  if (dev && dev->disk)
    {
      err = holy_disk_read (dev->disk, 0, 0, holy_DISK_SECTOR_SIZE, bs);
      if (err)
	{
	  holy_device_close (dev);
	  goto fail;
	}
      holy_chainloader_patch_bpb (bs, dev, edx);
    }

  if (dev)
    holy_device_close (dev);

  ntldrsize = holy_file_size (file);
  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (rel, &ch, holy_NTLDR_SEGMENT << 4,
					   ntldrsize);
    if (err)
      goto fail;
    ntldr = get_virtual_current_address (ch);
  }

  if (holy_file_read (file, ntldr, ntldrsize)
      != (holy_ssize_t) ntldrsize)
    goto fail;
 
  holy_loader_set (holy_ntldr_boot, holy_ntldr_unload, 1);
  return holy_ERR_NONE;

 fail:

  if (file)
    holy_file_close (file);

  holy_ntldr_unload ();

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(ntldr)
{
  cmd = holy_register_command ("ntldr", holy_cmd_ntldr,
			       0, N_("Load NTLDR or BootMGR."));
  my_mod = mod;
}

holy_MOD_FINI(ntldr)
{
  holy_unregister_command (cmd);
}
