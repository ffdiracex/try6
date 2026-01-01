/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/lib/hexdump.h>

void
hexdump (unsigned long bse, char *buf, int len)
{
  int pos;
  char line[80];

  while (len > 0)
    {
      int cnt, i;

      pos = holy_snprintf (line, sizeof (line), "%08lx  ", bse);
      cnt = 16;
      if (cnt > len)
	cnt = len;

      for (i = 0; i < cnt; i++)
	{
	  pos += holy_snprintf (&line[pos], sizeof (line) - pos,
				"%02x ", (unsigned char) buf[i]);
	  if ((i & 7) == 7)
	    line[pos++] = ' ';
	}

      for (; i < 16; i++)
	{
	  pos += holy_snprintf (&line[pos], sizeof (line) - pos, "   ");
	  if ((i & 7) == 7)
	    line[pos++] = ' ';
	}

      line[pos++] = '|';

      for (i = 0; i < cnt; i++)
	line[pos++] = ((buf[i] >= 32) && (buf[i] < 127)) ? buf[i] : '.';

      line[pos++] = '|';

      line[pos] = 0;

      holy_printf ("%s\n", line);

      /* Print only first and last line if more than 3 lines are identical.  */
      if (len >= 4 * 16
	  && ! holy_memcmp (buf, buf + 1 * 16, 16)
	  && ! holy_memcmp (buf, buf + 2 * 16, 16)
	  && ! holy_memcmp (buf, buf + 3 * 16, 16))
	{
	  holy_printf ("*\n");
	  do
	    {
	      bse += 16;
	      buf += 16;
	      len -= 16;
	    }
	  while (len >= 3 * 16 && ! holy_memcmp (buf, buf + 2 * 16, 16));
	}

      bse += 16;
      buf += 16;
      len -= cnt;
    }
}
