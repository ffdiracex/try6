/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/video.h>
#include <holy/gfxterm.h>
#include <holy/bitmap.h>
#include <holy/bitmap_scale.h>
#include <holy/term.h>
#include <holy/env.h>
#include <holy/normal.h>
#include <holy/gfxwidgets.h>
#include <holy/menu.h>
#include <holy/menu_viewer.h>
#include <holy/gfxmenu_model.h>
#include <holy/gfxmenu_view.h>
#include <holy/time.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_gfxmenu_view_t cached_view;

static void 
holy_gfxmenu_viewer_fini (void *data __attribute__ ((unused)))
{
}

/* FIXME: Previously 't' changed to text menu is it necessary?  */
static holy_err_t
holy_gfxmenu_try (int entry, holy_menu_t menu, int nested)
{
  holy_gfxmenu_view_t view = NULL;
  const char *theme_path;
  char *full_theme_path = 0;
  struct holy_menu_viewer *instance;
  holy_err_t err;
  struct holy_video_mode_info mode_info;

  theme_path = holy_env_get ("theme");
  if (! theme_path)
    return holy_error (holy_ERR_FILE_NOT_FOUND, N_("variable `%s' isn't set"),
		       "theme");

  err = holy_video_get_info (&mode_info);
  if (err)
    return err;

  instance = holy_zalloc (sizeof (*instance));
  if (!instance)
    return holy_errno;

  if (theme_path[0] != '/' && theme_path[0] != '(')
    {
      const char *prefix;
      prefix = holy_env_get ("prefix");
      full_theme_path = holy_xasprintf ("%s/themes/%s",
					prefix,
					theme_path);
    }

  if (!cached_view || holy_strcmp (cached_view->theme_path,
				   full_theme_path ? : theme_path) != 0
      || cached_view->screen.width != mode_info.width
      || cached_view->screen.height != mode_info.height)
    {
      holy_gfxmenu_view_destroy (cached_view);
      /* Create the view.  */
      cached_view = holy_gfxmenu_view_new (full_theme_path ? : theme_path,
					   mode_info.width,
					   mode_info.height);
    }
  holy_free (full_theme_path);

  if (! cached_view)
    {
      holy_free (instance);
      return holy_errno;
    }

  view = cached_view;

  view->double_repaint = (mode_info.mode_type
			  & holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED)
    && !(mode_info.mode_type & holy_VIDEO_MODE_TYPE_UPDATING_SWAP);
  view->selected = entry;
  view->menu = menu;
  view->nested = nested;
  view->first_timeout = -1;

  holy_video_set_viewport (0, 0, mode_info.width, mode_info.height);
  if (view->double_repaint)
    {
      holy_video_swap_buffers ();
      holy_video_set_viewport (0, 0, mode_info.width, mode_info.height);
    }

  holy_gfxmenu_view_draw (view);

  instance->data = view;
  instance->set_chosen_entry = holy_gfxmenu_set_chosen_entry;
  instance->fini = holy_gfxmenu_viewer_fini;
  instance->print_timeout = holy_gfxmenu_print_timeout;
  instance->clear_timeout = holy_gfxmenu_clear_timeout;

  holy_menu_register_viewer (instance);

  return holy_ERR_NONE;
}

holy_MOD_INIT (gfxmenu)
{
  struct holy_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
    if (holy_gfxmenu_try_hook && term->fullscreen)
      {
	term->fullscreen ();
	break;
      }

  holy_gfxmenu_try_hook = holy_gfxmenu_try;
}

holy_MOD_FINI (gfxmenu)
{
  holy_gfxmenu_view_destroy (cached_view);
  holy_gfxmenu_try_hook = NULL;
}
