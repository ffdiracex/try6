/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/gui.h>
#include <holy/gui_string_util.h>
#include <holy/bitmap.h>
#include <holy/bitmap_scale.h>

struct holy_gui_image
{
  struct holy_gui_component component;

  holy_gui_container_t parent;
  holy_video_rect_t bounds;
  char *id;
  char *theme_dir;
  struct holy_video_bitmap *raw_bitmap;
  struct holy_video_bitmap *bitmap;
};

typedef struct holy_gui_image *holy_gui_image_t;

static void
image_destroy (void *vself)
{
  holy_gui_image_t self = vself;

  /* Free the scaled bitmap, unless it's a reference to the raw bitmap.  */
  if (self->bitmap && (self->bitmap != self->raw_bitmap))
    holy_video_bitmap_destroy (self->bitmap);
  if (self->raw_bitmap)
    holy_video_bitmap_destroy (self->raw_bitmap);

  holy_free (self);
}

static const char *
image_get_id (void *vself)
{
  holy_gui_image_t self = vself;
  return self->id;
}

static int
image_is_instance (void *vself __attribute__((unused)), const char *type)
{
  return holy_strcmp (type, "component") == 0;
}

static void
image_paint (void *vself, const holy_video_rect_t *region)
{
  holy_gui_image_t self = vself;
  holy_video_rect_t vpsave;

  if (! self->bitmap)
    return;
  if (!holy_video_have_common_points (region, &self->bounds))
    return;

  holy_gui_set_viewport (&self->bounds, &vpsave);
  holy_video_blit_bitmap (self->bitmap, holy_VIDEO_BLIT_BLEND,
                          0, 0, 0, 0,
                          holy_video_bitmap_get_width (self->bitmap),
                          holy_video_bitmap_get_height (self->bitmap));
  holy_gui_restore_viewport (&vpsave);
}

static void
image_set_parent (void *vself, holy_gui_container_t parent)
{
  holy_gui_image_t self = vself;
  self->parent = parent;
}

static holy_gui_container_t
image_get_parent (void *vself)
{
  holy_gui_image_t self = vself;
  return self->parent;
}

static holy_err_t
rescale_image (holy_gui_image_t self)
{
  signed width;
  signed height;

  if (! self->raw_bitmap)
    {
      if (self->bitmap)
        {
          holy_video_bitmap_destroy (self->bitmap);
          self->bitmap = 0;
        }
      return holy_errno;
    }

  width = self->bounds.width;
  height = self->bounds.height;

  if (self->bitmap
      && ((signed) holy_video_bitmap_get_width (self->bitmap) == width)
      && ((signed) holy_video_bitmap_get_height (self->bitmap) == height))
    {
      /* Nothing to do; already the right size.  */
      return holy_errno;
    }

  /* Free any old scaled bitmap,
     *unless* it's a reference to the raw bitmap.  */
  if (self->bitmap && (self->bitmap != self->raw_bitmap))
    holy_video_bitmap_destroy (self->bitmap);

  self->bitmap = 0;

  /* Create a scaled bitmap, unless the requested size is the same
     as the raw size -- in that case a reference is made.  */
  if ((signed) holy_video_bitmap_get_width (self->raw_bitmap) == width
      && (signed) holy_video_bitmap_get_height (self->raw_bitmap) == height)
    {
      self->bitmap = self->raw_bitmap;
      return holy_errno;
    }

  /* Don't scale to an invalid size.  */
  if (width <= 0 || height <= 0)
    return holy_errno;

  /* Create the scaled bitmap.  */
  holy_video_bitmap_create_scaled (&self->bitmap,
                                   width,
                                   height,
                                   self->raw_bitmap,
                                   holy_VIDEO_BITMAP_SCALE_METHOD_BEST);
  return holy_errno;
}

static void
image_set_bounds (void *vself, const holy_video_rect_t *bounds)
{
  holy_gui_image_t self = vself;
  self->bounds = *bounds;
  rescale_image (self);
}

static void
image_get_bounds (void *vself, holy_video_rect_t *bounds)
{
  holy_gui_image_t self = vself;
  *bounds = self->bounds;
}

/* FIXME: inform rendering system it's not forced minimum.  */
static void
image_get_minimal_size (void *vself, unsigned *width, unsigned *height)
{
  holy_gui_image_t self = vself;

  if (self->raw_bitmap)
    {
      *width = holy_video_bitmap_get_width (self->raw_bitmap);
      *height = holy_video_bitmap_get_height (self->raw_bitmap);
    }
  else
    {
      *width = 0;
      *height = 0;
    }
}

static holy_err_t
load_image (holy_gui_image_t self, const char *path)
{
  struct holy_video_bitmap *bitmap;
  if (holy_video_bitmap_load (&bitmap, path) != holy_ERR_NONE)
    return holy_errno;

  if (self->bitmap && (self->bitmap != self->raw_bitmap))
    holy_video_bitmap_destroy (self->bitmap);
  if (self->raw_bitmap)
    holy_video_bitmap_destroy (self->raw_bitmap);

  self->raw_bitmap = bitmap;
  return rescale_image (self);
}

static holy_err_t
image_set_property (void *vself, const char *name, const char *value)
{
  holy_gui_image_t self = vself;
  if (holy_strcmp (name, "theme_dir") == 0)
    {
      holy_free (self->theme_dir);
      self->theme_dir = holy_strdup (value);
    }
  else if (holy_strcmp (name, "file") == 0)
    {
      char *absvalue;
      holy_err_t err;

      /* Resolve to an absolute path.  */
      if (! self->theme_dir)
	return holy_error (holy_ERR_BUG, "unspecified theme_dir");
      absvalue = holy_resolve_relative_path (self->theme_dir, value);
      if (! absvalue)
	return holy_errno;

      err = load_image (self, absvalue);
      holy_free (absvalue);

      return err;
    }
  else if (holy_strcmp (name, "id") == 0)
    {
      holy_free (self->id);
      if (value)
        self->id = holy_strdup (value);
      else
        self->id = 0;
    }
  return holy_errno;
}

static struct holy_gui_component_ops image_ops =
{
  .destroy = image_destroy,
  .get_id = image_get_id,
  .is_instance = image_is_instance,
  .paint = image_paint,
  .set_parent = image_set_parent,
  .get_parent = image_get_parent,
  .set_bounds = image_set_bounds,
  .get_bounds = image_get_bounds,
  .get_minimal_size = image_get_minimal_size,
  .set_property = image_set_property
};

holy_gui_component_t
holy_gui_image_new (void)
{
  holy_gui_image_t image;
  image = holy_zalloc (sizeof (*image));
  if (! image)
    return 0;
  image->component.ops = &image_ops;
  return (holy_gui_component_t) image;
}

