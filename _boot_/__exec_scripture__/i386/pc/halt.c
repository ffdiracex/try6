/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/machine/int.h>
#include <holy/acpi.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    {"no-apm", 'n', 0, N_("Do not use APM to halt the computer."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static inline void __attribute__ ((noreturn))
stop (void)
{
  while (1)
    {
      asm volatile ("hlt");
    }
}
/*
 * Halt the system, using APM if possible. If NO_APM is true, don't use
 * APM even if it is available.
 */
void  __attribute__ ((noreturn))
holy_halt (int no_apm)
{
  struct holy_bios_int_registers regs;

  holy_acpi_halt ();

  if (no_apm)
    stop ();

  /* detect APM */
  regs.eax = 0x5300;
  regs.ebx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);
  
  if (regs.flags & holy_CPU_INT_FLAGS_CARRY)
    stop ();

  /* disconnect APM first */
  regs.eax = 0x5304;
  regs.ebx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);

  /* connect APM */
  regs.eax = 0x5301;
  regs.ebx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);
  if (regs.flags & holy_CPU_INT_FLAGS_CARRY)
    stop ();

  /* set APM protocol level - 1.1 or bust. (this covers APM 1.2 also) */
  regs.eax = 0x530E;
  regs.ebx = 0;
  regs.ecx = 0x0101;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);
  if (regs.flags & holy_CPU_INT_FLAGS_CARRY)
    stop ();

  /* set the power state to off */
  regs.eax = 0x5307;
  regs.ebx = 1;
  regs.ecx = 3;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);

  /* shouldn't reach here */
  stop ();
}

static holy_err_t __attribute__ ((noreturn))
holy_cmd_halt (holy_extcmd_context_t ctxt,
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))

{
  struct holy_arg_list *state = ctxt->state;
  int no_apm = 0;

  if (state[0].set)
    no_apm = 1;
  holy_halt (no_apm);
}

static holy_extcmd_t cmd;

holy_MOD_INIT(halt)
{
  cmd = holy_register_extcmd ("halt", holy_cmd_halt, 0, "[-n]",
			      N_("Halt the system, if possible using APM."),
			      options);
}

holy_MOD_FINI(halt)
{
  holy_unregister_extcmd (cmd);
}
