/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/terminfo.h>
#include <holy/ieee1275/console.h>
#include <holy/ieee1275/ieee1275.h>

static holy_ieee1275_ihandle_t stdout_ihandle;
static holy_ieee1275_ihandle_t stdin_ihandle;

extern struct holy_terminfo_output_state holy_console_terminfo_output;

struct color
{
  int red;
  int green;
  int blue;
};

/* Use serial colors as they are default on most firmwares and some firmwares
   ignore set-color!. Additionally output may be redirected to serial.  */
static struct color colors[] =
  {
    // {R, G, B}
    {0x00, 0x00, 0x00}, // 0 = black
    {0xA8, 0x00, 0x00}, // 1 = red
    {0x00, 0xA8, 0x00}, // 2 = green
    {0xFE, 0xFE, 0x54}, // 3 = yellow
    {0x00, 0x00, 0xA8}, // 4 = blue
    {0xA8, 0x00, 0xA8}, // 5 = magenta
    {0x00, 0xA8, 0xA8}, // 6 = cyan
    {0xFE, 0xFE, 0xFE}  // 7 = white
  };

static void
put (struct holy_term_output *term __attribute__ ((unused)), const int c)
{
  char chr = c;

  holy_ieee1275_write (stdout_ihandle, &chr, 1, 0);
}

static int
readkey (struct holy_term_input *term __attribute__ ((unused)))
{
  holy_uint8_t c;
  holy_ssize_t actual = 0;

  holy_ieee1275_read (stdin_ihandle, &c, 1, &actual);
  if (actual > 0)
    return c;
  return -1;
}

static void
holy_console_dimensions (void)
{
  holy_ieee1275_ihandle_t options;
  holy_ieee1275_phandle_t stdout_phandle;
  char val[1024];

  /* Always assume 80x24 on serial since screen-#rows/screen-#columns is often
     garbage for such devices.  */
  if (! holy_ieee1275_instance_to_package (stdout_ihandle,
					   &stdout_phandle)
      && ! holy_ieee1275_package_to_path (stdout_phandle,
					  val, sizeof (val) - 1, 0))
    {
      holy_ieee1275_ihandle_t stdout_options;
      val[sizeof (val) - 1] = 0;      

      if (! holy_ieee1275_finddevice (val, &stdout_options)
	  && ! holy_ieee1275_get_property (stdout_options, "device_type",
					   val, sizeof (val) - 1, 0))
	{
	  val[sizeof (val) - 1] = 0;
	  if (holy_strcmp (val, "serial") == 0)
	    {
	      holy_console_terminfo_output.size.x = 80;
	      holy_console_terminfo_output.size.y = 24;
	      return;
	    }
	}
    }

  if (! holy_ieee1275_finddevice ("/options", &options)
      && options != (holy_ieee1275_ihandle_t) -1)
    {
      if (! holy_ieee1275_get_property (options, "screen-#columns",
					val, sizeof (val) - 1, 0))
	{
	  val[sizeof (val) - 1] = 0;
	  holy_console_terminfo_output.size.x
	    = (holy_uint8_t) holy_strtoul (val, 0, 10);
	}
      if (! holy_ieee1275_get_property (options, "screen-#rows",
					val, sizeof (val) - 1, 0))
	{
	  val[sizeof (val) - 1] = 0;
	  holy_console_terminfo_output.size.y
	    = (holy_uint8_t) holy_strtoul (val, 0, 10);
	}
    }

  /* Bogus default value on SLOF in QEMU.  */
  if (holy_console_terminfo_output.size.x == 200
      && holy_console_terminfo_output.size.y == 200)
    {
      holy_console_terminfo_output.size.x = 80;
      holy_console_terminfo_output.size.y = 24;
    }

  /* Use a small console by default.  */
  if (! holy_console_terminfo_output.size.x)
    holy_console_terminfo_output.size.x = 80;
  if (! holy_console_terminfo_output.size.y)
    holy_console_terminfo_output.size.y = 24;
}

static void
holy_console_setcursor (struct holy_term_output *term,
			  int on)
{
  holy_terminfo_setcursor (term, on);

  if (!holy_ieee1275_test_flag (holy_IEEE1275_FLAG_HAS_CURSORONOFF))
    return;

  /* Understood by the Open Firmware flavour in OLPC.  */
  if (on)
    holy_ieee1275_interpret ("cursor-on", 0);
  else
    holy_ieee1275_interpret ("cursor-off", 0);
}

static holy_err_t
holy_console_init_input (struct holy_term_input *term)
{
  holy_ssize_t actual;

  if (holy_ieee1275_get_integer_property (holy_ieee1275_chosen, "stdin", &stdin_ihandle,
					  sizeof stdin_ihandle, &actual)
      || actual != sizeof stdin_ihandle)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "cannot find stdin");

  return holy_terminfo_input_init (term);
}

static holy_err_t
holy_console_init_output (struct holy_term_output *term)
{
  holy_ssize_t actual;

  /* The latest PowerMacs don't actually initialize the screen for us, so we
   * use this trick to re-open the output device (but we avoid doing this on
   * platforms where it's known to be broken). */
  if (! holy_ieee1275_test_flag (holy_IEEE1275_FLAG_BROKEN_OUTPUT))
    holy_ieee1275_interpret ("output-device output", 0);

  if (holy_ieee1275_get_integer_property (holy_ieee1275_chosen, "stdout", &stdout_ihandle,
					  sizeof stdout_ihandle, &actual)
      || actual != sizeof stdout_ihandle)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "cannot find stdout");

  /* Initialize colors.  */
  if (! holy_ieee1275_test_flag (holy_IEEE1275_FLAG_CANNOT_SET_COLORS))
    {
      unsigned col;
      for (col = 0; col < ARRAY_SIZE (colors); col++)
	holy_ieee1275_set_color (stdout_ihandle, col, colors[col].red,
				 colors[col].green, colors[col].blue);

      /* Set the right fg and bg colors.  */
      holy_terminfo_setcolorstate (term, holy_TERM_COLOR_NORMAL);
    }

  holy_console_dimensions ();

  holy_terminfo_output_init (term);

  return 0;
}



struct holy_terminfo_input_state holy_console_terminfo_input =
  {
    .readkey = readkey
  };

struct holy_terminfo_output_state holy_console_terminfo_output =
  {
    .put = put,
    .size = { 80, 24 }
  };

static struct holy_term_input holy_console_term_input =
  {
    .name = "console",
    .init = holy_console_init_input,
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
    .setcursor = holy_console_setcursor,
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
  const char *type;

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_NO_ANSI))
    type = "dumb";
  else if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_CURSORONOFF_ANSI_BROKEN))
    type = "ieee1275-nocursor";
  else
    type = "ieee1275";
  holy_terminfo_init ();
  holy_terminfo_output_register (&holy_console_term_output, type);
}

void
holy_console_fini (void)
{
}
