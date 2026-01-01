/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CHARSET_HEADER
#define holy_CHARSET_HEADER	1

#include <holy/types.h>

#define holy_UINT8_1_LEADINGBIT 0x80
#define holy_UINT8_2_LEADINGBITS 0xc0
#define holy_UINT8_3_LEADINGBITS 0xe0
#define holy_UINT8_4_LEADINGBITS 0xf0
#define holy_UINT8_5_LEADINGBITS 0xf8
#define holy_UINT8_6_LEADINGBITS 0xfc
#define holy_UINT8_7_LEADINGBITS 0xfe

#define holy_UINT8_1_TRAILINGBIT 0x01
#define holy_UINT8_2_TRAILINGBITS 0x03
#define holy_UINT8_3_TRAILINGBITS 0x07
#define holy_UINT8_4_TRAILINGBITS 0x0f
#define holy_UINT8_5_TRAILINGBITS 0x1f
#define holy_UINT8_6_TRAILINGBITS 0x3f

#define holy_MAX_UTF8_PER_UTF16 4
/* You need at least one UTF-8 byte to have one UTF-16 word.
   You need at least three UTF-8 bytes to have 2 UTF-16 words (surrogate pairs).
 */
#define holy_MAX_UTF16_PER_UTF8 1
#define holy_MAX_UTF8_PER_CODEPOINT 4

#define holy_UCS2_LIMIT 0x10000
#define holy_UTF16_UPPER_SURROGATE(code) \
  (0xD800 | ((((code) - holy_UCS2_LIMIT) >> 10) & 0x3ff))
#define holy_UTF16_LOWER_SURROGATE(code) \
  (0xDC00 | (((code) - holy_UCS2_LIMIT) & 0x3ff))

/* Process one character from UTF8 sequence. 
   At beginning set *code = 0, *count = 0. Returns 0 on failure and
   1 on success. *count holds the number of trailing bytes.  */
static inline int
holy_utf8_process (holy_uint8_t c, holy_uint32_t *code, int *count)
{
  if (*count)
    {
      if ((c & holy_UINT8_2_LEADINGBITS) != holy_UINT8_1_LEADINGBIT)
	{
	  *count = 0;
	  /* invalid */
	  return 0;
	}
      else
	{
	  *code <<= 6;
	  *code |= (c & holy_UINT8_6_TRAILINGBITS);
	  (*count)--;
	  /* Overlong.  */
	  if ((*count == 1 && *code <= 0x1f)
	      || (*count == 2 && *code <= 0xf))
	    {
	      *code = 0;
	      *count = 0;
	      return 0;
	    }
	  return 1;
	}
    }

  if ((c & holy_UINT8_1_LEADINGBIT) == 0)
    {
      *code = c;
      return 1;
    }
  if ((c & holy_UINT8_3_LEADINGBITS) == holy_UINT8_2_LEADINGBITS)
    {
      *count = 1;
      *code = c & holy_UINT8_5_TRAILINGBITS;
      /* Overlong */
      if (*code <= 1)
	{
	  *count = 0;
	  *code = 0;
	  return 0;
	}
      return 1;
    }
  if ((c & holy_UINT8_4_LEADINGBITS) == holy_UINT8_3_LEADINGBITS)
    {
      *count = 2;
      *code = c & holy_UINT8_4_TRAILINGBITS;
      return 1;
    }
  if ((c & holy_UINT8_5_LEADINGBITS) == holy_UINT8_4_LEADINGBITS)
    {
      *count = 3;
      *code = c & holy_UINT8_3_TRAILINGBITS;
      return 1;
    }
  return 0;
}


/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UTF-16 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters. If an invalid sequence is found, return -1.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
static inline holy_size_t
holy_utf8_to_utf16 (holy_uint16_t *dest, holy_size_t destsize,
		    const holy_uint8_t *src, holy_size_t srcsize,
		    const holy_uint8_t **srcend)
{
  holy_uint16_t *p = dest;
  int count = 0;
  holy_uint32_t code = 0;

  if (srcend)
    *srcend = src;

  while (srcsize && destsize)
    {
      int was_count = count;
      if (srcsize != (holy_size_t)-1)
	srcsize--;
      if (!holy_utf8_process (*src++, &code, &count))
	{
	  code = '?';
	  count = 0;
	  /* Character c may be valid, don't eat it.  */
	  if (was_count)
	    src--;
	}
      if (count != 0)
	continue;
      if (code == 0)
	break;
      if (destsize < 2 && code >= holy_UCS2_LIMIT)
	break;
      if (code >= holy_UCS2_LIMIT)
	{
	  *p++ = holy_UTF16_UPPER_SURROGATE (code);
	  *p++ = holy_UTF16_LOWER_SURROGATE (code);
	  destsize -= 2;
	}
      else
	{
	  *p++ = code;
	  destsize--;
	}
    }

  if (srcend)
    *srcend = src;
  return p - dest;
}

/* Determine the last position where the UTF-8 string [beg, end) can
   be safely cut. */
static inline holy_size_t
holy_getend (const char *beg, const char *end)
{
  const char *ptr;
  for (ptr = end - 1; ptr >= beg; ptr--)
    if ((*ptr & holy_UINT8_2_LEADINGBITS) != holy_UINT8_1_LEADINGBIT)
      break;
  if (ptr < beg)
    return 0;
  if ((*ptr & holy_UINT8_1_LEADINGBIT) == 0)
    return ptr + 1 - beg;
  if ((*ptr & holy_UINT8_3_LEADINGBITS) == holy_UINT8_2_LEADINGBITS
      && ptr + 2 <= end)
    return ptr + 2 - beg;
  if ((*ptr & holy_UINT8_4_LEADINGBITS) == holy_UINT8_3_LEADINGBITS
      && ptr + 3 <= end)
    return ptr + 3 - beg;
  if ((*ptr & holy_UINT8_5_LEADINGBITS) == holy_UINT8_4_LEADINGBITS
      && ptr + 4 <= end)
    return ptr + 4 - beg;
  /* Invalid character or incomplete. Cut before it.  */
  return ptr - beg;
}

/* Convert UTF-16 to UTF-8.  */
static inline holy_uint8_t *
holy_utf16_to_utf8 (holy_uint8_t *dest, const holy_uint16_t *src,
		    holy_size_t size)
{
  holy_uint32_t code_high = 0;

  while (size--)
    {
      holy_uint32_t code = *src++;

      if (code_high)
	{
	  if (code >= 0xDC00 && code <= 0xDFFF)
	    {
	      /* Surrogate pair.  */
	      code = ((code_high - 0xD800) << 10) + (code - 0xDC00) + 0x10000;

	      *dest++ = (code >> 18) | 0xF0;
	      *dest++ = ((code >> 12) & 0x3F) | 0x80;
	      *dest++ = ((code >> 6) & 0x3F) | 0x80;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	  else
	    {
	      /* Error...  */
	      *dest++ = '?';
	      /* *src may be valid. Don't eat it.  */
	      src--;
	    }

	  code_high = 0;
	}
      else
	{
	  if (code <= 0x007F)
	    *dest++ = code;
	  else if (code <= 0x07FF)
	    {
	      *dest++ = (code >> 6) | 0xC0;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	  else if (code >= 0xD800 && code <= 0xDBFF)
	    {
	      code_high = code;
	      continue;
	    }
	  else if (code >= 0xDC00 && code <= 0xDFFF)
	    {
	      /* Error... */
	      *dest++ = '?';
	    }
	  else if (code < 0x10000)
	    {
	      *dest++ = (code >> 12) | 0xE0;
	      *dest++ = ((code >> 6) & 0x3F) | 0x80;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	  else
	    {
	      *dest++ = (code >> 18) | 0xF0;
	      *dest++ = ((code >> 12) & 0x3F) | 0x80;
	      *dest++ = ((code >> 6) & 0x3F) | 0x80;
	      *dest++ = (code & 0x3F) | 0x80;
	    }
	}
    }

  return dest;
}

#define holy_MAX_UTF8_PER_LATIN1 2

/* Convert Latin1 to UTF-8.  */
static inline holy_uint8_t *
holy_latin1_to_utf8 (holy_uint8_t *dest, const holy_uint8_t *src,
		     holy_size_t size)
{
  while (size--)
    {
      if (!(*src & 0x80))
	*dest++ = *src;
      else
	{
	  *dest++ = (*src >> 6) | 0xC0;
	  *dest++ = (*src & 0x3F) | 0x80;
	}
      src++;
    }

  return dest;
}

/* Convert UCS-4 to UTF-8.  */
char *holy_ucs4_to_utf8_alloc (const holy_uint32_t *src, holy_size_t size);

int
holy_is_valid_utf8 (const holy_uint8_t *src, holy_size_t srcsize);

holy_ssize_t holy_utf8_to_ucs4_alloc (const char *msg,
				      holy_uint32_t **unicode_msg,
				      holy_uint32_t **last_position);

/* Returns the number of bytes the string src would occupy is converted
   to UTF-8, excluding \0.  */
holy_size_t
holy_get_num_of_utf8_bytes (const holy_uint32_t *src, holy_size_t size);

/* Converts UCS-4 to UTF-8. Returns the number of bytes effectively written
   excluding the trailing \0.  */
holy_size_t
holy_ucs4_to_utf8 (const holy_uint32_t *src, holy_size_t size,
		   holy_uint8_t *dest, holy_size_t destsize);
holy_size_t holy_utf8_to_ucs4 (holy_uint32_t *dest, holy_size_t destsize,
			       const holy_uint8_t *src, holy_size_t srcsize,
			       const holy_uint8_t **srcend);
/* Returns -2 if not enough space, -1 on invalid character.  */
holy_ssize_t
holy_encode_utf8_character (holy_uint8_t *dest, holy_uint8_t *destend,
			    holy_uint32_t code);

const holy_uint32_t *
holy_unicode_get_comb_start (const holy_uint32_t *str,
			     const holy_uint32_t *cur);

#endif
