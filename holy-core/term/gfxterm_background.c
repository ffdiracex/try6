/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/font.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/video.h>
#include <holy/gfxterm.h>
#include <holy/bitmap.h>
#include <holy/command.h>
#include <holy/extcmd.h>
#include <holy/bitmap_scale.h>
#include <holy/i18n.h>
#include <holy/color.h>

holy_MOD_LICENSE ("GPLv2+");

/* Option array indices.  */
enum
  {
    BACKGROUND_CMD_ARGINDEX_MODE = 0
  };

static const struct holy_arg_option background_image_cmd_options[] =
  {
    {"mode", 'm', 0, N_("Background image mode."),
    /* TRANSLATORS: This refers to background image mode (stretched or 
       in left-top corner). Note that holy will accept only original
       keywords stretch and normal, not the translated ones.
       So please put both in translation
       e.g. stretch(=%STRETCH%)|normal(=%NORMAL%).
       The percents mark the translated version. Since many people
       may not know the word stretch or normal I recommend
       putting the translation either here or in "Background image mode."
       string.  */
     N_("stretch|normal"),
     ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static holy_err_t
holy_gfxterm_background_image_cmd (holy_extcmd_context_t ctxt,
                                   int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;

  /* Check that we have video adapter active.  */
  if (holy_video_get_info(NULL) != holy_ERR_NONE)
    return holy_errno;

  /* Destroy existing background bitmap if loaded.  */
  if (holy_gfxterm_background.bitmap)
    {
      holy_video_bitmap_destroy (holy_gfxterm_background.bitmap);
      holy_gfxterm_background.bitmap = 0;
      holy_gfxterm_background.blend_text_bg = 0;

      /* Mark whole screen as dirty.  */
      holy_gfxterm_schedule_repaint ();
    }

  /* If filename was provided, try to load that.  */
  if (argc >= 1)
    {
      /* Try to load new one.  */
      holy_video_bitmap_load (&holy_gfxterm_background.bitmap, args[0]);
      if (holy_errno != holy_ERR_NONE)
        return holy_errno;

      /* Determine if the bitmap should be scaled to fit the screen.  */
      if (!state[BACKGROUND_CMD_ARGINDEX_MODE].set
          || holy_strcmp (state[BACKGROUND_CMD_ARGINDEX_MODE].arg,
                          "stretch") == 0)
          {
	    unsigned int width, height;
	    holy_gfxterm_get_dimensions (&width, &height);
            if (width
		!= holy_video_bitmap_get_width (holy_gfxterm_background.bitmap)
                || height
		!= holy_video_bitmap_get_height (holy_gfxterm_background.bitmap))
              {
                struct holy_video_bitmap *scaled_bitmap;
                holy_video_bitmap_create_scaled (&scaled_bitmap,
                                                 width, 
                                                 height,
                                                 holy_gfxterm_background.bitmap,
                                                 holy_VIDEO_BITMAP_SCALE_METHOD_BEST);
                if (holy_errno == holy_ERR_NONE)
                  {
                    /* Replace the original bitmap with the scaled one.  */
                    holy_video_bitmap_destroy (holy_gfxterm_background.bitmap);
                    holy_gfxterm_background.bitmap = scaled_bitmap;
                  }
              }
          }

      /* If bitmap was loaded correctly, display it.  */
      if (holy_gfxterm_background.bitmap)
        {
	  holy_gfxterm_background.blend_text_bg = 1;

          /* Mark whole screen as dirty.  */
	  holy_gfxterm_schedule_repaint ();
        }
    }

  /* All was ok.  */
  holy_errno = holy_ERR_NONE;
  return holy_errno;
}

static holy_err_t
holy_gfxterm_background_color_cmd (holy_command_t cmd __attribute__ ((unused)),
                                   int argc, char **args)
{
  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  /* Check that we have video adapter active.  */
  if (holy_video_get_info (NULL) != holy_ERR_NONE)
    return holy_errno;

  if (holy_video_parse_color (args[0],
			      &holy_gfxterm_background.default_bg_color)
      != holy_ERR_NONE)
    return holy_errno;

  /* Destroy existing background bitmap if loaded.  */
  if (holy_gfxterm_background.bitmap)
    {
      holy_video_bitmap_destroy (holy_gfxterm_background.bitmap);
      holy_gfxterm_background.bitmap = 0;

      /* Mark whole screen as dirty.  */
      holy_gfxterm_schedule_repaint ();
    }

  /* Set the background and border colors.  The background color needs to be
     compatible with the text layer.  */
  holy_gfxterm_video_update_color ();
  holy_gfxterm_background.blend_text_bg = 1;

  /* Mark whole screen as dirty.  */
  holy_gfxterm_schedule_repaint ();

  return holy_ERR_NONE;
}

static holy_extcmd_t background_image_cmd_handle;
static holy_command_t background_color_cmd_handle;

holy_MOD_INIT(gfxterm_background)
{
  background_image_cmd_handle =
    holy_register_extcmd ("background_image",
                          holy_gfxterm_background_image_cmd, 0,
                          N_("[-m (stretch|normal)] FILE"),
                          N_("Load background image for active terminal."),
                          background_image_cmd_options);
  background_color_cmd_handle =
    holy_register_command ("background_color",
                           holy_gfxterm_background_color_cmd,
                           N_("COLOR"),
                           N_("Set background color for active terminal."));
}

holy_MOD_FINI(gfxterm_background)
{
  holy_unregister_command (background_color_cmd_handle);
  holy_unregister_extcmd (background_image_cmd_handle);
}
