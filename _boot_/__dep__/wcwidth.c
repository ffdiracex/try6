/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include <wchar.h>

/* Get iswprint.  */
#include <wctype.h>

#include "localcharset.h"
#include "streq.h"
#include "uniwidth.h"

int
wcwidth (wchar_t wc)
#undef wcwidth
{
  /* In UTF-8 locales, use a Unicode aware width function.  */
  const char *encoding = locale_charset ();
  if (STREQ_OPT (encoding, "UTF-8", 'U', 'T', 'F', '-', '8', 0, 0, 0 ,0))
    {
      /* We assume that in a UTF-8 locale, a wide character is the same as a
         Unicode character.  */
      return uc_width (wc, encoding);
    }
  else
    {
      /* Otherwise, fall back to the system's wcwidth function.  */
#if HAVE_WCWIDTH
      return wcwidth (wc);
#else
      return wc == 0 ? 0 : iswprint (wc) ? 1 : -1;
#endif
    }
}
