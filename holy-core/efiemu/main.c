/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/err.h>
#include <holy/normal.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/efiemu/efiemu.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* System table. Two version depending on mode */
holy_efi_system_table32_t *holy_efiemu_system_table32 = 0;
holy_efi_system_table64_t *holy_efiemu_system_table64 = 0;
/* Modules may need to execute some actions after memory allocation happens */
static struct holy_efiemu_prepare_hook *efiemu_prepare_hooks = 0;
/* Linked list of configuration tables */
static struct holy_efiemu_configuration_table *efiemu_config_tables = 0;
static int prepared = 0;

/* Free all allocated space */
holy_err_t
holy_efiemu_unload (void)
{
  struct holy_efiemu_configuration_table *cur, *d;
  struct holy_efiemu_prepare_hook *curhook, *d2;
  holy_efiemu_loadcore_unload ();

  holy_efiemu_mm_unload ();

  for (cur = efiemu_config_tables; cur;)
    {
      d = cur->next;
      if (cur->unload)
	cur->unload (cur->data);
      holy_free (cur);
      cur = d;
    }
  efiemu_config_tables = 0;

  for (curhook = efiemu_prepare_hooks; curhook;)
    {
      d2 = curhook->next;
      if (curhook->unload)
	curhook->unload (curhook->data);
      holy_free (curhook);
      curhook = d2;
    }
  efiemu_prepare_hooks = 0;

  prepared = 0;

  return holy_ERR_NONE;
}

/* Remove previously registered table from the list */
holy_err_t
holy_efiemu_unregister_configuration_table (holy_efi_guid_t guid)
{
  struct holy_efiemu_configuration_table *cur, *prev;

  /* Special treating if head is to remove */
  while (efiemu_config_tables
	 && !holy_memcmp (&(efiemu_config_tables->guid), &guid, sizeof (guid)))
    {
      if (efiemu_config_tables->unload)
	  efiemu_config_tables->unload (efiemu_config_tables->data);
	cur = efiemu_config_tables->next;
	holy_free (efiemu_config_tables);
	efiemu_config_tables = cur;
    }
  if (!efiemu_config_tables)
    return holy_ERR_NONE;

  /* Remove from chain */
  for (prev = efiemu_config_tables, cur = prev->next; cur;)
    if (holy_memcmp (&(cur->guid), &guid, sizeof (guid)) == 0)
      {
	if (cur->unload)
	  cur->unload (cur->data);
	prev->next = cur->next;
	holy_free (cur);
	cur = prev->next;
      }
    else
      {
	prev = cur;
	cur = cur->next;
      }
  return holy_ERR_NONE;
}

holy_err_t
holy_efiemu_register_prepare_hook (holy_err_t (*hook) (void *data),
				   void (*unload) (void *data),
				   void *data)
{
  struct holy_efiemu_prepare_hook *nhook;
  nhook = (struct holy_efiemu_prepare_hook *) holy_malloc (sizeof (*nhook));
  if (! nhook)
    return holy_errno;
  nhook->hook = hook;
  nhook->unload = unload;
  nhook->data = data;
  nhook->next = efiemu_prepare_hooks;
  efiemu_prepare_hooks = nhook;
  return holy_ERR_NONE;
}

/* Register a configuration table either supplying the address directly
   or with a hook
*/
holy_err_t
holy_efiemu_register_configuration_table (holy_efi_guid_t guid,
					  void * (*get_table) (void *data),
					  void (*unload) (void *data),
					  void *data)
{
  struct holy_efiemu_configuration_table *tbl;
  holy_err_t err;

 err = holy_efiemu_unregister_configuration_table (guid);
  if (err)
    return err;

  tbl = (struct holy_efiemu_configuration_table *) holy_malloc (sizeof (*tbl));
  if (! tbl)
    return holy_errno;

  tbl->guid = guid;
  tbl->get_table = get_table;
  tbl->unload = unload;
  tbl->data = data;
  tbl->next = efiemu_config_tables;
  efiemu_config_tables = tbl;

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_efiemu_unload (holy_command_t cmd __attribute__ ((unused)),
			int argc __attribute__ ((unused)),
			char *args[] __attribute__ ((unused)))
{
  return holy_efiemu_unload ();
}

static holy_err_t
holy_cmd_efiemu_prepare (holy_command_t cmd __attribute__ ((unused)),
			 int argc __attribute__ ((unused)),
			 char *args[] __attribute__ ((unused)))
{
  return holy_efiemu_prepare ();
}



/* Load the runtime from the file FILENAME.  */
static holy_err_t
holy_efiemu_load_file (const char *filename)
{
  holy_file_t file;
  holy_err_t err;

  file = holy_file_open (filename);
  if (! file)
    return holy_errno;

  err = holy_efiemu_mm_init ();
  if (err)
    {
      holy_file_close (file);
      holy_efiemu_unload ();
      return err;
    }

  holy_dprintf ("efiemu", "mm initialized\n");

  err = holy_efiemu_loadcore_init (file, filename);
  if (err)
    {
      holy_file_close (file);
      holy_efiemu_unload ();
      return err;
    }

  holy_file_close (file);

  /* For configuration tables entry in system table. */
  holy_efiemu_request_symbols (1);

  return holy_ERR_NONE;
}

holy_err_t
holy_efiemu_autocore (void)
{
  const char *prefix;
  char *filename;
  const char *suffix;
  holy_err_t err;

  if (holy_efiemu_sizeof_uintn_t () != 0)
    return holy_ERR_NONE;

  prefix = holy_env_get ("prefix");

  if (! prefix)
    return holy_error (holy_ERR_FILE_NOT_FOUND,
		       N_("variable `%s' isn't set"), "prefix");

  suffix = holy_efiemu_get_default_core_name ();

  filename = holy_xasprintf ("%s/" holy_TARGET_CPU "-" holy_PLATFORM "/%s",
			     prefix, suffix);
  if (! filename)
    return holy_errno;

  err = holy_efiemu_load_file (filename);
  holy_free (filename);
  if (err)
    return err;
#ifndef holy_MACHINE_EMU
  err = holy_machine_efiemu_init_tables ();
  if (err)
    return err;
#endif

  return holy_ERR_NONE;
}

holy_err_t
holy_efiemu_prepare (void)
{
  holy_err_t err;

  if (prepared)
    return holy_ERR_NONE;

  err = holy_efiemu_autocore ();
  if (err)
    return err;

  holy_dprintf ("efiemu", "Preparing %d-bit efiemu\n",
		8 * holy_efiemu_sizeof_uintn_t ());

  /* Create NVRAM. */
  holy_efiemu_pnvram ();

  prepared = 1;

  if (holy_efiemu_sizeof_uintn_t () == 4)
    return holy_efiemu_prepare32 (efiemu_prepare_hooks, efiemu_config_tables);
  else
    return holy_efiemu_prepare64 (efiemu_prepare_hooks, efiemu_config_tables);
}


static holy_err_t
holy_cmd_efiemu_load (holy_command_t cmd __attribute__ ((unused)),
		      int argc, char *args[])
{
  holy_err_t err;

  holy_efiemu_unload ();

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  err = holy_efiemu_load_file (args[0]);
  if (err)
    return err;
#ifndef holy_MACHINE_EMU
  err = holy_machine_efiemu_init_tables ();
  if (err)
    return err;
#endif
  return holy_ERR_NONE;
}

static holy_command_t cmd_loadcore, cmd_prepare, cmd_unload;

holy_MOD_INIT(efiemu)
{
  cmd_loadcore = holy_register_command ("efiemu_loadcore",
					holy_cmd_efiemu_load,
					N_("FILE"),
					N_("Load and initialize EFI emulator."));
  cmd_prepare = holy_register_command ("efiemu_prepare",
				       holy_cmd_efiemu_prepare,
				       0,
				       N_("Finalize loading of EFI emulator."));
  cmd_unload = holy_register_command ("efiemu_unload", holy_cmd_efiemu_unload,
				      0,
				      N_("Unload EFI emulator."));
}

holy_MOD_FINI(efiemu)
{
  holy_unregister_command (cmd_loadcore);
  holy_unregister_command (cmd_prepare);
  holy_unregister_command (cmd_unload);
}
