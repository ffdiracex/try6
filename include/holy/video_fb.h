/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_VIDEO_FB_HEADER
#define holy_VIDEO_FB_HEADER	1

#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/video.h>

/* FB module internal prototype (should not be used from elsewhere).  */

struct holy_video_fbblit_info;

struct holy_video_fbrender_target;

#define holy_VIDEO_FBSTD_NUMCOLORS 16
#define holy_VIDEO_FBSTD_EXT_NUMCOLORS 256

extern struct holy_video_palette_data EXPORT_VAR(holy_video_fbstd_colors)[holy_VIDEO_FBSTD_EXT_NUMCOLORS];

holy_err_t
EXPORT_FUNC(holy_video_fb_init) (void);

holy_err_t
EXPORT_FUNC(holy_video_fb_fini) (void);

holy_err_t
EXPORT_FUNC(holy_video_fb_get_info) (struct holy_video_mode_info *mode_info);

holy_err_t
EXPORT_FUNC(holy_video_fb_get_palette) (unsigned int start, unsigned int count,
					struct holy_video_palette_data *palette_data);
holy_err_t
EXPORT_FUNC(holy_video_fb_set_palette) (unsigned int start, unsigned int count,
					struct holy_video_palette_data *palette_data);
holy_err_t
EXPORT_FUNC(holy_video_fb_set_viewport) (unsigned int x, unsigned int y,
					 unsigned int width, unsigned int height);
holy_err_t
EXPORT_FUNC(holy_video_fb_get_viewport) (unsigned int *x, unsigned int *y,
					 unsigned int *width,
					 unsigned int *height);

holy_err_t
EXPORT_FUNC(holy_video_fb_set_region) (unsigned int x, unsigned int y,
                                       unsigned int width, unsigned int height);
holy_err_t
EXPORT_FUNC(holy_video_fb_get_region) (unsigned int *x, unsigned int *y,
                                       unsigned int *width,
                                       unsigned int *height);

holy_err_t
EXPORT_FUNC(holy_video_fb_set_area_status)
    (holy_video_area_status_t area_status);

holy_err_t
EXPORT_FUNC(holy_video_fb_get_area_status)
    (holy_video_area_status_t *area_status);

holy_video_color_t
EXPORT_FUNC(holy_video_fb_map_color) (holy_uint32_t color_name);

holy_video_color_t
EXPORT_FUNC(holy_video_fb_map_rgb) (holy_uint8_t red, holy_uint8_t green,
				    holy_uint8_t blue);

holy_video_color_t
EXPORT_FUNC(holy_video_fb_map_rgba) (holy_uint8_t red, holy_uint8_t green,
				     holy_uint8_t blue, holy_uint8_t alpha);

holy_err_t
EXPORT_FUNC(holy_video_fb_unmap_color) (holy_video_color_t color,
					holy_uint8_t *red, holy_uint8_t *green,
					holy_uint8_t *blue, holy_uint8_t *alpha);

void
holy_video_fb_unmap_color_int (struct holy_video_fbblit_info * source,
			       holy_video_color_t color,
			       holy_uint8_t *red, holy_uint8_t *green,
			       holy_uint8_t *blue, holy_uint8_t *alpha);

holy_err_t
EXPORT_FUNC(holy_video_fb_fill_rect) (holy_video_color_t color, int x, int y,
				      unsigned int width, unsigned int height);

holy_err_t
EXPORT_FUNC(holy_video_fb_blit_bitmap) (struct holy_video_bitmap *bitmap,
			   enum holy_video_blit_operators oper, int x, int y,
			   int offset_x, int offset_y,
			   unsigned int width, unsigned int height);

holy_err_t
EXPORT_FUNC(holy_video_fb_blit_render_target) (struct holy_video_fbrender_target *source,
				  enum holy_video_blit_operators oper,
				  int x, int y, int offset_x, int offset_y,
				  unsigned int width, unsigned int height);

holy_err_t
EXPORT_FUNC(holy_video_fb_scroll) (holy_video_color_t color, int dx, int dy);

holy_err_t
EXPORT_FUNC(holy_video_fb_create_render_target) (struct holy_video_fbrender_target **result,
				    unsigned int width, unsigned int height,
				    unsigned int mode_type __attribute__ ((unused)));

holy_err_t
EXPORT_FUNC(holy_video_fb_create_render_target_from_pointer) (struct holy_video_fbrender_target **result,
						 const struct holy_video_mode_info *mode_info,
						 void *ptr);

holy_err_t
EXPORT_FUNC(holy_video_fb_delete_render_target) (struct holy_video_fbrender_target *target);

holy_err_t
EXPORT_FUNC(holy_video_fb_get_active_render_target) (struct holy_video_fbrender_target **target);

holy_err_t
EXPORT_FUNC(holy_video_fb_set_active_render_target) (struct holy_video_fbrender_target *target);

typedef holy_err_t (*holy_video_fb_set_page_t) (int page);

holy_err_t
EXPORT_FUNC (holy_video_fb_setup) (unsigned int mode_type, unsigned int mode_mask,
		     struct holy_video_mode_info *mode_info,
		     volatile void *page0_ptr,
		     holy_video_fb_set_page_t set_page_in,
		     volatile void *page1_ptr);
holy_err_t
EXPORT_FUNC (holy_video_fb_swap_buffers) (void);
holy_err_t
EXPORT_FUNC (holy_video_fb_get_info_and_fini) (struct holy_video_mode_info *mode_info,
					       void **framebuf);

#endif /* ! holy_VIDEO_FB_HEADER */
