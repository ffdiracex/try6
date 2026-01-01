/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/test.h>
#include <holy/dl.h>
#include <holy/video.h>
#include <holy/video_fb.h>
#include <holy/command.h>
#include <holy/font.h>
#include <holy/procfs.h>
#include <holy/env.h>
#include <holy/normal.h>
#include <holy/time.h>

holy_MOD_LICENSE ("GPLv2+");


static const char testfile[] =
  "menuentry \"test\" {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \"s̛ ơ t o̒ s̒ u o̕̚ 8.04 m̂ñåh̊z̆x̣ a̡ b̢g̢ u᷎ô᷎ ô᷎ O̷ a̖̣ ȃ̐\" --class ubuntu --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \" הַרמלל(טוֹבָ) לֶךְ\" --class opensuse --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \"الرملل جِداً لِكَ\" --class gentoo --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \"ὑπόγυͅον\" --class kubuntu --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  "menuentry \"سَّ نِّ نَّ نٌّ نّْ\" --class linuxmint --class linux --class os {\n"
  "\ttrue\n"
  "}\n"
  /* Chinese & UTF-8 test from Carbon Jiao. */
  "menuentry \"从硬盘的第一主分区启动\" --class \"windows xp\" --class windows --class os {\n"
  "\ttrue\n"
  "}\n"
  "timeout=3\n";

static char *
get_test_cfg (holy_size_t *sz)
{
  *sz = holy_strlen (testfile);
  return holy_strdup (testfile);
}

struct holy_procfs_entry test_cfg =
{
  .name = "test.cfg",
  .get_contents = get_test_cfg
};

struct
{
  const char *name;
  const char *var;
  const char *val;  
} tests[] =
  {
    { "gfxterm_menu", NULL, NULL },
    { "gfxmenu", "theme", "starfield/theme.txt" },
    { "gfxterm_ar", "lang", "en@arabic" },
    { "gfxterm_cyr", "lang", "en@cyrillic" },
    { "gfxterm_heb", "lang", "en@hebrew" },
    { "gfxterm_gre", "lang", "en@greek" },
    { "gfxterm_ru", "lang", "ru" },
    { "gfxterm_fr", "lang", "fr" },
    { "gfxterm_quot", "lang", "en@quot" },
    { "gfxterm_piglatin", "lang", "en@piglatin" },
    { "gfxterm_ch", "lang", "de_CH" },
    { "gfxterm_red", "menu_color_normal", "red/blue" },
    { "gfxterm_high", "menu_color_highlight", "blue/red" },
  };

#define FONT_NAME "Unknown Regular 16"

/* Functional test main method.  */
static void
gfxterm_menu (void)
{
  unsigned i, j;
  holy_font_t font;

  holy_dl_load ("png");
  holy_dl_load ("gettext");
  holy_dl_load ("gfxterm");

  holy_errno = holy_ERR_NONE;

  holy_dl_load ("gfxmenu");

  font = holy_font_get (FONT_NAME);
  if (font && holy_strcmp (font->name, FONT_NAME) != 0)
    font = 0;
  if (!font)
    font = holy_font_load ("unicode");

  if (!font)
    {
      holy_test_assert (0, "unicode font not found: %s", holy_errmsg);
      return;
    }

  holy_procfs_register ("test.cfg", &test_cfg);
  
  for (j = 0; j < ARRAY_SIZE (tests); j++)
    for (i = 0; i < holy_TEST_VIDEO_SMALL_N_MODES; i++)
      {
	holy_uint64_t start;

#if defined (holy_MACHINE_MIPS_QEMU_MIPS) || defined (holy_MACHINE_IEEE1275)
	if (holy_test_video_modes[i].width > 1024)
	  continue;
	if (holy_strcmp (tests[j].name, "gfxmenu") == 0
	    && holy_test_video_modes[i].width > 800)
	  continue;
#endif
	start = holy_get_time_ms ();

	holy_video_capture_start (&holy_test_video_modes[i],
				  holy_video_fbstd_colors,
				  holy_test_video_modes[i].number_of_colors);
	if (holy_errno)
	  {
	    holy_test_assert (0, "can't start capture: %d: %s",
			      holy_errno, holy_errmsg);
	    return;
	  }
	holy_terminal_input_fake_sequence ((int []) { -1, -1, -1, holy_TERM_KEY_DOWN, -1, 'e',
	      -1, holy_TERM_KEY_RIGHT, -1, 'x', -1,  '\e', -1, '\e' }, 14);

	holy_video_checksum (tests[j].name);

	if (holy_test_use_gfxterm ())
	  return;

	holy_env_context_open ();
	if (tests[j].var)
	  holy_env_set (tests[j].var, tests[j].val);
	holy_normal_execute ("(proc)/test.cfg", 1, 0);
	holy_env_context_close ();

	holy_test_use_gfxterm_end ();

	holy_terminal_input_fake_sequence_end ();
	holy_video_checksum_end ();
	holy_video_capture_end ();

	if (tests[j].var)
	  holy_env_unset (tests[j].var);
	holy_printf ("%s %dx%dx%s done %lld ms\n", tests[j].name,
		     holy_test_video_modes[i].width,
		     holy_test_video_modes[i].height,
		     holy_video_checksum_get_modename (), (long long) (holy_get_time_ms () - start));
      }

  holy_procfs_unregister (&test_cfg);
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (gfxterm_menu, gfxterm_menu);
