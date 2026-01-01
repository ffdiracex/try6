/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NORMAL_HEADER
#define holy_NORMAL_HEADER	1

#include <holy/term.h>
#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/env.h>
#include <holy/menu.h>
#include <holy/command.h>
#include <holy/file.h>

/* The standard left and right margin for some messages.  */
#define STANDARD_MARGIN 6

/* The type of a completion item.  */
enum holy_completion_type
  {
    holy_COMPLETION_TYPE_COMMAND,
    holy_COMPLETION_TYPE_DEVICE,
    holy_COMPLETION_TYPE_PARTITION,
    holy_COMPLETION_TYPE_FILE,
    holy_COMPLETION_TYPE_ARGUMENT
  };
typedef enum holy_completion_type holy_completion_type_t;

extern struct holy_menu_viewer holy_normal_text_menu_viewer;
extern int holy_normal_exit_level;

/* Defined in `main.c'.  */
void holy_enter_normal_mode (const char *config);
void holy_normal_execute (const char *config, int nested, int batch);
struct holy_term_screen_geometry
{
  /* The number of entries shown at a time.  */
  int num_entries;
  int first_entry_y;
  int first_entry_x;
  int entry_width;
  int timeout_y;
  int timeout_lines;
  int border;
  int right_margin;
};

void holy_menu_init_page (int nested, int edit,
			  struct holy_term_screen_geometry *geo,
			  struct holy_term_output *term);
void holy_normal_init_page (struct holy_term_output *term, int y);
char *holy_file_getline (holy_file_t file);
void holy_cmdline_run (int nested, int force_auth);

/* Defined in `cmdline.c'.  */
char *holy_cmdline_get (const char *prompt);
holy_err_t holy_set_history (int newsize);

/* Defined in `completion.c'.  */
char *holy_normal_do_completion (char *buf, int *restore,
				 void (*hook) (const char *item, holy_completion_type_t type, int count));

/* Defined in `misc.c'.  */
holy_err_t holy_normal_print_device_info (const char *name);

/* Defined in `color.c'.  */
char *holy_env_write_color_normal (struct holy_env_var *var, const char *val);
char *holy_env_write_color_highlight (struct holy_env_var *var, const char *val);
int holy_parse_color_name_pair (holy_uint8_t *ret, const char *name);

/* Defined in `menu_text.c'.  */
void holy_wait_after_message (void);
void
holy_print_ucs4 (const holy_uint32_t * str,
		 const holy_uint32_t * last_position,
		 int margin_left, int margin_right,
		 struct holy_term_output *term);

void
holy_print_ucs4_menu (const holy_uint32_t * str,
		      const holy_uint32_t * last_position,
		      int margin_left, int margin_right,
		      struct holy_term_output *term,
		      int skip_lines, int max_lines, holy_uint32_t contchar,
		      struct holy_term_pos *pos);
int
holy_ucs4_count_lines (const holy_uint32_t * str,
		       const holy_uint32_t * last_position,
		       int margin_left, int margin_right,
		       struct holy_term_output *term);
holy_size_t holy_getstringwidth (holy_uint32_t * str,
				 const holy_uint32_t * last_position,
				 struct holy_term_output *term);
void holy_print_message_indented (const char *msg, int margin_left,
				  int margin_right,
				  struct holy_term_output *term);
void
holy_menu_text_register_instances (int entry, holy_menu_t menu, int nested);
holy_err_t
holy_show_menu (holy_menu_t menu, int nested, int autobooted);

/* Defined in `handler.c'.  */
void read_handler_list (void);
void free_handler_list (void);

/* Defined in `dyncmd.c'.  */
void read_command_list (const char *prefix);

/* Defined in `autofs.c'.  */
void read_fs_list (const char *prefix);

void holy_context_init (void);
void holy_context_fini (void);

void read_crypto_list (const char *prefix);

void read_terminal_list (const char *prefix);

void holy_set_more (int onoff);

void holy_normal_reset_more (void);

void holy_xputs_normal (const char *str);

extern int holy_extractor_level;

holy_err_t
holy_normal_add_menu_entry (int argc, const char **args, char **classes,
			    const char *id,
			    const char *users, const char *hotkey,
			    const char *prefix, const char *sourcecode,
			    int submenu);

holy_err_t
holy_normal_set_password (const char *user, const char *password);

void holy_normal_free_menu (holy_menu_t menu);

void holy_normal_auth_init (void);
void holy_normal_auth_fini (void);

void
holy_xnputs (const char *str, holy_size_t msg_len);

holy_command_t
holy_dyncmd_get_cmd (holy_command_t cmd);

void
holy_gettext_reread_prefix (const char *val);

enum holy_human_size_type
  {
    holy_HUMAN_SIZE_NORMAL,
    holy_HUMAN_SIZE_SHORT,
    holy_HUMAN_SIZE_SPEED,
  };

const char *
holy_get_human_size (holy_uint64_t size, enum holy_human_size_type type);

#endif /* ! holy_NORMAL_HEADER */
