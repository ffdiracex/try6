/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FBBLIT_HEADER
#define holy_FBBLIT_HEADER	1

/* NOTE: This header is private header for fb driver and should not be used
   in other parts of the code.  */

struct holy_video_fbblit_info;

/* NOTE: This function assumes that given coordinates are within bounds of
   handled data.  */
void
holy_video_fb_dispatch_blit (struct holy_video_fbblit_info *target,
			     struct holy_video_fbblit_info *source,
			     enum holy_video_blit_operators oper,
			     int x, int y,
			     unsigned int width, unsigned int height,
			     int offset_x, int offset_y);
#endif /* ! holy_FBBLIT_HEADER */
