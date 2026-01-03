/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/pci.h>
#include <holy/misc.h>

static holy_uint32_t base_win[holy_MACHINE_PCI_NUM_WIN];
static const holy_size_t sizes_win[holy_MACHINE_PCI_NUM_WIN] =
  {holy_MACHINE_PCI_WIN1_SIZE, holy_MACHINE_PCI_WIN_SIZE,
   holy_MACHINE_PCI_WIN_SIZE};
/* Usage counters.  */
static int usage_win[holy_MACHINE_PCI_NUM_WIN];
static holy_addr_t addr_win[holy_MACHINE_PCI_NUM_WIN] =
  {holy_MACHINE_PCI_WIN1_ADDR, holy_MACHINE_PCI_WIN2_ADDR,
   holy_MACHINE_PCI_WIN3_ADDR};

holy_bonito_type_t holy_bonito_type;

static volatile void *
config_addr (holy_pci_address_t addr)
{
  if (holy_bonito_type == holy_BONITO_2F)
    {
      holy_MACHINE_PCI_CONF_CTRL_REG_2F = 1 << ((addr >> 11) & 0xf);
      return (volatile void *) (holy_MACHINE_PCI_CONFSPACE_2F
				| (addr & 0x07ff));
    }
  else
    {

      if (addr >> 16)
	return (volatile void *) (holy_MACHINE_PCI_CONFSPACE_3A_EXT | addr);
      else
	return (volatile void *) (holy_MACHINE_PCI_CONFSPACE_3A | addr);
    }
}

holy_uint32_t
holy_pci_read (holy_pci_address_t addr)
{
  return *(volatile holy_uint32_t *) config_addr (addr);
}

holy_uint16_t
holy_pci_read_word (holy_pci_address_t addr)
{
  return *(volatile holy_uint16_t *) config_addr (addr);
}

holy_uint8_t
holy_pci_read_byte (holy_pci_address_t addr)
{
  return *(volatile holy_uint8_t *) config_addr (addr);
}

void
holy_pci_write (holy_pci_address_t addr, holy_uint32_t data)
{
  *(volatile holy_uint32_t *) config_addr (addr) = data;
}

void
holy_pci_write_word (holy_pci_address_t addr, holy_uint16_t data)
{
  *(volatile holy_uint16_t *) config_addr (addr) = data;
}

void
holy_pci_write_byte (holy_pci_address_t addr, holy_uint8_t data)
{
  *(volatile holy_uint8_t *) config_addr (addr) = data;
}


static inline void
write_bases_2f (void)
{
  int i;
  holy_uint32_t reg = 0;
  for (i = 0; i < holy_MACHINE_PCI_NUM_WIN; i++)
    reg |= (((base_win[i] >> holy_MACHINE_PCI_WIN_SHIFT)
	     & holy_MACHINE_PCI_WIN_MASK)
	    << (i * holy_MACHINE_PCI_WIN_MASK_SIZE));
  holy_MACHINE_PCI_IO_CTRL_REG_2F = reg;
}

volatile void *
holy_pci_device_map_range (holy_pci_device_t dev __attribute__ ((unused)),
			   holy_addr_t base, holy_size_t size)
{
  if (holy_bonito_type == holy_BONITO_2F)
    {
      int i;
      holy_addr_t newbase;

      /* First try already used registers. */
      for (i = 0; i < holy_MACHINE_PCI_NUM_WIN; i++)
	if (usage_win[i] && base_win[i] <= base 
	    && base_win[i] + sizes_win[i] > base + size)
	  {
	    usage_win[i]++;
	    return (void *) 
	      (addr_win[i] | (base & holy_MACHINE_PCI_WIN_OFFSET_MASK));
	  }
      /* Map new register.  */
      newbase = base & ~holy_MACHINE_PCI_WIN_OFFSET_MASK;
      for (i = 0; i < holy_MACHINE_PCI_NUM_WIN; i++)
	if (!usage_win[i] && newbase <= base 
	    && newbase + sizes_win[i] > base + size)
	  {
	    usage_win[i]++;
	    base_win[i] = newbase;
	    write_bases_2f ();
	    return (void *) 
	      (addr_win[i] | (base & holy_MACHINE_PCI_WIN_OFFSET_MASK));
	  }
      holy_fatal ("Out of PCI windows.");
    }
  else
    {
      int region = 0;
      if (base >= 0x10000000
	  && base + size <= 0x18000000)
	region = 1;
      if (base >= 0x1c000000
	  && base + size <= 0x1f000000)
	region = 2;
      if (region == 0)
	holy_fatal ("Attempt to map out of regions");
      return (void *) (0xa0000000 | base);
    }
}

void *
holy_pci_device_map_range_cached (holy_pci_device_t dev,
				  holy_addr_t base, holy_size_t size)
{
  return (void *) (((holy_addr_t) holy_pci_device_map_range (dev, base, size))
		   & ~0x20000000);
}

void
holy_pci_device_unmap_range (holy_pci_device_t dev __attribute__ ((unused)),
			     volatile void *mem,
			     holy_size_t size __attribute__ ((unused)))
{
  if (holy_bonito_type == holy_BONITO_2F)
    {
      int i;
      for (i = 0; i < holy_MACHINE_PCI_NUM_WIN; i++)
	if (usage_win[i] && addr_win[i] 
	    == (((holy_addr_t) mem | 0x20000000)
		& ~holy_MACHINE_PCI_WIN_OFFSET_MASK))
	  {
	    usage_win[i]--;
	    return;
	  }
      holy_fatal ("Tried to unmap not mapped region");
    }
}
