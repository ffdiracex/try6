/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/term.h>
#include <holy/loader.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* cat FILE */
static holy_err_t
holy_mini_cmd_cat (struct holy_command *cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  holy_file_t file;
  char buf[holy_DISK_SECTOR_SIZE];
  holy_ssize_t size;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = holy_file_open (argv[0]);
  if (! file)
    return holy_errno;

  while ((size = holy_file_read (file, buf, sizeof (buf))) > 0)
    {
      int i;

      for (i = 0; i < size; i++)
	{
	  unsigned char c = buf[i];

	  if ((holy_isprint (c) || holy_isspace (c)) && c != '\r')
	    holy_printf ("%c", c);
	  else
	    {
	      holy_setcolorstate (holy_TERM_COLOR_HIGHLIGHT);
	      holy_printf ("<%x>", (int) c);
	      holy_setcolorstate (holy_TERM_COLOR_STANDARD);
	    }
	}
    }

  holy_xputs ("\n");
  holy_refresh ();
  holy_file_close (file);

  return 0;
}

/* help */
static holy_err_t
holy_mini_cmd_help (struct holy_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char *argv[] __attribute__ ((unused)))
{
  holy_command_t p;

  for (p = holy_command_list; p; p = p->next)
    holy_printf ("%s (%d%c)\t%s\n", p->name,
		 p->prio & holy_COMMAND_PRIO_MASK,
		 (p->prio & holy_COMMAND_FLAG_ACTIVE) ? '+' : '-',
		 p->description);

  return 0;
}

/* dump ADDRESS [SIZE] */
static holy_err_t
holy_mini_cmd_dump (struct holy_command *cmd __attribute__ ((unused)),
		    int argc, char *argv[])
{
  holy_uint8_t *addr;
  holy_size_t size = 4;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no address specified");

#if holy_CPU_SIZEOF_VOID_P == holy_CPU_SIZEOF_LONG
#define holy_strtoaddr holy_strtoul
#else
#define holy_strtoaddr holy_strtoull
#endif

  addr = (holy_uint8_t *) holy_strtoaddr (argv[0], 0, 0);
  if (holy_errno)
    return holy_errno;

  if (argc > 1)
    size = (holy_size_t) holy_strtoaddr (argv[1], 0, 0);

  while (size--)
    {
      holy_printf ("%x%x ", *addr >> 4, *addr & 0xf);
      addr++;
    }

  return 0;
}

/* rmmod MODULE */
static holy_err_t
holy_mini_cmd_rmmod (struct holy_command *cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  holy_dl_t mod;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no module specified");

  mod = holy_dl_get (argv[0]);
  if (! mod)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no such module");

  if (holy_dl_unref (mod) <= 0)
    holy_dl_unload (mod);

  return 0;
}

/* lsmod */
static holy_err_t
holy_mini_cmd_lsmod (struct holy_command *cmd __attribute__ ((unused)),
		     int argc __attribute__ ((unused)),
		     char *argv[] __attribute__ ((unused)))
{
  holy_dl_t mod;

  /* TRANSLATORS: this is module list header.  Name
     is module name, Ref Count is a reference counter
     (how many modules or open descriptors use it).
     Dependencies are the other modules it uses.
   */
  holy_printf_ (N_("Name\tRef Count\tDependencies\n"));
  FOR_DL_MODULES (mod)
  {
    holy_dl_dep_t dep;

    holy_printf ("%s\t%d\t\t", mod->name, mod->ref_count);
    for (dep = mod->dep; dep; dep = dep->next)
      {
	if (dep != mod->dep)
	  holy_xputs (",");

	holy_printf ("%s", dep->mod->name);
      }
    holy_xputs ("\n");
  }

  return 0;
}

/* exit */
static holy_err_t __attribute__ ((noreturn))
holy_mini_cmd_exit (struct holy_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char *argv[] __attribute__ ((unused)))
{
  holy_exit ();
  /* Not reached.  */
}

static holy_command_t cmd_cat, cmd_help;
static holy_command_t cmd_dump, cmd_rmmod, cmd_lsmod, cmd_exit;

holy_MOD_INIT(minicmd)
{
  cmd_cat =
    holy_register_command ("cat", holy_mini_cmd_cat,
			   N_("FILE"), N_("Show the contents of a file."));
  cmd_help =
    holy_register_command ("help", holy_mini_cmd_help,
			   0, N_("Show this message."));
  cmd_dump =
    holy_register_command ("dump", holy_mini_cmd_dump,
			   N_("ADDR [SIZE]"), N_("Show memory contents."));
  cmd_rmmod =
    holy_register_command ("rmmod", holy_mini_cmd_rmmod,
			   N_("MODULE"), N_("Remove a module."));
  cmd_lsmod =
    holy_register_command ("lsmod", holy_mini_cmd_lsmod,
			   0, N_("Show loaded modules."));
  cmd_exit =
    holy_register_command ("exit", holy_mini_cmd_exit,
			   0, N_("Exit from holy."));
}

holy_MOD_FINI(minicmd)
{
  holy_unregister_command (cmd_cat);
  holy_unregister_command (cmd_help);
  holy_unregister_command (cmd_dump);
  holy_unregister_command (cmd_rmmod);
  holy_unregister_command (cmd_lsmod);
  holy_unregister_command (cmd_exit);
}
