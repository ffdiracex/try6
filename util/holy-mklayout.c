/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/misc.h>
#include <holy/i18n.h>
#include <holy/term.h>
#include <holy/keyboard_layouts.h>

#define _GNU_SOURCE	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"

#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

#include "progname.h"

struct arguments
{
  char *input;
  char *output;
  int verbosity;
};

static struct argp_option options[] = {
  {"input",  'i', N_("FILE"), 0,
   N_("set input filename. Default is STDIN"), 0},
  {"output",  'o', N_("FILE"), 0,
   N_("set output filename. Default is STDOUT"), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  { 0, 0, 0, 0, 0, 0 }
};

struct console_holy_equivalence
{
  const char *layout;
  holy_uint32_t holy;
};

static struct console_holy_equivalence console_holy_equivalences_shift[] = {
  {"KP_0", '0'},
  {"KP_1", '1'},
  {"KP_2", '2'},
  {"KP_3", '3'},
  {"KP_4", '4'},
  {"KP_5", '5'},
  {"KP_6", '6'},
  {"KP_7", '7'},
  {"KP_8", '8'},
  {"KP_9", '9'},
  {"KP_Period", '.'},

  {NULL, '\0'}
};

static struct console_holy_equivalence console_holy_equivalences_unshift[] = {
  {"KP_0", holy_TERM_KEY_INSERT},
  {"KP_1", holy_TERM_KEY_END},
  {"KP_2", holy_TERM_KEY_DOWN},
  {"KP_3", holy_TERM_KEY_NPAGE},
  {"KP_4", holy_TERM_KEY_LEFT},
  {"KP_5", holy_TERM_KEY_CENTER},
  {"KP_6", holy_TERM_KEY_RIGHT},
  {"KP_7", holy_TERM_KEY_HOME},
  {"KP_8", holy_TERM_KEY_UP},
  {"KP_9", holy_TERM_KEY_PPAGE},
  {"KP_Period", holy_TERM_KEY_DC},

  {NULL, '\0'}
};

static struct console_holy_equivalence console_holy_equivalences_common[] = {
  {"Escape", holy_TERM_ESC},
  {"Tab", holy_TERM_TAB},
  {"Delete", holy_TERM_BACKSPACE},

  {"KP_Enter", '\n'},
  {"Return", '\n'},

  {"KP_Multiply", '*'},
  {"KP_Subtract", '-'},
  {"KP_Add", '+'},
  {"KP_Divide", '/'},

  {"F1", holy_TERM_KEY_F1},
  {"F2", holy_TERM_KEY_F2},
  {"F3", holy_TERM_KEY_F3},
  {"F4", holy_TERM_KEY_F4},
  {"F5", holy_TERM_KEY_F5},
  {"F6", holy_TERM_KEY_F6},
  {"F7", holy_TERM_KEY_F7},
  {"F8", holy_TERM_KEY_F8},
  {"F9", holy_TERM_KEY_F9},
  {"F10", holy_TERM_KEY_F10},
  {"F11", holy_TERM_KEY_F11},
  {"F12", holy_TERM_KEY_F12},
  {"F13", holy_TERM_KEY_F1 | holy_TERM_SHIFT},
  {"F14", holy_TERM_KEY_F2 | holy_TERM_SHIFT},
  {"F15", holy_TERM_KEY_F3 | holy_TERM_SHIFT},
  {"F16", holy_TERM_KEY_F4 | holy_TERM_SHIFT},
  {"F17", holy_TERM_KEY_F5 | holy_TERM_SHIFT},
  {"F18", holy_TERM_KEY_F6 | holy_TERM_SHIFT},
  {"F19", holy_TERM_KEY_F7 | holy_TERM_SHIFT},
  {"F20", holy_TERM_KEY_F8 | holy_TERM_SHIFT},
  {"F21", holy_TERM_KEY_F9 | holy_TERM_SHIFT},
  {"F22", holy_TERM_KEY_F10 | holy_TERM_SHIFT},
  {"F23", holy_TERM_KEY_F11 | holy_TERM_SHIFT},
  {"F24", holy_TERM_KEY_F12 | holy_TERM_SHIFT},
  {"Console_13", holy_TERM_KEY_F1 | holy_TERM_ALT},
  {"Console_14", holy_TERM_KEY_F2 | holy_TERM_ALT},
  {"Console_15", holy_TERM_KEY_F3 | holy_TERM_ALT},
  {"Console_16", holy_TERM_KEY_F4 | holy_TERM_ALT},
  {"Console_17", holy_TERM_KEY_F5 | holy_TERM_ALT},
  {"Console_18", holy_TERM_KEY_F6 | holy_TERM_ALT},
  {"Console_19", holy_TERM_KEY_F7 | holy_TERM_ALT},
  {"Console_20", holy_TERM_KEY_F8 | holy_TERM_ALT},
  {"Console_21", holy_TERM_KEY_F9 | holy_TERM_ALT},
  {"Console_22", holy_TERM_KEY_F10 | holy_TERM_ALT},
  {"Console_23", holy_TERM_KEY_F11 | holy_TERM_ALT},
  {"Console_24", holy_TERM_KEY_F12 | holy_TERM_ALT},
  {"Console_25", holy_TERM_KEY_F1 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_26", holy_TERM_KEY_F2 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_27", holy_TERM_KEY_F3 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_28", holy_TERM_KEY_F4 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_29", holy_TERM_KEY_F5 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_30", holy_TERM_KEY_F6 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_31", holy_TERM_KEY_F7 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_32", holy_TERM_KEY_F8 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_33", holy_TERM_KEY_F9 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_34", holy_TERM_KEY_F10 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_35", holy_TERM_KEY_F11 | holy_TERM_SHIFT | holy_TERM_ALT},
  {"Console_36", holy_TERM_KEY_F12 | holy_TERM_SHIFT | holy_TERM_ALT},

  {"Insert", holy_TERM_KEY_INSERT},
  {"Down", holy_TERM_KEY_DOWN},
  {"Up", holy_TERM_KEY_UP},
  {"Home", holy_TERM_KEY_HOME},
  {"End", holy_TERM_KEY_END},
  {"Right", holy_TERM_KEY_RIGHT},
  {"Left", holy_TERM_KEY_LEFT},
  {"Next", holy_TERM_KEY_NPAGE},
  {"Prior", holy_TERM_KEY_PPAGE},
  {"Remove", holy_TERM_KEY_DC},
  {"VoidSymbol", 0},

  /* "Undead" keys since no dead key support in holy.  */
  {"dead_acute", '\''},
  {"dead_circumflex", '^'},
  {"dead_grave", '`'},
  {"dead_tilde", '~'},
  {"dead_diaeresis", '"'},
  
  /* Following ones don't provide any useful symbols for shell.  */
  {"dead_cedilla", 0},
  {"dead_ogonek", 0},
  {"dead_caron", 0},
  {"dead_breve", 0},
  {"dead_doubleacute", 0},

  /* Unused in holy.  */
  {"Pause", 0},
  {"Scroll_Forward", 0},
  {"Scroll_Backward", 0},
  {"Hex_0", 0},
  {"Hex_1", 0},
  {"Hex_2", 0},
  {"Hex_3", 0},
  {"Hex_4", 0},
  {"Hex_5", 0},
  {"Hex_6", 0},
  {"Hex_7", 0},
  {"Hex_8", 0},
  {"Hex_9", 0},
  {"Hex_A", 0},
  {"Hex_B", 0},
  {"Hex_C", 0},
  {"Hex_D", 0},
  {"Hex_E", 0},
  {"Hex_F", 0},
  {"Scroll_Lock", 0},
  {"Show_Memory", 0},
  {"Show_Registers", 0},
  {"Control_backslash", 0},
  {"Compose", 0},

  {NULL, '\0'}
};

static holy_uint8_t linux_to_usb_map[128] = {
  /* 0x00 */ 0 /* Unused  */,               holy_KEYBOARD_KEY_ESCAPE,
  /* 0x02 */ holy_KEYBOARD_KEY_1,           holy_KEYBOARD_KEY_2,
  /* 0x04 */ holy_KEYBOARD_KEY_3,           holy_KEYBOARD_KEY_4,
  /* 0x06 */ holy_KEYBOARD_KEY_5,           holy_KEYBOARD_KEY_6,
  /* 0x08 */ holy_KEYBOARD_KEY_7,           holy_KEYBOARD_KEY_8,
  /* 0x0a */ holy_KEYBOARD_KEY_9,           holy_KEYBOARD_KEY_0,
  /* 0x0c */ holy_KEYBOARD_KEY_DASH,        holy_KEYBOARD_KEY_EQUAL,
  /* 0x0e */ holy_KEYBOARD_KEY_BACKSPACE,   holy_KEYBOARD_KEY_TAB,
  /* 0x10 */ holy_KEYBOARD_KEY_Q,           holy_KEYBOARD_KEY_W,
  /* 0x12 */ holy_KEYBOARD_KEY_E,           holy_KEYBOARD_KEY_R,
  /* 0x14 */ holy_KEYBOARD_KEY_T,           holy_KEYBOARD_KEY_Y,
  /* 0x16 */ holy_KEYBOARD_KEY_U,           holy_KEYBOARD_KEY_I,
  /* 0x18 */ holy_KEYBOARD_KEY_O,           holy_KEYBOARD_KEY_P,
  /* 0x1a */ holy_KEYBOARD_KEY_LBRACKET,    holy_KEYBOARD_KEY_RBRACKET,
  /* 0x1c */ holy_KEYBOARD_KEY_ENTER,       holy_KEYBOARD_KEY_LEFT_CTRL,
  /* 0x1e */ holy_KEYBOARD_KEY_A,           holy_KEYBOARD_KEY_S,
  /* 0x20 */ holy_KEYBOARD_KEY_D,           holy_KEYBOARD_KEY_F,
  /* 0x22 */ holy_KEYBOARD_KEY_G,           holy_KEYBOARD_KEY_H,
  /* 0x24 */ holy_KEYBOARD_KEY_J,           holy_KEYBOARD_KEY_K,
  /* 0x26 */ holy_KEYBOARD_KEY_L,           holy_KEYBOARD_KEY_SEMICOLON,
  /* 0x28 */ holy_KEYBOARD_KEY_DQUOTE,      holy_KEYBOARD_KEY_RQUOTE,
  /* 0x2a */ holy_KEYBOARD_KEY_LEFT_SHIFT,  holy_KEYBOARD_KEY_BACKSLASH,
  /* 0x2c */ holy_KEYBOARD_KEY_Z,           holy_KEYBOARD_KEY_X,
  /* 0x2e */ holy_KEYBOARD_KEY_C,           holy_KEYBOARD_KEY_V,
  /* 0x30 */ holy_KEYBOARD_KEY_B,           holy_KEYBOARD_KEY_N,
  /* 0x32 */ holy_KEYBOARD_KEY_M,           holy_KEYBOARD_KEY_COMMA,
  /* 0x34 */ holy_KEYBOARD_KEY_DOT,         holy_KEYBOARD_KEY_SLASH,
  /* 0x36 */ holy_KEYBOARD_KEY_RIGHT_SHIFT, holy_KEYBOARD_KEY_NUMMUL,
  /* 0x38 */ holy_KEYBOARD_KEY_LEFT_ALT,    holy_KEYBOARD_KEY_SPACE,
  /* 0x3a */ holy_KEYBOARD_KEY_CAPS_LOCK,   holy_KEYBOARD_KEY_F1,
  /* 0x3c */ holy_KEYBOARD_KEY_F2,          holy_KEYBOARD_KEY_F3,
  /* 0x3e */ holy_KEYBOARD_KEY_F4,          holy_KEYBOARD_KEY_F5,
  /* 0x40 */ holy_KEYBOARD_KEY_F6,          holy_KEYBOARD_KEY_F7,
  /* 0x42 */ holy_KEYBOARD_KEY_F8,          holy_KEYBOARD_KEY_F9,
  /* 0x44 */ holy_KEYBOARD_KEY_F10,         holy_KEYBOARD_KEY_NUM_LOCK,
  /* 0x46 */ holy_KEYBOARD_KEY_SCROLL_LOCK, holy_KEYBOARD_KEY_NUM7,
  /* 0x48 */ holy_KEYBOARD_KEY_NUM8,        holy_KEYBOARD_KEY_NUM9,
  /* 0x4a */ holy_KEYBOARD_KEY_NUMMINUS,    holy_KEYBOARD_KEY_NUM4,
  /* 0x4c */ holy_KEYBOARD_KEY_NUM5,        holy_KEYBOARD_KEY_NUM6,
  /* 0x4e */ holy_KEYBOARD_KEY_NUMPLUS,     holy_KEYBOARD_KEY_NUM1,
  /* 0x50 */ holy_KEYBOARD_KEY_NUM2,        holy_KEYBOARD_KEY_NUM3,
  /* 0x52 */ holy_KEYBOARD_KEY_NUMDOT,      holy_KEYBOARD_KEY_NUMDOT,
  /* 0x54 */ 0,                             0, 
  /* 0x56 */ holy_KEYBOARD_KEY_102ND,       holy_KEYBOARD_KEY_F11,
  /* 0x58 */ holy_KEYBOARD_KEY_F12,         holy_KEYBOARD_KEY_JP_RO,
  /* 0x5a */ 0,                             0,
  /* 0x5c */ 0,                             0,
  /* 0x5e */ 0,                             0,
  /* 0x60 */ holy_KEYBOARD_KEY_NUMENTER,    holy_KEYBOARD_KEY_RIGHT_CTRL,
  /* 0x62 */ holy_KEYBOARD_KEY_NUMSLASH,    0,
  /* 0x64 */ holy_KEYBOARD_KEY_RIGHT_ALT,   0,
  /* 0x66 */ holy_KEYBOARD_KEY_HOME,        holy_KEYBOARD_KEY_UP,
  /* 0x68 */ holy_KEYBOARD_KEY_PPAGE,       holy_KEYBOARD_KEY_LEFT,
  /* 0x6a */ holy_KEYBOARD_KEY_RIGHT,       holy_KEYBOARD_KEY_END,
  /* 0x6c */ holy_KEYBOARD_KEY_DOWN,        holy_KEYBOARD_KEY_NPAGE,
  /* 0x6e */ holy_KEYBOARD_KEY_INSERT,      holy_KEYBOARD_KEY_DELETE,
  /* 0x70 */ 0,                             0,
  /* 0x72 */ 0,                             holy_KEYBOARD_KEY_JP_RO,
  /* 0x74 */ 0,                             0,
  /* 0x76 */ 0,                             0,
  /* 0x78 */ 0,                             holy_KEYBOARD_KEY_KPCOMMA,
  /* 0x7a */ 0,                             0,
  /* 0x7c */ holy_KEYBOARD_KEY_JP_YEN,
}; 

static void
add_special_keys (struct holy_keyboard_layout *layout)
{
  (void) layout;
}

static unsigned
lookup (char *code, int shift)
{
  int i;
  struct console_holy_equivalence *pr;

  if (shift)
    pr = console_holy_equivalences_shift;
  else
    pr =  console_holy_equivalences_unshift;

  for (i = 0; pr[i].layout != NULL; i++)
    if (strcmp (code, pr[i].layout) == 0)
      return pr[i].holy;

  for (i = 0; console_holy_equivalences_common[i].layout != NULL; i++)
    if (strcmp (code, console_holy_equivalences_common[i].layout) == 0)
      return console_holy_equivalences_common[i].holy;

  /* TRANSLATORS: scan identifier is keyboard key symbolic name.  */
  fprintf (stderr, _("Unknown keyboard scan identifier %s\n"), code);

  return '\0';
}

static unsigned int
get_holy_code (char *layout_code, int shift)
{
  unsigned int code;

  if (strncmp (layout_code, "U+", sizeof ("U+") - 1) == 0)
    sscanf (layout_code, "U+%x", &code);
  else if (strncmp (layout_code, "+U+", sizeof ("+U+") - 1) == 0)
    sscanf (layout_code, "+U+%x", &code);
  else
    code = lookup (layout_code, shift);
  return code;
}

static void
write_file (FILE *out, const char *fname, struct holy_keyboard_layout *layout)
{
  holy_uint32_t version;
  unsigned i;

  version = holy_cpu_to_le32_compile_time (holy_KEYBOARD_LAYOUTS_VERSION);
  
  for (i = 0; i < ARRAY_SIZE (layout->keyboard_map); i++)
    layout->keyboard_map[i] = holy_cpu_to_le32(layout->keyboard_map[i]);

  for (i = 0; i < ARRAY_SIZE (layout->keyboard_map_shift); i++)
    layout->keyboard_map_shift[i]
      = holy_cpu_to_le32(layout->keyboard_map_shift[i]);

  for (i = 0; i < ARRAY_SIZE (layout->keyboard_map_l3); i++)
    layout->keyboard_map_l3[i]
      = holy_cpu_to_le32(layout->keyboard_map_l3[i]);

  for (i = 0; i < ARRAY_SIZE (layout->keyboard_map_shift_l3); i++)
    layout->keyboard_map_shift_l3[i]
      = holy_cpu_to_le32(layout->keyboard_map_shift_l3[i]);

  if (fwrite (holy_KEYBOARD_LAYOUTS_FILEMAGIC, 1,
	      holy_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE, out)
      != holy_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE
      || fwrite (&version, sizeof (version), 1, out) != 1
      || fwrite (layout, 1, sizeof (*layout), out) != sizeof (*layout))
    {
      if (fname)
	holy_util_error ("cannot write to `%s': %s", fname, strerror (errno));
      else
	holy_util_error ("cannot write to the stdout: %s", strerror (errno));
    }
}

static void
write_keymaps (FILE *in, FILE *out, const char *out_filename)
{
  struct holy_keyboard_layout layout;
  char line[2048];
  int ok;

  memset (&layout, 0, sizeof (layout));

  /* Process the ckbcomp output and prepare the layouts.  */
  ok = 0;
  while (fgets (line, sizeof (line), in))
    {
      if (strncmp (line, "keycode", sizeof ("keycode") - 1) == 0)
	{
	  unsigned keycode_linux;
	  unsigned keycode_usb;
	  char normal[64];
	  char shift[64];
	  char normalalt[64];
	  char shiftalt[64];

	  sscanf (line, "keycode %u = %60s %60s %60s %60s", &keycode_linux,
		  normal, shift, normalalt, shiftalt);

	  if (keycode_linux >= ARRAY_SIZE (linux_to_usb_map))
	    {
	      /* TRANSLATORS: scan code is keyboard key numeric identifier.  */
	      fprintf (stderr, _("Unknown keyboard scan code 0x%02x\n"), keycode_linux);
	      continue;
	    }

	  /* Not used.  */
	  if (keycode_linux == 0x77 /* Pause */
	      /* Some obscure keys */
	      || keycode_linux == 0x63 || keycode_linux == 0x7d
	      || keycode_linux == 0x7e)
	    continue;

	  /* Not remappable.  */
	  if (keycode_linux == 0x1d /* Left CTRL */
	      || keycode_linux == 0x61 /* Right CTRL */
	      || keycode_linux == 0x2a /* Left Shift. */
	      || keycode_linux == 0x36 /* Right Shift. */
	      || keycode_linux == 0x38 /* Left ALT. */
	      || keycode_linux == 0x64 /* Right ALT. */
	      || keycode_linux == 0x3a /* CapsLock. */
	      || keycode_linux == 0x45 /* NumLock. */
	      || keycode_linux == 0x46 /* ScrollLock. */)
	    continue;

	  keycode_usb = linux_to_usb_map[keycode_linux];
	  if (keycode_usb == 0
	      || keycode_usb >= holy_KEYBOARD_LAYOUTS_ARRAY_SIZE)
	    {
	      /* TRANSLATORS: scan code is keyboard key numeric identifier.  */
	      fprintf (stderr, _("Unknown keyboard scan code 0x%02x\n"), keycode_linux);
	      continue;
	    }
	  if (keycode_usb < holy_KEYBOARD_LAYOUTS_ARRAY_SIZE)
	    {
	      layout.keyboard_map[keycode_usb] = get_holy_code (normal, 0);
	      layout.keyboard_map_shift[keycode_usb] = get_holy_code (shift, 1);
	      layout.keyboard_map_l3[keycode_usb]
		= get_holy_code (normalalt, 0);
	      layout.keyboard_map_shift_l3[keycode_usb]
		= get_holy_code (shiftalt, 1);
	      ok = 1;
	    }
	}
    }

  if (ok == 0)
    {
      /* TRANSLATORS: this error is triggered when input doesn't contain any
	 key descriptions.  */
      fprintf (stderr, "%s", _("ERROR: no valid keyboard layout found. Check the input.\n"));
      exit (1);
    }

  add_special_keys (&layout);

  write_file (out, out_filename, &layout);
}

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'i':
      arguments->input = xstrdup (arg);
      break;

    case 'o':
      arguments->output = xstrdup (arg);
      break;

    case 'v':
      arguments->verbosity++;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("[OPTIONS]"),
  /* TRANSLATORS: "one" is a shortcut for "keyboard layout".  */
  N_("Generate holy keyboard layout from Linux console one."),
  NULL, NULL, NULL
};

int
main (int argc, char *argv[])
{
  FILE *in, *out;
  struct arguments arguments;

  holy_util_host_init (&argc, &argv);

  /* Check for options.  */
  memset (&arguments, 0, sizeof (struct arguments));
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  if (arguments.input)
    in = holy_util_fopen (arguments.input, "r");
  else
    in = stdin;

  if (!in)
    holy_util_error (_("cannot open `%s': %s"), arguments.input ? : "stdin",
		     strerror (errno));

  if (arguments.output)
    out = holy_util_fopen (arguments.output, "wb");
  else
    out = stdout;

  if (!out)
    {
      if (in != stdin)
	fclose (in);
      holy_util_error (_("cannot open `%s': %s"), arguments.output ? : "stdout",
		       strerror (errno));
    }

  write_keymaps (in, out, arguments.output);

  if (in != stdin)
    fclose (in);

  if (out != stdout)
    fclose (out);

  return 0;
}
