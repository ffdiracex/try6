/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/terminfo.h>
#include <holy/uboot/uboot.h>
#include <holy/uboot/console.h>

static void
put (struct holy_term_output *term __attribute__ ((unused)), const int c)
{
  holy_uboot_putc (c);
}

static int
readkey (struct holy_term_input *term __attribute__ ((unused)))
{
  if (holy_uboot_tstc () > 0)
    return holy_uboot_getc ();

  return -1;
}

static void
uboot_console_setcursor (struct holy_term_output *term
			 __attribute__ ((unused)), int on
			 __attribute__ ((unused)))
{
  holy_terminfo_setcursor (term, on);
}

static holy_err_t
uboot_console_init_input (struct holy_term_input *term)
{
  return holy_terminfo_input_init (term);
}

extern struct holy_terminfo_output_state uboot_console_terminfo_output;


static holy_err_t
uboot_console_init_output (struct holy_term_output *term)
{
  holy_terminfo_output_init (term);

  return 0;
}

struct holy_terminfo_input_state uboot_console_terminfo_input = {
  .readkey = readkey
};

struct holy_terminfo_output_state uboot_console_terminfo_output = {
  .put = put,
  /* FIXME: In rare cases when console isn't serial,
     determine real width.  */
  .size = { 80, 24 }
};

static struct holy_term_input uboot_console_term_input = {
  .name = "console",
  .init = uboot_console_init_input,
  .getkey = holy_terminfo_getkey,
  .data = &uboot_console_terminfo_input
};

static struct holy_term_output uboot_console_term_output = {
  .name = "console",
  .init = uboot_console_init_output,
  .putchar = holy_terminfo_putchar,
  .getwh = holy_terminfo_getwh,
  .getxy = holy_terminfo_getxy,
  .gotoxy = holy_terminfo_gotoxy,
  .cls = holy_terminfo_cls,
  .setcolorstate = holy_terminfo_setcolorstate,
  .setcursor = uboot_console_setcursor,
  .flags = holy_TERM_CODE_TYPE_ASCII,
  .data = &uboot_console_terminfo_output,
  .progress_update_divisor = holy_PROGRESS_FAST
};

void
holy_console_init_early (void)
{
  holy_term_register_input ("console", &uboot_console_term_input);
  holy_term_register_output ("console", &uboot_console_term_output);
}


/*
 * holy_console_init_lately():
 *   Initializes terminfo formatting by registering terminal type.
 *   Called after heap has been configured.
 *   
 */
void
holy_console_init_lately (void)
{
  const char *type;

  /* See if explicitly set by U-Boot environment */
  type = holy_uboot_env_get ("holy_term");
  if (!type)
    type = "vt100";

  holy_terminfo_init ();
  holy_terminfo_output_register (&uboot_console_term_output, type);
}

void
holy_console_fini (void)
{
}
