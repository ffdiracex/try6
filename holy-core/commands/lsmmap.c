/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/memory.h>
#include <holy/mm.h>

holy_MOD_LICENSE ("GPLv2+");

#ifndef holy_MACHINE_EMU
static const char *names[] =
  {
    [holy_MEMORY_AVAILABLE] = N_("available RAM"),
    [holy_MEMORY_RESERVED] = N_("reserved RAM"),
    /* TRANSLATORS: this refers to memory where ACPI tables are stored
       and which can be used by OS once it loads ACPI tables.  */
    [holy_MEMORY_ACPI] = N_("ACPI reclaimable RAM"),
    /* TRANSLATORS: this refers to memory which ACPI-compliant OS
       is required to save accross hibernations.  */
    [holy_MEMORY_NVS] = N_("ACPI non-volatile storage RAM"),
    [holy_MEMORY_BADRAM] = N_("faulty RAM (BadRAM)"),
    [holy_MEMORY_PERSISTENT] = N_("persistent RAM"),
    [holy_MEMORY_PERSISTENT_LEGACY] = N_("persistent RAM (legacy)"),
    [holy_MEMORY_COREBOOT_TABLES] = N_("RAM holding coreboot tables"),
    [holy_MEMORY_CODE] = N_("RAM holding firmware code")
  };

/* Helper for holy_cmd_lsmmap.  */
static int
lsmmap_hook (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	     void *data __attribute__ ((unused)))
{
  if (type < (int) ARRAY_SIZE (names) && type >= 0 && names[type])
    holy_printf_ (N_("base_addr = 0x%llx, length = 0x%llx, %s\n"),
		  (long long) addr, (long long) size, _(names[type]));
  else
    holy_printf_ (N_("base_addr = 0x%llx, length = 0x%llx, type = 0x%x\n"),
		  (long long) addr, (long long) size, type);
  return 0;
}
#endif

static holy_err_t
holy_cmd_lsmmap (holy_command_t cmd __attribute__ ((unused)),
		 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))

{
#ifndef holy_MACHINE_EMU
  holy_machine_mmap_iterate (lsmmap_hook, NULL);
#endif

  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(lsmmap)
{
  cmd = holy_register_command ("lsmmap", holy_cmd_lsmmap,
			       0, N_("List memory map provided by firmware."));
}

holy_MOD_FINI(lsmmap)
{
  holy_unregister_command (cmd);
}
