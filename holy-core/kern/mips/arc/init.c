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
#include <holy/machine/kernel.h>
#include <holy/machine/memory.h>
#include <holy/arc/console.h>
#include <holy/cpu/memory.h>
#include <holy/cpu/time.h>
#include <holy/memory.h>
#include <holy/term.h>
#include <holy/arc/arc.h>
#include <holy/offsets.h>
#include <holy/i18n.h>
#include <holy/disk.h>
#include <holy/partition.h>

const char *type_names[] = {
#ifdef holy_CPU_WORDS_BIGENDIAN
  NULL,
#endif
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "eisa", "tc", "scsi", "dti", "multi", "disk", "tape", "cdrom", "worm",
  "serial", "net", "video", "par", "point", "key", "audio", "other",
  "rdisk", "fdisk", "tape", "modem", "monitor", "print", "pointer",
  "keyboard", "term",
#ifndef holy_CPU_WORDS_BIGENDIAN
  "other",
#endif
  "line", "network", NULL
};

static int
iterate_rec (const char *prefix, const struct holy_arc_component *parent,
	     holy_arc_iterate_devs_hook_t hook, void *hook_data,
	     int alt_names)
{
  const struct holy_arc_component *comp;
  FOR_ARC_CHILDREN(comp, parent)
    {
      char *name;
      const char *cname = NULL;
      if (comp->type < ARRAY_SIZE (type_names))
	cname = type_names[comp->type];
      if (!cname)
	cname = "unknown";
      if (alt_names)
	name = holy_xasprintf ("%s/%s%lu", prefix, cname, comp->key);
      else
	name = holy_xasprintf ("%s%s(%lu)", prefix, cname, comp->key);
      if (!name)
	return 1;
      if (hook (name, comp, hook_data))
	{
	  holy_free (name);
	  return 1;
	}
      if (iterate_rec ((parent ? name : prefix), comp, hook, hook_data,
		       alt_names))
	{
	  holy_free (name);
	  return 1;
	}
      holy_free (name);
    }
  return 0;
}

int
holy_arc_iterate_devs (holy_arc_iterate_devs_hook_t hook, void *hook_data,
		       int alt_names)
{
  return iterate_rec ((alt_names ? "arc" : ""), NULL, hook, hook_data,
		      alt_names);
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  struct holy_arc_memory_descriptor *cur = NULL;
  while (1)
    {
      holy_memory_type_t type;
      cur = holy_ARC_FIRMWARE_VECTOR->getmemorydescriptor (cur);
      if (!cur)
	return holy_ERR_NONE;
      switch (cur->type)
	{
	case holy_ARC_MEMORY_EXCEPTION_BLOCK:
	case holy_ARC_MEMORY_SYSTEM_PARAMETER_BLOCK:
	case holy_ARC_MEMORY_FW_PERMANENT:
	default:
	  type = holy_MEMORY_RESERVED;
	  break;

	case holy_ARC_MEMORY_FW_TEMPORARY:
	case holy_ARC_MEMORY_FREE:
	case holy_ARC_MEMORY_LOADED:
	case holy_ARC_MEMORY_FREE_CONTIGUOUS:
	  type = holy_MEMORY_AVAILABLE;
	  break;
	case holy_ARC_MEMORY_BADRAM:
	  type = holy_MEMORY_BADRAM;
	  break;
	}
      if (hook (((holy_uint64_t) cur->start_page) << 12,
		((holy_uint64_t) cur->num_pages)  << 12, type, hook_data))
	return holy_ERR_NONE;
    }
}

char *
holy_arc_alt_name_to_norm (const char *name, const char *suffix)
{
  char *optr;
  const char *iptr;
  char * ret = holy_malloc (2 * holy_strlen (name) + holy_strlen (suffix));
  int state = 0;

  if (!ret)
    return NULL;
  optr = ret;
  for (iptr = name + 4; *iptr; iptr++)
    if (state == 0)
      {
	if (!holy_isdigit (*iptr))
	  *optr++ = *iptr;
	else
	  {
	    *optr++ = '(';
	    *optr++ = *iptr;
	    state = 1;
	  }
      }
    else
      {
	if (holy_isdigit (*iptr))
	  *optr++ = *iptr;
	else
	  {
	    *optr++ = ')';
	    state = 0;
	  }
      }
  if (state)
    *optr++ = ')';
  holy_strcpy (optr, suffix);
  return ret;
}

static char *
norm_name_to_alt (const char *name)
{
  char *optr;
  const char *iptr;
  int state = 0;
  char * ret = holy_malloc (holy_strlen (name) + sizeof ("arc/"));

  if (!ret)
    return NULL;
  optr = holy_stpcpy (ret, "arc/");
  for (iptr = name; *iptr; iptr++)
    {
      if (state == 3)
	{
	  *optr++ = '/';
	  state = 0;
	}
      if (*iptr == '(')
	{
	  state = 1;
	  continue;
	}
      if (*iptr == ')')
	{
	  if (state == 1)
	    *optr++ = '0';
	  state = 3;
	  continue;
	}
      *optr++ = *iptr;
      if (state == 1)
	state = 2;
    }
  *optr = '\0';
  return ret;
}

extern holy_uint32_t holy_total_modules_size __attribute__ ((section(".text")));
holy_addr_t holy_modbase;

extern char _end[];
static char boot_location[256];

void
holy_machine_init (void)
{
  struct holy_arc_memory_descriptor *cur = NULL;
  holy_addr_t modend;

  holy_memcpy (boot_location,
	       (char *) (holy_DECOMPRESSOR_LINK_ADDR - 256), 256);

  holy_modbase = ALIGN_UP ((holy_addr_t) _end, holy_KERNEL_MACHINE_MOD_ALIGN);
  modend = holy_modbase + holy_total_modules_size;
  holy_console_init_early ();

  /* FIXME: measure this.  */
  holy_arch_cpuclock = 150000000;
  holy_install_get_time_ms (holy_rtc_get_time_ms);

  while (1)
    {
      holy_uint64_t start, end;
      cur = holy_ARC_FIRMWARE_VECTOR->getmemorydescriptor (cur);
      if (!cur)
	break;
      if (cur->type != holy_ARC_MEMORY_FREE
	  && cur->type != holy_ARC_MEMORY_LOADED
	  && cur->type != holy_ARC_MEMORY_FREE_CONTIGUOUS)
	continue;
      start = ((holy_uint64_t) cur->start_page) << 12;
      end = ((holy_uint64_t) cur->num_pages)  << 12;
      end += start;
      if ((holy_uint64_t) start < (modend & 0x1fffffff))
	start = (modend & 0x1fffffff);
      if ((holy_uint64_t) end > 0x20000000)
	end = 0x20000000;
      if (end > start)
	holy_mm_init_region ((void *) (holy_addr_t) (start | 0x80000000),
			     end - start);
    }

  holy_console_init_lately ();

  holy_arcdisk_init ();
}

void
holy_machine_fini (int flags __attribute__ ((unused)))
{
}

void
holy_halt (void)
{
  holy_ARC_FIRMWARE_VECTOR->powerdown ();

  holy_millisleep (1500);

  holy_puts_ (N_("Shutdown failed"));
  holy_refresh ();
  while (1);
}

void
holy_exit (void)
{
  holy_ARC_FIRMWARE_VECTOR->exit ();

  holy_millisleep (1500);

  holy_puts_ (N_("Exit failed"));
  holy_refresh ();
  while (1);
}

static char *
get_part (char *dev)
{
  char *ptr;
  if (!*dev)
    return 0;
  ptr = dev + holy_strlen (dev) - 1;
  if (ptr == dev || *ptr != ')')
    return 0;
  ptr--;
  while (holy_isdigit (*ptr) && ptr > dev)
    ptr--;
  if (*ptr != '(' || ptr == dev)
    return 0;
  ptr--;
  if (ptr - dev < (int) sizeof ("partition") - 2)
    return 0;
  ptr -= sizeof ("partition") - 2;
  if (holy_memcmp (ptr, "partition", sizeof ("partition") - 1) != 0)
    return 0;
  return ptr;
}

static holy_disk_addr_t
get_partition_offset (char *part, holy_disk_addr_t *en)
{
  holy_arc_fileno_t handle;
  holy_disk_addr_t ret = -1;
  struct holy_arc_fileinfo info;
  holy_arc_err_t r;

  if (holy_ARC_FIRMWARE_VECTOR->open (part, holy_ARC_FILE_ACCESS_OPEN_RO,
				      &handle))
    return -1;

  r = holy_ARC_FIRMWARE_VECTOR->getfileinformation (handle, &info);
  if (!r)
    {
      ret = (info.start >> 9);
      *en = (info.end >> 9);
    }
  holy_ARC_FIRMWARE_VECTOR->close (handle);
  return ret;
}

struct get_device_name_ctx
{
  char *partition_name;
  holy_disk_addr_t poff, pend;
};

static int
get_device_name_iter (holy_disk_t disk __attribute__ ((unused)),
		      const holy_partition_t part, void *data)
{
  struct get_device_name_ctx *ctx = data;

  if (holy_partition_get_start (part) == ctx->poff
      && holy_partition_get_len (part) == ctx->pend)
    {
      ctx->partition_name = holy_partition_get_name (part);
      return 1;
    }

  return 0;
}

void
holy_machine_get_bootlocation (char **device, char **path)
{
  char *loaddev = boot_location;
  char *pptr, *partptr;
  char *dname;
  holy_disk_addr_t poff = -1, pend = -1;
  struct get_device_name_ctx ctx;
  holy_disk_t parent = 0;
  unsigned i;

  for (i = 0; i < ARRAY_SIZE (type_names); i++)
    if (type_names[i]
	&& holy_memcmp (loaddev, type_names[i], holy_strlen (type_names[i])) == 0
	&& loaddev[holy_strlen (type_names[i])] == '(')
      break;
  if (i == ARRAY_SIZE (type_names))
    pptr = loaddev;
  else
    for (pptr = loaddev; *pptr && *pptr != '/' && *pptr != '\\'; pptr++);
  if (*pptr)
    {
      char *iptr, *optr;
      char sep = *pptr;
      *path = holy_malloc (holy_strlen (pptr) + 1);
      if (!*path)
	return;
      for (iptr = pptr, optr = *path; *iptr; iptr++, optr++)
	if (*iptr == sep)
	  *optr = '/';
	else
	  *optr = *iptr;
      *optr = '\0';
      *path = holy_strdup (pptr);
      *pptr = '\0';
    }

  if (*loaddev == '\0')
    {
      const char *syspart = 0;

      if (holy_ARC_SYSTEM_PARAMETER_BLOCK->firmware_vector_length
	  >= (unsigned) ((char *) (&holy_ARC_FIRMWARE_VECTOR->getenvironmentvariable + 1)
			 - (char *) holy_ARC_FIRMWARE_VECTOR)
	  && holy_ARC_FIRMWARE_VECTOR->getenvironmentvariable)
	syspart = holy_ARC_FIRMWARE_VECTOR->getenvironmentvariable ("SystemPartition");
      if (!syspart)
	return;
      loaddev = holy_strdup (syspart);
    }

  partptr = get_part (loaddev);
  if (partptr)
    {
      poff = get_partition_offset (loaddev, &pend);
      *partptr = '\0';
    }
  dname = norm_name_to_alt (loaddev);
  if (poff == (holy_addr_t) -1)
    {
      *device = dname;
      if (loaddev != boot_location)
	holy_free (loaddev);
      return;
    }

  parent = holy_disk_open (dname);
  if (!parent)
    {
      *device = dname;
      if (loaddev != boot_location)
	holy_free (loaddev);
      return;
    }

  if (poff == 0
      && pend == holy_disk_get_size (parent))
    {
      holy_disk_close (parent);
      *device = dname;
      if (loaddev != boot_location)
	holy_free (loaddev);
      return;
    }

  ctx.partition_name = NULL;
  ctx.poff = poff;
  ctx.pend = pend;

  holy_partition_iterate (parent, get_device_name_iter, &ctx);
  holy_disk_close (parent);

  if (! ctx.partition_name)
    {
      *device = dname;
      if (loaddev != boot_location)
	holy_free (loaddev);
      return;
    }

  *device = holy_xasprintf ("%s,%s", dname,
			    ctx.partition_name);
  holy_free (ctx.partition_name);
  holy_free (dname);
  if (loaddev != boot_location)
    holy_free (loaddev);
}
