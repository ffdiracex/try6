/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/machine/console.h>
#include <holy/term.h>
#include <holy/types.h>
#include <holy/machine/int.h>

static holy_uint8_t holy_console_cur_color = 0x7;

static void
int10_9 (holy_uint8_t ch, holy_uint16_t n)
{
  struct holy_bios_int_registers regs;

  regs.eax = ch | 0x0900;
  regs.ebx = holy_console_cur_color & 0xff;
  regs.ecx = n;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
}

/*
 * BIOS call "INT 10H Function 03h" to get cursor position
 *	Call with	%ah = 0x03
 *			%bh = page
 *      Returns         %ch = starting scan line
 *                      %cl = ending scan line
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */


static struct holy_term_coordinate
holy_console_getxy (struct holy_term_output *term __attribute__ ((unused)))
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x0300;
  regs.ebx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  return (struct holy_term_coordinate) {
    (regs.edx & 0xff), ((regs.edx & 0xff00) >> 8) };
}

/*
 * BIOS call "INT 10H Function 02h" to set cursor position
 *	Call with	%ah = 0x02
 *			%bh = page
 *                      %dh = row (0 is top)
 *                      %dl = column (0 is left)
 */
static void
holy_console_gotoxy (struct holy_term_output *term __attribute__ ((unused)),
		     struct holy_term_coordinate pos)
{
  struct holy_bios_int_registers regs;

  /* set page to 0 */
  regs.ebx = 0;
  regs.eax = 0x0200;
  regs.edx = (pos.y << 8) | pos.x;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
}

/*
 *
 * Put the character C on the console. Because holy wants to write a
 * character with an attribute, this implementation is a bit tricky.
 * If C is a control character (CR, LF, BEL, BS), use INT 10, AH = 0Eh
 * (TELETYPE OUTPUT). Otherwise, use INT 10, AH = 9 to write character
 * with attributes and advance cursor. If we are on the last column,
 * let BIOS to wrap line correctly.
 */
static void
holy_console_putchar_real (holy_uint8_t c)
{
  struct holy_bios_int_registers regs;
  struct holy_term_coordinate pos;

  if (c == 7 || c == 8 || c == 0xa || c == 0xd)
    {
      regs.eax = c | 0x0e00;
      regs.ebx = 0x0001;
      regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
      holy_bios_interrupt (0x10, &regs);
      return;
    }

  /* get the current position */
  pos = holy_console_getxy (NULL);
  
  /* write the character with the attribute */
  int10_9 (c, 1);

  /* check the column with the width */
  if (pos.x >= 79)
    {
      holy_console_putchar_real (0x0d);
      holy_console_putchar_real (0x0a);
    }
  else
    holy_console_gotoxy (NULL, (struct holy_term_coordinate) { pos.x + 1,
	  pos.y });
}

static void
holy_console_putchar (struct holy_term_output *term __attribute__ ((unused)),
		      const struct holy_unicode_glyph *c)
{
  holy_console_putchar_real (c->base);
}

/*
 * BIOS call "INT 10H Function 09h" to write character and attribute
 *	Call with	%ah = 0x09
 *                      %al = (character)
 *                      %bh = (page number)
 *                      %bl = (attribute)
 *                      %cx = (number of times)
 */
static void
holy_console_cls (struct holy_term_output *term)
{
  /* move the cursor to the beginning */
  holy_console_gotoxy (term, (struct holy_term_coordinate) { 0, 0 });

  /* write spaces to the entire screen */
  int10_9 (' ', 80 * 25);

  /* move back the cursor */
  holy_console_gotoxy (term, (struct holy_term_coordinate) { 0, 0 });
}

/*
 * void holy_console_setcursor (int on)
 * BIOS call "INT 10H Function 01h" to set cursor type
 *      Call with       %ah = 0x01
 *                      %ch = cursor starting scanline
 *                      %cl = cursor ending scanline
 */
static void 
holy_console_setcursor (struct holy_term_output *term __attribute__ ((unused)),
			int on)
{
  static holy_uint16_t console_cursor_shape = 0;
  struct holy_bios_int_registers regs;

  /* check if the standard cursor shape has already been saved */
  if (!console_cursor_shape)
    {
      regs.eax = 0x0300;
      regs.ebx = 0;
      regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
      holy_bios_interrupt (0x10, &regs);
      console_cursor_shape = regs.ecx;
      if ((console_cursor_shape >> 8) >= (console_cursor_shape & 0xff))
	console_cursor_shape = 0x0d0e;
    }
  /* set %cx to the designated cursor shape */
  regs.ecx = on ? console_cursor_shape : 0x2000;
  regs.eax = 0x0100;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
}

/*
 *	if there is a character pending, return it; otherwise return -1
 * BIOS call "INT 16H Function 01H" to check whether a character is pending
 *	Call with	%ah = 0x1
 *	Return:
 *		If key waiting to be input:
 *			%ah = keyboard scan code
 *			%al = ASCII character
 *			Zero flag = clear
 *		else
 *			Zero flag = set
 * BIOS call "INT 16H Function 00H" to read character from keyboard
 *	Call with	%ah = 0x0
 *	Return:		%ah = keyboard scan code
 *			%al = ASCII character
 */

static int
holy_console_getkey (struct holy_term_input *term __attribute__ ((unused)))
{
  const holy_uint16_t bypass_table[] = {
    0x0100 | '\e', 0x0f00 | '\t', 0x0e00 | '\b', 0x1c00 | '\r', 0x1c00 | '\n'
  };
  struct holy_bios_int_registers regs;
  unsigned i;

  /*
   * Due to a bug in apple's bootcamp implementation, INT 16/AH = 0 would
   * cause the machine to hang at the second keystroke. However, we can
   * work around this problem by ensuring the presence of keystroke with
   * INT 16/AH = 1 before calling INT 16/AH = 0.
   */

  regs.eax = 0x0100;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x16, &regs);
  if (regs.flags & holy_CPU_INT_FLAGS_ZERO)
    return holy_TERM_NO_KEY;

  regs.eax = 0x0000;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x16, &regs);
  if (!(regs.eax & 0xff))
    return ((regs.eax >> 8) & 0xff) | holy_TERM_EXTENDED;

  if ((regs.eax & 0xff) >= ' ')
    return regs.eax & 0xff;

  for (i = 0; i < ARRAY_SIZE (bypass_table); i++)
    if (bypass_table[i] == (regs.eax & 0xffff))
      return regs.eax & 0xff;

  return (regs.eax & 0xff) + (('a' - 1) | holy_TERM_CTRL);
}

static const struct holy_machine_bios_data_area *bios_data_area =
  (struct holy_machine_bios_data_area *) holy_MEMORY_MACHINE_BIOS_DATA_AREA_ADDR;

static int
holy_console_getkeystatus (struct holy_term_input *term __attribute__ ((unused)))
{
  /* conveniently holy keystatus is modelled after BIOS one.  */
  return bios_data_area->keyboard_flag_lower & ~0x80;
}

static struct holy_term_coordinate
holy_console_getwh (struct holy_term_output *term __attribute__ ((unused)))
{
  return (struct holy_term_coordinate) { 80, 25 };
}

static void
holy_console_setcolorstate (struct holy_term_output *term
			    __attribute__ ((unused)),
			    holy_term_color_state state)
{
  switch (state) {
    case holy_TERM_COLOR_STANDARD:
      holy_console_cur_color = holy_TERM_DEFAULT_STANDARD_COLOR & 0x7f;
      break;
    case holy_TERM_COLOR_NORMAL:
      holy_console_cur_color = holy_term_normal_color & 0x7f;
      break;
    case holy_TERM_COLOR_HIGHLIGHT:
      holy_console_cur_color = holy_term_highlight_color & 0x7f;
      break;
    default:
      break;
  }
}

static struct holy_term_input holy_console_term_input =
  {
    .name = "console",
    .getkey = holy_console_getkey,
    .getkeystatus = holy_console_getkeystatus
  };

static struct holy_term_output holy_console_term_output =
  {
    .name = "console",
    .putchar = holy_console_putchar,
    .getwh = holy_console_getwh,
    .getxy = holy_console_getxy,
    .gotoxy = holy_console_gotoxy,
    .cls = holy_console_cls,
    .setcolorstate = holy_console_setcolorstate,
    .setcursor = holy_console_setcursor,
    .flags = holy_TERM_CODE_TYPE_CP437,
    .progress_update_divisor = holy_PROGRESS_FAST
  };

void
holy_console_init (void)
{
  holy_term_register_output ("console", &holy_console_term_output);
  holy_term_register_input ("console", &holy_console_term_input);
}

void
holy_console_fini (void)
{
  holy_term_unregister_input (&holy_console_term_input);
  holy_term_unregister_output (&holy_console_term_output);
}
