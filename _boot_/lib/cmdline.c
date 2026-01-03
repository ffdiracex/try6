/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/lib/cmdline.h>
#include <holy/misc.h>
#include <holy/tpm.h>

static unsigned int check_arg (char *c, int *has_space)
{
  int space = 0;
  unsigned int size = 0;

  while (*c)
    {
      if (*c == '\\' || *c == '\'' || *c == '"')
	size++;
      else if (*c == ' ')
	space = 1;

      size++;
      c++;
    }

  if (space)
    size += 2;

  if (has_space)
    *has_space = space;

  return size;
}

unsigned int holy_loader_cmdline_size (int argc, char *argv[])
{
  int i;
  unsigned int size = 0;

  for (i = 0; i < argc; i++)
    {
      size += check_arg (argv[i], 0);
      size++; /* Separator space or NULL.  */
    }

  if (size == 0)
    size = 1;

  return size;
}

int holy_create_loader_cmdline (int argc, char *argv[], char *buf,
				holy_size_t size)
{
  int i, space;
  unsigned int arg_size;
  char *c, *orig = buf;

  for (i = 0; i < argc; i++)
    {
      c = argv[i];
      arg_size = check_arg(argv[i], &space);
      arg_size++; /* Separator space or NULL.  */

      if (size < arg_size)
	break;

      size -= arg_size;

      if (space)
	*buf++ = '"';

      while (*c)
	{
	  if (*c == '\\' || *c == '\'' || *c == '"')
	    *buf++ = '\\';

	  *buf++ = *c;
	  c++;
	}

      if (space)
	*buf++ = '"';

      *buf++ = ' ';
    }

  /* Replace last space with null.  */
  if (i)
    buf--;

  *buf = 0;

  holy_tpm_measure ((void *)orig, holy_strlen (orig), holy_ASCII_PCR,
		    "holy_kernel_cmdline", orig);
  holy_print_error();

  return i;
}
