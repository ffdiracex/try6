/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/int.h>
#include <holy/machine/apm.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

int
holy_apm_get_info (struct holy_apm_info *info)
{
  struct holy_bios_int_registers regs;

  /* detect APM */
  regs.eax = 0x5300;
  regs.ebx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);
  
  if (regs.flags & holy_CPU_INT_FLAGS_CARRY)
    return 0;
  info->version = regs.eax & 0xffff;
  info->flags = regs.ecx & 0xffff;

  /* disconnect APM first */
  regs.eax = 0x5304;
  regs.ebx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);

  /* connect APM */
  regs.eax = 0x5303;
  regs.ebx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);

  if (regs.flags & holy_CPU_INT_FLAGS_CARRY)
    return 0;

  info->cseg = regs.eax & 0xffff;
  info->offset = regs.ebx;
  info->cseg_16 = regs.ecx & 0xffff;
  info->dseg = regs.edx & 0xffff;
  info->cseg_len = regs.esi >> 16;
  info->cseg_16_len = regs.esi & 0xffff;
  info->dseg_len = regs.edi;

  return 1;
}

static holy_err_t
holy_cmd_lsapm (holy_command_t cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)), char **args __attribute__ ((unused)))
{
  struct holy_apm_info info;
  if (!holy_apm_get_info (&info))
    return holy_error (holy_ERR_IO, N_("no APM found"));

  holy_printf_ (N_("Version %u.%u\n"
		   "32-bit CS = 0x%x, len = 0x%x, offset = 0x%x\n"
		   "16-bit CS = 0x%x, len = 0x%x\n"
		   "DS = 0x%x, len = 0x%x\n"),
		info.version >> 8, info.version & 0xff,
		info.cseg, info.cseg_len, info.offset,
		info.cseg_16, info.cseg_16_len,
		info.dseg, info.dseg_len);
  holy_xputs (info.flags & holy_APM_FLAGS_16BITPROTECTED_SUPPORTED
	      ? _("16-bit protected interface supported\n")
	      : _("16-bit protected interface unsupported\n"));
  holy_xputs (info.flags & holy_APM_FLAGS_32BITPROTECTED_SUPPORTED
	      ? _("32-bit protected interface supported\n")
	      : _("32-bit protected interface unsupported\n"));
  holy_xputs (info.flags & holy_APM_FLAGS_CPUIDLE_SLOWS_DOWN
	      ? _("CPU Idle slows down processor\n")
	      : _("CPU Idle doesn't slow down processor\n"));
  holy_xputs (info.flags & holy_APM_FLAGS_DISABLED
	      ? _("APM disabled\n") : _("APM enabled\n"));
  holy_xputs (info.flags & holy_APM_FLAGS_DISENGAGED
	      ? _("APM disengaged\n") : _("APM engaged\n"));

  return holy_ERR_NONE;
}

static holy_command_t cmd;

holy_MOD_INIT(lsapm)
{
  cmd = holy_register_command ("lsapm", holy_cmd_lsapm, 0,
			      N_("Show APM information."));
}

holy_MOD_FINI(lsapm)
{
  holy_unregister_command (cmd);
}


