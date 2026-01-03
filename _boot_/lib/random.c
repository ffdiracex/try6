/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/random.h>
#include <holy/dl.h>
#include <holy/lib/hexdump.h>
#include <holy/command.h>
#include <holy/mm.h>

holy_MOD_LICENSE ("GPLv2+");

holy_err_t
holy_crypto_get_random (void *buffer, holy_size_t sz)
{
  /* This is an arbitrer between different methods.
     TODO: Add more methods in the future.  */
  /* TODO: Add some PRNG smartness to reduce damage from bad entropy. */
  if (holy_crypto_arch_get_random (buffer, sz))
    return holy_ERR_NONE;
  return holy_error (holy_ERR_IO, "no random sources found");
}

static int
get_num_digits (int val)
{
  int ret = 0;
  while (val != 0)
    {
      ret++;
      val /= 10;
    }
  if (ret == 0)
    return 1;
  return ret;
}

#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static holy_err_t
holy_cmd_hexdump_random (holy_command_t cmd __attribute__ ((unused)), int argc, char **args)
{
  holy_size_t length = 64;
  holy_err_t err;
  void *buffer;
  holy_uint8_t *ptr;
  int stats[256];
  int i, digits = 2;
  char template[10];

  if (argc >= 1)
    length = holy_strtoull (args[0], 0, 0);

  if (length == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, "length pust be positive");

  buffer = holy_malloc (length);
  if (!buffer)
    return holy_errno;

  err = holy_crypto_get_random (buffer, length);
  if (err)
    {
      holy_free (buffer);
      return err;
    }

  hexdump (0, buffer, length);
  holy_memset(stats, 0, sizeof(stats));
  for (ptr = buffer; ptr < (holy_uint8_t *) buffer + length; ptr++)
    stats[*ptr]++;
  holy_printf ("Statistics:\n");
  for (i = 0; i < 256; i++)
    {
      int z = get_num_digits (stats[i]);
      if (z > digits)
	digits = z;
    }

  holy_snprintf (template, sizeof (template), "%%0%dd ", digits);

  for (i = 0; i < 256; i++)
    {
      holy_printf ("%s", template);//, stats[i]);
      if ((i & 0xf) == 0xf)
	holy_printf ("\n");
    }

  holy_free (buffer);

  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT (random)
{
  cmd = holy_register_command ("hexdump_random", holy_cmd_hexdump_random,
			      N_("[LENGTH]"),
			      N_("Hexdump random data."));
}

holy_MOD_FINI (random)
{
  holy_unregister_command (cmd);
}
