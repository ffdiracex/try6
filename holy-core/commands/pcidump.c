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

struct iter_cxt
{
  holy_uint32_t pciid_check_mask, pciid_check_value;
  int bus, device, function;
  int check_bus, check_device, check_function;
};

static const struct holy_arg_option options[] =
  {
    {0, 'd', 0, N_("Select device by vendor and device IDs."),
     N_("[vendor]:[device]"), ARG_TYPE_STRING},
    {0, 's', 0, N_("Select device by its position on the bus."),
     N_("[bus]:[slot][.func]"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static int
holy_pcidump_iter (holy_pci_device_t dev, holy_pci_id_t pciid,
		  void *data)
{
  struct iter_cxt *ctx = data;
  holy_pci_address_t addr;
  int i;

  if ((pciid & ctx->pciid_check_mask) != ctx->pciid_check_value)
    return 0;

  if (ctx->check_bus && holy_pci_get_bus (dev) != ctx->bus)
    return 0;

  if (ctx->check_device && holy_pci_get_device (dev) != ctx->device)
    return 0;

  if (ctx->check_function && holy_pci_get_function (dev) != ctx->function)
    return 0;

  for (i = 0; i < 256; i += 4)
    {
      addr = holy_pci_make_address (dev, i);
      holy_printf ("%08x ", holy_pci_read (addr));
      if ((i & 0xc) == 0xc)
	holy_printf ("\n");
    }

  return 0;
}

static holy_err_t
holy_cmd_pcidump (holy_extcmd_context_t ctxt,
		  int argc __attribute__ ((unused)),
		  char **argv __attribute__ ((unused)))
{
  const char *ptr;
  struct iter_cxt ctx =
    {
      .pciid_check_value = 0,
      .pciid_check_mask = 0,
      .check_bus = 0,
      .check_device = 0,
      .check_function = 0,
      .bus = 0,
      .function = 0,
      .device = 0
    };

  if (ctxt->state[0].set)
    {
      ptr = ctxt->state[0].arg;
      ctx.pciid_check_value |= (holy_strtoul (ptr, (char **) &ptr, 16) & 0xffff);
      if (holy_errno == holy_ERR_BAD_NUMBER)
	{
	  holy_errno = holy_ERR_NONE;
	  ptr = ctxt->state[0].arg;
	}
      else
	ctx.pciid_check_mask |= 0xffff;
      if (holy_errno)
	return holy_errno;
      if (*ptr != ':')
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("missing `%c' symbol"), ':');
      ptr++;
      ctx.pciid_check_value |= (holy_strtoul (ptr, (char **) &ptr, 16) & 0xffff)
	<< 16;
      if (holy_errno == holy_ERR_BAD_NUMBER)
	holy_errno = holy_ERR_NONE;
      else
	ctx.pciid_check_mask |= 0xffff0000;
    }

  ctx.pciid_check_value &= ctx.pciid_check_mask;

  if (ctxt->state[1].set)
    {
      const char *optr;
      
      ptr = ctxt->state[1].arg;
      optr = ptr;
      ctx.bus = holy_strtoul (ptr, (char **) &ptr, 16);
      if (holy_errno == holy_ERR_BAD_NUMBER)
	{
	  holy_errno = holy_ERR_NONE;
	  ptr = optr;
	}
      else
	ctx.check_bus = 1;
      if (holy_errno)
	return holy_errno;
      if (*ptr != ':')
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("missing `%c' symbol"), ':');
      ptr++;
      optr = ptr;
      ctx.device = holy_strtoul (ptr, (char **) &ptr, 16);
      if (holy_errno == holy_ERR_BAD_NUMBER)
	{
	  holy_errno = holy_ERR_NONE;
	  ptr = optr;
	}
      else
	ctx.check_device = 1;
      if (*ptr == '.')
	{
	  ptr++;
	  ctx.function = holy_strtoul (ptr, (char **) &ptr, 16);
	  if (holy_errno)
	    return holy_errno;
	  ctx.check_function = 1;
	}
    }

  holy_pci_iterate (holy_pcidump_iter, &ctx);
  return holy_ERR_NONE;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(pcidump)
{
  cmd = holy_register_extcmd ("pcidump", holy_cmd_pcidump, 0,
			      N_("[-s POSITION] [-d DEVICE]"),
			      N_("Show raw dump of the PCI configuration space."), options);
}

holy_MOD_FINI(pcidump)
{
  holy_unregister_extcmd (cmd);
}
