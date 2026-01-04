/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/ec.h>
#include <holy/machine/kernel.h>
#include <holy/machine/memory.h>
#include <holy/misc.h>
#include <holy/pci.h>
#include <holy/cs5536.h>
#include <holy/time.h>
#include <holy/term.h>
#include <holy/i18n.h>

void
holy_reboot (void)
{
  switch (holy_arch_machine)
    {
    case holy_ARCH_MACHINE_FULOONG2E:
      holy_outl (holy_inl (0xbfe00104) & ~4, 0xbfe00104);
      holy_outl (holy_inl (0xbfe00104) | 4, 0xbfe00104);
      break;
    case holy_ARCH_MACHINE_FULOONG2F:
      {
	holy_pci_device_t dev;
	if (!holy_cs5536_find (&dev))
	  break;
	holy_cs5536_write_msr (dev, holy_CS5536_MSR_DIVIL_RESET,
			       holy_cs5536_read_msr (dev,
						     holy_CS5536_MSR_DIVIL_RESET)
			       | 1);
	break;
      }
    case holy_ARCH_MACHINE_YEELOONG:
      holy_write_ec (holy_MACHINE_EC_COMMAND_REBOOT);
      break;
    case holy_ARCH_MACHINE_YEELOONG_3A:
      holy_millisleep (1);
      holy_outb (0x4e, holy_MACHINE_PCI_IO_BASE_3A | 0x66);
      holy_millisleep (1);
      holy_outb (1, holy_MACHINE_PCI_IO_BASE_3A | 0x62);
      holy_millisleep (5000);
    }
  holy_millisleep (1500);

  holy_puts_ (N_("Reboot failed"));
  holy_refresh ();
  while (1);
}
