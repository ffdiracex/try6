/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/cs5536.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_lsspd (holy_command_t cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  holy_pci_device_t dev;
  holy_port_t smbbase;
  int i;
  holy_err_t err;

  if (!holy_cs5536_find (&dev))
    {
      holy_puts_ (N_("No CS5536 found"));
      return holy_ERR_NONE;
    }
  holy_printf_ (N_("CS5536 at %d:%d.%d\n"), holy_pci_get_bus (dev),
		holy_pci_get_device (dev), holy_pci_get_function (dev));
  
  err = holy_cs5536_init_smbus (dev, 0x7fff, &smbbase);
  if (err)
    return err;

  /* TRANSLATORS: System management bus is often used to access components like
     RAM (info only, not data) or batteries. I/O space is where in memory
     its ports are.  */
  holy_printf_ (N_("System management bus controller I/O space is at 0x%x\n"),
		smbbase);

  for (i = holy_SMB_RAM_START_ADDR;
       i < holy_SMB_RAM_START_ADDR + holy_SMB_RAM_NUM_MAX; i++)
    {
      struct holy_smbus_spd spd;
      holy_memset (&spd, 0, sizeof (spd));
      /* TRANSLATORS: it's shown in a report in a way
	 like number 1: ... number 2: ...
       */
      holy_printf_ (N_("RAM slot number %d\n"), i);
      err = holy_cs5536_read_spd (smbbase, i, &spd);
      if (err)
	{
	  holy_print_error ();
	  continue;
	}
      holy_printf_ (N_("Written SPD bytes: %d B.\n"), spd.written_size);
      holy_printf_ (N_("Total flash size: %d B.\n"),
		    1 << spd.log_total_flash_size);
      if (spd.memory_type == holy_SMBUS_SPD_MEMORY_TYPE_DDR2)
	{
	  char str[sizeof (spd.ddr2.part_number) + 1];
	  holy_puts_ (N_("Memory type: DDR2."));
	  holy_memcpy (str, spd.ddr2.part_number,
		       sizeof (spd.ddr2.part_number));
	  str[sizeof (spd.ddr2.part_number)] = 0;
	  holy_printf_ (N_("Part no: %s.\n"), str);
	}
      else
	holy_puts_ (N_("Memory type: Unknown."));
    }

  return holy_ERR_NONE;
}

static holy_command_t cmd;

holy_MOD_INIT(lsspd)
{
  cmd = holy_register_command ("lsspd", holy_cmd_lsspd, 0,
			       N_("Print Memory information."));
}

holy_MOD_FINI(lsspd)
{
  holy_unregister_command (cmd);
}
