/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/file.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/video.h>
#include <holy/gui_string_util.h>
#include <holy/bitmap.h>
#include <holy/bitmap_scale.h>
#include <holy/gfxwidgets.h>
#include <holy/gfxmenu_view.h>
#include <holy/gui.h>
#include <holy/color.h>

static holy_err_t
parse_proportional_spec (const char *value, signed *abs, holy_fixed_signed_t *prop);

/* Construct a new box widget using ABSPATTERN to find the pixmap files for
   it, storing the new box instance at *BOXPTR.
   PATTERN should be of the form: "(hd0,0)/somewhere/style*.png".
   The '*' then gets substituted with the various pixmap names that the
   box uses.  */
static holy_err_t
recreate_box_absolute (holy_gfxmenu_box_t *boxptr, const char *abspattern)
{
  char *prefix;
  char *suffix;
  char *star;
  holy_gfxmenu_box_t box;

  star = holy_strchr (abspattern, '*');
  if (! star)
    return holy_error (holy_ERR_BAD_ARGUMENT,
                       "missing `*' in box pixmap pattern `%s'", abspattern);

  /* Prefix:  Get the part before the '*'.  */
  prefix = holy_malloc (star - abspattern + 1);
  if (! prefix)
    return holy_errno;

  holy_memcpy (prefix, abspattern, star - abspattern);
  prefix[star - abspattern] = '\0';

  /* Suffix:  Everything after the '*' is the suffix.  */
  suffix = star + 1;

  box = holy_gfxmenu_create_box (prefix, suffix);
  holy_free (prefix);
  if (! box)
    return holy_errno;

  if (*boxptr)
    (*boxptr)->destroy (*boxptr);
  *boxptr = box;
  return holy_errno;
}


/* Construct a new box widget using PATTERN to find the pixmap files for it,
   storing the new widget at *BOXPTR.  PATTERN should be of the form:
   "somewhere/style*.png".  The '*' then gets substituted with the various
   pixmap names that the widget uses.

   Important!  The value of *BOXPTR must be initialized!  It must either
   (1) Be 0 (a NULL pointer), or
   (2) Be a pointer to a valid 'holy_gfxmenu_box_t' instance.
   In this case, the previous instance is destroyed.  */
holy_err_t
holy_gui_recreate_box (holy_gfxmenu_box_t *boxptr,
                       const char *pattern, const char *theme_dir)
{
  char *abspattern;

  /* Check arguments.  */
  if (! pattern)
    {
      /* If no pixmap pattern is given, then just create an empty box.  */
      if (*boxptr)
        (*boxptr)->destroy (*boxptr);
      *boxptr = holy_gfxmenu_create_box (0, 0);
      return holy_errno;
    }

  if (! theme_dir)
    return holy_error (holy_ERR_BAD_ARGUMENT,
                       "styled box missing theme directory");

  /* Resolve to an absolute path.  */
  abspattern = holy_resolve_relative_path (theme_dir, pattern);
  if (! abspattern)
    return holy_errno;

  /* Create the box.  */
  recreate_box_absolute (boxptr, abspattern);
  holy_free (abspattern);
  return holy_errno;
}

static holy_err_t
theme_get_unsigned_int_from_proportional (const char *value,
                                          unsigned absolute_value,
                                          unsigned int *parsed_value)
{
  holy_err_t err;
  holy_fixed_signed_t frac;
  signed pixels;
  err = parse_proportional_spec (value, &pixels, &frac);
  if (err != holy_ERR_NONE)
    return err;
  int result = holy_fixed_sfs_multiply (absolute_value, frac) + pixels;
  if (result < 0)
    result = 0;
  *parsed_value = result;
  return holy_ERR_NONE;
}

/* Set the specified property NAME on the view to the given string VALUE.
   The caller is responsible for the lifetimes of NAME and VALUE.  */
static holy_err_t
theme_set_string (holy_gfxmenu_view_t view,
                  const char *name,
                  const char *value,
                  const char *theme_dir,
                  const char *filename,
                  int line_num,
                  int col_num)
{
  if (! holy_strcmp ("title-font", name))
    view->title_font = holy_font_get (value);
  else if (! holy_strcmp ("message-font", name))
    view->message_font = holy_font_get (value);
  else if (! holy_strcmp ("terminal-font", name))
    {
      holy_free (view->terminal_font_name);
      view->terminal_font_name = holy_strdup (value);
      if (! view->terminal_font_name)
        return holy_errno;
    }
  else if (! holy_strcmp ("title-color", name))
    holy_video_parse_color (value, &view->title_color);
  else if (! holy_strcmp ("message-color", name))
    holy_video_parse_color (value, &view->message_color);
  else if (! holy_strcmp ("message-bg-color", name))
    holy_video_parse_color (value, &view->message_bg_color);
  else if (! holy_strcmp ("desktop-image", name))
    {
      struct holy_video_bitmap *raw_bitmap;
      char *path;
      path = holy_resolve_relative_path (theme_dir, value);
      if (! path)
        return holy_errno;
      if (holy_video_bitmap_load (&raw_bitmap, path) != holy_ERR_NONE)
        {
          holy_free (path);
          return holy_errno;
        }
      holy_free(path);
      holy_video_bitmap_destroy (view->raw_desktop_image);
      view->raw_desktop_image = raw_bitmap;
    }
  else if (! holy_strcmp ("desktop-image-scale-method", name))
    {
      if (! value || ! holy_strcmp ("stretch", value))
        view->desktop_image_scale_method =
            holy_VIDEO_BITMAP_SELECTION_METHOD_STRETCH;
      else if (! holy_strcmp ("crop", value))
        view->desktop_image_scale_method =
            holy_VIDEO_BITMAP_SELECTION_METHOD_CROP;
      else if (! holy_strcmp ("padding", value))
        view->desktop_image_scale_method =
            holy_VIDEO_BITMAP_SELECTION_METHOD_PADDING;
      else if (! holy_strcmp ("fitwidth", value))
        view->desktop_image_scale_method =
            holy_VIDEO_BITMAP_SELECTION_METHOD_FITWIDTH;
      else if (! holy_strcmp ("fitheight", value))
        view->desktop_image_scale_method =
            holy_VIDEO_BITMAP_SELECTION_METHOD_FITHEIGHT;
      else
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           "Unsupported scale method: %s",
                           value);
    }
  else if (! holy_strcmp ("desktop-image-h-align", name))
    {
      if (! holy_strcmp ("left", value))
        view->desktop_image_h_align = holy_VIDEO_BITMAP_H_ALIGN_LEFT;
      else if (! holy_strcmp ("center", value))
        view->desktop_image_h_align = holy_VIDEO_BITMAP_H_ALIGN_CENTER;
      else if (! holy_strcmp ("right", value))
        view->desktop_image_h_align = holy_VIDEO_BITMAP_H_ALIGN_RIGHT;
      else
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           "Unsupported horizontal align method: %s",
                           value);
    }
  else if (! holy_strcmp ("desktop-image-v-align", name))
    {
      if (! holy_strcmp ("top", value))
        view->desktop_image_v_align = holy_VIDEO_BITMAP_V_ALIGN_TOP;
      else if (! holy_strcmp ("center", value))
        view->desktop_image_v_align = holy_VIDEO_BITMAP_V_ALIGN_CENTER;
      else if (! holy_strcmp ("bottom", value))
        view->desktop_image_v_align = holy_VIDEO_BITMAP_V_ALIGN_BOTTOM;
      else
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           "Unsupported vertical align method: %s",
                           value);
    }
  else if (! holy_strcmp ("desktop-color", name))
     holy_video_parse_color (value, &view->desktop_color);
  else if (! holy_strcmp ("terminal-box", name))
    {
        holy_err_t err;
        err = holy_gui_recreate_box (&view->terminal_box, value, theme_dir);
        if (err != holy_ERR_NONE)
          return err;
    }
  else if (! holy_strcmp ("terminal-border", name))
    {
      view->terminal_border = holy_strtoul (value, 0, 10);
      if (holy_errno)
        return holy_errno;
    }
  else if (! holy_strcmp ("terminal-left", name))
    {
      unsigned int tmp;
      int err = theme_get_unsigned_int_from_proportional (value,
                                                          view->screen.width,
                                                          &tmp);
      if (err != holy_ERR_NONE)
        return err;
      view->terminal_rect.x = tmp;
    }
  else if (! holy_strcmp ("terminal-top", name))
    {
      unsigned int tmp;
      int err = theme_get_unsigned_int_from_proportional (value,
                                                          view->screen.height,
                                                          &tmp);
      if (err != holy_ERR_NONE)
        return err;
      view->terminal_rect.y = tmp;
    }
  else if (! holy_strcmp ("terminal-width", name))
    {
      unsigned int tmp;
      int err = theme_get_unsigned_int_from_proportional (value,
                                                          view->screen.width,
                                                          &tmp);
      if (err != holy_ERR_NONE)
        return err;
      view->terminal_rect.width = tmp;
    }
  else if (! holy_strcmp ("terminal-height", name))
    {
      unsigned int tmp;
      int err = theme_get_unsigned_int_from_proportional (value,
                                                          view->screen.height,
                                                          &tmp);
      if (err != holy_ERR_NONE)
        return err;
      view->terminal_rect.height = tmp;
    }
  else if (! holy_strcmp ("title-text", name))
    {
      holy_free (view->title_text);
      view->title_text = holy_strdup (value);
      if (! view->title_text)
        return holy_errno;
    }
  else
    {
      return holy_error (holy_ERR_BAD_ARGUMENT,
                         "%s:%d:%d unknown property `%s'",
                         filename, line_num, col_num, name);
    }
  return holy_errno;
}

struct parsebuf
{
  char *buf;
  int pos;
  int len;
  int line_num;
  int col_num;
  const char *filename;
  char *theme_dir;
  holy_gfxmenu_view_t view;
};

static int
has_more (struct parsebuf *p)
{
  return p->pos < p->len;
}

static int
read_char (struct parsebuf *p)
{
  if (has_more (p))
    {
      char c;
      c = p->buf[p->pos++];
      if (c == '\n')
        {
          p->line_num++;
          p->col_num = 1;
        }
      else
        {
          p->col_num++;
        }
      return c;
    }
  else
    return -1;
}

static int
peek_char (struct parsebuf *p)
{
  if (has_more (p))
    return p->buf[p->pos];
  else
    return -1;
}

static int
is_whitespace (char c)
{
  return (c == ' '
          || c == '\t'
          || c == '\r'
          || c == '\n'
          || c == '\f');
}

static void
skip_whitespace (struct parsebuf *p)
{
  while (has_more (p) && is_whitespace(peek_char (p)))
    read_char (p);
}

static void
advance_to_next_line (struct parsebuf *p)
{
  int c;

  /* Eat characters up to the newline.  */
  do
    {
      c = read_char (p);
    }
  while (c != -1 && c != '\n');
}

static int
is_identifier_char (int c)
{
  return (c != -1
          && (holy_isalpha(c)
              || holy_isdigit(c)
              || c == '_'
              || c == '-'));
}

static char *
read_identifier (struct parsebuf *p)
{
  /* Index of the first character of the identifier in p->buf.  */
  int start;
  /* Next index after the last character of the identifer in p->buf.  */
  int end;

  skip_whitespace (p);

  /* Capture the start of the identifier.  */
  start = p->pos;

  /* Scan for the end.  */
  while (is_identifier_char (peek_char (p)))
    read_char (p);
  end = p->pos;

  if (end - start < 1)
    return 0;

  return holy_new_substring (p->buf, start, end);
}

static char *
read_expression (struct parsebuf *p)
{
  int start;
  int end;

  skip_whitespace (p);
  if (peek_char (p) == '"')
    {
      /* Read as a quoted string.  
         The quotation marks are not included in the expression value.  */
      /* Skip opening quotation mark.  */
      read_char (p);
      start = p->pos;
      while (has_more (p) && peek_char (p) != '"')
        read_char (p);
      end = p->pos;
      /* Skip the terminating quotation mark.  */
      read_char (p);
    }
  else if (peek_char (p) == '(')
    {
      /* Read as a parenthesized string -- for tuples/coordinates.  */
      /* The parentheses are included in the expression value.  */
      int c;

      start = p->pos;
      do
        {
          c = read_char (p);
        }
      while (c != -1 && c != ')');
      end = p->pos;
    }
  else if (has_more (p))
    {
      /* Read as a single word -- for numeric values or words without
         whitespace.  */
      start = p->pos;
      while (has_more (p) && ! is_whitespace (peek_char (p)))
        read_char (p);
      end = p->pos;
    }
  else
    {
      /* The end of the theme file has been reached.  */
      holy_error (holy_ERR_IO, "%s:%d:%d expression expected in theme file",
                  p->filename, p->line_num, p->col_num);
      return 0;
    }

  return holy_new_substring (p->buf, start, end);
}

static holy_err_t
parse_proportional_spec (const char *value, signed *abs, holy_fixed_signed_t *prop)
{
  signed num;
  const char *ptr;
  int sig = 0;
  *abs = 0;
  *prop = 0;
  ptr = value;
  while (*ptr)
    {
      sig = 0;

      while (*ptr == '-' || *ptr == '+')
	{
	  if (*ptr == '-')
	    sig = !sig;
	  ptr++;
	}

      num = holy_strtoul (ptr, (char **) &ptr, 0);
      if (holy_errno)
	return holy_errno;
      if (sig)
	num = -num;
      if (*ptr == '%')
	{
	  *prop += holy_fixed_fsf_divide (holy_signed_to_fixed (num), 100);
	  ptr++;
	}
      else
	*abs += num;
    }
  return holy_ERR_NONE;
}


/* Read a GUI object specification from the theme file.
   Any components created will be added to the GUI container PARENT.  */
static holy_err_t
read_object (struct parsebuf *p, holy_gui_container_t parent)
{
  holy_video_rect_t bounds;

  char *name;
  name = read_identifier (p);
  if (! name)
    goto cleanup;

  holy_gui_component_t component = 0;
  if (holy_strcmp (name, "label") == 0)
    {
      component = holy_gui_label_new ();
    }
  else if (holy_strcmp (name, "image") == 0)
    {
      component = holy_gui_image_new ();
    }
  else if (holy_strcmp (name, "vbox") == 0)
    {
      component = (holy_gui_component_t) holy_gui_vbox_new ();
    }
  else if (holy_strcmp (name, "hbox") == 0)
    {
      component = (holy_gui_component_t) holy_gui_hbox_new ();
    }
  else if (holy_strcmp (name, "canvas") == 0)
    {
      component = (holy_gui_component_t) holy_gui_canvas_new ();
    }
  else if (holy_strcmp (name, "progress_bar") == 0)
    {
      component = holy_gui_progress_bar_new ();
    }
  else if (holy_strcmp (name, "circular_progress") == 0)
    {
      component = holy_gui_circular_progress_new ();
    }
  else if (holy_strcmp (name, "boot_menu") == 0)
    {
      component = holy_gui_list_new ();
    }
  else
    {
      /* Unknown type.  */
      holy_error (holy_ERR_IO, "%s:%d:%d unknown object type `%s'",
                  p->filename, p->line_num, p->col_num, name);
      goto cleanup;
    }

  if (! component)
    goto cleanup;

  /* Inform the component about the theme so it can find its resources.  */
  component->ops->set_property (component, "theme_dir", p->theme_dir);
  component->ops->set_property (component, "theme_path", p->filename);

  /* Add the component as a child of PARENT.  */
  bounds.x = 0;
  bounds.y = 0;
  bounds.width = -1;
  bounds.height = -1;
  component->ops->set_bounds (component, &bounds);
  parent->ops->add (parent, component);

  skip_whitespace (p);
  if (read_char (p) != '{')
    {
      holy_error (holy_ERR_IO,
                  "%s:%d:%d expected `{' after object type name `%s'",
                  p->filename, p->line_num, p->col_num, name);
      goto cleanup;
    }

  while (has_more (p))
    {
      skip_whitespace (p);

      /* Check whether the end has been encountered.  */
      if (peek_char (p) == '}')
        {
          /* Skip the closing brace.  */
          read_char (p);
          break;
        }

      if (peek_char (p) == '#')
        {
          /* Skip comments.  */
          advance_to_next_line (p);
          continue;
        }

      if (peek_char (p) == '+')
        {
          /* Skip the '+'.  */
          read_char (p);

          /* Check whether this component is a container.  */
          if (component->ops->is_instance (component, "container"))
            {
              /* Read the sub-object recursively and add it as a child.  */
              if (read_object (p, (holy_gui_container_t) component) != 0)
                goto cleanup;
              /* After reading the sub-object, resume parsing, expecting
                 another property assignment or sub-object definition.  */
              continue;
            }
          else
            {
              holy_error (holy_ERR_IO,
                          "%s:%d:%d attempted to add object to non-container",
                          p->filename, p->line_num, p->col_num);
              goto cleanup;
            }
        }

      char *property;
      property = read_identifier (p);
      if (! property)
        {
          holy_error (holy_ERR_IO, "%s:%d:%d identifier expected in theme file",
                      p->filename, p->line_num, p->col_num);
          goto cleanup;
        }

      skip_whitespace (p);
      if (read_char (p) != '=')
        {
          holy_error (holy_ERR_IO,
                      "%s:%d:%d expected `=' after property name `%s'",
                      p->filename, p->line_num, p->col_num, property);
          holy_free (property);
          goto cleanup;
        }
      skip_whitespace (p);

      char *value;
      value = read_expression (p);
      if (! value)
        {
          holy_free (property);
          goto cleanup;
        }

      /* Handle the property value.  */
      if (holy_strcmp (property, "left") == 0)
	parse_proportional_spec (value, &component->x, &component->xfrac);
      else if (holy_strcmp (property, "top") == 0)
	parse_proportional_spec (value, &component->y, &component->yfrac);
      else if (holy_strcmp (property, "width") == 0)
	parse_proportional_spec (value, &component->w, &component->wfrac);
      else if (holy_strcmp (property, "height") == 0)
	parse_proportional_spec (value, &component->h, &component->hfrac);
      else
	/* General property handling.  */
	component->ops->set_property (component, property, value);

      holy_free (value);
      holy_free (property);
      if (holy_errno != holy_ERR_NONE)
        goto cleanup;
    }

cleanup:
  holy_free (name);
  return holy_errno;
}

static holy_err_t
read_property (struct parsebuf *p)
{
  char *name;

  /* Read the property name.  */
  name = read_identifier (p);
  if (! name)
    {
      advance_to_next_line (p);
      return holy_errno;
    }

  /* Skip whitespace before separator.  */
  skip_whitespace (p);

  /* Read separator.  */
  if (read_char (p) != ':')
    {
      holy_error (holy_ERR_IO,
                  "%s:%d:%d missing separator after property name `%s'",
                  p->filename, p->line_num, p->col_num, name);
      goto done;
    }

  /* Skip whitespace after separator.  */
  skip_whitespace (p);

  /* Get the value based on its type.  */
  if (peek_char (p) == '"')
    {
      /* String value (e.g., '"My string"').  */
      char *value = read_expression (p);
      if (! value)
        {
          holy_error (holy_ERR_IO, "%s:%d:%d missing property value",
                      p->filename, p->line_num, p->col_num);
          goto done;
        }
      /* If theme_set_string results in an error, holy_errno will be returned
         below.  */
      theme_set_string (p->view, name, value, p->theme_dir,
                        p->filename, p->line_num, p->col_num);
      holy_free (value);
    }
  else
    {
      holy_error (holy_ERR_IO,
                  "%s:%d:%d property value invalid; "
                  "enclose literal values in quotes (\")",
                  p->filename, p->line_num, p->col_num);
      goto done;
    }

done:
  holy_free (name);
  return holy_errno;
}

/* Set properties on the view based on settings from the specified
   theme file.  */
holy_err_t
holy_gfxmenu_view_load_theme (holy_gfxmenu_view_t view, const char *theme_path)
{
  holy_file_t file;
  struct parsebuf p;

  p.view = view;
  p.theme_dir = holy_get_dirname (theme_path);

  file = holy_file_open (theme_path);
  if (! file)
    {
      holy_free (p.theme_dir);
      return holy_errno;
    }

  p.len = holy_file_size (file);
  p.buf = holy_malloc (p.len);
  p.pos = 0;
  p.line_num = 1;
  p.col_num = 1;
  p.filename = theme_path;
  if (! p.buf)
    {
      holy_file_close (file);
      holy_free (p.theme_dir);
      return holy_errno;
    }
  if (holy_file_read (file, p.buf, p.len) != p.len)
    {
      holy_free (p.buf);
      holy_file_close (file);
      holy_free (p.theme_dir);
      return holy_errno;
    }

  if (view->canvas)
    view->canvas->component.ops->destroy (view->canvas);

  view->canvas = holy_gui_canvas_new ();
  if (!view->canvas)
    goto fail;
  ((holy_gui_component_t) view->canvas)
    ->ops->set_bounds ((holy_gui_component_t) view->canvas,
                       &view->screen);

  while (has_more (&p))
    {
      /* Skip comments (lines beginning with #).  */
      if (peek_char (&p) == '#')
        {
          advance_to_next_line (&p);
          continue;
        }

      /* Find the first non-whitespace character.  */
      skip_whitespace (&p);

      /* Handle the content.  */
      if (peek_char (&p) == '+')
        {
          /* Skip the '+'.  */
          read_char (&p);
          read_object (&p, view->canvas);
        }
      else
        {
          read_property (&p);
        }

      if (holy_errno != holy_ERR_NONE)
        goto fail;
    }

  /* Set the new theme path.  */
  holy_free (view->theme_path);
  view->theme_path = holy_strdup (theme_path);
  goto cleanup;

fail:
  if (view->canvas)
    {
      view->canvas->component.ops->destroy (view->canvas);
      view->canvas = 0;
    }

cleanup:
  holy_free (p.buf);
  holy_file_close (file);
  holy_free (p.theme_dir);
  return holy_errno;
}
