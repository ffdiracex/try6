/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/efi/efi.h>
#include <holy/efi/api.h>
#include <holy/efi/console.h>

static holy_uint32_t
map_char (holy_uint32_t c)
{
  /* Map some unicode characters to the EFI character.  */
  switch (c)
    {
    case holy_UNICODE_LEFTARROW:
      c = holy_UNICODE_BLACK_LEFT_TRIANGLE;
      break;
    case holy_UNICODE_UPARROW:
      c = holy_UNICODE_BLACK_UP_TRIANGLE;
      break;
    case holy_UNICODE_RIGHTARROW:
      c = holy_UNICODE_BLACK_RIGHT_TRIANGLE;
      break;
    case holy_UNICODE_DOWNARROW:
      c = holy_UNICODE_BLACK_DOWN_TRIANGLE;
      break;
    case holy_UNICODE_HLINE:
      c = holy_UNICODE_LIGHT_HLINE;
      break;
    case holy_UNICODE_VLINE:
      c = holy_UNICODE_LIGHT_VLINE;
      break;
    case holy_UNICODE_CORNER_UL:
      c = holy_UNICODE_LIGHT_CORNER_UL;
      break;
    case holy_UNICODE_CORNER_UR:
      c = holy_UNICODE_LIGHT_CORNER_UR;
      break;
    case holy_UNICODE_CORNER_LL:
      c = holy_UNICODE_LIGHT_CORNER_LL;
      break;
    case holy_UNICODE_CORNER_LR:
      c = holy_UNICODE_LIGHT_CORNER_LR;
      break;
    }

  return c;
}

static void
holy_console_putchar (struct holy_term_output *term __attribute__ ((unused)),
		      const struct holy_unicode_glyph *c)
{
  holy_efi_char16_t str[2 + 30];
  holy_efi_simple_text_output_interface_t *o;
  unsigned i, j;

  if (holy_efi_is_finished)
    return;

  o = holy_efi_system_table->con_out;

  /* For now, do not try to use a surrogate pair.  */
  if (c->base > 0xffff)
    str[0] = '?';
  else
    str[0] = (holy_efi_char16_t)  map_char (c->base & 0xffff);
  j = 1;
  for (i = 0; i < c->ncomb && j + 1 < ARRAY_SIZE (str); i++)
    if (c->base < 0xffff)
      str[j++] = holy_unicode_get_comb (c)[i].code;
  str[j] = 0;

  /* Should this test be cached?  */
  if ((c->base > 0x7f || c->ncomb)
      && efi_call_2 (o->test_string, o, str) != holy_EFI_SUCCESS)
    return;

  efi_call_2 (o->output_string, o, str);
}

const unsigned efi_codes[] =
  {
    0, holy_TERM_KEY_UP, holy_TERM_KEY_DOWN, holy_TERM_KEY_RIGHT,
    holy_TERM_KEY_LEFT, holy_TERM_KEY_HOME, holy_TERM_KEY_END, holy_TERM_KEY_INSERT,
    holy_TERM_KEY_DC, holy_TERM_KEY_PPAGE, holy_TERM_KEY_NPAGE, holy_TERM_KEY_F1,
    holy_TERM_KEY_F2, holy_TERM_KEY_F3, holy_TERM_KEY_F4, holy_TERM_KEY_F5,
    holy_TERM_KEY_F6, holy_TERM_KEY_F7, holy_TERM_KEY_F8, holy_TERM_KEY_F9,
    holy_TERM_KEY_F10, holy_TERM_KEY_F11, holy_TERM_KEY_F12, '\e'
  };

static int
holy_efi_translate_key (holy_efi_input_key_t key)
{
  if (key.scan_code == 0)
    {
      /* Some firmware implementations use VT100-style codes against the spec.
	 This is especially likely if driven by serial.
       */
      if (key.unicode_char < 0x20 && key.unicode_char != 0
	  && key.unicode_char != '\t' && key.unicode_char != '\b'
	  && key.unicode_char != '\n' && key.unicode_char != '\r')
	return holy_TERM_CTRL | (key.unicode_char - 1 + 'a');
      else
	return key.unicode_char;
    }
  else if (key.scan_code < ARRAY_SIZE (efi_codes))
    return efi_codes[key.scan_code];

  if ((key.unicode_char >= 0x20 && key.unicode_char <= 0x7f)
      || key.unicode_char == '\t' || key.unicode_char == '\b'
      || key.unicode_char == '\n' || key.unicode_char == '\r')
    return key.unicode_char;

  return holy_TERM_NO_KEY;
}

static int
holy_console_getkey_con (struct holy_term_input *term __attribute__ ((unused)))
{
  holy_efi_simple_input_interface_t *i;
  holy_efi_input_key_t key;
  holy_efi_status_t status;

  i = holy_efi_system_table->con_in;
  status = efi_call_2 (i->read_key_stroke, i, &key);

  if (status != holy_EFI_SUCCESS)
    return holy_TERM_NO_KEY;

  return holy_efi_translate_key(key);
}

static int
holy_console_getkey_ex(struct holy_term_input *term)
{
  holy_efi_key_data_t key_data;
  holy_efi_status_t status;
  holy_efi_uint32_t kss;
  int key = -1;

  holy_efi_simple_text_input_ex_interface_t *text_input = term->data;

  status = efi_call_2 (text_input->read_key_stroke, text_input, &key_data);

  if (status != holy_EFI_SUCCESS)
    return holy_TERM_NO_KEY;

  kss = key_data.key_state.key_shift_state;
  key = holy_efi_translate_key(key_data.key);

  if (key == holy_TERM_NO_KEY)
    return holy_TERM_NO_KEY;

  if (kss & holy_EFI_SHIFT_STATE_VALID)
    {
      if ((kss & holy_EFI_LEFT_SHIFT_PRESSED
	   || kss & holy_EFI_RIGHT_SHIFT_PRESSED)
	  && (key & holy_TERM_EXTENDED))
	key |= holy_TERM_SHIFT;
      if (kss & holy_EFI_LEFT_ALT_PRESSED || kss & holy_EFI_RIGHT_ALT_PRESSED)
	key |= holy_TERM_ALT;
      if (kss & holy_EFI_LEFT_CONTROL_PRESSED
	  || kss & holy_EFI_RIGHT_CONTROL_PRESSED)
	key |= holy_TERM_CTRL;
    }

  return key;
}

static holy_err_t
holy_efi_console_input_init (struct holy_term_input *term)
{
  holy_efi_guid_t text_input_ex_guid =
    holy_EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

  if (holy_efi_is_finished)
    return 0;

  holy_efi_simple_text_input_ex_interface_t *text_input = term->data;
  if (text_input)
    return 0;

  text_input = holy_efi_open_protocol(holy_efi_system_table->console_in_handler,
				      &text_input_ex_guid,
				      holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  term->data = (void *)text_input;

  return 0;
}

static int
holy_console_getkey (struct holy_term_input *term)
{
  if (holy_efi_is_finished)
    return 0;

  if (term->data)
    return holy_console_getkey_ex(term);
  else
    return holy_console_getkey_con(term);
}

static struct holy_term_coordinate
holy_console_getwh (struct holy_term_output *term __attribute__ ((unused)))
{
  holy_efi_simple_text_output_interface_t *o;
  holy_efi_uintn_t columns, rows;

  o = holy_efi_system_table->con_out;
  if (holy_efi_is_finished || efi_call_4 (o->query_mode, o, o->mode->mode,
					  &columns, &rows) != holy_EFI_SUCCESS)
    {
      /* Why does this fail?  */
      columns = 80;
      rows = 25;
    }

  return (struct holy_term_coordinate) { columns, rows };
}

static struct holy_term_coordinate
holy_console_getxy (struct holy_term_output *term __attribute__ ((unused)))
{
  holy_efi_simple_text_output_interface_t *o;

  if (holy_efi_is_finished)
    return (struct holy_term_coordinate) { 0, 0 };

  o = holy_efi_system_table->con_out;
  return (struct holy_term_coordinate) { o->mode->cursor_column, o->mode->cursor_row };
}

static void
holy_console_gotoxy (struct holy_term_output *term __attribute__ ((unused)),
		     struct holy_term_coordinate pos)
{
  holy_efi_simple_text_output_interface_t *o;

  if (holy_efi_is_finished)
    return;

  o = holy_efi_system_table->con_out;
  efi_call_3 (o->set_cursor_position, o, pos.x, pos.y);
}

static void
holy_console_cls (struct holy_term_output *term __attribute__ ((unused)))
{
  holy_efi_simple_text_output_interface_t *o;
  holy_efi_int32_t orig_attr;

  if (holy_efi_is_finished)
    return;

  o = holy_efi_system_table->con_out;
  orig_attr = o->mode->attribute;
  efi_call_2 (o->set_attributes, o, holy_EFI_BACKGROUND_BLACK);
  efi_call_1 (o->clear_screen, o);
  efi_call_2 (o->set_attributes, o, orig_attr);
}

static void
holy_console_setcolorstate (struct holy_term_output *term
			    __attribute__ ((unused)),
			    holy_term_color_state state)
{
  holy_efi_simple_text_output_interface_t *o;

  if (holy_efi_is_finished)
    return;

  o = holy_efi_system_table->con_out;

  switch (state) {
    case holy_TERM_COLOR_STANDARD:
      efi_call_2 (o->set_attributes, o, holy_TERM_DEFAULT_STANDARD_COLOR
		  & 0x7f);
      break;
    case holy_TERM_COLOR_NORMAL:
      efi_call_2 (o->set_attributes, o, holy_term_normal_color & 0x7f);
      break;
    case holy_TERM_COLOR_HIGHLIGHT:
      efi_call_2 (o->set_attributes, o, holy_term_highlight_color & 0x7f);
      break;
    default:
      break;
  }
}

static void
holy_console_setcursor (struct holy_term_output *term __attribute__ ((unused)),
			int on)
{
  holy_efi_simple_text_output_interface_t *o;

  if (holy_efi_is_finished)
    return;

  o = holy_efi_system_table->con_out;
  efi_call_2 (o->enable_cursor, o, on);
}

static holy_err_t
holy_efi_console_output_init (struct holy_term_output *term)
{
  holy_efi_set_text_mode (1);
  holy_console_setcursor (term, 1);
  return 0;
}

static holy_err_t
holy_efi_console_output_fini (struct holy_term_output *term)
{
  holy_console_setcursor (term, 0);
  holy_efi_set_text_mode (0);
  return 0;
}

static struct holy_term_input holy_console_term_input =
  {
    .name = "console",
    .getkey = holy_console_getkey,
    .init = holy_efi_console_input_init,
  };

static struct holy_term_output holy_console_term_output =
  {
    .name = "console",
    .init = holy_efi_console_output_init,
    .fini = holy_efi_console_output_fini,
    .putchar = holy_console_putchar,
    .getwh = holy_console_getwh,
    .getxy = holy_console_getxy,
    .gotoxy = holy_console_gotoxy,
    .cls = holy_console_cls,
    .setcolorstate = holy_console_setcolorstate,
    .setcursor = holy_console_setcursor,
    .flags = holy_TERM_CODE_TYPE_VISUAL_GLYPHS,
    .progress_update_divisor = holy_PROGRESS_FAST
  };

void
holy_console_init (void)
{
  /* FIXME: it is necessary to consider the case where no console control
     is present but the default is already in text mode.  */
  if (! holy_efi_set_text_mode (1))
    {
      holy_error (holy_ERR_BAD_DEVICE, "cannot set text mode");
      return;
    }

  holy_term_register_output ("console", &holy_console_term_output);
  holy_term_register_input ("console", &holy_console_term_input);
}

void
holy_console_fini (void)
{
  holy_term_unregister_input (&holy_console_term_input);
  holy_term_unregister_output (&holy_console_term_output);
}
