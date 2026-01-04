/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/charset.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/unicode.h>
#include <holy/term.h>
#include <holy/normal.h>

#if HAVE_FONT_SOURCE
#include "widthspec.h"
#endif

/* Returns -2 if not enough space, -1 on invalid character.  */
holy_ssize_t
holy_encode_utf8_character (holy_uint8_t *dest, holy_uint8_t *destend,
			    holy_uint32_t code)
{
  if (dest >= destend)
    return -2;
  if (code <= 0x007F)
    {
      *dest++ = code;
      return 1;
    }
  if (code <= 0x07FF)
    {
      if (dest + 1 >= destend)
	return -2;
      *dest++ = (code >> 6) | 0xC0;
      *dest++ = (code & 0x3F) | 0x80;
      return 2;
    }
  if ((code >= 0xDC00 && code <= 0xDFFF)
      || (code >= 0xD800 && code <= 0xDBFF))
    {
      /* No surrogates in UCS-4... */
      return -1;
    }
  if (code < 0x10000)
    {
      if (dest + 2 >= destend)
	return -2;
      *dest++ = (code >> 12) | 0xE0;
      *dest++ = ((code >> 6) & 0x3F) | 0x80;
      *dest++ = (code & 0x3F) | 0x80;
      return 3;
    }
  {
    if (dest + 3 >= destend)
      return -2;
    *dest++ = (code >> 18) | 0xF0;
    *dest++ = ((code >> 12) & 0x3F) | 0x80;
    *dest++ = ((code >> 6) & 0x3F) | 0x80;
    *dest++ = (code & 0x3F) | 0x80;
    return 4;
  }

}

/* Convert UCS-4 to UTF-8.  */
holy_size_t
holy_ucs4_to_utf8 (const holy_uint32_t *src, holy_size_t size,
		   holy_uint8_t *dest, holy_size_t destsize)
{
  /* Keep last char for \0.  */
  holy_uint8_t *destend = dest + destsize - 1;
  holy_uint8_t *dest0 = dest;

  while (size-- && dest < destend)
    {
      holy_uint32_t code = *src++;
      holy_ssize_t s;
      s = holy_encode_utf8_character (dest, destend, code);
      if (s == -2)
	break;
      if (s == -1)
	{
	  *dest++ = '?';
	  continue;
	}
      dest += s;
    }
  *dest = 0;
  return dest - dest0;
}

/* Returns the number of bytes the string src would occupy is converted
   to UTF-8, excluding trailing \0.  */
holy_size_t
holy_get_num_of_utf8_bytes (const holy_uint32_t *src, holy_size_t size)
{
  holy_size_t remaining;
  const holy_uint32_t *ptr;
  holy_size_t cnt = 0;

  remaining = size;
  ptr = src;
  while (remaining--)
    {
      holy_uint32_t code = *ptr++;
      
      if (code <= 0x007F)
	cnt++;
      else if (code <= 0x07FF)
	cnt += 2;
      else if ((code >= 0xDC00 && code <= 0xDFFF)
	       || (code >= 0xD800 && code <= 0xDBFF))
	/* No surrogates in UCS-4... */
	cnt++;
      else if (code < 0x10000)
	cnt += 3;
      else
	cnt += 4;
    }
  return cnt;
}

/* Convert UCS-4 to UTF-8.  */
char *
holy_ucs4_to_utf8_alloc (const holy_uint32_t *src, holy_size_t size)
{
  holy_uint8_t *ret;
  holy_size_t cnt = holy_get_num_of_utf8_bytes (src, size) + 1;

  ret = holy_malloc (cnt);
  if (!ret)
    return 0;

  holy_ucs4_to_utf8 (src, size, ret, cnt);

  return (char *) ret;
}

int
holy_is_valid_utf8 (const holy_uint8_t *src, holy_size_t srcsize)
{
  int count = 0;
  holy_uint32_t code = 0;

  while (srcsize)
    {
      if (srcsize != (holy_size_t)-1)
	srcsize--;
      if (!holy_utf8_process (*src++, &code, &count))
	return 0;
      if (count != 0)
	continue;
      if (code == 0)
	return 1;
      if (code > holy_UNICODE_LAST_VALID)
	return 0;
    }

  return 1;
}

holy_ssize_t
holy_utf8_to_ucs4_alloc (const char *msg, holy_uint32_t **unicode_msg,
			 holy_uint32_t **last_position)
{
  holy_size_t msg_len = holy_strlen (msg);

  *unicode_msg = holy_malloc (msg_len * sizeof (holy_uint32_t));
 
  if (!*unicode_msg)
    return -1;

  msg_len = holy_utf8_to_ucs4 (*unicode_msg, msg_len,
  			      (holy_uint8_t *) msg, -1, 0);

  if (last_position)
    *last_position = *unicode_msg + msg_len;

  return msg_len;
}

/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UCS-4 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
holy_size_t
holy_utf8_to_ucs4 (holy_uint32_t *dest, holy_size_t destsize,
		   const holy_uint8_t *src, holy_size_t srcsize,
		   const holy_uint8_t **srcend)
{
  holy_uint32_t *p = dest;
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
      *p++ = code;
      destsize--;
    }

  if (srcend)
    *srcend = src;
  return p - dest;
}

static holy_uint8_t *join_types = NULL;

static void
unpack_join (void)
{
  unsigned i;
  struct holy_unicode_compact_range *cur;

  join_types = holy_zalloc (holy_UNICODE_MAX_CACHED_CHAR);
  if (!join_types)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }
  for (cur = holy_unicode_compact; cur->len; cur++)
    for (i = cur->start; i < cur->start + (unsigned) cur->len
	   && i < holy_UNICODE_MAX_CACHED_CHAR; i++)
      join_types[i] = cur->join_type;
}

static holy_uint8_t *bidi_types = NULL;

static void
unpack_bidi (void)
{
  unsigned i;
  struct holy_unicode_compact_range *cur;

  bidi_types = holy_zalloc (holy_UNICODE_MAX_CACHED_CHAR);
  if (!bidi_types)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }
  for (cur = holy_unicode_compact; cur->len; cur++)
    for (i = cur->start; i < cur->start + (unsigned) cur->len
	   && i < holy_UNICODE_MAX_CACHED_CHAR; i++)
      if (cur->bidi_mirror)
	bidi_types[i] = cur->bidi_type | 0x80;
      else
	bidi_types[i] = cur->bidi_type | 0x00;
}

static inline enum holy_bidi_type
get_bidi_type (holy_uint32_t c)
{
  struct holy_unicode_compact_range *cur;

  if (!bidi_types)
    unpack_bidi ();

  if (bidi_types && c < holy_UNICODE_MAX_CACHED_CHAR)
    return bidi_types[c] & 0x7f;

  for (cur = holy_unicode_compact; cur->len; cur++)
    if (cur->start <= c && c < cur->start + (unsigned) cur->len)
      return cur->bidi_type;

  return holy_BIDI_TYPE_L;
}

static inline enum holy_join_type
get_join_type (holy_uint32_t c)
{
  struct holy_unicode_compact_range *cur;

  if (!join_types)
    unpack_join ();

  if (join_types && c < holy_UNICODE_MAX_CACHED_CHAR)
    return join_types[c];

  for (cur = holy_unicode_compact; cur->len; cur++)
    if (cur->start <= c && c < cur->start + (unsigned) cur->len)
      return cur->join_type;

  return holy_JOIN_TYPE_NONJOINING;
}

static inline int
is_mirrored (holy_uint32_t c)
{
  struct holy_unicode_compact_range *cur;

  if (!bidi_types)
    unpack_bidi ();

  if (bidi_types && c < holy_UNICODE_MAX_CACHED_CHAR)
    return !!(bidi_types[c] & 0x80);

  for (cur = holy_unicode_compact; cur->len; cur++)
    if (cur->start <= c && c < cur->start + (unsigned) cur->len)
      return cur->bidi_mirror;

  return 0;
}

enum holy_comb_type
holy_unicode_get_comb_type (holy_uint32_t c)
{
  static holy_uint8_t *comb_types = NULL;
  struct holy_unicode_compact_range *cur;

  if (!comb_types)
    {
      unsigned i;
      comb_types = holy_zalloc (holy_UNICODE_MAX_CACHED_CHAR);
      if (comb_types)
	for (cur = holy_unicode_compact; cur->len; cur++)
	  for (i = cur->start; i < cur->start + (unsigned) cur->len
		 && i < holy_UNICODE_MAX_CACHED_CHAR; i++)
	    comb_types[i] = cur->comb_type;
      else
	holy_errno = holy_ERR_NONE;
    }

  if (comb_types && c < holy_UNICODE_MAX_CACHED_CHAR)
    return comb_types[c];

  for (cur = holy_unicode_compact; cur->len; cur++)
    if (cur->start <= c && c < cur->start + (unsigned) cur->len)
      return cur->comb_type;

  return holy_UNICODE_COMB_NONE;
}

#if HAVE_FONT_SOURCE

holy_size_t
holy_unicode_estimate_width (const struct holy_unicode_glyph *c)
{
  if (holy_unicode_get_comb_type (c->base))
    return 0;
  if (widthspec[c->base >> 3] & (1 << (c->base & 7)))
    return 2;
  else
    return 1;
}

#endif

static inline int
is_type_after (enum holy_comb_type a, enum holy_comb_type b)
{
  /* Shadda is numerically higher than most of Arabic diacritics but has
     to be rendered before them.  */
  if (a == holy_UNICODE_COMB_ARABIC_SHADDA
      && b <= holy_UNICODE_COMB_ARABIC_KASRA
      && b >= holy_UNICODE_COMB_ARABIC_FATHATAN)
    return 0;
  if (b == holy_UNICODE_COMB_ARABIC_SHADDA
      && a <= holy_UNICODE_COMB_ARABIC_KASRA
      && a >= holy_UNICODE_COMB_ARABIC_FATHATAN)
    return 1;
  return a > b;
}

holy_size_t
holy_unicode_aglomerate_comb (const holy_uint32_t *in, holy_size_t inlen,
			      struct holy_unicode_glyph *out)
{
  int haveout = 0;
  const holy_uint32_t *ptr;
  unsigned last_comb_pointer = 0;

  holy_memset (out, 0, sizeof (*out));

  if (inlen && holy_iscntrl (*in))
    {
      out->base = *in;
      out->variant = 0;
      out->attributes = 0;
      out->ncomb = 0;
      out->estimated_width = 1;
      return 1;
    }

  for (ptr = in; ptr < in + inlen; ptr++)
    {
      /* Variation selectors >= 17 are outside of BMP and SMP. 
	 Handle variation selectors first to avoid potentially costly lookups.
      */
      if (*ptr >= holy_UNICODE_VARIATION_SELECTOR_1
	  && *ptr <= holy_UNICODE_VARIATION_SELECTOR_16)
	{
	  if (haveout)
	    out->variant = *ptr - holy_UNICODE_VARIATION_SELECTOR_1 + 1;
	  continue;
	}
      if (*ptr >= holy_UNICODE_VARIATION_SELECTOR_17
	  && *ptr <= holy_UNICODE_VARIATION_SELECTOR_256)
	{
	  if (haveout)
	    out->variant = *ptr - holy_UNICODE_VARIATION_SELECTOR_17 + 17;
	  continue;
	}
	
      enum holy_comb_type comb_type;
      comb_type = holy_unicode_get_comb_type (*ptr);
      if (comb_type)
	{
	  struct holy_unicode_combining *n;
	  unsigned j;

	  if (!haveout)
	    continue;

	  if (comb_type == holy_UNICODE_COMB_MC
	      || comb_type == holy_UNICODE_COMB_ME
	      || comb_type == holy_UNICODE_COMB_MN)
	    last_comb_pointer = out->ncomb;

	  if (out->ncomb + 1 <= (int) ARRAY_SIZE (out->combining_inline))
	    n = out->combining_inline;
	  else if (out->ncomb > (int) ARRAY_SIZE (out->combining_inline))
	    {
	      n = holy_realloc (out->combining_ptr,
				sizeof (n[0]) * (out->ncomb + 1));
	      if (!n)
		{
		  holy_errno = holy_ERR_NONE;
		  continue;
		}
	      out->combining_ptr = n;
	    }
	  else
	    {
	      n = holy_malloc (sizeof (n[0]) * (out->ncomb + 1));
	      if (!n)
		{
		  holy_errno = holy_ERR_NONE;
		  continue;
		}
	      holy_memcpy (n, out->combining_inline,
			   sizeof (out->combining_inline));
	      out->combining_ptr = n;
	    }

	  for (j = last_comb_pointer; j < out->ncomb; j++)
	    if (is_type_after (n[j].type, comb_type))
	      break;
	  holy_memmove (n + j + 1,
			n + j,
			(out->ncomb - j)
			* sizeof (n[0]));
	  n[j].code = *ptr;
	  n[j].type = comb_type;
	  out->ncomb++;
	  continue;
	}
      if (haveout)
	return ptr - in;
      haveout = 1;
      out->base = *ptr;
      out->variant = 0;
      out->attributes = 0;
      out->ncomb = 0;
      out->estimated_width = 1;
    }
  return ptr - in;
}

static void
revert (struct holy_unicode_glyph *visual,
	struct holy_term_pos *pos,
	unsigned start, unsigned end)
{
  struct holy_unicode_glyph t;
  unsigned i;
  int a = 0, b = 0;
  if (pos)
    {
      a = pos[visual[start].orig_pos].x;
      b = pos[visual[end].orig_pos].x;
    }
  for (i = 0; i < (end - start) / 2 + 1; i++)
    {
      t = visual[start + i];
      visual[start + i] = visual[end - i];
      visual[end - i] = t;

      if (pos)
	{
	  pos[visual[start + i].orig_pos].x = a + b - pos[visual[start + i].orig_pos].x;
	  pos[visual[end - i].orig_pos].x = a + b - pos[visual[end - i].orig_pos].x;
	}
    }
}


static holy_ssize_t
bidi_line_wrap (struct holy_unicode_glyph *visual_out,
		struct holy_unicode_glyph *visual,
		holy_size_t visual_len,
		holy_size_t (*getcharwidth) (const struct holy_unicode_glyph *visual, void *getcharwidth_arg),
		void *getcharwidth_arg,
		holy_size_t maxwidth, holy_size_t startwidth,
		holy_uint32_t contchar,
		struct holy_term_pos *pos, int primitive_wrap,
		holy_size_t log_end)
{
  struct holy_unicode_glyph *outptr = visual_out;
  unsigned line_start = 0;
  holy_ssize_t line_width;
  unsigned k;
  holy_ssize_t last_space = -1;
  holy_ssize_t last_space_width = 0;
  int lines = 0;

  if (!visual_len)
    return 0;

  if (startwidth >= maxwidth && (holy_ssize_t) maxwidth > 0)
    {
      if (contchar)
	{
	  holy_memset (outptr, 0, sizeof (visual[0]));
	  outptr->base = contchar;
	  outptr++;
	}
      holy_memset (outptr, 0, sizeof (visual[0]));
      outptr->base = '\n';
      outptr++;
      startwidth = 0;
    }

  line_width = startwidth;

  for (k = 0; k <= visual_len; k++)
    {
      holy_ssize_t last_width = 0;
 
      if (pos && k != visual_len)
	{
	  pos[visual[k].orig_pos].x = line_width;
	  pos[visual[k].orig_pos].y = lines;
	  pos[visual[k].orig_pos].valid = 1;
	}

      if (k == visual_len && pos)
	{
	  pos[log_end].x = line_width;
	  pos[log_end].y = lines;
	  pos[log_end].valid = 1;
	}

      if (getcharwidth && k != visual_len)
	line_width += last_width = getcharwidth (&visual[k], getcharwidth_arg);

      if (k != visual_len && (visual[k].base == ' '
			      || visual[k].base == '\t')
	  && !primitive_wrap)
	{
	  last_space = k;
	  last_space_width = line_width;
	}

      if (((holy_ssize_t) maxwidth > 0
	   && line_width > (holy_ssize_t) maxwidth) || k == visual_len)
	{	  
	  unsigned min_odd_level = 0xffffffff;
	  unsigned max_level = 0;
	  unsigned kk = k;

	  lines++;

	  if (k != visual_len && last_space > (signed) line_start)
	    {
	      kk = last_space;
	      line_width -= last_space_width;
	    }
	  else if (k != visual_len && line_start == 0 && startwidth != 0
		   && !primitive_wrap && lines == 1
		   && line_width - startwidth < maxwidth)
	    {
	      kk = 0;
	      line_width -= startwidth;
	    }
	  else
	    line_width = last_width;

	  {
	    unsigned i;
	    for (i = line_start; i < kk; i++)
	      {
		if (visual[i].bidi_level > max_level)
		  max_level = visual[i].bidi_level;
		if (visual[i].bidi_level < min_odd_level && (visual[i].bidi_level & 1))
		  min_odd_level = visual[i].bidi_level;
	      }
	  }

	  {
	    unsigned j;	  
	    /* FIXME: can be optimized.  */
	    for (j = max_level; j > min_odd_level - 1; j--)
	      {
		unsigned in = line_start;
		unsigned i;
		for (i = line_start; i < kk; i++)
		  {
		    if (i != line_start && visual[i].bidi_level >= j
			&& visual[i-1].bidi_level < j)
		      in = i;
		    if (visual[i].bidi_level >= j && (i + 1 == kk
						 || visual[i+1].bidi_level < j))
		      revert (visual, pos, in, i);
		  }
	      }
	  }
	  
	  {
	    unsigned i;
	    for (i = line_start; i < kk; i++)
	      {
		if (is_mirrored (visual[i].base) && visual[i].bidi_level)
		  visual[i].attributes |= holy_UNICODE_GLYPH_ATTRIBUTE_MIRROR;
		if ((visual[i].attributes & holy_UNICODE_GLYPH_ATTRIBUTES_JOIN)
		    && visual[i].bidi_level)
		  {
		    int left, right;
		    left = visual[i].attributes
		      & (holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED
			 | holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT);
		    right = visual[i].attributes
		      & (holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED
			 | holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT);
		    visual[i].attributes &= ~holy_UNICODE_GLYPH_ATTRIBUTES_JOIN;
		    left <<= holy_UNICODE_GLYPH_ATTRIBUTES_JOIN_LEFT_TO_RIGHT_SHIFT;
		    right >>= holy_UNICODE_GLYPH_ATTRIBUTES_JOIN_LEFT_TO_RIGHT_SHIFT;
		    visual[i].attributes |= (left | right);
		  }
	      }
	  }

	  {
	    int left_join = 0;
	    unsigned i;
	    for (i = line_start; i < kk; i++)
	      {
		enum holy_join_type join_type = get_join_type (visual[i].base);
		if (!(visual[i].attributes
		      & holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT)
		    && (join_type == holy_JOIN_TYPE_LEFT
			|| join_type == holy_JOIN_TYPE_DUAL))
		  {
		    if (left_join)
		      visual[i].attributes
			|= holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED;
		    else
		      visual[i].attributes
			&= ~holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED;
		  }
		if (join_type == holy_JOIN_TYPE_NONJOINING
		    || join_type == holy_JOIN_TYPE_LEFT)
		  left_join = 0;
		if (join_type == holy_JOIN_TYPE_RIGHT
		    || join_type == holy_JOIN_TYPE_DUAL
		    || join_type == holy_JOIN_TYPE_CAUSING)
		  left_join = 1;
	      }
	  }

	  {
	    int right_join = 0;
	    signed i;
	    for (i = kk - 1; i >= 0 && (unsigned) i + 1 > line_start;
		 i--)
	      {
		enum holy_join_type join_type = get_join_type (visual[i].base);
		if (!(visual[i].attributes
		      & holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT)
		    && (join_type == holy_JOIN_TYPE_RIGHT
			|| join_type == holy_JOIN_TYPE_DUAL))
		  {
		    if (right_join)
		      visual[i].attributes
			|= holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED;
		    else
		      visual[i].attributes
			&= ~holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED;
		  }
		if (join_type == holy_JOIN_TYPE_NONJOINING
		    || join_type == holy_JOIN_TYPE_RIGHT)
		  right_join = 0;
		if (join_type == holy_JOIN_TYPE_LEFT
		    || join_type == holy_JOIN_TYPE_DUAL
		    || join_type == holy_JOIN_TYPE_CAUSING)
		  right_join = 1;
	      }
	  }		

	  holy_memcpy (outptr, &visual[line_start],
		       (kk - line_start) * sizeof (visual[0]));
	  outptr += kk - line_start;
	  if (kk != visual_len)
	    {
	      if (contchar)
		{
		  holy_memset (outptr, 0, sizeof (visual[0]));
		  outptr->base = contchar;
		  outptr++;
		}
	      holy_memset (outptr, 0, sizeof (visual[0]));
	      outptr->base = '\n';
	      outptr++;
	    }

	  if ((signed) kk == last_space)
	    kk++;

	  line_start = kk;
	  if (pos && kk != visual_len)
	    {
	      pos[visual[kk].orig_pos].x = 0;
	      pos[visual[kk].orig_pos].y = lines;
	    }
	}
    }

  return outptr - visual_out;
}


static holy_ssize_t
holy_bidi_line_logical_to_visual (const holy_uint32_t *logical,
				  holy_size_t logical_len,
				  struct holy_unicode_glyph *visual_out,
				  holy_size_t (*getcharwidth) (const struct holy_unicode_glyph *visual, void *getcharwidth_arg),
				  void *getcharwidth_arg,
				  holy_size_t maxwidth, holy_size_t startwidth,
				  holy_uint32_t contchar,
				  struct holy_term_pos *pos,
				  int primitive_wrap,
				  holy_size_t log_end)
{
  enum holy_bidi_type type = holy_BIDI_TYPE_L;
  enum override_status {OVERRIDE_NEUTRAL = 0, OVERRIDE_R, OVERRIDE_L};
  unsigned base_level;
  enum override_status cur_override;
  unsigned i;
  unsigned stack_level[holy_BIDI_MAX_EXPLICIT_LEVEL + 3];
  enum override_status stack_override[holy_BIDI_MAX_EXPLICIT_LEVEL + 3];
  unsigned stack_depth = 0;
  unsigned invalid_pushes = 0;
  unsigned visual_len = 0;
  unsigned run_start, run_end;
  struct holy_unicode_glyph *visual;
  unsigned cur_level;
  int bidi_needed = 0;

#define push_stack(new_override, new_level)		\
  {							\
    if (new_level > holy_BIDI_MAX_EXPLICIT_LEVEL)	\
      {							\
      invalid_pushes++;					\
      }							\
    else						\
      {							\
      stack_level[stack_depth] = cur_level;		\
      stack_override[stack_depth] = cur_override;	\
      stack_depth++;					\
      cur_level = new_level;				\
      cur_override = new_override;			\
      }							\
  }

#define pop_stack()				\
  {						\
    if (invalid_pushes)				\
      {						\
      invalid_pushes--;				\
      }						\
    else if (stack_depth)			\
      {						\
      stack_depth--;				\
      cur_level = stack_level[stack_depth];	\
      cur_override = stack_override[stack_depth];	\
      }							\
  }

  visual = holy_malloc (sizeof (visual[0]) * logical_len);
  if (!visual)
    return -1;

  for (i = 0; i < logical_len; i++)
    {
      type = get_bidi_type (logical[i]);
      if (type == holy_BIDI_TYPE_L || type == holy_BIDI_TYPE_AL
	  || type == holy_BIDI_TYPE_R)
	break;
    }
  if (type == holy_BIDI_TYPE_R || type == holy_BIDI_TYPE_AL)
    base_level = 1;
  else
    base_level = 0;
  
  cur_level = base_level;
  cur_override = OVERRIDE_NEUTRAL;
  {
    const holy_uint32_t *lptr;
    enum {JOIN_DEFAULT, NOJOIN, JOIN_FORCE} join_state = JOIN_DEFAULT;
    int zwj_propagate_to_previous = 0;
    for (lptr = logical; lptr < logical + logical_len;)
      {
	holy_size_t p;

	if (*lptr == holy_UNICODE_ZWJ)
	  {
	    if (zwj_propagate_to_previous)
	      {
		visual[visual_len - 1].attributes
		  |= holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT
		  | holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED;
	      }
	    zwj_propagate_to_previous = 0;
	    join_state = JOIN_FORCE;
	    lptr++;
	    continue;
	  }

	if (*lptr == holy_UNICODE_ZWNJ)
	  {
	    if (zwj_propagate_to_previous)
	      {
		visual[visual_len - 1].attributes
		  |= holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT;
		visual[visual_len - 1].attributes 
		  &= ~holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED;
	      }
	    zwj_propagate_to_previous = 0;
	    join_state = NOJOIN;
	    lptr++;
	    continue;
	  }

	/* The tags: deprecated, never used.  */
	if (*lptr >= holy_UNICODE_TAG_START && *lptr <= holy_UNICODE_TAG_END)
	  continue;

	p = holy_unicode_aglomerate_comb (lptr, logical + logical_len - lptr,
					  &visual[visual_len]);
	visual[visual_len].orig_pos = lptr - logical;
	type = get_bidi_type (visual[visual_len].base);
	switch (type)
	  {
	  case holy_BIDI_TYPE_RLE:
	    bidi_needed = 1;
	    push_stack (cur_override, (cur_level | 1) + 1);
	    break;
	  case holy_BIDI_TYPE_RLO:
	    bidi_needed = 1;
	    push_stack (OVERRIDE_R, (cur_level | 1) + 1);
	    break;
	  case holy_BIDI_TYPE_LRE:
	    push_stack (cur_override, (cur_level & ~1) + 2);
	    break;
	  case holy_BIDI_TYPE_LRO:
	    push_stack (OVERRIDE_L, (cur_level & ~1) + 2);
	    break;
	  case holy_BIDI_TYPE_PDF:
	    pop_stack ();
	    break;
	  case holy_BIDI_TYPE_BN:
	    break;
	  case holy_BIDI_TYPE_R:
	  case holy_BIDI_TYPE_AL:
	    bidi_needed = 1;
	    /* Fallthrough.  */
	  default:
	    {
	      if (join_state == JOIN_FORCE)
		{
		  visual[visual_len].attributes
		    |= holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT
		    | holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED;
		}
	      
	      if (join_state == NOJOIN)
		{
		  visual[visual_len].attributes
		    |= holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT;
		  visual[visual_len].attributes
		    &= ~holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED;
		}

	      join_state = JOIN_DEFAULT;
	      zwj_propagate_to_previous = 1;

	      visual[visual_len].bidi_level = cur_level;
	      if (cur_override != OVERRIDE_NEUTRAL)
		visual[visual_len].bidi_type = 
		  (cur_override == OVERRIDE_L) ? holy_BIDI_TYPE_L
		  : holy_BIDI_TYPE_R;
	      else
		visual[visual_len].bidi_type = type;
	      visual_len++;
	    }
	  }
	lptr += p;
      }
  }

  if (bidi_needed)
    {
      for (run_start = 0; run_start < visual_len; run_start = run_end)
	{
	  unsigned prev_level, next_level, cur_run_level;
	  unsigned last_type, last_strong_type;
	  for (run_end = run_start; run_end < visual_len &&
		 visual[run_end].bidi_level == visual[run_start].bidi_level; run_end++);
	  if (run_start == 0)
	    prev_level = base_level;
	  else
	    prev_level = visual[run_start - 1].bidi_level;
	  if (run_end == visual_len)
	    next_level = base_level;
	  else
	    next_level = visual[run_end].bidi_level;
	  cur_run_level = visual[run_start].bidi_level;
	  if (prev_level & 1)
	    last_type = holy_BIDI_TYPE_R;
	  else
	    last_type = holy_BIDI_TYPE_L;
	  last_strong_type = last_type;
	  for (i = run_start; i < run_end; i++)
	    {
	      switch (visual[i].bidi_type)
		{
		case holy_BIDI_TYPE_NSM:
		  visual[i].bidi_type = last_type;
		  break;
		case holy_BIDI_TYPE_EN:
		  if (last_strong_type == holy_BIDI_TYPE_AL)
		    visual[i].bidi_type = holy_BIDI_TYPE_AN;
		  break;
		case holy_BIDI_TYPE_L:
		case holy_BIDI_TYPE_R:
		  last_strong_type = visual[i].bidi_type;
		  break;
		case holy_BIDI_TYPE_ES:
		  if (last_type == holy_BIDI_TYPE_EN
		      && i + 1 < run_end 
		      && visual[i + 1].bidi_type == holy_BIDI_TYPE_EN)
		    visual[i].bidi_type = holy_BIDI_TYPE_EN;
		  else
		    visual[i].bidi_type = holy_BIDI_TYPE_ON;
		  break;
		case holy_BIDI_TYPE_ET:
		  {
		    unsigned j;
		    if (last_type == holy_BIDI_TYPE_EN)
		      {
			visual[i].bidi_type = holy_BIDI_TYPE_EN;
			break;
		      }
		    for (j = i; j < run_end
			   && visual[j].bidi_type == holy_BIDI_TYPE_ET; j++);
		    if (j != run_end && visual[j].bidi_type == holy_BIDI_TYPE_EN)
		      {
			for (; i < run_end
			       && visual[i].bidi_type == holy_BIDI_TYPE_ET; i++)
			  visual[i].bidi_type = holy_BIDI_TYPE_EN;
			i--;
			break;
		      }
		    for (; i < run_end
			   && visual[i].bidi_type == holy_BIDI_TYPE_ET; i++)
		      visual[i].bidi_type = holy_BIDI_TYPE_ON;
		    i--;
		    break;		
		  }
		  break;
		case holy_BIDI_TYPE_CS:
		  if (last_type == holy_BIDI_TYPE_EN
		      && i + 1 < run_end 
		      && visual[i + 1].bidi_type == holy_BIDI_TYPE_EN)
		    {
		      visual[i].bidi_type = holy_BIDI_TYPE_EN;
		      break;
		    }
		  if (last_type == holy_BIDI_TYPE_AN
		      && i + 1 < run_end 
		      && (visual[i + 1].bidi_type == holy_BIDI_TYPE_AN
			  || (visual[i + 1].bidi_type == holy_BIDI_TYPE_EN
			      && last_strong_type == holy_BIDI_TYPE_AL)))
		    {
		      visual[i].bidi_type = holy_BIDI_TYPE_EN;
		      break;
		    }
		  visual[i].bidi_type = holy_BIDI_TYPE_ON;
		  break;
		case holy_BIDI_TYPE_AL:
		  last_strong_type = visual[i].bidi_type;
		  visual[i].bidi_type = holy_BIDI_TYPE_R;
		  break;
		default: /* Make GCC happy.  */
		  break;
		}
	      last_type = visual[i].bidi_type;
	      if (visual[i].bidi_type == holy_BIDI_TYPE_EN
		  && last_strong_type == holy_BIDI_TYPE_L)
		visual[i].bidi_type = holy_BIDI_TYPE_L;
	    }
	  if (prev_level & 1)
	    last_type = holy_BIDI_TYPE_R;
	  else
	    last_type = holy_BIDI_TYPE_L;
	  for (i = run_start; i < run_end; )
	    {
	      unsigned j;
	      unsigned next_type;
	      for (j = i; j < run_end &&
		     (visual[j].bidi_type == holy_BIDI_TYPE_B
		      || visual[j].bidi_type == holy_BIDI_TYPE_S
		      || visual[j].bidi_type == holy_BIDI_TYPE_WS
		      || visual[j].bidi_type == holy_BIDI_TYPE_ON); j++);
	      if (j == i)
		{
		  if (visual[i].bidi_type == holy_BIDI_TYPE_L)
		    last_type = holy_BIDI_TYPE_L;
		  else
		    last_type = holy_BIDI_TYPE_R;
		  i++;
		  continue;
		}
	      if (j == run_end)
		next_type = (next_level & 1) ? holy_BIDI_TYPE_R : holy_BIDI_TYPE_L;
	      else
		{
		  if (visual[j].bidi_type == holy_BIDI_TYPE_L)
		    next_type = holy_BIDI_TYPE_L;
		  else
		    next_type = holy_BIDI_TYPE_R;
		}
	      if (next_type == last_type)
		for (; i < j; i++)
		  visual[i].bidi_type = last_type;
	      else
		for (; i < j; i++)
		  visual[i].bidi_type = (cur_run_level & 1) ? holy_BIDI_TYPE_R
		    : holy_BIDI_TYPE_L;
	    }
	}

      for (i = 0; i < visual_len; i++)
	{
	  if (!(visual[i].bidi_level & 1) && visual[i].bidi_type == holy_BIDI_TYPE_R)
	    {
	      visual[i].bidi_level++;
	      continue;
	    }
	  if (!(visual[i].bidi_level & 1) && (visual[i].bidi_type == holy_BIDI_TYPE_AN
				   || visual[i].bidi_type == holy_BIDI_TYPE_EN))
	    {
	      visual[i].bidi_level += 2;
	      continue;
	    }
	  if ((visual[i].bidi_level & 1) && (visual[i].bidi_type == holy_BIDI_TYPE_L
				  || visual[i].bidi_type == holy_BIDI_TYPE_AN
				  || visual[i].bidi_type == holy_BIDI_TYPE_EN))
	    {
	      visual[i].bidi_level++;
	      continue;
	    }
	}
    }
  else
    {
      for (i = 0; i < visual_len; i++)
	visual[i].bidi_level = 0;
    }

  {
    holy_ssize_t ret;
    ret = bidi_line_wrap (visual_out, visual, visual_len,
			  getcharwidth, getcharwidth_arg, maxwidth, startwidth, contchar,
			  pos, primitive_wrap, log_end);
    holy_free (visual);
    return ret;
  }
}

static int
is_visible (const struct holy_unicode_glyph *gl)
{
  if (gl->ncomb)
    return 1;
  if (gl->base == holy_UNICODE_LRM || gl->base == holy_UNICODE_RLM)
    return 0;
  return 1;
}

holy_ssize_t
holy_bidi_logical_to_visual (const holy_uint32_t *logical,
			     holy_size_t logical_len,
			     struct holy_unicode_glyph **visual_out,
			     holy_size_t (*getcharwidth) (const struct holy_unicode_glyph *visual, void *getcharwidth_arg),
			     void *getcharwidth_arg,
			     holy_size_t max_length, holy_size_t startwidth,
			     holy_uint32_t contchar, struct holy_term_pos *pos, int primitive_wrap)
{
  const holy_uint32_t *line_start = logical, *ptr;
  struct holy_unicode_glyph *visual_ptr;
  *visual_out = visual_ptr = holy_malloc (3 * sizeof (visual_ptr[0])
					  * (logical_len + 2));
  if (!visual_ptr)
    return -1;
  for (ptr = logical; ptr <= logical + logical_len; ptr++)
    {
      if (ptr == logical + logical_len || *ptr == '\n')
	{
	  holy_ssize_t ret;
	  holy_ssize_t i, j;
	  ret = holy_bidi_line_logical_to_visual (line_start,
						  ptr - line_start,
						  visual_ptr,
						  getcharwidth,
						  getcharwidth_arg,
						  max_length,
						  startwidth,
						  contchar,
						  pos,
						  primitive_wrap,
						  logical_len);
	  startwidth = 0;

	  if (ret < 0)
	    {
	      holy_free (*visual_out);
	      return ret;
	    }
	  for (i = 0, j = 0; i < ret; i++)
	    if (is_visible(&visual_ptr[i]))
	      visual_ptr[j++] = visual_ptr[i];
	  visual_ptr += j;
	  line_start = ptr;
	  if (ptr != logical + logical_len)
	    {
	      holy_memset (visual_ptr, 0, sizeof (visual_ptr[0]));
	      visual_ptr->base = '\n';
	      visual_ptr++;
	      line_start++;
	    }
	}
    }
  return visual_ptr - *visual_out;
}

holy_uint32_t
holy_unicode_mirror_code (holy_uint32_t in)
{
  int i;
  for (i = 0; holy_unicode_bidi_pairs[i].key; i++)
    if (holy_unicode_bidi_pairs[i].key == in)
      return holy_unicode_bidi_pairs[i].replace;
  return in;
}

holy_uint32_t
holy_unicode_shape_code (holy_uint32_t in, holy_uint8_t attr)
{
  int i;
  if (!(in >= holy_UNICODE_ARABIC_START
	&& in < holy_UNICODE_ARABIC_END))
    return in;

  for (i = 0; holy_unicode_arabic_shapes[i].code; i++)
    if (holy_unicode_arabic_shapes[i].code == in)
      {
	holy_uint32_t out = 0;
	switch (attr & (holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED
			| holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED))
	  {
	  case 0:
	    out = holy_unicode_arabic_shapes[i].isolated;
	    break;
	  case holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED:
	    out = holy_unicode_arabic_shapes[i].right_linked;
	    break;
	  case holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED:
	    out = holy_unicode_arabic_shapes[i].left_linked;
	    break;
	  case holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED
	    |holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED:
	    out = holy_unicode_arabic_shapes[i].both_linked;
	    break;
	  }
	if (out)
	  return out;
      }

  return in;
}

const holy_uint32_t *
holy_unicode_get_comb_start (const holy_uint32_t *str,
			     const holy_uint32_t *cur)
{
  const holy_uint32_t *ptr;
  for (ptr = cur; ptr >= str; ptr--)
    {
      if (*ptr >= holy_UNICODE_VARIATION_SELECTOR_1
	  && *ptr <= holy_UNICODE_VARIATION_SELECTOR_16)
	continue;

      if (*ptr >= holy_UNICODE_VARIATION_SELECTOR_17
	  && *ptr <= holy_UNICODE_VARIATION_SELECTOR_256)
	continue;
	
      enum holy_comb_type comb_type;
      comb_type = holy_unicode_get_comb_type (*ptr);
      if (comb_type)
	continue;
      return ptr;
    }
  return str;
}

const holy_uint32_t *
holy_unicode_get_comb_end (const holy_uint32_t *end,
			   const holy_uint32_t *cur)
{
  const holy_uint32_t *ptr;
  for (ptr = cur; ptr < end; ptr++)
    {
      if (*ptr >= holy_UNICODE_VARIATION_SELECTOR_1
	  && *ptr <= holy_UNICODE_VARIATION_SELECTOR_16)
	continue;

      if (*ptr >= holy_UNICODE_VARIATION_SELECTOR_17
	  && *ptr <= holy_UNICODE_VARIATION_SELECTOR_256)
	continue;
	
      enum holy_comb_type comb_type;
      comb_type = holy_unicode_get_comb_type (*ptr);
      if (comb_type)
	continue;
      return ptr;
    }
  return end;
}

