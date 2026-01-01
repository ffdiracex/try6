/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BITMAP_SCALE_HEADER
#define holy_BITMAP_SCALE_HEADER 1

#include <holy/err.h>
#include <holy/types.h>
#include <holy/bitmap_scale.h>

enum holy_video_bitmap_scale_method
{
  /* Choose the fastest interpolation algorithm.  */
  holy_VIDEO_BITMAP_SCALE_METHOD_FASTEST,
  /* Choose the highest quality interpolation algorithm.  */
  holy_VIDEO_BITMAP_SCALE_METHOD_BEST,

  /* Specific algorithms:  */
  /* Nearest neighbor interpolation.  */
  holy_VIDEO_BITMAP_SCALE_METHOD_NEAREST,
  /* Bilinear interpolation.  */
  holy_VIDEO_BITMAP_SCALE_METHOD_BILINEAR
};

typedef enum holy_video_bitmap_selection_method
{
  holy_VIDEO_BITMAP_SELECTION_METHOD_STRETCH,
  holy_VIDEO_BITMAP_SELECTION_METHOD_CROP,
  holy_VIDEO_BITMAP_SELECTION_METHOD_PADDING,
  holy_VIDEO_BITMAP_SELECTION_METHOD_FITWIDTH,
  holy_VIDEO_BITMAP_SELECTION_METHOD_FITHEIGHT
} holy_video_bitmap_selection_method_t;

typedef enum holy_video_bitmap_v_align
{
  holy_VIDEO_BITMAP_V_ALIGN_TOP,
  holy_VIDEO_BITMAP_V_ALIGN_CENTER,
  holy_VIDEO_BITMAP_V_ALIGN_BOTTOM
} holy_video_bitmap_v_align_t;

typedef enum holy_video_bitmap_h_align
{
  holy_VIDEO_BITMAP_H_ALIGN_LEFT,
  holy_VIDEO_BITMAP_H_ALIGN_CENTER,
  holy_VIDEO_BITMAP_H_ALIGN_RIGHT
} holy_video_bitmap_h_align_t;

holy_err_t
EXPORT_FUNC (holy_video_bitmap_create_scaled) (struct holy_video_bitmap **dst,
					       int dst_width, int dst_height,
					       struct holy_video_bitmap *src,
					       enum 
					       holy_video_bitmap_scale_method
					       scale_method);

holy_err_t
EXPORT_FUNC (holy_video_bitmap_scale_proportional)
                                     (struct holy_video_bitmap **dst,
                                      int dst_width, int dst_height,
                                      struct holy_video_bitmap *src,
                                      enum holy_video_bitmap_scale_method
                                      scale_method,
                                      holy_video_bitmap_selection_method_t
                                      selection_method,
                                      holy_video_bitmap_v_align_t v_align,
                                      holy_video_bitmap_h_align_t h_align);


#endif /* ! holy_BITMAP_SCALE_HEADER */
