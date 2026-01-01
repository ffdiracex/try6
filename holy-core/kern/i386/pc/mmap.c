/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/machine/int.h>
#include <holy/err.h>
#include <holy/types.h>
#include <holy/misc.h>

struct holy_machine_mmap_entry
{
  holy_uint32_t size;
  holy_uint64_t addr;
  holy_uint64_t len;
#define holy_MACHINE_MEMORY_AVAILABLE	1
#define holy_MACHINE_MEMORY_RESERVED	2
#define holy_MACHINE_MEMORY_ACPI	3
#define holy_MACHINE_MEMORY_NVS 	4
#define holy_MACHINE_MEMORY_BADRAM 	5
  holy_uint32_t type;
} holy_PACKED;


/*
 *
 * holy_get_conv_memsize(i) :  return the conventional memory size in KB.
 *	BIOS call "INT 12H" to get conventional memory size
 *      The return value in AX.
 */
static inline holy_uint16_t
holy_get_conv_memsize (void)
{
  struct holy_bios_int_registers regs;

  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x12, &regs);
  return regs.eax & 0xffff;
}

/*
 * holy_get_ext_memsize() :  return the extended memory size in KB.
 *	BIOS call "INT 15H, AH=88H" to get extended memory size
 *	The return value in AX.
 *
 */
static inline holy_uint16_t
holy_get_ext_memsize (void)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x8800;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x15, &regs);
  return regs.eax & 0xffff;
}

/* Get a packed EISA memory map. Lower 16 bits are between 1MB and 16MB
   in 1KB parts, and upper 16 bits are above 16MB in 64KB parts. If error, return zero.
   BIOS call "INT 15H, AH=E801H" to get EISA memory map,
     AX = memory between 1M and 16M in 1K parts.
     BX = memory above 16M in 64K parts. 
*/
 
static inline holy_uint32_t
holy_get_eisa_mmap (void)
{
  struct holy_bios_int_registers regs;

  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  regs.eax = 0xe801;
  holy_bios_interrupt (0x15, &regs);

  if ((regs.eax & 0xff00) == 0x8600)
    return 0;

  return (regs.eax & 0xffff) | (regs.ebx << 16);
}

/*
 *
 * holy_get_mmap_entry(addr, cont) : address and old continuation value (zero to
 *		start), for the Query System Address Map BIOS call.
 *
 *  Sets the first 4-byte int value of "addr" to the size returned by
 *  the call.  If the call fails, sets it to zero.
 *
 *	Returns:  new (non-zero) continuation value, 0 if done.
 */
/* Get a memory map entry. Return next continuation value. Zero means
   the end.  */
static holy_uint32_t
holy_get_mmap_entry (struct holy_machine_mmap_entry *entry,
		     holy_uint32_t cont)
{
  struct holy_bios_int_registers regs;

  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;

  /* place address (+4) in ES:DI */
  regs.es = ((holy_addr_t) &entry->addr) >> 4;
  regs.edi = ((holy_addr_t) &entry->addr) & 0xf;
	
  /* set continuation value */
  regs.ebx = cont;

  /* set default maximum buffer size */
  regs.ecx = sizeof (*entry) - sizeof (entry->size);

  /* set EDX to 'SMAP' */
  regs.edx = 0x534d4150;

  regs.eax = 0xe820;
  holy_bios_interrupt (0x15, &regs);

  /* write length of buffer (zero if error) into ADDR */	
  if ((regs.flags & holy_CPU_INT_FLAGS_CARRY) || regs.eax != 0x534d4150
      || regs.ecx < 0x14 || regs.ecx > 0x400)
    entry->size = 0;
  else
    entry->size = regs.ecx;

  /* return the continuation value */
  return regs.ebx;
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  holy_uint32_t cont = 0;
  struct holy_machine_mmap_entry *entry
    = (struct holy_machine_mmap_entry *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
  int e820_works = 0;

  while (1)
    {
      holy_memset (entry, 0, sizeof (*entry));

      cont = holy_get_mmap_entry (entry, cont);

      if (!entry->size)
	break;

      if (entry->len)
	e820_works = 1;
      if (entry->len
	  && hook (entry->addr, entry->len,
		   /* holy mmaps have been defined to match with
		      the E820 definition.
		      Therefore, we can just pass type through.  */
		   entry->type, hook_data))
	break;

      if (! cont)
	break;
    }

  if (!e820_works)
    {
      holy_uint32_t eisa_mmap = holy_get_eisa_mmap ();

      if (hook (0x0, ((holy_uint32_t) holy_get_conv_memsize ()) << 10,
		holy_MEMORY_AVAILABLE, hook_data))
	return 0;

      if (eisa_mmap)
	{
	  if (hook (0x100000, (eisa_mmap & 0xFFFF) << 10,
		    holy_MEMORY_AVAILABLE, hook_data) == 0)
	    hook (0x1000000, eisa_mmap & ~0xFFFF, holy_MEMORY_AVAILABLE,
		  hook_data);
	}
      else
	hook (0x100000, ((holy_uint32_t) holy_get_ext_memsize ()) << 10,
	      holy_MEMORY_AVAILABLE, hook_data);
    }

  return 0;
}
