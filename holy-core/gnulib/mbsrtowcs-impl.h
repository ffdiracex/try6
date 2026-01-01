/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

size_t
mbsrtowcs (wchar_t *dest, const char **srcp, size_t len, mbstate_t *ps)
{
  if (ps == NULL)
    ps = &_gl_mbsrtowcs_state;
  {
    const char *src = *srcp;

    if (dest != NULL)
      {
        wchar_t *destptr = dest;

        for (; len > 0; destptr++, len--)
          {
            size_t src_avail;
            size_t ret;

            /* An optimized variant of
               src_avail = strnlen1 (src, MB_LEN_MAX);  */
            if (src[0] == '\0')
              src_avail = 1;
            else if (src[1] == '\0')
              src_avail = 2;
            else if (src[2] == '\0')
              src_avail = 3;
            else if (MB_LEN_MAX <= 4 || src[3] == '\0')
              src_avail = 4;
            else
              src_avail = 4 + strnlen1 (src + 4, MB_LEN_MAX - 4);

            /* Parse the next multibyte character.  */
            ret = mbrtowc (destptr, src, src_avail, ps);

            if (ret == (size_t)(-2))
              /* Encountered a multibyte character that extends past a '\0' byte
                 or that is longer than MB_LEN_MAX bytes.  Cannot happen.  */
              abort ();

            if (ret == (size_t)(-1))
              goto bad_input;
            if (ret == 0)
              {
                src = NULL;
                /* Here mbsinit (ps).  */
                break;
              }
            src += ret;
          }

        *srcp = src;
        return destptr - dest;
      }
    else
      {
        /* Ignore dest and len, don't store *srcp at the end, and
           don't clobber *ps.  */
        mbstate_t state = *ps;
        size_t totalcount = 0;

        for (;; totalcount++)
          {
            size_t src_avail;
            size_t ret;

            /* An optimized variant of
               src_avail = strnlen1 (src, MB_LEN_MAX);  */
            if (src[0] == '\0')
              src_avail = 1;
            else if (src[1] == '\0')
              src_avail = 2;
            else if (src[2] == '\0')
              src_avail = 3;
            else if (MB_LEN_MAX <= 4 || src[3] == '\0')
              src_avail = 4;
            else
              src_avail = 4 + strnlen1 (src + 4, MB_LEN_MAX - 4);

            /* Parse the next multibyte character.  */
            ret = mbrtowc (NULL, src, src_avail, &state);

            if (ret == (size_t)(-2))
              /* Encountered a multibyte character that extends past a '\0' byte
                 or that is longer than MB_LEN_MAX bytes.  Cannot happen.  */
              abort ();

            if (ret == (size_t)(-1))
              goto bad_input2;
            if (ret == 0)
              {
                /* Here mbsinit (&state).  */
                break;
              }
            src += ret;
          }

        return totalcount;
      }

   bad_input:
    *srcp = src;
   bad_input2:
    errno = EILSEQ;
    return (size_t)(-1);
  }
}
