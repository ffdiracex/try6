/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/mm.h>
#include <holy/machine/time.h>
#include <holy/machine/memory.h>
#include <holy/machine/console.h>
#include <holy/offsets.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/loader.h>
#include <holy/env.h>
#include <holy/cache.h>
#include <holy/time.h>
#include <holy/symbol.h>
#include <holy/cpu/io.h>
#include <holy/cpu/floppy.h>
#include <holy/cpu/tsc.h>
#include <holy/video.h>

extern holy_uint8_t _start[];
extern holy_uint8_t _end[];
extern holy_uint8_t _edata[];

void  __attribute__ ((noreturn))
holy_exit (void)
{
  /* We can't use holy_fatal() in this function.  This would create an infinite
     loop, since holy_fatal() calls holy_abort() which in turn calls holy_exit().  */
  while (1)
    holy_cpu_idle ();
}

holy_addr_t holy_modbase = holy_KERNEL_I386_COREBOOT_MODULES_ADDR;
static holy_uint64_t modend;
static int have_memory = 0;

/* Helper for holy_machine_init.  */
static int
heap_init (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	   void *data __attribute__ ((unused)))
{
  holy_uint64_t begin = addr, end = addr + size;

#if holy_CPU_SIZEOF_VOID_P == 4
  /* Restrict ourselves to 32-bit memory space.  */
  if (begin > holy_ULONG_MAX)
    return 0;
  if (end > holy_ULONG_MAX)
    end = holy_ULONG_MAX;
#endif

  if (type != holy_MEMORY_AVAILABLE)
    return 0;

  /* Avoid the lower memory.  */
  if (begin < holy_MEMORY_MACHINE_LOWER_SIZE)
    begin = holy_MEMORY_MACHINE_LOWER_SIZE;

  if (modend && begin < modend)
    begin = modend;
  
  if (end <= begin)
    return 0;

  holy_mm_init_region ((void *) (holy_addr_t) begin, (holy_size_t) (end - begin));

  have_memory = 1;

  return 0;
}

#ifndef holy_MACHINE_MULTIBOOT

void
holy_machine_init (void)
{
  modend = holy_modules_get_end ();

  holy_video_coreboot_fb_early_init ();

  holy_vga_text_init ();

  holy_machine_mmap_iterate (heap_init, NULL);
  if (!have_memory)
    holy_fatal ("No memory found");

  holy_video_coreboot_fb_late_init ();

  holy_font_init ();
  holy_gfxterm_init ();

  holy_tsc_init ();
}

#else

void
holy_machine_init (void)
{
  modend = holy_modules_get_end ();

  holy_vga_text_init ();

  holy_machine_mmap_init ();
  holy_machine_mmap_iterate (heap_init, NULL);

  holy_tsc_init ();
}

#endif

void
holy_machine_get_bootlocation (char **device __attribute__ ((unused)),
			       char **path __attribute__ ((unused)))
{
}

void
holy_machine_fini (int flags)
{
  if (flags & holy_LOADER_FLAG_NORETURN)
    holy_vga_text_fini ();
  holy_stop_floppy ();
}
