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
#include <holy/dl.h>
#include <holy/keyboard_layouts.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/file.h>

holy_MOD_LICENSE ("GPLv2+");

static struct holy_keyboard_layout layout_us = {
  .keyboard_map = {
    /* Keyboard errors. Handled by driver.  */
    /* 0x00 */   0,   0,   0,   0,

    /* 0x04 */ 'a',  'b',  'c',  'd', 
    /* 0x08 */ 'e',  'f',  'g',  'h',  'i', 'j', 'k', 'l',
    /* 0x10 */ 'm',  'n',  'o',  'p',  'q', 'r', 's', 't',
    /* 0x18 */ 'u',  'v',  'w',  'x',  'y', 'z', '1', '2',
    /* 0x20 */ '3',  '4',  '5',  '6',  '7', '8', '9', '0',
    /* 0x28 */ '\n', '\e', '\b', '\t', ' ', '-', '=', '[',
    /* According to usage table 0x31 should be mapped to '/'
       but testing with real keyboard shows that 0x32 is remapped to '/'.
       Map 0x31 to 0. 
    */
    /* 0x30 */ ']',   0,   '\\', ';', '\'', '`', ',', '.',
    /* 0x39 is CapsLock. Handled by driver.  */
    /* 0x38 */ '/',   0,   holy_TERM_KEY_F1, holy_TERM_KEY_F2,
    /* 0x3c */ holy_TERM_KEY_F3,     holy_TERM_KEY_F4,
    /* 0x3e */ holy_TERM_KEY_F5,     holy_TERM_KEY_F6,
    /* 0x40 */ holy_TERM_KEY_F7,     holy_TERM_KEY_F8,
    /* 0x42 */ holy_TERM_KEY_F9,     holy_TERM_KEY_F10,
    /* 0x44 */ holy_TERM_KEY_F11,    holy_TERM_KEY_F12,
    /* PrtScr and ScrollLock. Not handled yet.  */
    /* 0x46 */ 0,                    0,
    /* 0x48 is Pause. Not handled yet.  */
    /* 0x48 */ 0,                    holy_TERM_KEY_INSERT,
    /* 0x4a */ holy_TERM_KEY_HOME,   holy_TERM_KEY_PPAGE,
    /* 0x4c */ holy_TERM_KEY_DC,     holy_TERM_KEY_END,
    /* 0x4e */ holy_TERM_KEY_NPAGE,  holy_TERM_KEY_RIGHT,
    /* 0x50 */ holy_TERM_KEY_LEFT,   holy_TERM_KEY_DOWN,
    /* 0x53 is NumLock. Handled by driver.  */
    /* 0x52 */ holy_TERM_KEY_UP,     0,
    /* 0x54 */ '/',                  '*', 
    /* 0x56 */ '-',                  '+',
    /* 0x58 */ '\n',                 holy_TERM_KEY_END,
    /* 0x5a */ holy_TERM_KEY_DOWN,   holy_TERM_KEY_NPAGE,
    /* 0x5c */ holy_TERM_KEY_LEFT,   holy_TERM_KEY_CENTER,
    /* 0x5e */ holy_TERM_KEY_RIGHT,  holy_TERM_KEY_HOME,
    /* 0x60 */ holy_TERM_KEY_UP,     holy_TERM_KEY_PPAGE,
    /* 0x62 */ holy_TERM_KEY_INSERT, holy_TERM_KEY_DC,
    /* 0x64 */ '\\'
  },
  .keyboard_map_shift = {
    /* Keyboard errors. Handled by driver.  */
    /* 0x00 */   0,   0,   0,   0,

    /* 0x04 */ 'A',  'B',  'C',  'D', 
    /* 0x08 */ 'E',  'F',  'G',  'H',  'I', 'J', 'K', 'L',
    /* 0x10 */ 'M',  'N',  'O',  'P',  'Q', 'R', 'S', 'T',
    /* 0x18 */ 'U',  'V',  'W',  'X',  'Y', 'Z', '!', '@',
    /* 0x20 */ '#',  '$',  '%',  '^',  '&', '*', '(', ')',
    /* 0x28 */ '\n' | holy_TERM_SHIFT, '\e' | holy_TERM_SHIFT,
    /* 0x2a */ '\b' | holy_TERM_SHIFT, '\t' | holy_TERM_SHIFT,
    /* 0x2c */ ' '  | holy_TERM_SHIFT,  '_', '+', '{',
    /* According to usage table 0x31 should be mapped to '/'
       but testing with real keyboard shows that 0x32 is remapped to '/'.
       Map 0x31 to 0. 
    */
    /* 0x30 */ '}',  0,    '|',  ':',  '"', '~', '<', '>',
    /* 0x39 is CapsLock. Handled by driver.  */
    /* 0x38 */ '?',  0,
    /* 0x3a */ holy_TERM_KEY_F1 | holy_TERM_SHIFT,
    /* 0x3b */ holy_TERM_KEY_F2 | holy_TERM_SHIFT,
    /* 0x3c */ holy_TERM_KEY_F3 | holy_TERM_SHIFT,
    /* 0x3d */ holy_TERM_KEY_F4 | holy_TERM_SHIFT,
    /* 0x3e */ holy_TERM_KEY_F5 | holy_TERM_SHIFT,
    /* 0x3f */ holy_TERM_KEY_F6 | holy_TERM_SHIFT,
    /* 0x40 */ holy_TERM_KEY_F7 | holy_TERM_SHIFT,
    /* 0x41 */ holy_TERM_KEY_F8 | holy_TERM_SHIFT,
    /* 0x42 */ holy_TERM_KEY_F9 | holy_TERM_SHIFT,
    /* 0x43 */ holy_TERM_KEY_F10 | holy_TERM_SHIFT,
    /* 0x44 */ holy_TERM_KEY_F11 | holy_TERM_SHIFT,
    /* 0x45 */ holy_TERM_KEY_F12 | holy_TERM_SHIFT,
    /* PrtScr and ScrollLock. Not handled yet.  */
    /* 0x46 */ 0,                    0,
    /* 0x48 is Pause. Not handled yet.  */
    /* 0x48 */ 0,                    holy_TERM_KEY_INSERT | holy_TERM_SHIFT,
    /* 0x4a */ holy_TERM_KEY_HOME | holy_TERM_SHIFT,
    /* 0x4b */ holy_TERM_KEY_PPAGE | holy_TERM_SHIFT,
    /* 0x4c */ holy_TERM_KEY_DC | holy_TERM_SHIFT,
    /* 0x4d */ holy_TERM_KEY_END | holy_TERM_SHIFT,
    /* 0x4e */ holy_TERM_KEY_NPAGE | holy_TERM_SHIFT,
    /* 0x4f */ holy_TERM_KEY_RIGHT | holy_TERM_SHIFT,
    /* 0x50 */ holy_TERM_KEY_LEFT | holy_TERM_SHIFT,
    /* 0x51 */ holy_TERM_KEY_DOWN | holy_TERM_SHIFT,
    /* 0x53 is NumLock. Handled by driver.  */
    /* 0x52 */ holy_TERM_KEY_UP | holy_TERM_SHIFT,     0,
    /* 0x54 */ '/',                    '*', 
    /* 0x56 */ '-',                    '+',
    /* 0x58 */ '\n' | holy_TERM_SHIFT, '1', '2', '3', '4', '5','6', '7',
    /* 0x60 */ '8', '9', '0', '.', '|'
  }
};

static struct holy_keyboard_layout *holy_current_layout = &layout_us;

static int
map_key_core (int code, int status, int *alt_gr_consumed)
{
  *alt_gr_consumed = 0;

  if (code >= holy_KEYBOARD_LAYOUTS_ARRAY_SIZE)
    return 0;

  if (status & holy_TERM_STATUS_RALT)
    {
      if (status & (holy_TERM_STATUS_LSHIFT | holy_TERM_STATUS_RSHIFT))
	{
	  if (holy_current_layout->keyboard_map_shift_l3[code])
	    {
	      *alt_gr_consumed = 1;
	      return holy_current_layout->keyboard_map_shift_l3[code];
	    }
	}
      else if (holy_current_layout->keyboard_map_l3[code])
	{
	  *alt_gr_consumed = 1;
	  return holy_current_layout->keyboard_map_l3[code];
	}
    }
  if (status & (holy_TERM_STATUS_LSHIFT | holy_TERM_STATUS_RSHIFT))
    return holy_current_layout->keyboard_map_shift[code];
  else
    return holy_current_layout->keyboard_map[code];
}

unsigned
holy_term_map_key (holy_keyboard_key_t code, int status)
{
  int alt_gr_consumed = 0;
  int key;

  if (code >= 0x59 && code <= 0x63 && (status & holy_TERM_STATUS_NUM))
    {
      if (status & (holy_TERM_STATUS_RSHIFT | holy_TERM_STATUS_LSHIFT))
	status &= ~(holy_TERM_STATUS_RSHIFT | holy_TERM_STATUS_LSHIFT);
      else
	status |= holy_TERM_STATUS_RSHIFT;
    }

  key = map_key_core (code, status, &alt_gr_consumed);
  
  if (key == 0 || key == holy_TERM_SHIFT) {
    holy_printf ("Unknown key 0x%x detected\n", code);
    return holy_TERM_NO_KEY;
  }
  
  if (status & holy_TERM_STATUS_CAPS)
    {
      if ((key >= 'a') && (key <= 'z'))
	key += 'A' - 'a';
      else if ((key >= 'A') && (key <= 'Z'))
	key += 'a' - 'A';
    }
  
  if ((status & holy_TERM_STATUS_LALT) ||
      ((status & holy_TERM_STATUS_RALT) && !alt_gr_consumed))
    key |= holy_TERM_ALT;
  if (status & (holy_TERM_STATUS_LCTRL | holy_TERM_STATUS_RCTRL))
    key |= holy_TERM_CTRL;

  return key;
}

static holy_err_t
holy_cmd_keymap (struct holy_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  char *filename;
  holy_file_t file;
  holy_uint32_t version;
  holy_uint8_t magic[holy_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE];
  struct holy_keyboard_layout *newmap = NULL;
  unsigned i;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "file or layout name required");
  if (argv[0][0] != '(' && argv[0][0] != '/' && argv[0][0] != '+')
    {
      const char *prefix = holy_env_get ("prefix");
      if (!prefix)
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("variable `%s' isn't set"), "prefix");
      filename = holy_xasprintf ("%s/layouts/%s.gkb", prefix, argv[0]);
      if (!filename)
	return holy_errno;
    }
  else
    filename = argv[0];

  file = holy_file_open (filename);
  if (! file)
    goto fail;

  if (holy_file_read (file, magic, sizeof (magic)) != sizeof (magic))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_ARGUMENT, N_("premature end of file %s"),
		    filename);
      goto fail;
    }

  if (holy_memcmp (magic, holy_KEYBOARD_LAYOUTS_FILEMAGIC,
		   holy_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE) != 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, "invalid magic");
      goto fail;
    }

  if (holy_file_read (file, &version, sizeof (version)) != sizeof (version))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_ARGUMENT, N_("premature end of file %s"),
		    filename);
      goto fail;
    }

  if (version != holy_cpu_to_le32_compile_time (holy_KEYBOARD_LAYOUTS_VERSION))
    {
      holy_error (holy_ERR_BAD_ARGUMENT, "invalid version");
      goto fail;
    }

  newmap = holy_malloc (sizeof (*newmap));
  if (!newmap)
    goto fail;

  if (holy_file_read (file, newmap, sizeof (*newmap)) != sizeof (*newmap))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_ARGUMENT, N_("premature end of file %s"),
		    filename);
      goto fail;
    }

  for (i = 0; i < ARRAY_SIZE (newmap->keyboard_map); i++)
    newmap->keyboard_map[i] = holy_le_to_cpu32(newmap->keyboard_map[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->keyboard_map_shift); i++)
    newmap->keyboard_map_shift[i]
      = holy_le_to_cpu32(newmap->keyboard_map_shift[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->keyboard_map_l3); i++)
    newmap->keyboard_map_l3[i]
      = holy_le_to_cpu32(newmap->keyboard_map_l3[i]);

  for (i = 0; i < ARRAY_SIZE (newmap->keyboard_map_shift_l3); i++)
    newmap->keyboard_map_shift_l3[i]
      = holy_le_to_cpu32(newmap->keyboard_map_shift_l3[i]);

  holy_current_layout = newmap;

  return holy_ERR_NONE;

 fail:
  if (filename != argv[0])
    holy_free (filename);
  holy_free (newmap);
  if (file)
    holy_file_close (file);
  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(keylayouts)
{
  cmd = holy_register_command ("keymap", holy_cmd_keymap,
			       0, N_("Load a keyboard layout."));
}

holy_MOD_FINI(keylayouts)
{
  holy_unregister_command (cmd);
}
