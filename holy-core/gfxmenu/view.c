/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/file.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/normal.h>
#include <holy/video.h>
#include <holy/gfxterm.h>
#include <holy/bitmap.h>
#include <holy/bitmap_scale.h>
#include <holy/term.h>
#include <holy/gfxwidgets.h>
#include <holy/time.h>
#include <holy/menu.h>
#include <holy/menu_viewer.h>
#include <holy/gfxmenu_view.h>
#include <holy/gui_string_util.h>
#include <holy/icon_manager.h>
#include <holy/i18n.h>

static void
init_terminal (holy_gfxmenu_view_t view);
static void
init_background (holy_gfxmenu_view_t view);
static holy_gfxmenu_view_t term_view;

/* Create a new view object, loading the theme specified by THEME_PATH and
   associating MODEL with the view.  */
holy_gfxmenu_view_t
holy_gfxmenu_view_new (const char *theme_path,
		       int width, int height)
{
  holy_gfxmenu_view_t view;
  holy_font_t default_font;
  holy_video_rgba_color_t default_fg_color;
  holy_video_rgba_color_t default_bg_color;

  view = holy_malloc (sizeof (*view));
  if (! view)
    return 0;

  while (holy_gfxmenu_timeout_notifications)
    {
      struct holy_gfxmenu_timeout_notify *p;
      p = holy_gfxmenu_timeout_notifications;
      holy_gfxmenu_timeout_notifications = holy_gfxmenu_timeout_notifications->next;
      holy_free (p);
    }

  view->screen.x = 0;
  view->screen.y = 0;
  view->screen.width = width;
  view->screen.height = height;

  view->need_to_check_sanity = 1;
  view->terminal_border = 3;
  view->terminal_rect.width = view->screen.width * 7 / 10;
  view->terminal_rect.height = view->screen.height * 7 / 10;
  view->terminal_rect.x = view->screen.x + (view->screen.width
                                            - view->terminal_rect.width) / 2;
  view->terminal_rect.y = view->screen.y + (view->screen.height
                                            - view->terminal_rect.height) / 2;

  default_font = holy_font_get ("Unknown Regular 16");
  default_fg_color = holy_video_rgba_color_rgb (0, 0, 0);
  default_bg_color = holy_video_rgba_color_rgb (255, 255, 255);

  view->canvas = 0;

  view->title_font = default_font;
  view->message_font = default_font;
  view->terminal_font_name = holy_strdup ("Fixed 10");
  view->title_color = default_fg_color;
  view->message_color = default_bg_color;
  view->message_bg_color = default_fg_color;
  view->raw_desktop_image = 0;
  view->scaled_desktop_image = 0;
  view->desktop_image_scale_method = holy_VIDEO_BITMAP_SELECTION_METHOD_STRETCH;
  view->desktop_image_h_align = holy_VIDEO_BITMAP_H_ALIGN_CENTER;
  view->desktop_image_v_align = holy_VIDEO_BITMAP_V_ALIGN_CENTER;
  view->desktop_color = default_bg_color;
  view->terminal_box = holy_gfxmenu_create_box (0, 0);
  view->title_text = holy_strdup (_("holy Boot Menu"));
  view->progress_message_text = 0;
  view->theme_path = 0;

  /* Set the timeout bar's frame.  */
  view->progress_message_frame.width = view->screen.width * 4 / 5;
  view->progress_message_frame.height = 50;
  view->progress_message_frame.x = view->screen.x
    + (view->screen.width - view->progress_message_frame.width) / 2;
  view->progress_message_frame.y = view->screen.y
    + view->screen.height - 90 - 20 - view->progress_message_frame.height;

  if (holy_gfxmenu_view_load_theme (view, theme_path) != 0)
    {
      holy_gfxmenu_view_destroy (view);
      return 0;
    }

  return view;
}

/* Destroy the view object.  All used memory is freed.  */
void
holy_gfxmenu_view_destroy (holy_gfxmenu_view_t view)
{
  if (!view)
    return;
  while (holy_gfxmenu_timeout_notifications)
    {
      struct holy_gfxmenu_timeout_notify *p;
      p = holy_gfxmenu_timeout_notifications;
      holy_gfxmenu_timeout_notifications = holy_gfxmenu_timeout_notifications->next;
      holy_free (p);
    }
  holy_video_bitmap_destroy (view->raw_desktop_image);
  holy_video_bitmap_destroy (view->scaled_desktop_image);
  if (view->terminal_box)
    view->terminal_box->destroy (view->terminal_box);
  holy_free (view->terminal_font_name);
  holy_free (view->title_text);
  holy_free (view->progress_message_text);
  holy_free (view->theme_path);
  if (view->canvas)
    view->canvas->component.ops->destroy (view->canvas);
  holy_free (view);
}

static void
redraw_background (holy_gfxmenu_view_t view,
		   const holy_video_rect_t *bounds)
{
  if (view->scaled_desktop_image)
    {
      struct holy_video_bitmap *img = view->scaled_desktop_image;
      holy_video_blit_bitmap (img, holy_VIDEO_BLIT_REPLACE,
                              bounds->x, bounds->y,
			      bounds->x - view->screen.x,
			      bounds->y - view->screen.y,
			      bounds->width, bounds->height);
    }
  else
    {
      holy_video_fill_rect (holy_video_map_rgba_color (view->desktop_color),
                            bounds->x, bounds->y,
                            bounds->width, bounds->height);
    }
}

static void
draw_title (holy_gfxmenu_view_t view)
{
  if (! view->title_text)
    return;

  /* Center the title. */
  int title_width = holy_font_get_string_width (view->title_font,
                                                view->title_text);
  int x = (view->screen.width - title_width) / 2;
  int y = 40 + holy_font_get_ascent (view->title_font);
  holy_font_draw_string (view->title_text,
                         view->title_font,
                         holy_video_map_rgba_color (view->title_color),
                         x, y);
}

struct progress_value_data
{
  int visible;
  int start;
  int end;
  int value;
};

struct holy_gfxmenu_timeout_notify *holy_gfxmenu_timeout_notifications;

static void
update_timeouts (int visible, int start, int value, int end)
{
  struct holy_gfxmenu_timeout_notify *cur;

  for (cur = holy_gfxmenu_timeout_notifications; cur; cur = cur->next)
    cur->set_state (cur->self, visible, start, value, end);
}

static void
redraw_timeouts (struct holy_gfxmenu_view *view)
{
  struct holy_gfxmenu_timeout_notify *cur;

  for (cur = holy_gfxmenu_timeout_notifications; cur; cur = cur->next)
    {
      holy_video_rect_t bounds;
      cur->self->ops->get_bounds (cur->self, &bounds);
      holy_video_set_area_status (holy_VIDEO_AREA_ENABLED);
      holy_gfxmenu_view_redraw (view, &bounds);
    }
}

void 
holy_gfxmenu_print_timeout (int timeout, void *data)
{
  struct holy_gfxmenu_view *view = data;

  if (view->first_timeout == -1)
    view->first_timeout = timeout;

  update_timeouts (1, -view->first_timeout, -timeout, 0);
  redraw_timeouts (view);
  holy_video_swap_buffers ();
  if (view->double_repaint)
    redraw_timeouts (view);
}

void 
holy_gfxmenu_clear_timeout (void *data)
{
  struct holy_gfxmenu_view *view = data;

  update_timeouts (0, 1, 0, 0);
  redraw_timeouts (view);
  holy_video_swap_buffers ();
  if (view->double_repaint)
    redraw_timeouts (view);
}

static void
update_menu_visit (holy_gui_component_t component,
                   void *userdata)
{
  holy_gfxmenu_view_t view;
  view = userdata;
  if (component->ops->is_instance (component, "list"))
    {
      holy_gui_list_t list = (holy_gui_list_t) component;
      list->ops->set_view_info (list, view);
    }
}

/* Update any boot menu components with the current menu model and
   theme path.  */
static void
update_menu_components (holy_gfxmenu_view_t view)
{
  holy_gui_iterate_recursively ((holy_gui_component_t) view->canvas,
                                update_menu_visit, view);
}

static void
refresh_menu_visit (holy_gui_component_t component,
              void *userdata)
{
  holy_gfxmenu_view_t view;
  view = userdata;
  if (component->ops->is_instance (component, "list"))
    {
      holy_gui_list_t list = (holy_gui_list_t) component;
      list->ops->refresh_list (list, view);
    }
}

/* Refresh list information (useful for submenus) */
static void
refresh_menu_components (holy_gfxmenu_view_t view)
{
  holy_gui_iterate_recursively ((holy_gui_component_t) view->canvas,
                                refresh_menu_visit, view);
}

static void
draw_message (holy_gfxmenu_view_t view)
{
  char *text = view->progress_message_text;
  holy_video_rect_t f = view->progress_message_frame;
  if (! text)
    return;

  holy_font_t font = view->message_font;
  holy_video_color_t color = holy_video_map_rgba_color (view->message_color);

  /* Border.  */
  holy_video_fill_rect (color,
                        f.x-1, f.y-1, f.width+2, f.height+2);
  /* Fill.  */
  holy_video_fill_rect (holy_video_map_rgba_color (view->message_bg_color),
                        f.x, f.y, f.width, f.height);

  /* Center the text. */
  int text_width = holy_font_get_string_width (font, text);
  int x = f.x + (f.width - text_width) / 2;
  int y = (f.y + (f.height - holy_font_get_descent (font)) / 2
           + holy_font_get_ascent (font) / 2);
  holy_font_draw_string (text, font, color, x, y);
}

void
holy_gfxmenu_view_redraw (holy_gfxmenu_view_t view,
			  const holy_video_rect_t *region)
{
  if (holy_video_have_common_points (&view->terminal_rect, region))
    holy_gfxterm_schedule_repaint ();

  holy_video_set_active_render_target (holy_VIDEO_RENDER_TARGET_DISPLAY);
  holy_video_area_status_t area_status;
  holy_video_get_area_status (&area_status);
  if (area_status == holy_VIDEO_AREA_ENABLED)
    holy_video_set_region (region->x, region->y,
                           region->width, region->height);

  redraw_background (view, region);
  if (view->canvas)
    view->canvas->component.ops->paint (view->canvas, region);
  draw_title (view);
  if (holy_video_have_common_points (&view->progress_message_frame, region))
    draw_message (view);

  if (area_status == holy_VIDEO_AREA_ENABLED)
    holy_video_set_area_status (holy_VIDEO_AREA_ENABLED);
}

void
holy_gfxmenu_view_draw (holy_gfxmenu_view_t view)
{
  init_terminal (view);

  init_background (view);

  /* Clear the screen; there may be garbage left over in video memory. */
  holy_video_fill_rect (holy_video_map_rgb (0, 0, 0),
                        view->screen.x, view->screen.y,
                        view->screen.width, view->screen.height);
  holy_video_swap_buffers ();
  if (view->double_repaint)
    holy_video_fill_rect (holy_video_map_rgb (0, 0, 0),
			  view->screen.x, view->screen.y,
			  view->screen.width, view->screen.height);

  refresh_menu_components (view);
  update_menu_components (view);

  holy_video_set_area_status (holy_VIDEO_AREA_DISABLED);
  holy_gfxmenu_view_redraw (view, &view->screen);
  holy_video_swap_buffers ();
  if (view->double_repaint)
    {
      holy_video_set_area_status (holy_VIDEO_AREA_DISABLED);
      holy_gfxmenu_view_redraw (view, &view->screen);
    }

}

static void
redraw_menu_visit (holy_gui_component_t component,
                   void *userdata)
{
  holy_gfxmenu_view_t view;
  view = userdata;
  if (component->ops->is_instance (component, "list"))
    {
      holy_video_rect_t bounds;

      component->ops->get_bounds (component, &bounds);
      holy_video_set_area_status (holy_VIDEO_AREA_ENABLED);
      holy_gfxmenu_view_redraw (view, &bounds);
    }
}

void
holy_gfxmenu_redraw_menu (holy_gfxmenu_view_t view)
{
  update_menu_components (view);

  holy_gui_iterate_recursively ((holy_gui_component_t) view->canvas,
                                redraw_menu_visit, view);
  holy_video_swap_buffers ();
  if (view->double_repaint)
    {
      holy_gui_iterate_recursively ((holy_gui_component_t) view->canvas,
				    redraw_menu_visit, view);
    }
}

void 
holy_gfxmenu_set_chosen_entry (int entry, void *data)
{
  holy_gfxmenu_view_t view = data;

  view->selected = entry;
  holy_gfxmenu_redraw_menu (view);
}

static void
holy_gfxmenu_draw_terminal_box (void)
{
  holy_gfxmenu_box_t term_box;

  term_box = term_view->terminal_box;
  if (!term_box)
    return;

  holy_video_set_area_status (holy_VIDEO_AREA_DISABLED);

  term_box->set_content_size (term_box, term_view->terminal_rect.width,
			      term_view->terminal_rect.height);
  
  term_box->draw (term_box,
		  term_view->terminal_rect.x - term_box->get_left_pad (term_box),
		  term_view->terminal_rect.y - term_box->get_top_pad (term_box));
}

static void
get_min_terminal (holy_font_t terminal_font,
                  unsigned int border_width,
                  unsigned int *min_terminal_width,
                  unsigned int *min_terminal_height)
{
  struct holy_font_glyph *glyph;
  glyph = holy_font_get_glyph (terminal_font, 'M');
  *min_terminal_width = (glyph? glyph->device_width : 8) * 80
                        + 2 * border_width;
  *min_terminal_height = holy_font_get_max_char_height (terminal_font) * 24
                         + 2 * border_width;
}

static void
terminal_sanity_check (holy_gfxmenu_view_t view)
{
  if (!view->need_to_check_sanity)
    return;

  /* terminal_font was checked before in the init_terminal function. */
  holy_font_t terminal_font = holy_font_get (view->terminal_font_name);

  /* Non-negative numbers below. */
  int scr_x = view->screen.x;
  int scr_y = view->screen.y;
  int scr_width = view->screen.width;
  int scr_height = view->screen.height;
  int term_x = view->terminal_rect.x;
  int term_y = view->terminal_rect.y;
  int term_width = view->terminal_rect.width;
  int term_height = view->terminal_rect.height;

  /* Check that border_width isn't too big. */
  unsigned int border_width = view->terminal_border;
  unsigned int min_terminal_width;
  unsigned int min_terminal_height;
  get_min_terminal (terminal_font, border_width,
                    &min_terminal_width, &min_terminal_height);
  if (border_width > 3 && ((int) min_terminal_width >= scr_width
                           || (int) min_terminal_height >= scr_height))
    {
      border_width = 3;
      get_min_terminal (terminal_font, border_width,
                        &min_terminal_width, &min_terminal_height);
    }

  /* Sanity checks. */
  if (term_width > scr_width)
    term_width = scr_width;
  if (term_height > scr_height)
    term_height = scr_height;

  if (scr_width <= (int) min_terminal_width
      || scr_height <= (int) min_terminal_height)
    {
      /* The screen resulution is too low. Use all space, except a small border
         to show the user, that it is a window. Then center the window. */
      term_width = scr_width - 6 * border_width;
      term_height = scr_height - 6 * border_width;
      term_x = scr_x + (scr_width - term_width) / 2;
      term_y = scr_y + (scr_height - term_height) / 2;
    }
  else if (term_width < (int) min_terminal_width
           || term_height < (int) min_terminal_height)
    {
      /* The screen resolution is big enough. Make sure, that terminal screen
         dimensions aren't less than minimal values. Then center the window. */
      term_width = (int) min_terminal_width;
      term_height = (int) min_terminal_height;
      term_x = scr_x + (scr_width - term_width) / 2;
      term_y = scr_y + (scr_height - term_height) / 2;
    }

  /* At this point w and h are satisfying. */
  if (term_x + term_width > scr_width)
    term_x = scr_width - term_width;
  if (term_y + term_height > scr_height)
    term_y = scr_height - term_height;

  /* Write down corrected data. */
  view->terminal_rect.x = (unsigned int) term_x;
  view->terminal_rect.y = (unsigned int) term_y;
  view->terminal_rect.width = (unsigned int) term_width;
  view->terminal_rect.height = (unsigned int) term_height;
  view->terminal_border = border_width;

  view->need_to_check_sanity = 0;
}

static void
init_terminal (holy_gfxmenu_view_t view)
{
  holy_font_t terminal_font;

  terminal_font = holy_font_get (view->terminal_font_name);
  if (!terminal_font)
    {
      holy_error (holy_ERR_BAD_FONT, "no font loaded");
      return;
    }

  /* Check that terminal window size and position are sane. */
  terminal_sanity_check (view);

  term_view = view;

  /* Note: currently there is no API for changing the gfxterm font
     on the fly, so whatever font the initially loaded theme specifies
     will be permanent.  */
  holy_gfxterm_set_window (holy_VIDEO_RENDER_TARGET_DISPLAY,
                           view->terminal_rect.x,
                           view->terminal_rect.y,
                           view->terminal_rect.width,
                           view->terminal_rect.height,
                           view->double_repaint,
                           terminal_font,
                           view->terminal_border);
  holy_gfxterm_decorator_hook = holy_gfxmenu_draw_terminal_box;
}

static void
init_background (holy_gfxmenu_view_t view)
{
  if (view->scaled_desktop_image)
    return;

  struct holy_video_bitmap *scaled_bitmap;
  if (view->desktop_image_scale_method ==
      holy_VIDEO_BITMAP_SELECTION_METHOD_STRETCH)
    holy_video_bitmap_create_scaled (&scaled_bitmap,
                                     view->screen.width,
                                     view->screen.height,
                                     view->raw_desktop_image,
                                     holy_VIDEO_BITMAP_SCALE_METHOD_BEST);
  else
    holy_video_bitmap_scale_proportional (&scaled_bitmap,
                                          view->screen.width,
                                          view->screen.height,
                                          view->raw_desktop_image,
                                          holy_VIDEO_BITMAP_SCALE_METHOD_BEST,
                                          view->desktop_image_scale_method,
                                          view->desktop_image_v_align,
                                          view->desktop_image_h_align);
  if (! scaled_bitmap)
    return;
  view->scaled_desktop_image = scaled_bitmap;

}

/* FIXME: previously notifications were displayed in special case.
   Is it necessary?
 */
#if 0
/* Sets MESSAGE as the progress message for the view.
   MESSAGE can be 0, in which case no message is displayed.  */
static void
set_progress_message (holy_gfxmenu_view_t view, const char *message)
{
  holy_free (view->progress_message_text);
  if (message)
    view->progress_message_text = holy_strdup (message);
  else
    view->progress_message_text = 0;
}

static void
notify_booting (holy_menu_entry_t entry, void *userdata)
{
  holy_gfxmenu_view_t view = (holy_gfxmenu_view_t) userdata;

  char *s = holy_malloc (100 + holy_strlen (entry->title));
  if (!s)
    return;

  holy_sprintf (s, "Booting '%s'", entry->title);
  set_progress_message (view, s);
  holy_free (s);
  holy_gfxmenu_view_redraw (view, &view->progress_message_frame);
  holy_video_swap_buffers ();
  if (view->double_repaint)
    holy_gfxmenu_view_redraw (view, &view->progress_message_frame);
}

static void
notify_fallback (holy_menu_entry_t entry, void *userdata)
{
  holy_gfxmenu_view_t view = (holy_gfxmenu_view_t) userdata;

  char *s = holy_malloc (100 + holy_strlen (entry->title));
  if (!s)
    return;

  holy_sprintf (s, "Falling back to '%s'", entry->title);
  set_progress_message (view, s);
  holy_free (s);
  holy_gfxmenu_view_redraw (view, &view->progress_message_frame);
  holy_video_swap_buffers ();
  if (view->double_repaint)
    holy_gfxmenu_view_redraw (view, &view->progress_message_frame);
}

static void
notify_execution_failure (void *userdata __attribute__ ((unused)))
{
}


static struct holy_menu_execute_callback execute_callback =
{
  .notify_booting = notify_booting,
  .notify_fallback = notify_fallback,
  .notify_failure = notify_execution_failure
};

#endif
