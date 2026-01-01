/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MENU_VIEWER_HEADER
#define holy_MENU_VIEWER_HEADER 1

#include <holy/err.h>
#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/menu.h>
#include <holy/term.h>

struct holy_menu_viewer
{
  struct holy_menu_viewer *next;
  void *data;
  void (*set_chosen_entry) (int entry, void *data);
  void (*print_timeout) (int timeout, void *data);
  void (*clear_timeout) (void *data);
  void (*fini) (void *fini);
};

void holy_menu_register_viewer (struct holy_menu_viewer *viewer);

holy_err_t
holy_menu_try_text (struct holy_term_output *term,
		    int entry, holy_menu_t menu, int nested);

extern holy_err_t (*holy_gfxmenu_try_hook) (int entry, holy_menu_t menu,
					    int nested);

#endif /* holy_MENU_VIEWER_HEADER */
