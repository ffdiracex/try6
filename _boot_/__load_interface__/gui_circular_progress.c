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
#include <holy/trig.h>

struct holy_gui_circular_progress
{
  struct holy_gui_progress progress;

  holy_gui_container_t parent;
  holy_video_rect_t bounds;
  char *id;
  int visible;
  int start;
  int end;
  int value;
  unsigned num_ticks;
  int start_angle;
  int ticks_disappear;
  char *theme_dir;
  int need_to_load_pixmaps;
  char *center_file;
  char *tick_file;
  struct holy_video_bitmap *center_bitmap;
  struct holy_video_bitmap *tick_bitmap;
};

typedef struct holy_gui_circular_progress *circular_progress_t;

static void
circprog_destroy (void *vself)
{
  circular_progress_t self = vself;
  holy_gfxmenu_timeout_unregister ((holy_gui_component_t) self);
  holy_free (self);
}

static const char *
circprog_get_id (void *vself)
{
  circular_progress_t self = vself;
  return self->id;
}

static int
circprog_is_instance (void *vself __attribute__((unused)), const char *type)
{
  return holy_strcmp (type, "component") == 0;
}

static struct holy_video_bitmap *
load_bitmap (const char *dir, const char *file)
{
  struct holy_video_bitmap *bitmap;
  char *abspath;

  /* Check arguments.  */
  if (! dir || ! file)
    return 0;

  /* Resolve to an absolute path.  */
  abspath = holy_resolve_relative_path (dir, file);
  if (! abspath)
    return 0;

  /* Load the image.  */
  holy_errno = holy_ERR_NONE;
  holy_video_bitmap_load (&bitmap, abspath);
  holy_errno = holy_ERR_NONE;

  holy_free (abspath);
  return bitmap;
}

static int
check_pixmaps (circular_progress_t self)
{
  if (self->need_to_load_pixmaps)
    {
      if (self->center_bitmap)
        holy_video_bitmap_destroy (self->center_bitmap);
      self->center_bitmap = load_bitmap (self->theme_dir, self->center_file);
      self->tick_bitmap = load_bitmap (self->theme_dir, self->tick_file);
      self->need_to_load_pixmaps = 0;
    }

  return (self->center_bitmap != 0 && self->tick_bitmap != 0);
}

static void
circprog_paint (void *vself, const holy_video_rect_t *region)
{
  circular_progress_t self = vself;

  if (! self->visible)
    return;

  if (!holy_video_have_common_points (region, &self->bounds))
    return;

  if (! check_pixmaps (self))
    return;

  holy_video_rect_t vpsave;
  holy_gui_set_viewport (&self->bounds, &vpsave);

  int width = self->bounds.width;
  int height = self->bounds.height;
  int center_width = holy_video_bitmap_get_width (self->center_bitmap);
  int center_height = holy_video_bitmap_get_height (self->center_bitmap);
  int tick_width = holy_video_bitmap_get_width (self->tick_bitmap);
  int tick_height = holy_video_bitmap_get_height (self->tick_bitmap);
  holy_video_blit_bitmap (self->center_bitmap, holy_VIDEO_BLIT_BLEND,
                          (width - center_width) / 2,
                          (height - center_height) / 2, 0, 0,
                          center_width, center_height);

  if (self->num_ticks)
    {
      int radius = holy_min (height, width) / 2 - holy_max (tick_height, tick_width) / 2 - 1;
      unsigned nticks;
      unsigned tick_begin;
      unsigned tick_end;
      if (self->end <= self->start
	  || self->value <= self->start)
	nticks = 0;
      else
	nticks = ((unsigned) (self->num_ticks
			      * (self->value - self->start)))
	  / ((unsigned) (self->end - self->start));
      /* Do ticks appear or disappear as the value approached the end?  */
      if (self->ticks_disappear)
	{
	  tick_begin = nticks;
	  tick_end = self->num_ticks;
	}
      else
	{
	  tick_begin = 0;
	  tick_end = nticks;
	}

      unsigned i;
      for (i = tick_begin; i < tick_end; i++)
	{
	  int x;
	  int y;
	  int angle;

	  /* Calculate the location of the tick.  */
	  angle = self->start_angle
	    + i * holy_TRIG_ANGLE_MAX / self->num_ticks;
	  x = width / 2 + (holy_cos (angle) * radius / holy_TRIG_FRACTION_SCALE);
	  y = height / 2 + (holy_sin (angle) * radius / holy_TRIG_FRACTION_SCALE);

	  /* Adjust (x,y) so the tick is centered.  */
	  x -= tick_width / 2;
	  y -= tick_height / 2;

	  /* Draw the tick.  */
	  holy_video_blit_bitmap (self->tick_bitmap, holy_VIDEO_BLIT_BLEND,
				  x, y, 0, 0, tick_width, tick_height);
	}
    }
  holy_gui_restore_viewport (&vpsave);
}

static void
circprog_set_parent (void *vself, holy_gui_container_t parent)
{
  circular_progress_t self = vself;
  self->parent = parent;
}

static holy_gui_container_t
circprog_get_parent (void *vself)
{
  circular_progress_t self = vself;
  return self->parent;
}

static void
circprog_set_bounds (void *vself, const holy_video_rect_t *bounds)
{
  circular_progress_t self = vself;
  self->bounds = *bounds;
}

static void
circprog_get_bounds (void *vself, holy_video_rect_t *bounds)
{
  circular_progress_t self = vself;
  *bounds = self->bounds;
}

static void
circprog_set_state (void *vself, int visible, int start,
		    int current, int end)
{
  circular_progress_t self = vself;  
  self->visible = visible;
  self->start = start;
  self->value = current;
  self->end = end;
}

static int
parse_angle (const char *value)
{
  char *ptr;
  int angle;

  angle = holy_strtol (value, &ptr, 10);
  if (holy_errno)
    return 0;
  while (holy_isspace (*ptr))
    ptr++;
  if (holy_strcmp (ptr, "deg") == 0
      /* Unicode symbol of degrees (a circle, U+b0). Put here in UTF-8 to
	 avoid potential problem with text file reesncoding  */
      || holy_strcmp (ptr, "\xc2\xb0") == 0)
    angle = holy_divide_round (angle * 64, 90);
  return angle;
}

static holy_err_t
circprog_set_property (void *vself, const char *name, const char *value)
{
  circular_progress_t self = vself;
  if (holy_strcmp (name, "num_ticks") == 0)
    {
      self->num_ticks = holy_strtoul (value, 0, 10);
    }
  else if (holy_strcmp (name, "start_angle") == 0)
    {
      self->start_angle = parse_angle (value);
    }
  else if (holy_strcmp (name, "ticks_disappear") == 0)
    {
      self->ticks_disappear = holy_strcmp (value, "false") != 0;
    }
  else if (holy_strcmp (name, "center_bitmap") == 0)
    {
      self->need_to_load_pixmaps = 1;
      holy_free (self->center_file);
      self->center_file = value ? holy_strdup (value) : 0;
    }
  else if (holy_strcmp (name, "tick_bitmap") == 0)
    {
      self->need_to_load_pixmaps = 1;
      holy_free (self->tick_file);
      self->tick_file = value ? holy_strdup (value) : 0;
    }
  else if (holy_strcmp (name, "theme_dir") == 0)
    {
      self->need_to_load_pixmaps = 1;
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
				       circprog_set_state);
    }
  return holy_errno;
}

static struct holy_gui_component_ops circprog_ops =
{
  .destroy = circprog_destroy,
  .get_id = circprog_get_id,
  .is_instance = circprog_is_instance,
  .paint = circprog_paint,
  .set_parent = circprog_set_parent,
  .get_parent = circprog_get_parent,
  .set_bounds = circprog_set_bounds,
  .get_bounds = circprog_get_bounds,
  .set_property = circprog_set_property
};

static struct holy_gui_progress_ops circprog_prog_ops =
  {
    .set_state = circprog_set_state
  };

holy_gui_component_t
holy_gui_circular_progress_new (void)
{
  circular_progress_t self;
  self = holy_zalloc (sizeof (*self));
  if (! self)
    return 0;
  self->progress.ops = &circprog_prog_ops;
  self->progress.component.ops = &circprog_ops;
  self->visible = 1;
  self->num_ticks = 64;
  self->start_angle = -64;

  return (holy_gui_component_t) self;
}
