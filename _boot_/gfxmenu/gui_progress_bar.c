/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/gui.h>
#include <holy/font.h>
#include <holy/gui_string_util.h>
#include <holy/gfxmenu_view.h>
#include <holy/gfxwidgets.h>
#include <holy/i18n.h>
#include <holy/color.h>

struct holy_gui_progress_bar
{
  struct holy_gui_progress progress;

  holy_gui_container_t parent;
  holy_video_rect_t bounds;
  char *id;
  int visible;
  int start;
  int end;
  int value;
  char *template;
  holy_font_t font;
  holy_video_rgba_color_t text_color;
  holy_video_rgba_color_t border_color;
  holy_video_rgba_color_t bg_color;
  holy_video_rgba_color_t fg_color;

  char *theme_dir;
  int need_to_recreate_pixmaps;
  int pixmapbar_available;
  char *bar_pattern;
  char *highlight_pattern;
  holy_gfxmenu_box_t bar_box;
  holy_gfxmenu_box_t highlight_box;
  int highlight_overlay;
};

typedef struct holy_gui_progress_bar *holy_gui_progress_bar_t;

static void
progress_bar_destroy (void *vself)
{
  holy_gui_progress_bar_t self = vself;
  holy_free (self->theme_dir);
  holy_free (self->template);
  holy_free (self->id);
  holy_gfxmenu_timeout_unregister ((holy_gui_component_t) self);
  holy_free (self);
}

static const char *
progress_bar_get_id (void *vself)
{
  holy_gui_progress_bar_t self = vself;
  return self->id;
}

static int
progress_bar_is_instance (void *vself __attribute__((unused)), const char *type)
{
  return holy_strcmp (type, "component") == 0;
}

static int
check_pixmaps (holy_gui_progress_bar_t self)
{
  if (!self->pixmapbar_available)
    return 0;
  if (self->need_to_recreate_pixmaps)
    {
      holy_gui_recreate_box (&self->bar_box,
                             self->bar_pattern,
                             self->theme_dir);

      holy_gui_recreate_box (&self->highlight_box,
                             self->highlight_pattern,
                             self->theme_dir);

      self->need_to_recreate_pixmaps = 0;
    }

  return (self->bar_box != 0 && self->highlight_box != 0);
}

static void
draw_filled_rect_bar (holy_gui_progress_bar_t self)
{
  /* Set the progress bar's frame.  */
  holy_video_rect_t f;
  f.x = 1;
  f.y = 1;
  f.width = self->bounds.width - 2;
  f.height = self->bounds.height - 2;

  /* Border.  */
  holy_video_fill_rect (holy_video_map_rgba_color (self->border_color),
                        f.x - 1, f.y - 1,
                        f.width + 2, f.height + 2);

  /* Bar background.  */
  unsigned barwidth;

  if (self->end <= self->start
      || self->value <= self->start)
    barwidth = 0;
  else
    barwidth = (f.width
		* (self->value - self->start)
		/ (self->end - self->start));
  holy_video_fill_rect (holy_video_map_rgba_color (self->bg_color),
                        f.x + barwidth, f.y,
                        f.width - barwidth, f.height);

  /* Bar foreground.  */
  holy_video_fill_rect (holy_video_map_rgba_color (self->fg_color),
                        f.x, f.y,
                        barwidth, f.height);
}

static void
draw_pixmap_bar (holy_gui_progress_bar_t self)
{
  holy_gfxmenu_box_t bar = self->bar_box;
  holy_gfxmenu_box_t hl = self->highlight_box;
  int w = self->bounds.width;
  int h = self->bounds.height;
  int bar_l_pad = bar->get_left_pad (bar);
  int bar_r_pad = bar->get_right_pad (bar);
  int bar_t_pad = bar->get_top_pad (bar);
  int bar_b_pad = bar->get_bottom_pad (bar);
  int bar_h_pad = bar_l_pad + bar_r_pad;
  int bar_v_pad = bar_t_pad + bar_b_pad;
  int hl_l_pad = hl->get_left_pad (hl);
  int hl_r_pad = hl->get_right_pad (hl);
  int hl_t_pad = hl->get_top_pad (hl);
  int hl_b_pad = hl->get_bottom_pad (hl);
  int hl_h_pad = hl_l_pad + hl_r_pad;
  int hl_v_pad = hl_t_pad + hl_b_pad;
  int tracklen = w - bar_h_pad;
  int trackheight = h - bar_v_pad;
  int barwidth;
  int hlheight = trackheight;
  int hlx = bar_l_pad;
  int hly = bar_t_pad;

  bar->set_content_size (bar, tracklen, trackheight);
  bar->draw (bar, 0, 0);

  if (self->highlight_overlay)
    {
      tracklen += hl_h_pad;
      hlx -= hl_l_pad;
      hly -= hl_t_pad;
    }
  else
    hlheight -= hl_v_pad;

  if (self->value <= self->start
      || self->end <= self->start)
    barwidth = 0;
  else
    barwidth = ((unsigned) (tracklen * (self->value - self->start))
		/ ((unsigned) (self->end - self->start)));

  if (barwidth >= hl_h_pad)
    {
      hl->set_content_size (hl, barwidth - hl_h_pad, hlheight);
      hl->draw (hl, hlx, hly);
    }
}

#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static void
draw_text (holy_gui_progress_bar_t self)
{
  if (self->template)
    {
      holy_font_t font = self->font;
      holy_video_color_t text_color =
	holy_video_map_rgba_color (self->text_color);
      int width = self->bounds.width;
      int height = self->bounds.height;
      char *text;
      text = holy_xasprintf (self->template,
			     self->value > 0 ? self->value : -self->value);
      if (!text)
	{
	  holy_print_error ();
	  holy_errno = holy_ERR_NONE;
	  return;
	}
      /* Center the text. */
      int text_width = holy_font_get_string_width (font, text);
      int x = (width - text_width) / 2;
      int y = ((height - holy_font_get_descent (font)) / 2
               + holy_font_get_ascent (font) / 2);
      holy_font_draw_string (text, font, text_color, x, y);
      holy_free (text);
    }
}

#pragma GCC diagnostic error "-Wformat-nonliteral"

static void
progress_bar_paint (void *vself, const holy_video_rect_t *region)
{
  holy_gui_progress_bar_t self = vself;
  holy_video_rect_t vpsave;

  if (! self->visible)
    return;
  if (!holy_video_have_common_points (region, &self->bounds))
    return;

  if (self->end == self->start)
    return;

  holy_gui_set_viewport (&self->bounds, &vpsave);

  if (check_pixmaps (self))
    draw_pixmap_bar (self);
  else
    draw_filled_rect_bar (self);

  draw_text (self);

  holy_gui_restore_viewport (&vpsave);
}

static void
progress_bar_set_parent (void *vself, holy_gui_container_t parent)
{
  holy_gui_progress_bar_t self = vself;
  self->parent = parent;
}

static holy_gui_container_t
progress_bar_get_parent (void *vself)
{
  holy_gui_progress_bar_t self = vself;
  return self->parent;
}

static void
progress_bar_set_bounds (void *vself, const holy_video_rect_t *bounds)
{
  holy_gui_progress_bar_t self = vself;
  self->bounds = *bounds;
}

static void
progress_bar_get_bounds (void *vself, holy_video_rect_t *bounds)
{
  holy_gui_progress_bar_t self = vself;
  *bounds = self->bounds;
}

static void
progress_bar_get_minimal_size (void *vself,
			       unsigned *width, unsigned *height)
{
  unsigned min_width = 0;
  unsigned min_height = 0;
  holy_gui_progress_bar_t self = vself;

  if (self->template)
    {
      min_width = holy_font_get_string_width (self->font, self->template);
      min_width += holy_font_get_string_width (self->font, "XXXXXXXXXX");
      min_height = holy_font_get_descent (self->font)
                   + holy_font_get_ascent (self->font);
    }
  if (check_pixmaps (self))
    {
      holy_gfxmenu_box_t bar = self->bar_box;
      holy_gfxmenu_box_t hl = self->highlight_box;
      min_width += bar->get_left_pad (bar) + bar->get_right_pad (bar);
      min_height += bar->get_top_pad (bar) + bar->get_bottom_pad (bar);
      if (!self->highlight_overlay)
        {
          min_width += hl->get_left_pad (hl) + hl->get_right_pad (hl);
          min_height += hl->get_top_pad (hl) + hl->get_bottom_pad (hl);
        }
    }
  else
    {
      min_height += 2;
      min_width += 2;
    }
  *width = 200;
  if (*width < min_width)
    *width = min_width;
  *height = 28;
  if (*height < min_height)
    *height = min_height;
}

static void
progress_bar_set_state (void *vself, int visible, int start,
			int current, int end)
{
  holy_gui_progress_bar_t self = vself;
  self->visible = visible;
  self->start = start;
  self->value = current;
  self->end = end;
}

static holy_err_t
progress_bar_set_property (void *vself, const char *name, const char *value)
{
  holy_gui_progress_bar_t self = vself;
  if (holy_strcmp (name, "text") == 0)
    {
      holy_free (self->template);
      if (holy_strcmp (value, "@TIMEOUT_NOTIFICATION_LONG@") == 0)
	value 
	  = _("The highlighted entry will be executed automatically in %ds.");
      else if (holy_strcmp (value, "@TIMEOUT_NOTIFICATION_MIDDLE@") == 0)
	/* TRANSLATORS:  's' stands for seconds.
	   It's a standalone timeout notification.
	   Please use the short form in your language.  */
	value = _("%ds remaining.");
      else if (holy_strcmp (value, "@TIMEOUT_NOTIFICATION_SHORT@") == 0)
	/* TRANSLATORS:  's' stands for seconds.
	   It's a standalone timeout notification.
	   Please use the shortest form available in you language.  */
	value = _("%ds");

      self->template = holy_strdup (value);
    }
  else if (holy_strcmp (name, "font") == 0)
    {
      self->font = holy_font_get (value);
    }
  else if (holy_strcmp (name, "text_color") == 0)
    {
      holy_video_parse_color (value, &self->text_color);
    }
  else if (holy_strcmp (name, "border_color") == 0)
    {
       holy_video_parse_color (value, &self->border_color);
    }
  else if (holy_strcmp (name, "bg_color") == 0)
    {
       holy_video_parse_color (value, &self->bg_color);
    }
  else if (holy_strcmp (name, "fg_color") == 0)
    {
      holy_video_parse_color (value, &self->fg_color);
    }
  else if (holy_strcmp (name, "bar_style") == 0)
    {
      self->need_to_recreate_pixmaps = 1;
      self->pixmapbar_available = 1;
      holy_free (self->bar_pattern);
      self->bar_pattern = value ? holy_strdup (value) : 0;
    }
  else if (holy_strcmp (name, "highlight_style") == 0)
    {
      self->need_to_recreate_pixmaps = 1;
      self->pixmapbar_available = 1;
      holy_free (self->highlight_pattern);
      self->highlight_pattern = value ? holy_strdup (value) : 0;
    }
  else if (holy_strcmp (name, "highlight_overlay") == 0)
    {
      self->highlight_overlay = holy_strcmp (value, "true") == 0;
    }
  else if (holy_strcmp (name, "theme_dir") == 0)
    {
      self->need_to_recreate_pixmaps = 1;
      holy_free (self->theme_dir);
      self->theme_dir = value ? holy_strdup (value) : 0;
    }
  else if (holy_strcmp (name, "id") == 0)
    {
      holy_gfxmenu_timeout_unregister ((holy_gui_component_t) self);
      holy_free (self->id);
      if (value)
        self->id = holy_strdup (value);
      else
        self->id = 0;
      if (self->id && holy_strcmp (self->id, holy_GFXMENU_TIMEOUT_COMPONENT_ID)
	  == 0)
	holy_gfxmenu_timeout_register ((holy_gui_component_t) self,
				       progress_bar_set_state);
    }
  return holy_errno;
}

static struct holy_gui_component_ops progress_bar_ops =
{
  .destroy = progress_bar_destroy,
  .get_id = progress_bar_get_id,
  .is_instance = progress_bar_is_instance,
  .paint = progress_bar_paint,
  .set_parent = progress_bar_set_parent,
  .get_parent = progress_bar_get_parent,
  .set_bounds = progress_bar_set_bounds,
  .get_bounds = progress_bar_get_bounds,
  .get_minimal_size = progress_bar_get_minimal_size,
  .set_property = progress_bar_set_property
};

static struct holy_gui_progress_ops progress_bar_pb_ops =
  {
    .set_state = progress_bar_set_state
  };

holy_gui_component_t
holy_gui_progress_bar_new (void)
{
  holy_gui_progress_bar_t self;
  self = holy_zalloc (sizeof (*self));
  if (! self)
    return 0;

  self->progress.ops = &progress_bar_pb_ops;
  self->progress.component.ops = &progress_bar_ops;
  self->visible = 1;
  self->font = holy_font_get ("Unknown Regular 16");
  holy_video_rgba_color_t black = { .red = 0, .green = 0, .blue = 0, .alpha = 255 };
  holy_video_rgba_color_t gray = { .red = 128, .green = 128, .blue = 128, .alpha = 255 };
  holy_video_rgba_color_t lightgray = { .red = 200, .green = 200, .blue = 200, .alpha = 255 };
  self->text_color = black;
  self->border_color = black;
  self->bg_color = gray;
  self->fg_color = lightgray;
  self->highlight_overlay = 0;

  return (holy_gui_component_t) self;
}
