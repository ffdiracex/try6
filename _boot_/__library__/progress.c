/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/time.h>
#include <holy/term.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/normal.h>
#include <holy/net.h>

holy_MOD_LICENSE ("GPLv2+");

#define UPDATE_INTERVAL 800

static void
holy_file_progress_hook_real (holy_disk_addr_t sector __attribute__ ((unused)),
                              unsigned offset __attribute__ ((unused)),
                              unsigned length, void *data)
{
  static int call_depth = 0;
  holy_uint64_t now;
  static holy_uint64_t last_progress_update_time;
  holy_file_t file = data;
  const char *e;
  file->progress_offset += length;

  if (call_depth)
    return;

  e = holy_env_get ("enable_progress_indicator");
  if (e && e[0] == '0') {
    return;
  }

  call_depth = 1;
  now = holy_get_time_ms ();

  if (((now - last_progress_update_time > UPDATE_INTERVAL) &&
       (file->progress_offset - file->offset > 0)) ||
      (file->progress_offset == file->size))
    {
      static char buffer[80];
      struct holy_term_output *term;
      const char *partial_file_name;

      unsigned long long percent;
      holy_uint64_t current_speed;

      if (now - file->last_progress_time < 10)
	current_speed = 0;
      else
	current_speed = holy_divmod64 ((file->progress_offset
					- file->last_progress_offset)
				       * 100ULL * 1000ULL,
				       now - file->last_progress_time, 0);

      if (file->size == 0)
	percent = 100;
      else
	percent = holy_divmod64 (100 * file->progress_offset,
				 file->size, 0);

      /* holy_net_fs_open() saves off partial file structure before name is initialized.
         It already saves passed file name in net structure so just use it in this case.
       */
      if (file->device->net)
	partial_file_name = holy_strrchr (file->device->net->name, '/');
      else if (file->name) /* holy_file_open() may leave it as NULL */
	partial_file_name = holy_strrchr (file->name, '/');
      else
	partial_file_name = NULL;
      if (partial_file_name)
	partial_file_name++;
      else
	partial_file_name = "";

      file->estimated_speed = (file->estimated_speed + current_speed) >> 1;

      holy_snprintf (buffer, sizeof (buffer), "      [ %.20s  %s  %llu%%  ",
                     partial_file_name,
                     holy_get_human_size (file->progress_offset,
                                          holy_HUMAN_SIZE_NORMAL),
                     (unsigned long long) percent);

      char *ptr = buffer + holy_strlen (buffer);
      holy_snprintf (ptr, sizeof (buffer) - (ptr - buffer), "%s ]",
                     holy_get_human_size (file->estimated_speed,
                                          holy_HUMAN_SIZE_SPEED));

      holy_size_t len = holy_strlen (buffer);
      FOR_ACTIVE_TERM_OUTPUTS (term)
        {
          if (term->progress_update_counter++ > term->progress_update_divisor
	      || (file->progress_offset == file->size
		  && term->progress_update_divisor
		  != (unsigned) holy_PROGRESS_NO_UPDATE))
            {
              struct holy_term_coordinate old_pos = holy_term_getxy (term);
              struct holy_term_coordinate new_pos = old_pos;
              new_pos.x = holy_term_width (term) - len - 1;

              holy_term_gotoxy (term, new_pos);
	      holy_puts_terminal (buffer, term);
              holy_term_gotoxy (term, old_pos);

              term->progress_update_counter = 0;

	      if (term->refresh)
		term->refresh (term);
            }
        }

      file->last_progress_offset = file->progress_offset;
      file->last_progress_time = now;
      last_progress_update_time = now;
    }
  call_depth = 0;
}

holy_MOD_INIT(progress)
{
  holy_file_progress_hook = holy_file_progress_hook_real;
}

holy_MOD_FINI(progress)
{
  holy_file_progress_hook = 0;
}
