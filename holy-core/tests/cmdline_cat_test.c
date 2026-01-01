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
  /* Chinese & UTF-8 test from Carbon Jiao. */
  "从硬盘的第一主分区启动\n"
  "The quick brown fox jumped over the lazy dog.\n"
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
  "Unicode test: happy\xE2\x98\xBA \xC2\xA3 5.00"
  " \xC2\xA1\xCF\x84\xC3\xA4u! "
  " \xE2\x84\xA4\xE2\x8A\x86\xE2\x84\x9D\n"
  /* Test handling of bad (non-UTF8) sequences*/
  "\x99Hello\xc2Hello\xc1\x81Hello\n";
;

static char *
get_test_txt (holy_size_t *sz)
{
  *sz = holy_strlen (testfile);
  return holy_strdup (testfile);
}

struct holy_procfs_entry test_txt =
{
  .name = "test.txt",
  .get_contents = get_test_txt
};

#define FONT_NAME "Unknown Regular 16"

/* Functional test main method.  */
static void
cmdline_cat_test (void)
{
  unsigned i;
  holy_font_t font;

  holy_dl_load ("gfxterm");
  holy_errno = holy_ERR_NONE;

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

  holy_procfs_register ("test.txt", &test_txt);
  
  for (i = 0; i < holy_TEST_VIDEO_SMALL_N_MODES; i++)
    {
      holy_video_capture_start (&holy_test_video_modes[i],
				holy_video_fbstd_colors,
				holy_test_video_modes[i].number_of_colors);
      holy_terminal_input_fake_sequence ((int [])
					 {  'c', 'a', 't', ' ', 
					     '(', 'p', 'r', 'o', 'c', ')',
					     '/', 't', 'e', 's', 't', '.',
					     't', 'x', 't', '\n',
					     holy_TERM_NO_KEY,
					     holy_TERM_NO_KEY, '\e'},
					 23);

      holy_video_checksum ("cmdline_cat");

      if (!holy_test_use_gfxterm ())
	holy_cmdline_run (1, 0);

      holy_test_use_gfxterm_end ();

      holy_terminal_input_fake_sequence_end ();
      holy_video_checksum_end ();
      holy_video_capture_end ();
    }

  holy_procfs_unregister (&test_txt);
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (cmdline_cat_test, cmdline_cat_test);
