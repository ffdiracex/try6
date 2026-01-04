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
#include <holy/time.h>
#include <holy/machine/memory.h>
#include <holy/machine/kernel.h>
#include <holy/machine/console.h>
#include <holy/cpu/memory.h>
#include <holy/memory.h>
#include <holy/video.h>
#include <holy/terminfo.h>
#include <holy/keyboard_layouts.h>
#include <holy/serial.h>
#include <holy/loader.h>
#include <holy/at_keyboard.h>

static inline int
probe_mem (holy_addr_t addr)
{
  volatile holy_uint8_t *ptr = (holy_uint8_t *) (0xa0000000 | addr);
  holy_uint8_t c = *ptr;
  *ptr = 0xAA;
  if (*ptr != 0xAA)
    return 0;
  *ptr = 0x55;
  if (*ptr != 0x55)
    return 0;
  *ptr = c;
  return 1;
}

void
holy_machine_init (void)
{
  holy_addr_t modend;

  if (holy_arch_memsize == 0)
    {
      int i;
      
      for (i = 27; i >= 0; i--)
	if (probe_mem (holy_arch_memsize | (1 << i)))
	  holy_arch_memsize |= (1 << i);
      holy_arch_memsize++;
    }

  /* FIXME: measure this.  */
  holy_arch_cpuclock = 200000000;

  modend = holy_modules_get_end ();
  holy_mm_init_region ((void *) modend, holy_arch_memsize
		       - (modend - holy_ARCH_LOWMEMVSTART));

  holy_install_get_time_ms (holy_rtc_get_time_ms);

  holy_keylayouts_init ();
  holy_at_keyboard_init ();

  holy_qemu_init_cirrus ();
  holy_vga_text_init ();

  holy_terminfo_init ();
  holy_serial_init ();

  holy_boot_init ();
}

void
holy_machine_fini (int flags __attribute__ ((unused)))
{
}

void
holy_exit (void)
{
  holy_halt ();
}

void
holy_halt (void)
{
  holy_outl (42, 0xbfbf0004);
  while (1);
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  hook (0, holy_arch_memsize, holy_MEMORY_AVAILABLE, hook_data);
  return holy_ERR_NONE;
}

void
holy_machine_get_bootlocation (char **device __attribute__ ((unused)),
			       char **path __attribute__ ((unused)))
{
}

extern char _end[];
holy_addr_t holy_modbase = (holy_addr_t) _end;

