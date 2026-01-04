/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/video.h>
#include <holy/dl.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

struct hook_ctx
{
  unsigned height, width, depth; 
  struct holy_video_mode_info *current_mode;
};

static int
hook (const struct holy_video_mode_info *info, void *hook_arg)
{
  struct hook_ctx *ctx = hook_arg;

  if (ctx->height && ctx->width && (info->width != ctx->width || info->height != ctx->height))
    return 0;

  if (ctx->depth && info->bpp != ctx->depth)
    return 0;

  if (info->mode_number == holy_VIDEO_MODE_NUMBER_INVALID)
    holy_printf ("        ");
  else
    {
      if (ctx->current_mode && info->mode_number == ctx->current_mode->mode_number)
	holy_printf ("*");
      else
	holy_printf (" ");
      holy_printf (" 0x%03x ", info->mode_number);
    }
  holy_printf ("%4d x %4d x %2d (%4d)  ", info->width, info->height, info->bpp,
	       info->pitch);

  if (info->mode_type & holy_VIDEO_MODE_TYPE_PURE_TEXT)
    holy_xputs (_("Text-only "));
  /* Show mask and position details for direct color modes.  */
  if (info->mode_type & holy_VIDEO_MODE_TYPE_RGB)
    /* TRANSLATORS: "Direct color" is a mode when the color components
       are written dirrectly into memory.  */
    holy_printf_ (N_("Direct color, mask: %d/%d/%d/%d  pos: %d/%d/%d/%d"),
		  info->red_mask_size,
		  info->green_mask_size,
		  info->blue_mask_size,
		  info->reserved_mask_size,
		  info->red_field_pos,
		  info->green_field_pos,
		  info->blue_field_pos,
		  info->reserved_field_pos);
  if (info->mode_type & holy_VIDEO_MODE_TYPE_INDEX_COLOR)
    /* TRANSLATORS: In "paletted color" mode you write the index of the color
       in the palette. Synonyms include "packed pixel".  */
    holy_xputs (_("Paletted "));
  if (info->mode_type & holy_VIDEO_MODE_TYPE_YUV)
    holy_xputs (_("YUV "));
  if (info->mode_type & holy_VIDEO_MODE_TYPE_PLANAR)
    /* TRANSLATORS: "Planar" is the video memory where you have to write
       in several different banks "plans" to control the different color
       components of the same pixel.  */
    holy_xputs (_("Planar "));
  if (info->mode_type & holy_VIDEO_MODE_TYPE_HERCULES)
    holy_xputs (_("Hercules "));
  if (info->mode_type & holy_VIDEO_MODE_TYPE_CGA)
    holy_xputs (_("CGA "));
  if (info->mode_type & holy_VIDEO_MODE_TYPE_NONCHAIN4)
    /* TRANSLATORS: Non-chain 4 is a 256-color planar
       (unchained) video memory mode.  */
    holy_xputs (_("Non-chain 4 "));
  if (info->mode_type & holy_VIDEO_MODE_TYPE_1BIT_BITMAP)
    holy_xputs (_("Monochrome "));
  if (info->mode_type & holy_VIDEO_MODE_TYPE_UNKNOWN)
    holy_xputs (_("Unknown video mode "));

  holy_xputs ("\n");

  return 0;
}

static void
print_edid (struct holy_video_edid_info *edid_info)
{
  unsigned int edid_width, edid_height;

  if (holy_video_edid_checksum (edid_info))
    {
      holy_puts_ (N_("  EDID checksum invalid"));
      holy_errno = holy_ERR_NONE;
      return;
    }

  holy_printf_ (N_("  EDID version: %u.%u\n"),
		edid_info->version, edid_info->revision);
  if (holy_video_edid_preferred_mode (edid_info, &edid_width, &edid_height)
	== holy_ERR_NONE)
    holy_printf_ (N_("    Preferred mode: %ux%u\n"), edid_width, edid_height);
  else
    {
      holy_printf_ (N_("    No preferred mode available\n"));
      holy_errno = holy_ERR_NONE;
    }
}

static holy_err_t
holy_cmd_videoinfo (holy_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  holy_video_adapter_t adapter;
  holy_video_driver_id_t id;
  struct hook_ctx ctx;

  ctx.height = ctx.width = ctx.depth = 0;
  if (argc)
    {
      char *ptr;
      ptr = args[0];
      ctx.width = holy_strtoul (ptr, &ptr, 0);
      if (holy_errno)
	return holy_errno;
      if (*ptr != 'x')
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   N_("invalid video mode specification `%s'"),
			   args[0]);
      ptr++;
      ctx.height = holy_strtoul (ptr, &ptr, 0);
      if (holy_errno)
	return holy_errno;
      if (*ptr == 'x')
	{
	  ptr++;
	  ctx.depth = holy_strtoul (ptr, &ptr, 0);
	  if (holy_errno)
	    return holy_errno;
	}
    }

#ifdef holy_MACHINE_PCBIOS
  if (holy_strcmp (cmd->name, "vbeinfo") == 0)
    holy_dl_load ("vbe");
#endif

  id = holy_video_get_driver_id ();

  holy_puts_ (N_("List of supported video modes:"));
  holy_puts_ (N_("Legend: mask/position=red/green/blue/reserved"));

  FOR_VIDEO_ADAPTERS (adapter)
  {
    struct holy_video_mode_info info;
    struct holy_video_edid_info edid_info;

    holy_printf_ (N_("Adapter `%s':\n"), adapter->name);

    if (!adapter->iterate)
      {
	holy_puts_ (N_("  No info available"));
	continue;
      }

    ctx.current_mode = NULL;

    if (adapter->id == id)
      {
	if (holy_video_get_info (&info) == holy_ERR_NONE)
	  ctx.current_mode = &info;
	else
	  /* Don't worry about errors.  */
	  holy_errno = holy_ERR_NONE;
      }
    else
      {
	if (adapter->init ())
	  {
	    holy_puts_ (N_("  Failed to initialize video adapter"));
	    holy_errno = holy_ERR_NONE;
	    continue;
	  }
      }

    if (adapter->print_adapter_specific_info)
      adapter->print_adapter_specific_info ();

    adapter->iterate (hook, &ctx);

    if (adapter->get_edid && adapter->get_edid (&edid_info) == holy_ERR_NONE)
      print_edid (&edid_info);
    else
      holy_errno = holy_ERR_NONE;

    ctx.current_mode = NULL;

    if (adapter->id != id)
      {
	if (adapter->fini ())
	  {
	    holy_errno = holy_ERR_NONE;
	    continue;
	  }
      }
  }
  return holy_ERR_NONE;
}

static holy_command_t cmd;
#ifdef holy_MACHINE_PCBIOS
static holy_command_t cmd_vbe;
#endif

holy_MOD_INIT(videoinfo)
{
  cmd = holy_register_command ("videoinfo", holy_cmd_videoinfo,
			       /* TRANSLATORS: "x" has to be entered in,
				  like an identifier, so please don't
				  use better Unicode codepoints.  */
			       N_("[WxH[xD]]"),
			       N_("List available video modes. If "
				     "resolution is given show only modes"
				     " matching it."));
#ifdef holy_MACHINE_PCBIOS
  cmd_vbe = holy_register_command ("vbeinfo", holy_cmd_videoinfo,
				   /* TRANSLATORS: "x" has to be entered in,
				      like an identifier, so please don't
				      use better Unicode codepoints.  */
				   N_("[WxH[xD]]"),
				   N_("List available video modes. If "
				      "resolution is given show only modes"
				      " matching it."));
#endif
}

holy_MOD_FINI(videoinfo)
{
  holy_unregister_command (cmd);
#ifdef holy_MACHINE_PCBIOS
  holy_unregister_command (cmd_vbe);
#endif
}

