/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_VBEUTIL_MACHINE_HEADER
#define holy_VBEUTIL_MACHINE_HEADER	1

#include <holy/types.h>
#include <holy/video.h>

struct holy_video_fbblit_info
{
  struct holy_video_mode_info *mode_info;
  holy_uint8_t *data;
};

/* Don't use for 1-bit bitmaps, addressing needs to be done at the bit level
   and it doesn't make sense, in general, to ask for a pointer
   to a particular pixel's data.  */
static inline void *
holy_video_fb_get_video_ptr (struct holy_video_fbblit_info *source,
              unsigned int x, unsigned int y)
{
  return source->data + y * source->mode_info->pitch + x * source->mode_info->bytes_per_pixel;
}

/* Advance pointer by VAL bytes. If there is no unaligned access available,
   VAL has to be divisible by size of pointed type.
 */
#ifdef holy_HAVE_UNALIGNED_ACCESS
#define holy_VIDEO_FB_ADVANCE_POINTER(ptr, val) ((ptr) = (typeof (ptr)) ((char *) ptr + val))
#else
#define holy_VIDEO_FB_ADVANCE_POINTER(ptr, val) ((ptr) += (val) / sizeof (*(ptr)))
#endif

holy_video_color_t get_pixel (struct holy_video_fbblit_info *source,
                              unsigned int x, unsigned int y);

void set_pixel (struct holy_video_fbblit_info *source,
                unsigned int x, unsigned int y, holy_video_color_t color);

#endif /* ! holy_VBEUTIL_MACHINE_HEADER */
