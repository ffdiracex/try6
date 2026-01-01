/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/video.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/font.h>
#include <holy/term.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/gfxmenu_view.h>
#include <holy/env.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_videotest (holy_command_t cmd __attribute__ ((unused)),
                    int argc, char **args)
{
  holy_err_t err;
  holy_video_color_t color;
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;
  int i;
  struct holy_video_render_target *text_layer;
  holy_video_color_t palette[16];
  const char *mode = NULL;

#ifdef holy_MACHINE_PCBIOS
  if (holy_strcmp (cmd->name, "vbetest") == 0)
    holy_dl_load ("vbe");
#endif
  mode = holy_env_get ("gfxmode");
  if (argc)
    mode = args[0];
  if (!mode)
    mode = "auto";

  err = holy_video_set_mode (mode, holy_VIDEO_MODE_TYPE_PURE_TEXT, 0);
  if (err)
    return err;

  holy_video_get_viewport (&x, &y, &width, &height);

  {
    const char *str;
    int texty;
    holy_font_t sansbig;
    holy_font_t sans;
    holy_font_t sanssmall;
    holy_font_t fixed;
    struct holy_font_glyph *glyph;

    if (holy_video_create_render_target (&text_layer, width, height,
					 holy_VIDEO_MODE_TYPE_RGB
					 | holy_VIDEO_MODE_TYPE_ALPHA)
	|| !text_layer)
      goto fail;

    holy_video_set_active_render_target (text_layer);

    color = holy_video_map_rgb (0, 255, 255);
    sansbig = holy_font_get ("Unknown Regular 16");
    sans = holy_font_get ("Unknown Regular 16");
    sanssmall = holy_font_get ("Unknown Regular 16");
    fixed = holy_font_get ("Fixed 20");
    if (! sansbig || ! sans || ! sanssmall || ! fixed)
      return holy_error (holy_ERR_BAD_FONT, "no font loaded");

    glyph = holy_font_get_glyph (fixed, '*');
    holy_font_draw_glyph (glyph, color, 200 ,0);

    color = holy_video_map_rgb (255, 255, 255);

    texty = 32;
    holy_font_draw_string ("The quick brown fox jumped over the lazy dog.",
			   sans, color, 16, texty);
    texty += holy_font_get_descent (sans) + holy_font_get_leading (sans);

    texty += holy_font_get_ascent (fixed);
    holy_font_draw_string ("The quick brown fox jumped over the lazy dog.",
			   fixed, color, 16, texty);
    texty += holy_font_get_descent (fixed) + holy_font_get_leading (fixed);

    /* To convert Unicode characters into UTF-8 for this test, the following
       command is useful:
       echo -ne '\x00\x00\x26\x3A' | iconv -f UTF-32BE -t UTF-8 | od -t x1
       This converts the Unicode character U+263A to UTF-8.  */

    /* Characters used:
       Code point  Description                    UTF-8 encoding
       ----------- ------------------------------ --------------
       U+263A      unfilled smiley face           E2 98 BA
       U+00A1      inverted exclamation point     C2 A1
       U+00A3      British pound currency symbol  C2 A3
       U+03C4      Greek tau                      CF 84
       U+00E4      lowercase letter a with umlaut C3 A4
       U+2124      set 'Z' symbol (integers)      E2 84 A4
       U+2286      subset symbol                  E2 8A 86
       U+211D      set 'R' symbol (real numbers)  E2 84 9D  */

    str =
      "Unicode test: happy\xE2\x98\xBA \xC2\xA3 5.00"
      " \xC2\xA1\xCF\x84\xC3\xA4u! "
      " \xE2\x84\xA4\xE2\x8A\x86\xE2\x84\x9D";
    color = holy_video_map_rgb (128, 128, 255);

    /* All characters in the string exist in the 'Fixed 20' (10x20) font.  */
    texty += holy_font_get_ascent(fixed);
    holy_font_draw_string (str, fixed, color, 16, texty);
    texty += holy_font_get_descent (fixed) + holy_font_get_leading (fixed);

    texty += holy_font_get_ascent(sansbig);
    holy_font_draw_string (str, sansbig, color, 16, texty);
    texty += holy_font_get_descent (sansbig) + holy_font_get_leading (sansbig);

    texty += holy_font_get_ascent(sans);
    holy_font_draw_string (str, sans, color, 16, texty);
    texty += holy_font_get_descent (sans) + holy_font_get_leading (sans);

    texty += holy_font_get_ascent(sanssmall);
    holy_font_draw_string (str, sanssmall, color, 16, texty);
    texty += (holy_font_get_descent (sanssmall)
	      + holy_font_get_leading (sanssmall));

    glyph = holy_font_get_glyph (fixed, '*');

    for (i = 0; i < 16; i++)
      {
	color = holy_video_map_color (i);
	palette[i] = color;
	holy_font_draw_glyph (glyph, color, 16 + i * 16, 220);
      }
  }

  holy_video_set_active_render_target (holy_VIDEO_RENDER_TARGET_DISPLAY);

  for (i = 0; i < 5; i++)
    {

      if (i == 0 || i == 1)
	{	  
	  color = holy_video_map_rgb (0, 0, 0);
	  holy_video_fill_rect (color, 0, 0, width, height);

	  color = holy_video_map_rgb (255, 0, 0);
	  holy_video_fill_rect (color, 0, 0, 100, 100);

	  color = holy_video_map_rgb (0, 255, 0);
	  holy_video_fill_rect (color, 100, 0, 100, 100);

	  color = holy_video_map_rgb (0, 0, 255);
	  holy_video_fill_rect (color, 200, 0, 100, 100);

	  color = holy_video_map_rgb (0, 255, 255);
	  holy_video_fill_rect (color, 0, 100, 100, 100);

	  color = holy_video_map_rgb (255, 0, 255);
	  holy_video_fill_rect (color, 100, 100, 100, 100);

	  color = holy_video_map_rgb (255, 255, 0);
	  holy_video_fill_rect (color, 200, 100, 100, 100);

	  holy_video_set_viewport (x + 150, y + 150,
				   width - 150 * 2, height - 150 * 2);
	  color = holy_video_map_rgb (77, 33, 77);
	  holy_video_fill_rect (color, 0, 0, width, height);
	}

      color = holy_video_map_rgb (i, 33, 77);
      holy_video_fill_rect (color, 0, 0, width, height);
      holy_video_blit_render_target (text_layer, holy_VIDEO_BLIT_BLEND, 0, 0,
                                     0, 0, width, height);
      holy_video_swap_buffers ();
    }

  holy_getkey ();

  holy_video_delete_render_target (text_layer);

  holy_video_restore ();

  for (i = 0; i < 16; i++)
    holy_printf("color %d: %08x\n", i, palette[i]);

  holy_errno = holy_ERR_NONE;
  return holy_errno;

 fail:
  holy_video_delete_render_target (text_layer);
  holy_video_restore ();
  return holy_errno;
}

static holy_command_t cmd;
#ifdef holy_MACHINE_PCBIOS
static holy_command_t cmd_vbe;
#endif

holy_MOD_INIT(videotest)
{
  cmd = holy_register_command ("videotest", holy_cmd_videotest,
			       /* TRANSLATORS: "x" has to be entered in,
				  like an identifier, so please don't
				  use better Unicode codepoints.  */
			       N_("[WxH]"),
			       /* TRANSLATORS: Here, on the other hand, it's
				  nicer to use unicode cross instead of x.  */
			       N_("Test video subsystem in mode WxH."));
#ifdef holy_MACHINE_PCBIOS
  cmd_vbe = holy_register_command ("vbetest", holy_cmd_videotest,
			       0, N_("Test video subsystem."));
#endif
}

holy_MOD_FINI(videotest)
{
  holy_unregister_command (cmd);
#ifdef holy_MACHINE_PCBIOS
  holy_unregister_command (cmd_vbe);
#endif
}
