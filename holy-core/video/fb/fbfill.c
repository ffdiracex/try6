/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/video_fb.h>
#include <holy/fbfill.h>
#include <holy/fbutil.h>
#include <holy/types.h>
#include <holy/video.h>

/* Generic filler that works for every supported mode.  */
static void
holy_video_fbfill (struct holy_video_fbblit_info *dst,
		   holy_video_color_t color, int x, int y,
		   int width, int height)
{
  int i;
  int j;

  for (j = 0; j < height; j++)
    for (i = 0; i < width; i++)
      set_pixel (dst, x + i, y + j, color);
}

/* Optimized filler for direct color 32 bit modes.  It is assumed that color
   is already mapped to destination format.  */
static void
holy_video_fbfill_direct32 (struct holy_video_fbblit_info *dst,
			    holy_video_color_t color, int x, int y,
			    int width, int height)
{
  int i;
  int j;
  holy_uint32_t *dstptr;
  holy_size_t rowskip;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  rowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  /* Get the start address.  */
  dstptr = holy_video_fb_get_video_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        *dstptr++ = color;

      /* Advance the dest pointer to the right location on the next line.  */
      holy_VIDEO_FB_ADVANCE_POINTER (dstptr, rowskip);
    }
}

/* Optimized filler for direct color 24 bit modes.  It is assumed that color
   is already mapped to destination format.  */
static void
holy_video_fbfill_direct24 (struct holy_video_fbblit_info *dst,
			    holy_video_color_t color, int x, int y,
			    int width, int height)
{
  int i;
  int j;
  holy_size_t rowskip;
  holy_uint8_t *dstptr;
#ifndef holy_CPU_WORDS_BIGENDIAN
  holy_uint8_t fill0 = (holy_uint8_t)((color >> 0) & 0xFF);
  holy_uint8_t fill1 = (holy_uint8_t)((color >> 8) & 0xFF);
  holy_uint8_t fill2 = (holy_uint8_t)((color >> 16) & 0xFF);
#else
  holy_uint8_t fill2 = (holy_uint8_t)((color >> 0) & 0xFF);
  holy_uint8_t fill1 = (holy_uint8_t)((color >> 8) & 0xFF);
  holy_uint8_t fill0 = (holy_uint8_t)((color >> 16) & 0xFF);
#endif
  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  rowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  /* Get the start address.  */
  dstptr = holy_video_fb_get_video_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        {
          *dstptr++ = fill0;
          *dstptr++ = fill1;
          *dstptr++ = fill2;
        }

      /* Advance the dest pointer to the right location on the next line.  */
      dstptr += rowskip;
    }
}

/* Optimized filler for direct color 16 bit modes.  It is assumed that color
   is already mapped to destination format.  */
static void
holy_video_fbfill_direct16 (struct holy_video_fbblit_info *dst,
			    holy_video_color_t color, int x, int y,
			    int width, int height)
{
  int i;
  int j;
  holy_size_t rowskip;
  holy_uint16_t *dstptr;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  rowskip = (dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width);

  /* Get the start address.  */
  dstptr = holy_video_fb_get_video_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
	*dstptr++ = color;

      /* Advance the dest pointer to the right location on the next line.  */
      holy_VIDEO_FB_ADVANCE_POINTER (dstptr, rowskip);
    }
}

/* Optimized filler for index color.  It is assumed that color
   is already mapped to destination format.  */
static void
holy_video_fbfill_direct8 (struct holy_video_fbblit_info *dst,
			   holy_video_color_t color, int x, int y,
			   int width, int height)
{
  int i;
  int j;
  holy_size_t rowskip;
  holy_uint8_t *dstptr;
  holy_uint8_t fill = (holy_uint8_t)color & 0xFF;

  /* Calculate the number of bytes to advance from the end of one line
     to the beginning of the next line.  */
  rowskip = dst->mode_info->pitch - dst->mode_info->bytes_per_pixel * width;

  /* Get the start address.  */
  dstptr = holy_video_fb_get_video_ptr (dst, x, y);

  for (j = 0; j < height; j++)
    {
      for (i = 0; i < width; i++)
        *dstptr++ = fill;

      /* Advance the dest pointer to the right location on the next line.  */
      dstptr += rowskip;
    }
}

void
holy_video_fb_fill_dispatch (struct holy_video_fbblit_info *target,
			     holy_video_color_t color, int x, int y,
			     unsigned int width, unsigned int height)
{
  /* Try to figure out more optimized version.  Note that color is already
     mapped to target format so we can make assumptions based on that.  */
  switch (target->mode_info->bytes_per_pixel)
    {
    case 4:
      holy_video_fbfill_direct32 (target, color, x, y,
				  width, height);
      return;
    case 3:
      holy_video_fbfill_direct24 (target, color, x, y,
				  width, height);
      return;
    case 2:
      holy_video_fbfill_direct16 (target, color, x, y,
                                        width, height);
      return;
    case 1:
      holy_video_fbfill_direct8 (target, color, x, y,
				       width, height);
      return;
    }

  /* No optimized version found, use default (slow) filler.  */
  holy_video_fbfill (target, color, x, y, width, height);
}
