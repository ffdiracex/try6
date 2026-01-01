/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/font.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/video.h>
#include <holy/gfxterm.h>
#include <holy/bitmap.h>
#include <holy/command.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#define DEFAULT_VIDEO_MODE	"auto"
#define DEFAULT_BORDER_WIDTH	10

#define DEFAULT_STANDARD_COLOR  0x07

struct holy_dirty_region
{
  int top_left_x;
  int top_left_y;
  int bottom_right_x;
  int bottom_right_y;
};

struct holy_colored_char
{
  /* An Unicode codepoint.  */
  struct holy_unicode_glyph code;

  /* Color values.  */
  holy_video_color_t fg_color;
  holy_video_color_t bg_color;
};

struct holy_virtual_screen
{
  /* Dimensions of the virtual screen in pixels.  */
  unsigned int width;
  unsigned int height;

  /* Offset in the display in pixels.  */
  unsigned int offset_x;
  unsigned int offset_y;

  /* TTY Character sizes in pixes.  */
  unsigned int normal_char_width;
  unsigned int normal_char_height;

  /* Virtual screen TTY size in characters.  */
  unsigned int columns;
  unsigned int rows;

  /* Current cursor location in characters.  */
  unsigned int cursor_x;
  unsigned int cursor_y;

  /* Current cursor state. */
  int cursor_state;

  /* Font settings. */
  holy_font_t font;

  /* Terminal color settings.  */
  holy_uint8_t standard_color_setting;
  holy_uint8_t term_color;

  /* Color settings.  */
  holy_video_color_t fg_color;
  holy_video_color_t bg_color;
  holy_video_color_t bg_color_display;

  /* Text buffer for virtual screen.  Contains (columns * rows) number
     of entries.  */
  struct holy_colored_char *text_buffer;

  int total_scroll;

  int functional;
};

struct holy_gfxterm_window
{
  unsigned x;
  unsigned y;
  unsigned width;
  unsigned height;
  int double_repaint;
};

static struct holy_video_render_target *render_target;
void (*holy_gfxterm_decorator_hook) (void) = NULL;
static struct holy_gfxterm_window window;
static struct holy_virtual_screen virtual_screen;
static int repaint_scheduled = 0;
static int repaint_was_scheduled = 0;

static void destroy_window (void);

static struct holy_video_render_target *text_layer;

struct holy_gfxterm_background holy_gfxterm_background;

static struct holy_dirty_region dirty_region;

static void dirty_region_reset (void);

static int dirty_region_is_empty (void);

static void dirty_region_add (int x, int y,
                              unsigned int width, unsigned int height);

static unsigned int calculate_normal_character_width (holy_font_t font);

static unsigned char calculate_character_width (struct holy_font_glyph *glyph);

static void holy_gfxterm_refresh (struct holy_term_output *term __attribute__ ((unused)));

static holy_size_t
holy_gfxterm_getcharwidth (struct holy_term_output *term __attribute__ ((unused)),
			   const struct holy_unicode_glyph *c);

static void
set_term_color (holy_uint8_t term_color)
{
  struct holy_video_render_target *old_target;

  /* Save previous target and switch to text layer.  */
  holy_video_get_active_render_target (&old_target);
  holy_video_set_active_render_target (text_layer);

  /* Map terminal color to text layer compatible video colors.  */
  virtual_screen.fg_color = holy_video_map_color(term_color & 0x0f);

  /* Special case: use black as transparent color.  */
  if (((term_color >> 4) & 0x0f) == 0)
    {
      virtual_screen.bg_color = holy_video_map_rgba(0, 0, 0, 0);
    }
  else
    {
      virtual_screen.bg_color = holy_video_map_color((term_color >> 4) & 0x0f);
    }

  /* Restore previous target.  */
  holy_video_set_active_render_target (old_target);
}

static void
clear_char (struct holy_colored_char *c)
{
  holy_unicode_destroy_glyph (&c->code);
  holy_unicode_set_glyph_from_code (&c->code, ' ');
  c->fg_color = virtual_screen.fg_color;
  c->bg_color = virtual_screen.bg_color;
}

static void
holy_virtual_screen_free (void)
{
  virtual_screen.functional = 0;

  /* If virtual screen has been allocated, free it.  */
  if (virtual_screen.text_buffer != 0)
    {
      unsigned i;
      for (i = 0;
	   i < virtual_screen.columns * virtual_screen.rows;
	   i++)
	holy_unicode_destroy_glyph (&virtual_screen.text_buffer[i].code);
      holy_free (virtual_screen.text_buffer);
    }

  /* Reset virtual screen data.  */
  holy_memset (&virtual_screen, 0, sizeof (virtual_screen));

  /* Free render targets.  */
  holy_video_delete_render_target (text_layer);
  text_layer = 0;
}

static holy_err_t
holy_virtual_screen_setup (unsigned int x, unsigned int y,
                           unsigned int width, unsigned int height,
			   holy_font_t font)
{
  unsigned int i;

  /* Free old virtual screen.  */
  holy_virtual_screen_free ();

  /* Initialize with default data.  */
  virtual_screen.font = font;
  virtual_screen.width = width;
  virtual_screen.height = height;
  virtual_screen.offset_x = x;
  virtual_screen.offset_y = y;
  virtual_screen.normal_char_width =
    calculate_normal_character_width (virtual_screen.font);
  virtual_screen.normal_char_height =
    holy_font_get_max_char_height (virtual_screen.font);
  if (virtual_screen.normal_char_height == 0)
    virtual_screen.normal_char_height = 16;
  virtual_screen.cursor_x = 0;
  virtual_screen.cursor_y = 0;
  virtual_screen.cursor_state = 1;
  virtual_screen.total_scroll = 0;

  /* Calculate size of text buffer.  */
  virtual_screen.columns = virtual_screen.width / virtual_screen.normal_char_width;
  virtual_screen.rows = virtual_screen.height / virtual_screen.normal_char_height;

  /* Allocate memory for text buffer.  */
  virtual_screen.text_buffer =
    (struct holy_colored_char *) holy_malloc (virtual_screen.columns
                                              * virtual_screen.rows
                                              * sizeof (*virtual_screen.text_buffer));
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  /* Create new render target for text layer.  */
  holy_video_create_render_target (&text_layer,
                                   virtual_screen.width,
                                   virtual_screen.height,
                                   holy_VIDEO_MODE_TYPE_INDEX_COLOR
                                   | holy_VIDEO_MODE_TYPE_ALPHA);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  /* As we want to have colors compatible with rendering target,
     we can only have those after mode is initialized.  */
  holy_video_set_active_render_target (text_layer);

  virtual_screen.standard_color_setting = DEFAULT_STANDARD_COLOR;

  virtual_screen.term_color = virtual_screen.standard_color_setting;

  set_term_color (virtual_screen.term_color);

  holy_video_set_active_render_target (render_target);

  virtual_screen.bg_color_display =
    holy_video_map_rgba_color (holy_gfxterm_background.default_bg_color);

  /* Clear out text buffer. */
  for (i = 0; i < virtual_screen.columns * virtual_screen.rows; i++)
    {
      virtual_screen.text_buffer[i].code.ncomb = 0;
      clear_char (&(virtual_screen.text_buffer[i]));
    }
  if (holy_errno)
    return holy_errno;

  virtual_screen.functional = 1;

  return holy_ERR_NONE;
}

void
holy_gfxterm_schedule_repaint (void)
{
  repaint_scheduled = 1;
}

holy_err_t
holy_gfxterm_set_window (struct holy_video_render_target *target,
			 int x, int y, int width, int height,
			 int double_repaint,
			 holy_font_t font, int border_width)
{
  /* Clean up any prior instance.  */
  destroy_window ();

  /* Set the render target.  */
  render_target = target;

  /* Create virtual screen.  */
  if (holy_virtual_screen_setup (border_width, border_width,
                                 width - 2 * border_width, 
                                 height - 2 * border_width, 
                                 font) 
      != holy_ERR_NONE)
    {
      return holy_errno;
    }

  /* Set window bounds.  */
  window.x = x;
  window.y = y;
  window.width = width;
  window.height = height;
  window.double_repaint = double_repaint;

  dirty_region_reset ();
  holy_gfxterm_schedule_repaint ();

  return holy_errno;
}

static holy_err_t
holy_gfxterm_fullscreen (void)
{
  const char *font_name;
  struct holy_video_mode_info mode_info;
  holy_video_color_t color;
  holy_err_t err;
  int double_redraw;
  holy_font_t font;

  err = holy_video_get_info (&mode_info);
  /* Figure out what mode we ended up.  */
  if (err)
    return err;

  holy_video_set_active_render_target (holy_VIDEO_RENDER_TARGET_DISPLAY);

  double_redraw = mode_info.mode_type & holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED
    && !(mode_info.mode_type & holy_VIDEO_MODE_TYPE_UPDATING_SWAP);

  /* Make sure screen is set to the default background color.  */
  color = holy_video_map_rgba_color (holy_gfxterm_background.default_bg_color);
  holy_video_fill_rect (color, 0, 0, mode_info.width, mode_info.height);
  if (double_redraw)
    {
      holy_video_swap_buffers ();
      holy_video_fill_rect (color, 0, 0, mode_info.width, mode_info.height);
    }

  /* Select the font to use.  */
  font_name = holy_env_get ("gfxterm_font");
  if (! font_name)
    font_name = "";   /* Allow fallback to any font.  */

  font = holy_font_get (font_name);
  if (!font)
    return holy_error (holy_ERR_BAD_FONT, "no font loaded");

  holy_gfxterm_decorator_hook = NULL;

  return holy_gfxterm_set_window (holy_VIDEO_RENDER_TARGET_DISPLAY,
				  0, 0, mode_info.width, mode_info.height,
				  double_redraw,
				  font, DEFAULT_BORDER_WIDTH);
}

static holy_err_t
holy_gfxterm_term_init (struct holy_term_output *term __attribute__ ((unused)))
{
  char *tmp;
  holy_err_t err;
  const char *modevar;

  /* Parse gfxmode environment variable if set.  */
  modevar = holy_env_get ("gfxmode");
  if (! modevar || *modevar == 0)
    err = holy_video_set_mode (DEFAULT_VIDEO_MODE,
			       holy_VIDEO_MODE_TYPE_PURE_TEXT, 0);
  else
    {
      tmp = holy_xasprintf ("%s;" DEFAULT_VIDEO_MODE, modevar);
      if (!tmp)
	return holy_errno;
      err = holy_video_set_mode (tmp, holy_VIDEO_MODE_TYPE_PURE_TEXT, 0);
      holy_free (tmp);
    }

  if (err)
    return err;

  err = holy_gfxterm_fullscreen ();
  if (err)
    holy_video_restore ();

  return err;
}

static void
destroy_window (void)
{
  holy_virtual_screen_free ();
}

static holy_err_t
holy_gfxterm_term_fini (struct holy_term_output *term __attribute__ ((unused)))
{
  unsigned i;
  destroy_window ();
  holy_video_restore ();

  for (i = 0; i < virtual_screen.columns * virtual_screen.rows; i++)
    {
      holy_unicode_destroy_glyph (&virtual_screen.text_buffer[i].code);
      virtual_screen.text_buffer[i].code.ncomb = 0;
      virtual_screen.text_buffer[i].code.base = 0;
    }

  /* Clear error state.  */
  holy_errno = holy_ERR_NONE;
  return holy_ERR_NONE;
}

static void
redraw_screen_rect (unsigned int x, unsigned int y,
                    unsigned int width, unsigned int height)
{
  holy_video_color_t color;
  holy_video_rect_t saved_view;

  holy_video_set_active_render_target (render_target);
  /* Save viewport and set it to our window.  */
  holy_video_get_viewport ((unsigned *) &saved_view.x,
                           (unsigned *) &saved_view.y, 
                           (unsigned *) &saved_view.width, 
                           (unsigned *) &saved_view.height);
  holy_video_set_viewport (window.x, window.y, window.width, window.height);

  if (holy_gfxterm_background.bitmap)
    {
      /* Render bitmap as background.  */
      holy_video_blit_bitmap (holy_gfxterm_background.bitmap,
			      holy_VIDEO_BLIT_REPLACE, x, y,
			      x, y,
                              width, height);

      /* If bitmap is smaller than requested blit area, use background
         color.  */
      color = virtual_screen.bg_color_display;

      /* Fill right side of the bitmap if needed.  */
      if ((x + width >= holy_gfxterm_background.bitmap->mode_info.width)
	  && (y < holy_gfxterm_background.bitmap->mode_info.height))
        {
          int w = (x + width) - holy_gfxterm_background.bitmap->mode_info.width;
          int h = height;
          unsigned int tx = x;

          if (y + height >= holy_gfxterm_background.bitmap->mode_info.height)
            {
              h = holy_gfxterm_background.bitmap->mode_info.height - y;
            }

          if (holy_gfxterm_background.bitmap->mode_info.width > tx)
            {
              tx = holy_gfxterm_background.bitmap->mode_info.width;
            }

          /* Render background layer.  */
	  holy_video_fill_rect (color, tx, y, w, h);
        }

      /* Fill bottom side of the bitmap if needed.  */
      if (y + height >= holy_gfxterm_background.bitmap->mode_info.height)
        {
          int h = (y + height) - holy_gfxterm_background.bitmap->mode_info.height;
          unsigned int ty = y;

          if (holy_gfxterm_background.bitmap->mode_info.height > ty)
            {
              ty = holy_gfxterm_background.bitmap->mode_info.height;
            }

          /* Render background layer.  */
	  holy_video_fill_rect (color, x, ty, width, h);
        }
    }
  else
    {
      /* Render background layer.  */
      color = virtual_screen.bg_color_display;
      holy_video_fill_rect (color, x, y, width, height);
    }

  if (holy_gfxterm_background.blend_text_bg)
    /* Render text layer as blended.  */
    holy_video_blit_render_target (text_layer, holy_VIDEO_BLIT_BLEND, x, y,
                                   x - virtual_screen.offset_x,
                                   y - virtual_screen.offset_y,
                                   width, height);
  else
    /* Render text layer as replaced (to get texts background color).  */
    holy_video_blit_render_target (text_layer, holy_VIDEO_BLIT_REPLACE, x, y,
                                   x - virtual_screen.offset_x,
                                   y - virtual_screen.offset_y,
                                   width, height);

  /* Restore saved viewport.  */
  holy_video_set_viewport (saved_view.x, saved_view.y,
                           saved_view.width, saved_view.height);
  holy_video_set_active_render_target (render_target);
}

static void
dirty_region_reset (void)
{
  dirty_region.top_left_x = -1;
  dirty_region.top_left_y = -1;
  dirty_region.bottom_right_x = -1;
  dirty_region.bottom_right_y = -1;
  repaint_was_scheduled = 0;
}

static int
dirty_region_is_empty (void)
{
  if ((dirty_region.top_left_x == -1)
      || (dirty_region.top_left_y == -1)
      || (dirty_region.bottom_right_x == -1)
      || (dirty_region.bottom_right_y == -1))
    return 1;
  return 0;
}

static void
dirty_region_add_real (int x, int y, unsigned int width, unsigned int height)
{
  if (dirty_region_is_empty ())
    {
      dirty_region.top_left_x = x;
      dirty_region.top_left_y = y;
      dirty_region.bottom_right_x = x + width - 1;
      dirty_region.bottom_right_y = y + height - 1;
    }
  else
    {
      if (x < dirty_region.top_left_x)
        dirty_region.top_left_x = x;
      if (y < dirty_region.top_left_y)
        dirty_region.top_left_y = y;
      if ((x + (int)width - 1) > dirty_region.bottom_right_x)
        dirty_region.bottom_right_x = x + width - 1;
      if ((y + (int)height - 1) > dirty_region.bottom_right_y)
        dirty_region.bottom_right_y = y + height - 1;
    }
}

static void
dirty_region_add (int x, int y, unsigned int width, unsigned int height)
{
  if ((width == 0) || (height == 0))
    return;

  if (repaint_scheduled)
    {
      dirty_region_add_real (0, 0,
			     window.width, window.height);
      repaint_scheduled = 0;
      repaint_was_scheduled = 1;
    }
  dirty_region_add_real (x, y, width, height);
}

static void
dirty_region_add_virtualscreen (void)
{
  /* Mark virtual screen as dirty.  */
  dirty_region_add (virtual_screen.offset_x, virtual_screen.offset_y,
                    virtual_screen.width, virtual_screen.height);
}


static void
dirty_region_redraw (void)
{
  int x;
  int y;
  int width;
  int height;

  if (dirty_region_is_empty ())
    return;

  x = dirty_region.top_left_x;
  y = dirty_region.top_left_y;

  width = dirty_region.bottom_right_x - x + 1;
  height = dirty_region.bottom_right_y - y + 1;

  if (repaint_was_scheduled && holy_gfxterm_decorator_hook)
    holy_gfxterm_decorator_hook ();

  redraw_screen_rect (x, y, width, height);
}

static inline void
paint_char (unsigned cx, unsigned cy)
{
  struct holy_colored_char *p;
  struct holy_font_glyph *glyph;
  holy_video_color_t color;
  holy_video_color_t bgcolor;
  unsigned int x;
  unsigned int y;
  int ascent;
  unsigned int height;
  unsigned int width;

  if (cy + virtual_screen.total_scroll >= virtual_screen.rows)
    return;

  /* Find out active character.  */
  p = (virtual_screen.text_buffer
       + cx + (cy * virtual_screen.columns));

  if (!p->code.base)
    return;

  /* Get glyph for character.  */
  glyph = holy_font_construct_glyph (virtual_screen.font, &p->code);
  if (!glyph)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }
  ascent = holy_font_get_ascent (virtual_screen.font);

  width = virtual_screen.normal_char_width * calculate_character_width(glyph);
  height = virtual_screen.normal_char_height;

  color = p->fg_color;
  bgcolor = p->bg_color;

  x = cx * virtual_screen.normal_char_width;
  y = (cy + virtual_screen.total_scroll) * virtual_screen.normal_char_height;

  /* Render glyph to text layer.  */
  holy_video_set_active_render_target (text_layer);
  holy_video_fill_rect (bgcolor, x, y, width, height);
  holy_font_draw_glyph (glyph, color, x, y + ascent);
  holy_video_set_active_render_target (render_target);

  /* Mark character to be drawn.  */
  dirty_region_add (virtual_screen.offset_x + x, virtual_screen.offset_y + y,
                    width, height);
}

static inline void
write_char (void)
{
  paint_char (virtual_screen.cursor_x, virtual_screen.cursor_y);
}

static inline void
draw_cursor (int show)
{
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;
  unsigned int ascent;
  holy_video_color_t color;
  
  write_char ();

  if (!show)
    return;

  if (virtual_screen.cursor_y + virtual_screen.total_scroll
      >= virtual_screen.rows)
    return;

  /* Ensure that cursor doesn't go outside of character box.  */
  ascent = holy_font_get_ascent(virtual_screen.font);
  if (ascent > virtual_screen.normal_char_height - 2)
    ascent = virtual_screen.normal_char_height - 2;

  /* Determine cursor properties and position on text layer. */
  x = virtual_screen.cursor_x * virtual_screen.normal_char_width;
  width = virtual_screen.normal_char_width;
  color = virtual_screen.fg_color;
  y = ((virtual_screen.cursor_y + virtual_screen.total_scroll)
       * virtual_screen.normal_char_height
       + ascent);
  height = 2;
  
  /* Render cursor to text layer.  */
  holy_video_set_active_render_target (text_layer);
  holy_video_fill_rect (color, x, y, width, height);
  holy_video_set_active_render_target (render_target);
  
  /* Mark cursor to be redrawn.  */
  dirty_region_add (virtual_screen.offset_x + x,
		    virtual_screen.offset_y + y,
		    width, height);
}

static void
real_scroll (void)
{
  unsigned int i, j, was_scroll;
  holy_video_color_t color;

  if (!virtual_screen.total_scroll)
    return;

  /* If we have bitmap, re-draw screen, otherwise scroll physical screen too.  */
  if (holy_gfxterm_background.bitmap)
    {
      /* Scroll physical screen.  */
      holy_video_set_active_render_target (text_layer);
      color = virtual_screen.bg_color;
      holy_video_scroll (color, 0, -virtual_screen.normal_char_height
			 * virtual_screen.total_scroll);

      /* Mark virtual screen to be redrawn.  */
      dirty_region_add_virtualscreen ();
    }
  else
    {
      holy_video_rect_t saved_view;

      /* Remove cursor.  */
      draw_cursor (0);

      holy_video_set_active_render_target (render_target);

      i = window.double_repaint ? 2 : 1;

      color = virtual_screen.bg_color_display;

      while (i--)
	{
	  /* Save viewport and set it to our window.  */
	  holy_video_get_viewport ((unsigned *) &saved_view.x,
				   (unsigned *) &saved_view.y, 
				   (unsigned *) &saved_view.width, 
				   (unsigned *) &saved_view.height);

	  holy_video_set_viewport (window.x, window.y, window.width,
				   window.height);

	  /* Clear new border area.  */
	  holy_video_fill_rect (color,
				virtual_screen.offset_x,
				virtual_screen.offset_y,
				virtual_screen.width,
				virtual_screen.normal_char_height
				* virtual_screen.total_scroll);

	  holy_video_set_active_render_target (render_target);
	  dirty_region_redraw ();

	  /* Scroll physical screen.  */
	  holy_video_scroll (color, 0, -virtual_screen.normal_char_height
			     * virtual_screen.total_scroll);

	  /* Restore saved viewport.  */
	  holy_video_set_viewport (saved_view.x, saved_view.y,
				   saved_view.width, saved_view.height);

	  if (i)
	    holy_video_swap_buffers ();
	}
      dirty_region_reset ();

      /* Scroll physical screen.  */
      holy_video_set_active_render_target (text_layer);
      color = virtual_screen.bg_color;
      holy_video_scroll (color, 0, -virtual_screen.normal_char_height
			 * virtual_screen.total_scroll);

      holy_video_set_active_render_target (render_target);

    }

  was_scroll = virtual_screen.total_scroll;
  virtual_screen.total_scroll = 0;

  if (was_scroll > virtual_screen.rows)
    was_scroll = virtual_screen.rows;

  /* Draw shadow part.  */
  for (i = virtual_screen.rows - was_scroll;
       i < virtual_screen.rows; i++)
    for (j = 0; j < virtual_screen.columns; j++)
      paint_char (j, i);

  /* Draw cursor if visible.  */
  if (virtual_screen.cursor_state)
    draw_cursor (1);
}

static void
scroll_up (void)
{
  unsigned int i;

  /* Clear first line in text buffer.  */
  for (i = 0; i < virtual_screen.columns; i++)
    holy_unicode_destroy_glyph (&virtual_screen.text_buffer[i].code);
  
  /* Scroll text buffer with one line to up.  */
  holy_memmove (virtual_screen.text_buffer,
                virtual_screen.text_buffer + virtual_screen.columns,
                sizeof (*virtual_screen.text_buffer)
                * virtual_screen.columns
                * (virtual_screen.rows - 1));

  /* Clear last line in text buffer.  */
  for (i = virtual_screen.columns * (virtual_screen.rows - 1);
       i < virtual_screen.columns * virtual_screen.rows;
       i++)
    clear_char (&(virtual_screen.text_buffer[i]));

  virtual_screen.total_scroll++;
}

static void
holy_gfxterm_putchar (struct holy_term_output *term,
		      const struct holy_unicode_glyph *c)
{
  if (!virtual_screen.functional)
    return;

  if (c->base == '\a')
    /* FIXME */
    return;

  /* Erase current cursor, if any.  */
  if (virtual_screen.cursor_state)
    draw_cursor (0);

  if (c->base == '\b' || c->base == '\n' || c->base == '\r')
    {
      switch (c->base)
        {
        case '\b':
          if (virtual_screen.cursor_x > 0)
            virtual_screen.cursor_x--;
          break;

        case '\n':
          if (virtual_screen.cursor_y >= virtual_screen.rows - 1)
            scroll_up ();
          else
            virtual_screen.cursor_y++;
          break;

        case '\r':
          virtual_screen.cursor_x = 0;
          break;
        }
    }
  else
    {
      struct holy_colored_char *p;
      unsigned char char_width;

      /* Calculate actual character width for glyph. This is number of
         times of normal_font_width.  */
      char_width = holy_gfxterm_getcharwidth (term, c);

      /* If we are about to exceed line length, wrap to next line.  */
      if (virtual_screen.cursor_x + char_width > virtual_screen.columns)
	{
          if (virtual_screen.cursor_y >= virtual_screen.rows - 1)
            scroll_up ();
          else
            virtual_screen.cursor_y++;
	}

      /* Find position on virtual screen, and fill information.  */
      p = (virtual_screen.text_buffer +
           virtual_screen.cursor_x +
           virtual_screen.cursor_y * virtual_screen.columns);
      holy_unicode_destroy_glyph (&p->code);
      holy_unicode_set_glyph (&p->code, c);
      holy_errno = holy_ERR_NONE;
      p->fg_color = virtual_screen.fg_color;
      p->bg_color = virtual_screen.bg_color;

      /* If we have large glyph, add fixup info.  */
      if (char_width > 1)
        {
          unsigned i;

          for (i = 1; i < char_width && p + i < 
		 virtual_screen.text_buffer + virtual_screen.columns
		 * virtual_screen.rows; i++)
	      {
		holy_unicode_destroy_glyph (&p[i].code);
		p[i].code.base = 0;
	      }
        }

      /* Draw glyph.  */
      write_char ();

      /* Make sure we scroll screen when needed and wrap line correctly.  */
      virtual_screen.cursor_x += char_width;
      if (virtual_screen.cursor_x >= virtual_screen.columns)
        {
          virtual_screen.cursor_x = 0;

          if (virtual_screen.cursor_y >= virtual_screen.rows - 1)
            scroll_up ();
          else
            virtual_screen.cursor_y++;
        }
    }

  /* Redraw cursor if it should be visible.  */
  /* Note: This will redraw the character as well, which means that the
     above call to write_char is redundant when the cursor is showing.  */
  if (virtual_screen.cursor_state)
    draw_cursor (1);
}

/* Use ASCII characters to determine normal character width.  */
static unsigned int
calculate_normal_character_width (holy_font_t font)
{
  struct holy_font_glyph *glyph;
  unsigned int width = 0;
  unsigned int i;

  /* Get properties of every printable ASCII character.  */
  for (i = 32; i < 127; i++)
    {
      glyph = holy_font_get_glyph (font, i);

      /* Skip unknown characters.  Should never happen on normal conditions.  */
      if (! glyph)
	continue;

      if (glyph->device_width > width)
	width = glyph->device_width;
    }
  if (!width)
    return 8;

  return width;
}

static unsigned char
calculate_character_width (struct holy_font_glyph *glyph)
{
  if (! glyph || glyph->device_width == 0)
    return 1;

  return (glyph->device_width
          + (virtual_screen.normal_char_width - 1))
         / virtual_screen.normal_char_width;
}

static holy_size_t
holy_gfxterm_getcharwidth (struct holy_term_output *term __attribute__ ((unused)),
			   const struct holy_unicode_glyph *c)
{
  int dev_width;
  dev_width = holy_font_get_constructed_device_width (virtual_screen.font, c);

  if (dev_width == 0)
    return 1;

  return (dev_width + (virtual_screen.normal_char_width - 1))
    / virtual_screen.normal_char_width;
}

static struct holy_term_coordinate
holy_virtual_screen_getwh (struct holy_term_output *term __attribute__ ((unused)))
{
  return (struct holy_term_coordinate) { virtual_screen.columns, virtual_screen.rows };
}

static struct holy_term_coordinate
holy_virtual_screen_getxy (struct holy_term_output *term __attribute__ ((unused)))
{
  return (struct holy_term_coordinate) { virtual_screen.cursor_x, virtual_screen.cursor_y };
}

static void
holy_gfxterm_gotoxy (struct holy_term_output *term __attribute__ ((unused)),
		     struct holy_term_coordinate pos)
{
  if (pos.x >= virtual_screen.columns)
    pos.x = virtual_screen.columns - 1;

  if (pos.y >= virtual_screen.rows)
    pos.y = virtual_screen.rows - 1;

  /* Erase current cursor, if any.  */
  if (virtual_screen.cursor_state)
    draw_cursor (0);

  virtual_screen.cursor_x = pos.x;
  virtual_screen.cursor_y = pos.y;

  /* Draw cursor if visible.  */
  if (virtual_screen.cursor_state)
    draw_cursor (1);
}

static void
holy_virtual_screen_cls (struct holy_term_output *term __attribute__ ((unused)))
{
  holy_uint32_t i;

  for (i = 0; i < virtual_screen.columns * virtual_screen.rows; i++)
    clear_char (&(virtual_screen.text_buffer[i]));

  virtual_screen.cursor_x = virtual_screen.cursor_y = 0;
}

static void
holy_gfxterm_cls (struct holy_term_output *term)
{
  holy_video_color_t color;

  /* Clear virtual screen.  */
  holy_virtual_screen_cls (term);

  /* Clear text layer.  */
  holy_video_set_active_render_target (text_layer);
  color = virtual_screen.bg_color;
  holy_video_fill_rect (color, 0, 0,
                        virtual_screen.width, virtual_screen.height);
  holy_video_set_active_render_target (render_target);

  /* Mark virtual screen to be redrawn.  */
  dirty_region_add_virtualscreen ();

  holy_gfxterm_refresh (term);
}

static void
holy_virtual_screen_setcolorstate (struct holy_term_output *term __attribute__ ((unused)),
				   holy_term_color_state state)
{
  switch (state)
    {
    case holy_TERM_COLOR_STANDARD:
      virtual_screen.term_color = virtual_screen.standard_color_setting;
      break;

    case holy_TERM_COLOR_NORMAL:
      virtual_screen.term_color = holy_term_normal_color;
      break;

    case holy_TERM_COLOR_HIGHLIGHT:
      virtual_screen.term_color = holy_term_highlight_color;
      break;

    default:
      break;
    }

  /* Change color to virtual terminal.  */
  set_term_color (virtual_screen.term_color);
}

static void
holy_gfxterm_setcursor (struct holy_term_output *term __attribute__ ((unused)),
			int on)
{
  if (virtual_screen.cursor_state != on)
    {
      if (virtual_screen.cursor_state)
	draw_cursor (0);
      else
	draw_cursor (1);

      virtual_screen.cursor_state = on;
    }
}

static void
holy_gfxterm_refresh (struct holy_term_output *term __attribute__ ((unused)))
{
  real_scroll ();

  /* Redraw only changed regions.  */
  dirty_region_redraw ();

  holy_video_swap_buffers ();

  if (window.double_repaint)
    dirty_region_redraw ();
  dirty_region_reset ();
}

static struct holy_term_output holy_video_term =
  {
    .name = "gfxterm",
    .init = holy_gfxterm_term_init,
    .fini = holy_gfxterm_term_fini,
    .putchar = holy_gfxterm_putchar,
    .getcharwidth = holy_gfxterm_getcharwidth,
    .getwh = holy_virtual_screen_getwh,
    .getxy = holy_virtual_screen_getxy,
    .gotoxy = holy_gfxterm_gotoxy,
    .cls = holy_gfxterm_cls,
    .setcolorstate = holy_virtual_screen_setcolorstate,
    .setcursor = holy_gfxterm_setcursor,
    .refresh = holy_gfxterm_refresh,
    .fullscreen = holy_gfxterm_fullscreen,
    .flags = holy_TERM_CODE_TYPE_VISUAL_GLYPHS,
    .progress_update_divisor = holy_PROGRESS_SLOW,
    .next = 0
  };

void
holy_gfxterm_video_update_color (void)
{
  struct holy_video_render_target *old_target;

  holy_video_get_active_render_target (&old_target);
  holy_video_set_active_render_target (text_layer);
  virtual_screen.bg_color = holy_video_map_rgba_color (holy_gfxterm_background.default_bg_color);
  holy_video_set_active_render_target (old_target);
  virtual_screen.bg_color_display =
    holy_video_map_rgba_color (holy_gfxterm_background.default_bg_color);
}

void
holy_gfxterm_get_dimensions (unsigned *width, unsigned *height)
{
  *width = window.width;
  *height = window.height;
}

holy_MOD_INIT(gfxterm)
{
  holy_term_register_output ("gfxterm", &holy_video_term);
}

holy_MOD_FINI(gfxterm)
{
  holy_term_unregister_output (&holy_video_term);
}
