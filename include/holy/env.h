/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ENV_HEADER
#define holy_ENV_HEADER	1

#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/types.h>
#include <holy/menu.h>

struct holy_env_var;

typedef const char *(*holy_env_read_hook_t) (struct holy_env_var *var,
					     const char *val);
typedef char *(*holy_env_write_hook_t) (struct holy_env_var *var,
					const char *val);

struct holy_env_var
{
  char *name;
  char *value;
  holy_env_read_hook_t read_hook;
  holy_env_write_hook_t write_hook;
  struct holy_env_var *next;
  struct holy_env_var **prevp;
  struct holy_env_var *sorted_next;
  int global;
};

holy_err_t EXPORT_FUNC(holy_env_set) (const char *name, const char *val);
const char *EXPORT_FUNC(holy_env_get) (const char *name);
void EXPORT_FUNC(holy_env_unset) (const char *name);
struct holy_env_var *EXPORT_FUNC(holy_env_update_get_sorted) (void);

#define FOR_SORTED_ENV(var) for (var = holy_env_update_get_sorted (); var; var = var->sorted_next)

holy_err_t EXPORT_FUNC(holy_register_variable_hook) (const char *name,
						     holy_env_read_hook_t read_hook,
						     holy_env_write_hook_t write_hook);

holy_err_t holy_env_context_open (void);
holy_err_t holy_env_context_close (void);
holy_err_t EXPORT_FUNC(holy_env_export) (const char *name);

void holy_env_unset_menu (void);
holy_menu_t holy_env_get_menu (void);
void holy_env_set_menu (holy_menu_t nmenu);

holy_err_t
holy_env_extractor_open (int source);

holy_err_t
holy_env_extractor_close (int source);


#endif /* ! holy_ENV_HEADER */
