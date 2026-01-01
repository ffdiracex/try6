/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include <wchar.h>

#include <errno.h>
#include <stdlib.h>


size_t
wcrtomb (char *s, wchar_t wc, mbstate_t *ps)
{
  /* This implementation of wcrtomb on top of wctomb() supports only
     stateless encodings.  ps must be in the initial state.  */
  if (ps != NULL && !mbsinit (ps))
    {
      errno = EINVAL;
      return (size_t)(-1);
    }

  if (s == NULL)
    /* We know the NUL wide character corresponds to the NUL character.  */
    return 1;
  else
    {
      int ret = wctomb (s, wc);

      if (ret >= 0)
        return ret;
      else
        {
          errno = EILSEQ;
          return (size_t)(-1);
        }
    }
}
