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
#include <holy/deflate.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;
static struct holy_relocator *rel;
static holy_uint32_t edx = 0xffffffff;
static holy_uint16_t sp;
static holy_uint32_t destaddr;

#define holy_TRUECRYPT_SEGMENT         0x2000

static holy_err_t
holy_truecrypt_boot (void)
{
  holy_uint16_t segment = destaddr >> 4;
  struct holy_relocator16_state state = {
    .cs = segment,
    .ds = segment,
    .es = segment,
    .fs = segment,
    .gs = segment,
    .ss = segment,
    .ip = 0x100,
    .sp = sp,
    .edx = edx,
    .a20 = 1
  };
  holy_video_set_mode ("text", 0, 0);

  return holy_relocator16_boot (rel, state);
}

static holy_err_t
holy_truecrypt_unload (void)
{
  holy_relocator_unload (rel);
  rel = NULL;
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

/* Information on protocol supplied by Attila Lendvai.  */
#define MAGIC "\0CD001\1EL TORITO SPECIFICATION"

static holy_err_t
holy_cmd_truecrypt (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_err_t err;
  void *truecrypt;
  holy_size_t truecryptsize;
  const holy_size_t truecryptmemsize = 42 * 1024;
  holy_uint8_t dh;
  holy_uint32_t catalog, rba;
  holy_uint8_t buf[128];
  char *compressed = NULL;
  char *uncompressed = NULL;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  rel = NULL;

  holy_dl_ref (my_mod);

  file = holy_file_open (argv[0]);
  if (! file)
    goto fail;

  if (holy_file_seek (file, 17 * 2048) == (holy_size_t) -1)
    goto fail;

  if (holy_file_read (file, buf, sizeof (buf))
      != sizeof (buf))
    goto fail;

  if (holy_memcmp (buf, MAGIC, sizeof (MAGIC)) != 0)
    {
      holy_error (holy_ERR_BAD_OS, "invalid eltorito signature");
      goto fail;
    }

  catalog = holy_get_unaligned32 (buf + 0x47);

  if (holy_file_seek (file, catalog * 2048) == (holy_size_t)-1)
    goto fail;

  if (holy_file_read (file, buf, sizeof (buf))
      != sizeof (buf))
    goto fail;

  if (buf[0] != 1 || buf[1] != 0 || buf[0x1e] != 0x55
      || buf[0x1f] != 0xaa || buf[0x20] != 0x88
      || buf[0x26] != 1 || buf[0x27] != 0)
    {
      holy_error (holy_ERR_BAD_OS, "invalid eltorito catalog");
      goto fail;
    }

  rba = holy_get_unaligned32 (buf + 0x28);

  if (holy_file_seek (file, rba * 2048 + 0x1b7) == (holy_size_t) -1)
    goto fail;

  if (holy_file_read (file, &dh, 1)
      != 1)
    goto fail;

  if (holy_file_seek (file, rba * 2048 + 512 + 2048) == (holy_size_t) -1)
    goto fail;

  compressed = holy_malloc (57 * 512);
  if (!compressed)
    goto fail;

  if (holy_file_read (file, compressed, 57 * 512)
      != 57 * 512)
    goto fail;

  uncompressed = holy_malloc (truecryptmemsize);
  if (!uncompressed)
    goto fail;

  /* It's actually gzip but our gzip decompressor isn't able to handle
     trailing garbage, hence let's use raw decompressor.  */
  truecryptsize = holy_deflate_decompress (compressed + 10, 57 * 512 - 10,
					   0, uncompressed, truecryptmemsize);
  if ((holy_ssize_t) truecryptsize < 0)
    goto fail;

  if (truecryptmemsize <= truecryptsize + 0x100)
    {
      holy_error (holy_ERR_BAD_OS, "file is too big");
      goto fail;
    }

  rel = holy_relocator_new ();
  if (!rel)
    goto fail;

  edx = (dh << 8) | holy_get_root_biosnumber ();

  destaddr = ALIGN_DOWN (holy_min (0x90000, holy_mmap_get_lower ())
			 - truecryptmemsize, 64 * 1024);

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (rel, &ch, destaddr,
					   truecryptmemsize);
    if (err)
      goto fail;
    truecrypt = get_virtual_current_address (ch);
  }

  holy_memset (truecrypt, 0, 0x100);
  holy_memcpy ((char *) truecrypt + 0x100, uncompressed, truecryptsize);

  holy_memset ((char *) truecrypt + truecryptsize + 0x100,
	       0, truecryptmemsize - truecryptsize - 0x100);
  sp = truecryptmemsize - 4;
 
  holy_loader_set (holy_truecrypt_boot, holy_truecrypt_unload, 1);

  holy_free (uncompressed);
  holy_free (compressed);

  return holy_ERR_NONE;

 fail:

  if (!holy_errno)
    holy_error (holy_ERR_BAD_OS, "bad truecrypt ISO");

  if (file)
    holy_file_close (file);

  holy_truecrypt_unload ();

  holy_free (uncompressed);
  holy_free (compressed);

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(truecrypt)
{
  cmd = holy_register_command ("truecrypt", holy_cmd_truecrypt,
			       0, N_("Load Truecrypt ISO."));
  my_mod = mod;
}

holy_MOD_FINI(truecrypt)
{
  holy_unregister_command (cmd);
}
