/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/types.h>
#include <holy/crypto.h>
#include <holy/auth.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/i18n.h>
#include <holy/misc.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE	1

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"


#include "progname.h"

static struct argp_option options[] = {
  {"iteration-count",  'c', N_("NUM"), 0, N_("Number of PBKDF2 iterations"), 0},
  {"buflen",  'l', N_("NUM"), 0, N_("Length of generated hash"), 0},
  {"salt",  's', N_("NUM"), 0, N_("Length of salt"), 0},
  { 0, 0, 0, 0, 0, 0 }
};

struct arguments
{
  unsigned int count;
  unsigned int buflen;
  unsigned int saltlen;
};

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'c':
      arguments->count = strtoul (arg, NULL, 0);
      break;

    case 'l':
      arguments->buflen = strtoul (arg, NULL, 0);
      break;

    case 's':
      arguments->saltlen = strtoul (arg, NULL, 0);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("[OPTIONS]"),
  N_("Generate PBKDF2 password hash."),
  NULL, NULL, NULL
};


static void
hexify (char *hex, holy_uint8_t *bin, holy_size_t n)
{
  while (n--)
    {
      if (((*bin & 0xf0) >> 4) < 10)
	*hex = ((*bin & 0xf0) >> 4) + '0';
      else
	*hex = ((*bin & 0xf0) >> 4) + 'A' - 10;
      hex++;

      if ((*bin & 0xf) < 10)
	*hex = (*bin & 0xf) + '0';
      else
	*hex = (*bin & 0xf) + 'A' - 10;
      hex++;
      bin++;
    }
  *hex = 0;
}

int
main (int argc, char *argv[])
{
  struct arguments arguments = {
    .count = 10000,
    .buflen = 64,
    .saltlen = 64
  };
  char *result, *ptr;
  gcry_err_code_t gcry_err;
  holy_uint8_t *buf, *salt;
  char pass1[holy_AUTH_MAX_PASSLEN];
  char pass2[holy_AUTH_MAX_PASSLEN];

  holy_util_host_init (&argc, &argv);

  /* Check for options.  */
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  buf = xmalloc (arguments.buflen);
  salt = xmalloc (arguments.saltlen);
  
  printf ("%s", _("Enter password: "));
  if (!holy_password_get (pass1, holy_AUTH_MAX_PASSLEN))
    {
      free (buf);
      free (salt);
      holy_util_error ("%s", _("failure to read password"));
    }
  printf ("%s", _("Reenter password: "));
  if (!holy_password_get (pass2, holy_AUTH_MAX_PASSLEN))
    {
      free (buf);
      free (salt);
      holy_util_error ("%s", _("failure to read password"));
    }

  if (strcmp (pass1, pass2) != 0)
    {
      memset (pass1, 0, sizeof (pass1));
      memset (pass2, 0, sizeof (pass2));
      free (buf);
      free (salt);
      holy_util_error ("%s", _("passwords don't match"));
    }
  memset (pass2, 0, sizeof (pass2));

  if (holy_get_random (salt, arguments.saltlen))
    {
      memset (pass1, 0, sizeof (pass1));
      free (buf);
      free (salt);
      holy_util_error ("%s", _("couldn't retrieve random data for salt"));
    }

  gcry_err = holy_crypto_pbkdf2 (holy_MD_SHA512,
				 (holy_uint8_t *) pass1, strlen (pass1),
				 salt, arguments.saltlen,
				 arguments.count, buf, arguments.buflen);
  memset (pass1, 0, sizeof (pass1));

  if (gcry_err)
    {
      memset (buf, 0, arguments.buflen);
      free (buf);
      memset (salt, 0, arguments.saltlen);
      free (salt);
      holy_util_error (_("cryptographic error number %d"), gcry_err);
    }

  result = xmalloc (sizeof ("holy.pbkdf2.sha512.XXXXXXXXXXXXXXXXXXX.S.S")
		    + arguments.buflen * 2 + arguments.saltlen * 2);
  ptr = result;
  memcpy (ptr, "holy.pbkdf2.sha512.", sizeof ("holy.pbkdf2.sha512.") - 1);
  ptr += sizeof ("holy.pbkdf2.sha512.") - 1;
  
  holy_snprintf (ptr, sizeof ("XXXXXXXXXXXXXXXXXXX"), "%d", arguments.count);
  ptr += strlen (ptr);
  *ptr++ = '.';
  hexify (ptr, salt, arguments.saltlen);
  ptr += arguments.saltlen * 2;
  *ptr++ = '.';
  hexify (ptr, buf, arguments.buflen);
  ptr += arguments.buflen * 2;
  *ptr = '\0';

  printf (_("PBKDF2 hash of your password is %s\n"), result);
  memset (buf, 0, arguments.buflen);
  free (buf);
  memset (salt, 0, arguments.saltlen);
  free (salt);

  return 0;
}
