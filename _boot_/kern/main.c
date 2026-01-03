/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/misc.h>
#include <holy/symbol.h>
#include <holy/dl.h>
#include <holy/term.h>
#include <holy/file.h>
#include <holy/device.h>
#include <holy/env.h>
#include <holy/mm.h>
#include <holy/command.h>
#include <holy/reader.h>
#include <holy/parser.h>

#ifdef holy_MACHINE_PCBIOS
#include <holy/machine/memory.h>
#endif

holy_addr_t
holy_modules_get_end (void)
{
  struct holy_module_info *modinfo;

  modinfo = (struct holy_module_info *) holy_modbase;

  /* Check if there are any modules.  */
  if ((modinfo == 0) || modinfo->magic != holy_MODULE_MAGIC)
    return holy_modbase;

  return holy_modbase + modinfo->size;
}

/* Load all modules in core.  */
static void
holy_load_modules (void)
{
  struct holy_module_header *header;
  FOR_MODULES (header)
  {
    /* Not an ELF module, skip.  */
    if (header->type != OBJ_TYPE_ELF)
      continue;

    if (! holy_dl_load_core ((char *) header + sizeof (struct holy_module_header),
			     (header->size - sizeof (struct holy_module_header))))
      holy_fatal ("%s", holy_errmsg);

    if (holy_errno)
      holy_print_error ();
  }
}

static char *load_config;

static void
holy_load_config (void)
{
  struct holy_module_header *header;
  FOR_MODULES (header)
  {
    /* Not an embedded config, skip.  */
    if (header->type != OBJ_TYPE_CONFIG)
      continue;

    load_config = holy_malloc (header->size - sizeof (struct holy_module_header) + 1);
    if (!load_config)
      {
	holy_print_error ();
	break;
      }
    holy_memcpy (load_config, (char *) header +
		 sizeof (struct holy_module_header),
		 header->size - sizeof (struct holy_module_header));
    load_config[header->size - sizeof (struct holy_module_header)] = 0;
    break;
  }
}

/* Write hook for the environment variables of root. Remove surrounding
   parentheses, if any.  */
static char *
holy_env_write_root (struct holy_env_var *var __attribute__ ((unused)),
		     const char *val)
{
  /* XXX Is it better to check the existence of the device?  */
  holy_size_t len = holy_strlen (val);

  if (val[0] == '(' && val[len - 1] == ')')
    return holy_strndup (val + 1, len - 2);

  return holy_strdup (val);
}

static void
holy_set_prefix_and_root (void)
{
  char *device = NULL;
  char *path = NULL;
  char *fwdevice = NULL;
  char *fwpath = NULL;
  char *prefix = NULL;
  struct holy_module_header *header;

  FOR_MODULES (header)
    if (header->type == OBJ_TYPE_PREFIX)
      prefix = (char *) header + sizeof (struct holy_module_header);

  holy_register_variable_hook ("root", 0, holy_env_write_root);

  holy_machine_get_bootlocation (&fwdevice, &fwpath);

  if (fwdevice)
    {
      char *cmdpath;

      holy_env_set ("cmddevice", fwdevice);
      holy_env_export ("cmddevice");

      cmdpath = holy_xasprintf ("(%s)%s", fwdevice, fwpath ? : "");
      if (cmdpath)
	{
	  holy_env_set ("cmdpath", cmdpath);
	  holy_env_export ("cmdpath");
	  holy_free (cmdpath);
	}
    }

  if (prefix)
    {
      char *pptr = NULL;
      if (prefix[0] == '(')
	{
	  pptr = holy_strrchr (prefix, ')');
	  if (pptr)
	    {
	      device = holy_strndup (prefix + 1, pptr - prefix - 1);
	      pptr++;
	    }
	}
      if (!pptr)
	pptr = prefix;
      if (pptr[0])
	path = holy_strdup (pptr);
    }

  if (!device && fwdevice)
    device = fwdevice;
  else if (fwdevice && (device[0] == ',' || !device[0]))
    {
      /* We have a partition, but still need to fill in the drive.  */
      char *comma, *new_device;

      for (comma = fwdevice; *comma; )
	{
	  if (comma[0] == '\\' && comma[1] == ',')
	    {
	      comma += 2;
	      continue;
	    }
	  if (*comma == ',')
	    break;
	  comma++;
	}
      if (*comma)
	{
	  char *drive = holy_strndup (fwdevice, comma - fwdevice);
	  new_device = holy_xasprintf ("%s%s", drive, device);
	  holy_free (drive);
	}
      else
	new_device = holy_xasprintf ("%s%s", fwdevice, device);

      holy_free (fwdevice);
      holy_free (device);
      device = new_device;
    }
  else
    holy_free (fwdevice);
  if (fwpath && !path)
    {
      holy_size_t len = holy_strlen (fwpath);
      while (len > 1 && fwpath[len - 1] == '/')
	fwpath[--len] = 0;
      if (len >= sizeof (holy_TARGET_CPU "-" holy_PLATFORM) - 1
	  && holy_memcmp (fwpath + len - (sizeof (holy_TARGET_CPU "-" holy_PLATFORM) - 1), holy_TARGET_CPU "-" holy_PLATFORM,
			  sizeof (holy_TARGET_CPU "-" holy_PLATFORM) - 1) == 0)
	fwpath[len - (sizeof (holy_TARGET_CPU "-" holy_PLATFORM) - 1)] = 0;
      path = fwpath;
    }
  else
    holy_free (fwpath);
  if (device)
    {
      char *prefix_set;
    
      prefix_set = holy_xasprintf ("(%s)%s", device, path ? : "");
      if (prefix_set)
	{
	  holy_env_set ("prefix", prefix_set);
	  holy_free (prefix_set);
	}
      holy_env_set ("root", device);
    }

  holy_free (device);
  holy_free (path);
  holy_print_error ();
}

/* Load the normal mode module and execute the normal mode if possible.  */
static void
holy_load_normal_mode (void)
{
  /* Load the module.  */
  holy_dl_load ("normal");

  /* Print errors if any.  */
  holy_print_error ();
  holy_errno = 0;

  holy_command_execute ("normal", 0, 0);
}

static void
reclaim_module_space (void)
{
  holy_addr_t modstart, modend;

  if (!holy_modbase)
    return;

#ifdef holy_MACHINE_PCBIOS
  modstart = holy_MEMORY_MACHINE_DECOMPRESSION_ADDR;
#else
  modstart = holy_modbase;
#endif
  modend = holy_modules_get_end ();
  holy_modbase = 0;

#if holy_KERNEL_PRELOAD_SPACE_REUSABLE
  holy_mm_init_region ((void *) modstart, modend - modstart);
#else
  (void) modstart;
  (void) modend;
#endif
}

/* The main routine.  */
void __attribute__ ((noreturn))
holy_main (void)
{
  /* First of all, initialize the machine.  */
  holy_machine_init ();

  holy_boot_time ("After machine init.");

  /* Hello.  */
  holy_setcolorstate (holy_TERM_COLOR_HIGHLIGHT);
  holy_printf ("Welcome to holy!\n\n");
  holy_setcolorstate (holy_TERM_COLOR_STANDARD);

  holy_load_config ();

  holy_boot_time ("Before loading embedded modules.");

  /* Load pre-loaded modules and free the space.  */
  holy_register_exported_symbols ();
#ifdef holy_LINKER_HAVE_INIT
  holy_arch_dl_init_linker ();
#endif  
  holy_load_modules ();

  holy_boot_time ("After loading embedded modules.");

  /* It is better to set the root device as soon as possible,
     for convenience.  */
  holy_set_prefix_and_root ();
  holy_env_export ("root");
  holy_env_export ("prefix");

  /* Reclaim space used for modules.  */
  reclaim_module_space ();

  holy_boot_time ("After reclaiming module space.");

  holy_register_core_commands ();

  holy_boot_time ("Before execution of embedded config.");

  if (load_config)
    holy_parser_execute (load_config);

  holy_boot_time ("After execution of embedded config. Attempt to go to normal mode");

  holy_load_normal_mode ();
  holy_rescue_run ();
}
