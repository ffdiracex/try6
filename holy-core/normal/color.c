/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/normal.h>
#include <holy/term.h>
#include <holy/i18n.h>

/* Borrowed from holy Legacy */
static const char *color_list[16] =
{
  "black",
  "blue",
  "green",
  "cyan",
  "red",
  "magenta",
  "brown",
  "light-gray",
  "dark-gray",
  "light-blue",
  "light-green",
  "light-cyan",
  "light-red",
  "light-magenta",
  "yellow",
  "white"
};

static int
parse_color_name (holy_uint8_t *ret, char *name)
{
  holy_uint8_t i;
  for (i = 0; i < ARRAY_SIZE(color_list); i++)
    if (! holy_strcmp (name, color_list[i]))
      {
        *ret = i;
        return 0;
      }
  return -1;
}

int
holy_parse_color_name_pair (holy_uint8_t *color, const char *name)
{
  int result = 1;
  holy_uint8_t fg, bg;
  char *fg_name, *bg_name;

  /* nothing specified by user */
  if (name == NULL)
    return result;

  fg_name = holy_strdup (name);
  if (fg_name == NULL)
    {
      /* "out of memory" message was printed by holy_strdup() */
      holy_wait_after_message ();
      return result;
    }

  bg_name = holy_strchr (fg_name, '/');
  if (bg_name == NULL)
    {
      holy_printf_ (N_("Warning: syntax error (missing slash) in `%s'\n"), fg_name);
      holy_wait_after_message ();
      goto free_and_return;
    }

  *(bg_name++) = '\0';

  if (parse_color_name (&fg, fg_name) == -1)
    {
      holy_printf_ (N_("Warning: invalid foreground color `%s'\n"), fg_name);
      holy_wait_after_message ();
      goto free_and_return;
    }
  if (parse_color_name (&bg, bg_name) == -1)
    {
      holy_printf_ (N_("Warning: invalid background color `%s'\n"), bg_name);
      holy_wait_after_message ();
      goto free_and_return;
    }

  *color = (bg << 4) | fg;
  result = 0;

free_and_return:
  holy_free (fg_name);
  return result;
}

static void
set_colors (void)
{
  struct holy_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    /* Propagates `normal' color to terminal current color.  */
    holy_term_setcolorstate (term, holy_TERM_COLOR_NORMAL);
  }
}

/* Replace default `normal' colors with the ones specified by user (if any).  */
char *
holy_env_write_color_normal (struct holy_env_var *var __attribute__ ((unused)),
			     const char *val)
{
  if (holy_parse_color_name_pair (&holy_term_normal_color, val))
    return NULL;

  set_colors ();

  return holy_strdup (val);
}

/* Replace default `highlight' colors with the ones specified by user (if any).  */
char *
holy_env_write_color_highlight (struct holy_env_var *var __attribute__ ((unused)),
				const char *val)
{
  if (holy_parse_color_name_pair (&holy_term_highlight_color, val))
    return NULL;

  set_colors ();

  return holy_strdup (val);
}
