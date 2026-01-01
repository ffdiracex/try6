/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/usb.h>
#include <holy/usbtrans.h>
#include <holy/misc.h>
#include <holy/pci.h>
#include <holy/cpu/pci.h>
#include <holy/cpu/io.h>
#include <holy/time.h>
#include <holy/cs5536.h>
#include <holy/loader.h>
#include <holy/disk.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_ohci_hcca
{
  /* Pointers to Interrupt Endpoint Descriptors.  Not used by
     holy.  */
  holy_uint32_t inttable[32];

  /* Current frame number.  */
  holy_uint16_t framenumber;

  holy_uint16_t pad;

  /* List of completed TDs.  */
  holy_uint32_t donehead;

  holy_uint8_t reserved[116];
} holy_PACKED;

/* OHCI General Transfer Descriptor */
struct holy_ohci_td
{
  /* Information used to construct the TOKEN packet.  */
  holy_uint32_t token;
  holy_uint32_t buffer; /* LittleEndian physical address */
  holy_uint32_t next_td; /* LittleEndian physical address */
  holy_uint32_t buffer_end; /* LittleEndian physical address */
  /* next values are not for OHCI HW */
  volatile struct holy_ohci_td *link_td; /* pointer to next free/chained TD
                          * pointer as uint32 */
  holy_uint32_t prev_td_phys; /* we need it to find previous TD
                               * physical address in CPU endian */
  holy_uint32_t tr_index; /* index of TD in transfer */
  holy_uint8_t pad[8 - sizeof (volatile struct holy_ohci_td *)]; /* padding to 32 bytes */
} holy_PACKED;

/* OHCI Endpoint Descriptor.  */
struct holy_ohci_ed
{
  holy_uint32_t target;
  holy_uint32_t td_tail;
  holy_uint32_t td_head;
  holy_uint32_t next_ed;
} holy_PACKED;

typedef volatile struct holy_ohci_td *holy_ohci_td_t;
typedef volatile struct holy_ohci_ed *holy_ohci_ed_t;

/* Experimental change of ED/TD allocation */
/* Little bit similar as in UHCI */
/* Implementation assumes:
 *      32-bits architecture - XXX: fix for 64-bits
 *      memory allocated by holy_memalign_dma32 must be continuous
 *      in virtual and also in physical memory */
struct holy_ohci
{
  volatile holy_uint32_t *iobase;
  volatile struct holy_ohci_hcca *hcca;
  holy_uint32_t hcca_addr;
  struct holy_pci_dma_chunk *hcca_chunk;
  holy_ohci_ed_t ed_ctrl; /* EDs for CONTROL */
  holy_uint32_t ed_ctrl_addr;
  struct holy_pci_dma_chunk *ed_ctrl_chunk;
  holy_ohci_ed_t ed_bulk; /* EDs for BULK */
  holy_uint32_t ed_bulk_addr;
  struct holy_pci_dma_chunk *ed_bulk_chunk;
  holy_ohci_td_t td; /* TDs */
  holy_uint32_t td_addr;
  struct holy_pci_dma_chunk *td_chunk;
  struct holy_ohci *next;
  holy_ohci_td_t td_free; /* Pointer to first free TD */
};

static struct holy_ohci *ohci;

typedef enum
{
  holy_OHCI_REG_REVISION = 0x00,
  holy_OHCI_REG_CONTROL,
  holy_OHCI_REG_CMDSTATUS,
  holy_OHCI_REG_INTSTATUS,
  holy_OHCI_REG_INTENA,
  holy_OHCI_REG_INTDIS,
  holy_OHCI_REG_HCCA,
  holy_OHCI_REG_PERIODIC,
  holy_OHCI_REG_CONTROLHEAD,
  holy_OHCI_REG_CONTROLCURR,
  holy_OHCI_REG_BULKHEAD,
  holy_OHCI_REG_BULKCURR,
  holy_OHCI_REG_DONEHEAD,
  holy_OHCI_REG_FRAME_INTERVAL,
  holy_OHCI_REG_PERIODIC_START = 16,
  holy_OHCI_REG_RHUBA = 18,
  holy_OHCI_REG_RHUBPORT = 21,
  holy_OHCI_REG_LEGACY_CONTROL = 0x100,
  holy_OHCI_REG_LEGACY_INPUT = 0x104,
  holy_OHCI_REG_LEGACY_OUTPUT = 0x108,
  holy_OHCI_REG_LEGACY_STATUS = 0x10c
} holy_ohci_reg_t;

#define holy_OHCI_RHUB_PORT_POWER_MASK 0x300
#define holy_OHCI_RHUB_PORT_ALL_POWERED 0x200

#define holy_OHCI_REG_FRAME_INTERVAL_FSMPS_MASK 0x8fff0000
#define holy_OHCI_REG_FRAME_INTERVAL_FSMPS_SHIFT 16
#define holy_OHCI_REG_FRAME_INTERVAL_FI_SHIFT 0

/* XXX: Is this choice of timings sane?  */
#define holy_OHCI_FSMPS 0x2778
#define holy_OHCI_PERIODIC_START 0x257f
#define holy_OHCI_FRAME_INTERVAL 0x2edf

#define holy_OHCI_SET_PORT_ENABLE (1 << 1)
#define holy_OHCI_CLEAR_PORT_ENABLE (1 << 0)
#define holy_OHCI_SET_PORT_RESET (1 << 4)
#define holy_OHCI_SET_PORT_RESET_STATUS_CHANGE (1 << 20)

#define holy_OHCI_REG_CONTROL_BULK_ENABLE (1 << 5)
#define holy_OHCI_REG_CONTROL_CONTROL_ENABLE (1 << 4)

#define holy_OHCI_RESET_CONNECT_CHANGE (1 << 16)
#define holy_OHCI_CTRL_EDS 256
#define holy_OHCI_BULK_EDS 510
#define holy_OHCI_TDS 640

#define holy_OHCI_ED_ADDR_MASK 0x7ff

static inline holy_ohci_ed_t
holy_ohci_ed_phys2virt (struct holy_ohci *o, int bulk, holy_uint32_t x)
{
  if (!x)
    return NULL;
  if (bulk)
    return (holy_ohci_ed_t) (x - o->ed_bulk_addr
			     + (holy_uint8_t *) o->ed_bulk);
  return (holy_ohci_ed_t) (x - o->ed_ctrl_addr
			   + (holy_uint8_t *) o->ed_ctrl);
}

static holy_uint32_t
holy_ohci_virt_to_phys (struct holy_ohci *o, int bulk, holy_ohci_ed_t x)
{
  if (!x)
    return 0;

  if (bulk)
    return (holy_uint8_t *) x - (holy_uint8_t *) o->ed_bulk + o->ed_bulk_addr;
  return (holy_uint8_t *) x - (holy_uint8_t *) o->ed_ctrl + o->ed_ctrl_addr;
}

static inline holy_ohci_td_t
holy_ohci_td_phys2virt (struct holy_ohci *o, holy_uint32_t x)
{
  if (!x)
    return NULL;
  return (holy_ohci_td_t) (x - o->td_addr + (holy_uint8_t *) o->td);
}

static holy_uint32_t
holy_ohci_td_virt2phys (struct holy_ohci *o,  holy_ohci_td_t x)
{
  if (!x)
    return 0;
  return (holy_uint8_t *)x - (holy_uint8_t *)o->td + o->td_addr;
}

  
static holy_uint32_t
holy_ohci_readreg32 (struct holy_ohci *o, holy_ohci_reg_t reg)
{
  return holy_le_to_cpu32 (*(o->iobase + reg));
}

static void
holy_ohci_writereg32 (struct holy_ohci *o,
		      holy_ohci_reg_t reg, holy_uint32_t val)
{
  *(o->iobase + reg) = holy_cpu_to_le32 (val);
}



/* Iterate over all PCI devices.  Determine if a device is an OHCI
   controller.  If this is the case, initialize it.  */
static int
holy_ohci_pci_iter (holy_pci_device_t dev, holy_pci_id_t pciid,
		    void *data __attribute__ ((unused)))
{
  holy_uint32_t interf;
  holy_uint32_t base;
  holy_pci_address_t addr;
  struct holy_ohci *o;
  holy_uint32_t revision;
  int j;
  
  /* Determine IO base address.  */
  holy_dprintf ("ohci", "pciid = %x\n", pciid);

  if (pciid == holy_CS5536_PCIID)
    {
      holy_uint64_t basereg;

      basereg = holy_cs5536_read_msr (dev, holy_CS5536_MSR_USB_OHCI_BASE);
      if (!(basereg & holy_CS5536_MSR_USB_BASE_MEMORY_ENABLE))
	{
	  /* Shouldn't happen.  */
	  holy_dprintf ("ohci", "No OHCI address is assigned\n");
	  return 0;
	}
      base = (basereg & holy_CS5536_MSR_USB_BASE_ADDR_MASK);
      basereg |= holy_CS5536_MSR_USB_BASE_BUS_MASTER;
      basereg &= ~holy_CS5536_MSR_USB_BASE_PME_ENABLED;
      basereg &= ~holy_CS5536_MSR_USB_BASE_PME_STATUS;
      holy_cs5536_write_msr (dev, holy_CS5536_MSR_USB_OHCI_BASE, basereg);
    }
  else
    {
      holy_uint32_t class_code;
      holy_uint32_t class;
      holy_uint32_t subclass;

      addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
      class_code = holy_pci_read (addr) >> 8;
      
      interf = class_code & 0xFF;
      subclass = (class_code >> 8) & 0xFF;
      class = class_code >> 16;

      /* If this is not an OHCI controller, just return.  */
      if (class != 0x0c || subclass != 0x03 || interf != 0x10)
	return 0;

      addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0);
      base = holy_pci_read (addr);

      base &= holy_PCI_ADDR_MEM_MASK;
      if (!base)
	{
	  holy_dprintf ("ehci",
			"EHCI: EHCI is not mapper\n");
	  return 0;
	}

      /* Set bus master - needed for coreboot, VMware, broken BIOSes etc. */
      addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
      holy_pci_write_word(addr,
			  holy_PCI_COMMAND_MEM_ENABLED
			  | holy_PCI_COMMAND_BUS_MASTER
			  | holy_pci_read_word(addr));

      holy_dprintf ("ohci", "class=0x%02x 0x%02x interface 0x%02x\n",
		    class, subclass, interf);
    }

  /* Allocate memory for the controller and register it.  */
  o = holy_malloc (sizeof (*o));
  if (! o)
    return 1;
  holy_memset ((void*)o, 0, sizeof (*o));
  o->iobase = holy_pci_device_map_range (dev, base, 0x800);

  holy_dprintf ("ohci", "base=%p\n", o->iobase);

  /* Reserve memory for the HCCA.  */
  o->hcca_chunk = holy_memalign_dma32 (256, 256);
  if (! o->hcca_chunk)
    goto fail;
  o->hcca = holy_dma_get_virt (o->hcca_chunk);
  o->hcca_addr = holy_dma_get_phys (o->hcca_chunk);
  holy_memset ((void*)o->hcca, 0, sizeof(*o->hcca));
  holy_dprintf ("ohci", "hcca: chunk=%p, virt=%p, phys=0x%02x\n",
                o->hcca_chunk, o->hcca, o->hcca_addr);

  /* Reserve memory for ctrl EDs.  */
  o->ed_ctrl_chunk = holy_memalign_dma32 (16, sizeof(struct holy_ohci_ed)
					  * holy_OHCI_CTRL_EDS);
  if (! o->ed_ctrl_chunk)
    goto fail;
  o->ed_ctrl = holy_dma_get_virt (o->ed_ctrl_chunk);
  o->ed_ctrl_addr = holy_dma_get_phys (o->ed_ctrl_chunk);
  /* Preset EDs */
  holy_memset ((void *) o->ed_ctrl, 0, sizeof (struct holy_ohci_ed)
	       * holy_OHCI_CTRL_EDS);
  for (j=0; j < holy_OHCI_CTRL_EDS; j++)
    o->ed_ctrl[j].target = holy_cpu_to_le32_compile_time (1 << 14); /* skip */
    
  holy_dprintf ("ohci", "EDs-C: chunk=%p, virt=%p, phys=0x%02x\n",
                o->ed_ctrl_chunk, o->ed_ctrl, o->ed_ctrl_addr);

  /* Reserve memory for bulk EDs.  */
  o->ed_bulk_chunk = holy_memalign_dma32 (16, sizeof (struct holy_ohci_ed)
					  * holy_OHCI_BULK_EDS);
  if (! o->ed_bulk_chunk)
    goto fail;
  o->ed_bulk = holy_dma_get_virt (o->ed_bulk_chunk);
  o->ed_bulk_addr = holy_dma_get_phys (o->ed_bulk_chunk);
  /* Preset EDs */
  holy_memset ((void*)o->ed_bulk, 0, sizeof(struct holy_ohci_ed) * holy_OHCI_BULK_EDS);
  for (j=0; j < holy_OHCI_BULK_EDS; j++)
    o->ed_bulk[j].target = holy_cpu_to_le32_compile_time (1 << 14); /* skip */

  holy_dprintf ("ohci", "EDs-B: chunk=%p, virt=%p, phys=0x%02x\n",
                o->ed_bulk_chunk, o->ed_bulk, o->ed_bulk_addr);

  /* Reserve memory for TDs.  */
  o->td_chunk = holy_memalign_dma32 (32, sizeof(struct holy_ohci_td)*holy_OHCI_TDS);
  /* Why is it aligned on 32 boundary if spec. says 16 ?
   * We have structure 32 bytes long and we don't want cross
   * 4K boundary inside structure. */
  if (! o->td_chunk)
    goto fail;
  o->td_free = o->td = holy_dma_get_virt (o->td_chunk);
  o->td_addr = holy_dma_get_phys (o->td_chunk);
  /* Preset free TDs chain in TDs */
  holy_memset ((void*)o->td, 0, sizeof(struct holy_ohci_td) * holy_OHCI_TDS);
  for (j=0; j < (holy_OHCI_TDS-1); j++)
    o->td[j].link_td = &o->td[j+1];

  holy_dprintf ("ohci", "TDs: chunk=%p, virt=%p, phys=0x%02x\n",
                o->td_chunk, o->td, o->td_addr);

  /* Check if the OHCI revision is actually 1.0 as supported.  */
  revision = holy_ohci_readreg32 (o, holy_OHCI_REG_REVISION);
  holy_dprintf ("ohci", "OHCI revision=0x%02x\n", revision & 0xFF);
  if ((revision & 0xFF) != 0x10)
    goto fail;

  {
    holy_uint32_t control;
    /* Check SMM/BIOS ownership of OHCI (SMM = USB Legacy Support driver for BIOS) */
    control = holy_ohci_readreg32 (o, holy_OHCI_REG_CONTROL);
    if ((control & 0x100) != 0)
      {
	unsigned i;
	holy_dprintf("ohci", "OHCI is owned by SMM\n");
	/* Do change of ownership */
	/* Ownership change request */
	holy_ohci_writereg32 (o, holy_OHCI_REG_CMDSTATUS, (1<<3)); /* XXX: Magic.  */
	/* Waiting for SMM deactivation */
	for (i=0; i < 10; i++)
	  {
	    if ((holy_ohci_readreg32 (o, holy_OHCI_REG_CONTROL) & 0x100) == 0)
	      {
		holy_dprintf("ohci", "Ownership changed normally.\n");
		break;
	      }
	    holy_millisleep (100);
          }
	if (i >= 10)
	  {
	    holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROL,
				  holy_ohci_readreg32 (o, holy_OHCI_REG_CONTROL) & ~0x100);
	    holy_dprintf("ohci", "Ownership changing timeout, change forced !\n");
	  }
      }
    else if (((control & 0x100) == 0) && 
	     ((control & 0xc0) != 0)) /* Not owned by SMM nor reset */
      {
	holy_dprintf("ohci", "OHCI is owned by BIOS\n");
	/* Do change of ownership - not implemented yet... */
	/* In fact we probably need to do nothing ...? */
      }
    else
      {
	holy_dprintf("ohci", "OHCI is not owned by SMM nor BIOS\n");
	/* We can setup OHCI. */
      }  
  }

  /* Suspend the OHCI by issuing a reset.  */
  holy_ohci_writereg32 (o, holy_OHCI_REG_CMDSTATUS, 1); /* XXX: Magic.  */
  holy_millisleep (1);
  holy_dprintf ("ohci", "OHCI reset\n");

  holy_ohci_writereg32 (o, holy_OHCI_REG_FRAME_INTERVAL,
			(holy_OHCI_FSMPS
			 << holy_OHCI_REG_FRAME_INTERVAL_FSMPS_SHIFT)
			| (holy_OHCI_FRAME_INTERVAL
			   << holy_OHCI_REG_FRAME_INTERVAL_FI_SHIFT));

  holy_ohci_writereg32 (o, holy_OHCI_REG_PERIODIC_START,
			holy_OHCI_PERIODIC_START);

  /* Setup the HCCA.  */
  o->hcca->donehead = 0;
  holy_ohci_writereg32 (o, holy_OHCI_REG_HCCA, o->hcca_addr);
  holy_dprintf ("ohci", "OHCI HCCA\n");

  /* Misc. pre-sets. */
  o->hcca->donehead = 0;
  holy_ohci_writereg32 (o, holy_OHCI_REG_INTSTATUS, 0x7f); /* Clears everything */
  /* We don't want modify CONTROL/BULK HEAD registers.
   * So we assign to HEAD registers zero ED from related array
   * and we will not use this ED, it will be always skipped.
   * It should not produce notable performance penalty (I hope). */
  holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROLHEAD, o->ed_ctrl_addr);
  holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROLCURR, 0);
  holy_ohci_writereg32 (o, holy_OHCI_REG_BULKHEAD, o->ed_bulk_addr);
  holy_ohci_writereg32 (o, holy_OHCI_REG_BULKCURR, 0);

  /* Check OHCI Legacy Support */
  if ((revision & 0x100) != 0)
    {
      holy_dprintf ("ohci", "Legacy Support registers detected\n");
      holy_dprintf ("ohci", "Current state of legacy control reg.: 0x%04x\n",
		    holy_ohci_readreg32 (o, holy_OHCI_REG_LEGACY_CONTROL));
      holy_ohci_writereg32 (o, holy_OHCI_REG_LEGACY_CONTROL,
			    (holy_ohci_readreg32 (o, holy_OHCI_REG_LEGACY_CONTROL)) & ~1);
      holy_dprintf ("ohci", "OHCI Legacy Support disabled.\n");
    }

  /* Enable the OHCI + enable CONTROL and BULK LIST.  */
  holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROL,
			(2 << 6)
			| holy_OHCI_REG_CONTROL_CONTROL_ENABLE
			| holy_OHCI_REG_CONTROL_BULK_ENABLE );
  holy_dprintf ("ohci", "OHCI enable: 0x%02x\n",
		(holy_ohci_readreg32 (o, holy_OHCI_REG_CONTROL) >> 6) & 3);

  /* Power on all ports */
  holy_ohci_writereg32 (o, holy_OHCI_REG_RHUBA,
                       (holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBA)
                        & ~holy_OHCI_RHUB_PORT_POWER_MASK)
                       | holy_OHCI_RHUB_PORT_ALL_POWERED);
#if 0 /* We don't need it at all, handled via hotplugging */
  /* Now we have hot-plugging, we need to wait for stable power only */
  holy_millisleep (100);
#endif

  /* Link to ohci now that initialisation is successful.  */
  o->next = ohci;
  ohci = o;

  return 0;

 fail:
  if (o)
    {
      holy_dma_free (o->td_chunk);
      holy_dma_free (o->ed_bulk_chunk);
      holy_dma_free (o->ed_ctrl_chunk);
      holy_dma_free (o->hcca_chunk);
    }
  holy_free (o);

  return 0;
}


static void
holy_ohci_inithw (void)
{
  holy_pci_iterate (holy_ohci_pci_iter, NULL);
}



static int
holy_ohci_iterate (holy_usb_controller_iterate_hook_t hook, void *hook_data)
{
  struct holy_ohci *o;
  struct holy_usb_controller dev;

  for (o = ohci; o; o = o->next)
    {
      dev.data = o;
      if (hook (&dev, hook_data))
	return 1;
    }

  return 0;
}

static holy_ohci_ed_t
holy_ohci_find_ed (struct holy_ohci *o, int bulk, holy_uint32_t target)
{
  holy_ohci_ed_t ed, ed_next;
  holy_uint32_t target_addr = target & holy_OHCI_ED_ADDR_MASK;
  int count;
  int i;

  /* Use proper values and structures. */
  if (bulk)
    {    
      count = holy_OHCI_BULK_EDS;
      ed = o->ed_bulk;
      ed_next = holy_ohci_ed_phys2virt(o, bulk,
                  holy_le_to_cpu32 (ed->next_ed) );
    }
  else
    {
      count = holy_OHCI_CTRL_EDS;
      ed = o->ed_ctrl;
      ed_next = holy_ohci_ed_phys2virt(o, bulk,
                  holy_le_to_cpu32 (ed->next_ed) );
    }

   /* First try to find existing ED with proper target address */
  for (i = 0; ; )
    {
      if (i && /* We ignore zero ED */
           ((ed->target & holy_OHCI_ED_ADDR_MASK) == target_addr))
        return ed; /* Found proper existing ED */
      i++;
      if (ed_next && (i < count))
        {
          ed = ed_next;
          ed_next = holy_ohci_ed_phys2virt(o, bulk,
                      holy_le_to_cpu32 (ed->next_ed) );
          continue;
        }
      break;
    }
  /* ED with target_addr does not exist, we have to add it */
  /* Have we any free ED in array ? */
  if (i >= count) /* No. */
    return NULL;
  /* Currently we simply take next ED in array, no allocation
   * function is used. It should be no problem until hot-plugging
   * will be implemented, i.e. until we will need to de-allocate EDs
   * of unplugged devices. */
  /* We can link new ED to previous ED safely as the new ED should
   * still have set skip bit. */
  ed->next_ed = holy_cpu_to_le32 ( holy_ohci_virt_to_phys (o,
                                     bulk, &ed[1]));
  return &ed[1];
}

static holy_ohci_td_t
holy_ohci_alloc_td (struct holy_ohci *o)
{
  holy_ohci_td_t ret;

  /* Check if there is a Transfer Descriptor available.  */
  if (! o->td_free)
    return NULL;

  ret = o->td_free; /* Take current free TD */
  o->td_free = (holy_ohci_td_t)ret->link_td; /* Advance to next free TD in chain */
  ret->link_td = 0; /* Reset link_td in allocated TD */
  return ret;
}

static void
holy_ohci_free_td (struct holy_ohci *o, holy_ohci_td_t td)
{
  holy_memset ( (void*)td, 0, sizeof(struct holy_ohci_td) );
  td->link_td = o->td_free; /* Cahin new free TD & rest */
  o->td_free = td; /* Change address of first free TD */
}

static void
holy_ohci_free_tds (struct holy_ohci *o, holy_ohci_td_t td)
{
  if (!td)
    return;
    
  /* Unchain first TD from previous TD if it is chained */
  if (td->prev_td_phys)
    {
      holy_ohci_td_t td_prev_virt = holy_ohci_td_phys2virt(o,
                                      td->prev_td_phys);

      if (td == (holy_ohci_td_t) td_prev_virt->link_td)
        td_prev_virt->link_td = 0;
    }
  
  /* Free all TDs from td  (chained by link_td) */
  while (td)
    {
      holy_ohci_td_t tdprev;
      
      /* Unlink the queue.  */
      tdprev = td;
      td = (holy_ohci_td_t) td->link_td;

      /* Free the TD.  */
      holy_ohci_free_td (o, tdprev);
    }
}

static void
holy_ohci_transaction (holy_ohci_td_t td,
		       holy_transfer_type_t type, unsigned int toggle,
		       holy_size_t size, holy_uint32_t data)
{
  holy_uint32_t token;
  holy_uint32_t buffer;
  holy_uint32_t buffer_end;

  holy_dprintf ("ohci", "OHCI transaction td=%p type=%d, toggle=%d, size=%lu\n",
		td, type, toggle, (unsigned long) size);

  switch (type)
    {
    case holy_USB_TRANSFER_TYPE_SETUP:
      token = 0 << 19;
      break;
    case holy_USB_TRANSFER_TYPE_IN:
      token = 2 << 19;
      break;
    case holy_USB_TRANSFER_TYPE_OUT:
      token = 1 << 19;
      break;
    default:
      token = 0;
      break;
    }

  /* Set the token */
  token |= ( 7 << 21); /* Never generate interrupt */
  token |= toggle << 24;
  token |= 1 << 25;

  /* Set "Not accessed" error code */
  token |= 15 << 28;

  buffer = data;
  buffer_end = buffer + size - 1;

  /* Set correct buffer values in TD if zero transfer occurs */
  if (size)
    {
      buffer = (holy_uint32_t) data;
      buffer_end = buffer + size - 1;
      td->buffer = holy_cpu_to_le32 (buffer);
      td->buffer_end = holy_cpu_to_le32 (buffer_end);
    }
  else 
    {
      td->buffer = 0;
      td->buffer_end = 0;
    }

  /* Set the rest of TD */
  td->token = holy_cpu_to_le32 (token);
  td->next_td = 0;
}

struct holy_ohci_transfer_controller_data
{
  holy_uint32_t tderr_phys;
  holy_uint32_t td_last_phys;
  holy_ohci_ed_t ed_virt;
  holy_ohci_td_t td_current_virt;
  holy_ohci_td_t td_head_virt;
};

static holy_usb_err_t
holy_ohci_setup_transfer (holy_usb_controller_t dev,
			  holy_usb_transfer_t transfer)
{
  struct holy_ohci *o = (struct holy_ohci *) dev->data;
  int bulk = 0;
  holy_ohci_td_t td_next_virt;
  holy_uint32_t target;
  holy_uint32_t td_head_phys;
  holy_uint32_t td_tail_phys;
  int i;
  struct holy_ohci_transfer_controller_data *cdata;

  cdata = holy_zalloc (sizeof (*cdata));
  if (!cdata)
    return holy_USB_ERR_INTERNAL;

  /* Pre-set target for ED - we need it to find proper ED */
  /* Set the device address.  */
  target = transfer->devaddr;
  /* Set the endpoint. It should be masked, we need 4 bits only. */
  target |= (transfer->endpoint & 15) << 7;
  /* Set the device speed.  */
  target |= (transfer->dev->speed == holy_USB_SPEED_LOW) << 13;
  /* Set the maximum packet size.  */
  target |= transfer->max << 16;

  /* Determine if transfer type is bulk - we need to select proper ED */
  switch (transfer->type)
    {
      case holy_USB_TRANSACTION_TYPE_BULK:
        bulk = 1;
	break;

      case holy_USB_TRANSACTION_TYPE_CONTROL:
        break;

      default:
	holy_free (cdata);
        return holy_USB_ERR_INTERNAL;
    }

  /* Find proper ED or add new ED */
  cdata->ed_virt = holy_ohci_find_ed (o, bulk, target);
  if (!cdata->ed_virt)
    {
      holy_dprintf ("ohci","Fatal: No free ED !\n");
      holy_free (cdata);
      return holy_USB_ERR_INTERNAL;
    }
  
  /* Take pointer to first TD from ED */
  td_head_phys = holy_le_to_cpu32 (cdata->ed_virt->td_head) & ~0xf;
  td_tail_phys = holy_le_to_cpu32 (cdata->ed_virt->td_tail) & ~0xf;

  /* Sanity check - td_head should be equal to td_tail */
  if (td_head_phys != td_tail_phys) /* Should never happen ! */
    {
      holy_dprintf ("ohci", "Fatal: HEAD is not equal to TAIL !\n");
      holy_dprintf ("ohci", "HEAD = 0x%02x, TAIL = 0x%02x\n",
                    td_head_phys, td_tail_phys);
      /* XXX: Fix: What to do ? */
      holy_free (cdata);
      return holy_USB_ERR_INTERNAL;
    }
  
  /* Now we should handle first TD. If ED is newly allocated,
   * we must allocate the first TD. */
  if (!td_head_phys)
    {
      cdata->td_head_virt = holy_ohci_alloc_td (o);
      if (!cdata->td_head_virt)
	{
	  holy_free (cdata);
	  return holy_USB_ERR_INTERNAL; /* We don't need de-allocate ED */
	}
      /* We can set td_head only when ED is not active, i.e.
       * when it is newly allocated. */
      cdata->ed_virt->td_head
	= holy_cpu_to_le32 (holy_ohci_td_virt2phys (o, cdata->td_head_virt));
      cdata->ed_virt->td_tail = cdata->ed_virt->td_head;
    }
  else
    cdata->td_head_virt = holy_ohci_td_phys2virt ( o, td_head_phys );
    
  /* Set TDs */
  cdata->td_last_phys = td_head_phys; /* initial value to make compiler happy... */
  for (i = 0, cdata->td_current_virt = cdata->td_head_virt;
       i < transfer->transcnt; i++)
    {
      holy_usb_transaction_t tr = &transfer->transactions[i];

      holy_ohci_transaction (cdata->td_current_virt, tr->pid, tr->toggle,
			     tr->size, tr->data);

      /* Set index of TD in transfer */
      cdata->td_current_virt->tr_index = (holy_uint32_t) i;
      
      /* Remember last used (processed) TD phys. addr. */
      cdata->td_last_phys = holy_ohci_td_virt2phys (o, cdata->td_current_virt);
      
      /* Allocate next TD */
      td_next_virt = holy_ohci_alloc_td (o);
      if (!td_next_virt) /* No free TD, cancel transfer and free TDs except head TD */
        {
          if (i) /* if i==0 we have nothing to free... */
            holy_ohci_free_tds (o, holy_ohci_td_phys2virt(o,
							  holy_le_to_cpu32 (cdata->td_head_virt->next_td)));
          /* Reset head TD */
          holy_memset ( (void*)cdata->td_head_virt, 0,
                        sizeof(struct holy_ohci_td) );
          holy_dprintf ("ohci", "Fatal: No free TD !");
	  holy_free (cdata);
          return holy_USB_ERR_INTERNAL;
        }

      /* Chain TDs */

      cdata->td_current_virt->link_td = td_next_virt;
      cdata->td_current_virt->next_td = holy_cpu_to_le32 (
                                   holy_ohci_td_virt2phys (o,
                                     td_next_virt) );
      td_next_virt->prev_td_phys = holy_ohci_td_virt2phys (o,
                                cdata->td_current_virt);
      cdata->td_current_virt = td_next_virt;
    }

  holy_dprintf ("ohci", "Tail TD (not processed) = %p\n",
                cdata->td_current_virt);
  
  /* Setup the Endpoint Descriptor for transfer.  */
  /* First set necessary fields in TARGET but keep (or set) skip bit */
  /* Note: It could be simpler if speed, format and max. packet
   * size never change after first allocation of ED.
   * But unfortunately max. packet size may change during initial
   * setup sequence and we must handle it. */
  cdata->ed_virt->target = holy_cpu_to_le32 (target | (1 << 14));
  /* Set td_tail */
  cdata->ed_virt->td_tail
    = holy_cpu_to_le32 (holy_ohci_td_virt2phys (o, cdata->td_current_virt));
  /* Now reset skip bit */
  cdata->ed_virt->target = holy_cpu_to_le32 (target);
  /* ed_virt->td_head = holy_cpu_to_le32 (td_head); Must not be changed, it is maintained by OHCI */
  /* ed_virt->next_ed = holy_cpu_to_le32 (0); Handled by holy_ohci_find_ed, do not change ! */

  holy_dprintf ("ohci", "program OHCI\n");

  /* Program the OHCI to actually transfer.  */
  switch (transfer->type)
    {
    case holy_USB_TRANSACTION_TYPE_BULK:
      {
	holy_dprintf ("ohci", "BULK list filled\n");
	/* Set BulkListFilled.  */
	holy_ohci_writereg32 (o, holy_OHCI_REG_CMDSTATUS, 1 << 2);
	/* Read back of register should ensure it is really written */
	holy_ohci_readreg32 (o, holy_OHCI_REG_CMDSTATUS);
	break;
      }

    case holy_USB_TRANSACTION_TYPE_CONTROL:
      {
	holy_dprintf ("ohci", "CONTROL list filled\n");
	/* Set ControlListFilled.  */
	holy_ohci_writereg32 (o, holy_OHCI_REG_CMDSTATUS, 1 << 1);
	/* Read back of register should ensure it is really written */
	holy_ohci_readreg32 (o, holy_OHCI_REG_CMDSTATUS);
	break;
      }
    }

  transfer->controller_data = cdata;

  return holy_USB_ERR_NONE;
}

static void
pre_finish_transfer (holy_usb_controller_t dev,
		     holy_usb_transfer_t transfer)
{
  struct holy_ohci *o = dev->data;
  struct holy_ohci_transfer_controller_data *cdata = transfer->controller_data;
  holy_uint32_t target;
  holy_uint32_t status;
  holy_uint32_t control;
  holy_uint32_t intstatus;

  /* There are many ways how the loop above can finish:
   * - normally without any error via INTSTATUS WDH bit
   *   : tderr_phys == td_last_phys, td_head == td_tail
   * - normally with error via HALT bit in ED TD HEAD
   *   : td_head = next TD after TD with error
   *   : tderr_phys = last processed and retired TD with error,
   *     i.e. should be != 0
   *   : if bad_OHCI == TRUE, tderr_phys will be probably invalid
   * - unrecoverable error - I never seen it but it could be
   *   : err_unrec == TRUE, other values can contain anything...
   * - timeout, it can be caused by:
   *  -- bad USB device - some devices have some bugs, see Linux source
   *     and related links
   *  -- bad OHCI controller - e.g. lost interrupts or does not set
   *     proper bits in INTSTATUS when real IRQ not enabled etc.,
   *     see Linux source and related links
   *     One known bug is handled - if transfer finished
   *     successfully (i.e. HEAD==TAIL, last transfer TD is retired,
   *     HALT bit is not set) and WDH bit is not set in INTSTATUS - in
   *     this case we set o->bad_OHCI=TRUE and do alternate loop
   *     and error handling - but there is problem how to find retired
   *     TD with error code if HALT occurs and if DONEHEAD is not
   *     working - we need to find TD previous to current ED HEAD
   *  -- bad code of this driver or some unknown reasons - :-(
   *     it can be e.g. bad handling of EDs/TDs/toggle bit...
   */

  /* Remember target for debug and set skip flag in ED */
  /* It should be normaly not necessary but we need it at least
   * in case of timeout */
  target = holy_le_to_cpu32 ( cdata->ed_virt->target );
  cdata->ed_virt->target = holy_cpu_to_le32 (target | (1 << 14));
  /* Read registers for debug - they should be read now because
   * debug prints case unwanted delays, so something can happen
   * in the meantime... */
  control = holy_ohci_readreg32 (o, holy_OHCI_REG_CONTROL);
  status = holy_ohci_readreg32 (o, holy_OHCI_REG_CMDSTATUS);
  intstatus = holy_ohci_readreg32 (o, holy_OHCI_REG_INTSTATUS);
  /* Now print debug values - to have full info what happened */
  holy_dprintf ("ohci", "loop finished: control=0x%02x status=0x%02x\n",
		control, status);
  holy_dprintf ("ohci", "intstatus=0x%02x, td_last_phys=0x%02x\n",
		intstatus, cdata->td_last_phys);
  holy_dprintf ("ohci", "TARGET=0x%02x, HEAD=0x%02x, TAIL=0x%02x\n",
                target,
                holy_le_to_cpu32 (cdata->ed_virt->td_head),
                holy_le_to_cpu32 (cdata->ed_virt->td_tail) );

}

static void
finish_transfer (holy_usb_controller_t dev,
		 holy_usb_transfer_t transfer)
{
  struct holy_ohci *o = dev->data;
  struct holy_ohci_transfer_controller_data *cdata = transfer->controller_data;

  /* Set empty ED - set HEAD = TAIL = last (not processed) TD */
  cdata->ed_virt->td_head = holy_cpu_to_le32 (holy_le_to_cpu32 (cdata->ed_virt->td_tail) & ~0xf);

  /* At this point always should be:
   * ED has skip bit set and halted or empty or after next SOF,
   *    i.e. it is safe to free all TDs except last not processed
   * ED HEAD == TAIL == phys. addr. of td_current_virt */

  /* Un-chainig of last TD */
  if (cdata->td_current_virt->prev_td_phys)
    {
      holy_ohci_td_t td_prev_virt
	= holy_ohci_td_phys2virt (o, cdata->td_current_virt->prev_td_phys);
      
      if (cdata->td_current_virt == (holy_ohci_td_t) td_prev_virt->link_td)
        td_prev_virt->link_td = 0;

      cdata->td_current_virt->prev_td_phys = 0;
    }

  holy_dprintf ("ohci", "OHCI finished, freeing\n");
  holy_ohci_free_tds (o, cdata->td_head_virt);
  holy_free (cdata);
}

static holy_usb_err_t
parse_halt (holy_usb_controller_t dev,
	    holy_usb_transfer_t transfer,
	    holy_size_t *actual)
{
  struct holy_ohci *o = dev->data;
  struct holy_ohci_transfer_controller_data *cdata = transfer->controller_data;
  holy_uint8_t errcode = 0;
  holy_usb_err_t err = holy_USB_ERR_NAK;
  holy_ohci_td_t tderr_virt = NULL;

  *actual = 0;

  pre_finish_transfer (dev, transfer);

  /* First we must get proper tderr_phys value */
  /* Retired TD with error should be previous TD to ED->td_head */
  cdata->tderr_phys = holy_ohci_td_phys2virt (o,
                                                holy_le_to_cpu32 (cdata->ed_virt->td_head) & ~0xf )
	              ->prev_td_phys;

  /* Prepare pointer to last processed TD and get error code */
  tderr_virt = holy_ohci_td_phys2virt (o, cdata->tderr_phys);
  /* Set index of last processed TD */
  if (tderr_virt)
    {
      errcode = holy_le_to_cpu32 (tderr_virt->token) >> 28;
      transfer->last_trans = tderr_virt->tr_index;
    }
  else
    transfer->last_trans = -1;
  
  /* Evaluation of error code */
  holy_dprintf ("ohci", "OHCI tderr_phys=0x%02x, errcode=0x%02x\n",
		cdata->tderr_phys, errcode);
  switch (errcode)
    {
    case 0:
      /* XXX: Should not happen!  */
      holy_error (holy_ERR_IO, "OHCI failed without reporting the reason");
      err = holy_USB_ERR_INTERNAL;
      break;

    case 1:
      /* XXX: CRC error.  */
      err = holy_USB_ERR_TIMEOUT;
      break;

    case 2:
      err = holy_USB_ERR_BITSTUFF;
      break;

    case 3:
      /* XXX: Data Toggle error.  */
      err = holy_USB_ERR_DATA;
      break;

    case 4:
      err = holy_USB_ERR_STALL;
      break;

    case 5:
      /* XXX: Not responding.  */
      err = holy_USB_ERR_TIMEOUT;
      break;

    case 6:
      /* XXX: PID Check bits failed.  */
      err = holy_USB_ERR_BABBLE;
      break;

    case 7:
      /* XXX: PID unexpected failed.  */
      err = holy_USB_ERR_BABBLE;
      break;

    case 8:
      /* XXX: Data overrun error.  */
      err = holy_USB_ERR_DATA;
      holy_dprintf ("ohci", "Overrun, failed TD address: %p, index: %d\n",
		    tderr_virt, tderr_virt->tr_index);
      break;

    case 9:
      /* XXX: Data underrun error.  */
      holy_dprintf ("ohci", "Underrun, failed TD address: %p, index: %d\n",
		    tderr_virt, tderr_virt->tr_index);
      if (transfer->last_trans == -1)
	break;
      *actual = transfer->transactions[transfer->last_trans].size
	- (holy_le_to_cpu32 (tderr_virt->buffer_end)
	   - holy_le_to_cpu32 (tderr_virt->buffer))
	+ transfer->transactions[transfer->last_trans].preceding;
      err = holy_USB_ERR_NONE;
      break;

    case 10:
      /* XXX: Reserved.  */
      err = holy_USB_ERR_NAK;
      break;

    case 11:
      /* XXX: Reserved.  */
      err = holy_USB_ERR_NAK;
      break;

    case 12:
      /* XXX: Buffer overrun.  */
      err = holy_USB_ERR_DATA;
      break;

    case 13:
      /* XXX: Buffer underrun.  */
      err = holy_USB_ERR_DATA;
      break;

    default:
      err = holy_USB_ERR_NAK;
      break;
    }

  finish_transfer (dev, transfer);

  return err;
}

static holy_usb_err_t
parse_success (holy_usb_controller_t dev,
	       holy_usb_transfer_t transfer,
	       holy_size_t *actual)
{
  struct holy_ohci *o = dev->data;
  struct holy_ohci_transfer_controller_data *cdata = transfer->controller_data;
  holy_ohci_td_t tderr_virt = NULL;

  pre_finish_transfer (dev, transfer);

  /* I hope we can do it as transfer (most probably) finished OK */
  cdata->tderr_phys = cdata->td_last_phys;

  /* Prepare pointer to last processed TD */
  tderr_virt = holy_ohci_td_phys2virt (o, cdata->tderr_phys);
  
  /* Set index of last processed TD */
  if (tderr_virt)
    transfer->last_trans = tderr_virt->tr_index;
  else
    transfer->last_trans = -1;
  *actual = transfer->size + 1;

  finish_transfer (dev, transfer);

  return holy_USB_ERR_NONE;
}

static holy_usb_err_t
parse_unrec (holy_usb_controller_t dev,
	     holy_usb_transfer_t transfer,
	     holy_size_t *actual)
{
  struct holy_ohci *o = dev->data;

  *actual = 0;

  pre_finish_transfer (dev, transfer);

  /* Don't try to get error code and last processed TD for proper
   * toggle bit value - anything can be invalid */
  holy_dprintf("ohci", "Unrecoverable error!");

  /* Do OHCI reset in case of unrecoverable error - maybe we will need
   * do more - re-enumerate bus etc. (?) */

  /* Suspend the OHCI by issuing a reset.  */
  holy_ohci_writereg32 (o, holy_OHCI_REG_CMDSTATUS, 1); /* XXX: Magic.  */
  /* Read back of register should ensure it is really written */
  holy_ohci_readreg32 (o, holy_OHCI_REG_CMDSTATUS);
  holy_millisleep (1);
  holy_dprintf ("ohci", "Unrecoverable error - OHCI reset\n");

  /* Misc. resets. */
  o->hcca->donehead = 0;
  holy_ohci_writereg32 (o, holy_OHCI_REG_INTSTATUS, 0x7f); /* Clears everything */
  holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROLHEAD, o->ed_ctrl_addr);
  holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROLCURR, 0);
  holy_ohci_writereg32 (o, holy_OHCI_REG_BULKHEAD, o->ed_bulk_addr);
  holy_ohci_writereg32 (o, holy_OHCI_REG_BULKCURR, 0);
  /* Read back of register should ensure it is really written */
  holy_ohci_readreg32 (o, holy_OHCI_REG_INTSTATUS);

  /* Enable the OHCI.  */
  holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROL,
			(2 << 6)
			| holy_OHCI_REG_CONTROL_CONTROL_ENABLE
			| holy_OHCI_REG_CONTROL_BULK_ENABLE );
  finish_transfer (dev, transfer);

  return holy_USB_ERR_UNRECOVERABLE;
}

static holy_usb_err_t
holy_ohci_check_transfer (holy_usb_controller_t dev,
			  holy_usb_transfer_t transfer,
			  holy_size_t *actual)
{
  struct holy_ohci *o = dev->data;
  struct holy_ohci_transfer_controller_data *cdata = transfer->controller_data;
  holy_uint32_t intstatus;

  /* Check transfer status */
  intstatus = holy_ohci_readreg32 (o, holy_OHCI_REG_INTSTATUS);

  if ((intstatus & 0x10) != 0)
    /* Unrecoverable error - only reset can help...! */
    return parse_unrec (dev, transfer, actual);

  /* Detected a HALT.  */
  if ((holy_le_to_cpu32 (cdata->ed_virt->td_head) & 1))
    return parse_halt (dev, transfer, actual);

  /* Finished ED detection */
  if ( (holy_le_to_cpu32 (cdata->ed_virt->td_head) & ~0xfU) ==
       (holy_le_to_cpu32 (cdata->ed_virt->td_tail) & ~0xfU) ) /* Empty ED */
    {
      /* Check the HALT bit */
      /* It looks like nonsense - it was tested previously...
       * but it can change because OHCI is working
       * simultaneously via DMA... */
      if (holy_le_to_cpu32 (cdata->ed_virt->td_head) & 1)
	return parse_halt (dev, transfer, actual);
      else
        return parse_success (dev, transfer, actual);
    }

  return holy_USB_ERR_WAIT;
}

static holy_usb_err_t
holy_ohci_cancel_transfer (holy_usb_controller_t dev,
			   holy_usb_transfer_t transfer)
{
  struct holy_ohci *o = dev->data;
  struct holy_ohci_transfer_controller_data *cdata = transfer->controller_data;
  holy_ohci_td_t tderr_virt = NULL;

  pre_finish_transfer (dev, transfer);

  holy_dprintf("ohci", "Timeout !\n");

  /* We should wait for next SOF to be sure that ED is unaccessed
   * by OHCI */
  /* SF bit reset. (SF bit indicates Start Of Frame (SOF) packet) */
  holy_ohci_writereg32 (o, holy_OHCI_REG_INTSTATUS, (1<<2));
  /* Wait for new SOF */
  while ((holy_ohci_readreg32 (o, holy_OHCI_REG_INTSTATUS) & 0x4) == 0);

  /* Possible retired TD with error should be previous TD to ED->td_head */
  cdata->tderr_phys
    = holy_ohci_td_phys2virt (o, holy_le_to_cpu32 (cdata->ed_virt->td_head)
                              & ~0xf)->prev_td_phys;
    
  tderr_virt = holy_ohci_td_phys2virt (o,cdata-> tderr_phys);

  holy_dprintf ("ohci", "Cancel: tderr_phys=0x%x, tderr_virt=%p\n",
                cdata->tderr_phys, tderr_virt);

  if (tderr_virt)
    transfer->last_trans = tderr_virt->tr_index;
  else
    transfer->last_trans = -1;

  finish_transfer (dev, transfer);

  return holy_USB_ERR_NONE;
}

static holy_usb_err_t
holy_ohci_portstatus (holy_usb_controller_t dev,
		      unsigned int port, unsigned int enable)
{
   struct holy_ohci *o = (struct holy_ohci *) dev->data;
   holy_uint64_t endtime;
   int i;

   holy_dprintf ("ohci", "begin of portstatus=0x%02x\n",
                 holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port));

   if (!enable) /* We don't need reset port */
     {
       /* Disable the port and wait for it. */
       holy_ohci_writereg32 (o, holy_OHCI_REG_RHUBPORT + port,
                             holy_OHCI_CLEAR_PORT_ENABLE);
       endtime = holy_get_time_ms () + 1000;
       while ((holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port)
               & (1 << 1)))
         if (holy_get_time_ms () > endtime)
           return holy_USB_ERR_TIMEOUT;

       holy_dprintf ("ohci", "end of portstatus=0x%02x\n",
         holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port));
       return holy_USB_ERR_NONE;
     }
     
   /* OHCI does one reset signal 10ms long but USB spec.
    * requests 50ms for root hub (no need to be continuous).
    * So, we do reset 5 times... */
   for (i = 0; i < 5; i++)
     {
       /* Reset the port - timing of reset is done by OHCI */
       holy_ohci_writereg32 (o, holy_OHCI_REG_RHUBPORT + port,
                             holy_OHCI_SET_PORT_RESET);

       /* Wait for reset completion */
       endtime = holy_get_time_ms () + 1000;
       while (! (holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port)
               & holy_OHCI_SET_PORT_RESET_STATUS_CHANGE))
         if (holy_get_time_ms () > endtime)
           return holy_USB_ERR_TIMEOUT;

       /* End the reset signaling - reset the reset status change */
       holy_ohci_writereg32 (o, holy_OHCI_REG_RHUBPORT + port,
			     holy_OHCI_SET_PORT_RESET_STATUS_CHANGE);
       holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port);
     }

   /* Enable port */
   holy_ohci_writereg32 (o, holy_OHCI_REG_RHUBPORT + port,
                         holy_OHCI_SET_PORT_ENABLE);
   holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port);
   
   /* Wait for signal enabled */
   endtime = holy_get_time_ms () + 1000;
   while (! (holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port)
           & (1 << 1)))
     if (holy_get_time_ms () > endtime)
       return holy_USB_ERR_TIMEOUT;

   /* Reset bit Connect Status Change */
   holy_ohci_writereg32 (o, holy_OHCI_REG_RHUBPORT + port,
                         holy_OHCI_RESET_CONNECT_CHANGE);

   /* "Reset recovery time" (USB spec.) */
   holy_millisleep (10);
   
   holy_dprintf ("ohci", "end of portstatus=0x%02x\n",
		 holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port));
 
   return holy_USB_ERR_NONE;
}

static holy_usb_speed_t
holy_ohci_detect_dev (holy_usb_controller_t dev, int port, int *changed)
{
   struct holy_ohci *o = (struct holy_ohci *) dev->data;
   holy_uint32_t status;

   status = holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBPORT + port);

   holy_dprintf ("ohci", "detect_dev status=0x%02x\n", status);

   /* Connect Status Change bit - it detects change of connection */
   if (status & holy_OHCI_RESET_CONNECT_CHANGE)
     {
       *changed = 1;
       /* Reset bit Connect Status Change */
       holy_ohci_writereg32 (o, holy_OHCI_REG_RHUBPORT + port,
			     holy_OHCI_RESET_CONNECT_CHANGE);
     }
   else
     *changed = 0;

   if (! (status & 1))
     return holy_USB_SPEED_NONE;
   else if (status & (1 << 9))
     return holy_USB_SPEED_LOW;
   else
     return holy_USB_SPEED_FULL;
}

static int
holy_ohci_hubports (holy_usb_controller_t dev)
{
  struct holy_ohci *o = (struct holy_ohci *) dev->data;
  holy_uint32_t portinfo;

  portinfo = holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBA);

  holy_dprintf ("ohci", "root hub ports=%d\n", portinfo & 0xFF);

  return portinfo & 0xFF;
}

static holy_err_t
holy_ohci_fini_hw (int noreturn __attribute__ ((unused)))
{
  struct holy_ohci *o;

  for (o = ohci; o; o = o->next)
    {
      int i, nports = holy_ohci_readreg32 (o, holy_OHCI_REG_RHUBA) & 0xff;
      holy_uint64_t maxtime;

      /* Set skip in all EDs */
      if (o->ed_bulk)
        for (i=0; i < holy_OHCI_BULK_EDS; i++)
          o->ed_bulk[i].target |= holy_cpu_to_le32_compile_time (1 << 14); /* skip */
      if (o->ed_ctrl)
        for (i=0; i < holy_OHCI_CTRL_EDS; i++)
          o->ed_ctrl[i].target |= holy_cpu_to_le32_compile_time (1 << 14); /* skip */

      /* We should wait for next SOF to be sure that all EDs are
       * unaccessed by OHCI. But OHCI can be non-functional, so
       * more than 1ms timeout have to be applied. */
      /* SF bit reset. (SF bit indicates Start Of Frame (SOF) packet) */
      holy_ohci_writereg32 (o, holy_OHCI_REG_INTSTATUS, (1<<2));
      maxtime = holy_get_time_ms () + 2;
      /* Wait for new SOF or timeout */
      while ( ((holy_ohci_readreg32 (o, holy_OHCI_REG_INTSTATUS) & 0x4)
                 == 0) || (holy_get_time_ms () >= maxtime) );

      for (i = 0; i < nports; i++)
	holy_ohci_writereg32 (o, holy_OHCI_REG_RHUBPORT + i,
			      holy_OHCI_CLEAR_PORT_ENABLE);

      holy_ohci_writereg32 (o, holy_OHCI_REG_CMDSTATUS, 1);
      holy_millisleep (1);
      holy_ohci_writereg32 (o, holy_OHCI_REG_HCCA, 0);
      holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROLHEAD, 0);
      holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROLCURR, 0);
      holy_ohci_writereg32 (o, holy_OHCI_REG_BULKHEAD, 0);
      holy_ohci_writereg32 (o, holy_OHCI_REG_BULKCURR, 0);
      holy_ohci_writereg32 (o, holy_OHCI_REG_DONEHEAD, 0);
      holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROL, 0);
      /* Read back of register should ensure it is really written */
      holy_ohci_readreg32 (o, holy_OHCI_REG_INTSTATUS);

#if 0 /* Is this necessary before booting? Probably not .(?)
       * But it must be done if module is removed ! (Or not ?)
       * How to do it ? - Probably holy_ohci_restore_hw should be more
       * complicated. (?)
       * (If we do it, we need to reallocate EDs and TDs in function
       * holy_ohci_restore_hw ! */

      /* Free allocated EDs and TDs */
      holy_dma_free (o->td_chunk);
      holy_dma_free (o->ed_bulk_chunk);
      holy_dma_free (o->ed_ctrl_chunk);
      holy_dma_free (o->hcca_chunk);
#endif
    }
  holy_millisleep (10);

  return holy_ERR_NONE;
}

static holy_err_t
holy_ohci_restore_hw (void)
{
  struct holy_ohci *o;

  for (o = ohci; o; o = o->next)
    {
      holy_ohci_writereg32 (o, holy_OHCI_REG_HCCA, o->hcca_addr);
      o->hcca->donehead = 0;
      holy_ohci_writereg32 (o, holy_OHCI_REG_INTSTATUS, 0x7f); /* Clears everything */
      holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROLHEAD, o->ed_ctrl_addr);
      holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROLCURR, 0);
      holy_ohci_writereg32 (o, holy_OHCI_REG_BULKHEAD, o->ed_bulk_addr);
      holy_ohci_writereg32 (o, holy_OHCI_REG_BULKCURR, 0);
      /* Read back of register should ensure it is really written */
      holy_ohci_readreg32 (o, holy_OHCI_REG_INTSTATUS);

      /* Enable the OHCI.  */
      holy_ohci_writereg32 (o, holy_OHCI_REG_CONTROL,
                            (2 << 6)
                            | holy_OHCI_REG_CONTROL_CONTROL_ENABLE
                            | holy_OHCI_REG_CONTROL_BULK_ENABLE );
    }

  return holy_ERR_NONE;
}


static struct holy_usb_controller_dev usb_controller =
{
  .name = "ohci",
  .iterate = holy_ohci_iterate,
  .setup_transfer = holy_ohci_setup_transfer,
  .check_transfer = holy_ohci_check_transfer,
  .cancel_transfer = holy_ohci_cancel_transfer,
  .hubports = holy_ohci_hubports,
  .portstatus = holy_ohci_portstatus,
  .detect_dev = holy_ohci_detect_dev,
  /* estimated max. count of TDs for one bulk transfer */
  .max_bulk_tds = holy_OHCI_TDS * 3 / 4
};

static struct holy_preboot *fini_hnd;

holy_MOD_INIT(ohci)
{
  COMPILE_TIME_ASSERT (sizeof (struct holy_ohci_td) == 32);
  COMPILE_TIME_ASSERT (sizeof (struct holy_ohci_ed) == 16);

  holy_stop_disk_firmware ();

  holy_ohci_inithw ();
  holy_usb_controller_dev_register (&usb_controller);
  fini_hnd = holy_loader_register_preboot_hook (holy_ohci_fini_hw,
						holy_ohci_restore_hw,
						holy_LOADER_PREBOOT_HOOK_PRIO_DISK);
}

holy_MOD_FINI(ohci)
{
  holy_ohci_fini_hw (0);
  holy_loader_unregister_preboot_hook (fini_hnd);
  holy_usb_controller_dev_unregister (&usb_controller);
}
