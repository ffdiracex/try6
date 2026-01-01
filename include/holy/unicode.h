/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BIDI_HEADER
#define holy_BIDI_HEADER	1

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>

struct holy_unicode_bidi_pair
{
  holy_uint32_t key;
  holy_uint32_t replace;
};

struct holy_unicode_compact_range
{
  unsigned start:21;
  unsigned len:9;
  unsigned bidi_type:5;
  unsigned comb_type:8;
  unsigned bidi_mirror:1;
  unsigned join_type:3;
} holy_PACKED;

/* Old-style Arabic shaping. Used for "visual UTF-8" and
   in holy-mkfont to find variant glyphs in absence of GPOS tables.  */
struct holy_unicode_arabic_shape
{
  holy_uint32_t code;
  holy_uint32_t isolated;
  holy_uint32_t right_linked;
  holy_uint32_t both_linked;
  holy_uint32_t left_linked;
};

extern struct holy_unicode_arabic_shape holy_unicode_arabic_shapes[];

enum holy_bidi_type
  {
    holy_BIDI_TYPE_L = 0,
    holy_BIDI_TYPE_LRE,
    holy_BIDI_TYPE_LRO,
    holy_BIDI_TYPE_R,
    holy_BIDI_TYPE_AL,
    holy_BIDI_TYPE_RLE,
    holy_BIDI_TYPE_RLO,
    holy_BIDI_TYPE_PDF,
    holy_BIDI_TYPE_EN,
    holy_BIDI_TYPE_ES,
    holy_BIDI_TYPE_ET,
    holy_BIDI_TYPE_AN,
    holy_BIDI_TYPE_CS,
    holy_BIDI_TYPE_NSM,
    holy_BIDI_TYPE_BN,
    holy_BIDI_TYPE_B,
    holy_BIDI_TYPE_S,
    holy_BIDI_TYPE_WS,
    holy_BIDI_TYPE_ON
  };

enum holy_join_type
  {
    holy_JOIN_TYPE_NONJOINING = 0,
    holy_JOIN_TYPE_LEFT = 1,
    holy_JOIN_TYPE_RIGHT = 2,
    holy_JOIN_TYPE_DUAL = 3,
    holy_JOIN_TYPE_CAUSING = 4,
    holy_JOIN_TYPE_TRANSPARENT = 5
  };

enum holy_comb_type
  {
    holy_UNICODE_COMB_NONE = 0,
    holy_UNICODE_COMB_OVERLAY = 1,
    holy_UNICODE_COMB_HEBREW_SHEVA = 10,
    holy_UNICODE_COMB_HEBREW_HATAF_SEGOL = 11,
    holy_UNICODE_COMB_HEBREW_HATAF_PATAH = 12,
    holy_UNICODE_COMB_HEBREW_HATAF_QAMATS = 13,
    holy_UNICODE_COMB_HEBREW_HIRIQ = 14,
    holy_UNICODE_COMB_HEBREW_TSERE = 15,
    holy_UNICODE_COMB_HEBREW_SEGOL = 16,
    holy_UNICODE_COMB_HEBREW_PATAH = 17,
    holy_UNICODE_COMB_HEBREW_QAMATS = 18,
    holy_UNICODE_COMB_HEBREW_HOLAM = 19,
    holy_UNICODE_COMB_HEBREW_QUBUTS = 20,
    holy_UNICODE_COMB_HEBREW_DAGESH = 21,
    holy_UNICODE_COMB_HEBREW_METEG = 22,
    holy_UNICODE_COMB_HEBREW_RAFE = 23,
    holy_UNICODE_COMB_HEBREW_SHIN_DOT = 24,
    holy_UNICODE_COMB_HEBREW_SIN_DOT = 25,
    holy_UNICODE_COMB_HEBREW_VARIKA = 26,
    holy_UNICODE_COMB_ARABIC_FATHATAN = 27,
    holy_UNICODE_COMB_ARABIC_DAMMATAN = 28,
    holy_UNICODE_COMB_ARABIC_KASRATAN = 29,
    holy_UNICODE_COMB_ARABIC_FATHAH = 30,
    holy_UNICODE_COMB_ARABIC_DAMMAH = 31,
    holy_UNICODE_COMB_ARABIC_KASRA = 32,
    holy_UNICODE_COMB_ARABIC_SHADDA = 33,
    holy_UNICODE_COMB_ARABIC_SUKUN = 34,
    holy_UNICODE_COMB_ARABIC_SUPERSCRIPT_ALIF = 35,
    holy_UNICODE_COMB_SYRIAC_SUPERSCRIPT_ALAPH = 36,
    holy_UNICODE_STACK_ATTACHED_BELOW = 202,
    holy_UNICODE_STACK_ATTACHED_ABOVE = 214,
    holy_UNICODE_COMB_ATTACHED_ABOVE_RIGHT = 216,
    holy_UNICODE_STACK_BELOW = 220,
    holy_UNICODE_COMB_BELOW_RIGHT = 222,
    holy_UNICODE_COMB_ABOVE_LEFT = 228,
    holy_UNICODE_STACK_ABOVE = 230,
    holy_UNICODE_COMB_ABOVE_RIGHT = 232,
    holy_UNICODE_COMB_YPOGEGRAMMENI = 240,
    /* If combining nature is indicated only by class and
       not "combining type".  */
    holy_UNICODE_COMB_ME = 253,
    holy_UNICODE_COMB_MC = 254,
    holy_UNICODE_COMB_MN = 255,
  };

struct holy_unicode_combining
{
  holy_uint32_t code:21;
  enum holy_comb_type type:8;
};
/* This structure describes a glyph as opposed to character.  */
struct holy_unicode_glyph
{
  holy_uint32_t base:23; /* minimum: 21 */
  holy_uint16_t variant:9; /* minimum: 9 */

  holy_uint8_t attributes:5; /* minimum: 5 */
  holy_uint8_t bidi_level:6; /* minimum: 6 */
  enum holy_bidi_type bidi_type:5; /* minimum: :5 */

  unsigned ncomb:8;
  /* Hint by unicode subsystem how wide this character usually is.
     Real width is determined by font. Set only in UTF-8 stream.  */
  int estimated_width:8;

  holy_size_t orig_pos;
  union
  {
    struct holy_unicode_combining combining_inline[sizeof (void *)
						   / sizeof (struct holy_unicode_combining)];
    struct holy_unicode_combining *combining_ptr;
  };
};

#define holy_UNICODE_GLYPH_ATTRIBUTE_MIRROR 0x1
#define holy_UNICODE_GLYPH_ATTRIBUTES_JOIN_LEFT_TO_RIGHT_SHIFT 1
#define holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED 0x2
#define holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED \
  (holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED \
   << holy_UNICODE_GLYPH_ATTRIBUTES_JOIN_LEFT_TO_RIGHT_SHIFT)
/* Set iff the corresponding joining flags come from ZWJ or ZWNJ.  */
#define holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT 0x8
#define holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT \
  (holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT \
   << holy_UNICODE_GLYPH_ATTRIBUTES_JOIN_LEFT_TO_RIGHT_SHIFT)
#define holy_UNICODE_GLYPH_ATTRIBUTES_JOIN \
  (holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED \
   | holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED \
   | holy_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT \
   | holy_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT)

enum
  {
    holy_UNICODE_DOTLESS_LOWERCASE_I       = 0x0131,
    holy_UNICODE_DOTLESS_LOWERCASE_J       = 0x0237,
    holy_UNICODE_COMBINING_GRAPHEME_JOINER = 0x034f,
    holy_UNICODE_HEBREW_WAW                = 0x05d5,
    holy_UNICODE_ARABIC_START              = 0x0600,
    holy_UNICODE_ARABIC_END                = 0x0700,
    holy_UNICODE_THAANA_ABAFILI            = 0x07a6,
    holy_UNICODE_THAANA_AABAAFILI          = 0x07a7,
    holy_UNICODE_THAANA_IBIFILI            = 0x07a8,
    holy_UNICODE_THAANA_EEBEEFILI          = 0x07a9,
    holy_UNICODE_THAANA_UBUFILI            = 0x07aa,
    holy_UNICODE_THAANA_OOBOOFILI          = 0x07ab,
    holy_UNICODE_THAANA_EBEFILI            = 0x07ac,
    holy_UNICODE_THAANA_EYBEYFILI          = 0x07ad,
    holy_UNICODE_THAANA_OBOFILI            = 0x07ae,
    holy_UNICODE_THAANA_OABOAFILI          = 0x07af,
    holy_UNICODE_THAANA_SUKUN              = 0x07b0,
    holy_UNICODE_ZWNJ                      = 0x200c,
    holy_UNICODE_ZWJ                       = 0x200d,
    holy_UNICODE_LRM                       = 0x200e,
    holy_UNICODE_RLM                       = 0x200f,
    holy_UNICODE_LRE                       = 0x202a,
    holy_UNICODE_RLE                       = 0x202b,
    holy_UNICODE_PDF                       = 0x202c,
    holy_UNICODE_LRO                       = 0x202d,
    holy_UNICODE_RLO                       = 0x202e,
    holy_UNICODE_LEFTARROW                 = 0x2190,
    holy_UNICODE_UPARROW                   = 0x2191,
    holy_UNICODE_RIGHTARROW                = 0x2192,
    holy_UNICODE_DOWNARROW                 = 0x2193,
    holy_UNICODE_UPDOWNARROW               = 0x2195,
    holy_UNICODE_LIGHT_HLINE               = 0x2500,
    holy_UNICODE_HLINE                     = 0x2501,
    holy_UNICODE_LIGHT_VLINE               = 0x2502,
    holy_UNICODE_VLINE                     = 0x2503,
    holy_UNICODE_LIGHT_CORNER_UL           = 0x250c,
    holy_UNICODE_CORNER_UL                 = 0x250f,
    holy_UNICODE_LIGHT_CORNER_UR           = 0x2510,
    holy_UNICODE_CORNER_UR                 = 0x2513,
    holy_UNICODE_LIGHT_CORNER_LL           = 0x2514,
    holy_UNICODE_CORNER_LL                 = 0x2517,
    holy_UNICODE_LIGHT_CORNER_LR           = 0x2518,
    holy_UNICODE_CORNER_LR                 = 0x251b,
    holy_UNICODE_BLACK_UP_TRIANGLE         = 0x25b2,
    holy_UNICODE_BLACK_RIGHT_TRIANGLE      = 0x25ba,
    holy_UNICODE_BLACK_DOWN_TRIANGLE       = 0x25bc,
    holy_UNICODE_BLACK_LEFT_TRIANGLE       = 0x25c4,
    holy_UNICODE_VARIATION_SELECTOR_1      = 0xfe00,
    holy_UNICODE_VARIATION_SELECTOR_16     = 0xfe0f,
    holy_UNICODE_TAG_START                 = 0xe0000,
    holy_UNICODE_TAG_END                   = 0xe007f,
    holy_UNICODE_VARIATION_SELECTOR_17     = 0xe0100,
    holy_UNICODE_VARIATION_SELECTOR_256    = 0xe01ef,
    holy_UNICODE_LAST_VALID                = 0x10ffff
  };

extern struct holy_unicode_compact_range holy_unicode_compact[];
extern struct holy_unicode_bidi_pair holy_unicode_bidi_pairs[];

#define holy_UNICODE_MAX_CACHED_CHAR 0x20000
/*  Unicode mandates an arbitrary limit.  */
#define holy_BIDI_MAX_EXPLICIT_LEVEL 61

struct holy_term_pos
{
  unsigned valid:1;
  unsigned x:15, y:16;
};

holy_ssize_t
holy_bidi_logical_to_visual (const holy_uint32_t *logical,
			     holy_size_t logical_len,
			     struct holy_unicode_glyph **visual_out,
			     holy_size_t (*getcharwidth) (const struct holy_unicode_glyph *visual, void *getcharwidth_arg),
			     void *getcharwidth_arg,
			     holy_size_t max_width,
			     holy_size_t start_width, holy_uint32_t codechar,
			     struct holy_term_pos *pos,
			     int primitive_wrap);

enum holy_comb_type
holy_unicode_get_comb_type (holy_uint32_t c);
holy_size_t
holy_unicode_aglomerate_comb (const holy_uint32_t *in, holy_size_t inlen,
			      struct holy_unicode_glyph *out);

static inline const struct holy_unicode_combining *
holy_unicode_get_comb (const struct holy_unicode_glyph *in)
{
  if (in->ncomb == 0)
    return NULL;
  if (in->ncomb > ARRAY_SIZE (in->combining_inline))
    return in->combining_ptr;
  return in->combining_inline;
}

static inline void
holy_unicode_destroy_glyph (struct holy_unicode_glyph *glyph)
{
  if (glyph->ncomb > ARRAY_SIZE (glyph->combining_inline))
    holy_free (glyph->combining_ptr);
  glyph->ncomb = 0;
}

static inline struct holy_unicode_glyph *
holy_unicode_glyph_dup (const struct holy_unicode_glyph *in)
{
  struct holy_unicode_glyph *out = holy_malloc (sizeof (*out));
  if (!out)
    return NULL;
  holy_memcpy (out, in, sizeof (*in));
  if (in->ncomb > ARRAY_SIZE (out->combining_inline))
    {
      out->combining_ptr = holy_malloc (in->ncomb * sizeof (out->combining_ptr[0]));
      if (!out->combining_ptr)
	{
	  holy_free (out);
	  return NULL;
	}
      holy_memcpy (out->combining_ptr, in->combining_ptr,
		   in->ncomb * sizeof (out->combining_ptr[0]));
    }
  else
    holy_memcpy (&out->combining_inline, &in->combining_inline,
		 sizeof (out->combining_inline));
  return out;
}

static inline void
holy_unicode_set_glyph (struct holy_unicode_glyph *out,
			const struct holy_unicode_glyph *in)
{
  holy_memcpy (out, in, sizeof (*in));
  if (in->ncomb > ARRAY_SIZE (out->combining_inline))
    {
      out->combining_ptr = holy_malloc (in->ncomb * sizeof (out->combining_ptr[0]));
      if (!out->combining_ptr)
	return;
      holy_memcpy (out->combining_ptr, in->combining_ptr,
		   in->ncomb * sizeof (out->combining_ptr[0]));
    }
  else
    holy_memcpy (&out->combining_inline, &in->combining_inline,
		 sizeof (out->combining_inline));
}

static inline struct holy_unicode_glyph *
holy_unicode_glyph_from_code (holy_uint32_t code)
{
  struct holy_unicode_glyph *ret;
  ret = holy_zalloc (sizeof (*ret));
  if (!ret)
    return NULL;

  ret->base = code;

  return ret;
}

static inline void
holy_unicode_set_glyph_from_code (struct holy_unicode_glyph *glyph,
				  holy_uint32_t code)
{
  holy_memset (glyph, 0, sizeof (*glyph));

  glyph->base = code;
}

holy_uint32_t
holy_unicode_mirror_code (holy_uint32_t in);
holy_uint32_t
holy_unicode_shape_code (holy_uint32_t in, holy_uint8_t attr);

const holy_uint32_t *
holy_unicode_get_comb_end (const holy_uint32_t *end,
			   const holy_uint32_t *cur);

#endif
