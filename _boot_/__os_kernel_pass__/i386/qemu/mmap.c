/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/i386/memory.h>
#include <holy/machine/memory.h>
#include <holy/machine/boot.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/cmos.h>
#include <holy/offsets.h>

#define QEMU_CMOS_MEMSIZE_HIGH		0x35
#define QEMU_CMOS_MEMSIZE_LOW		0x34

#define QEMU_CMOS_MEMSIZE2_HIGH		0x31
#define QEMU_CMOS_MEMSIZE2_LOW		0x30

#define min(a,b)	((a) > (b) ? (b) : (a))

extern char _start[];
extern char _end[];

static holy_uint64_t mem_size, above_4g;

void
holy_machine_mmap_init (void)
{
  holy_uint8_t high, low, b, c, d;
  holy_cmos_read (QEMU_CMOS_MEMSIZE_HIGH, &high);
  holy_cmos_read (QEMU_CMOS_MEMSIZE_LOW, &low);
  mem_size = ((holy_uint64_t) high) << 24
    | ((holy_uint64_t) low) << 16;
  if (mem_size > 0)
    {
      /* Don't ask... */
      mem_size += (16 * 1024 * 1024);
    }
  else
    {
      holy_cmos_read (QEMU_CMOS_MEMSIZE2_HIGH, &high);
      holy_cmos_read (QEMU_CMOS_MEMSIZE2_LOW, &low);
      mem_size
	= ((((holy_uint64_t) high) << 18) | (((holy_uint64_t) low) << 10))
	+ 1024 * 1024;
    }

  holy_cmos_read (0x5b, &b);
  holy_cmos_read (0x5c, &c);
  holy_cmos_read (0x5d, &d);
  above_4g = (((holy_uint64_t) b) << 16)
    | (((holy_uint64_t) c) << 24)
    | (((holy_uint64_t) d) << 32);
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  if (hook (0x0,
	    (holy_addr_t) _start,
	    holy_MEMORY_AVAILABLE, hook_data))
    return 1;

  if (hook ((holy_addr_t) _end,
           0xa0000 - (holy_addr_t) _end,
           holy_MEMORY_AVAILABLE, hook_data))
    return 1;

  if (hook (holy_MEMORY_MACHINE_UPPER,
	    0x100000 - holy_MEMORY_MACHINE_UPPER,
	    holy_MEMORY_RESERVED, hook_data))
    return 1;

  /* Everything else is free.  */
  if (hook (0x100000,
	    min (mem_size, (holy_uint32_t) -holy_BOOT_MACHINE_SIZE) - 0x100000,
	    holy_MEMORY_AVAILABLE, hook_data))
    return 1;

  /* Protect boot.img, which contains the gdt.  It is mapped at the top of memory
     (it is also mapped below 0x100000, but we already reserved that area).  */
  if (hook ((holy_uint32_t) -holy_BOOT_MACHINE_SIZE,
	    holy_BOOT_MACHINE_SIZE,
	    holy_MEMORY_RESERVED, hook_data))
    return 1;

  if (above_4g != 0 && hook (0x100000000ULL, above_4g,
			     holy_MEMORY_AVAILABLE, hook_data))
    return 1;

  return 0;
}
