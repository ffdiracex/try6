/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_VIDEO_COLOR_HEADER
#define holy_VIDEO_COLOR_HEADER	1

#include <holy/video.h>

int holy_video_get_named_color (const char *name,
				holy_video_rgba_color_t *color);

holy_err_t holy_video_parse_color (const char *s,
				   holy_video_rgba_color_t *color);

#endif
