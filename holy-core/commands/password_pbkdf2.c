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

struct pbkdf2_password
{
  holy_uint8_t *salt;
  holy_size_t saltlen;
  unsigned int c;
  holy_uint8_t *expected;
  holy_size_t buflen;
};

static holy_err_t
check_password (const char *user, const char *entered, void *pin)
{
  holy_uint8_t *buf;
  struct pbkdf2_password *pass = pin;
  gcry_err_code_t err;
  holy_err_t ret;

  buf = holy_malloc (pass->buflen);
  if (!buf)
    return holy_crypto_gcry_error (GPG_ERR_OUT_OF_MEMORY);

  err = holy_crypto_pbkdf2 (holy_MD_SHA512, (holy_uint8_t *) entered,
			    holy_strlen (entered),
			    pass->salt, pass->saltlen, pass->c,
			    buf, pass->buflen);
  if (err)
      ret = holy_crypto_gcry_error (err);
  else if (holy_crypto_memcmp (buf, pass->expected, pass->buflen) != 0)
      ret = holy_ACCESS_DENIED;
  else
    {
      holy_auth_authenticate (user);
      ret = holy_ERR_NONE;
    }

  holy_free (buf);
  return ret;
}

static inline int
hex2val (char hex)
{
  if ('0' <= hex && hex <= '9')
    return hex - '0';
  if ('a' <= hex && hex <= 'f')
    return hex - 'a' + 10;
  if ('A' <= hex && hex <= 'F')
    return hex - 'A' + 10;
  return -1;
}

static holy_err_t
holy_cmd_password (holy_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  holy_err_t err;
  char *ptr, *ptr2;
  holy_uint8_t *ptro;
  struct pbkdf2_password *pass;

  if (argc != 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  if (holy_memcmp (args[1], "holy.pbkdf2.sha512.",
		   sizeof ("holy.pbkdf2.sha512.") - 1) != 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("invalid PBKDF2 password"));

  ptr = args[1] + sizeof ("holy.pbkdf2.sha512.") - 1;

  pass = holy_malloc (sizeof (*pass));
  if (!pass)
    return holy_errno;

  pass->c = holy_strtoul (ptr, &ptr, 0);
  if (holy_errno)
    {
      holy_free (pass);
      return holy_errno;
    }
  if (*ptr != '.')
    {
      holy_free (pass);
      return holy_error (holy_ERR_BAD_ARGUMENT, N_("invalid PBKDF2 password"));
    }
  ptr++;

  ptr2 = holy_strchr (ptr, '.');
  if (!ptr2 || ((ptr2 - ptr) & 1) || holy_strlen (ptr2 + 1) & 1)
    {
      holy_free (pass);
      return holy_error (holy_ERR_BAD_ARGUMENT, N_("invalid PBKDF2 password"));
    }

  pass->saltlen = (ptr2 - ptr) >> 1;
  pass->buflen = holy_strlen (ptr2 + 1) >> 1;
  ptro = pass->salt = holy_malloc (pass->saltlen);
  if (!ptro)
    {
      holy_free (pass);
      return holy_errno;
    }
  while (ptr < ptr2)
    {
      int hex1, hex2;
      hex1 = hex2val (*ptr);
      ptr++;
      hex2 = hex2val (*ptr);
      ptr++;
      if (hex1 < 0 || hex2 < 0)
	{
	  holy_free (pass->salt);
	  holy_free (pass);
	  return holy_error (holy_ERR_BAD_ARGUMENT,
			     /* TRANSLATORS: it means that the string which
				was supposed to be a password hash doesn't
				have a correct format, not to password
				mismatch.  */
			     N_("invalid PBKDF2 password"));
	}

      *ptro = (hex1 << 4) | hex2;
      ptro++;
    }

  ptro = pass->expected = holy_malloc (pass->buflen);
  if (!ptro)
    {
      holy_free (pass->salt);
      holy_free (pass);
      return holy_errno;
    }
  ptr = ptr2 + 1;
  ptr2 += holy_strlen (ptr2);
  while (ptr < ptr2)
    {
      int hex1, hex2;
      hex1 = hex2val (*ptr);
      ptr++;
      hex2 = hex2val (*ptr);
      ptr++;
      if (hex1 < 0 || hex2 < 0)
	{
	  holy_free (pass->expected);
	  holy_free (pass->salt);
	  holy_free (pass);
	  return holy_error (holy_ERR_BAD_ARGUMENT,
			     N_("invalid PBKDF2 password"));
	}

      *ptro = (hex1 << 4) | hex2;
      ptro++;
    }

  err = holy_auth_register_authentication (args[0], check_password, pass);
  if (err)
    {
      holy_free (pass);
      return err;
    }
  holy_dl_ref (my_mod);
  return holy_ERR_NONE;
}

static holy_command_t cmd;

holy_MOD_INIT(password_pbkdf2)
{
  my_mod = mod;
  cmd = holy_register_command ("password_pbkdf2", holy_cmd_password,
			       N_("USER PBKDF2_PASSWORD"),
			       N_("Set user password (PBKDF2). "));
}

holy_MOD_FINI(password_pbkdf2)
{
  holy_unregister_command (cmd);
}
