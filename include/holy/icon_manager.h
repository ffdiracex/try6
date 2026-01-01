/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ICON_MANAGER_HEADER
#define holy_ICON_MANAGER_HEADER 1

#include <holy/menu.h>
#include <holy/bitmap.h>

/* Forward declaration of opaque structure handle type.  */
typedef struct holy_gfxmenu_icon_manager *holy_gfxmenu_icon_manager_t;

holy_gfxmenu_icon_manager_t holy_gfxmenu_icon_manager_new (void);
void holy_gfxmenu_icon_manager_destroy (holy_gfxmenu_icon_manager_t mgr);
void holy_gfxmenu_icon_manager_clear_cache (holy_gfxmenu_icon_manager_t mgr);
void holy_gfxmenu_icon_manager_set_theme_path (holy_gfxmenu_icon_manager_t mgr,
                                               const char *path);
void holy_gfxmenu_icon_manager_set_icon_size (holy_gfxmenu_icon_manager_t mgr,
                                              int width, int height);
struct holy_video_bitmap *
holy_gfxmenu_icon_manager_get_icon (holy_gfxmenu_icon_manager_t mgr,
                                    holy_menu_entry_t entry);

#endif /* holy_ICON_MANAGER_HEADER */

