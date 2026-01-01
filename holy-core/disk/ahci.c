/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/pci.h>
#include <holy/ata.h>
#include <holy/scsi.h>
#include <holy/misc.h>
#include <holy/list.h>
#include <holy/loader.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_ahci_cmd_head
{
  holy_uint32_t config;
  holy_uint32_t transferred;
  holy_uint64_t command_table_base;
  holy_uint32_t unused[4];
};

struct holy_ahci_prdt_entry
{
  holy_uint64_t data_base;
  holy_uint32_t unused;
  holy_uint32_t size;
};

struct holy_ahci_cmd_table
{
  holy_uint8_t cfis[0x40];
  holy_uint8_t command[0x10];
  holy_uint8_t reserved[0x30];
  struct holy_ahci_prdt_entry prdt[1];
};

struct holy_ahci_hba_port
{
  holy_uint64_t command_list_base;
  holy_uint64_t fis_base;
  holy_uint32_t intstatus;
  holy_uint32_t inten;
  holy_uint32_t command;
  holy_uint32_t unused1;
  holy_uint32_t task_file_data;
  holy_uint32_t sig;
  holy_uint32_t status;
  holy_uint32_t unused2;
  holy_uint32_t sata_error;
  holy_uint32_t sata_active;
  holy_uint32_t command_issue;
  holy_uint32_t unused3;
  holy_uint32_t fbs;
  holy_uint32_t unused4[15];
};

enum holy_ahci_hba_port_command
  {
    holy_AHCI_HBA_PORT_CMD_ST  = 0x01,
    holy_AHCI_HBA_PORT_CMD_SPIN_UP = 0x02,
    holy_AHCI_HBA_PORT_CMD_POWER_ON = 0x04,
    holy_AHCI_HBA_PORT_CMD_FRE = 0x10,
    holy_AHCI_HBA_PORT_CMD_CR = 0x8000,
    holy_AHCI_HBA_PORT_CMD_FR = 0x4000,
  };

struct holy_ahci_hba
{
  holy_uint32_t cap;
  holy_uint32_t global_control;
  holy_uint32_t intr_status;
  holy_uint32_t ports_implemented;
  holy_uint32_t unused1[6];
  holy_uint32_t bios_handoff;
  holy_uint32_t unused2[53];
  struct holy_ahci_hba_port ports[32];
};

struct holy_ahci_received_fis
{
  char raw[4096];
};

enum
  {
    holy_AHCI_HBA_CAP_NPORTS_MASK = 0x1f
  };

enum
  {
    holy_AHCI_HBA_GLOBAL_CONTROL_RESET = 0x00000001,
    holy_AHCI_HBA_GLOBAL_CONTROL_INTR_EN = 0x00000002,
    holy_AHCI_HBA_GLOBAL_CONTROL_AHCI_EN = 0x80000000,
  };

enum
  {
    holy_AHCI_BIOS_HANDOFF_BIOS_OWNED = 1,
    holy_AHCI_BIOS_HANDOFF_OS_OWNED = 2,
    holy_AHCI_BIOS_HANDOFF_OS_OWNERSHIP_CHANGED = 8,
    holy_AHCI_BIOS_HANDOFF_RWC = 8
  };


struct holy_ahci_device
{
  struct holy_ahci_device *next;
  struct holy_ahci_device **prev;
  volatile struct holy_ahci_hba *hba;
  int port;
  int num;
  struct holy_pci_dma_chunk *command_list_chunk;
  volatile struct holy_ahci_cmd_head *command_list;
  struct holy_pci_dma_chunk *command_table_chunk;
  volatile struct holy_ahci_cmd_table *command_table;
  struct holy_pci_dma_chunk *rfis;
  int present;
  int atapi;
};

static holy_err_t
holy_ahci_readwrite_real (struct holy_ahci_device *dev,
			  struct holy_disk_ata_pass_through_parms *parms,
			  int spinup, int reset);


enum
  {
    holy_AHCI_CONFIG_READ = 0,
    holy_AHCI_CONFIG_CFIS_LENGTH_MASK = 0x1f,
    holy_AHCI_CONFIG_ATAPI = 0x20,
    holy_AHCI_CONFIG_WRITE = 0x40,
    holy_AHCI_CONFIG_PREFETCH = 0x80,
    holy_AHCI_CONFIG_RESET = 0x100,
    holy_AHCI_CONFIG_BIST = 0x200,
    holy_AHCI_CONFIG_CLEAR_R_OK = 0x400,
    holy_AHCI_CONFIG_PMP_MASK = 0xf000,
    holy_AHCI_CONFIG_PRDT_LENGTH_MASK = 0xffff0000,
  };
#define holy_AHCI_CONFIG_CFIS_LENGTH_SHIFT 0
#define holy_AHCI_CONFIG_PMP_SHIFT 12
#define holy_AHCI_CONFIG_PRDT_LENGTH_SHIFT 16
#define holy_AHCI_INTERRUPT_ON_COMPLETE 0x80000000

#define holy_AHCI_PRDT_MAX_CHUNK_LENGTH 0x200000

static struct holy_ahci_device *holy_ahci_devices;
static int numdevs;

static int
holy_ahci_pciinit (holy_pci_device_t dev,
		   holy_pci_id_t pciid __attribute__ ((unused)),
		   void *data __attribute__ ((unused)))
{
  holy_pci_address_t addr;
  holy_uint32_t class;
  holy_uint32_t bar;
  unsigned i, nports;
  volatile struct holy_ahci_hba *hba;

  /* Read class.  */
  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  class = holy_pci_read (addr);

  /* Check if this class ID matches that of a PCI IDE Controller.  */
  if (class >> 8 != 0x010601)
    return 0;

  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG5);

  bar = holy_pci_read (addr);

  if ((bar & (holy_PCI_ADDR_SPACE_MASK | holy_PCI_ADDR_MEM_TYPE_MASK
	      | holy_PCI_ADDR_MEM_PREFETCH))
      != (holy_PCI_ADDR_SPACE_MEMORY | holy_PCI_ADDR_MEM_TYPE_32))
    return 0;

  addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
  holy_pci_write_word (addr, holy_pci_read_word (addr)
		    | holy_PCI_COMMAND_MEM_ENABLED | holy_PCI_COMMAND_BUS_MASTER);

  hba = holy_pci_device_map_range (dev, bar & holy_PCI_ADDR_MEM_MASK,
				   sizeof (*hba));
  holy_dprintf ("ahci", "dev: %x:%x.%x\n", dev.bus, dev.device, dev.function);

  holy_dprintf ("ahci", "tfd[0]: %x\n",
		hba->ports[0].task_file_data);
  holy_dprintf ("ahci", "cmd[0]: %x\n",
		hba->ports[0].command);
  holy_dprintf ("ahci", "st[0]: %x\n",
		hba->ports[0].status);
  holy_dprintf ("ahci", "err[0]: %x\n",
		hba->ports[0].sata_error);

  holy_dprintf ("ahci", "tfd[1]: %x\n",
		hba->ports[1].task_file_data);
  holy_dprintf ("ahci", "cmd[1]: %x\n",
		hba->ports[1].command);
  holy_dprintf ("ahci", "st[1]: %x\n",
		hba->ports[1].status);
  holy_dprintf ("ahci", "err[1]: %x\n",
		hba->ports[1].sata_error);

  hba->ports[1].sata_error = hba->ports[1].sata_error;

  holy_dprintf ("ahci", "err[1]: %x\n",
		hba->ports[1].sata_error);

  holy_dprintf ("ahci", "BH:%x\n", hba->bios_handoff);

  if (! (hba->bios_handoff & holy_AHCI_BIOS_HANDOFF_OS_OWNED))
    {
      holy_uint64_t endtime;

      holy_dprintf ("ahci", "Requesting AHCI ownership\n");
      hba->bios_handoff = (hba->bios_handoff & ~holy_AHCI_BIOS_HANDOFF_RWC)
	| holy_AHCI_BIOS_HANDOFF_OS_OWNED;
      holy_dprintf ("ahci", "Waiting for BIOS to give up ownership\n");
      endtime = holy_get_time_ms () + 1000;
      while ((hba->bios_handoff & holy_AHCI_BIOS_HANDOFF_BIOS_OWNED)
	     && holy_get_time_ms () < endtime);
      if (hba->bios_handoff & holy_AHCI_BIOS_HANDOFF_BIOS_OWNED)
	{
	  holy_dprintf ("ahci", "Forcibly taking ownership\n");
	  hba->bios_handoff = holy_AHCI_BIOS_HANDOFF_OS_OWNED;
	  hba->bios_handoff |= holy_AHCI_BIOS_HANDOFF_OS_OWNERSHIP_CHANGED;
	}
      else
	holy_dprintf ("ahci", "AHCI ownership obtained\n");
    }
  else
    holy_dprintf ("ahci", "AHCI is already in OS mode\n");

  holy_dprintf ("ahci", "GLC:%x\n", hba->global_control);

  holy_dprintf ("ahci", "err[1]: %x\n",
		hba->ports[1].sata_error);

  if (!(hba->global_control & holy_AHCI_HBA_GLOBAL_CONTROL_AHCI_EN))
    holy_dprintf ("ahci", "AHCI is in compat mode. Switching\n");
  else
    holy_dprintf ("ahci", "AHCI is in AHCI mode.\n");

  holy_dprintf ("ahci", "err[1]: %x\n",
		hba->ports[1].sata_error);

  holy_dprintf ("ahci", "GLC:%x\n", hba->global_control);

  /*  {
      holy_uint64_t endtime;
      hba->global_control |= 1;
      endtime = holy_get_time_ms () + 1000;
      while (hba->global_control & 1)
	if (holy_get_time_ms () > endtime)
	  {
	    holy_dprintf ("ahci", "couldn't reset AHCI\n");
	    return 0;
	  }
	  }*/

  holy_dprintf ("ahci", "GLC:%x\n", hba->global_control);

  holy_dprintf ("ahci", "err[1]: %x\n",
		hba->ports[1].sata_error);

  for (i = 0; i < 5; i++)
    {
      hba->global_control |= holy_AHCI_HBA_GLOBAL_CONTROL_AHCI_EN;
      holy_millisleep (1);
      if (hba->global_control & holy_AHCI_HBA_GLOBAL_CONTROL_AHCI_EN)
	break;
    }
  if (i == 5)
    {
      holy_dprintf ("ahci", "Couldn't put AHCI in AHCI mode\n");
      return 0;
    }

  holy_dprintf ("ahci", "GLC:%x\n", hba->global_control);

  holy_dprintf ("ahci", "err[1]: %x\n",
		hba->ports[1].sata_error);

  holy_dprintf ("ahci", "err[1]: %x\n",
		hba->ports[1].sata_error);

  holy_dprintf ("ahci", "GLC:%x\n", hba->global_control);

  for (i = 0; i < 5; i++)
    {
      hba->global_control |= holy_AHCI_HBA_GLOBAL_CONTROL_AHCI_EN;
      holy_millisleep (1);
      if (hba->global_control & holy_AHCI_HBA_GLOBAL_CONTROL_AHCI_EN)
	break;
    }
  if (i == 5)
    {
      holy_dprintf ("ahci", "Couldn't put AHCI in AHCI mode\n");
      return 0;
    }

  holy_dprintf ("ahci", "err[1]: %x\n",
		hba->ports[1].sata_error);

  holy_dprintf ("ahci", "GLC:%x\n", hba->global_control);

  nports = (holy_AHCI_HBA_CAP_NPORTS_MASK) + 1;

  holy_dprintf ("ahci", "%d AHCI ports, PI = 0x%x\n", nports,
		hba->ports_implemented);

  struct holy_ahci_device *adevs[holy_AHCI_HBA_CAP_NPORTS_MASK + 1];
  struct holy_ahci_device *failed_adevs[holy_AHCI_HBA_CAP_NPORTS_MASK + 1];
  holy_uint32_t fr_running = 0;

  for (i = 0; i < nports; i++)
    failed_adevs[i] = 0;
  for (i = 0; i < nports; i++)
    {
      if (!(hba->ports_implemented & (1 << i)))
	{
	  adevs[i] = 0;
	  continue;
	}

      adevs[i] = holy_zalloc (sizeof (*adevs[i]));
      if (!adevs[i])
	return 1;

      adevs[i]->hba = hba;
      adevs[i]->port = i;
      adevs[i]->present = 1;
      adevs[i]->num = numdevs++;
    }

  for (i = 0; i < nports; i++)
    if (adevs[i])
      {
	adevs[i]->hba->ports[adevs[i]->port].sata_error = adevs[i]->hba->ports[adevs[i]->port].sata_error;
	holy_dprintf ("ahci", "port: %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);

	adevs[i]->command_list_chunk = holy_memalign_dma32 (1024, sizeof (struct holy_ahci_cmd_head) * 32);
	if (!adevs[i]->command_list_chunk)
	  {
	    adevs[i] = 0;
	    continue;
	  }

	adevs[i]->command_table_chunk = holy_memalign_dma32 (1024,
							    sizeof (struct holy_ahci_cmd_table));
	if (!adevs[i]->command_table_chunk)
	  {
	    holy_dma_free (adevs[i]->command_list_chunk);
	    adevs[i] = 0;
	    continue;
	  }

	adevs[i]->command_list = holy_dma_get_virt (adevs[i]->command_list_chunk);
	adevs[i]->command_table = holy_dma_get_virt (adevs[i]->command_table_chunk);

	holy_memset ((void *) adevs[i]->command_list, 0,
		     sizeof (struct holy_ahci_cmd_table));
	holy_memset ((void *) adevs[i]->command_table, 0,
		     sizeof (struct holy_ahci_cmd_head) * 32);

	adevs[i]->command_list->command_table_base
	  = holy_dma_get_phys (adevs[i]->command_table_chunk);

	holy_dprintf ("ahci", "found device ahci%d (port %d), command_table = %p, command_list = %p\n",
		      adevs[i]->num, adevs[i]->port, holy_dma_get_virt (adevs[i]->command_table_chunk),
		      holy_dma_get_virt (adevs[i]->command_list_chunk));

	adevs[i]->hba->ports[adevs[i]->port].command &= ~holy_AHCI_HBA_PORT_CMD_FRE;
      }

  holy_uint64_t endtime;
  endtime = holy_get_time_ms () + 1000;

  while (holy_get_time_ms () < endtime)
    {
      for (i = 0; i < nports; i++)
	if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_FR))
	  break;
      if (i == nports)
	break;
    }

  for (i = 0; i < nports; i++)
    if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_FR))
      {
	holy_dprintf ("ahci", "couldn't stop FR on port %d\n", i);
	failed_adevs[i] = adevs[i];
	adevs[i] = 0;
      }

  for (i = 0; i < nports; i++)
    if (adevs[i])
      adevs[i]->hba->ports[adevs[i]->port].command &= ~holy_AHCI_HBA_PORT_CMD_ST;
  endtime = holy_get_time_ms () + 1000;

  while (holy_get_time_ms () < endtime)
    {
      for (i = 0; i < nports; i++)
	if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_CR))
	  break;
      if (i == nports)
	break;
    }

  for (i = 0; i < nports; i++)
    if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_CR))
      {
	holy_dprintf ("ahci", "couldn't stop CR on port %d\n", i);
	failed_adevs[i] = adevs[i];
	adevs[i] = 0;
      }
  for (i = 0; i < nports; i++)
    if (adevs[i])
      {
	adevs[i]->hba->ports[adevs[i]->port].inten = 0;
	adevs[i]->hba->ports[adevs[i]->port].intstatus = ~0;
	//  adevs[i]->hba->ports[adevs[i]->port].fbs = 0;

	holy_dprintf ("ahci", "port: %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);

	adevs[i]->rfis = holy_memalign_dma32 (4096,
					     sizeof (struct holy_ahci_received_fis));
	holy_memset ((char *) holy_dma_get_virt (adevs[i]->rfis), 0,
		     sizeof (struct holy_ahci_received_fis));
	holy_memset ((char *) holy_dma_get_virt (adevs[i]->command_list_chunk), 0,
		     sizeof (struct holy_ahci_cmd_head));
	holy_memset ((char *) holy_dma_get_virt (adevs[i]->command_table_chunk), 0,
		     sizeof (struct holy_ahci_cmd_table));
	adevs[i]->hba->ports[adevs[i]->port].fis_base = holy_dma_get_phys (adevs[i]->rfis);
	adevs[i]->hba->ports[adevs[i]->port].command_list_base
	  = holy_dma_get_phys (adevs[i]->command_list_chunk);
	adevs[i]->hba->ports[adevs[i]->port].command_issue = 0;
	adevs[i]->hba->ports[adevs[i]->port].command |= holy_AHCI_HBA_PORT_CMD_FRE;
      }

  endtime = holy_get_time_ms () + 1000;

  while (holy_get_time_ms () < endtime)
    {
      for (i = 0; i < nports; i++)
	if (adevs[i] && !(adevs[i]->hba->ports[adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_FR))
	  break;
      if (i == nports)
	break;
    }

  for (i = 0; i < nports; i++)
    if (adevs[i] && !(adevs[i]->hba->ports[adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_FR))
      {
	holy_dprintf ("ahci", "couldn't start FR on port %d\n", i);
	failed_adevs[i] = adevs[i];
	adevs[i] = 0;
      }

  for (i = 0; i < nports; i++)
    if (adevs[i])
      {
	holy_dprintf ("ahci", "port: %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);
	fr_running |= (1 << i);

	adevs[i]->hba->ports[adevs[i]->port].command |= holy_AHCI_HBA_PORT_CMD_SPIN_UP;
	adevs[i]->hba->ports[adevs[i]->port].command |= holy_AHCI_HBA_PORT_CMD_POWER_ON;
	adevs[i]->hba->ports[adevs[i]->port].command |= 1 << 28;

	holy_dprintf ("ahci", "port: %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);
      }

  /* 10ms should actually be enough.  */
  endtime = holy_get_time_ms () + 100;

  while (holy_get_time_ms () < endtime)
    {
      for (i = 0; i < nports; i++)
	if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].status & 7) != 3)
	  break;
      if (i == nports)
	break;
    }

  for (i = 0; i < nports; i++)
    if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].status & 7) != 3)
      {
	holy_dprintf ("ahci", "couldn't detect device on port %d\n", i);
	failed_adevs[i] = adevs[i];
	adevs[i] = 0;
      }

  for (i = 0; i < nports; i++)
    if (adevs[i])
      {
	holy_dprintf ("ahci", "port %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);

	adevs[i]->hba->ports[adevs[i]->port].command |= holy_AHCI_HBA_PORT_CMD_POWER_ON;
	adevs[i]->hba->ports[adevs[i]->port].command |= holy_AHCI_HBA_PORT_CMD_SPIN_UP;

	holy_dprintf ("ahci", "port %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);

	adevs[i]->hba->ports[adevs[i]->port].sata_error = ~0;
	holy_dprintf ("ahci", "port %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);

	holy_dprintf ("ahci", "port %d, offset: %x, tfd:%x, CMD: %x\n", adevs[i]->port,
		      (int) ((char *) &adevs[i]->hba->ports[adevs[i]->port].task_file_data - 
			     (char *) adevs[i]->hba),
		      adevs[i]->hba->ports[adevs[i]->port].task_file_data,
		      adevs[i]->hba->ports[adevs[i]->port].command);

	holy_dprintf ("ahci", "port %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);
      }


  for (i = 0; i < nports; i++)
    if (adevs[i])
      {
	holy_dprintf ("ahci", "port %d, offset: %x, tfd:%x, CMD: %x\n", adevs[i]->port,
		      (int) ((char *) &adevs[i]->hba->ports[adevs[i]->port].task_file_data - 
			     (char *) adevs[i]->hba),
		      adevs[i]->hba->ports[adevs[i]->port].task_file_data,
		      adevs[i]->hba->ports[adevs[i]->port].command);

	holy_dprintf ("ahci", "port: %d, err: %x\n", adevs[i]->port,
		      adevs[i]->hba->ports[adevs[i]->port].sata_error);

	adevs[i]->hba->ports[adevs[i]->port].command
	  = (adevs[i]->hba->ports[adevs[i]->port].command & 0x0fffffff) | (1 << 28)
	  | holy_AHCI_HBA_PORT_CMD_SPIN_UP
	  | holy_AHCI_HBA_PORT_CMD_POWER_ON;

	/*  struct holy_disk_ata_pass_through_parms parms2;
	    holy_memset (&parms2, 0, sizeof (parms2));
	    parms2.taskfile.cmd = 8;
	    holy_ahci_readwrite_real (dev, &parms2, 1, 1);*/
      }

  endtime = holy_get_time_ms () + 10000;

  while (holy_get_time_ms () < endtime)
    {
      for (i = 0; i < nports; i++)
	if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].task_file_data & (holy_ATA_STATUS_BUSY | holy_ATA_STATUS_DRQ)))
	  break;
      if (i == nports)
	break;
    }

  for (i = 0; i < nports; i++)
    if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].task_file_data & (holy_ATA_STATUS_BUSY | holy_ATA_STATUS_DRQ)))
      {
	holy_dprintf ("ahci", "port %d is busy\n", i);
	failed_adevs[i] = adevs[i];
	adevs[i] = 0;
      }

  for (i = 0; i < nports; i++)
    if (adevs[i])
      adevs[i]->hba->ports[adevs[i]->port].command |= holy_AHCI_HBA_PORT_CMD_ST;

  endtime = holy_get_time_ms () + 1000;

  while (holy_get_time_ms () < endtime)
    {
      for (i = 0; i < nports; i++)
	if (adevs[i] && !(adevs[i]->hba->ports[adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_CR))
	  break;
      if (i == nports)
	break;
    }

  for (i = 0; i < nports; i++)
    if (adevs[i] && !(adevs[i]->hba->ports[adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_CR))
      {
	holy_dprintf ("ahci", "couldn't start CR on port %d\n", i);
	failed_adevs[i] = adevs[i];
	adevs[i] = 0;
      }

  holy_dprintf ("ahci", "cleaning up failed devs\n");

  for (i = 0; i < nports; i++)
    if (failed_adevs[i] && (fr_running & (1 << i)))
      failed_adevs[i]->hba->ports[failed_adevs[i]->port].command &= ~holy_AHCI_HBA_PORT_CMD_FRE;

  endtime = holy_get_time_ms () + 1000;
  while (holy_get_time_ms () < endtime)
    {
      for (i = 0; i < nports; i++)
	if (failed_adevs[i] && (fr_running & (1 << i)) && (failed_adevs[i]->hba->ports[failed_adevs[i]->port].command & holy_AHCI_HBA_PORT_CMD_FR))
	  break;
      if (i == nports)
	break;
    }
  for (i = 0; i < nports; i++)
    if (failed_adevs[i])
      {
	holy_dma_free (failed_adevs[i]->command_list_chunk);
	holy_dma_free (failed_adevs[i]->command_table_chunk);
	holy_dma_free (failed_adevs[i]->rfis);
      }

  for (i = 0; i < nports; i++)
    if (adevs[i] && (adevs[i]->hba->ports[adevs[i]->port].sig >> 16) == 0xeb14)
      adevs[i]->atapi = 1;

  addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
  holy_pci_write_word (addr, holy_pci_read_word (addr)
		    | holy_PCI_COMMAND_BUS_MASTER);

  for (i = 0; i < nports; i++)
    if (adevs[i])
      {
	holy_list_push (holy_AS_LIST_P (&holy_ahci_devices),
			holy_AS_LIST (adevs[i]));
      }

  return 0;
}

static holy_err_t
holy_ahci_initialize (void)
{
  holy_pci_iterate (holy_ahci_pciinit, NULL);
  return holy_errno;
}

static holy_err_t
holy_ahci_fini_hw (int noreturn __attribute__ ((unused)))
{
  struct holy_ahci_device *dev;

  for (dev = holy_ahci_devices; dev; dev = dev->next)
    {
      holy_uint64_t endtime;

      dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_FRE;
      endtime = holy_get_time_ms () + 1000;
      while ((dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_FR))
	if (holy_get_time_ms () > endtime)
	  {
	    holy_dprintf ("ahci", "couldn't stop FR\n");
	    break;
	  }

      dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_ST;
      endtime = holy_get_time_ms () + 1000;
      while ((dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_CR))
	if (holy_get_time_ms () > endtime)
	  {
	    holy_dprintf ("ahci", "couldn't stop CR\n");
	    break;
	  }
      holy_dma_free (dev->command_list_chunk);
      holy_dma_free (dev->command_table_chunk);
      holy_dma_free (dev->rfis);
      dev->command_list_chunk = NULL;
      dev->command_table_chunk = NULL;
      dev->rfis = NULL;
    }
  return holy_ERR_NONE;
}

static int 
reinit_port (struct holy_ahci_device *dev)
{
  struct holy_pci_dma_chunk *command_list;
  struct holy_pci_dma_chunk *command_table;
  holy_uint64_t endtime;

  command_list = holy_memalign_dma32 (1024, sizeof (struct holy_ahci_cmd_head));
  if (!command_list)
    return 1;

  command_table = holy_memalign_dma32 (1024,
				       sizeof (struct holy_ahci_cmd_table));
  if (!command_table)
    {
      holy_dma_free (command_list);
      return 1;
    }

  holy_dprintf ("ahci", "found device ahci%d (port %d)\n", dev->num, dev->port);

  dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_FRE;
  endtime = holy_get_time_ms () + 1000;
  while ((dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_FR))
    if (holy_get_time_ms () > endtime)
      {
	holy_dprintf ("ahci", "couldn't stop FR\n");
	goto out;
      }

  dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_ST;
  endtime = holy_get_time_ms () + 1000;
  while ((dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_CR))
    if (holy_get_time_ms () > endtime)
      {
	holy_dprintf ("ahci", "couldn't stop CR\n");
	goto out;
      }

  dev->hba->ports[dev->port].fbs = 2;

  dev->rfis = holy_memalign_dma32 (4096,
				   sizeof (struct holy_ahci_received_fis));
  holy_memset ((char *) holy_dma_get_virt (dev->rfis), 0,
	       sizeof (struct holy_ahci_received_fis));
  dev->hba->ports[dev->port].fis_base = holy_dma_get_phys (dev->rfis);
  dev->hba->ports[dev->port].command_list_base
    = holy_dma_get_phys (command_list);
  dev->hba->ports[dev->port].command |= holy_AHCI_HBA_PORT_CMD_FRE;
  while (!(dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_FR))
    if (holy_get_time_ms () > endtime)
      {
	holy_dprintf ("ahci", "couldn't start FR\n");
	dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_FRE;
	goto out;
      }
  dev->hba->ports[dev->port].command |= holy_AHCI_HBA_PORT_CMD_ST;
  while (!(dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_CR))
    if (holy_get_time_ms () > endtime)
      {
	holy_dprintf ("ahci", "couldn't start CR\n");
	dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_CR;
	goto out_stop_fr;
      }

  dev->hba->ports[dev->port].command
    = (dev->hba->ports[dev->port].command & 0x0fffffff) | (1 << 28) | 2 | 4;

  dev->command_list_chunk = command_list;
  dev->command_list = holy_dma_get_virt (command_list);
  dev->command_table_chunk = command_table;
  dev->command_table = holy_dma_get_virt (command_table);
  dev->command_list->command_table_base
    = holy_dma_get_phys (command_table);

  return 0;
 out_stop_fr:
  dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_FRE;
  endtime = holy_get_time_ms () + 1000;
  while ((dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_FR))
    if (holy_get_time_ms () > endtime)
      {
	holy_dprintf ("ahci", "couldn't stop FR\n");
	break;
      }
 out:
  holy_dma_free (command_list);
  holy_dma_free (command_table);
  holy_dma_free (dev->rfis);
  return 1;
}

static holy_err_t
holy_ahci_restore_hw (void)
{
  struct holy_ahci_device **pdev;

  for (pdev = &holy_ahci_devices; *pdev; pdev = &((*pdev)->next))
    if (reinit_port (*pdev))
      {
	struct holy_ahci_device *odev;
	odev = *pdev;
	*pdev = (*pdev)->next;
	holy_free (odev);
      }
  return holy_ERR_NONE;
}




static int
holy_ahci_iterate (holy_ata_dev_iterate_hook_t hook, void *hook_data,
		   holy_disk_pull_t pull)
{
  struct holy_ahci_device *dev;

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  FOR_LIST_ELEMENTS(dev, holy_ahci_devices)
    if (hook (holy_SCSI_SUBSYSTEM_AHCI, dev->num, hook_data))
      return 1;

  return 0;
}

#if 0
static int
find_free_cmd_slot (struct holy_ahci_device *dev)
{
  int i;
  for (i = 0; i < 32; i++)
    {
      if (dev->hda->ports[dev->port].command_issue & (1 << i))
	continue;
      if (dev->hda->ports[dev->port].sata_active & (1 << i))
	continue;
      return i;
    }
  return -1;
}
#endif

enum
  {
    holy_AHCI_FIS_REG_H2D = 0x27
  };

static const int register_map[11] = { 3 /* Features */,
				      12 /* Sectors */,
				      4 /* LBA low */,
				      5 /* LBA mid */,
				      6 /* LBA high */,
				      7 /* Device */,
				      2 /* CMD register */,
				      13 /* Sectors 48  */,
				      8 /* LBA48 low */,
				      9 /* LBA48 mid */,
				      10 /* LBA48 high */ }; 

static holy_err_t
holy_ahci_reset_port (struct holy_ahci_device *dev, int force)
{
  holy_uint64_t endtime;
  
  dev->hba->ports[dev->port].sata_error = dev->hba->ports[dev->port].sata_error;

  if (force || (dev->hba->ports[dev->port].command_issue & 1)
      || (dev->hba->ports[dev->port].task_file_data & 0x80))
    {
      struct holy_disk_ata_pass_through_parms parms2;
      dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_ST;
      dev->hba->ports[dev->port].command_issue = 0;
      dev->command_list[0].config = 0;
      dev->command_table[0].prdt[0].unused = 0;
      dev->command_table[0].prdt[0].size = 0;
      dev->command_table[0].prdt[0].data_base = 0;

      endtime = holy_get_time_ms () + 1000;
      while ((dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_CR))
	if (holy_get_time_ms () > endtime)
	  {
	    holy_dprintf ("ahci", "couldn't stop CR");
	    return holy_error (holy_ERR_IO, "couldn't stop CR");
	  }
      dev->hba->ports[dev->port].command |= 8;
      while (dev->hba->ports[dev->port].command & 8)
	if (holy_get_time_ms () > endtime)
	  {
	    holy_dprintf ("ahci", "couldn't set CLO\n");
	    dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_FRE;
	    return holy_error (holy_ERR_IO, "couldn't set CLO");
	  }

      dev->hba->ports[dev->port].command |= holy_AHCI_HBA_PORT_CMD_ST;
      while (!(dev->hba->ports[dev->port].command & holy_AHCI_HBA_PORT_CMD_CR))
	if (holy_get_time_ms () > endtime)
	  {
	    holy_dprintf ("ahci", "couldn't stop CR");
	    dev->hba->ports[dev->port].command &= ~holy_AHCI_HBA_PORT_CMD_ST;
	    return holy_error (holy_ERR_IO, "couldn't stop CR");
	  }
      dev->hba->ports[dev->port].sata_error = dev->hba->ports[dev->port].sata_error;
      holy_memset (&parms2, 0, sizeof (parms2));
      parms2.taskfile.cmd = 8;
      return holy_ahci_readwrite_real (dev, &parms2, 1, 1);
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_ahci_readwrite_real (struct holy_ahci_device *dev,
			  struct holy_disk_ata_pass_through_parms *parms,
			  int spinup, int reset)
{
  struct holy_pci_dma_chunk *bufc;
  holy_uint64_t endtime;
  unsigned i;
  holy_err_t err = holy_ERR_NONE;

  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);

  if (!reset)
    holy_ahci_reset_port (dev, 0);
 
  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);
  dev->hba->ports[dev->port].task_file_data = 0;
  dev->hba->ports[dev->port].command_issue = 0;
  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);

  dev->hba->ports[dev->port].sata_error = dev->hba->ports[dev->port].sata_error;

  holy_dprintf("ahci", "holy_ahci_read (size=%llu, cmdsize = %llu)\n",
	       (unsigned long long) parms->size,
	       (unsigned long long) parms->cmdsize);

  if (parms->cmdsize != 0 && parms->cmdsize != 12 && parms->cmdsize != 16)
    return holy_error (holy_ERR_BUG, "incorrect ATAPI command size");

  if (parms->size > holy_AHCI_PRDT_MAX_CHUNK_LENGTH)
    return holy_error (holy_ERR_BUG, "too big data buffer");

  if (parms->size)
    bufc = holy_memalign_dma32 (1024, parms->size + (parms->size & 1));
  else
    bufc = holy_memalign_dma32 (1024, 512);

  holy_dprintf ("ahci", "AHCI tfd = %x, CL=%p\n",
		dev->hba->ports[dev->port].task_file_data,
		dev->command_list);
  /* FIXME: support port multipliers.  */
  dev->command_list[0].config
    = (5 << holy_AHCI_CONFIG_CFIS_LENGTH_SHIFT)
    //    | holy_AHCI_CONFIG_CLEAR_R_OK
    | (0 << holy_AHCI_CONFIG_PMP_SHIFT)
    | ((parms->size ? 1 : 0) << holy_AHCI_CONFIG_PRDT_LENGTH_SHIFT)
    | (parms->cmdsize ? holy_AHCI_CONFIG_ATAPI : 0)
    | (parms->write ? holy_AHCI_CONFIG_WRITE : holy_AHCI_CONFIG_READ)
    | (parms->taskfile.cmd == 8 ? (1 << 8) : 0);
  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);

  dev->command_list[0].transferred = 0;
  dev->command_list[0].command_table_base
    = holy_dma_get_phys (dev->command_table_chunk);

  holy_memset ((char *) dev->command_list[0].unused, 0,
	       sizeof (dev->command_list[0].unused));

  holy_memset ((char *) &dev->command_table[0], 0,
	       sizeof (dev->command_table[0]));
  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);

  if (parms->cmdsize)
    holy_memcpy ((char *) dev->command_table[0].command, parms->cmd,
		 parms->cmdsize);

  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);

  dev->command_table[0].cfis[0] = holy_AHCI_FIS_REG_H2D;
  dev->command_table[0].cfis[1] = 0x80;
  for (i = 0; i < sizeof (parms->taskfile.raw); i++)
    dev->command_table[0].cfis[register_map[i]] = parms->taskfile.raw[i]; 

  holy_dprintf ("ahci", "cfis: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		dev->command_table[0].cfis[0], dev->command_table[0].cfis[1],
		dev->command_table[0].cfis[2], dev->command_table[0].cfis[3],
		dev->command_table[0].cfis[4], dev->command_table[0].cfis[5],
		dev->command_table[0].cfis[6], dev->command_table[0].cfis[7]);
  holy_dprintf ("ahci", "cfis: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		dev->command_table[0].cfis[8], dev->command_table[0].cfis[9],
		dev->command_table[0].cfis[10], dev->command_table[0].cfis[11],
		dev->command_table[0].cfis[12], dev->command_table[0].cfis[13],
		dev->command_table[0].cfis[14], dev->command_table[0].cfis[15]);

  dev->command_table[0].prdt[0].data_base = holy_dma_get_phys (bufc);
  dev->command_table[0].prdt[0].unused = 0;
  dev->command_table[0].prdt[0].size = (parms->size - 1);

  holy_dprintf ("ahci", "PRDT = %" PRIxholy_UINT64_T ", %x, %x (%"
		PRIuholy_SIZE ")\n",
		dev->command_table[0].prdt[0].data_base,
		dev->command_table[0].prdt[0].unused,
		dev->command_table[0].prdt[0].size,
		(holy_size_t) ((char *) &dev->command_table[0].prdt[0]
			       - (char *) &dev->command_table[0]));

  if (parms->write)
    holy_memcpy ((char *) holy_dma_get_virt (bufc), parms->buffer, parms->size);

  holy_dprintf ("ahci", "AHCI command scheduled\n");
  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);
  holy_dprintf ("ahci", "AHCI inten = %x\n",
		dev->hba->ports[dev->port].inten);
  holy_dprintf ("ahci", "AHCI intstatus = %x\n",
		dev->hba->ports[dev->port].intstatus);

  dev->hba->ports[dev->port].inten = 0xffffffff;//(1 << 2) | (1 << 5);
  dev->hba->ports[dev->port].intstatus = 0xffffffff;//(1 << 2) | (1 << 5);
  holy_dprintf ("ahci", "AHCI inten = %x\n",
		dev->hba->ports[dev->port].inten);
  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);
  dev->hba->ports[dev->port].sata_active = 1;
  dev->hba->ports[dev->port].command_issue = 1;
  holy_dprintf ("ahci", "AHCI sig = %x\n", dev->hba->ports[dev->port].sig);
  holy_dprintf ("ahci", "AHCI tfd = %x\n",
		dev->hba->ports[dev->port].task_file_data);

  endtime = holy_get_time_ms () + (spinup ? 20000 : 20000);
  while ((dev->hba->ports[dev->port].command_issue & 1))
    if (holy_get_time_ms () > endtime)
      {
	holy_dprintf ("ahci", "AHCI status <%x %x %x %x>\n",
		      dev->hba->ports[dev->port].command_issue,
		      dev->hba->ports[dev->port].sata_active,
		      dev->hba->ports[dev->port].intstatus,
		      dev->hba->ports[dev->port].task_file_data);
	dev->hba->ports[dev->port].command_issue = 0;
	err = holy_error (holy_ERR_IO, "AHCI transfer timed out");
	if (!reset)
	  holy_ahci_reset_port (dev, 1);
	break;
      }

  holy_dprintf ("ahci", "AHCI command completed <%x %x %x %x %x, %x %x>\n",
		dev->hba->ports[dev->port].command_issue,
		dev->hba->ports[dev->port].intstatus,
		dev->hba->ports[dev->port].task_file_data,
		dev->command_list[0].transferred,
		dev->hba->ports[dev->port].sata_error,
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x00],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x18]);
  holy_dprintf ("ahci",
		"last PIO FIS %08x %08x %08x %08x %08x %08x %08x %08x\n",
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x08],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x09],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x0a],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x0b],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x0c],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x0d],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x0e],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x0f]);
  holy_dprintf ("ahci",
		"last REG FIS %08x %08x %08x %08x %08x %08x %08x %08x\n",
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x10],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x11],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x12],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x13],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x14],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x15],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x16],
		((holy_uint32_t *) holy_dma_get_virt (dev->rfis))[0x17]);

  if (!parms->write)
    holy_memcpy (parms->buffer, (char *) holy_dma_get_virt (bufc), parms->size);
  holy_dma_free (bufc);

  return err;
}

static holy_err_t
holy_ahci_readwrite (holy_ata_t disk,
		     struct holy_disk_ata_pass_through_parms *parms,
		     int spinup)
{
  return holy_ahci_readwrite_real (disk->data, parms, spinup, 0);
}

static holy_err_t
holy_ahci_open (int id, int devnum, struct holy_ata *ata)
{
  struct holy_ahci_device *dev;

  if (id != holy_SCSI_SUBSYSTEM_AHCI)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "not an AHCI device");

  FOR_LIST_ELEMENTS(dev, holy_ahci_devices)
    if (dev->num == devnum)
      break;

  if (! dev)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "no such AHCI device");

  holy_dprintf ("ahci", "opening AHCI dev `ahci%d'\n", dev->num);

  ata->data = dev;
  ata->dma = 1;
  ata->atapi = dev->atapi;
  ata->maxbuffer = holy_AHCI_PRDT_MAX_CHUNK_LENGTH;
  ata->present = &dev->present;

  return holy_ERR_NONE;
}

static struct holy_ata_dev holy_ahci_dev =
  {
    .iterate = holy_ahci_iterate,
    .open = holy_ahci_open,
    .readwrite = holy_ahci_readwrite,
  };



static struct holy_preboot *fini_hnd;

holy_MOD_INIT(ahci)
{
  holy_stop_disk_firmware ();

  /* AHCI initialization.  */
  holy_ahci_initialize ();

  /* AHCI devices are handled by scsi.mod.  */
  holy_ata_dev_register (&holy_ahci_dev);

  fini_hnd = holy_loader_register_preboot_hook (holy_ahci_fini_hw,
						holy_ahci_restore_hw,
						holy_LOADER_PREBOOT_HOOK_PRIO_DISK);
}

holy_MOD_FINI(ahci)
{
  holy_ahci_fini_hw (0);
  holy_loader_unregister_preboot_hook (fini_hnd);

  holy_ata_dev_unregister (&holy_ahci_dev);
}
