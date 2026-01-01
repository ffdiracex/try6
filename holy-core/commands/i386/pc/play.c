/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/term.h>
#include <holy/misc.h>
#include <holy/cpu/io.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/time.h>
#include <holy/speaker.h>

holy_MOD_LICENSE ("GPLv2+");

#define BASE_TEMPO (60 * 1000)


#define T_REST			((holy_uint16_t) 0)
#define T_FINE			((holy_uint16_t) -1)

struct note
{
  holy_uint16_t pitch;
  holy_uint16_t duration;
};

/* Returns whether playing should continue.  */
static int
play (unsigned tempo, struct note *note)
{
  holy_uint64_t to;

  if (note->pitch == T_FINE || holy_getkey_noblock () != holy_TERM_NO_KEY)
    return 1;

  holy_dprintf ("play", "pitch = %d, duration = %d\n", note->pitch,
                note->duration);

  switch (note->pitch)
    {
      case T_REST:
        holy_speaker_beep_off ();
        break;

      default:
        holy_speaker_beep_on (note->pitch);
        break;
    }

  to = holy_get_time_ms () + BASE_TEMPO * note->duration / tempo;
  while ((holy_get_time_ms () <= to)
	 && (holy_getkey_noblock () == holy_TERM_NO_KEY));

  return 0;
}

static holy_err_t
holy_cmd_play (holy_command_t cmd __attribute__ ((unused)),
	       int argc, char **args)
{

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       /* TRANSLATORS: It's musical notes, not the notes
			  you take. Play command expects arguments which can
			  be either a filename or tempo+notes.
			  This error happens if none is specified.  */
		       N_("filename or tempo and notes expected"));

  if (argc == 1)
    {
      struct note buf;
      holy_uint32_t tempo;
      holy_file_t file;

      file = holy_file_open (args[0]);

      if (! file)
        return holy_errno;

      if (holy_file_read (file, &tempo, sizeof (tempo)) != sizeof (tempo))
        {
          holy_file_close (file);
	  if (!holy_errno)
	    holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			args[0]);
          return holy_errno;
        }

      if (!tempo)
        {
          holy_file_close (file);
	  holy_error (holy_ERR_BAD_ARGUMENT, N_("Invalid tempo in %s"),
		      args[0]);
          return holy_errno;
        }

      tempo = holy_le_to_cpu32 (tempo);
      holy_dprintf ("play","tempo = %d\n", tempo);

      while (holy_file_read (file, &buf,
                             sizeof (struct note)) == sizeof (struct note))
        {
          buf.pitch = holy_le_to_cpu16 (buf.pitch);
          buf.duration = holy_le_to_cpu16 (buf.duration);

          if (play (tempo, &buf))
            break;
        }

      holy_file_close (file);
    }
  else
    {
      char *end;
      unsigned tempo;
      struct note note;
      int i;

      tempo = holy_strtoul (args[0], &end, 0);

      if (!tempo)
        {
	  holy_error (holy_ERR_BAD_ARGUMENT, N_("Invalid tempo in %s"),
		      args[0]);
          return holy_errno;
        }

      if (*end)
        /* Was not a number either, assume it was supposed to be a file name.  */
        return holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), args[0]);

      holy_dprintf ("play","tempo = %d\n", tempo);

      for (i = 1; i + 1 < argc; i += 2)
        {
          note.pitch = holy_strtoul (args[i], &end, 0);
	  if (holy_errno)
	    break;
          if (*end)
            {
              holy_error (holy_ERR_BAD_NUMBER, N_("unrecognized number"));
              break;
            }

          note.duration = holy_strtoul (args[i + 1], &end, 0);
	  if (holy_errno)
	    break;
          if (*end)
            {
              holy_error (holy_ERR_BAD_NUMBER, N_("unrecognized number"));
              break;
            }

          if (play (tempo, &note))
            break;
        }
    }

  holy_speaker_beep_off ();

  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(play)
{
  cmd = holy_register_command ("play", holy_cmd_play,
			       N_("FILE | TEMPO [PITCH1 DURATION1] [PITCH2 DURATION2] ... "),
			       N_("Play a tune."));
}

holy_MOD_FINI(play)
{
  holy_unregister_command (cmd);
}
