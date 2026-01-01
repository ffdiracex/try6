/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FBFILL_HEADER
#define holy_FBFILL_HEADER	1

/* NOTE: This header is private header for fb driver and should not be used
   in other parts of the code.  */

struct holy_video_fbblit_info;

struct holy_video_fbrender_target
{
  /* Copy of the screen's mode info structure, except that width, height and
     mode_type has been re-adjusted to requested render target settings.  */
  struct holy_video_mode_info mode_info;

  /* We should not draw outside of viewport.  */
  holy_video_rect_t viewport;
  /* Set region to make a re-draw of a part of the screen.  */
  holy_video_rect_t region;
  /* Should be set to 0 if the viewport is inside of the region.  */
  int area_enabled;
  /* Internal structure - intersection of the viewport and the region.  */
  holy_video_rect_t area;
  /* Internal values - offsets from the left top point of the viewport.  */
  int area_offset_x;
  int area_offset_y;

  /* Indicates whether the data has been allocated by us and must be freed
     when render target is destroyed.  */
  int is_allocated;

  /* Pointer to data.  Can either be in video card memory or in local host's
     memory.  */
  holy_uint8_t *data;
};

void
holy_video_fb_fill_dispatch (struct holy_video_fbblit_info *target,
			     holy_video_color_t color, int x, int y,
			     unsigned int width, unsigned int height);

#endif /* ! holy_FBFILL_HEADER */
