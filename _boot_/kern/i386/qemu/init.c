/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/mm.h>
#include <holy/machine/time.h>
#include <holy/machine/memory.h>
#include <holy/machine/console.h>
#include <holy/offsets.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/loader.h>
#include <holy/env.h>
#include <holy/cache.h>
#include <holy/time.h>
#include <holy/symbol.h>
#include <holy/cpu/io.h>
#include <holy/cpu/floppy.h>
#include <holy/cpu/tsc.h>
#include <holy/machine/kernel.h>
#include <holy/pci.h>

extern holy_uint8_t _start[];
extern holy_uint8_t _end[];
extern holy_uint8_t _edata[];

void  __attribute__ ((noreturn))
holy_exit (void)
{
  /* We can't use holy_fatal() in this function.  This would create an infinite
     loop, since holy_fatal() calls holy_abort() which in turn calls holy_exit().  */
  while (1)
    holy_cpu_idle ();
}

holy_addr_t holy_modbase;

/* Helper for holy_machine_init.  */
static int
heap_init (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	   void *data __attribute__ ((unused)))
{
  holy_uint64_t begin = addr, end = addr + size;

#if holy_CPU_SIZEOF_VOID_P == 4
  /* Restrict ourselves to 32-bit memory space.  */
  if (begin > holy_ULONG_MAX)
    return 0;
  if (end > holy_ULONG_MAX)
    end = holy_ULONG_MAX;
#endif

  if (type != holy_MEMORY_AVAILABLE)
    return 0;

  /* Avoid the lower memory.  */
  if (begin < holy_MEMORY_MACHINE_LOWER_SIZE)
    begin = holy_MEMORY_MACHINE_LOWER_SIZE;

  if (end <= begin)
    return 0;

  holy_mm_init_region ((void *) (holy_addr_t) begin, (holy_size_t) (end - begin));

  return 0;
}

struct resource
{
  holy_pci_device_t dev;
  holy_size_t size;
  unsigned type:4;
  unsigned bar:3;
};

struct iterator_ctx
{
  struct resource *resources;
  holy_size_t nresources;
};

/* We don't support bridges, so can't have more than 32 devices.  */
#define MAX_DEVICES 32

static int
find_resources (holy_pci_device_t dev,
		holy_pci_id_t pciid __attribute__ ((unused)),
		void *data)
{
  struct iterator_ctx *ctx = data;
  int bar;

  if (ctx->nresources >= MAX_DEVICES * 6)
    return 1;

  for (bar = 0; bar < 6; bar++)
    {
      holy_pci_address_t addr;
      holy_uint32_t ones, zeros, mask;
      struct resource *res;
      addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0
				    + 4 * bar);
      holy_pci_write (addr, 0xffffffff);
      holy_pci_read (addr);
      ones = holy_pci_read (addr);
      holy_pci_write (addr, 0);
      holy_pci_read (addr);
      zeros = holy_pci_read (addr);
      if (ones == zeros)
	continue;
      res = &ctx->resources[ctx->nresources++];
      if ((zeros & holy_PCI_ADDR_SPACE_MASK) == holy_PCI_ADDR_SPACE_IO)
	mask = holy_PCI_ADDR_SPACE_MASK;
      else
	mask = (holy_PCI_ADDR_MEM_TYPE_MASK | holy_PCI_ADDR_SPACE_MASK | holy_PCI_ADDR_MEM_PREFETCH);

      res->type = ones & mask;
      res->dev = dev;
      res->bar = bar;
      res->size = (~((zeros ^ ones)) | mask) + 1;
      if ((zeros & (holy_PCI_ADDR_MEM_TYPE_MASK | holy_PCI_ADDR_SPACE_MASK))
	  == (holy_PCI_ADDR_SPACE_MEMORY | holy_PCI_ADDR_MEM_TYPE_64))
	bar++;
    }
  return 0;
}

static int
enable_cards (holy_pci_device_t dev,
	      holy_pci_id_t pciid __attribute__ ((unused)),
	      void *data __attribute__ ((unused)))
{
  holy_uint16_t cmd = 0;
  holy_pci_address_t addr;
  holy_uint32_t class;
  int bar;

  for (bar = 0; bar < 6; bar++)
    {
      holy_uint32_t val;
      addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0
				    + 4 * bar);
      val = holy_pci_read (addr);
      if (!val)
	continue;
      if ((val & holy_PCI_ADDR_SPACE_MASK) == holy_PCI_ADDR_SPACE_IO)
	cmd |= holy_PCI_COMMAND_IO_ENABLED;
      else
	cmd |= holy_PCI_COMMAND_MEM_ENABLED;
    }

  class = (holy_pci_read (addr) >> 16) & 0xffff;

  if (class == holy_PCI_CLASS_SUBCLASS_VGA)
    cmd |= holy_PCI_COMMAND_IO_ENABLED
      | holy_PCI_COMMAND_MEM_ENABLED;

  if (class == holy_PCI_CLASS_SUBCLASS_USB)
    return 0;
  
  addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
  holy_pci_write (addr, cmd);

  return 0;
}

static void
holy_pci_assign_addresses (void)
{
  struct iterator_ctx ctx;

  {
    struct resource resources[MAX_DEVICES * 6];
    int done;
    unsigned i;
    ctx.nresources = 0;
    ctx.resources = resources;
    holy_uint32_t memptr = 0xf0000000;
    holy_uint16_t ioptr = 0x1000;

    holy_pci_iterate (find_resources, &ctx);
    /* FIXME: do we need a better sort here?  */
    do
      {
	done = 0;
	for (i = 0; i + 1 < ctx.nresources; i++)
	  if (resources[i].size < resources[i+1].size)
	    {
	      struct resource t;
	      t = resources[i];
	      resources[i] = resources[i+1];
	      resources[i+1] = t;
	      done = 1;
	    }
      }
    while (done);

    for (i = 0; i < ctx.nresources; i++)
      {
	holy_pci_address_t addr;
	addr = holy_pci_make_address (resources[i].dev,
				      holy_PCI_REG_ADDRESS_REG0
				      + 4 * resources[i].bar);
	if ((resources[i].type & holy_PCI_ADDR_SPACE_MASK)
	    == holy_PCI_ADDR_SPACE_IO)
	  {
	    holy_pci_write (addr, ioptr | resources[i].type);
	    ioptr += resources[i].size;
	  }
	else
	  {
	    holy_pci_write (addr, memptr | resources[i].type);
	    memptr += resources[i].size;
	    if ((resources[i].type & (holy_PCI_ADDR_MEM_TYPE_MASK
				      | holy_PCI_ADDR_SPACE_MASK))
		== (holy_PCI_ADDR_SPACE_MEMORY | holy_PCI_ADDR_MEM_TYPE_64))
	      {
		addr = holy_pci_make_address (resources[i].dev,
					      holy_PCI_REG_ADDRESS_REG0
					      + 4 * resources[i].bar + 4);
		holy_pci_write (addr, 0);
	      }
	  }	  
      }
    holy_pci_iterate (enable_cards, NULL);
  }
}

void
holy_machine_init (void)
{
  holy_modbase = holy_core_entry_addr + (_edata - _start);

  holy_pci_assign_addresses ();

  holy_qemu_init_cirrus ();

  holy_vga_text_init ();

  holy_machine_mmap_init ();
  holy_machine_mmap_iterate (heap_init, NULL);


  holy_tsc_init ();
}

void
holy_machine_get_bootlocation (char **device __attribute__ ((unused)),
			       char **path __attribute__ ((unused)))
{
}

void
holy_machine_fini (int flags)
{
  if (flags & holy_LOADER_FLAG_NORETURN)
    holy_vga_text_fini ();
  holy_stop_floppy ();
}
