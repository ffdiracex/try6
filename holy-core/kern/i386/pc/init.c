/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/mm.h>
#include <holy/machine/boot.h>
#include <holy/i386/floppy.h>
#include <holy/machine/memory.h>
#include <holy/machine/console.h>
#include <holy/machine/kernel.h>
#include <holy/machine/int.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/loader.h>
#include <holy/env.h>
#include <holy/cache.h>
#include <holy/time.h>
#include <holy/cpu/cpuid.h>
#include <holy/cpu/tsc.h>
#include <holy/machine/time.h>

struct mem_region
{
  holy_addr_t addr;
  holy_size_t size;
};

#define MAX_REGIONS	32

static struct mem_region mem_regions[MAX_REGIONS];
static int num_regions;

void (*holy_pc_net_config) (char **device, char **path);

/*
 *	return the real time in ticks, of which there are about
 *	18-20 per second
 */
holy_uint64_t
holy_rtc_get_time_ms (void)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x1a, &regs);

  return ((regs.ecx << 16) | (regs.edx & 0xffff)) * 55ULL;
}

void
holy_machine_get_bootlocation (char **device, char **path)
{
  char *ptr;
  holy_uint8_t boot_drive, dos_part, bsd_part;

  boot_drive = (holy_boot_device >> 24);
  dos_part = (holy_boot_device >> 16);
  bsd_part = (holy_boot_device >> 8);

  /* No hardcoded root partition - make it from the boot drive and the
     partition number encoded at the install time.  */
  if (boot_drive == holy_BOOT_MACHINE_PXE_DL)
    {
      if (holy_pc_net_config)
	holy_pc_net_config (device, path);
      return;
    }

  /* XXX: This should be enough.  */
#define DEV_SIZE 100
  *device = holy_malloc (DEV_SIZE);
  ptr = *device;
  holy_snprintf (*device, DEV_SIZE,
		 "%cd%u", (boot_drive & 0x80) ? 'h' : 'f',
		 boot_drive & 0x7f);
  ptr += holy_strlen (ptr);

  if (dos_part != 0xff)
    holy_snprintf (ptr, DEV_SIZE - (ptr - *device),
		   ",%u", dos_part + 1);
  ptr += holy_strlen (ptr);

  if (bsd_part != 0xff)
    holy_snprintf (ptr, DEV_SIZE - (ptr - *device), ",%u",
		   bsd_part + 1);
  ptr += holy_strlen (ptr);
  *ptr = 0;
}

/* Add a memory region.  */
static void
add_mem_region (holy_addr_t addr, holy_size_t size)
{
  if (num_regions == MAX_REGIONS)
    /* Ignore.  */
    return;

  mem_regions[num_regions].addr = addr;
  mem_regions[num_regions].size = size;
  num_regions++;
}

/* Compact memory regions.  */
static void
compact_mem_regions (void)
{
  int i, j;

  /* Sort them.  */
  for (i = 0; i < num_regions - 1; i++)
    for (j = i + 1; j < num_regions; j++)
      if (mem_regions[i].addr > mem_regions[j].addr)
	{
	  struct mem_region tmp = mem_regions[i];
	  mem_regions[i] = mem_regions[j];
	  mem_regions[j] = tmp;
	}

  /* Merge overlaps.  */
  for (i = 0; i < num_regions - 1; i++)
    if (mem_regions[i].addr + mem_regions[i].size >= mem_regions[i + 1].addr)
      {
	j = i + 1;

	if (mem_regions[i].addr + mem_regions[i].size
	    < mem_regions[j].addr + mem_regions[j].size)
	  mem_regions[i].size = (mem_regions[j].addr + mem_regions[j].size
				 - mem_regions[i].addr);

	holy_memmove (mem_regions + j, mem_regions + j + 1,
		      (num_regions - j - 1) * sizeof (struct mem_region));
	i--;
        num_regions--;
      }
}

holy_addr_t holy_modbase;
extern holy_uint8_t _start[], _edata[];

/* Helper for holy_machine_init.  */
static int
mmap_iterate_hook (holy_uint64_t addr, holy_uint64_t size,
		   holy_memory_type_t type,
		   void *data __attribute__ ((unused)))
{
  /* Avoid the lower memory.  */
  if (addr < holy_MEMORY_MACHINE_UPPER_START)
    {
      if (size <= holy_MEMORY_MACHINE_UPPER_START - addr)
	return 0;

      size -= holy_MEMORY_MACHINE_UPPER_START - addr;
      addr = holy_MEMORY_MACHINE_UPPER_START;
    }

  /* Ignore >4GB.  */
  if (addr <= 0xFFFFFFFF && type == holy_MEMORY_AVAILABLE)
    {
      holy_size_t len;

      len = (holy_size_t) ((addr + size > 0xFFFFFFFF)
	     ? 0xFFFFFFFF - addr
	     : size);
      add_mem_region (addr, len);
    }

  return 0;
}

extern holy_uint16_t holy_bios_via_workaround1, holy_bios_via_workaround2;

/* Via needs additional wbinvd.  */
static void
holy_via_workaround_init (void)
{
  holy_uint32_t manufacturer[3], max_cpuid;
  if (! holy_cpu_is_cpuid_supported ())
    return;

  holy_cpuid (0, max_cpuid, manufacturer[0], manufacturer[2], manufacturer[1]);

  if (holy_memcmp (manufacturer, "CentaurHauls", 12) != 0)
    return;

  holy_bios_via_workaround1 = 0x090f;
  holy_bios_via_workaround2 = 0x090f;
  asm volatile ("wbinvd");
}

void
holy_machine_init (void)
{
  int i;
#if 0
  int holy_lower_mem;
#endif
  holy_addr_t modend;

  /* This has to happen before any BIOS calls. */
  holy_via_workaround_init ();

  holy_modbase = holy_MEMORY_MACHINE_DECOMPRESSION_ADDR + (_edata - _start);

  /* Initialize the console as early as possible.  */
  holy_console_init ();

  /* This sanity check is useless since top of holy_MEMORY_MACHINE_RESERVED_END
     is used for stack and if it's unavailable we wouldn't have gotten so far.
   */
#if 0
  holy_lower_mem = holy_get_conv_memsize () << 10;

  /* Sanity check.  */
  if (holy_lower_mem < holy_MEMORY_MACHINE_RESERVED_END)
    holy_fatal ("too small memory");
#endif

/* FIXME: This prevents loader/i386/linux.c from using low memory.  When our
   heap implements support for requesting a chunk in low memory, this should
   no longer be a problem.  */
#if 0
  /* Add the lower memory into free memory.  */
  if (holy_lower_mem >= holy_MEMORY_MACHINE_RESERVED_END)
    add_mem_region (holy_MEMORY_MACHINE_RESERVED_END,
		    holy_lower_mem - holy_MEMORY_MACHINE_RESERVED_END);
#endif

  holy_machine_mmap_iterate (mmap_iterate_hook, NULL);

  compact_mem_regions ();

  modend = holy_modules_get_end ();
  for (i = 0; i < num_regions; i++)
    {
      holy_addr_t beg = mem_regions[i].addr;
      holy_addr_t fin = mem_regions[i].addr + mem_regions[i].size;
      if (modend && beg < modend)
	beg = modend;
      if (beg >= fin)
	continue;
      holy_mm_init_region ((void *) beg, fin - beg);
    }

  holy_tsc_init ();
}

void
holy_machine_fini (int flags)
{
  if (flags & holy_LOADER_FLAG_NORETURN)
    holy_console_fini ();
  holy_stop_floppy ();
}
