/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/gui.h>
#include <holy/font.h>
#include <holy/gui_string_util.h>
#include <holy/i18n.h>
#include <holy/color.h>

static const char *align_options[] =
{
  "left",
  "center",
  "right",
  0
};

enum align_mode {
  align_left,
  align_center,
  align_right
};

struct holy_gui_label
{
  struct holy_gui_component comp;

  holy_gui_container_t parent;
  holy_video_rect_t bounds;
  char *id;
  int visible;
  char *text;
  char *template;
  holy_font_t font;
  holy_video_rgba_color_t color;
  int value;
  enum align_mode align;
};

typedef struct holy_gui_label *holy_gui_label_t;

static void
label_destroy (void *vself)
{
  holy_gui_label_t self = vself;
  holy_gfxmenu_timeout_unregister ((holy_gui_component_t) self);
  holy_free (self->text);
  holy_free (self->template);
  holy_free (self);
}

static const char *
label_get_id (void *vself)
{
  holy_gui_label_t self = vself;
  return self->id;
}

static int
label_is_instance (void *vself __attribute__((unused)), const char *type)
{
  return holy_strcmp (type, "component") == 0;
}

static void
label_paint (void *vself, const holy_video_rect_t *region)
{
  holy_gui_label_t self = vself;

  if (! self->visible)
    return;

  if (!holy_video_have_common_points (region, &self->bounds))
    return;

  /* Calculate the starting x coordinate.  */
  int left_x;
  if (self->align == align_left)
    left_x = 0;
  else if (self->align == align_center)
    left_x = (self->bounds.width
	      - holy_font_get_string_width (self->font, self->text)) / 2;
  else if (self->align == align_right)
    left_x = (self->bounds.width
              - holy_font_get_string_width (self->font, self->text));
  else
    return;   /* Invalid alignment.  */

  if (left_x < 0 || left_x > (int) self->bounds.width)
    left_x = 0;

  holy_video_rect_t vpsave;
  holy_gui_set_viewport (&self->bounds, &vpsave);
  holy_font_draw_string (self->text,
                         self->font,
                         holy_video_map_rgba_color (self->color),
                         left_x,
                         holy_font_get_ascent (self->font));
  holy_gui_restore_viewport (&vpsave);
}

static void
label_set_parent (void *vself, holy_gui_container_t parent)
{
  holy_gui_label_t self = vself;
  self->parent = parent;
}

static holy_gui_container_t
label_get_parent (void *vself)
{
  holy_gui_label_t self = vself;
  return self->parent;
}

static void
label_set_bounds (void *vself, const holy_video_rect_t *bounds)
{
  holy_gui_label_t self = vself;
  self->bounds = *bounds;
}

static void
label_get_bounds (void *vself, holy_video_rect_t *bounds)
{
  holy_gui_label_t self = vself;
  *bounds = self->bounds;
}

static void
label_get_minimal_size (void *vself, unsigned *width, unsigned *height)
{
  holy_gui_label_t self = vself;
  *width = holy_font_get_string_width (self->font, self->text);
  *height = (holy_font_get_ascent (self->font)
             + holy_font_get_descent (self->font));
}

#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static void
label_set_state (void *vself, int visible, int start __attribute__ ((unused)),
		 int current, int end __attribute__ ((unused)))
{
  holy_gui_label_t self = vself;
  self->value = -current;
  self->visible = visible;
  holy_free (self->text);
  self->text = holy_xasprintf (self->template ? : "%d", self->value);
}

static holy_err_t
label_set_property (void *vself, const char *name, const char *value)
{
  holy_gui_label_t self = vself;
  if (holy_strcmp (name, "text") == 0)
    {
      holy_free (self->text);
      holy_free (self->template);
      if (! value)
	{
	  self->template = NULL;
	  self->text = holy_strdup ("");
	}
      else
	{
	   if (holy_strcmp (value, "@KEYMAP_LONG@") == 0)
	    value = _("Press enter to boot the selected OS, "
	       "`e' to edit the commands before booting "
	       "or `c' for a command-line. ESC to return previous menu.");
           else if (holy_strcmp (value, "@KEYMAP_MIDDLE@") == 0)
	    value = _("Press enter to boot the selected OS, "
	       "`e' to edit the commands before booting "
	       "or `c' for a command-line.");
	   else if (holy_strcmp (value, "@KEYMAP_SHORT@") == 0)
	    value = _("enter: boot, `e': options, `c': cmd-line");
	   /* FIXME: Add more templates here if needed.  */
	  self->template = holy_strdup (value);
	  self->text = holy_xasprintf (value, self->value);
	}
    }
  else if (holy_strcmp (name, "font") == 0)
    {
      self->font = holy_font_get (value);
    }
  else if (holy_strcmp (name, "color") == 0)
    {
      holy_video_parse_color (value, &self->color);
    }
  else if (holy_strcmp (name, "align") == 0)
    {
      int i;
      for (i = 0; align_options[i]; i++)
        {
          if (holy_strcmp (align_options[i], value) == 0)
            {
              self->align = i;   /* Set the alignment mode.  */
              break;
            }
        }
    }
  else if (holy_strcmp (name, "visible") == 0)
    {
      self->visible = holy_strcmp (value, "false") != 0;
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
				       label_set_state);
    }
  return holy_ERR_NONE;
}

#pragma GCC diagnostic error "-Wformat-nonliteral"

static struct holy_gui_component_ops label_ops =
{
  .destroy = label_destroy,
  .get_id = label_get_id,
  .is_instance = label_is_instance,
  .paint = label_paint,
  .set_parent = label_set_parent,
  .get_parent = label_get_parent,
  .set_bounds = label_set_bounds,
  .get_bounds = label_get_bounds,
  .get_minimal_size = label_get_minimal_size,
  .set_property = label_set_property
};

holy_gui_component_t
holy_gui_label_new (void)
{
  holy_gui_label_t label;
  label = holy_zalloc (sizeof (*label));
  if (! label)
    return 0;
  label->comp.ops = &label_ops;
  label->visible = 1;
  label->text = holy_strdup ("");
  label->font = holy_font_get ("Unknown Regular 16");
  label->color.red = 0;
  label->color.green = 0;
  label->color.blue = 0;
  label->color.alpha = 255;
  label->align = align_left;
  return (holy_gui_component_t) label;
}
