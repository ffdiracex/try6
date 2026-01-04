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
#include <holy/machine/pxe.h>
#include <holy/net.h>

static holy_dl_t my_mod;
static struct holy_relocator *rel;
static holy_uint32_t edx = 0xffffffff;
static char boot_file[128];
static char server_name[64];

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_pxechain_boot (void)
{
  struct holy_relocator16_state state = {
    .cs = 0,
    .ip = 0x7c00,
    .ds = 0,
    .es = 0,
    .fs = 0,
    .gs = 0,
    .ss = 0,
    .sp = 0x7c00,
    .edx = edx
  };
  struct holy_net_bootp_packet *bp;

  bp = holy_pxe_get_cached (holy_PXENV_PACKET_TYPE_DHCP_ACK);

  holy_video_set_mode ("text", 0, 0);

  if (bp && boot_file[0])
    holy_memcpy (bp->boot_file, boot_file, sizeof (bp->boot_file));
  if (bp && server_name[0])
    holy_memcpy (bp->server_name, server_name, sizeof (bp->server_name));

  return holy_relocator16_boot (rel, state);
}

static holy_err_t
holy_pxechain_unload (void)
{
  holy_relocator_unload (rel);
  rel = NULL;
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_pxechain (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_err_t err;
  void *image;
  holy_size_t imagesize;
  char *fname;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_dl_ref (my_mod);

  rel = holy_relocator_new ();
  if (!rel)
    goto fail;

  file = holy_file_open (argv[0]);
  if (! file)
    goto fail;

  if (file->device->net && file->device->net->name)
    fname = file->device->net->name;
  else
    {
      fname = argv[0];
      if (fname[0] == '(')
	{
	  fname = holy_strchr (fname, ')');
	  if (fname)
	    fname++;
	  else
	    fname = argv[0];
	}
    }

  holy_memset (boot_file, 0, sizeof (boot_file));
  holy_strncpy (boot_file, fname, sizeof (boot_file));

  holy_memset (server_name, 0, sizeof (server_name));
  if (file->device->net && file->device->net->server)
    holy_strncpy (server_name, file->device->net->server, sizeof (server_name));

  edx = holy_get_root_biosnumber ();

  imagesize = holy_file_size (file);
  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (rel, &ch, 0x7c00, imagesize);
    if (err)
      goto fail;
    image = get_virtual_current_address (ch);
  }

  if (holy_file_read (file, image, imagesize) != (holy_ssize_t) imagesize)
    goto fail;
 
  holy_loader_set (holy_pxechain_boot, holy_pxechain_unload,
		   holy_LOADER_FLAG_NORETURN | holy_LOADER_FLAG_PXE_NOT_UNLOAD);
  return holy_ERR_NONE;

 fail:

  if (file)
    holy_file_close (file);

  holy_pxechain_unload ();

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(pxechainloader)
{
  cmd = holy_register_command ("pxechainloader", holy_cmd_pxechain,
			       0, N_("Load a PXE image."));
  my_mod = mod;
}

holy_MOD_FINI(pxechainloader)
{
  holy_unregister_command (cmd);
}
