/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_TERMINFO_HEADER
#define holy_TERMINFO_HEADER	1

#include <holy/err.h>
#include <holy/types.h>
#include <holy/term.h>

char *EXPORT_FUNC(holy_terminfo_get_current) (struct holy_term_output *term);
holy_err_t EXPORT_FUNC(holy_terminfo_set_current) (struct holy_term_output *term,
												const char *);

#define holy_TERMINFO_READKEY_MAX_LEN 6
struct holy_terminfo_input_state
{
  int input_buf[holy_TERMINFO_READKEY_MAX_LEN];
  int npending;
#if defined(__powerpc__) && defined(holy_MACHINE_IEEE1275)
  int last_key;
  holy_uint64_t last_key_time;
#endif
  int (*readkey) (struct holy_term_input *term);
};

struct holy_terminfo_output_state
{
  struct holy_term_output *next;

  char *name;

  char *gotoxy;
  char *cls;
  char *reverse_video_on;
  char *reverse_video_off;
  char *cursor_on;
  char *cursor_off;
  char *setcolor;

  struct holy_term_coordinate size;
  struct holy_term_coordinate pos;

  void (*put) (struct holy_term_output *term, const int c);
};

holy_err_t EXPORT_FUNC(holy_terminfo_output_init) (struct holy_term_output *term);
void EXPORT_FUNC(holy_terminfo_gotoxy) (holy_term_output_t term,
					struct holy_term_coordinate pos);
void EXPORT_FUNC(holy_terminfo_cls) (holy_term_output_t term);
struct holy_term_coordinate EXPORT_FUNC (holy_terminfo_getxy) (struct holy_term_output *term);
void EXPORT_FUNC (holy_terminfo_setcursor) (struct holy_term_output *term,
					    const int on);
void EXPORT_FUNC (holy_terminfo_setcolorstate) (struct holy_term_output *term,
				  const holy_term_color_state state);


holy_err_t EXPORT_FUNC (holy_terminfo_input_init) (struct holy_term_input *term);
int EXPORT_FUNC (holy_terminfo_getkey) (struct holy_term_input *term);
void EXPORT_FUNC (holy_terminfo_putchar) (struct holy_term_output *term,
					  const struct holy_unicode_glyph *c);
struct holy_term_coordinate EXPORT_FUNC (holy_terminfo_getwh) (struct holy_term_output *term);


holy_err_t EXPORT_FUNC (holy_terminfo_output_register) (struct holy_term_output *term,
							const char *type);
holy_err_t EXPORT_FUNC (holy_terminfo_output_unregister) (struct holy_term_output *term);

void holy_terminfo_init (void);
void holy_terminfo_fini (void);

#endif /* ! holy_TERMINFO_HEADER */
