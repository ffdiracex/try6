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
#include <holy/loader.h>
#include <holy/cs5536.h>
#include <holy/disk.h>
#include <holy/cache.h>

holy_MOD_LICENSE ("GPLv2+");

/* This simple holy implementation of EHCI driver:
 *      - assumes no IRQ
 *      - is not supporting isochronous transfers (iTD, siTD)
 *      - is not supporting interrupt transfers
 */

#define holy_EHCI_PCI_SBRN_REG  0x60

/* Capability registers offsets */
enum
{
  holy_EHCI_EHCC_CAPLEN = 0x00,	/* byte */
  holy_EHCI_EHCC_VERSION = 0x02,	/* word */
  holy_EHCI_EHCC_SPARAMS = 0x04,	/* dword */
  holy_EHCI_EHCC_CPARAMS = 0x08,	/* dword */
  holy_EHCI_EHCC_PROUTE = 0x0c,	/* 60 bits */
};

#define holy_EHCI_EECP_MASK     (0xff << 8)
#define holy_EHCI_EECP_SHIFT    8

#define holy_EHCI_ADDR_MEM_MASK	(~0xff)
#define holy_EHCI_POINTER_MASK	(~0x1f)

/* Capability register SPARAMS bits */
enum
{
  holy_EHCI_SPARAMS_N_PORTS = (0xf << 0),
  holy_EHCI_SPARAMS_PPC = (1 << 4),	/* Power port control */
  holy_EHCI_SPARAMS_PRR = (1 << 7),	/* Port routing rules */
  holy_EHCI_SPARAMS_N_PCC = (0xf << 8),	/* No of ports per comp. */
  holy_EHCI_SPARAMS_NCC = (0xf << 12),	/* No of com. controllers */
  holy_EHCI_SPARAMS_P_IND = (1 << 16),	/* Port indicators present */
  holy_EHCI_SPARAMS_DEBUG_P = (0xf << 20)	/* Debug port */
};

#define holy_EHCI_MAX_N_PORTS     15	/* Max. number of ports */

/* Capability register CPARAMS bits */
enum
{
  holy_EHCI_CPARAMS_64BIT = (1 << 0),
  holy_EHCI_CPARAMS_PROG_FRAMELIST = (1 << 1),
  holy_EHCI_CPARAMS_PARK_CAP = (1 << 2)
};

#define holy_EHCI_N_FRAMELIST   1024
#define holy_EHCI_N_QH  256
#define holy_EHCI_N_TD  640

#define holy_EHCI_QH_EMPTY 1

/* USBLEGSUP bits and related OS OWNED byte offset */
enum
{
  holy_EHCI_BIOS_OWNED = (1 << 16),
  holy_EHCI_OS_OWNED = (1 << 24)
};

/* Operational registers offsets */
enum
{
  holy_EHCI_COMMAND = 0x00,
  holy_EHCI_STATUS = 0x04,
  holy_EHCI_INTERRUPT = 0x08,
  holy_EHCI_FRAME_INDEX = 0x0c,
  holy_EHCI_64BIT_SEL = 0x10,
  holy_EHCI_FL_BASE = 0x14,
  holy_EHCI_CUR_AL_ADDR = 0x18,
  holy_EHCI_CONFIG_FLAG = 0x40,
  holy_EHCI_PORT_STAT_CMD = 0x44
};

/* Operational register COMMAND bits */
enum
{
  holy_EHCI_CMD_RUNSTOP = (1 << 0),
  holy_EHCI_CMD_HC_RESET = (1 << 1),
  holy_EHCI_CMD_FL_SIZE = (3 << 2),
  holy_EHCI_CMD_PS_ENABL = (1 << 4),
  holy_EHCI_CMD_AS_ENABL = (1 << 5),
  holy_EHCI_CMD_AS_ADV_D = (1 << 6),
  holy_EHCI_CMD_L_HC_RES = (1 << 7),
  holy_EHCI_CMD_AS_PARKM = (3 << 8),
  holy_EHCI_CMD_AS_PARKE = (1 << 11),
  holy_EHCI_CMD_INT_THRS = (0xff << 16)
};

/* Operational register STATUS bits */
enum
{
  holy_EHCI_ST_INTERRUPT = (1 << 0),
  holy_EHCI_ST_ERROR_INT = (1 << 1),
  holy_EHCI_ST_PORT_CHG = (1 << 2),
  holy_EHCI_ST_FL_ROLLOVR = (1 << 3),
  holy_EHCI_ST_HS_ERROR = (1 << 4),
  holy_EHCI_ST_AS_ADVANCE = (1 << 5),
  holy_EHCI_ST_HC_HALTED = (1 << 12),
  holy_EHCI_ST_RECLAM = (1 << 13),
  holy_EHCI_ST_PS_STATUS = (1 << 14),
  holy_EHCI_ST_AS_STATUS = (1 << 15)
};

/* Operational register PORT_STAT_CMD bits */
enum
{
  holy_EHCI_PORT_CONNECT = (1 << 0),
  holy_EHCI_PORT_CONNECT_CH = (1 << 1),
  holy_EHCI_PORT_ENABLED = (1 << 2),
  holy_EHCI_PORT_ENABLED_CH = (1 << 3),
  holy_EHCI_PORT_OVERCUR = (1 << 4),
  holy_EHCI_PORT_OVERCUR_CH = (1 << 5),
  holy_EHCI_PORT_RESUME = (1 << 6),
  holy_EHCI_PORT_SUSPEND = (1 << 7),
  holy_EHCI_PORT_RESET = (1 << 8),
  holy_EHCI_PORT_LINE_STAT = (3 << 10),
  holy_EHCI_PORT_POWER = (1 << 12),
  holy_EHCI_PORT_OWNER = (1 << 13),
  holy_EHCI_PORT_INDICATOR = (3 << 14),
  holy_EHCI_PORT_TEST = (0xf << 16),
  holy_EHCI_PORT_WON_CONN_E = (1 << 20),
  holy_EHCI_PORT_WON_DISC_E = (1 << 21),
  holy_EHCI_PORT_WON_OVER_E = (1 << 22),

  holy_EHCI_PORT_LINE_SE0 = (0 << 10),
  holy_EHCI_PORT_LINE_K = (1 << 10),
  holy_EHCI_PORT_LINE_J = (2 << 10),
  holy_EHCI_PORT_LINE_UNDEF = (3 << 10),
  holy_EHCI_PORT_LINE_LOWSP = holy_EHCI_PORT_LINE_K,	/* K state means low speed */
  holy_EHCI_PORT_WMASK = ~(holy_EHCI_PORT_CONNECT_CH
			   | holy_EHCI_PORT_ENABLED_CH
			   | holy_EHCI_PORT_OVERCUR_CH)
};

/* Operational register CONFIGFLAGS bits */
enum
{
  holy_EHCI_CF_EHCI_OWNER = (1 << 0)
};

/* Queue Head & Transfer Descriptor constants */
#define holy_EHCI_HPTR_OFF       5	/* Horiz. pointer bit offset */
enum
{
  holy_EHCI_HPTR_TYPE_MASK = (3 << 1),
  holy_EHCI_HPTR_TYPE_ITD = (0 << 1),
  holy_EHCI_HPTR_TYPE_QH = (1 << 1),
  holy_EHCI_HPTR_TYPE_SITD = (2 << 1),
  holy_EHCI_HPTR_TYPE_FSTN = (3 << 1)
};

enum
{
  holy_EHCI_C = (1 << 27),
  holy_EHCI_MAXPLEN_MASK = (0x7ff << 16),
  holy_EHCI_H = (1 << 15),
  holy_EHCI_DTC = (1 << 14),
  holy_EHCI_SPEED_MASK = (3 << 12),
  holy_EHCI_SPEED_FULL = (0 << 12),
  holy_EHCI_SPEED_LOW = (1 << 12),
  holy_EHCI_SPEED_HIGH = (2 << 12),
  holy_EHCI_SPEED_RESERVED = (3 << 12),
  holy_EHCI_EP_NUM_MASK = (0xf << 8),
  holy_EHCI_DEVADDR_MASK = 0x7f,
  holy_EHCI_TARGET_MASK = (holy_EHCI_EP_NUM_MASK | holy_EHCI_DEVADDR_MASK)
};

enum
{
  holy_EHCI_MAXPLEN_OFF = 16,
  holy_EHCI_SPEED_OFF = 12,
  holy_EHCI_EP_NUM_OFF = 8
};

enum
{
  holy_EHCI_MULT_MASK = (3 << 30),
  holy_EHCI_MULT_RESERVED = (0 << 30),
  holy_EHCI_MULT_ONE = (1 << 30),
  holy_EHCI_MULT_TWO = (2 << 30),
  holy_EHCI_MULT_THREE = (3 << 30),
  holy_EHCI_DEVPORT_MASK = (0x7f << 23),
  holy_EHCI_HUBADDR_MASK = (0x7f << 16),
  holy_EHCI_CMASK_MASK = (0xff << 8),
  holy_EHCI_SMASK_MASK = (0xff << 0),
};

enum
{
  holy_EHCI_MULT_OFF = 30,
  holy_EHCI_DEVPORT_OFF = 23,
  holy_EHCI_HUBADDR_OFF = 16,
  holy_EHCI_CMASK_OFF = 8,
  holy_EHCI_SMASK_OFF = 0,
};

#define holy_EHCI_TERMINATE      (1<<0)

#define holy_EHCI_TOGGLE         (1<<31)

enum
{
  holy_EHCI_TOTAL_MASK = (0x7fff << 16),
  holy_EHCI_CERR_MASK = (3 << 10),
  holy_EHCI_CERR_0 = (0 << 10),
  holy_EHCI_CERR_1 = (1 << 10),
  holy_EHCI_CERR_2 = (2 << 10),
  holy_EHCI_CERR_3 = (3 << 10),
  holy_EHCI_PIDCODE_OUT = (0 << 8),
  holy_EHCI_PIDCODE_IN = (1 << 8),
  holy_EHCI_PIDCODE_SETUP = (2 << 8),
  holy_EHCI_STATUS_MASK = 0xff,
  holy_EHCI_STATUS_ACTIVE = (1 << 7),
  holy_EHCI_STATUS_HALTED = (1 << 6),
  holy_EHCI_STATUS_BUFERR = (1 << 5),
  holy_EHCI_STATUS_BABBLE = (1 << 4),
  holy_EHCI_STATUS_TRANERR = (1 << 3),
  holy_EHCI_STATUS_MISSDMF = (1 << 2),
  holy_EHCI_STATUS_SPLITST = (1 << 1),
  holy_EHCI_STATUS_PINGERR = (1 << 0)
};

enum
{
  holy_EHCI_TOTAL_OFF = 16,
  holy_EHCI_CERR_OFF = 10
};

#define holy_EHCI_BUFPTR_MASK    (0xfffff<<12)
#define holy_EHCI_QHTDPTR_MASK   0xffffffe0

#define holy_EHCI_TD_BUF_PAGES   5

#define holy_EHCI_BUFPAGELEN     0x1000
#define holy_EHCI_MAXBUFLEN      0x5000

struct holy_ehci_td;
struct holy_ehci_qh;
typedef volatile struct holy_ehci_td *holy_ehci_td_t;
typedef volatile struct holy_ehci_qh *holy_ehci_qh_t;

/* EHCI Isochronous Transfer Descriptor */
/* Currently not supported */

/* EHCI Split Transaction Isochronous Transfer Descriptor */
/* Currently not supported */

/* EHCI Queue Element Transfer Descriptor (qTD) */
/* Align to 32-byte boundaries */
struct holy_ehci_td
{
  /* EHCI HW part */
  holy_uint32_t next_td;	/* Pointer to next qTD */
  holy_uint32_t alt_next_td;	/* Pointer to alternate next qTD */
  holy_uint32_t token;		/* Toggle, Len, Interrupt, Page, Error, PID, Status */
  holy_uint32_t buffer_page[holy_EHCI_TD_BUF_PAGES];	/* Buffer pointer (+ cur. offset in page 0 */
  /* 64-bits part */
  holy_uint32_t buffer_page_high[holy_EHCI_TD_BUF_PAGES];
  /* EHCI driver part */
  holy_uint32_t link_td;	/* pointer to next free/chained TD */
  holy_uint32_t size;
  holy_uint32_t pad[1];		/* padding to some multiple of 32 bytes */
};

/* EHCI Queue Head */
/* Align to 32-byte boundaries */
/* QH allocation is made in the similar/same way as in OHCI driver,
 * because unlninking QH from the Asynchronous list is not so
 * trivial as on UHCI (at least it is time consuming) */
struct holy_ehci_qh
{
  /* EHCI HW part */
  holy_uint32_t qh_hptr;	/* Horiz. pointer & Terminate */
  holy_uint32_t ep_char;	/* EP characteristics */
  holy_uint32_t ep_cap;		/* EP capabilities */
  holy_uint32_t td_current;	/* current TD link pointer  */
  struct holy_ehci_td td_overlay;	/* TD overlay area = 64 bytes */
  /* EHCI driver part */
  holy_uint32_t pad[4];		/* padding to some multiple of 32 bytes */
};

/* EHCI Periodic Frame Span Traversal Node */
/* Currently not supported */

struct holy_ehci
{
  volatile holy_uint32_t *iobase_ehcc;	/* Capability registers */
  volatile holy_uint32_t *iobase;	/* Operational registers */
  struct holy_pci_dma_chunk *framelist_chunk;	/* Currently not used */
  volatile holy_uint32_t *framelist_virt;
  holy_uint32_t framelist_phys;
  struct holy_pci_dma_chunk *qh_chunk;	/* holy_EHCI_N_QH Queue Heads */
  holy_ehci_qh_t qh_virt;
  holy_uint32_t qh_phys;
  struct holy_pci_dma_chunk *td_chunk;	/* holy_EHCI_N_TD Transfer Descriptors */
  holy_ehci_td_t td_virt;
  holy_uint32_t td_phys;
  holy_ehci_td_t tdfree_virt;	/* Free Transfer Descriptors */
  int flag64;
  holy_uint32_t reset;		/* bits 1-15 are flags if port was reset from connected time or not */
  struct holy_ehci *next;
};

static struct holy_ehci *ehci;

static void
sync_all_caches (struct holy_ehci *e)
{
  if (!e)
    return;
  if (e->td_virt)
    holy_arch_sync_dma_caches (e->td_virt, sizeof (struct holy_ehci_td) *
			       holy_EHCI_N_TD);
  if (e->qh_virt)
    holy_arch_sync_dma_caches (e->qh_virt, sizeof (struct holy_ehci_qh) *
			       holy_EHCI_N_QH);
  if (e->framelist_virt)
    holy_arch_sync_dma_caches (e->framelist_virt, 4096);
}

/* EHCC registers access functions */
static inline holy_uint32_t
holy_ehci_ehcc_read32 (struct holy_ehci *e, holy_uint32_t addr)
{
  return
    holy_le_to_cpu32 (*((volatile holy_uint32_t *) e->iobase_ehcc +
		       (addr / sizeof (holy_uint32_t))));
}

static inline holy_uint16_t
holy_ehci_ehcc_read16 (struct holy_ehci *e, holy_uint32_t addr)
{
  return
    holy_le_to_cpu16 (*((volatile holy_uint16_t *) e->iobase_ehcc +
		       (addr / sizeof (holy_uint16_t))));
}

static inline holy_uint8_t
holy_ehci_ehcc_read8 (struct holy_ehci *e, holy_uint32_t addr)
{
  return *((volatile holy_uint8_t *) e->iobase_ehcc + addr);
}

/* Operational registers access functions */
static inline holy_uint32_t
holy_ehci_oper_read32 (struct holy_ehci *e, holy_uint32_t addr)
{
  return
    holy_le_to_cpu32 (*
		      ((volatile holy_uint32_t *) e->iobase +
		       (addr / sizeof (holy_uint32_t))));
}

static inline void
holy_ehci_oper_write32 (struct holy_ehci *e, holy_uint32_t addr,
			holy_uint32_t value)
{
  *((volatile holy_uint32_t *) e->iobase + (addr / sizeof (holy_uint32_t))) =
    holy_cpu_to_le32 (value);
}

static inline holy_uint32_t
holy_ehci_port_read (struct holy_ehci *e, holy_uint32_t port)
{
  return holy_ehci_oper_read32 (e, holy_EHCI_PORT_STAT_CMD + port * 4);
}

static inline void
holy_ehci_port_resbits (struct holy_ehci *e, holy_uint32_t port,
			holy_uint32_t bits)
{
  holy_ehci_oper_write32 (e, holy_EHCI_PORT_STAT_CMD + port * 4,
			  holy_ehci_port_read (e,
					       port) & holy_EHCI_PORT_WMASK &
			  ~(bits));
  holy_ehci_port_read (e, port);
}

static inline void
holy_ehci_port_setbits (struct holy_ehci *e, holy_uint32_t port,
			holy_uint32_t bits)
{
  holy_ehci_oper_write32 (e, holy_EHCI_PORT_STAT_CMD + port * 4,
			  (holy_ehci_port_read (e, port) &
			   holy_EHCI_PORT_WMASK) | bits);
  holy_ehci_port_read (e, port);
}

/* Halt if EHCI HC not halted */
static holy_usb_err_t
holy_ehci_halt (struct holy_ehci *e)
{
  holy_uint64_t maxtime;

  if ((holy_ehci_oper_read32 (e, holy_EHCI_STATUS) & holy_EHCI_ST_HC_HALTED) == 0)	/* EHCI is not halted */
    {
      /* Halt EHCI */
      holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
			      ~holy_EHCI_CMD_RUNSTOP
			      & holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));
      /* Ensure command is written */
      holy_ehci_oper_read32 (e, holy_EHCI_COMMAND);
      maxtime = holy_get_time_ms () + 1000;	/* Fix: Should be 2ms ! */
      while (((holy_ehci_oper_read32 (e, holy_EHCI_STATUS)
	       & holy_EHCI_ST_HC_HALTED) == 0)
	     && (holy_get_time_ms () < maxtime));
      if ((holy_ehci_oper_read32 (e, holy_EHCI_STATUS)
	   & holy_EHCI_ST_HC_HALTED) == 0)
	return holy_USB_ERR_TIMEOUT;
    }

  return holy_USB_ERR_NONE;
}

/* EHCI HC reset */
static holy_usb_err_t
holy_ehci_reset (struct holy_ehci *e)
{
  holy_uint64_t maxtime;

  sync_all_caches (e);

  holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
			  holy_EHCI_CMD_HC_RESET
			  | holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));
  /* Ensure command is written */
  holy_ehci_oper_read32 (e, holy_EHCI_COMMAND);
  /* XXX: How long time could take reset of HC ? */
  maxtime = holy_get_time_ms () + 1000;
  while (((holy_ehci_oper_read32 (e, holy_EHCI_COMMAND)
	   & holy_EHCI_CMD_HC_RESET) != 0)
	 && (holy_get_time_ms () < maxtime));
  if ((holy_ehci_oper_read32 (e, holy_EHCI_COMMAND)
       & holy_EHCI_CMD_HC_RESET) != 0)
    return holy_USB_ERR_TIMEOUT;

  return holy_USB_ERR_NONE;
}

/* PCI iteration function... */
static int
holy_ehci_pci_iter (holy_pci_device_t dev, holy_pci_id_t pciid,
		    void *data __attribute__ ((unused)))
{
  holy_uint8_t release;
  holy_uint32_t class_code;
  holy_uint32_t interf;
  holy_uint32_t subclass;
  holy_uint32_t class;
  holy_uint32_t base, base_h;
  struct holy_ehci *e;
  holy_uint32_t eecp_offset;
  holy_uint32_t fp;
  int i;
  holy_uint32_t usblegsup = 0;
  holy_uint64_t maxtime;
  holy_uint32_t n_ports;
  holy_uint8_t caplen;

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: begin\n");

  if (pciid == holy_CS5536_PCIID)
    {
      holy_uint64_t basereg;

      basereg = holy_cs5536_read_msr (dev, holy_CS5536_MSR_USB_EHCI_BASE);
      if (!(basereg & holy_CS5536_MSR_USB_BASE_MEMORY_ENABLE))
	{
	  /* Shouldn't happen.  */
	  holy_dprintf ("ehci", "No EHCI address is assigned\n");
	  return 0;
	}
      base = (basereg & holy_CS5536_MSR_USB_BASE_ADDR_MASK);
      basereg |= holy_CS5536_MSR_USB_BASE_BUS_MASTER;
      basereg &= ~holy_CS5536_MSR_USB_BASE_PME_ENABLED;
      basereg &= ~holy_CS5536_MSR_USB_BASE_PME_STATUS;
      basereg &= ~holy_CS5536_MSR_USB_BASE_SMI_ENABLE;
      holy_cs5536_write_msr (dev, holy_CS5536_MSR_USB_EHCI_BASE, basereg);
    }
  else
    {
      holy_pci_address_t addr;
      addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
      class_code = holy_pci_read (addr) >> 8;
      interf = class_code & 0xFF;
      subclass = (class_code >> 8) & 0xFF;
      class = class_code >> 16;

      /* If this is not an EHCI controller, just return.  */
      if (class != 0x0c || subclass != 0x03 || interf != 0x20)
	return 0;

      holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: class OK\n");

      /* Check Serial Bus Release Number */
      addr = holy_pci_make_address (dev, holy_EHCI_PCI_SBRN_REG);
      release = holy_pci_read_byte (addr);
      if (release != 0x20)
	{
	  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: Wrong SBRN: %0x\n",
			release);
	  return 0;
	}
      holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: bus rev. num. OK\n");
  
      /* Determine EHCI EHCC registers base address.  */
      addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0);
      base = holy_pci_read (addr);
      addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG1);
      base_h = holy_pci_read (addr);
      /* Stop if registers are mapped above 4G - holy does not currently
       * work with registers mapped above 4G */
      if (((base & holy_PCI_ADDR_MEM_TYPE_MASK) != holy_PCI_ADDR_MEM_TYPE_32)
	  && (base_h != 0))
	{
	  holy_dprintf ("ehci",
			"EHCI holy_ehci_pci_iter: registers above 4G are not supported\n");
	  return 0;
	}
      base &= holy_PCI_ADDR_MEM_MASK;
      if (!base)
	{
	  holy_dprintf ("ehci",
			"EHCI: EHCI is not mapped\n");
	  return 0;
	}

      /* Set bus master - needed for coreboot, VMware, broken BIOSes etc. */
      addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
      holy_pci_write_word(addr,
			  holy_PCI_COMMAND_MEM_ENABLED
			  | holy_PCI_COMMAND_BUS_MASTER
			  | holy_pci_read_word(addr));
      
      holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: 32-bit EHCI OK\n");
    }

  /* Allocate memory for the controller and fill basic values. */
  e = holy_zalloc (sizeof (*e));
  if (!e)
    return 1;
  e->framelist_chunk = NULL;
  e->td_chunk = NULL;
  e->qh_chunk = NULL;
  e->iobase_ehcc = holy_pci_device_map_range (dev,
					      (base & holy_EHCI_ADDR_MEM_MASK),
					      0x100);

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: iobase of EHCC: %08x\n",
		(base & holy_EHCI_ADDR_MEM_MASK));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: CAPLEN: %02x\n",
		holy_ehci_ehcc_read8 (e, holy_EHCI_EHCC_CAPLEN));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: VERSION: %04x\n",
		holy_ehci_ehcc_read16 (e, holy_EHCI_EHCC_VERSION));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: SPARAMS: %08x\n",
		holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_SPARAMS));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: CPARAMS: %08x\n",
		holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_CPARAMS));

  /* Determine base address of EHCI operational registers */
  caplen = holy_ehci_ehcc_read8 (e, holy_EHCI_EHCC_CAPLEN);
#ifndef holy_HAVE_UNALIGNED_ACCESS
  if (caplen & (sizeof (holy_uint32_t) - 1))
    {
      holy_dprintf ("ehci", "Unaligned caplen\n");
      return 0;
    }
  e->iobase = ((volatile holy_uint32_t *) e->iobase_ehcc
	       + (caplen / sizeof (holy_uint32_t)));
#else  
  e->iobase = (volatile holy_uint32_t *)
    ((holy_uint8_t *) e->iobase_ehcc + caplen);
#endif

  holy_dprintf ("ehci",
		"EHCI holy_ehci_pci_iter: iobase of oper. regs: %08x\n",
		(base & holy_EHCI_ADDR_MEM_MASK) + caplen);
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: COMMAND: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: STATUS: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_STATUS));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: INTERRUPT: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_INTERRUPT));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: FRAME_INDEX: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_FRAME_INDEX));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: FL_BASE: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_FL_BASE));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: CUR_AL_ADDR: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_CUR_AL_ADDR));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: CONFIG_FLAG: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_CONFIG_FLAG));

  /* Is there EECP ? */
  eecp_offset = (holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_CPARAMS)
		 & holy_EHCI_EECP_MASK) >> holy_EHCI_EECP_SHIFT;

  /* Check format of data structures requested by EHCI */
  /* XXX: In fact it is not used at any place, it is prepared for future
   * This implementation uses 32-bits pointers only */
  e->flag64 = ((holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_CPARAMS)
		& holy_EHCI_CPARAMS_64BIT) != 0);

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: flag64=%d\n", e->flag64);

  /* Reserve a page for the frame list - it is accurate for max.
   * possible size of framelist. But currently it is not used. */
  e->framelist_chunk = holy_memalign_dma32 (4096, 4096);
  if (!e->framelist_chunk)
    goto fail;
  e->framelist_virt = holy_dma_get_virt (e->framelist_chunk);
  e->framelist_phys = holy_dma_get_phys (e->framelist_chunk);
  holy_memset ((void *) e->framelist_virt, 0, 4096);

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: framelist mem=%p. OK\n",
		e->framelist_virt);

  /* Allocate memory for the QHs and register it in "e".  */
  e->qh_chunk = holy_memalign_dma32 (4096,
				     sizeof (struct holy_ehci_qh) *
				     holy_EHCI_N_QH);
  if (!e->qh_chunk)
    goto fail;
  e->qh_virt = (holy_ehci_qh_t) holy_dma_get_virt (e->qh_chunk);
  e->qh_phys = holy_dma_get_phys (e->qh_chunk);
  holy_memset ((void *) e->qh_virt, 0,
	       sizeof (struct holy_ehci_qh) * holy_EHCI_N_QH);

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: QH mem=%p. OK\n",
		e->qh_virt);

  /* Allocate memory for the TDs and register it in "e".  */
  e->td_chunk = holy_memalign_dma32 (4096,
				     sizeof (struct holy_ehci_td) *
				     holy_EHCI_N_TD);
  if (!e->td_chunk)
    goto fail;
  e->td_virt = (holy_ehci_td_t) holy_dma_get_virt (e->td_chunk);
  e->td_phys = holy_dma_get_phys (e->td_chunk);
  holy_memset ((void *) e->td_virt, 0,
	       sizeof (struct holy_ehci_td) * holy_EHCI_N_TD);

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: TD mem=%p. OK\n",
		e->td_virt);

  /* Setup all frame list pointers. Since no isochronous transfers
     are supported, they all point to the (same!) queue
     head with index 0. */
  fp = holy_cpu_to_le32 ((e->qh_phys & holy_EHCI_POINTER_MASK)
			 | holy_EHCI_HPTR_TYPE_QH);
  for (i = 0; i < holy_EHCI_N_FRAMELIST; i++)
    e->framelist_virt[i] = fp;
  /* Prepare chain of all TDs and set Terminate in all TDs */
  for (i = 0; i < (holy_EHCI_N_TD - 1); i++)
    {
      e->td_virt[i].link_td = e->td_phys + (i + 1) * sizeof (struct holy_ehci_td);
      e->td_virt[i].next_td = holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
      e->td_virt[i].alt_next_td = holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
    }
  e->td_virt[holy_EHCI_N_TD - 1].next_td =
    holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
  e->td_virt[holy_EHCI_N_TD - 1].alt_next_td =
    holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
  e->tdfree_virt = e->td_virt;
  /* Set Terminate in first QH, which is used in framelist */
  e->qh_virt[0].qh_hptr = holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE | holy_EHCI_HPTR_TYPE_QH);
  e->qh_virt[0].td_overlay.next_td = holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
  e->qh_virt[0].td_overlay.alt_next_td =
    holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
  /* Also set Halted bit in token */
  e->qh_virt[0].td_overlay.token = holy_cpu_to_le32_compile_time (holy_EHCI_STATUS_HALTED);
  /* Set the H bit in first QH used for AL */
  e->qh_virt[1].ep_char = holy_cpu_to_le32_compile_time (holy_EHCI_H);
  /* Set Terminate into TD in rest of QHs and set horizontal link
   * pointer to itself - these QHs will be used for asynchronous
   * schedule and they should have valid value in horiz. link */
  for (i = 1; i < holy_EHCI_N_QH; i++)
    {
      e->qh_virt[i].qh_hptr =
	holy_cpu_to_le32 ((holy_dma_virt2phys (&e->qh_virt[i],
						e->qh_chunk) &
			   holy_EHCI_POINTER_MASK) | holy_EHCI_HPTR_TYPE_QH);
      e->qh_virt[i].td_overlay.next_td =
	holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
      e->qh_virt[i].td_overlay.alt_next_td =
	holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
      /* Also set Halted bit in token */
      e->qh_virt[i].td_overlay.token =
	holy_cpu_to_le32_compile_time (holy_EHCI_STATUS_HALTED);
    }

  /* Note: QH 0 and QH 1 are reserved and must not be used anywhere.
   * QH 0 is used as empty QH for framelist
   * QH 1 is used as starting empty QH for asynchronous schedule
   * QH 1 must exist at any time because at least one QH linked to
   * itself must exist in asynchronous schedule
   * QH 1 has the H flag set to one */

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: QH/TD init. OK\n");

  /* Determine and change ownership. */
  /* EECP offset valid in HCCPARAMS */
  /* Ownership can be changed via EECP only */
  if (pciid != holy_CS5536_PCIID && eecp_offset >= 0x40)
    {
      holy_pci_address_t pciaddr_eecp;
      pciaddr_eecp = holy_pci_make_address (dev, eecp_offset);

      usblegsup = holy_pci_read (pciaddr_eecp);
      if (usblegsup & holy_EHCI_BIOS_OWNED)
	{
	  holy_boot_time ("Taking ownership of EHCI controller");
	  holy_dprintf ("ehci",
			"EHCI holy_ehci_pci_iter: EHCI owned by: BIOS\n");
	  /* Ownership change - set OS_OWNED bit */
	  holy_pci_write (pciaddr_eecp, usblegsup | holy_EHCI_OS_OWNED);
	  /* Ensure PCI register is written */
	  holy_pci_read (pciaddr_eecp);

	  /* Wait for finish of ownership change, EHCI specification
	   * doesn't say how long it can take... */
	  maxtime = holy_get_time_ms () + 1000;
	  while ((holy_pci_read (pciaddr_eecp) & holy_EHCI_BIOS_OWNED)
		 && (holy_get_time_ms () < maxtime));
	  if (holy_pci_read (pciaddr_eecp) & holy_EHCI_BIOS_OWNED)
	    {
	      holy_dprintf ("ehci",
			    "EHCI holy_ehci_pci_iter: EHCI change ownership timeout");
	      /* Change ownership in "hard way" - reset BIOS ownership */
	      holy_pci_write (pciaddr_eecp, holy_EHCI_OS_OWNED);
	      /* Ensure PCI register is written */
	      holy_pci_read (pciaddr_eecp);
	    }
	}
      else if (usblegsup & holy_EHCI_OS_OWNED)
	/* XXX: What to do in this case - nothing ? Can it happen ? */
	holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: EHCI owned by: OS\n");
      else
	{
	  holy_dprintf ("ehci",
			"EHCI holy_ehci_pci_iter: EHCI owned by: NONE\n");
	  /* XXX: What to do in this case ? Can it happen ?
	   * Is code below correct ? */
	  /* Ownership change - set OS_OWNED bit */
	  holy_pci_write (pciaddr_eecp, holy_EHCI_OS_OWNED);
	  /* Ensure PCI register is written */
	  holy_pci_read (pciaddr_eecp);
	}

    /* Disable SMI, just to be sure.  */
    pciaddr_eecp = holy_pci_make_address (dev, eecp_offset + 4);
    holy_pci_write (pciaddr_eecp, 0);
    /* Ensure PCI register is written */
    holy_pci_read (pciaddr_eecp);

    }

  holy_dprintf ("ehci", "inithw: EHCI holy_ehci_pci_iter: ownership OK\n");

  /* Now we can setup EHCI (maybe...) */

  /* Check if EHCI is halted and halt it if not */
  if (holy_ehci_halt (e) != holy_USB_ERR_NONE)
    {
      holy_error (holy_ERR_TIMEOUT,
		  "EHCI holy_ehci_pci_iter: EHCI halt timeout");
      goto fail;
    }

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: halted OK\n");

  /* Reset EHCI */
  if (holy_ehci_reset (e) != holy_USB_ERR_NONE)
    {
      holy_error (holy_ERR_TIMEOUT,
		  "EHCI holy_ehci_pci_iter: EHCI reset timeout");
      goto fail;
    }

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: reset OK\n");

  /* Setup list address registers */
  holy_ehci_oper_write32 (e, holy_EHCI_FL_BASE, e->framelist_phys);
  holy_ehci_oper_write32 (e, holy_EHCI_CUR_AL_ADDR,
			  holy_dma_virt2phys (&e->qh_virt[1],
					       e->qh_chunk));

  /* Set ownership of root hub ports to EHCI */
  holy_ehci_oper_write32 (e, holy_EHCI_CONFIG_FLAG, holy_EHCI_CF_EHCI_OWNER);

  /* Enable both lists */
  holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
			  holy_EHCI_CMD_AS_ENABL
			  | holy_EHCI_CMD_PS_ENABL
			  | holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));

  /* Now should be possible to power-up and enumerate ports etc. */
  if ((holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_SPARAMS)
       & holy_EHCI_SPARAMS_PPC) != 0)
    {				/* EHCI has port powering control */
      /* Power on all ports */
      n_ports = holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_SPARAMS)
	& holy_EHCI_SPARAMS_N_PORTS;
      for (i = 0; i < (int) n_ports; i++)
	holy_ehci_oper_write32 (e, holy_EHCI_PORT_STAT_CMD + i * 4,
				holy_EHCI_PORT_POWER
				| holy_ehci_oper_read32 (e,
							 holy_EHCI_PORT_STAT_CMD
							 + i * 4));
    }

  /* Ensure all commands are written */
  holy_ehci_oper_read32 (e, holy_EHCI_COMMAND);

  /* Enable EHCI */
  holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
			  holy_EHCI_CMD_RUNSTOP
			  | holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));

  /* Ensure command is written */
  holy_ehci_oper_read32 (e, holy_EHCI_COMMAND);

  /* Link to ehci now that initialisation is successful.  */
  e->next = ehci;
  ehci = e;

  sync_all_caches (e);

  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: OK at all\n");

  holy_dprintf ("ehci",
		"EHCI holy_ehci_pci_iter: iobase of oper. regs: %08x\n",
		(base & holy_EHCI_ADDR_MEM_MASK));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: COMMAND: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: STATUS: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_STATUS));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: INTERRUPT: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_INTERRUPT));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: FRAME_INDEX: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_FRAME_INDEX));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: FL_BASE: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_FL_BASE));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: CUR_AL_ADDR: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_CUR_AL_ADDR));
  holy_dprintf ("ehci", "EHCI holy_ehci_pci_iter: CONFIG_FLAG: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_CONFIG_FLAG));

  return 0;

fail:
  if (e)
    {
      if (e->td_chunk)
	holy_dma_free ((void *) e->td_chunk);
      if (e->qh_chunk)
	holy_dma_free ((void *) e->qh_chunk);
      if (e->framelist_chunk)
	holy_dma_free (e->framelist_chunk);
    }
  holy_free (e);

  return 0;
}

static int
holy_ehci_iterate (holy_usb_controller_iterate_hook_t hook, void *hook_data)
{
  struct holy_ehci *e;
  struct holy_usb_controller dev;

  for (e = ehci; e; e = e->next)
    {
      dev.data = e;
      if (hook (&dev, hook_data))
	return 1;
    }

  return 0;
}

static void
holy_ehci_setup_qh (holy_ehci_qh_t qh, holy_usb_transfer_t transfer)
{
  holy_uint32_t ep_char = 0;
  holy_uint32_t ep_cap = 0;

  /* Note: Another part of code is responsible to this QH is
   * Halted ! But it can be linked in AL, so we cannot erase or
   * change qh_hptr ! */
  /* We will not change any TD field because they should/must be
   * in safe state from previous use. */

  /* EP characteristic setup */
  /* Currently not used NAK counter (RL=0),
   * C bit set if EP is not HIGH speed and is control,
   * Max Packet Length is taken from transfer structure,
   * H bit = 0 (because QH[1] has this bit set),
   * DTC bit set to 1 because we are using our own toggle bit control,
   * SPEED is selected according to value from transfer structure,
   * EP number is taken from transfer structure
   * "I" bit must not be set,
   * Device Address is taken from transfer structure
   * */
  if ((transfer->dev->speed != holy_USB_SPEED_HIGH)
      && (transfer->type == holy_USB_TRANSACTION_TYPE_CONTROL))
    ep_char |= holy_EHCI_C;
  ep_char |= (transfer->max << holy_EHCI_MAXPLEN_OFF)
    & holy_EHCI_MAXPLEN_MASK;
  ep_char |= holy_EHCI_DTC;
  switch (transfer->dev->speed)
    {
    case holy_USB_SPEED_LOW:
      ep_char |= holy_EHCI_SPEED_LOW;
      break;
    case holy_USB_SPEED_FULL:
      ep_char |= holy_EHCI_SPEED_FULL;
      break;
    case holy_USB_SPEED_HIGH:
    default:
      ep_char |= holy_EHCI_SPEED_HIGH;
      /* XXX: How we will handle unknown value of speed? */
    }
  ep_char |= (transfer->endpoint << holy_EHCI_EP_NUM_OFF)
    & holy_EHCI_EP_NUM_MASK;
  ep_char |= transfer->devaddr & holy_EHCI_DEVADDR_MASK;
  qh->ep_char = holy_cpu_to_le32 (ep_char);
  /* EP capabilities setup */
  /* MULT field - we try to use max. number
   * PortNumber - included now in device structure referenced
   *              inside transfer structure
   * HubAddress - included now in device structure referenced
   *              inside transfer structure
   * SplitCompletionMask - AFAIK it is ignored in asynchronous list,
   * InterruptScheduleMask - AFAIK it should be zero in async. list */
  ep_cap |= holy_EHCI_MULT_THREE;
  ep_cap |= (transfer->dev->split_hubport << holy_EHCI_DEVPORT_OFF)
    & holy_EHCI_DEVPORT_MASK;
  ep_cap |= (transfer->dev->split_hubaddr << holy_EHCI_HUBADDR_OFF)
    & holy_EHCI_HUBADDR_MASK;
  if (transfer->dev->speed == holy_USB_SPEED_LOW
      && transfer->type != holy_USB_TRANSACTION_TYPE_CONTROL)
  {
    ep_cap |= (1<<0) << holy_EHCI_SMASK_OFF;
    ep_cap |= (7<<2) << holy_EHCI_CMASK_OFF;
  }
  qh->ep_cap = holy_cpu_to_le32 (ep_cap);

  holy_dprintf ("ehci", "setup_qh: qh=%p, not changed: qh_hptr=%08x\n",
		qh, holy_le_to_cpu32 (qh->qh_hptr));
  holy_dprintf ("ehci", "setup_qh: ep_char=%08x, ep_cap=%08x\n",
		ep_char, ep_cap);
  holy_dprintf ("ehci", "setup_qh: end\n");
  holy_dprintf ("ehci", "setup_qh: not changed: td_current=%08x\n",
		holy_le_to_cpu32 (qh->td_current));
  holy_dprintf ("ehci", "setup_qh: not changed: next_td=%08x\n",
		holy_le_to_cpu32 (qh->td_overlay.next_td));
  holy_dprintf ("ehci", "setup_qh: not changed: alt_next_td=%08x\n",
		holy_le_to_cpu32 (qh->td_overlay.alt_next_td));
  holy_dprintf ("ehci", "setup_qh: not changed: token=%08x\n",
		holy_le_to_cpu32 (qh->td_overlay.token));
}

static holy_ehci_qh_t
holy_ehci_find_qh (struct holy_ehci *e, holy_usb_transfer_t transfer)
{
  holy_uint32_t target, mask;
  int i;
  holy_ehci_qh_t qh = e->qh_virt;
  holy_ehci_qh_t head;
  holy_uint32_t qh_phys;
  holy_uint32_t qh_terminate =
    holy_EHCI_TERMINATE | holy_EHCI_HPTR_TYPE_QH;
  holy_ehci_qh_t qh_iter;

  /* Prepare part of EP Characteristic to find existing QH */
  target = ((transfer->endpoint << holy_EHCI_EP_NUM_OFF) |
	    transfer->devaddr) & holy_EHCI_TARGET_MASK;
  target = holy_cpu_to_le32 (target);
  mask = holy_cpu_to_le32_compile_time (holy_EHCI_TARGET_MASK);

  /* low speed interrupt transfers are linked to the periodic */
  /* schedule, everything else to the asynchronous schedule */
  if (transfer->dev->speed == holy_USB_SPEED_LOW
      && transfer->type != holy_USB_TRANSACTION_TYPE_CONTROL)
    head = &qh[0];
  else
    head = &qh[1];

  /* First try to find existing QH with proper target in proper list */
  qh_phys = holy_le_to_cpu32( head->qh_hptr );
  if (qh_phys != qh_terminate)
    qh_iter = holy_dma_phys2virt ( qh_phys & holy_EHCI_QHTDPTR_MASK,
      e->qh_chunk );
  else
    qh_iter = NULL;

  for (
    i = 0;
    (qh_phys != qh_terminate) && (qh_iter != NULL) &&
    (qh_iter != head) && (i < holy_EHCI_N_QH);
    i++ )
    {
      if (target == (qh_iter->ep_char & mask))
	{		
	  /* Found proper existing (and linked) QH, do setup of QH */
	  holy_dprintf ("ehci", "find_qh: found, QH=%p\n", qh_iter);
	  holy_ehci_setup_qh (qh_iter, transfer);
	  sync_all_caches (e);
	  return qh_iter;
	}

      qh_phys = holy_le_to_cpu32( qh_iter->qh_hptr );
      if (qh_phys != qh_terminate)
        qh_iter = holy_dma_phys2virt ( qh_phys & holy_EHCI_QHTDPTR_MASK,
	  e->qh_chunk );
      else
        qh_iter = NULL;
    }

  /* variable "i" should be never equal to holy_EHCI_N_QH here */
  if (i >= holy_EHCI_N_QH)
    { /* Something very bad happened in QH list(s) ! */
      holy_dprintf ("ehci", "find_qh: Mismatch in QH list! head=%p\n",
        head);
    }

  /* QH with target_addr does not exist, we have to find and add it */
  for (i = 2; i < holy_EHCI_N_QH; i++) /* We ignore zero and first QH */
    {
      if (!qh[i].ep_char)
	break;	             /* Found first not-allocated QH, finish */
    }

  /* Have we any free QH in array ? */
  if (i >= holy_EHCI_N_QH)	/* No. */
    {
      holy_dprintf ("ehci", "find_qh: end - no free QH\n");
      return NULL;
    }
  holy_dprintf ("ehci", "find_qh: new, i=%d, QH=%p\n",
		i, &qh[i]);
  /* Currently we simply take next (current) QH in array, no allocation
   * function is used. It should be no problem until we will need to
   * de-allocate QHs of unplugged devices. */
  /* We should preset new QH and link it into AL */
  holy_ehci_setup_qh (&qh[i], transfer);

  /* Linking - this new (last) QH will copy the QH from the head QH */
  qh[i].qh_hptr = head->qh_hptr;
  /* Linking - the head QH will point to this new QH */
  head->qh_hptr = holy_cpu_to_le32 (holy_EHCI_HPTR_TYPE_QH
                                    | holy_dma_virt2phys (&qh[i],
                                                          e->qh_chunk));

  return &qh[i];
}

static holy_ehci_td_t
holy_ehci_alloc_td (struct holy_ehci *e)
{
  holy_ehci_td_t ret;

  /* Check if there is a Transfer Descriptor available.  */
  if (!e->tdfree_virt)
    {
      holy_dprintf ("ehci", "alloc_td: end - no free TD\n");
      return NULL;
    }

  ret = e->tdfree_virt;		/* Take current free TD */
  /* Advance to next free TD in chain */
  if (ret->link_td)
    e->tdfree_virt = holy_dma_phys2virt (ret->link_td, e->td_chunk);
  else
    e->tdfree_virt = NULL;
  ret->link_td = 0;		/* Reset link_td in allocated TD */
  return ret;
}

static void
holy_ehci_free_td (struct holy_ehci *e, holy_ehci_td_t td)
{
  /* Chain new free TD & rest */
  if (e->tdfree_virt)
    td->link_td = holy_dma_virt2phys (e->tdfree_virt, e->td_chunk);
  else
    td->link_td = 0;
  e->tdfree_virt = td;		/* Change address of first free TD */
}

static void
holy_ehci_free_tds (struct holy_ehci *e, holy_ehci_td_t td,
		    holy_usb_transfer_t transfer, holy_size_t * actual)
{
  int i;			/* Index of TD in transfer */
  holy_uint32_t token, to_transfer;

  /* Note: Another part of code is responsible to this QH is
   * INACTIVE ! */
  *actual = 0;

  /* Free the TDs in this queue and set last_trans.  */
  for (i = 0; td; i++)
    {
      holy_ehci_td_t tdprev;

      token = holy_le_to_cpu32 (td->token);
      to_transfer = (token & holy_EHCI_TOTAL_MASK) >> holy_EHCI_TOTAL_OFF;

      /* Check state of TD - if it did not transfer
       * whole data then set last_trans - it should be last executed TD
       * in case when something went wrong. */
      if (transfer && (td->size != to_transfer))
	transfer->last_trans = i;

      *actual += td->size - to_transfer;

      /* Unlink the TD */
      tdprev = td;
      if (td->link_td)
	td = holy_dma_phys2virt (td->link_td, e->td_chunk);
      else
	td = NULL;

      /* Free the TD.  */
      holy_ehci_free_td (e, tdprev);
    }

  /* Check if last_trans was set. If not and something was
   * transferred (it should be all data in this case), set it
   * to index of last TD, i.e. i-1 */
  if (transfer && (transfer->last_trans < 0) && (*actual != 0))
    transfer->last_trans = i - 1;

  /* XXX: Fix it: last_trans may be set to bad index.
   * Probably we should test more error flags to distinguish
   * if TD was at least partialy executed or not at all.
   * Generaly, we still could have problem with toggling because
   * EHCI can probably split transactions into smaller parts then
   * we defined in transaction even if we did not exceed MaxFrame
   * length - it probably could happen at the end of microframe (?)
   * and if the buffer is crossing page boundary (?). */
}

static holy_ehci_td_t
holy_ehci_transaction (struct holy_ehci *e,
		       holy_transfer_type_t type,
		       unsigned int toggle, holy_size_t size,
		       holy_uint32_t data, holy_ehci_td_t td_alt)
{
  holy_ehci_td_t td;
  holy_uint32_t token;
  holy_uint32_t bufadr;
  int i;

  /* Test of transfer size, it can be:
   * <= holy_EHCI_MAXBUFLEN if data aligned to page boundary
   * <= holy_EHCI_MAXBUFLEN - holy_EHCI_BUFPAGELEN if not aligned
   *    (worst case)
   */
  if ((((data % holy_EHCI_BUFPAGELEN) == 0)
       && (size > holy_EHCI_MAXBUFLEN))
      ||
      (((data % holy_EHCI_BUFPAGELEN) != 0)
       && (size > (holy_EHCI_MAXBUFLEN - holy_EHCI_BUFPAGELEN))))
    {
      holy_error (holy_ERR_OUT_OF_MEMORY,
		  "too long data buffer for EHCI transaction");
      return 0;
    }

  /* Grab a free Transfer Descriptor and initialize it.  */
  td = holy_ehci_alloc_td (e);
  if (!td)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY,
		  "no transfer descriptors available for EHCI transfer");
      return 0;
    }

  holy_dprintf ("ehci",
		"transaction: type=%d, toggle=%d, size=%lu data=0x%x td=%p\n",
		type, toggle, (unsigned long) size, data, td);

  /* Fill whole TD by zeros */
  holy_memset ((void *) td, 0, sizeof (struct holy_ehci_td));

  /* Don't point to any TD yet, just terminate.  */
  td->next_td = holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
  /* Set alternate pointer. When short packet occurs, alternate TD
   * will not be really fetched because it is not active. But don't
   * forget, EHCI will try to fetch alternate TD every scan of AL
   * until QH is halted. */
  td->alt_next_td = holy_cpu_to_le32 (holy_dma_virt2phys (td_alt,
							   e->td_chunk));
  /* token:
   * TOGGLE - according to toggle
   * TOTAL SIZE = size
   * Interrupt On Complete = FALSE, we don't need IRQ
   * Current Page = 0
   * Error Counter = max. value = 3
   * PID Code - according to type
   * STATUS:
   *  ACTIVE bit should be set to one
   *  SPLIT TRANS. STATE bit should be zero. It is ignored
   *   in HIGH speed transaction, and should be zero for LOW/FULL
   *   speed to indicate state Do Split Transaction */
  token = toggle ? holy_EHCI_TOGGLE : 0;
  token |= (size << holy_EHCI_TOTAL_OFF) & holy_EHCI_TOTAL_MASK;
  token |= holy_EHCI_CERR_3;
  switch (type)
    {
    case holy_USB_TRANSFER_TYPE_IN:
      token |= holy_EHCI_PIDCODE_IN;
      break;
    case holy_USB_TRANSFER_TYPE_OUT:
      token |= holy_EHCI_PIDCODE_OUT;
      break;
    case holy_USB_TRANSFER_TYPE_SETUP:
      token |= holy_EHCI_PIDCODE_SETUP;
      break;
    default:			/* XXX: Should not happen, but what to do if it does ? */
      break;
    }
  token |= holy_EHCI_STATUS_ACTIVE;
  td->token = holy_cpu_to_le32 (token);

  /* Fill buffer pointers according to size */
  bufadr = data;
  td->buffer_page[0] = holy_cpu_to_le32 (bufadr);
  bufadr = ((bufadr / holy_EHCI_BUFPAGELEN) + 1) * holy_EHCI_BUFPAGELEN;
  for (i = 1; ((bufadr - data) < size) && (i < holy_EHCI_TD_BUF_PAGES); i++)
    {
      td->buffer_page[i] = holy_cpu_to_le32 (bufadr & holy_EHCI_BUFPTR_MASK);
      bufadr = ((bufadr / holy_EHCI_BUFPAGELEN) + 1) * holy_EHCI_BUFPAGELEN;
    }

  /* Remember data size for future use... */
  td->size = (holy_uint32_t) size;

  holy_dprintf ("ehci", "td=%p\n", td);
  holy_dprintf ("ehci", "HW: next_td=%08x, alt_next_td=%08x\n",
		holy_le_to_cpu32 (td->next_td),
		holy_le_to_cpu32 (td->alt_next_td));
  holy_dprintf ("ehci", "HW: token=%08x, buffer[0]=%08x\n",
		holy_le_to_cpu32 (td->token),
		holy_le_to_cpu32 (td->buffer_page[0]));
  holy_dprintf ("ehci", "HW: buffer[1]=%08x, buffer[2]=%08x\n",
		holy_le_to_cpu32 (td->buffer_page[1]),
		holy_le_to_cpu32 (td->buffer_page[2]));
  holy_dprintf ("ehci", "HW: buffer[3]=%08x, buffer[4]=%08x\n",
		holy_le_to_cpu32 (td->buffer_page[3]),
		holy_le_to_cpu32 (td->buffer_page[4]));
  holy_dprintf ("ehci", "link_td=%08x, size=%08x\n",
		td->link_td, td->size);

  return td;
}

struct holy_ehci_transfer_controller_data
{
  holy_ehci_qh_t qh_virt;
  holy_ehci_td_t td_first_virt;
  holy_ehci_td_t td_alt_virt;
  holy_ehci_td_t td_last_virt;
  holy_uint32_t td_last_phys;
};

static holy_usb_err_t
holy_ehci_setup_transfer (holy_usb_controller_t dev,
			  holy_usb_transfer_t transfer)
{
  struct holy_ehci *e = (struct holy_ehci *) dev->data;
  holy_ehci_td_t td = NULL;
  holy_ehci_td_t td_prev = NULL;
  int i;
  struct holy_ehci_transfer_controller_data *cdata;
  holy_uint32_t status;

  sync_all_caches (e);

  /* Check if EHCI is running and AL is enabled */
  status = holy_ehci_oper_read32 (e, holy_EHCI_STATUS);
  if ((status & holy_EHCI_ST_HC_HALTED) != 0)
    /* XXX: Fix it: Currently we don't do anything to restart EHCI */
    {
      holy_dprintf ("ehci", "setup_transfer: halted, status = 0x%x\n",
		    status);
      return holy_USB_ERR_INTERNAL;
    }
  status = holy_ehci_oper_read32 (e, holy_EHCI_STATUS);
  if ((status
       & (holy_EHCI_ST_AS_STATUS | holy_EHCI_ST_PS_STATUS)) == 0)
    /* XXX: Fix it: Currently we don't do anything to restart EHCI */
    {
      holy_dprintf ("ehci", "setup_transfer: no AS/PS, status = 0x%x\n",
		    status);
      return holy_USB_ERR_INTERNAL;
    }

  /* Allocate memory for controller transfer data.  */
  cdata = holy_malloc (sizeof (*cdata));
  if (!cdata)
    return holy_USB_ERR_INTERNAL;
  cdata->td_first_virt = NULL;

  /* Allocate a queue head for the transfer queue.  */
  cdata->qh_virt = holy_ehci_find_qh (e, transfer);
  if (!cdata->qh_virt)
    {
      holy_dprintf ("ehci", "setup_transfer: no QH\n");
      holy_free (cdata);
      return holy_USB_ERR_INTERNAL;
    }

  /* To detect short packet we need some additional "alternate" TD,
   * allocate it first. */
  cdata->td_alt_virt = holy_ehci_alloc_td (e);
  if (!cdata->td_alt_virt)
    {
      holy_dprintf ("ehci", "setup_transfer: no TDs\n");
      holy_free (cdata);
      return holy_USB_ERR_INTERNAL;
    }
  /* Fill whole alternate TD by zeros (= inactive) and set
   * Terminate bits and Halt bit */
  holy_memset ((void *) cdata->td_alt_virt, 0, sizeof (struct holy_ehci_td));
  cdata->td_alt_virt->next_td = holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
  cdata->td_alt_virt->alt_next_td = holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
  cdata->td_alt_virt->token = holy_cpu_to_le32_compile_time (holy_EHCI_STATUS_HALTED);

  /* Allocate appropriate number of TDs and set */
  for (i = 0; i < transfer->transcnt; i++)
    {
      holy_usb_transaction_t tr = &transfer->transactions[i];

      td = holy_ehci_transaction (e, tr->pid, tr->toggle, tr->size,
				  tr->data, cdata->td_alt_virt);

      if (!td)			/* de-allocate and free all */
	{
	  holy_size_t actual = 0;

	  if (cdata->td_first_virt)
	    holy_ehci_free_tds (e, cdata->td_first_virt, NULL, &actual);

	  holy_free (cdata);
	  holy_dprintf ("ehci", "setup_transfer: no TD\n");
	  return holy_USB_ERR_INTERNAL;
	}

      /* Register new TD in cdata or previous TD */
      if (!cdata->td_first_virt)
	cdata->td_first_virt = td;
      else
	{
	  td_prev->link_td = holy_dma_virt2phys (td, e->td_chunk);
	  td_prev->next_td =
	    holy_cpu_to_le32 (holy_dma_virt2phys (td, e->td_chunk));
	}
      td_prev = td;
    }

  /* Remember last TD */
  cdata->td_last_virt = td;
  cdata->td_last_phys = holy_dma_virt2phys (td, e->td_chunk);
  /* Last TD should not have set alternate TD */
  cdata->td_last_virt->alt_next_td = holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);

  holy_dprintf ("ehci", "setup_transfer: cdata=%p, qh=%p\n",
		cdata,cdata->qh_virt);
  holy_dprintf ("ehci", "setup_transfer: td_first=%p, td_alt=%p\n",
		cdata->td_first_virt,
		cdata->td_alt_virt);
  holy_dprintf ("ehci", "setup_transfer: td_last=%p\n",
		cdata->td_last_virt);

  /* Start transfer: */
  /* Unlink possible alternate pointer in QH */
  cdata->qh_virt->td_overlay.alt_next_td =
    holy_cpu_to_le32_compile_time (holy_EHCI_TERMINATE);
  /* Link new TDs with QH via next_td */
  cdata->qh_virt->td_overlay.next_td =
    holy_cpu_to_le32 (holy_dma_virt2phys
		      (cdata->td_first_virt, e->td_chunk));
  /* Reset Active and Halted bits in QH to activate Advance Queue,
   * i.e. reset token */
  cdata->qh_virt->td_overlay.token = holy_cpu_to_le32_compile_time (0);

  sync_all_caches (e);

  /* Finito */
  transfer->controller_data = cdata;

  return holy_USB_ERR_NONE;
}

/* This function expects QH is not active.
 * Function set Halt bit in QH TD overlay and possibly prints
 * necessary debug information. */
static void
holy_ehci_pre_finish_transfer (holy_usb_transfer_t transfer)
{
  struct holy_ehci_transfer_controller_data *cdata =
    transfer->controller_data;

  /* Collect debug data here if necessary */

  /* Set Halt bit in not active QH. AL will not attempt to do
   * Advance Queue on QH with Halt bit set, i.e., we can then
   * safely manipulate with QH TD part. */
  cdata->qh_virt->td_overlay.token = (cdata->qh_virt->td_overlay.token
				      |
				      holy_cpu_to_le32_compile_time
				      (holy_EHCI_STATUS_HALTED)) &
    holy_cpu_to_le32_compile_time (~holy_EHCI_STATUS_ACTIVE);

  /* Print debug data here if necessary */

}

static holy_usb_err_t
holy_ehci_parse_notrun (holy_usb_controller_t dev,
			holy_usb_transfer_t transfer, holy_size_t * actual)
{
  struct holy_ehci *e = dev->data;
  struct holy_ehci_transfer_controller_data *cdata =
    transfer->controller_data;

  holy_dprintf ("ehci", "parse_notrun: info\n");

  /* QH can be in any state in this case. */
  /* But EHCI or AL is not running, so QH is surely not active
   * even if it has Active bit set... */
  holy_ehci_pre_finish_transfer (transfer);
  holy_ehci_free_tds (e, cdata->td_first_virt, transfer, actual);
  holy_ehci_free_td (e, cdata->td_alt_virt);
  holy_free (cdata);

  sync_all_caches (e);

  /* Additionally, do something with EHCI to make it running (what?) */
  /* Try enable EHCI and AL */
  holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
			  holy_EHCI_CMD_RUNSTOP | holy_EHCI_CMD_AS_ENABL
			  | holy_EHCI_CMD_PS_ENABL
			  | holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));
  /* Ensure command is written */
  holy_ehci_oper_read32 (e, holy_EHCI_COMMAND);

  return holy_USB_ERR_UNRECOVERABLE;
}

static holy_usb_err_t
holy_ehci_parse_halt (holy_usb_controller_t dev,
		      holy_usb_transfer_t transfer, holy_size_t * actual)
{
  struct holy_ehci *e = dev->data;
  struct holy_ehci_transfer_controller_data *cdata =
    transfer->controller_data;
  holy_uint32_t token;
  holy_usb_err_t err = holy_USB_ERR_NAK;

  /* QH should be halted and not active in this case. */

  holy_dprintf ("ehci", "parse_halt: info\n");

  /* Remember token before call pre-finish function */
  token = holy_le_to_cpu32 (cdata->qh_virt->td_overlay.token);

  /* Do things like in normal finish */
  holy_ehci_pre_finish_transfer (transfer);
  holy_ehci_free_tds (e, cdata->td_first_virt, transfer, actual);
  holy_ehci_free_td (e, cdata->td_alt_virt);
  holy_free (cdata);

  sync_all_caches (e);

  /* Evaluation of error code - currently we don't have holy USB error
   * codes for some EHCI states, holy_USB_ERR_DATA is used for them.
   * Order of evaluation is critical, specially bubble/stall. */
  if ((token & holy_EHCI_STATUS_BABBLE) != 0)
    err = holy_USB_ERR_BABBLE;
  else if ((token & holy_EHCI_CERR_MASK) != 0)
    err = holy_USB_ERR_STALL;
  else if ((token & holy_EHCI_STATUS_TRANERR) != 0)
    err = holy_USB_ERR_DATA;
  else if ((token & holy_EHCI_STATUS_BUFERR) != 0)
    err = holy_USB_ERR_DATA;
  else if ((token & holy_EHCI_STATUS_MISSDMF) != 0)
    err = holy_USB_ERR_DATA;

  return err;
}

static holy_usb_err_t
holy_ehci_parse_success (holy_usb_controller_t dev,
			 holy_usb_transfer_t transfer, holy_size_t * actual)
{
  struct holy_ehci *e = dev->data;
  struct holy_ehci_transfer_controller_data *cdata =
    transfer->controller_data;

  holy_dprintf ("ehci", "parse_success: info\n");

  /* QH should be not active in this case, but it is not halted. */
  holy_ehci_pre_finish_transfer (transfer);
  holy_ehci_free_tds (e, cdata->td_first_virt, transfer, actual);
  holy_ehci_free_td (e, cdata->td_alt_virt);
  holy_free (cdata);

  sync_all_caches (e);

  return holy_USB_ERR_NONE;
}


static holy_usb_err_t
holy_ehci_check_transfer (holy_usb_controller_t dev,
			  holy_usb_transfer_t transfer, holy_size_t * actual)
{
  struct holy_ehci *e = dev->data;
  struct holy_ehci_transfer_controller_data *cdata =
    transfer->controller_data;
  holy_uint32_t token, token_ftd;

  sync_all_caches (e);

  holy_dprintf ("ehci",
		"check_transfer: EHCI STATUS=%08x, cdata=%p, qh=%p\n",
		holy_ehci_oper_read32 (e, holy_EHCI_STATUS),
		cdata, cdata->qh_virt);
  holy_dprintf ("ehci", "check_transfer: qh_hptr=%08x, ep_char=%08x\n",
		holy_le_to_cpu32 (cdata->qh_virt->qh_hptr),
		holy_le_to_cpu32 (cdata->qh_virt->ep_char));
  holy_dprintf ("ehci", "check_transfer: ep_cap=%08x, td_current=%08x\n",
		holy_le_to_cpu32 (cdata->qh_virt->ep_cap),
		holy_le_to_cpu32 (cdata->qh_virt->td_current));
  holy_dprintf ("ehci", "check_transfer: next_td=%08x, alt_next_td=%08x\n",
		holy_le_to_cpu32 (cdata->qh_virt->td_overlay.next_td),
		holy_le_to_cpu32 (cdata->qh_virt->td_overlay.alt_next_td));
  holy_dprintf ("ehci", "check_transfer: token=%08x, buffer[0]=%08x\n",
		holy_le_to_cpu32 (cdata->qh_virt->td_overlay.token),
		holy_le_to_cpu32 (cdata->qh_virt->td_overlay.buffer_page[0]));

  /* Check if EHCI is running and AL is enabled */
  if ((holy_ehci_oper_read32 (e, holy_EHCI_STATUS)
       & holy_EHCI_ST_HC_HALTED) != 0)
    return holy_ehci_parse_notrun (dev, transfer, actual);
  if ((holy_ehci_oper_read32 (e, holy_EHCI_STATUS)
       & (holy_EHCI_ST_AS_STATUS | holy_EHCI_ST_PS_STATUS)) == 0)
    return holy_ehci_parse_notrun (dev, transfer, actual);

  token = holy_le_to_cpu32 (cdata->qh_virt->td_overlay.token);
  /* If the transfer consist from only one TD, we should check */
  /* if the TD was really executed and deactivated - to prevent */
  /* false detection of transfer finish. */
  token_ftd = holy_le_to_cpu32 (cdata->td_first_virt->token);

  /* Detect QH halted */
  if ((token & holy_EHCI_STATUS_HALTED) != 0)
    return holy_ehci_parse_halt (dev, transfer, actual);

  /* Detect QH not active - QH is not active and no next TD */
  if (token && ((token & holy_EHCI_STATUS_ACTIVE) == 0)
	&& ((token_ftd & holy_EHCI_STATUS_ACTIVE) == 0))
    {
      /* It could be finish at all or short packet condition */
      if ((holy_le_to_cpu32 (cdata->qh_virt->td_overlay.next_td)
	   & holy_EHCI_TERMINATE) &&
	  ((holy_le_to_cpu32 (cdata->qh_virt->td_current)
	    & holy_EHCI_QHTDPTR_MASK) == cdata->td_last_phys))
	/* Normal finish */
	return holy_ehci_parse_success (dev, transfer, actual);
      else if ((token & holy_EHCI_TOTAL_MASK) != 0)
	/* Short packet condition */
	/* But currently we don't handle it - higher level will do it */
	return holy_ehci_parse_success (dev, transfer, actual);
    }

  return holy_USB_ERR_WAIT;
}

static holy_usb_err_t
holy_ehci_cancel_transfer (holy_usb_controller_t dev,
			   holy_usb_transfer_t transfer)
{
  struct holy_ehci *e = dev->data;
  struct holy_ehci_transfer_controller_data *cdata =
    transfer->controller_data;
  holy_size_t actual;
  int i;
  holy_uint64_t maxtime;
  holy_uint32_t qh_phys;

  sync_all_caches (e);

  holy_uint32_t interrupt =
    cdata->qh_virt->ep_cap & holy_EHCI_SMASK_MASK;

  /* QH can be active and should be de-activated and halted */

  holy_dprintf ("ehci", "cancel_transfer: begin\n");

  /* First check if EHCI is running - if not, there is no problem */
  /* to cancel any transfer. Or, if transfer is asynchronous, check */
  /* if AL is enabled - if not, transfer can be canceled also. */
  if (((holy_ehci_oper_read32 (e, holy_EHCI_STATUS) &
      holy_EHCI_ST_HC_HALTED) != 0) ||
    (!interrupt && ((holy_ehci_oper_read32 (e, holy_EHCI_STATUS) &
      (holy_EHCI_ST_AS_STATUS | holy_EHCI_ST_PS_STATUS)) == 0)))
    {
      holy_ehci_pre_finish_transfer (transfer);
      holy_ehci_free_tds (e, cdata->td_first_virt, transfer, &actual);
      holy_ehci_free_td (e, cdata->td_alt_virt);
      holy_free (cdata);
      sync_all_caches (e);
      holy_dprintf ("ehci", "cancel_transfer: end - EHCI not running\n");
      return holy_USB_ERR_NONE;
    }

  /* EHCI and (AL or SL) are running. What to do? */
  /* Try to Halt QH via de-scheduling QH. */
  /* Find index of previous QH */
  qh_phys = holy_dma_virt2phys(cdata->qh_virt, e->qh_chunk);
  for (i = 0; i < holy_EHCI_N_QH; i++)
    {
      if ((holy_le_to_cpu32(e->qh_virt[i].qh_hptr)
        & holy_EHCI_QHTDPTR_MASK) == qh_phys)
        break;
    }
  if (i == holy_EHCI_N_QH)
    {
      holy_printf ("%s: prev not found, queues are corrupt\n", __func__);
      return holy_USB_ERR_UNRECOVERABLE;
    }
  /* Unlink QH from AL */
  e->qh_virt[i].qh_hptr = cdata->qh_virt->qh_hptr;

  sync_all_caches (e);

  /* If this is an interrupt transfer, we just wait for the periodic
   * schedule to advance a few times and then assume that the EHCI
   * controller has read the updated QH. */
  if (cdata->qh_virt->ep_cap & holy_EHCI_SMASK_MASK)
    {
      holy_millisleep(20);
    }
  else
    {
      /* For the asynchronous schedule we use the advance doorbell to find
       * out when the EHCI controller has read the updated QH. */

      /* Ring the doorbell */
      holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
                              holy_EHCI_CMD_AS_ADV_D
                              | holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));
      /* Ensure command is written */
      holy_ehci_oper_read32 (e, holy_EHCI_COMMAND);
      /* Wait answer with timeout */
      maxtime = holy_get_time_ms () + 2;
      while (((holy_ehci_oper_read32 (e, holy_EHCI_STATUS)
               & holy_EHCI_ST_AS_ADVANCE) == 0)
             && (holy_get_time_ms () < maxtime));

      /* We do not detect the timeout because if timeout occurs, it most
       * probably means something wrong with EHCI - maybe stopped etc. */

      /* Shut up the doorbell */
      holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
                              ~holy_EHCI_CMD_AS_ADV_D
                              & holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));
      holy_ehci_oper_write32 (e, holy_EHCI_STATUS,
                              holy_EHCI_ST_AS_ADVANCE
                              | holy_ehci_oper_read32 (e, holy_EHCI_STATUS));
      /* Ensure command is written */
      holy_ehci_oper_read32 (e, holy_EHCI_STATUS);
    }

  /* Now is QH out of AL and we can do anything with it... */
  holy_ehci_pre_finish_transfer (transfer);
  holy_ehci_free_tds (e, cdata->td_first_virt, transfer, &actual);
  holy_ehci_free_td (e, cdata->td_alt_virt);

  /* "Free" the QH - link it to itself */
  cdata->qh_virt->ep_char = 0;
  cdata->qh_virt->qh_hptr =
    holy_cpu_to_le32 ((holy_dma_virt2phys (cdata->qh_virt,
                                           e->qh_chunk)
                       & holy_EHCI_POINTER_MASK) | holy_EHCI_HPTR_TYPE_QH);

  holy_free (cdata);

  holy_dprintf ("ehci", "cancel_transfer: end\n");

  sync_all_caches (e);

  return holy_USB_ERR_NONE;
}

static int
holy_ehci_hubports (holy_usb_controller_t dev)
{
  struct holy_ehci *e = (struct holy_ehci *) dev->data;
  holy_uint32_t portinfo;

  portinfo = holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_SPARAMS)
    & holy_EHCI_SPARAMS_N_PORTS;
  holy_dprintf ("ehci", "root hub ports=%d\n", portinfo);
  return portinfo;
}

static holy_usb_err_t
holy_ehci_portstatus (holy_usb_controller_t dev,
		      unsigned int port, unsigned int enable)
{
  struct holy_ehci *e = (struct holy_ehci *) dev->data;
  holy_uint64_t endtime;

  holy_dprintf ("ehci", "portstatus: EHCI STATUS: %08x\n",
		holy_ehci_oper_read32 (e, holy_EHCI_STATUS));
  holy_dprintf ("ehci",
		"portstatus: begin, iobase=%p, port=%d, status=0x%02x\n",
		e->iobase, port, holy_ehci_port_read (e, port));

  /* In any case we need to disable port:
   * - if enable==false - we should disable port
   * - if enable==true we will do the reset and the specification says
   *   PortEnable should be FALSE in such case */
  /* Disable the port and wait for it. */
  holy_ehci_port_resbits (e, port, holy_EHCI_PORT_ENABLED);
  endtime = holy_get_time_ms () + 1000;
  while (holy_ehci_port_read (e, port) & holy_EHCI_PORT_ENABLED)
    if (holy_get_time_ms () > endtime)
      return holy_USB_ERR_TIMEOUT;

  if (!enable)			/* We don't need reset port */
    {
      holy_dprintf ("ehci", "portstatus: Disabled.\n");
      holy_dprintf ("ehci", "portstatus: end, status=0x%02x\n",
		    holy_ehci_port_read (e, port));
      return holy_USB_ERR_NONE;
    }

  holy_dprintf ("ehci", "portstatus: enable\n");

  holy_boot_time ("Resetting port %d", port);

  /* Now we will do reset - if HIGH speed device connected, it will
   * result in Enabled state, otherwise port remains disabled. */
  /* Set RESET bit for 50ms */
  holy_ehci_port_setbits (e, port, holy_EHCI_PORT_RESET);
  holy_millisleep (50);

  /* Reset RESET bit and wait for the end of reset */
  holy_ehci_port_resbits (e, port, holy_EHCI_PORT_RESET);
  endtime = holy_get_time_ms () + 1000;
  while (holy_ehci_port_read (e, port) & holy_EHCI_PORT_RESET)
    if (holy_get_time_ms () > endtime)
      return holy_USB_ERR_TIMEOUT;
  holy_boot_time ("Port %d reset", port);
  /* Remember "we did the reset" - needed by detect_dev */
  e->reset |= (1 << port);
  /* Test if port enabled, i.e. HIGH speed device connected */
  if ((holy_ehci_port_read (e, port) & holy_EHCI_PORT_ENABLED) != 0)	/* yes! */
    {
      holy_dprintf ("ehci", "portstatus: Enabled!\n");
      /* "Reset recovery time" (USB spec.) */
      holy_millisleep (10);
    }
  else				/* no... */
    {
      /* FULL speed device connected - change port ownership.
       * It results in disconnected state of this EHCI port. */
      holy_ehci_port_setbits (e, port, holy_EHCI_PORT_OWNER);
      return holy_USB_ERR_BADDEVICE;
    }

  /* XXX: Fix it! There is possible problem - we can say to calling
   * function that we lost device if it is FULL speed onlu via
   * return value <> holy_ERR_NONE. It (maybe) displays also error
   * message on screen - but this situation is not error, it is normal
   * state! */

  holy_dprintf ("ehci", "portstatus: end, status=0x%02x\n",
		holy_ehci_port_read (e, port));

  return holy_USB_ERR_NONE;
}

static holy_usb_speed_t
holy_ehci_detect_dev (holy_usb_controller_t dev, int port, int *changed)
{
  struct holy_ehci *e = (struct holy_ehci *) dev->data;
  holy_uint32_t status, line_state;

  status = holy_ehci_port_read (e, port);

  /* Connect Status Change bit - it detects change of connection */
  if (status & holy_EHCI_PORT_CONNECT_CH)
    {
      *changed = 1;
      /* Reset bit Connect Status Change */
      holy_ehci_port_setbits (e, port, holy_EHCI_PORT_CONNECT_CH);
    }
  else
    *changed = 0;

  if (!(status & holy_EHCI_PORT_CONNECT))
    {				/* We should reset related "reset" flag in not connected state */
      e->reset &= ~(1 << port);
      return holy_USB_SPEED_NONE;
    }
  /* Detected connected state, so we should return speed.
   * But we can detect only LOW speed device and only at connection
   * time when PortEnabled=FALSE. FULL / HIGH speed detection is made
   * later by EHCI-specific reset procedure.
   * Another thing - if detected speed is LOW at connection time,
   * we should change port ownership to companion controller.
   * So:
   * 1. If we detect connected and enabled and EHCI-owned port,
   * we can say it is HIGH speed.
   * 2. If we detect connected and not EHCI-owned port, we can say
   * NONE speed, because such devices are not handled by EHCI.
   * 3. If we detect connected, not enabled but reset port, we can say
   * NONE speed, because it means FULL device connected to port and
   * such devices are not handled by EHCI.
   * 4. If we detect connected, not enabled and not reset port, which
   * has line state != "K", we will say HIGH - it could be FULL or HIGH
   * device, we will see it later after end of EHCI-specific reset
   * procedure.
   * 5. If we detect connected, not enabled and not reset port, which
   * has line state == "K", we can say NONE speed, because LOW speed
   * device is connected and we should change port ownership. */
  if ((status & holy_EHCI_PORT_ENABLED) != 0)	/* Port already enabled, return high speed. */
    return holy_USB_SPEED_HIGH;
  if ((status & holy_EHCI_PORT_OWNER) != 0)	/* EHCI is not port owner */
    return holy_USB_SPEED_NONE;	/* EHCI driver is ignoring this port. */
  if ((e->reset & (1 << port)) != 0)	/* Port reset was done = FULL speed */
    return holy_USB_SPEED_NONE;	/* EHCI driver is ignoring this port. */
  else				/* Port connected but not enabled - test port speed. */
    {
      line_state = status & holy_EHCI_PORT_LINE_STAT;
      if (line_state != holy_EHCI_PORT_LINE_LOWSP)
	return holy_USB_SPEED_HIGH;
      /* Detected LOW speed device, we should change
       * port ownership.
       * XXX: Fix it!: There should be test if related companion
       * controler is available ! And what to do if it does not exist ? */
      holy_ehci_port_setbits (e, port, holy_EHCI_PORT_OWNER);
      return holy_USB_SPEED_NONE;	/* Ignore this port */
      /* Note: Reset of PORT_OWNER bit is done by EHCI HW when
       * device is really disconnected from port.
       * Don't do PORT_OWNER bit reset by SW when not connected signal
       * is detected in port register ! */
    }
}

static void
holy_ehci_inithw (void)
{
  holy_pci_iterate (holy_ehci_pci_iter, NULL);
}

static holy_err_t
holy_ehci_restore_hw (void)
{
  struct holy_ehci *e;
  holy_uint32_t n_ports;
  int i;

  /* We should re-enable all EHCI HW similarly as on inithw */
  for (e = ehci; e; e = e->next)
    {
      /* Check if EHCI is halted and halt it if not */
      if (holy_ehci_halt (e) != holy_USB_ERR_NONE)
	holy_error (holy_ERR_TIMEOUT, "restore_hw: EHCI halt timeout");

      /* Reset EHCI */
      if (holy_ehci_reset (e) != holy_USB_ERR_NONE)
	holy_error (holy_ERR_TIMEOUT, "restore_hw: EHCI reset timeout");

      /* Setup some EHCI registers and enable EHCI */
      holy_ehci_oper_write32 (e, holy_EHCI_FL_BASE, e->framelist_phys);
      holy_ehci_oper_write32 (e, holy_EHCI_CUR_AL_ADDR,
			      holy_dma_virt2phys (&e->qh_virt[1],
						   e->qh_chunk));
      holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
			      holy_EHCI_CMD_RUNSTOP |
			      holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));

      /* Set ownership of root hub ports to EHCI */
      holy_ehci_oper_write32 (e, holy_EHCI_CONFIG_FLAG,
			      holy_EHCI_CF_EHCI_OWNER);

      /* Enable asynchronous list */
      holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
			      holy_EHCI_CMD_AS_ENABL
			      | holy_EHCI_CMD_PS_ENABL
			      | holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));

      /* Now should be possible to power-up and enumerate ports etc. */
      if ((holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_SPARAMS)
	   & holy_EHCI_SPARAMS_PPC) != 0)
	{			/* EHCI has port powering control */
	  /* Power on all ports */
	  n_ports = holy_ehci_ehcc_read32 (e, holy_EHCI_EHCC_SPARAMS)
	    & holy_EHCI_SPARAMS_N_PORTS;
	  for (i = 0; i < (int) n_ports; i++)
	    holy_ehci_oper_write32 (e, holy_EHCI_PORT_STAT_CMD + i * 4,
				    holy_EHCI_PORT_POWER
				    | holy_ehci_oper_read32 (e,
							     holy_EHCI_PORT_STAT_CMD
							     + i * 4));
	}
    }

  return holy_ERR_NONE;
}

static holy_err_t
holy_ehci_fini_hw (int noreturn __attribute__ ((unused)))
{
  struct holy_ehci *e;

  /* We should disable all EHCI HW to prevent any DMA access etc. */
  for (e = ehci; e; e = e->next)
    {
      /* Disable both lists */
      holy_ehci_oper_write32 (e, holy_EHCI_COMMAND,
        ~(holy_EHCI_CMD_AS_ENABL | holy_EHCI_CMD_PS_ENABL)
        & holy_ehci_oper_read32 (e, holy_EHCI_COMMAND));

      /* Check if EHCI is halted and halt it if not */
      holy_ehci_halt (e);

      /* Reset EHCI */
      holy_ehci_reset (e);
    }

  return holy_ERR_NONE;
}

static struct holy_usb_controller_dev usb_controller = {
  .name = "ehci",
  .iterate = holy_ehci_iterate,
  .setup_transfer = holy_ehci_setup_transfer,
  .check_transfer = holy_ehci_check_transfer,
  .cancel_transfer = holy_ehci_cancel_transfer,
  .hubports = holy_ehci_hubports,
  .portstatus = holy_ehci_portstatus,
  .detect_dev = holy_ehci_detect_dev,
  /* estimated max. count of TDs for one bulk transfer */
  .max_bulk_tds = holy_EHCI_N_TD * 3 / 4
};

holy_MOD_INIT (ehci)
{
  COMPILE_TIME_ASSERT (sizeof (struct holy_ehci_td) == 64);
  COMPILE_TIME_ASSERT (sizeof (struct holy_ehci_qh) == 96);

  holy_stop_disk_firmware ();

  holy_boot_time ("Initing EHCI hardware");
  holy_ehci_inithw ();
  holy_boot_time ("Registering EHCI driver");
  holy_usb_controller_dev_register (&usb_controller);
  holy_boot_time ("EHCI driver registered");
  holy_loader_register_preboot_hook (holy_ehci_fini_hw, holy_ehci_restore_hw,
				     holy_LOADER_PREBOOT_HOOK_PRIO_DISK);
}

holy_MOD_FINI (ehci)
{
  holy_ehci_fini_hw (0);
  holy_usb_controller_dev_unregister (&usb_controller);
}
