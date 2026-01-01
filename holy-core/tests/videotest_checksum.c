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

holy_MOD_LICENSE ("GPLv2+");

#define FONT_NAME "Unknown Regular 16"

/* Functional test main method.  */
static void
videotest_checksum (void)
{
  unsigned i;
  holy_font_t font;

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

  for (i = 0; i < ARRAY_SIZE (holy_test_video_modes); i++)
    {
      holy_err_t err;
#if defined (holy_MACHINE_MIPS_QEMU_MIPS) || defined (holy_MACHINE_IEEE1275)
      if (holy_test_video_modes[i].width > 1024)
	continue;
#endif
      err = holy_video_capture_start (&holy_test_video_modes[i],
				      holy_video_fbstd_colors,
				      holy_test_video_modes[i].number_of_colors);
      if (err)
	{
	  holy_test_assert (0, "can't start capture: %s", holy_errmsg);
	  holy_print_error ();
	  continue;
	}
      holy_terminal_input_fake_sequence ((int []) { '\n' }, 1);

      holy_video_checksum ("videotest");

      char *args[] = { 0 };
      holy_command_execute ("videotest", 0, args);

      holy_terminal_input_fake_sequence_end ();
      holy_video_checksum_end ();
      holy_video_capture_end ();
    }
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (videotest_checksum, videotest_checksum);
