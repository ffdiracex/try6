/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GFXWIDGETS_HEADER
#define holy_GFXWIDGETS_HEADER 1

#include <holy/video.h>

typedef struct holy_gfxmenu_box *holy_gfxmenu_box_t;

struct holy_gfxmenu_box
{
  /* The size of the content.  */
  int content_width;
  int content_height;

  struct holy_video_bitmap **raw_pixmaps;
  struct holy_video_bitmap **scaled_pixmaps;

  void (*draw) (holy_gfxmenu_box_t self, int x, int y);
  void (*set_content_size) (holy_gfxmenu_box_t self,
                            int width, int height);
  int (*get_border_width) (holy_gfxmenu_box_t self);
  int (*get_left_pad) (holy_gfxmenu_box_t self);
  int (*get_top_pad) (holy_gfxmenu_box_t self);
  int (*get_right_pad) (holy_gfxmenu_box_t self);
  int (*get_bottom_pad) (holy_gfxmenu_box_t self);
  void (*destroy) (holy_gfxmenu_box_t self);
};

holy_gfxmenu_box_t holy_gfxmenu_create_box (const char *pixmaps_prefix,
                                            const char *pixmaps_suffix);

#endif /* ! holy_GFXWIDGETS_HEADER */
