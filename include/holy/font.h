/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FONT_HEADER
#define holy_FONT_HEADER	1

#include <holy/types.h>
#include <holy/video.h>
#include <holy/file.h>
#include <holy/unicode.h>

/* Forward declaration of opaque structure holy_font.
   Users only pass struct holy_font pointers to the font module functions,
   and do not have knowledge of the structure contents.  */
/* Full structure was moved here for inline function but still
   shouldn't be used directly.
 */
struct holy_font
{
  char *name;
  holy_file_t file;
  char *family;
  short point_size;
  short weight;
  short max_char_width;
  short max_char_height;
  short ascent;
  short descent;
  short leading;
  holy_uint32_t num_chars;
  struct char_index_entry *char_index;
  holy_uint16_t *bmp_idx;
};

/* Font type used to access font functions.  */
typedef struct holy_font *holy_font_t;

struct holy_font_node
{
  struct holy_font_node *next;
  holy_font_t value;
};

/* Global font registry.  */
extern struct holy_font_node *holy_font_list;

struct holy_font_glyph
{
  /* Reference to the font this glyph belongs to.  */
  holy_font_t font;

  /* Glyph bitmap width in pixels.  */
  holy_uint16_t width;

  /* Glyph bitmap height in pixels.  */
  holy_uint16_t height;

  /* Glyph bitmap x offset in pixels.  Add to screen coordinate.  */
  holy_int16_t offset_x;

  /* Glyph bitmap y offset in pixels.  Subtract from screen coordinate.  */
  holy_int16_t offset_y;

  /* Number of pixels to advance to start the next character.  */
  holy_uint16_t device_width;

  /* Row-major order, packed bits (no padding; rows can break within a byte).
     The length of the array is (width * height + 7) / 8.  Within a
     byte, the most significant bit is the first (leftmost/uppermost) pixel.
     Pixels are coded as bits, value 1 meaning of opaque pixel and 0 is
     transparent.  If the length of the array does not fit byte boundary, it
     will be padded with 0 bits to make it fit.  */
  holy_uint8_t bitmap[0];
};

/* Part of code field which is really used as such.  */
#define holy_FONT_CODE_CHAR_MASK     0x001fffff
#define holy_FONT_CODE_RIGHT_JOINED  0x80000000
#define holy_FONT_CODE_LEFT_JOINED   0x40000000

/* Initialize the font loader.
   Must be called before any fonts are loaded or used.  */
void holy_font_loader_init (void);

/* Load a font and add it to the beginning of the global font list.
   Returns: 0 upon success; nonzero upon failure.  */
holy_font_t EXPORT_FUNC(holy_font_load) (const char *filename);

/* Get the font that has the specified name.  Font names are in the form
   "Family Name Bold Italic 14", where Bold and Italic are optional.
   If no font matches the name specified, the most recently loaded font
   is returned as a fallback.  */
holy_font_t EXPORT_FUNC (holy_font_get) (const char *font_name);

const char *EXPORT_FUNC (holy_font_get_name) (holy_font_t font);

int EXPORT_FUNC (holy_font_get_max_char_width) (holy_font_t font);

/* Get the maximum height of any character in the font in pixels.  */
static inline int
holy_font_get_max_char_height (holy_font_t font)
{
  return font->max_char_height;
}

/* Get the distance in pixels from the top of characters to the baseline.  */
static inline int
holy_font_get_ascent (holy_font_t font)
{
  return font->ascent;
}

int EXPORT_FUNC (holy_font_get_descent) (holy_font_t font);

int EXPORT_FUNC (holy_font_get_leading) (holy_font_t font);

int EXPORT_FUNC (holy_font_get_height) (holy_font_t font);

int EXPORT_FUNC (holy_font_get_xheight) (holy_font_t font);

struct holy_font_glyph *EXPORT_FUNC (holy_font_get_glyph) (holy_font_t font,
							   holy_uint32_t code);

struct holy_font_glyph *EXPORT_FUNC (holy_font_get_glyph_with_fallback) (holy_font_t font,
									 holy_uint32_t code);

holy_err_t EXPORT_FUNC (holy_font_draw_glyph) (struct holy_font_glyph *glyph,
					       holy_video_color_t color,
					       int left_x, int baseline_y);

int
EXPORT_FUNC (holy_font_get_constructed_device_width) (holy_font_t hinted_font,
					const struct holy_unicode_glyph *glyph_id);
struct holy_font_glyph *
EXPORT_FUNC (holy_font_construct_glyph) (holy_font_t hinted_font,
			   const struct holy_unicode_glyph *glyph_id);

#endif /* ! holy_FONT_HEADER */
