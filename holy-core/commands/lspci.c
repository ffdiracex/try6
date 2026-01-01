/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/pci.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/mm.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_pci_classname
{
  int class;
  int subclass;
  const char *desc;
};

static const struct holy_pci_classname holy_pci_classes[] =
  {
    { 0, 0, "" },
    { 1, 0, "SCSI Controller" },
    { 1, 1, "IDE Controller" },
    { 1, 2, "Floppy Controller" },
    { 1, 3, "IPI Controller" },
    { 1, 4, "RAID Controller" },
    { 1, 6, "SATA Controller" },
    { 1, 0x80, "Mass storage Controller" },
    { 2, 0, "Ethernet Controller" },
    { 2, 1, "Token Ring Controller" },
    { 2, 2, "FDDI Controller" },
    { 2, 3, "ATM Controller" },
    { 2, 4, "ISDN Controller" },
    { 2, 0x80, "Network controller" },
    { 3, 0, "VGA Controller" },
    { 3, 1, "XGA Controller" },
    { 3, 2, "3D Controller" },
    { 3, 0x80, "Display Controller" },
    { 4, 0, "Multimedia Video Device" },
    { 4, 1, "Multimedia Audio Device" },
    { 4, 2, "Multimedia Telephony Device" },
    { 4, 0x80, "Multimedia device" },
    { 5, 0, "RAM Controller" },
    { 5, 1, "Flash Memory Controller" },
    { 5, 0x80, "Memory Controller" },
    { 6, 0, "Host Bridge" },
    { 6, 1, "ISA Bridge" },
    { 6, 2, "EISA Bride" },
    { 6, 3, "MCA Bridge" },
    { 6, 4, "PCI-PCI Bridge" },
    { 6, 5, "PCMCIA Bridge" },
    { 6, 6, "NuBus Bridge" },
    { 6, 7, "CardBus Bridge" },
    { 6, 8, "Raceway Bridge" },
    { 6, 0x80, "Unknown Bridge" },
    { 7, 0x80, "Communication controller" },
    { 8, 0x80, "System hardware" },
    { 9, 0, "Keyboard Controller" },
    { 9, 1, "Digitizer" },
    { 9, 2, "Mouse Controller" },
    { 9, 3, "Scanner Controller" },
    { 9, 4, "Gameport Controller" },
    { 9, 0x80, "Unknown Input Device" },
    { 10, 0, "Generic Docking Station" },
    { 10, 0x80, "Unknown Docking Station" },
    { 11, 0, "80386 Processor" },
    { 11, 1, "80486 Processor" },
    { 11, 2, "Pentium Processor" },
    { 11, 0x10, "Alpha Processor" },
    { 11, 0x20, "PowerPC Processor" },
    { 11, 0x30, "MIPS Processor" },
    { 11, 0x40, "Co-Processor" },
    { 11, 0x80, "Unknown Processor" },
    { 12, 3, "USB Controller" },
    { 12, 0x80, "Serial Bus Controller" },
    { 13, 0x80, "Wireless Controller" },
    { 14, 0, "I2O" },
    { 15, 0, "IrDA Controller" },
    { 15, 1, "Consumer IR" },
    { 15, 0x10, "RF-Controller" },
    { 15, 0x80, "Satellite Communication Controller" },
    { 16, 0, "Network Decryption" },
    { 16, 1, "Entertainment Decryption" },
    { 16, 0x80, "Unknown Decryption Controller" },
    { 17, 0, "Digital IO Module" },
    { 17, 0x80, "Unknown Data Input System" },
    { 0, 0, 0 },
  };

static const char *
holy_pci_get_class (int class, int subclass)
{
  const struct holy_pci_classname *curr = holy_pci_classes;

  while (curr->desc)
    {
      if (curr->class == class && curr->subclass == subclass)
	return curr->desc;
      curr++;
    }

  return 0;
}

static const struct holy_arg_option options[] =
  {
    {"iospace", 'i', 0, "show I/O spaces", 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static int iospace;

static int
holy_lspci_iter (holy_pci_device_t dev, holy_pci_id_t pciid,
		 void *data __attribute__ ((unused)))
{
  holy_uint32_t class;
  const char *sclass;
  holy_pci_address_t addr;
  int reg;

  holy_printf ("%02x:%02x.%x %04x:%04x", holy_pci_get_bus (dev),
	       holy_pci_get_device (dev), holy_pci_get_function (dev),
	       pciid & 0xFFFF, pciid >> 16);
  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  class = holy_pci_read (addr);

  /* Lookup the class name, if there isn't a specific one,
     retry with 0x80 to get the generic class name.  */
  sclass = holy_pci_get_class (class >> 24, (class >> 16) & 0xFF);
  if (! sclass)
    sclass = holy_pci_get_class (class >> 24, 0x80);
  if (! sclass)
    sclass = "";

  holy_printf (" [%04x] %s", (class >> 16) & 0xffff, sclass);

  holy_uint8_t pi = (class >> 8) & 0xff;
  if (pi)
    holy_printf (" [PI %02x]", pi);

  holy_printf ("\n");

  if (iospace)
    {
      reg = holy_PCI_REG_ADDRESSES;
      while (reg < holy_PCI_REG_CIS_POINTER)
	{
	  holy_uint64_t space;
	  addr = holy_pci_make_address (dev, reg);
	  space = holy_pci_read (addr);

	  reg += sizeof (holy_uint32_t);

	  if (space == 0)
	    continue;
	 
	  switch (space & holy_PCI_ADDR_SPACE_MASK)
	    {
	    case holy_PCI_ADDR_SPACE_IO:
	      holy_printf ("\tIO space %d at 0x%llx\n",
			   (unsigned) ((reg - holy_PCI_REG_ADDRESSES)
			    / sizeof (holy_uint32_t)) - 1,
			   (unsigned long long)
			   (space & holy_PCI_ADDR_IO_MASK));
	      break;
	    case holy_PCI_ADDR_SPACE_MEMORY:
	      if ((space & holy_PCI_ADDR_MEM_TYPE_MASK)
		  == holy_PCI_ADDR_MEM_TYPE_64)
		{
		  addr = holy_pci_make_address (dev, reg);
		  space |= ((holy_uint64_t) holy_pci_read (addr)) << 32;
		  reg += sizeof (holy_uint32_t);
		  holy_printf ("\t64-bit memory space %d at 0x%016llx [%s]\n",
			       (unsigned) ((reg - holy_PCI_REG_ADDRESSES)
				/ sizeof (holy_uint32_t)) - 2,
			       (unsigned long long)
			       (space & holy_PCI_ADDR_MEM_MASK),
			       space & holy_PCI_ADDR_MEM_PREFETCH
			       ? "prefetchable" : "non-prefetchable");
		 
		}
	      else
		holy_printf ("\t32-bit memory space %d at 0x%016llx [%s]\n",
			     (unsigned) ((reg - holy_PCI_REG_ADDRESSES)
			      / sizeof (holy_uint32_t)) - 1,
			     (unsigned long long) 
			     (space & holy_PCI_ADDR_MEM_MASK),
			     space & holy_PCI_ADDR_MEM_PREFETCH
			     ? "prefetchable" : "non-prefetchable");
	      break;
	    }
	}
    }


  return 0;
}

static holy_err_t
holy_cmd_lspci (holy_extcmd_context_t ctxt,
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  iospace = ctxt->state[0].set;
  holy_pci_iterate (holy_lspci_iter, NULL);
  return holy_ERR_NONE;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(lspci)
{
  cmd = holy_register_extcmd ("lspci", holy_cmd_lspci, 0, "[-i]",
			      N_("List PCI devices."), options);
}

holy_MOD_FINI(lspci)
{
  holy_unregister_extcmd (cmd);
}
