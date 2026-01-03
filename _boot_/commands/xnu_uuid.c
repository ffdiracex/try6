/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/fs.h>
#include <holy/file.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/crypto.h>

holy_MOD_LICENSE ("GPLv2+");

/* This prefix is used by xnu and boot-132 to hash
   together with volume serial. */
static holy_uint8_t hash_prefix[16]
  = {0xB3, 0xE2, 0x0F, 0x39, 0xF2, 0x92, 0x11, 0xD6,
     0x97, 0xA4, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC};

static holy_err_t
holy_cmd_xnu_uuid (holy_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  holy_uint64_t serial;
  holy_uint8_t *xnu_uuid;
  char uuid_string[sizeof ("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")];
  char *ptr;
  void *ctx;
  int low = 0;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "UUID required");

  if (argc > 1 && holy_strcmp (args[0], "-l") == 0)
    {
      low = 1;
      argc--;
      args++;
    }

  serial = holy_cpu_to_be64 (holy_strtoull (args[0], 0, 16));

  ctx = holy_zalloc (holy_MD_MD5->contextsize);
  if (!ctx)
    return holy_errno;
  holy_MD_MD5->init (ctx);
  holy_MD_MD5->write (ctx, hash_prefix, sizeof (hash_prefix));
  holy_MD_MD5->write (ctx, &serial, sizeof (serial));
  holy_MD_MD5->final (ctx);
  xnu_uuid = holy_MD_MD5->read (ctx);

  holy_snprintf (uuid_string, sizeof (uuid_string),
		"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		(unsigned int) xnu_uuid[0], (unsigned int) xnu_uuid[1],
		(unsigned int) xnu_uuid[2], (unsigned int) xnu_uuid[3],
		(unsigned int) xnu_uuid[4], (unsigned int) xnu_uuid[5],
		(unsigned int) ((xnu_uuid[6] & 0xf) | 0x30),
		(unsigned int) xnu_uuid[7],
		(unsigned int) ((xnu_uuid[8] & 0x3f) | 0x80),
		(unsigned int) xnu_uuid[9],
		(unsigned int) xnu_uuid[10], (unsigned int) xnu_uuid[11],
		(unsigned int) xnu_uuid[12], (unsigned int) xnu_uuid[13],
		(unsigned int) xnu_uuid[14], (unsigned int) xnu_uuid[15]);
  if (!low)
    for (ptr = uuid_string; *ptr; ptr++)
      *ptr = holy_toupper (*ptr);
  if (argc == 1)
    holy_printf ("%s\n", uuid_string);
  if (argc > 1)
    holy_env_set (args[1], uuid_string);

  holy_free (ctx);

  return holy_ERR_NONE;
}

static holy_command_t cmd;


holy_MOD_INIT (xnu_uuid)
{
  cmd = holy_register_command ("xnu_uuid", holy_cmd_xnu_uuid,
			       /* TRANSLATORS: holyUUID stands for "filesystem
				  UUID as used in holy".  */
			       N_("[-l] holyUUID [VARNAME]"),
			       N_("Transform 64-bit UUID to format "
				  "suitable for XNU. If -l is given keep "
				  "it lowercase as done by blkid."));
}

holy_MOD_FINI (xnu_uuid)
{
  holy_unregister_command (cmd);
}
