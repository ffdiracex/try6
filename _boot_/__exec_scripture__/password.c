/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/auth.h>
#include <holy/crypto.h>
#include <holy/list.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/normal.h>
#include <holy/dl.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;

static holy_err_t
check_password (const char *user, const char *entered,
		void *password)
{
  if (holy_crypto_memcmp (entered, password, holy_AUTH_MAX_PASSLEN) != 0)
    return holy_ACCESS_DENIED;

  holy_auth_authenticate (user);

  return holy_ERR_NONE;
}

holy_err_t
holy_normal_set_password (const char *user, const char *password)
{
  holy_err_t err;
  char *pass;
  int copylen;

  pass = holy_zalloc (holy_AUTH_MAX_PASSLEN);
  if (!pass)
    return holy_errno;
  copylen = holy_strlen (password);
  if (copylen >= holy_AUTH_MAX_PASSLEN)
    copylen = holy_AUTH_MAX_PASSLEN - 1;
  holy_memcpy (pass, password, copylen);

  err = holy_auth_register_authentication (user, check_password, pass);
  if (err)
    {
      holy_free (pass);
      return err;
    }
  holy_dl_ref (my_mod);
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_password (holy_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  if (argc != 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));
  return holy_normal_set_password (args[0], args[1]);
}

static holy_command_t cmd;

holy_MOD_INIT(password)
{
  my_mod = mod;
  cmd = holy_register_command ("password", holy_cmd_password,
			       N_("USER PASSWORD"),
			       N_("Set user password (plaintext). "
			       "Unrecommended and insecure."));
}

holy_MOD_FINI(password)
{
  holy_unregister_command (cmd);
}
