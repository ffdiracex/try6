/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/bufio.h>
#include <holy/dl.h>
#include <holy/file.h>
#include <holy/font.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/video.h>
#include <holy/bitmap.h>
#include <holy/charset.h>
#include <holy/unicode.h>
#include <holy/fontformat.h>
#include <holy/gfxmenu_view.h>

/* Draw a UTF-8 string of text on the current video render target.
   The x coordinate specifies the starting x position for the first character,
   while the y coordinate specifies the baseline position.
   If the string contains a character that FONT does not contain, then
   a glyph from another loaded font may be used instead.  */
holy_err_t
holy_font_draw_string (const char *str, holy_font_t font,
                       holy_video_color_t color,
                       int left_x, int baseline_y)
{
  int x;
  holy_uint32_t *logical;
  holy_ssize_t logical_len, visual_len;
  struct holy_unicode_glyph *visual, *ptr;
  holy_err_t err;

  logical_len = holy_utf8_to_ucs4_alloc (str, &logical, 0);
  if (logical_len < 0)
    return holy_errno;

  visual_len = holy_bidi_logical_to_visual (logical, logical_len, &visual,
					    0, 0, 0, 0, 0, 0, 0);
  holy_free (logical);
  if (visual_len < 0)
    return holy_errno;

  err = holy_ERR_NONE;
  for (ptr = visual, x = left_x; ptr < visual + visual_len; ptr++)
    {
      struct holy_font_glyph *glyph;
      glyph = holy_font_construct_glyph (font, ptr);
      if (!glyph)
	{
	  err = holy_errno;
	  goto out;
	}
      err = holy_font_draw_glyph (glyph, color, x, baseline_y);
      if (err)
	goto out;
      x += glyph->device_width;
    }

out:
  for (ptr = visual; ptr < visual + visual_len; ptr++)
    holy_unicode_destroy_glyph (ptr);
  holy_free (visual);

  return err;
}

/* Get the width in pixels of the specified UTF-8 string, when rendered in
   in the specified font (but falling back on other fonts for glyphs that
   are missing).  */
int
holy_font_get_string_width (holy_font_t font, const char *str)
{
  int width = 0;
  holy_uint32_t *ptr;
  holy_ssize_t logical_len;
  holy_uint32_t *logical;

  logical_len = holy_utf8_to_ucs4_alloc (str, &logical, 0);
  if (logical_len < 0)
    {
      holy_errno = holy_ERR_NONE;
      return 0;
    }

  for (ptr = logical; ptr < logical + logical_len;)
    {
      struct holy_unicode_glyph glyph;

      ptr += holy_unicode_aglomerate_comb (ptr,
					   logical_len - (ptr - logical),
					   &glyph);
      width += holy_font_get_constructed_device_width (font, &glyph);

      holy_unicode_destroy_glyph (&glyph);
    }
  holy_free (logical);

  return width;
}
