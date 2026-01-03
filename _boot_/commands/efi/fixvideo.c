/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/pci.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/mm.h>

holy_MOD_LICENSE ("GPLv2+");

static struct holy_video_patch
{
  const char *name;
  holy_uint32_t pci_id;
  holy_uint32_t mmio_bar;
  holy_uint32_t mmio_reg;
  holy_uint32_t mmio_old;
} video_patches[] =
  {
    {"Intel 945GM", 0x27a28086, 0, 0x71184, 0x1000000}, /* DSPBBASE  */
    {"Intel 965GM", 0x2a028086, 0, 0x7119C, 0x1000000}, /* DSPBSURF  */
    {0, 0, 0, 0, 0}
  };

static int
scan_card (holy_pci_device_t dev, holy_pci_id_t pciid,
	   void *data __attribute__ ((unused)))
{
  holy_pci_address_t addr;

  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  if (holy_pci_read_byte (addr + 3) == 0x3)
    {
      struct holy_video_patch *p = video_patches;

      while (p->name)
	{
	  if (p->pci_id == pciid)
	    {
	      holy_addr_t base;

	      holy_dprintf ("fixvideo", "Found graphic card: %s\n", p->name);
	      addr += 8 + p->mmio_bar * 4;
	      base = holy_pci_read (addr);
	      if ((! base) || (base & holy_PCI_ADDR_SPACE_IO) ||
		  (base & holy_PCI_ADDR_MEM_PREFETCH))
		holy_dprintf ("fixvideo", "Invalid MMIO bar %d\n", p->mmio_bar);
	      else
		{
		  base &= holy_PCI_ADDR_MEM_MASK;
		  base += p->mmio_reg;

		  if (*((volatile holy_uint32_t *) base) != p->mmio_old)
		    holy_dprintf ("fixvideo", "Old value doesn't match\n");
		  else
		    {
		      *((volatile holy_uint32_t *) base) = 0;
		      if (*((volatile holy_uint32_t *) base))
			holy_dprintf ("fixvideo", "Setting MMIO fails\n");
		    }
		}

	      return 1;
	    }
	  p++;
	}

      holy_dprintf ("fixvideo", "Unknown graphic card: %x\n", pciid);
    }

  return 0;
}

static holy_err_t
holy_cmd_fixvideo (holy_command_t cmd __attribute__ ((unused)),
		   int argc __attribute__ ((unused)),
		   char *argv[] __attribute__ ((unused)))
{
  holy_pci_iterate (scan_card, NULL);
  return 0;
}

static holy_command_t cmd_fixvideo;

holy_MOD_INIT(fixvideo)
{
  cmd_fixvideo = holy_register_command ("fix_video", holy_cmd_fixvideo,
					0, N_("Fix video problem."));

}

holy_MOD_FINI(fixvideo)
{
  holy_unregister_command (cmd_fixvideo);
}
