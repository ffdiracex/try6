/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

static unsigned long long buffer[1048576];

int
main (int argc, char **argv)
{
  unsigned long long high = 0, low = 1;
  unsigned long i, j;
  unsigned long long cnt = strtoull (argv[1], 0, 0);
  struct timeval tv;
  gettimeofday (&tv, NULL);
  high = tv.tv_sec;
  low = tv.tv_usec;
  if (!high)
    high = 1;
  if (!low)
    low = 2;

  for (j = 0; j < (cnt + sizeof (buffer) - 1)  / sizeof (buffer); j++)
    {
      for (i = 0; i < sizeof (buffer) / sizeof (buffer[0]); i += 2)
	{
	  int c1 = 0, c2 = 0;
	  buffer[i] = low;
	  buffer[i+1] = high;
	  if (low & (1ULL << 63))
	    c1 = 1;
	  low <<= 1;
	  if (high & (1ULL << 63))
	    c2 = 1;
	  high = (high << 1) | c1;
	  if (c2)
	    low ^= 0x87;
	}
      if (sizeof (buffer) < cnt - sizeof (buffer) * j)
	fwrite (buffer, 1, sizeof (buffer), stdout);
      else
	fwrite (buffer, 1, cnt - sizeof (buffer) * j, stdout);
    }
  return 0;
}
