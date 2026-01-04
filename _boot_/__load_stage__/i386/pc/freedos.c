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
static holy_uint32_t ebx = 0xffffffff;

#define holy_FREEDOS_SEGMENT         0x60
#define holy_FREEDOS_ADDR            (holy_FREEDOS_SEGMENT << 4)
#define holy_FREEDOS_STACK_SEGMENT         0x1fe0
#define holy_FREEDOS_STACK_BPB_POINTER     0x7c00
#define holy_FREEDOS_BPB_ADDR        ((holy_FREEDOS_STACK_SEGMENT << 4) \
                                       + holy_FREEDOS_STACK_BPB_POINTER)

/* FreeDOS boot.asm passes register sp as exactly this. Importantly,
   it must point below the BPB (to avoid overwriting any of it). */
#define holy_FREEDOS_STACK_POINTER         (holy_FREEDOS_STACK_BPB_POINTER \
                                             - 0x60)

/* In this, the additional 8192 bytes are the stack reservation; the
   remaining parts trivially give the maximum allowed size. */
#define holy_FREEDOS_MAX_SIZE        ((holy_FREEDOS_STACK_SEGMENT << 4) \
                                       + holy_FREEDOS_STACK_POINTER \
                                       - holy_FREEDOS_ADDR \
                                       - 8192)

static holy_err_t
holy_freedos_boot (void)
{
  struct holy_relocator16_state state = {
    .cs = holy_FREEDOS_SEGMENT,
    .ip = 0,

    .ds = holy_FREEDOS_STACK_SEGMENT,
    .es = 0,
    .fs = 0,
    .gs = 0,
    .ss = holy_FREEDOS_STACK_SEGMENT,
    .sp = holy_FREEDOS_STACK_POINTER,
    .ebp = holy_FREEDOS_STACK_BPB_POINTER,
    .ebx = ebx,
    .edx = ebx,
    .a20 = 1
  };
  holy_video_set_mode ("text", 0, 0);

  return holy_relocator16_boot (rel, state);
}

static holy_err_t
holy_freedos_unload (void)
{
  holy_relocator_unload (rel);
  rel = NULL;
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_freedos (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_err_t err;
  void *bs, *kernelsys;
  holy_size_t kernelsyssize;
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
    err = holy_relocator_alloc_chunk_addr (rel, &ch, holy_FREEDOS_BPB_ADDR,
					   holy_DISK_SECTOR_SIZE);
    if (err)
      goto fail;
    bs = get_virtual_current_address (ch);
  }

  ebx = holy_get_root_biosnumber ();
  dev = holy_device_open (0);

  if (dev && dev->disk)
    {
      err = holy_disk_read (dev->disk, 0, 0, holy_DISK_SECTOR_SIZE, bs);
      if (err)
	{
	  holy_device_close (dev);
	  goto fail;
	}
      holy_chainloader_patch_bpb (bs, dev, ebx);
    }

  if (dev)
    holy_device_close (dev);

  kernelsyssize = holy_file_size (file);

  if (kernelsyssize > holy_FREEDOS_MAX_SIZE)
    {
      holy_error (holy_ERR_BAD_OS,
		  N_("the size of `%s' is too large"), argv[0]);
      goto fail;
    }

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (rel, &ch, holy_FREEDOS_ADDR,
					   kernelsyssize);
    if (err)
      goto fail;
    kernelsys = get_virtual_current_address (ch);
  }

  if (holy_file_read (file, kernelsys, kernelsyssize)
      != (holy_ssize_t) kernelsyssize)
    goto fail;
 
  holy_loader_set (holy_freedos_boot, holy_freedos_unload, 1);
  return holy_ERR_NONE;

 fail:

  if (file)
    holy_file_close (file);

  holy_freedos_unload ();

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(freedos)
{
  cmd = holy_register_command ("freedos", holy_cmd_freedos,
			       0, N_("Load FreeDOS kernel.sys."));
  my_mod = mod;
}

holy_MOD_FINI(freedos)
{
  holy_unregister_command (cmd);
}
