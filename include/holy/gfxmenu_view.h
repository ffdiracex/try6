/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GFXMENU_VIEW_HEADER
#define holy_GFXMENU_VIEW_HEADER 1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/menu.h>
#include <holy/font.h>
#include <holy/gfxwidgets.h>

struct holy_gfxmenu_view;   /* Forward declaration of opaque type.  */
typedef struct holy_gfxmenu_view *holy_gfxmenu_view_t;


holy_gfxmenu_view_t holy_gfxmenu_view_new (const char *theme_path,
					   int width, int height);

void holy_gfxmenu_view_destroy (holy_gfxmenu_view_t view);

/* Set properties on the view based on settings from the specified
   theme file.  */
holy_err_t holy_gfxmenu_view_load_theme (holy_gfxmenu_view_t view,
                                         const char *theme_path);

holy_err_t holy_gui_recreate_box (holy_gfxmenu_box_t *boxptr,
                                  const char *pattern, const char *theme_dir);

void holy_gfxmenu_view_draw (holy_gfxmenu_view_t view);

void
holy_gfxmenu_redraw_menu (holy_gfxmenu_view_t view);

void
holy_gfxmenu_redraw_timeout (holy_gfxmenu_view_t view);

void
holy_gfxmenu_view_redraw (holy_gfxmenu_view_t view,
			  const holy_video_rect_t *region);

void 
holy_gfxmenu_clear_timeout (void *data);
void 
holy_gfxmenu_print_timeout (int timeout, void *data);
void
holy_gfxmenu_set_chosen_entry (int entry, void *data);

holy_err_t holy_font_draw_string (const char *str,
				  holy_font_t font,
				  holy_video_color_t color,
				  int left_x, int baseline_y);
int holy_font_get_string_width (holy_font_t font,
				const char *str);


/* Implementation details -- this should not be used outside of the
   view itself.  */

#include <holy/video.h>
#include <holy/bitmap.h>
#include <holy/bitmap_scale.h>
#include <holy/gui.h>
#include <holy/gfxwidgets.h>
#include <holy/icon_manager.h>

/* Definition of the private representation of the view.  */
struct holy_gfxmenu_view
{
  holy_video_rect_t screen;

  int need_to_check_sanity;
  holy_video_rect_t terminal_rect;
  int terminal_border;

  holy_font_t title_font;
  holy_font_t message_font;
  char *terminal_font_name;
  holy_video_rgba_color_t title_color;
  holy_video_rgba_color_t message_color;
  holy_video_rgba_color_t message_bg_color;
  struct holy_video_bitmap *raw_desktop_image;
  struct holy_video_bitmap *scaled_desktop_image;
  holy_video_bitmap_selection_method_t desktop_image_scale_method;
  holy_video_bitmap_h_align_t desktop_image_h_align;
  holy_video_bitmap_v_align_t desktop_image_v_align;
  holy_video_rgba_color_t desktop_color;
  holy_gfxmenu_box_t terminal_box;
  char *title_text;
  char *progress_message_text;
  char *theme_path;

  holy_gui_container_t canvas;

  int double_repaint;

  int selected;

  holy_video_rect_t progress_message_frame;

  holy_menu_t menu;

  int nested;

  int first_timeout;
};

#endif /* ! holy_GFXMENU_VIEW_HEADER */
