/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/time.h>

struct holy_term_output *holy_term_outputs_disabled;
struct holy_term_input *holy_term_inputs_disabled;
struct holy_term_output *holy_term_outputs;
struct holy_term_input *holy_term_inputs;

/* Current color state.  */
holy_uint8_t holy_term_normal_color = holy_TERM_DEFAULT_NORMAL_COLOR;
holy_uint8_t holy_term_highlight_color = holy_TERM_DEFAULT_HIGHLIGHT_COLOR;

void (*holy_term_poll_usb) (int wait_for_completion) = NULL;
void (*holy_net_poll_cards_idle) (void) = NULL;

/* Put a Unicode character.  */
static void
holy_putcode_dumb (holy_uint32_t code,
		   struct holy_term_output *term)
{
  struct holy_unicode_glyph c =
    {
      .base = code,
      .variant = 0,
      .attributes = 0,
      .ncomb = 0,
      .estimated_width = 1
    };

  if (code == '\t' && term->getxy)
    {
      int n;

      n = holy_TERM_TAB_WIDTH - ((term->getxy (term).x)
				 % holy_TERM_TAB_WIDTH);
      while (n--)
	holy_putcode_dumb (' ', term);

      return;
    }

  (term->putchar) (term, &c);
  if (code == '\n')
    holy_putcode_dumb ('\r', term);
}

static void
holy_xputs_dumb (const char *str)
{
  for (; *str; str++)
    {
      holy_term_output_t term;
      holy_uint32_t code = *str;
      if (code > 0x7f)
	code = '?';

      FOR_ACTIVE_TERM_OUTPUTS(term)
	holy_putcode_dumb (code, term);
    }
}

void (*holy_xputs) (const char *str) = holy_xputs_dumb;

int
holy_getkey_noblock (void)
{
  holy_term_input_t term;

  if (holy_term_poll_usb)
    holy_term_poll_usb (0);

  if (holy_net_poll_cards_idle)
    holy_net_poll_cards_idle ();

  FOR_ACTIVE_TERM_INPUTS(term)
  {
    int key = term->getkey (term);
    if (key != holy_TERM_NO_KEY)
      return key;
  }

  return holy_TERM_NO_KEY;
}

int
holy_getkey (void)
{
  int ret;

  holy_refresh ();

  while (1)
    {
      ret = holy_getkey_noblock ();
      if (ret != holy_TERM_NO_KEY)
	return ret;
      holy_cpu_idle ();
    }
}

void
holy_refresh (void)
{
  struct holy_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)
    holy_term_refresh (term);
}
