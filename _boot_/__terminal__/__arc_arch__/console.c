/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/arc/arc.h>
#include <holy/arc/console.h>
#include <holy/term.h>
#include <holy/terminfo.h>

/* FIXME: use unicode.  */

static int
readkey (struct holy_term_input *term __attribute__ ((unused)))
{
  unsigned long count;
  char chr;

  if (holy_ARC_FIRMWARE_VECTOR->get_read_status (holy_ARC_STDIN))
    return -1;

  if (holy_ARC_FIRMWARE_VECTOR->read (holy_ARC_STDIN, &chr, 1, &count))
    return -1;
  if (!count)
    return -1;
  return chr;
}

static void
put (struct holy_term_output *term __attribute__ ((unused)), const int c)
{
  unsigned long count;
  char chr = c;

  holy_ARC_FIRMWARE_VECTOR->write (holy_ARC_STDOUT, &chr, 1, &count);
}

static struct holy_terminfo_output_state holy_console_terminfo_output;

int
holy_arc_is_device_serial (const char *name, int alt_names)
{
  if (name[0] == '\0')
    return 0;

  const char *ptr = name + holy_strlen (name) - 1;
  int i;
  /*
    Recognize:
    serial(N)
    serial(N)line(M)
   */
  for (i = 0; i < 2; i++)
    {
      if (!alt_names)
	{
	  if (*ptr != ')')
	    return 0;
	  ptr--;
	}
      for (; ptr >= name && holy_isdigit (*ptr); ptr--);
      if (ptr < name)
	return 0;
      if (!alt_names)
	{
	  if (*ptr != '(')
	    return 0;
	  ptr--;
	}
      if (ptr + 1 >= name + sizeof ("serial") - 1
	  && holy_memcmp (ptr + 1 - (sizeof ("serial") - 1),
			  "serial", sizeof ("serial") - 1) == 0)
	return 1;
      if (!(ptr + 1 >= name + sizeof ("line") - 1
	    && holy_memcmp (ptr + 1 - (sizeof ("line") - 1),
			    "line", sizeof ("line") - 1) == 0))
	return 0;
      ptr -= sizeof ("line") - 1;
      if (alt_names)
	{
	  if (*ptr != '/')
	    return 0;
	  ptr--;
	}
    }
  return 0;
}

static int
check_is_serial (void)
{
  static int is_serial = -1;

  if (is_serial != -1)
    return is_serial;

  const char *consout = 0;

  /* Check for serial. It works unless user manually overrides ConsoleOut
     variable. If he does there is nothing we can do. Fortunately failure
     isn't critical.
  */
  if (holy_ARC_SYSTEM_PARAMETER_BLOCK->firmware_vector_length
      >= (unsigned) ((char *) (&holy_ARC_FIRMWARE_VECTOR->getenvironmentvariable + 1)
		     - (char *) holy_ARC_FIRMWARE_VECTOR)
      && holy_ARC_FIRMWARE_VECTOR->getenvironmentvariable)
    consout = holy_ARC_FIRMWARE_VECTOR->getenvironmentvariable ("ConsoleOut");
  if (!consout)
    return is_serial = 0;
  return is_serial = holy_arc_is_device_serial (consout, 0);
}
    
static void
set_console_dimensions (void)
{
  struct holy_arc_display_status *info = NULL;

  if (check_is_serial ())
    {
      holy_console_terminfo_output.size.x = 80;
      holy_console_terminfo_output.size.y = 24;
      return;
    }

  if (holy_ARC_SYSTEM_PARAMETER_BLOCK->firmware_vector_length
      >= (unsigned) ((char *) (&holy_ARC_FIRMWARE_VECTOR->getdisplaystatus + 1)
		     - (char *) holy_ARC_FIRMWARE_VECTOR)
      && holy_ARC_FIRMWARE_VECTOR->getdisplaystatus)
    info = holy_ARC_FIRMWARE_VECTOR->getdisplaystatus (holy_ARC_STDOUT);
  if (info)
    {
      holy_console_terminfo_output.size.x = info->w + 1;
      holy_console_terminfo_output.size.y = info->h + 1;
    }
}

static holy_err_t
holy_console_init_output (struct holy_term_output *term)
{
  set_console_dimensions ();
  holy_terminfo_output_init (term);

  return 0;
}

static struct holy_terminfo_input_state holy_console_terminfo_input =
  {
    .readkey = readkey
  };

static struct holy_terminfo_output_state holy_console_terminfo_output =
  {
    .put = put,
    .size = { 80, 20 }
  };

static struct holy_term_input holy_console_term_input =
  {
    .name = "console",
    .init = holy_terminfo_input_init,
    .getkey = holy_terminfo_getkey,
    .data = &holy_console_terminfo_input
  };

static struct holy_term_output holy_console_term_output =
  {
    .name = "console",
    .init = holy_console_init_output,
    .putchar = holy_terminfo_putchar,
    .getxy = holy_terminfo_getxy,
    .getwh = holy_terminfo_getwh,
    .gotoxy = holy_terminfo_gotoxy,
    .cls = holy_terminfo_cls,
    .setcolorstate = holy_terminfo_setcolorstate,
    .setcursor = holy_terminfo_setcursor,
    .flags = holy_TERM_CODE_TYPE_ASCII,
    .data = &holy_console_terminfo_output,
    .progress_update_divisor = holy_PROGRESS_FAST
  };

void
holy_console_init_early (void)
{
  holy_term_register_input ("console", &holy_console_term_input);
  holy_term_register_output ("console", &holy_console_term_output);
}

void
holy_console_init_lately (void)
{
  holy_terminfo_init ();
  if (check_is_serial ())
    holy_terminfo_output_register (&holy_console_term_output, "vt100");
  else
    holy_terminfo_output_register (&holy_console_term_output, "arc");
}
