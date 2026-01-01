/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/video.h>
#include <holy/bitmap.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* List of bitmap readers registered to system.  */
static holy_video_bitmap_reader_t bitmap_readers_list;

/* Register bitmap reader.  */
void
holy_video_bitmap_reader_register (holy_video_bitmap_reader_t reader)
{
  reader->next = bitmap_readers_list;
  bitmap_readers_list = reader;
}

/* Unregister bitmap reader.  */
void
holy_video_bitmap_reader_unregister (holy_video_bitmap_reader_t reader)
{
  holy_video_bitmap_reader_t *p, q;

  for (p = &bitmap_readers_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == reader)
      {
        *p = q->next;
        break;
      }
}

/* Creates new bitmap, saves created bitmap on success to *bitmap.  */
holy_err_t
holy_video_bitmap_create (struct holy_video_bitmap **bitmap,
                          unsigned int width, unsigned int height,
                          enum holy_video_blit_format blit_format)
{
  struct holy_video_mode_info *mode_info;
  unsigned int size;

  if (!bitmap)
    return holy_error (holy_ERR_BUG, "invalid argument");

  *bitmap = 0;

  if (width == 0 || height == 0)
    return holy_error (holy_ERR_BUG, "invalid argument");

  *bitmap = (struct holy_video_bitmap *)holy_malloc (sizeof (struct holy_video_bitmap));
  if (! *bitmap)
    return holy_errno;

  mode_info = &((*bitmap)->mode_info);

  /* Populate mode_info.  */
  mode_info->width = width;
  mode_info->height = height;
  mode_info->blit_format = blit_format;

  switch (blit_format)
    {
      case holy_VIDEO_BLIT_FORMAT_RGBA_8888:
        mode_info->mode_type = holy_VIDEO_MODE_TYPE_RGB
                               | holy_VIDEO_MODE_TYPE_ALPHA;
        mode_info->bpp = 32;
        mode_info->bytes_per_pixel = 4;
        mode_info->number_of_colors = 256;
        mode_info->red_mask_size = 8;
        mode_info->red_field_pos = 0;
        mode_info->green_mask_size = 8;
        mode_info->green_field_pos = 8;
        mode_info->blue_mask_size = 8;
        mode_info->blue_field_pos = 16;
        mode_info->reserved_mask_size = 8;
        mode_info->reserved_field_pos = 24;
        break;

      case holy_VIDEO_BLIT_FORMAT_RGB_888:
        mode_info->mode_type = holy_VIDEO_MODE_TYPE_RGB;
        mode_info->bpp = 24;
        mode_info->bytes_per_pixel = 3;
        mode_info->number_of_colors = 256;
        mode_info->red_mask_size = 8;
        mode_info->red_field_pos = 0;
        mode_info->green_mask_size = 8;
        mode_info->green_field_pos = 8;
        mode_info->blue_mask_size = 8;
        mode_info->blue_field_pos = 16;
        mode_info->reserved_mask_size = 0;
        mode_info->reserved_field_pos = 0;
        break;

      case holy_VIDEO_BLIT_FORMAT_INDEXCOLOR:
        mode_info->mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR;
        mode_info->bpp = 8;
        mode_info->bytes_per_pixel = 1;
        mode_info->number_of_colors = 256;
        mode_info->red_mask_size = 0;
        mode_info->red_field_pos = 0;
        mode_info->green_mask_size = 0;
        mode_info->green_field_pos = 0;
        mode_info->blue_mask_size = 0;
        mode_info->blue_field_pos = 0;
        mode_info->reserved_mask_size = 0;
        mode_info->reserved_field_pos = 0;
        break;

      default:
        holy_free (*bitmap);
        *bitmap = 0;

        return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
                           "unsupported bitmap format");
    }

  mode_info->pitch = width * mode_info->bytes_per_pixel;

  /* Calculate size needed for the data.  */
  size = (width * mode_info->bytes_per_pixel) * height;

  (*bitmap)->data = holy_zalloc (size);
  if (! (*bitmap)->data)
    {
      holy_free (*bitmap);
      *bitmap = 0;

      return holy_errno;
    }

  return holy_ERR_NONE;
}

/* Frees all resources allocated by bitmap.  */
holy_err_t
holy_video_bitmap_destroy (struct holy_video_bitmap *bitmap)
{
  if (! bitmap)
    return holy_ERR_NONE;

  holy_free (bitmap->data);
  holy_free (bitmap);

  return holy_ERR_NONE;
}

/* Match extension to filename.  */
static int
match_extension (const char *filename, const char *ext)
{
  int pos;
  int ext_len;

  pos = holy_strlen (filename);
  ext_len = holy_strlen (ext);

  if (! pos || ! ext_len || ext_len > pos)
    return 0;

  pos -= ext_len;

  return holy_strcasecmp (filename + pos, ext) == 0;
}

/* Loads bitmap using registered bitmap readers.  */
holy_err_t
holy_video_bitmap_load (struct holy_video_bitmap **bitmap,
                        const char *filename)
{
  holy_video_bitmap_reader_t reader = bitmap_readers_list;

  if (!bitmap)
    return holy_error (holy_ERR_BUG, "invalid argument");

  *bitmap = 0;

  while (reader)
    {
      if (match_extension (filename, reader->extension))
        return reader->reader (bitmap, filename);

      reader = reader->next;
    }

  return holy_error (holy_ERR_BAD_FILE_TYPE,
		     /* TRANSLATORS: We're speaking about bitmap images like
			JPEG or PNG.  */
		     N_("bitmap file `%s' is of"
			" unsupported format"), filename);
}

/* Return mode info for bitmap.  */
void holy_video_bitmap_get_mode_info (struct holy_video_bitmap *bitmap,
                                      struct holy_video_mode_info *mode_info)
{
  if (!bitmap)
    return;

  *mode_info = bitmap->mode_info;
}

/* Return pointer to bitmap's raw data.  */
void *holy_video_bitmap_get_data (struct holy_video_bitmap *bitmap)
{
  if (!bitmap)
    return 0;

  return bitmap->data;
}

