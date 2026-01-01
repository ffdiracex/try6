/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/command.h>
#include <holy/extcmd.h>
#include <holy/i386/cpuid.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    /* TRANSLATORS: "(default)" at the end means that this option is used if
       no argument is specified.  */
    {"long-mode", 'l', 0, N_("Check if CPU supports 64-bit (long) mode (default)."), 0, 0},
    {"pae", 'p', 0, N_("Check if CPU supports Physical Address Extension."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

enum
  {
    MODE_LM = 0,
    MODE_PAE = 1
  };

enum
  {
    bit_PAE = (1 << 6),
  };
enum
  {
    bit_LM = (1 << 29)
  };

unsigned char holy_cpuid_has_longmode = 0, holy_cpuid_has_pae = 0;

static holy_err_t
holy_cmd_cpuid (holy_extcmd_context_t ctxt,
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  int val = 0;
  if (ctxt->state[MODE_PAE].set)
    val = holy_cpuid_has_pae;
  else
    val = holy_cpuid_has_longmode;
  return val ? holy_ERR_NONE
    /* TRANSLATORS: it's a standalone boolean value,
       opposite of "true".  */
    : holy_error (holy_ERR_TEST_FAILURE, N_("false"));
}

static holy_extcmd_t cmd;

holy_MOD_INIT(cpuid)
{
#ifdef __x86_64__
  /* holy-emu */
  holy_cpuid_has_longmode = 1;
  holy_cpuid_has_pae = 1;
#else
  unsigned int eax, ebx, ecx, edx;
  unsigned int max_level;
  unsigned int ext_level;

  /* See if we can use cpuid.  */
  asm volatile ("pushfl; pushfl; popl %0; movl %0,%1; xorl %2,%0;"
		"pushl %0; popfl; pushfl; popl %0; popfl"
		: "=&r" (eax), "=&r" (ebx)
		: "i" (0x00200000));
  if (((eax ^ ebx) & 0x00200000) == 0)
    goto done;

  /* Check the highest input value for eax.  */
  holy_cpuid (0, eax, ebx, ecx, edx);
  /* We only look at the first four characters.  */
  max_level = eax;
  if (max_level == 0)
    goto done;

  if (max_level >= 1)
    {
      holy_cpuid (1, eax, ebx, ecx, edx);
      holy_cpuid_has_pae = !!(edx & bit_PAE);
    }

  holy_cpuid (0x80000000, eax, ebx, ecx, edx);
  ext_level = eax;
  if (ext_level < 0x80000000)
    goto done;

  holy_cpuid (0x80000001, eax, ebx, ecx, edx);
  holy_cpuid_has_longmode = !!(edx & bit_LM);
done:
#endif

  cmd = holy_register_extcmd ("cpuid", holy_cmd_cpuid, 0,
			      "[-l]", N_("Check for CPU features."), options);
}

holy_MOD_FINI(cpuid)
{
  holy_unregister_extcmd (cmd);
}
