/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/gui_string_util.h>
#include <holy/bitmap.h>
#include <holy/bitmap_scale.h>
#include <holy/menu.h>
#include <holy/icon_manager.h>
#include <holy/env.h>

/* Currently hard coded to '.png' extension.  */
static const char icon_extension[] = ".png";

typedef struct icon_entry
{
  char *class_name;
  struct holy_video_bitmap *bitmap;
  struct icon_entry *next;
} *icon_entry_t;

struct holy_gfxmenu_icon_manager
{
  char *theme_path;
  int icon_width;
  int icon_height;

  /* Icon cache: linked list w/ dummy head node.  */
  struct icon_entry cache;
};


/* Create a new icon manager and return a point to it.  */
holy_gfxmenu_icon_manager_t
holy_gfxmenu_icon_manager_new (void)
{
  holy_gfxmenu_icon_manager_t mgr;
  mgr = holy_malloc (sizeof (*mgr));
  if (! mgr)
    return 0;

  mgr->theme_path = 0;
  mgr->icon_width = 0;
  mgr->icon_height = 0;

  /* Initialize the dummy head node.  */
  mgr->cache.class_name = 0;
  mgr->cache.bitmap = 0;
  mgr->cache.next = 0;

  return mgr;
}

/* Destroy the icon manager MGR, freeing all resources used by it.

Note: Any bitmaps returned by holy_gfxmenu_icon_manager_get_icon()
are destroyed and must not be used by the caller after this function
is called.  */
void
holy_gfxmenu_icon_manager_destroy (holy_gfxmenu_icon_manager_t mgr)
{
  holy_gfxmenu_icon_manager_clear_cache (mgr);
  holy_free (mgr->theme_path);
  holy_free (mgr);
}

/* Clear the icon cache.  */
void
holy_gfxmenu_icon_manager_clear_cache (holy_gfxmenu_icon_manager_t mgr)
{
  icon_entry_t cur;
  icon_entry_t next;
  for (cur = mgr->cache.next; cur; cur = next)
    {
      next = cur->next;
      holy_free (cur->class_name);
      holy_video_bitmap_destroy (cur->bitmap);
      holy_free (cur);
    }
  mgr->cache.next = 0;
}

/* Set the theme path.  If the theme path is changed, the icon cache
   is cleared.  */
void
holy_gfxmenu_icon_manager_set_theme_path (holy_gfxmenu_icon_manager_t mgr,
                                          const char *path)
{
  /* Clear the cache if the theme path has changed.  */
  if (mgr->theme_path == 0 && path == 0)
    return;
  if (mgr->theme_path == 0 || path == 0
      || holy_strcmp (mgr->theme_path, path) != 0)
    holy_gfxmenu_icon_manager_clear_cache (mgr);

  holy_free (mgr->theme_path);
  mgr->theme_path = path ? holy_strdup (path) : 0;
}

/* Set the icon size.  When icons are requested from the icon manager,
   they are scaled to this size before being returned.  If the size is
   changed, the icon cache is cleared.  */
void
holy_gfxmenu_icon_manager_set_icon_size (holy_gfxmenu_icon_manager_t mgr,
                                         int width, int height)
{
  /* If the width or height is changed, we must clear the cache, since the
     scaled bitmaps are stored in the cache.  */
  if (width != mgr->icon_width || height != mgr->icon_height)
    holy_gfxmenu_icon_manager_clear_cache (mgr);

  mgr->icon_width = width;
  mgr->icon_height = height;
}

/* Try to load an icon for the specified CLASS_NAME in the directory DIR.
   Returns 0 if the icon could not be loaded, or returns a pointer to a new
   bitmap if it was successful.  */
static struct holy_video_bitmap *
try_loading_icon (holy_gfxmenu_icon_manager_t mgr,
                  const char *dir, const char *class_name)
{
  char *path, *ptr;

  path = holy_malloc (holy_strlen (dir) + holy_strlen (class_name)
		      + holy_strlen (icon_extension) + 3);
  if (! path)
    return 0;

  ptr = holy_stpcpy (path, dir);
  if (path == ptr || ptr[-1] != '/')
    *ptr++ = '/';
  ptr = holy_stpcpy (ptr, class_name);
  ptr = holy_stpcpy (ptr, icon_extension);
  *ptr = '\0';

  struct holy_video_bitmap *raw_bitmap;
  holy_video_bitmap_load (&raw_bitmap, path);
  holy_free (path);
  holy_errno = holy_ERR_NONE;  /* Critical to clear the error!!  */
  if (! raw_bitmap)
    return 0;

  struct holy_video_bitmap *scaled_bitmap;
  holy_video_bitmap_create_scaled (&scaled_bitmap,
                                   mgr->icon_width, mgr->icon_height,
                                   raw_bitmap,
                                   holy_VIDEO_BITMAP_SCALE_METHOD_BEST);
  holy_video_bitmap_destroy (raw_bitmap);
  if (! scaled_bitmap)
    return 0;

  return scaled_bitmap;
}

/* Get the icon for the specified class CLASS_NAME.  If an icon for
   CLASS_NAME already exists in the cache, then a reference to the cached
   bitmap is returned.  If it is not cached, then it is loaded and cached.
   If no icon could be could for CLASS_NAME, then 0 is returned.  */
static struct holy_video_bitmap *
get_icon_by_class (holy_gfxmenu_icon_manager_t mgr, const char *class_name)
{
  /* First check the icon cache.  */
  icon_entry_t entry;
  for (entry = mgr->cache.next; entry; entry = entry->next)
    {
      if (holy_strcmp (entry->class_name, class_name) == 0)
        return entry->bitmap;
    }

  if (! mgr->theme_path)
    return 0;

  /* Otherwise, we search for an icon to load.  */
  char *theme_dir = holy_get_dirname (mgr->theme_path);
  char *icons_dir;
  struct holy_video_bitmap *icon;
  icon = 0;
  /* First try the theme's own icons, from "holy/themes/NAME/icons/"  */
  icons_dir = holy_resolve_relative_path (theme_dir, "icons/");
  if (icons_dir)
    {
      icon = try_loading_icon (mgr, icons_dir, class_name);
      holy_free (icons_dir);
    }

  holy_free (theme_dir);
  if (! icon)
    {
      const char *icondir;

      icondir = holy_env_get ("icondir");
      if (icondir)
	icon = try_loading_icon (mgr, icondir, class_name);
    }

  /* No icon was found.  */
  /* This should probably be noted in the cache, so that a search is not
     performed each time an icon for CLASS_NAME is requested.  */
  if (! icon)
    return 0;

  /* Insert a new cache entry for this icon.  */
  entry = holy_malloc (sizeof (*entry));
  if (! entry)
    {
      holy_video_bitmap_destroy (icon);
      return 0;
    }
  entry->class_name = holy_strdup (class_name);
  entry->bitmap = icon;
  entry->next = mgr->cache.next;
  mgr->cache.next = entry;   /* Link it into the cache.  */
  return entry->bitmap;
}

/* Get the best available icon for ENTRY.  Beginning with the first class
   listed in the menu entry and proceeding forward, an icon for each class
   is searched for.  The first icon found is returned.  The returned icon
   is scaled to the size specified by
   holy_gfxmenu_icon_manager_set_icon_size().

     Note:  Bitmaps returned by this function are destroyed when the
            icon manager is destroyed.
 */
struct holy_video_bitmap *
holy_gfxmenu_icon_manager_get_icon (holy_gfxmenu_icon_manager_t mgr,
                                    holy_menu_entry_t entry)
{
  struct holy_menu_entry_class *c;
  struct holy_video_bitmap *icon;

  /* Try each class in succession.  */
  icon = 0;
  for (c = entry->classes; c && ! icon; c = c->next)
    icon = get_icon_by_class (mgr, c->name);
  return icon;
}
