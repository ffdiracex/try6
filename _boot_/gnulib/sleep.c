/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include <unistd.h>

#include <limits.h>

#include "verify.h"

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__

# define WIN32_LEAN_AND_MEAN  /* avoid including junk */
# include <windows.h>

unsigned int
sleep (unsigned int seconds)
{
  unsigned int remaining;

  /* Sleep for 1 second many times, because
       1. Sleep is not interruptible by Ctrl-C,
       2. we want to avoid arithmetic overflow while multiplying with 1000.  */
  for (remaining = seconds; remaining > 0; remaining--)
    Sleep (1000);

  return remaining;
}

#elif HAVE_SLEEP

# undef sleep

/* Guarantee unlimited sleep and a reasonable return value.  Cygwin
   1.5.x rejects attempts to sleep more than 49.7 days (2**32
   milliseconds), but uses uninitialized memory which results in a
   garbage answer.  Similarly, Linux 2.6.9 with glibc 2.3.4 has a too
   small return value when asked to sleep more than 24.85 days.  */
unsigned int
rpl_sleep (unsigned int seconds)
{
  /* This requires int larger than 16 bits.  */
  verify (UINT_MAX / 24 / 24 / 60 / 60);
  const unsigned int limit = 24 * 24 * 60 * 60;
  while (limit < seconds)
    {
      unsigned int result;
      seconds -= limit;
      result = sleep (limit);
      if (result)
        return seconds + result;
    }
  return sleep (seconds);
}

#else /* !HAVE_SLEEP */

 #error "Please port gnulib sleep.c to your platform, possibly using usleep() or select(), then report this to bug-gnulib."

#endif
