/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/pci.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/mm_private.h>
#include <holy/cache.h>

holy_MOD_LICENSE ("GPLv2+");

/* FIXME: correctly support 64-bit architectures.  */
/* #if holy_TARGET_SIZEOF_VOID_P == 4 */
struct holy_pci_dma_chunk *
holy_memalign_dma32 (holy_size_t align, holy_size_t size)
{
  void *ret;
  if (align < 64)
    align = 64;
  size = ALIGN_UP (size, align);
  ret = holy_memalign (align, size);
#if holy_CPU_SIZEOF_VOID_P == 8
  if ((holy_addr_t) ret >> 32)
    {
      /* Shouldn't happend since the only platform in this case is
	 x86_64-efi and it skips any regions > 4GiB because
	 of EFI bugs anyway.  */
      holy_error (holy_ERR_BUG, "allocation outside 32-bit range");
      return 0;
    }
#endif
  if (!ret)
    return 0;
  holy_arch_sync_dma_caches (ret, size);
  return ret;
}

/* FIXME: evil.  */
void
holy_dma_free (struct holy_pci_dma_chunk *ch)
{
  holy_size_t size = (((struct holy_mm_header *) ch) - 1)->size * holy_MM_ALIGN;
  holy_arch_sync_dma_caches (ch, size);
  holy_free (ch);
}
/* #endif */

#ifdef holy_MACHINE_MIPS_LOONGSON
volatile void *
holy_dma_get_virt (struct holy_pci_dma_chunk *ch)
{
  return (void *) ((((holy_uint32_t) ch) & 0x1fffffff) | 0xa0000000);
}

holy_uint32_t
holy_dma_get_phys (struct holy_pci_dma_chunk *ch)
{
  return (((holy_uint32_t) ch) & 0x1fffffff) | 0x80000000;
}
#else

volatile void *
holy_dma_get_virt (struct holy_pci_dma_chunk *ch)
{
  return (void *) ch;
}

holy_uint32_t
holy_dma_get_phys (struct holy_pci_dma_chunk *ch)
{
  return (holy_uint32_t) (holy_addr_t) ch;
}

#endif

holy_pci_address_t
holy_pci_make_address (holy_pci_device_t dev, int reg)
{
  return (1 << 31) | (dev.bus << 16) | (dev.device << 11)
    | (dev.function << 8) | reg;
}

void
holy_pci_iterate (holy_pci_iteratefunc_t hook, void *hook_data)
{
  holy_pci_device_t dev;
  holy_pci_address_t addr;
  holy_pci_id_t id;
  holy_uint32_t hdr;

  for (dev.bus = 0; dev.bus < holy_PCI_NUM_BUS; dev.bus++)
    {
      for (dev.device = 0; dev.device < holy_PCI_NUM_DEVICES; dev.device++)
	{
	  for (dev.function = 0; dev.function < 8; dev.function++)
	    {
	      addr = holy_pci_make_address (dev, holy_PCI_REG_PCI_ID);
	      id = holy_pci_read (addr);

	      /* Check if there is a device present.  */
	      if (id >> 16 == 0xFFFF)
		{
		  if (dev.function == 0)
		    /* Devices are required to implement function 0, so if
		       it's missing then there is no device here.  */
		    break;
		  else
		    continue;
		}

	      if (hook (dev, id, hook_data))
		return;

	      /* Probe only func = 0 if the device if not multifunction */
	      if (dev.function == 0)
		{
		  addr = holy_pci_make_address (dev, holy_PCI_REG_CACHELINE);
		  hdr = holy_pci_read (addr);
		  if (!(hdr & 0x800000))
		    break;
		}
	    }
	}
    }
}

holy_uint8_t
holy_pci_find_capability (holy_pci_device_t dev, holy_uint8_t cap)
{
  holy_uint8_t pos = 0x34;
  int ttl = 48;

  while (ttl--)
    {
      holy_uint8_t id;
      holy_pci_address_t addr;

      addr = holy_pci_make_address (dev, pos);
      pos = holy_pci_read_byte (addr);
      if (pos < 0x40)
	break;

      pos &= ~3;

      addr = holy_pci_make_address (dev, pos);
      id = holy_pci_read_byte (addr);

      if (id == 0xff)
	break;
      
      if (id == cap)
	return pos;
      pos++;
    }
  return 0;
}
