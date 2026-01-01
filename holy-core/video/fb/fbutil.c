/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/fbutil.h>
#include <holy/types.h>
#include <holy/video.h>

holy_video_color_t
get_pixel (struct holy_video_fbblit_info *source,
           unsigned int x, unsigned int y)
{
  holy_video_color_t color = 0;

  switch (source->mode_info->bpp)
    {
    case 32:
      color = *(holy_uint32_t *)holy_video_fb_get_video_ptr (source, x, y);
      break;

    case 24:
      {
        holy_uint8_t *ptr;
        ptr = holy_video_fb_get_video_ptr (source, x, y);
#ifdef holy_CPU_WORDS_BIGENDIAN
        color = ptr[2] | (ptr[1] << 8) | (ptr[0] << 16);
#else
        color = ptr[0] | (ptr[1] << 8) | (ptr[2] << 16);
#endif
      }
      break;

    case 16:
    case 15:
      color = *(holy_uint16_t *)holy_video_fb_get_video_ptr (source, x, y);
      break;

    case 8:
      color = *(holy_uint8_t *)holy_video_fb_get_video_ptr (source, x, y);
      break;

    case 1:
      if (source->mode_info->blit_format == holy_VIDEO_BLIT_FORMAT_1BIT_PACKED)
        {
          int bit_index = y * source->mode_info->width + x;
          holy_uint8_t *ptr = source->data + bit_index / 8;
          int bit_pos = 7 - bit_index % 8;
          color = (*ptr >> bit_pos) & 0x01;
        }
      break;

    default:
      break;
    }

  return color;
}

void
set_pixel (struct holy_video_fbblit_info *source,
           unsigned int x, unsigned int y, holy_video_color_t color)
{
  switch (source->mode_info->bpp)
    {
    case 32:
      {
        holy_uint32_t *ptr;

        ptr = (holy_uint32_t *)holy_video_fb_get_video_ptr (source, x, y);

        *ptr = color;
      }
      break;

    case 24:
      {
        holy_uint8_t *ptr;
#ifdef holy_CPU_WORDS_BIGENDIAN
        holy_uint8_t *colorptr = ((holy_uint8_t *)&color) + 1;
#else
        holy_uint8_t *colorptr = (holy_uint8_t *)&color;
#endif

        ptr = holy_video_fb_get_video_ptr (source, x, y);

        ptr[0] = colorptr[0];
        ptr[1] = colorptr[1];
        ptr[2] = colorptr[2];
      }
      break;

    case 16:
    case 15:
      {
        holy_uint16_t *ptr;

        ptr = (holy_uint16_t *)holy_video_fb_get_video_ptr (source, x, y);

        *ptr = (holy_uint16_t) (color & 0xFFFF);
      }
      break;

    case 8:
      {
        holy_uint8_t *ptr;

        ptr = (holy_uint8_t *)holy_video_fb_get_video_ptr (source, x, y);

        *ptr = (holy_uint8_t) (color & 0xFF);
      }
      break;

    case 1:
      if (source->mode_info->blit_format == holy_VIDEO_BLIT_FORMAT_1BIT_PACKED)
        {
          int bit_index = y * source->mode_info->width + x;
          holy_uint8_t *ptr = source->data + bit_index / 8;
          int bit_pos = 7 - bit_index % 8;
          *ptr = (*ptr & ~(1 << bit_pos)) | ((color & 0x01) << bit_pos);
        }
      break;

    default:
      break;
    }
}
