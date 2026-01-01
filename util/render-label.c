/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/misc.h>
#include <holy/util/install.h>
#include <holy/i18n.h>
#include <holy/term.h>
#include <holy/font.h>
#include <holy/gfxmenu_view.h>
#include <holy/color.h>
#include <holy/emu/hostdisk.h>

#define _GNU_SOURCE	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

struct header
{
  holy_uint8_t magic;
  holy_uint16_t width;
  holy_uint16_t height;
} holy_PACKED;

static struct holy_video_palette_data ieee1275_palette[256];

void
holy_util_render_label (const char *label_font,
			const char *label_bgcolor,
			const char *label_color,
			const char *text,
			const char *output)
{
  struct header head;
  FILE *out;
  int i, j, k, cptr = 0;
  holy_font_t font;
  char *fontfull;
  const holy_uint8_t vals[] = { 0xff, 0xda, 0xb3, 0x87, 0x54, 0x00 };
  const holy_uint8_t vals2[] = { 0xf3, 0xe7, 0xcd, 0xc0, 0xa5, 0x96,
				 0x77, 0x66, 0x3f, 0x27 };
  int width, height;
  holy_uint8_t bg, fg;
  struct holy_video_mode_info mode_info;
  holy_err_t err;
  holy_video_rgba_color_t fgcolor;
  holy_video_rgba_color_t bgcolor;

  if (output)
    out = holy_util_fopen (output, "wb");
  else
    out = stdout;
  if (!out)
    {
      holy_util_error (_("cannot open `%s': %s"), output ? : "stdout",
		       strerror (errno));
    }

  if (label_color)
    {
      err = holy_video_parse_color (label_color, &fgcolor);
      if (err)
	holy_util_error (_("invalid color specification `%s'"), label_color);
    }
  else
    {
      fgcolor.red = 0x00;
      fgcolor.green = 0x00;
      fgcolor.blue = 0x00;
      fgcolor.alpha = 0xff;
    }

  if (label_bgcolor)
    {
      err = holy_video_parse_color (label_bgcolor, &bgcolor);
      if (err)
	holy_util_error (_("invalid color specification `%s'"), label_bgcolor);
    }
  else
    {
      bgcolor.red = 0xff;
      bgcolor.green = 0xff;
      bgcolor.blue = 0xff;
      bgcolor.alpha = 0xff;
    }

  for (i = 0; i < 256; i++)
    ieee1275_palette[i].a = 0xff;

  for (i = 0; i < 6; i++)
    for (j = 0; j < 6; j++)
      for (k = 0; k < 6; k++)
	{
	  ieee1275_palette[cptr].r = vals[i];
	  ieee1275_palette[cptr].g = vals[j];
	  ieee1275_palette[cptr].b = vals[k];
	  ieee1275_palette[cptr].a = 0xff;
	  cptr++;
	}
  cptr--;
  for (i = 0; i < 10; i++)
    {
      ieee1275_palette[cptr].r = vals2[i];
      ieee1275_palette[cptr].g = 0;
      ieee1275_palette[cptr].b = 0;
      ieee1275_palette[cptr].a = 0xff;
      cptr++;
    }
  for (i = 0; i < 10; i++)
    {
      ieee1275_palette[cptr].r = 0;
      ieee1275_palette[cptr].g = vals2[i];
      ieee1275_palette[cptr].b = 0;
      ieee1275_palette[cptr].a = 0xff;
      cptr++;
    }
  for (i = 0; i < 10; i++)
    {
      ieee1275_palette[cptr].r = 0;
      ieee1275_palette[cptr].g = 0;
      ieee1275_palette[cptr].b = vals2[i];
      ieee1275_palette[cptr].a = 0xff;
      cptr++;
    }
  for (i = 0; i < 10; i++)
    {
      ieee1275_palette[cptr].r = vals2[i];
      ieee1275_palette[cptr].g = vals2[i];
      ieee1275_palette[cptr].b = vals2[i];
      ieee1275_palette[cptr].a = 0xff;
      cptr++;
    }
  ieee1275_palette[cptr].r = 0;
  ieee1275_palette[cptr].g = 0;
  ieee1275_palette[cptr].b = 0;
  ieee1275_palette[cptr].a = 0xff;

  char * t;
  t = holy_canonicalize_file_name (label_font);
  if (!t)
    {
      holy_util_error (_("cannot open `%s': %s"), label_font,
		       strerror (errno));
    }  

  fontfull = xasprintf ("(host)/%s", t);
  free (t);

  holy_font_loader_init ();
  font = holy_font_load (fontfull);
  if (!font)
    {
      holy_util_error (_("cannot open `%s': %s"), label_font,
		       holy_errmsg);
    }  

  width = holy_font_get_string_width (font, text) + 10;
  height = holy_font_get_height (font);

  mode_info.width = width;
  mode_info.height = height;
  mode_info.pitch = width;

  mode_info.mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR;
  mode_info.bpp = 8;
  mode_info.bytes_per_pixel = 1;
  mode_info.number_of_colors = 256;

  holy_video_capture_start (&mode_info, ieee1275_palette,
			    ARRAY_SIZE (ieee1275_palette));

  fg = holy_video_map_rgb (fgcolor.red,
			   fgcolor.green,
			   fgcolor.blue);
  bg = holy_video_map_rgb (bgcolor.red,
			   bgcolor.green,
			   bgcolor.blue);

  holy_memset (holy_video_capture_get_framebuffer (), bg, height * width);
  holy_font_draw_string (text, font, fg,
                         5, holy_font_get_ascent (font));

  head.magic = 1;
  head.width = holy_cpu_to_be16 (width);
  head.height = holy_cpu_to_be16 (height);
  fwrite (&head, 1, sizeof (head), out);
  fwrite (holy_video_capture_get_framebuffer (), 1, width * height, out);

  holy_video_capture_end ();
  if (out != stdout)
    fclose (out);

  free (fontfull);
}
