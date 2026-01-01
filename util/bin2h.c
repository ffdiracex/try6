/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
  int b, i;
  char *sym;

  if (2 != argc)
    return 1;

  sym = argv[1];

  b = getchar ();
  if (b == EOF)
    goto abort;

  printf ("/* THIS CHUNK OF BYTES IS AUTOMATICALLY GENERATED */\n"
	  "unsigned char %s[] =\n{\n", sym);

  while (1)
    {
      printf ("0x%02x", b);

      b = getchar ();
      if (b == EOF)
	goto end;

      for (i = 0; i < 16 - 1; i++)
	{
	  printf (", 0x%02x", b);

	  b = getchar ();
	  if (b == EOF)
	    goto end;
	}

      printf (",\n");
    }

end:
  printf ("\n};\n");

abort:
  exit (0);
}
