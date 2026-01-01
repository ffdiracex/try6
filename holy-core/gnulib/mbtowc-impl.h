/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

int
mbtowc (wchar_t *pwc, const char *s, size_t n)
{
  if (s == NULL)
    return 0;
  else
    {
      mbstate_t state;
      wchar_t wc;
      size_t result;

      memset (&state, 0, sizeof (mbstate_t));
      result = mbrtowc (&wc, s, n, &state);
      if (result == (size_t)-1 || result == (size_t)-2)
        {
          errno = EILSEQ;
          return -1;
        }
      if (pwc != NULL)
        *pwc = wc;
      return (wc == 0 ? 0 : result);
    }
}
