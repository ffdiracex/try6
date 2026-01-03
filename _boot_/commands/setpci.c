/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/pci.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/env.h>
#include <holy/mm.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

struct pci_register
{
  const char *name;
  holy_uint16_t addr;
  unsigned size;
};

static struct pci_register pci_registers[] =
  {
    {"VENDOR_ID",       holy_PCI_REG_VENDOR      , 2},
    {"DEVICE_ID",       holy_PCI_REG_DEVICE      , 2},
    {"COMMAND",         holy_PCI_REG_COMMAND     , 2},
    {"STATUS",          holy_PCI_REG_STATUS      , 2},
    {"REVISION",        holy_PCI_REG_REVISION    , 1},
    {"CLASS_PROG",      holy_PCI_REG_CLASS + 1   , 1},
    {"CLASS_DEVICE",    holy_PCI_REG_CLASS + 2   , 2},
    {"CACHE_LINE_SIZE", holy_PCI_REG_CACHELINE   , 1},
    {"LATENCY_TIMER",   holy_PCI_REG_LAT_TIMER   , 1},
    {"HEADER_TYPE",     holy_PCI_REG_HEADER_TYPE , 1},
    {"BIST",            holy_PCI_REG_BIST        , 1},
    {"BASE_ADDRESS_0",  holy_PCI_REG_ADDRESS_REG0, 4},
    {"BASE_ADDRESS_1",  holy_PCI_REG_ADDRESS_REG1, 4},
    {"BASE_ADDRESS_2",  holy_PCI_REG_ADDRESS_REG2, 4},
    {"BASE_ADDRESS_3",  holy_PCI_REG_ADDRESS_REG3, 4},
    {"BASE_ADDRESS_4",  holy_PCI_REG_ADDRESS_REG4, 4},
    {"BASE_ADDRESS_5",  holy_PCI_REG_ADDRESS_REG5, 4},
    {"CARDBUS_CIS",     holy_PCI_REG_CIS_POINTER , 4},
    {"SUBVENDOR_ID",    holy_PCI_REG_SUBVENDOR   , 2},
    {"SUBSYSTEM_ID",    holy_PCI_REG_SUBSYSTEM   , 2},
    {"ROM_ADDRESS",     holy_PCI_REG_ROM_ADDRESS , 4},
    {"CAP_POINTER",     holy_PCI_REG_CAP_POINTER , 1},
    {"INTERRUPT_LINE",  holy_PCI_REG_IRQ_LINE    , 1},
    {"INTERRUPT_PIN",   holy_PCI_REG_IRQ_PIN     , 1},
    {"MIN_GNT",         holy_PCI_REG_MIN_GNT     , 1},
    {"MAX_LAT",         holy_PCI_REG_MIN_GNT     , 1},
  };

static const struct holy_arg_option options[] =
  {
    {0, 'd', 0, N_("Select device by vendor and device IDs."),
     N_("[vendor]:[device]"), ARG_TYPE_STRING},
    {0, 's', 0, N_("Select device by its position on the bus."),
     N_("[bus]:[slot][.func]"), ARG_TYPE_STRING},
    {0, 'v', 0, N_("Save read value into variable VARNAME."),
     N_("VARNAME"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static holy_uint32_t pciid_check_mask, pciid_check_value;
static int bus, device, function;
static int check_bus, check_device, check_function;
static holy_uint32_t write_mask, regwrite;
static int regsize;
static holy_uint16_t regaddr;
static const char *varname;

static int
holy_setpci_iter (holy_pci_device_t dev, holy_pci_id_t pciid,
		  void *data __attribute__ ((unused)))
{
  holy_uint32_t regval = 0;
  holy_pci_address_t addr;

  if ((pciid & pciid_check_mask) != pciid_check_value)
    return 0;

  if (check_bus && holy_pci_get_bus (dev) != bus)
    return 0;

  if (check_device && holy_pci_get_device (dev) != device)
    return 0;

  if (check_function && holy_pci_get_function (dev) != function)
    return 0;

  addr = holy_pci_make_address (dev, regaddr);

  switch (regsize)
    {
    case 1:
      regval = holy_pci_read_byte (addr);
      break;

    case 2:
      regval = holy_pci_read_word (addr);
      break;

    case 4:
      regval = holy_pci_read (addr);
      break;
    }

  if (varname)
    {
      char buf[sizeof ("XXXXXXXX")];
      holy_snprintf (buf, sizeof (buf), "%x", regval);
      holy_env_set (varname, buf);
      return 1;
    }

  if (!write_mask)
    {
      holy_printf (_("Register %x of %x:%02x.%x is %x\n"), regaddr,
		   holy_pci_get_bus (dev),
		   holy_pci_get_device (dev),
		   holy_pci_get_function (dev),
		   regval);
      return 0;
    }

  regval = (regval & ~write_mask) | regwrite;

  switch (regsize)
    {
    case 1:
      holy_pci_write_byte (addr, regval);
      break;

    case 2:
      holy_pci_write_word (addr, regval);
      break;

    case 4:
      holy_pci_write (addr, regval);
      break;
    }

  return 0;
}

static holy_err_t
holy_cmd_setpci (holy_extcmd_context_t ctxt, int argc, char **argv)
{
  const char *ptr;
  unsigned i;

  pciid_check_value = 0;
  pciid_check_mask = 0;

  if (ctxt->state[0].set)
    {
      ptr = ctxt->state[0].arg;
      pciid_check_value |= (holy_strtoul (ptr, (char **) &ptr, 16) & 0xffff);
      if (holy_errno == holy_ERR_BAD_NUMBER)
	{
	  holy_errno = holy_ERR_NONE;
	  ptr = ctxt->state[0].arg;
	}
      else
	pciid_check_mask |= 0xffff;
      if (holy_errno)
	return holy_errno;
      if (*ptr != ':')
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("missing `%c' symbol"), ':');
      ptr++;
      pciid_check_value |= (holy_strtoul (ptr, (char **) &ptr, 16) & 0xffff)
	<< 16;
      if (holy_errno == holy_ERR_BAD_NUMBER)
	holy_errno = holy_ERR_NONE;
      else
	pciid_check_mask |= 0xffff0000;
    }

  pciid_check_value &= pciid_check_mask;

  check_bus = check_device = check_function = 0;

  if (ctxt->state[1].set)
    {
      const char *optr;
      
      ptr = ctxt->state[1].arg;
      optr = ptr;
      bus = holy_strtoul (ptr, (char **) &ptr, 16);
      if (holy_errno == holy_ERR_BAD_NUMBER)
	{
	  holy_errno = holy_ERR_NONE;
	  ptr = optr;
	}
      else
	check_bus = 1;
      if (holy_errno)
	return holy_errno;
      if (*ptr != ':')
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("missing `%c' symbol"), ':');
      ptr++;
      optr = ptr;
      device = holy_strtoul (ptr, (char **) &ptr, 16);
      if (holy_errno == holy_ERR_BAD_NUMBER)
	{
	  holy_errno = holy_ERR_NONE;
	  ptr = optr;
	}
      else
	check_device = 1;
      if (*ptr == '.')
	{
	  ptr++;
	  function = holy_strtoul (ptr, (char **) &ptr, 16);
	  if (holy_errno)
	    return holy_errno;
	  check_function = 1;
	}
    }

  if (ctxt->state[2].set)
    varname = ctxt->state[2].arg;
  else
    varname = NULL;

  write_mask = 0;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  ptr = argv[0];

  for (i = 0; i < ARRAY_SIZE (pci_registers); i++)
    {
      if (holy_strncmp (ptr, pci_registers[i].name,
			holy_strlen (pci_registers[i].name)) == 0)
	break;
    }
  if (i == ARRAY_SIZE (pci_registers))
    {
      regsize = 0;
      regaddr = holy_strtoul (ptr, (char **) &ptr, 16);
      if (holy_errno)
	return holy_error (holy_ERR_BAD_ARGUMENT, "unknown register");
    }
  else
    {
      regaddr = pci_registers[i].addr;
      regsize = pci_registers[i].size;
      ptr += holy_strlen (pci_registers[i].name);
    }

  if (holy_errno)
    return holy_errno;

  if (*ptr == '+')
    {
      ptr++;
      regaddr += holy_strtoul (ptr, (char **) &ptr, 16);
      if (holy_errno)
	return holy_errno;
    }

  if (holy_memcmp (ptr, ".L", sizeof (".L") - 1) == 0
      || holy_memcmp (ptr, ".l", sizeof (".l") - 1) == 0)
    {
      regsize = 4;
      ptr += sizeof (".l") - 1;
    }
  else if (holy_memcmp (ptr, ".W", sizeof (".W") - 1) == 0
      || holy_memcmp (ptr, ".w", sizeof (".w") - 1) == 0)
    {
      regsize = 2;
      ptr += sizeof (".w") - 1;
    }
  else if (holy_memcmp (ptr, ".B", sizeof (".B") - 1) == 0
      || holy_memcmp (ptr, ".b", sizeof (".b") - 1) == 0)
    {
      regsize = 1;
      ptr += sizeof (".b") - 1;
    }

  if (!regsize)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "unknown register size");

  write_mask = 0;
  if (*ptr == '=')
    {
      ptr++;
      regwrite = holy_strtoul (ptr, (char **) &ptr, 16);
      if (holy_errno)
	return holy_errno;
      write_mask = 0xffffffff;
      if (*ptr == ':')
	{
	  ptr++;
	  write_mask = holy_strtoul (ptr, (char **) &ptr, 16);
	  if (holy_errno)
	    return holy_errno;
	  write_mask = 0xffffffff;
	}
      regwrite &= write_mask;
    }

  if (write_mask && varname)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "option -v isn't valid for writes");

  holy_pci_iterate (holy_setpci_iter, NULL);
  return holy_ERR_NONE;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(setpci)
{
  cmd = holy_register_extcmd ("setpci", holy_cmd_setpci, 0,
			      N_("[-s POSITION] [-d DEVICE] [-v VAR] "
				 "REGISTER[=VALUE[:MASK]]"),
			      N_("Manipulate PCI devices."), options);
}

holy_MOD_FINI(setpci)
{
  holy_unregister_extcmd (cmd);
}
