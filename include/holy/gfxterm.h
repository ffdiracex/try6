/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GFXTERM_HEADER
#define holy_GFXTERM_HEADER	1

#include <holy/err.h>
#include <holy/types.h>
#include <holy/term.h>
#include <holy/video.h>
#include <holy/font.h>

holy_err_t
EXPORT_FUNC (holy_gfxterm_set_window) (struct holy_video_render_target *target,
				       int x, int y, int width, int height,
				       int double_repaint,
				       holy_font_t font, int border_width);

void EXPORT_FUNC (holy_gfxterm_schedule_repaint) (void);

extern void (*EXPORT_VAR (holy_gfxterm_decorator_hook)) (void);

struct holy_gfxterm_background
{
  struct holy_video_bitmap *bitmap;
  int blend_text_bg;
  holy_video_rgba_color_t default_bg_color;
};

extern struct holy_gfxterm_background EXPORT_VAR (holy_gfxterm_background);

void EXPORT_FUNC (holy_gfxterm_video_update_color) (void);
void
EXPORT_FUNC (holy_gfxterm_get_dimensions) (unsigned *width, unsigned *height);

#endif /* ! holy_GFXTERM_HEADER */
