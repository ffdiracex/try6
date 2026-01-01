/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MENU_HEADER
#define holy_MENU_HEADER 1

struct holy_menu_entry_class
{
  char *name;
  struct holy_menu_entry_class *next;
};

/* The menu entry.  */
struct holy_menu_entry
{
  /* The title name.  */
  const char *title;

  /* The identifier.  */
  const char *id;

  /* If set means not everybody is allowed to boot this entry.  */
  int restricted;

  /* Allowed users.  */
  const char *users;

  /* The classes associated with the menu entry:
     used to choose an icon or other style attributes.
     This is a dummy head node for the linked list, so for an entry E,
     E.classes->next is the first class if it is not NULL.  */
  struct holy_menu_entry_class *classes;

  /* The sourcecode of the menu entry, used by the editor.  */
  const char *sourcecode;

  /* Parameters to be passed to menu definition.  */
  int argc;
  char **args;

  int hotkey;

  int submenu;

  /* The next element.  */
  struct holy_menu_entry *next;
};
typedef struct holy_menu_entry *holy_menu_entry_t;

/* The menu.  */
struct holy_menu
{
  /* The size of a menu.  */
  int size;

  /* The list of menu entries.  */
  holy_menu_entry_t entry_list;
};
typedef struct holy_menu *holy_menu_t;

/* Callback structure menu viewers can use to provide user feedback when
   default entries are executed, possibly including fallback entries.  */
typedef struct holy_menu_execute_callback
{
  /* Called immediately before ENTRY is booted.  */
  void (*notify_booting) (holy_menu_entry_t entry, void *userdata);

  /* Called when executing one entry has failed, and another entry, ENTRY, will
     be executed as a fallback.  The implementation of this function should
     delay for a period of at least 2 seconds before returning in order to
     allow the user time to read the information before it can be lost by
     executing ENTRY.  */
  void (*notify_fallback) (holy_menu_entry_t entry, void *userdata);

  /* Called when an entry has failed to execute and there is no remaining
     fallback entry to attempt.  */
  void (*notify_failure) (void *userdata);
}
*holy_menu_execute_callback_t;

holy_menu_entry_t holy_menu_get_entry (holy_menu_t menu, int no);
int holy_menu_get_timeout (void);
void holy_menu_set_timeout (int timeout);
void holy_menu_entry_run (holy_menu_entry_t entry);
int holy_menu_get_default_entry_index (holy_menu_t menu);

void holy_menu_init (void);
void holy_menu_fini (void);

#endif /* holy_MENU_HEADER */
