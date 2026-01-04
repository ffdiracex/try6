/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/env.h>
#include <holy/cpu/io.h>
#include <holy/i18n.h>
#include <holy/mm.h>

holy_MOD_LICENSE ("GPLv2+");

#define SELECTOR 0x510
#define DATA 0x511

#define SIGNATURE_INDEX 0x00
#define DIRECTORY_INDEX 0x19

static holy_extcmd_t cmd_read_fwconfig;

struct holy_qemu_fwcfgfile {
  holy_uint32_t size;
  holy_uint16_t select;
  holy_uint16_t reserved;
  char name[56];
};

static const struct holy_arg_option options[] =
  {
    {0, 'v', 0, N_("Save read value into variable VARNAME."),
     N_("VARNAME"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static holy_err_t
holy_cmd_fwconfig (holy_extcmd_context_t ctxt __attribute__ ((unused)),
                   int argc, char **argv)
{
  holy_uint32_t i, j, value = 0;
  struct holy_qemu_fwcfgfile file;
  char fwsig[4], signature[4] = { 'Q', 'E', 'M', 'U' };

  if (argc != 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  /* Verify that we have meaningful hardware here */
  holy_outw(SIGNATURE_INDEX, SELECTOR);
  for (i=0; i<sizeof(fwsig); i++)
      fwsig[i] = holy_inb(DATA);

  if (holy_memcmp(fwsig, signature, sizeof(signature)) != 0)
    return holy_error (holy_ERR_BAD_DEVICE, N_("invalid fwconfig hardware signature: got 0x%x%x%x%x"), fwsig[0], fwsig[1], fwsig[2], fwsig[3]);

  /* Find out how many file entries we have */
  holy_outw(DIRECTORY_INDEX, SELECTOR);
  value = holy_inb(DATA) | holy_inb(DATA) << 8 | holy_inb(DATA) << 16 | holy_inb(DATA) << 24;
  value = holy_be_to_cpu32(value);
  /* Read the file description for each file */
  for (i=0; i<value; i++)
    {
      for (j=0; j<sizeof(file); j++)
	{
	  ((char *)&file)[j] = holy_inb(DATA);
	}
      /* Check whether it matches what we're looking for, and if so read the file */
      if (holy_strncmp(file.name, argv[0], sizeof(file.name)) == 0)
	{
	  holy_uint32_t filesize = holy_be_to_cpu32(file.size);
	  holy_uint16_t location = holy_be_to_cpu16(file.select);
	  char *data = holy_malloc(filesize+1);

	  if (!data)
	      return holy_error (holy_ERR_OUT_OF_MEMORY, N_("can't allocate buffer for data"));

	  holy_outw(location, SELECTOR);
	  for (j=0; j<filesize; j++)
	    {
	      data[j] = holy_inb(DATA);
	    }

	  data[filesize] = '\0';

	  holy_env_set (argv[1], data);

	  holy_free(data);
	  return 0;
	}
    }

  return holy_error (holy_ERR_FILE_NOT_FOUND, N_("couldn't find entry %s"), argv[0]);
}

holy_MOD_INIT(fwconfig)
{
  cmd_read_fwconfig =
    holy_register_extcmd ("fwconfig", holy_cmd_fwconfig, 0,
			  N_("PATH VAR"),
			  N_("Set VAR to the contents of fwconfig PATH"),
			  options);
}

holy_MOD_FINI(fwconfig)
{
  holy_unregister_extcmd (cmd_read_fwconfig);
}
