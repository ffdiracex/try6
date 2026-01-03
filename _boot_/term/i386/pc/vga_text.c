/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/cpu/io.h>
#include <holy/types.h>
#include <holy/vga.h>
#include <holy/term.h>

#if defined (holy_MACHINE_COREBOOT)
#include <holy/machine/console.h>
#endif

/* MODESET is used for testing to force monochrome or colour mode.
   You shouldn't use mda_text on vga.
 */
#ifdef MODESET
#include <holy/machine/int.h>
#endif

#if defined (holy_MACHINE_COREBOOT) || defined (holy_MACHINE_QEMU) || defined (holy_MACHINE_MIPS_QEMU_MIPS) || defined (holy_MACHINE_MULTIBOOT)
#include <holy/machine/console.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

#define COLS	80
#define ROWS	25

static struct holy_term_coordinate holy_curr_pos;

#ifdef __mips__
#define VGA_TEXT_SCREEN		((holy_uint16_t *) 0xb00b8000)
#define cr_read holy_vga_cr_read
#define cr_write holy_vga_cr_write
#elif defined (MODE_MDA)
#define VGA_TEXT_SCREEN		((holy_uint16_t *) 0xb0000)
#define cr_read holy_vga_cr_bw_read
#define cr_write holy_vga_cr_bw_write
#else
#define VGA_TEXT_SCREEN		((holy_uint16_t *) 0xb8000)
#define cr_read holy_vga_cr_read
#define cr_write holy_vga_cr_write
#endif

static holy_uint8_t cur_color = 0x7;

static void
screen_write_char (int x, int y, short c)
{
  VGA_TEXT_SCREEN[y * COLS + x] = holy_cpu_to_le16 (c);
}

static short
screen_read_char (int x, int y)
{
  return holy_le_to_cpu16 (VGA_TEXT_SCREEN[y * COLS + x]);
}

static void
update_cursor (void)
{
  unsigned int pos = holy_curr_pos.y * COLS + holy_curr_pos.x;
  cr_write (pos >> 8, holy_VGA_CR_CURSOR_ADDR_HIGH);
  cr_write (pos & 0xFF, holy_VGA_CR_CURSOR_ADDR_LOW);
}

static void
inc_y (void)
{
  holy_curr_pos.x = 0;
  if (holy_curr_pos.y < ROWS - 1)
    holy_curr_pos.y++;
  else
    {
      int x, y;
      for (y = 0; y < ROWS - 1; y++)
        for (x = 0; x < COLS; x++)
          screen_write_char (x, y, screen_read_char (x, y + 1));
      for (x = 0; x < COLS; x++)
	screen_write_char (x, ROWS - 1, ' ' | (cur_color << 8));
    }
}

static void
inc_x (void)
{
  if (holy_curr_pos.x >= COLS - 1)
    inc_y ();
  else
    holy_curr_pos.x++;
}

static void
holy_vga_text_putchar (struct holy_term_output *term __attribute__ ((unused)),
		       const struct holy_unicode_glyph *c)
{
  switch (c->base)
    {
      case '\b':
	if (holy_curr_pos.x != 0)
	  screen_write_char (holy_curr_pos.x--, holy_curr_pos.y, ' ');
	break;
      case '\n':
	inc_y ();
	break;
      case '\r':
	holy_curr_pos.x = 0;
	break;
      default:
	screen_write_char (holy_curr_pos.x, holy_curr_pos.y,
			   c->base | (cur_color << 8));
	inc_x ();
    }

  update_cursor ();
}

static struct holy_term_coordinate
holy_vga_text_getxy (struct holy_term_output *term __attribute__ ((unused)))
{
  return holy_curr_pos;
}

static void
holy_vga_text_gotoxy (struct holy_term_output *term __attribute__ ((unused)),
		      struct holy_term_coordinate pos)
{
  holy_curr_pos = pos;
  update_cursor ();
}

static void
holy_vga_text_cls (struct holy_term_output *term)
{
  int i;
  for (i = 0; i < ROWS * COLS; i++)
    VGA_TEXT_SCREEN[i] = holy_cpu_to_le16 (' ' | (cur_color << 8));
  holy_vga_text_gotoxy (term, (struct holy_term_coordinate) { 0, 0 });
}

static void
holy_vga_text_setcursor (struct holy_term_output *term __attribute__ ((unused)),
			 int on)
{
  holy_uint8_t old;
  old = cr_read (holy_VGA_CR_CURSOR_START);
  if (on)
    cr_write (old & ~holy_VGA_CR_CURSOR_START_DISABLE,
	      holy_VGA_CR_CURSOR_START);
  else
    cr_write (old | holy_VGA_CR_CURSOR_START_DISABLE,
	      holy_VGA_CR_CURSOR_START);
}

static holy_err_t
holy_vga_text_init_real (struct holy_term_output *term)
{
#ifdef MODESET
  struct holy_bios_int_registers regs;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;

#ifdef MODE_MDA
  regs.eax = 7;
#else
  regs.eax = 3;
#endif
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
#endif
  holy_vga_text_cls (term);
  return 0;
}

static holy_err_t
holy_vga_text_fini_real (struct holy_term_output *term)
{
#ifdef MODESET
  struct holy_bios_int_registers regs;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;

  regs.eax = 3;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
#endif
  holy_vga_text_cls (term);
  return 0;
}

static struct holy_term_coordinate
holy_vga_text_getwh (struct holy_term_output *term __attribute__ ((unused)))
{
  return (struct holy_term_coordinate) { 80, 25 };
}

#ifndef MODE_MDA

static void
holy_vga_text_setcolorstate (struct holy_term_output *term __attribute__ ((unused)),
			     holy_term_color_state state)
{
  switch (state) {
    case holy_TERM_COLOR_STANDARD:
      cur_color = holy_TERM_DEFAULT_STANDARD_COLOR & 0x7f;
      break;
    case holy_TERM_COLOR_NORMAL:
      cur_color = holy_term_normal_color & 0x7f;
      break;
    case holy_TERM_COLOR_HIGHLIGHT:
      cur_color = holy_term_highlight_color & 0x7f;
      break;
    default:
      break;
  }
}

#else
static void
holy_vga_text_setcolorstate (struct holy_term_output *term __attribute__ ((unused)),
			     holy_term_color_state state)
{
  switch (state) {
    case holy_TERM_COLOR_STANDARD:
      cur_color = 0x07;
      break;
    case holy_TERM_COLOR_NORMAL:
      cur_color = 0x07;
      break;
    case holy_TERM_COLOR_HIGHLIGHT:
      cur_color = 0x70;
      break;
    default:
      break;
  }
}
#endif

static struct holy_term_output holy_vga_text_term =
  {
#ifdef MODE_MDA
    .name = "mda_text",
#else
    .name = "vga_text",
#endif
    .init = holy_vga_text_init_real,
    .fini = holy_vga_text_fini_real,
    .putchar = holy_vga_text_putchar,
    .getwh = holy_vga_text_getwh,
    .getxy = holy_vga_text_getxy,
    .gotoxy = holy_vga_text_gotoxy,
    .cls = holy_vga_text_cls,
    .setcolorstate = holy_vga_text_setcolorstate,
    .setcursor = holy_vga_text_setcursor,
    .flags = holy_TERM_CODE_TYPE_CP437,
    .progress_update_divisor = holy_PROGRESS_FAST
  };

#ifndef MODE_MDA

holy_MOD_INIT(vga_text)
{
#ifdef holy_MACHINE_COREBOOT
  if (!holy_video_coreboot_fbtable)
#endif
    holy_term_register_output ("vga_text", &holy_vga_text_term);
}

holy_MOD_FINI(vga_text)
{
  holy_term_unregister_output (&holy_vga_text_term);
}

#endif
