/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/env.h>
#include <holy/kernel.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/offsets.h>
#include <holy/term.h>
#include <holy/time.h>
#include <holy/machine/kernel.h>
#include <holy/uboot/console.h>
#include <holy/uboot/disk.h>
#include <holy/uboot/uboot.h>
#include <holy/uboot/api_public.h>
#include <holy/cpu/system.h>
#include <holy/cache.h>

extern char __bss_start[];
extern char _end[];
extern holy_size_t holy_total_module_size;
extern int (*holy_uboot_syscall_ptr) (int, int *, ...);
static unsigned long timer_start;

extern holy_uint32_t holy_uboot_machine_type;
extern holy_addr_t holy_uboot_boot_data;

void
holy_exit (void)
{
  holy_uboot_return (0);
}

holy_uint32_t
holy_uboot_get_machine_type (void)
{
  return holy_uboot_machine_type;
}

holy_addr_t
holy_uboot_get_boot_data (void)
{
  return holy_uboot_boot_data;
}

static holy_uint64_t
uboot_timer_ms (void)
{
  static holy_uint32_t last = 0, high = 0;
  holy_uint32_t cur = holy_uboot_get_timer (timer_start);
  if (cur < last)
    high++;
  last = cur;
  return (((holy_uint64_t) high) << 32) | cur;
}

#ifdef __arm__
static holy_uint64_t
rpi_timer_ms (void)
{
  static holy_uint32_t last = 0, high = 0;
  holy_uint32_t cur = *(volatile holy_uint32_t *) 0x20003004;
  if (cur < last)
    high++;
  last = cur;
  return holy_divmod64 ((((holy_uint64_t) high) << 32) | cur, 1000, 0);
}
#endif

void
holy_machine_init (void)
{
  int ver;

  /* First of all - establish connection with U-Boot */
  ver = holy_uboot_api_init ();
  if (!ver)
    {
      /* Don't even have a console to log errors to... */
      holy_exit ();
    }
  else if (ver > API_SIG_VERSION)
    {
      /* Try to print an error message */
      holy_uboot_puts ("invalid U-Boot API version\n");
    }

  /* Initialize the console so that holy can display messages.  */
  holy_console_init_early ();

  /* Enumerate memory and initialize the memory management system. */
  holy_uboot_mm_init ();

  /* Should be earlier but it needs memalign.  */
#ifdef __arm__
  holy_arm_enable_caches_mmu ();
#endif

  holy_dprintf ("init", "__bss_start: %p\n", __bss_start);
  holy_dprintf ("init", "_end: %p\n", _end);
  holy_dprintf ("init", "holy_modbase: %p\n", (void *) holy_modbase);
  holy_dprintf ("init", "holy_modules_get_end(): %p\n",
		(void *) holy_modules_get_end ());

  /* Initialise full terminfo support */
  holy_console_init_lately ();

  /* Enumerate uboot devices */
  holy_uboot_probe_hardware ();

  /* Initialise timer */
#ifdef __arm__
  if (holy_uboot_get_machine_type () == holy_ARM_MACHINE_TYPE_RASPBERRY_PI)
    {
      holy_install_get_time_ms (rpi_timer_ms);
    }
  else
#endif
    {
      timer_start = holy_uboot_get_timer (0);
      holy_install_get_time_ms (uboot_timer_ms);
    }

  /* Initialize  */
  holy_ubootdisk_init ();
}


void
holy_machine_fini (int flags __attribute__ ((unused)))
{
}

/*
 * holy_machine_get_bootlocation():
 *   Called from kern/main.c, which expects a device name (minus parentheses)
 *   and a filesystem path back, if any are known.
 *   Any returned values must be pointers to dynamically allocated strings.
 */
void
holy_machine_get_bootlocation (char **device, char **path)
{
  char *tmp;

  tmp = holy_uboot_env_get ("holy_bootdev");
  if (tmp)
    {
      *device = holy_strdup (tmp);
      if (*device == NULL)
	return;
    }
  else
    *device = NULL;

  tmp = holy_uboot_env_get ("holy_bootpath");
  if (tmp)
    {
      *path = holy_strdup (tmp);
      if (*path == NULL)
	return;
    }
  else
    *path = NULL;
}

void
holy_uboot_fini (void)
{
  holy_ubootdisk_fini ();
  holy_console_fini ();
}
