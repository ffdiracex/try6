/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/efi/efi.h>
#include <holy/dl.h>
#include <holy/env.h>
#include <holy/err.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/misc.h>
#include <holy/mm.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options_getenv[] = {
  {"var-name", 'e', 0,
   N_("Environment variable to query"),
   N_("VARNAME"), ARG_TYPE_STRING},
  {"var-guid", 'g', 0,
   N_("GUID of environment variable to query"),
   N_("GUID"), ARG_TYPE_STRING},
  {"binary", 'b', 0,
   N_("Read binary data and represent it as hex"),
   0, ARG_TYPE_NONE},
  {0, 0, 0, 0, 0, 0}
};

enum options_getenv
{
  GETENV_VAR_NAME,
  GETENV_VAR_GUID,
  GETENV_BINARY,
};

static holy_err_t
holy_cmd_getenv (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  char *envvar = NULL, *guid = NULL, *bindata = NULL, *data = NULL;
  holy_size_t datasize;
  holy_efi_guid_t efi_var_guid;
  holy_efi_boolean_t binary = state[GETENV_BINARY].set;
  unsigned int i;

  if (!state[GETENV_VAR_NAME].set || !state[GETENV_VAR_GUID].set)
    {
      holy_error (holy_ERR_INVALID_COMMAND, N_("-e and -g are required"));
      goto done;
    }

  if (argc != 1)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("unexpected arguments"));
      goto done;
    }

  envvar = state[GETENV_VAR_NAME].arg;
  guid = state[GETENV_VAR_GUID].arg;

  if (holy_strlen(guid) != 36 ||
      guid[8] != '-' ||
      guid[13] != '-' ||
      guid[18] != '-' ||
      guid[23] != '-')
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("invalid GUID"));
      goto done;
    }

  /* Forgive me father for I have sinned */
  guid[8] = 0;
  efi_var_guid.data1 = holy_strtoul(guid, NULL, 16);
  guid[13] = 0;
  efi_var_guid.data2 = holy_strtoul(guid + 9, NULL, 16);
  guid[18] = 0;
  efi_var_guid.data3 = holy_strtoul(guid + 14, NULL, 16);
  efi_var_guid.data4[7] = holy_strtoul(guid + 34, NULL, 16);
  guid[34] = 0;
  efi_var_guid.data4[6] = holy_strtoul(guid + 32, NULL, 16);
  guid[32] = 0;
  efi_var_guid.data4[5] = holy_strtoul(guid + 30, NULL, 16);
  guid[30] = 0;
  efi_var_guid.data4[4] = holy_strtoul(guid + 28, NULL, 16);
  guid[28] = 0;
  efi_var_guid.data4[3] = holy_strtoul(guid + 26, NULL, 16);
  guid[26] = 0;
  efi_var_guid.data4[2] = holy_strtoul(guid + 24, NULL, 16);
  guid[23] = 0;
  efi_var_guid.data4[1] = holy_strtoul(guid + 21, NULL, 16);
  guid[21] = 0;
  efi_var_guid.data4[0] = holy_strtoul(guid + 19, NULL, 16);

  data = holy_efi_get_variable(envvar, &efi_var_guid, &datasize);

  if (!data || !datasize)
    {
      holy_error (holy_ERR_FILE_NOT_FOUND, N_("No such variable"));
      goto done;
    }

  if (binary)
    {
      bindata = holy_zalloc(datasize * 2 + 1);
      for (i=0; i<datasize; i++)
	  holy_snprintf(bindata + i*2, 3, "%02x", data[i] & 0xff);

      if (holy_env_set (args[0], bindata))
	goto done;
    }
  else if (holy_env_set (args[0], data))
    {
      goto done;
    }

  holy_errno = holy_ERR_NONE;

done:
  holy_free(bindata);
  holy_free(data);
  return holy_errno;
}

static holy_extcmd_t cmd_getenv;

holy_MOD_INIT(getenv)
{
  cmd_getenv = holy_register_extcmd ("getenv", holy_cmd_getenv, 0,
				   N_("-e envvar -g guidenv setvar"),
				   N_("Read a firmware environment variable"),
				   options_getenv);
}

holy_MOD_FINI(getenv)
{
  holy_unregister_extcmd (cmd_getenv);
}
