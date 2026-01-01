/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/cs5536.h>
#include <holy/pci.h>
#include <holy/time.h>
#include <holy/ata.h>
#ifdef holy_MACHINE_MIPS_LOONGSON
#include <holy/machine/kernel.h>
#endif

#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

/* Context for holy_cs5536_find.  */
struct holy_cs5536_find_ctx
{
  holy_pci_device_t *devp;
  int found;
};

/* Helper for holy_cs5536_find.  */
static int
holy_cs5536_find_iter (holy_pci_device_t dev, holy_pci_id_t pciid, void *data)
{
  struct holy_cs5536_find_ctx *ctx = data;

  if (pciid == holy_CS5536_PCIID)
    {
      *ctx->devp = dev;
      ctx->found = 1;
      return 1;
    }
  return 0;
}

int
holy_cs5536_find (holy_pci_device_t *devp)
{
  struct holy_cs5536_find_ctx ctx = {
    .devp = devp,
    .found = 0
  };

  holy_pci_iterate (holy_cs5536_find_iter, &ctx);

  return ctx.found;
}

holy_uint64_t
holy_cs5536_read_msr (holy_pci_device_t dev, holy_uint32_t addr)
{
  holy_uint64_t ret = 0;
  holy_pci_write (holy_pci_make_address (dev, holy_CS5536_MSR_MAILBOX_ADDR),
		  addr);
  ret = (holy_uint64_t)
    holy_pci_read (holy_pci_make_address (dev, holy_CS5536_MSR_MAILBOX_DATA0));
  ret |= (((holy_uint64_t)
	  holy_pci_read (holy_pci_make_address (dev,
						holy_CS5536_MSR_MAILBOX_DATA1)))
	  << 32);
  return ret;
}

void
holy_cs5536_write_msr (holy_pci_device_t dev, holy_uint32_t addr,
		       holy_uint64_t val)
{
  holy_pci_write (holy_pci_make_address (dev, holy_CS5536_MSR_MAILBOX_ADDR),
		  addr);
  holy_pci_write (holy_pci_make_address (dev, holy_CS5536_MSR_MAILBOX_DATA0),
		  val & 0xffffffff);
  holy_pci_write (holy_pci_make_address (dev, holy_CS5536_MSR_MAILBOX_DATA1),
		  val >> 32);
}

holy_err_t
holy_cs5536_smbus_wait (holy_port_t smbbase)
{
  holy_uint64_t start = holy_get_time_ms ();
  while (1)
    {
      holy_uint8_t status;
      status = holy_inb (smbbase + holy_CS5536_SMB_REG_STATUS);
      if (status & holy_CS5536_SMB_REG_STATUS_SDAST)
	return holy_ERR_NONE;
      if (status & holy_CS5536_SMB_REG_STATUS_BER)
	return holy_error (holy_ERR_IO, "SM bus error");
      if (status & holy_CS5536_SMB_REG_STATUS_NACK)
	return holy_error (holy_ERR_IO, "NACK received");
      if (holy_get_time_ms () > start + 40)
	return holy_error (holy_ERR_IO, "SM stalled");
    }
}

holy_err_t
holy_cs5536_read_spd_byte (holy_port_t smbbase, holy_uint8_t dev,
			   holy_uint8_t addr, holy_uint8_t *res)
{
  holy_err_t err;

  /* Send START.  */
  holy_outb (holy_inb (smbbase + holy_CS5536_SMB_REG_CTRL1)
	     | holy_CS5536_SMB_REG_CTRL1_START,
	     smbbase + holy_CS5536_SMB_REG_CTRL1);

  /* Send device address.  */
  err = holy_cs5536_smbus_wait (smbbase);
  if (err) 
    return err;
  holy_outb (dev << 1, smbbase + holy_CS5536_SMB_REG_DATA);

  /* Send ACK.  */
  err = holy_cs5536_smbus_wait (smbbase);
  if (err)
    return err;
  holy_outb (holy_inb (smbbase + holy_CS5536_SMB_REG_CTRL1)
	     | holy_CS5536_SMB_REG_CTRL1_ACK,
	     smbbase + holy_CS5536_SMB_REG_CTRL1);

  /* Send byte address.  */
  holy_outb (addr, smbbase + holy_CS5536_SMB_REG_DATA);

  /* Send START.  */
  err = holy_cs5536_smbus_wait (smbbase);
  if (err) 
    return err;
  holy_outb (holy_inb (smbbase + holy_CS5536_SMB_REG_CTRL1)
	     | holy_CS5536_SMB_REG_CTRL1_START,
	     smbbase + holy_CS5536_SMB_REG_CTRL1);

  /* Send device address.  */
  err = holy_cs5536_smbus_wait (smbbase);
  if (err)
    return err;
  holy_outb ((dev << 1) | 1, smbbase + holy_CS5536_SMB_REG_DATA);

  /* Send STOP.  */
  err = holy_cs5536_smbus_wait (smbbase);
  if (err)
    return err;
  holy_outb (holy_inb (smbbase + holy_CS5536_SMB_REG_CTRL1)
	     | holy_CS5536_SMB_REG_CTRL1_STOP,
	     smbbase + holy_CS5536_SMB_REG_CTRL1);

  err = holy_cs5536_smbus_wait (smbbase);
  if (err) 
    return err;
  *res = holy_inb (smbbase + holy_CS5536_SMB_REG_DATA);

  return holy_ERR_NONE;
}

holy_err_t
holy_cs5536_init_smbus (holy_pci_device_t dev, holy_uint16_t divisor,
			holy_port_t *smbbase)
{
  holy_uint64_t smbbar;

  smbbar = holy_cs5536_read_msr (dev, holy_CS5536_MSR_SMB_BAR);

  /* FIXME  */
  if (!(smbbar & holy_CS5536_LBAR_ENABLE))
    return holy_error(holy_ERR_IO, "SMB controller not enabled\n");
  *smbbase = (smbbar & holy_CS5536_LBAR_ADDR_MASK) + holy_MACHINE_PCI_IO_BASE;

  if (divisor < 8)
    return holy_error (holy_ERR_BAD_ARGUMENT, "invalid divisor");

  /* Disable SMB.  */
  holy_outb (0, *smbbase + holy_CS5536_SMB_REG_CTRL2);

  /* Disable interrupts.  */
  holy_outb (0, *smbbase + holy_CS5536_SMB_REG_CTRL1);

  /* Set as master.  */
  holy_outb (holy_CS5536_SMB_REG_ADDR_MASTER,
  	     *smbbase + holy_CS5536_SMB_REG_ADDR);

  /* Launch.  */
  holy_outb (((divisor >> 7) & 0xff), *smbbase + holy_CS5536_SMB_REG_CTRL3);
  holy_outb (((divisor << 1) & 0xfe) | holy_CS5536_SMB_REG_CTRL2_ENABLE,
	     *smbbase + holy_CS5536_SMB_REG_CTRL2);
  
  return holy_ERR_NONE;
}

holy_err_t
holy_cs5536_read_spd (holy_port_t smbbase, holy_uint8_t dev,
		      struct holy_smbus_spd *res)
{
  holy_err_t err;
  holy_size_t size;
  holy_uint8_t b;
  holy_size_t ptr;

  err = holy_cs5536_read_spd_byte (smbbase, dev, 0, &b);
  if (err)
    return err;
  if (b == 0)
    return holy_error (holy_ERR_IO, "no SPD found");
  size = b;
  
  ((holy_uint8_t *) res)[0] = b;
  for (ptr = 1; ptr < size; ptr++)
    {
      err = holy_cs5536_read_spd_byte (smbbase, dev, ptr,
				       &((holy_uint8_t *) res)[ptr]);
      if (err)
	return err;
    }
  return holy_ERR_NONE;
}

static inline void
set_io_space (holy_pci_device_t dev, int num, holy_uint16_t start,
	      holy_uint16_t len)
{
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_GL_REGIONS_START + num,
			 ((((holy_uint64_t) start + len - 4)
			   << holy_CS5536_MSR_GL_REGION_IO_TOP_SHIFT)
			  & holy_CS5536_MSR_GL_REGION_TOP_MASK)
			 | (((holy_uint64_t) start
			     << holy_CS5536_MSR_GL_REGION_IO_BASE_SHIFT)
			  & holy_CS5536_MSR_GL_REGION_BASE_MASK)
			 | holy_CS5536_MSR_GL_REGION_IO
			 | holy_CS5536_MSR_GL_REGION_ENABLE);
}

static inline void
set_iod (holy_pci_device_t dev, int num, int dest, int start, int mask)
{
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_GL_IOD_START + num,
			 ((holy_uint64_t) dest << holy_CS5536_IOD_DEST_SHIFT)
			 | (((holy_uint64_t) start & holy_CS5536_IOD_ADDR_MASK)
			    << holy_CS5536_IOD_BASE_SHIFT)
			 | ((mask & holy_CS5536_IOD_ADDR_MASK)
			    << holy_CS5536_IOD_MASK_SHIFT));
}

static inline void
set_p2d (holy_pci_device_t dev, int num, int dest, holy_uint32_t start)
{
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_GL_P2D_START + num,
			 (((holy_uint64_t) dest) << holy_CS5536_P2D_DEST_SHIFT)
			 | ((holy_uint64_t) (start >> holy_CS5536_P2D_LOG_ALIGN)
			    << holy_CS5536_P2D_BASE_SHIFT)
			 | (((1 << (32 - holy_CS5536_P2D_LOG_ALIGN)) - 1)
			    << holy_CS5536_P2D_MASK_SHIFT));
}

void
holy_cs5536_init_geode (holy_pci_device_t dev)
{
  /* Enable more BARs.  */
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_IRQ_MAP_BAR,
			 holy_CS5536_LBAR_TURN_ON | holy_CS5536_LBAR_IRQ_MAP);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_MFGPT_BAR,
			 holy_CS5536_LBAR_TURN_ON | holy_CS5536_LBAR_MFGPT);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_ACPI_BAR,
			 holy_CS5536_LBAR_TURN_ON | holy_CS5536_LBAR_ACPI);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_PM_BAR,
			 holy_CS5536_LBAR_TURN_ON | holy_CS5536_LBAR_PM);

  /* Setup DIVIL.  */
#ifdef holy_MACHINE_MIPS_LOONGSON
  switch (holy_arch_machine)
    {
    case holy_ARCH_MACHINE_YEELOONG:
      holy_cs5536_write_msr (dev, holy_CS5536_MSR_DIVIL_LEG_IO,
			     holy_CS5536_MSR_DIVIL_LEG_IO_MODE_X86
			     | holy_CS5536_MSR_DIVIL_LEG_IO_F_REMAP
			     | holy_CS5536_MSR_DIVIL_LEG_IO_RTC_ENABLE0
			     | holy_CS5536_MSR_DIVIL_LEG_IO_RTC_ENABLE1);
      break;
    case holy_ARCH_MACHINE_FULOONG2F:
      holy_cs5536_write_msr (dev, holy_CS5536_MSR_DIVIL_LEG_IO,
			     holy_CS5536_MSR_DIVIL_LEG_IO_UART2_COM3
			     | holy_CS5536_MSR_DIVIL_LEG_IO_UART1_COM1
			     | holy_CS5536_MSR_DIVIL_LEG_IO_MODE_X86
			     | holy_CS5536_MSR_DIVIL_LEG_IO_F_REMAP
			     | holy_CS5536_MSR_DIVIL_LEG_IO_RTC_ENABLE0
			     | holy_CS5536_MSR_DIVIL_LEG_IO_RTC_ENABLE1);
      break;
    }
#endif
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_DIVIL_IRQ_MAPPER_PRIMARY_MASK,
			 (~holy_CS5536_DIVIL_LPC_INTERRUPTS) & 0xffff);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_DIVIL_IRQ_MAPPER_LPC_MASK,
			 holy_CS5536_DIVIL_LPC_INTERRUPTS);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_DIVIL_LPC_SERIAL_IRQ_CONTROL,
			 holy_CS5536_MSR_DIVIL_LPC_SERIAL_IRQ_CONTROL_ENABLE);

  /* Initialise USB controller.  */
  /* FIXME: assign adresses dynamically.  */
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_USB_OHCI_BASE,
			 holy_CS5536_MSR_USB_BASE_BUS_MASTER
			 | holy_CS5536_MSR_USB_BASE_MEMORY_ENABLE
			 | 0x05024000);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_USB_EHCI_BASE,
			 holy_CS5536_MSR_USB_BASE_BUS_MASTER
			 | holy_CS5536_MSR_USB_BASE_MEMORY_ENABLE
			 | (0x20ULL << holy_CS5536_MSR_USB_EHCI_BASE_FLDJ_SHIFT)
			 | 0x05023000);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_USB_CONTROLLER_BASE,
			 holy_CS5536_MSR_USB_BASE_BUS_MASTER
			 | holy_CS5536_MSR_USB_BASE_MEMORY_ENABLE | 0x05020000);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_USB_OPTION_CONTROLLER_BASE,
			 holy_CS5536_MSR_USB_BASE_MEMORY_ENABLE | 0x05022000);
  set_p2d (dev, 0, holy_CS5536_DESTINATION_USB, 0x05020000);
  set_p2d (dev, 1, holy_CS5536_DESTINATION_USB, 0x05022000);
  set_p2d (dev, 5, holy_CS5536_DESTINATION_USB, 0x05024000);
  set_p2d (dev, 6, holy_CS5536_DESTINATION_USB, 0x05023000);

  {
    volatile holy_uint32_t *oc;
    oc = holy_pci_device_map_range (dev, 0x05022000,
				    holy_CS5536_USB_OPTION_REGS_SIZE);

    oc[holy_CS5536_USB_OPTION_REG_UOCMUX] =
      (oc[holy_CS5536_USB_OPTION_REG_UOCMUX]
       & ~holy_CS5536_USB_OPTION_REG_UOCMUX_PMUX_MASK)
      | holy_CS5536_USB_OPTION_REG_UOCMUX_PMUX_HC;
    holy_pci_device_unmap_range (dev, oc, holy_CS5536_USB_OPTION_REGS_SIZE);
  }

  /* Setup IDE controller.  */
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_IDE_IO_BAR,
			 holy_CS5536_LBAR_IDE
			 | holy_CS5536_MSR_IDE_IO_BAR_UNITS);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_IDE_CFG,
			 holy_CS5536_MSR_IDE_CFG_CHANNEL_ENABLE);
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_IDE_TIMING,
			 (holy_CS5536_MSR_IDE_TIMING_PIO0
			  << holy_CS5536_MSR_IDE_TIMING_DRIVE0_SHIFT)
			 | (holy_CS5536_MSR_IDE_TIMING_PIO0
			    << holy_CS5536_MSR_IDE_TIMING_DRIVE1_SHIFT));
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_IDE_CAS_TIMING,
			 (holy_CS5536_MSR_IDE_CAS_TIMING_CMD_PIO0
			  << holy_CS5536_MSR_IDE_CAS_TIMING_CMD_SHIFT)
			 | (holy_CS5536_MSR_IDE_CAS_TIMING_PIO0
			    << holy_CS5536_MSR_IDE_CAS_TIMING_DRIVE0_SHIFT)
			 | (holy_CS5536_MSR_IDE_CAS_TIMING_PIO0
			    << holy_CS5536_MSR_IDE_CAS_TIMING_DRIVE1_SHIFT));

  /* Setup Geodelink PCI.  */
  holy_cs5536_write_msr (dev, holy_CS5536_MSR_GL_PCI_CTRL,
			 (4ULL << holy_CS5536_MSR_GL_PCI_CTRL_OUT_THR_SHIFT)
			 | (4ULL << holy_CS5536_MSR_GL_PCI_CTRL_IN_THR_SHIFT)
			 | (8ULL << holy_CS5536_MSR_GL_PCI_CTRL_LATENCY_SHIFT)
			 | holy_CS5536_MSR_GL_PCI_CTRL_IO_ENABLE
			 | holy_CS5536_MSR_GL_PCI_CTRL_MEMORY_ENABLE);

  /* Setup windows.  */
  set_io_space (dev, 0, holy_CS5536_LBAR_SMBUS, holy_CS5536_SMBUS_REGS_SIZE);
  set_io_space (dev, 1, holy_CS5536_LBAR_GPIO, holy_CS5536_GPIO_REGS_SIZE);
  set_io_space (dev, 2, holy_CS5536_LBAR_MFGPT, holy_CS5536_MFGPT_REGS_SIZE);
  set_io_space (dev, 3, holy_CS5536_LBAR_IRQ_MAP, holy_CS5536_IRQ_MAP_REGS_SIZE);
  set_io_space (dev, 4, holy_CS5536_LBAR_PM, holy_CS5536_PM_REGS_SIZE);
  set_io_space (dev, 5, holy_CS5536_LBAR_ACPI, holy_CS5536_ACPI_REGS_SIZE);
  set_iod (dev, 0, holy_CS5536_DESTINATION_IDE, holy_ATA_CH0_PORT1, 0xffff8);
  set_iod (dev, 1, holy_CS5536_DESTINATION_ACC, holy_CS5536_LBAR_ACC, 0xfff80);
  set_iod (dev, 2, holy_CS5536_DESTINATION_IDE, holy_CS5536_LBAR_IDE, 0xffff0);
}
