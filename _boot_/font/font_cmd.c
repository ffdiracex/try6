/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/font.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>

static holy_err_t
loadfont_command (holy_command_t cmd __attribute__ ((unused)),
		  int argc,
		  char **args)
{
  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  while (argc--)
    if (holy_font_load (*args++) == 0)
      {
	if (!holy_errno)
	  return holy_error (holy_ERR_BAD_FONT, "invalid font");
	return holy_errno;
      }

  return holy_ERR_NONE;
}

static holy_err_t
lsfonts_command (holy_command_t cmd __attribute__ ((unused)),
                 int argc __attribute__ ((unused)),
                 char **args __attribute__ ((unused)))
{
  struct holy_font_node *node;

  holy_puts_ (N_("Loaded fonts:"));
  for (node = holy_font_list; node; node = node->next)
    {
      holy_font_t font = node->value;
      holy_printf ("%s\n", holy_font_get_name (font));
    }

  return holy_ERR_NONE;
}

static holy_command_t cmd_loadfont, cmd_lsfonts;

#if defined (holy_MACHINE_MIPS_LOONGSON) || defined (holy_MACHINE_COREBOOT)
void holy_font_init (void)
#else
holy_MOD_INIT(font)
#endif
{
  holy_font_loader_init ();

  cmd_loadfont =
    holy_register_command ("loadfont", loadfont_command,
			   N_("FILE..."),
			   N_("Specify one or more font files to load."));
  cmd_lsfonts =
    holy_register_command ("lsfonts", lsfonts_command,
			   0, N_("List the loaded fonts."));
}

#if defined (holy_MACHINE_MIPS_LOONGSON) || defined (holy_MACHINE_COREBOOT)
void holy_font_fini (void)
#else
holy_MOD_FINI(font)
#endif
{
  /* TODO: Determine way to free allocated resources.
     Warning: possible pointer references could be in use.  */

  holy_unregister_command (cmd_loadfont);
  holy_unregister_command (cmd_lsfonts);
}
