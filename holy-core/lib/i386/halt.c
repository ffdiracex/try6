/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/cpu/io.h>
#include <holy/misc.h>
#include <holy/acpi.h>
#include <holy/i18n.h>
#include <holy/pci.h>
#include <holy/mm.h>

const char bochs_shutdown[] = "Shutdown";

/*
 *  This call is special...  it never returns...  in fact it should simply
 *  hang at this point!
 */
static inline void  __attribute__ ((noreturn))
stop (void)
{
  asm volatile ("cli");
  while (1)
    {
      asm volatile ("hlt");
    }
}

static int
holy_shutdown_pci_iter (holy_pci_device_t dev, holy_pci_id_t pciid,
			void *data __attribute__ ((unused)))
{
  /* QEMU.  */
  if (pciid == 0x71138086)
    {
      holy_pci_address_t addr;
      addr = holy_pci_make_address (dev, 0x40);
      holy_pci_write (addr, 0x7001);
      addr = holy_pci_make_address (dev, 0x80);
      holy_pci_write (addr, holy_pci_read (addr) | 1);
      holy_outw (0x2000, 0x7004);
    }
  return 0;
}

void
holy_halt (void)
{
  unsigned int i;

#if defined (holy_MACHINE_COREBOOT) || defined (holy_MACHINE_MULTIBOOT)
  holy_acpi_halt ();
#endif

  /* Disable interrupts.  */
  __asm__ __volatile__ ("cli");

  /* Bochs, QEMU, etc. Removed in newer QEMU releases.  */
  for (i = 0; i < sizeof (bochs_shutdown) - 1; i++)
    holy_outb (bochs_shutdown[i], 0x8900);

  holy_pci_iterate (holy_shutdown_pci_iter, NULL);

  holy_puts_ (N_("holy doesn't know how to halt this machine yet!"));

  /* In order to return we'd have to check what the previous status of IF
     flag was.  But user most likely doesn't want to return anyway ...  */
  stop ();
}
