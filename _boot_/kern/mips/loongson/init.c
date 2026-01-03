/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/time.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/machine/time.h>
#include <holy/machine/kernel.h>
#include <holy/machine/memory.h>
#include <holy/memory.h>
#include <holy/mips/loongson.h>
#include <holy/cs5536.h>
#include <holy/term.h>
#include <holy/cpu/memory.h>
#include <holy/i18n.h>
#include <holy/video.h>
#include <holy/terminfo.h>
#include <holy/keyboard_layouts.h>
#include <holy/serial.h>
#include <holy/loader.h>
#include <holy/at_keyboard.h>

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  hook (holy_ARCH_LOWMEMPSTART, holy_arch_memsize << 20,
	holy_MEMORY_AVAILABLE, hook_data);
  hook (holy_ARCH_HIGHMEMPSTART, holy_arch_highmemsize << 20,
	holy_MEMORY_AVAILABLE, hook_data);
  return holy_ERR_NONE;
}

/* Helper for init_pci.  */
static int
set_card (holy_pci_device_t dev, holy_pci_id_t pciid,
	  void *data __attribute__ ((unused)))
{
  holy_pci_address_t addr;
  /* We could use holy_pci_assign_addresses for this but we prefer to
     have exactly same memory map as on pmon.  */
  switch (pciid)
    {
    case holy_LOONGSON_OHCI_PCIID:
      addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0);
      holy_pci_write (addr, 0x5025000);
      addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
      holy_pci_write_word (addr, holy_PCI_COMMAND_SERR_ENABLE
			   | holy_PCI_COMMAND_PARITY_ERROR
			   | holy_PCI_COMMAND_BUS_MASTER
			   | holy_PCI_COMMAND_MEM_ENABLED);

      addr = holy_pci_make_address (dev, holy_PCI_REG_STATUS);
      holy_pci_write_word (addr, 0x0200 | holy_PCI_STATUS_CAPABILITIES);
      break;
    case holy_LOONGSON_EHCI_PCIID:
      addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0);
      holy_pci_write (addr, 0x5026000);
      addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
      holy_pci_write_word (addr, holy_PCI_COMMAND_SERR_ENABLE
			   | holy_PCI_COMMAND_PARITY_ERROR
			   | holy_PCI_COMMAND_BUS_MASTER
			   | holy_PCI_COMMAND_MEM_ENABLED);

      addr = holy_pci_make_address (dev, holy_PCI_REG_STATUS);
      holy_pci_write_word (addr, (1 << holy_PCI_STATUS_DEVSEL_TIMING_SHIFT)
			   | holy_PCI_STATUS_CAPABILITIES);
      break;
    }
  return 0;
}

static void
init_pci (void)
{
  *((volatile holy_uint32_t *) holy_CPU_LOONGSON_PCI_HIT1_SEL_LO) = 0x8000000c;
  *((volatile holy_uint32_t *) holy_CPU_LOONGSON_PCI_HIT1_SEL_HI) = 0xffffffff;

  /* Setup PCI controller.  */
  *((volatile holy_uint16_t *) (holy_MACHINE_PCI_CONTROLLER_HEADER
				+ holy_PCI_REG_COMMAND))
    = holy_PCI_COMMAND_PARITY_ERROR | holy_PCI_COMMAND_BUS_MASTER
    | holy_PCI_COMMAND_MEM_ENABLED;
  *((volatile holy_uint16_t *) (holy_MACHINE_PCI_CONTROLLER_HEADER
				+ holy_PCI_REG_STATUS))
    = (1 << holy_PCI_STATUS_DEVSEL_TIMING_SHIFT)
    | holy_PCI_STATUS_FAST_B2B_CAPABLE | holy_PCI_STATUS_66MHZ_CAPABLE
    | holy_PCI_STATUS_CAPABILITIES;

  *((volatile holy_uint32_t *) (holy_MACHINE_PCI_CONTROLLER_HEADER
				+ holy_PCI_REG_CACHELINE)) = 0xff;
  *((volatile holy_uint32_t *) (holy_MACHINE_PCI_CONTROLLER_HEADER
				+ holy_PCI_REG_ADDRESS_REG0))
    = 0x80000000 | holy_PCI_ADDR_MEM_TYPE_64 | holy_PCI_ADDR_MEM_PREFETCH;
  *((volatile holy_uint32_t *) (holy_MACHINE_PCI_CONTROLLER_HEADER
				+ holy_PCI_REG_ADDRESS_REG1)) = 0;

  holy_pci_iterate (set_card, NULL);
}

void
holy_machine_init (void)
{
  holy_addr_t modend;
  holy_uint32_t prid;

  asm volatile ("mfc0 %0, " holy_CPU_LOONGSON_COP0_PRID : "=r" (prid));

  switch (prid)
    {
      /* Loongson 2E.  */
    case 0x6302:
      holy_arch_machine = holy_ARCH_MACHINE_FULOONG2E;
      holy_bonito_type = holy_BONITO_2F;
      break;
      /* Loongson 2F.  */
    case 0x6303:
      if (holy_arch_machine != holy_ARCH_MACHINE_FULOONG2F
	  && holy_arch_machine != holy_ARCH_MACHINE_YEELOONG)
	holy_arch_machine = holy_ARCH_MACHINE_YEELOONG;
      holy_bonito_type = holy_BONITO_2F;
      break;
      /* Loongson 3A. */
    case 0x6305:
      holy_arch_machine = holy_ARCH_MACHINE_YEELOONG_3A;
      holy_bonito_type = holy_BONITO_3A;
      break;
    }

  /* FIXME: measure this.  */
  if (holy_arch_busclock == 0)
    {
      holy_arch_busclock = 66000000;
      holy_arch_cpuclock = 797000000;
    }

  holy_install_get_time_ms (holy_rtc_get_time_ms);

  if (holy_arch_memsize == 0)
    {
      holy_port_t smbbase;
      holy_err_t err;
      holy_pci_device_t dev;
      struct holy_smbus_spd spd;
      unsigned totalmem;
      int i;

      if (!holy_cs5536_find (&dev))
	holy_fatal ("No CS5536 found\n");

      err = holy_cs5536_init_smbus (dev, 0x7ff, &smbbase);
      if (err)
	holy_fatal ("Couldn't init SMBus: %s\n", holy_errmsg);

      /* Yeeloong and Fuloong have only one memory slot.  */
      err = holy_cs5536_read_spd (smbbase, holy_SMB_RAM_START_ADDR, &spd);
      if (err)
	holy_fatal ("Couldn't read SPD: %s\n", holy_errmsg);
      for (i = 5; i < 13; i++)
	if (spd.ddr2.rank_capacity & (1 << (i & 7)))
	  break;
      /* Something is wrong.  */
      if (i == 13)
	totalmem = 256;
      else
	totalmem = ((spd.ddr2.num_of_ranks
		     & holy_SMBUS_SPD_MEMORY_NUM_OF_RANKS_MASK) + 1) << (i + 2);
      
      if (totalmem >= 256)
	{
	  holy_arch_memsize = 256;
	  holy_arch_highmemsize = totalmem - 256;
	}
      else
	{
	  holy_arch_memsize = totalmem;
	  holy_arch_highmemsize = 0;
	}

      holy_cs5536_init_geode (dev);

      init_pci ();
    }

  modend = holy_modules_get_end ();
  holy_mm_init_region ((void *) modend, (holy_arch_memsize << 20)
		       - (modend - holy_ARCH_LOWMEMVSTART));
  /* FIXME: use upper memory as well.  */

  /* Initialize output terminal (can't be done earlier, as gfxterm
     relies on a working heap.  */
  holy_video_sm712_init ();
  holy_video_sis315pro_init ();
  holy_video_radeon_fuloong2e_init ();
  holy_video_radeon_yeeloong3a_init ();
  holy_font_init ();
  holy_gfxterm_init ();

  holy_keylayouts_init ();
  if (holy_arch_machine == holy_ARCH_MACHINE_YEELOONG
      || holy_arch_machine == holy_ARCH_MACHINE_YEELOONG_3A)
    holy_at_keyboard_init ();

  holy_terminfo_init ();
  holy_serial_init ();

  holy_boot_init ();
}

void
holy_machine_fini (int flags __attribute__ ((unused)))
{
}

static int
halt_via (holy_pci_device_t dev, holy_pci_id_t pciid,
	  void *data __attribute__ ((unused)))
{
  holy_uint16_t pm;
  holy_pci_address_t addr;

  if (pciid != 0x30571106)
    return 0;

  addr = holy_pci_make_address (dev, 0x40);
  pm = holy_pci_read (addr) & ~1;

  if (pm == 0)
    {
      holy_pci_write (addr, 0x1801);
      pm = 0x1800;
    }

  addr = holy_pci_make_address (dev, 0x80);
  holy_pci_write_byte (addr, 0xff);

  addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
  holy_pci_write_word (addr, holy_pci_read_word (addr) | holy_PCI_COMMAND_IO_ENABLED);

  /* FIXME: This one is derived from qemu. Check on real hardware.  */
  holy_outw (0x2000, pm + 4 + holy_MACHINE_PCI_IO_BASE);
  holy_millisleep (5000);

  return 0;
}

void
holy_halt (void)
{
  switch (holy_arch_machine)
    {
    case holy_ARCH_MACHINE_FULOONG2E:
      holy_pci_iterate (halt_via, NULL);
      break;
    case holy_ARCH_MACHINE_FULOONG2F:
      {
	holy_pci_device_t dev;
	holy_port_t p;
	if (holy_cs5536_find (&dev))
	  {
	    p = (holy_cs5536_read_msr (dev, holy_CS5536_MSR_GPIO_BAR)
		 & holy_CS5536_LBAR_ADDR_MASK) + holy_MACHINE_PCI_IO_BASE;
	    holy_outl ((1 << 13), p + 4);
	    holy_outl ((1 << 29), p);
	    holy_millisleep (5000);
	  }
      }
      break;
    case holy_ARCH_MACHINE_YEELOONG:
      holy_outb (holy_inb (holy_CPU_LOONGSON_GPIOCFG)
		 & ~holy_CPU_YEELOONG_SHUTDOWN_GPIO, holy_CPU_LOONGSON_GPIOCFG);
      holy_millisleep (1500);
      break;
    case holy_ARCH_MACHINE_YEELOONG_3A:
      holy_millisleep (1);
      holy_outb (0x4e, holy_MACHINE_PCI_IO_BASE_3A | 0x66);
      holy_millisleep (1);
      holy_outb (2, holy_MACHINE_PCI_IO_BASE_3A | 0x62);
      holy_millisleep (5000);
      break;
    }

  holy_puts_ (N_("Shutdown failed"));
  holy_refresh ();
  while (1);
}

void
holy_exit (void)
{
  holy_halt ();
}

void
holy_machine_get_bootlocation (char **device __attribute__ ((unused)),
			       char **path __attribute__ ((unused)))
{
}

extern char _end[];
holy_addr_t holy_modbase = (holy_addr_t) _end;

