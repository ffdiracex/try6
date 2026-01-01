/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BITMAP_HEADER
#define holy_BITMAP_HEADER	1

#include <holy/err.h>
#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/video.h>

struct holy_video_bitmap
{
  /* Bitmap format description.  */
  struct holy_video_mode_info mode_info;

  /* Pointer to bitmap data formatted according to mode_info.  */
  void *data;
};

struct holy_video_bitmap_reader
{
  /* File extension for this bitmap type (including dot).  */
  const char *extension;

  /* Reader function to load bitmap.  */
  holy_err_t (*reader) (struct holy_video_bitmap **bitmap,
                        const char *filename);

  /* Next reader.  */
  struct holy_video_bitmap_reader *next;
};
typedef struct holy_video_bitmap_reader *holy_video_bitmap_reader_t;

void EXPORT_FUNC (holy_video_bitmap_reader_register) (holy_video_bitmap_reader_t reader);
void EXPORT_FUNC (holy_video_bitmap_reader_unregister) (holy_video_bitmap_reader_t reader);

holy_err_t EXPORT_FUNC (holy_video_bitmap_create) (struct holy_video_bitmap **bitmap,
						   unsigned int width, unsigned int height,
						   enum holy_video_blit_format blit_format);

holy_err_t EXPORT_FUNC (holy_video_bitmap_destroy) (struct holy_video_bitmap *bitmap);

holy_err_t EXPORT_FUNC (holy_video_bitmap_load) (struct holy_video_bitmap **bitmap,
						 const char *filename);

/* Return bitmap width.  */
static inline unsigned int
holy_video_bitmap_get_width (struct holy_video_bitmap *bitmap)
{
  if (!bitmap)
    return 0;

  return bitmap->mode_info.width;
}

/* Return bitmap height.  */
static inline unsigned int
holy_video_bitmap_get_height (struct holy_video_bitmap *bitmap)
{
  if (!bitmap)
    return 0;

  return bitmap->mode_info.height;
}

void EXPORT_FUNC (holy_video_bitmap_get_mode_info) (struct holy_video_bitmap *bitmap,
						    struct holy_video_mode_info *mode_info);

void *EXPORT_FUNC (holy_video_bitmap_get_data) (struct holy_video_bitmap *bitmap);

#endif /* ! holy_BITMAP_HEADER */
