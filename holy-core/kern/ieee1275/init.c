/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/partition.h>
#include <holy/normal.h>
#include <holy/fs.h>
#include <holy/setjmp.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/time.h>
#include <holy/ieee1275/console.h>
#include <holy/ieee1275/ofdisk.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/net.h>
#include <holy/offsets.h>
#include <holy/memory.h>
#include <holy/loader.h>
#ifdef __i386__
#include <holy/cpu/tsc.h>
#endif
#ifdef __sparc__
#include <holy/machine/kernel.h>
#endif

/* The minimal heap size we can live with. */
#define HEAP_MIN_SIZE		(unsigned long) (2 * 1024 * 1024)

/* The maximum heap size we're going to claim */
#ifdef __i386__
#define HEAP_MAX_SIZE		(unsigned long) (64 * 1024 * 1024)
#else
#define HEAP_MAX_SIZE		(unsigned long) (32 * 1024 * 1024)
#endif

/* If possible, we will avoid claiming heap above this address, because it
   seems to cause relocation problems with OSes that link at 4 MiB */
#ifdef __i386__
#define HEAP_MAX_ADDR		(unsigned long) (64 * 1024 * 1024)
#else
#define HEAP_MAX_ADDR		(unsigned long) (32 * 1024 * 1024)
#endif

extern char _start[];
extern char _end[];

#ifdef __sparc__
holy_addr_t holy_ieee1275_original_stack;
#endif

void
holy_exit (void)
{
  holy_ieee1275_exit ();
}

/* Translate an OF filesystem path (separated by backslashes), into a holy
   path (separated by forward slashes).  */
static void
holy_translate_ieee1275_path (char *filepath)
{
  char *backslash;

  backslash = holy_strchr (filepath, '\\');
  while (backslash != 0)
    {
      *backslash = '/';
      backslash = holy_strchr (filepath, '\\');
    }
}

void (*holy_ieee1275_net_config) (const char *dev, char **device, char **path,
                                  char *bootpath);
void
holy_machine_get_bootlocation (char **device, char **path)
{
  char *bootpath;
  holy_ssize_t bootpath_size;
  char *filename;
  char *type;

  if (holy_ieee1275_get_property_length (holy_ieee1275_chosen, "bootpath",
					 &bootpath_size)
      || bootpath_size <= 0)
    {
      /* Should never happen.  */
      holy_printf ("/chosen/bootpath property missing!\n");
      return;
    }

  bootpath = (char *) holy_malloc ((holy_size_t) bootpath_size + 64);
  if (! bootpath)
    {
      holy_print_error ();
      return;
    }
  holy_ieee1275_get_property (holy_ieee1275_chosen, "bootpath", bootpath,
                              (holy_size_t) bootpath_size + 1, 0);
  bootpath[bootpath_size] = '\0';

  /* Transform an OF device path to a holy path.  */

  type = holy_ieee1275_get_device_type (bootpath);
  if (type && holy_strcmp (type, "network") == 0)
    {
      char *dev, *canon;
      char *ptr;
      dev = holy_ieee1275_get_aliasdevname (bootpath);
      canon = holy_ieee1275_canonicalise_devname (dev);
      ptr = canon + holy_strlen (canon) - 1;
      while (ptr > canon && (*ptr == ',' || *ptr == ':'))
	ptr--;
      ptr++;
      *ptr = 0;

      if (holy_ieee1275_net_config)
	holy_ieee1275_net_config (canon, device, path, bootpath);
      holy_free (dev);
      holy_free (canon);
    }
  else
    *device = holy_ieee1275_encode_devname (bootpath);
  holy_free (type);

  filename = holy_ieee1275_get_filename (bootpath);
  if (filename)
    {
      char *lastslash = holy_strrchr (filename, '\\');

      /* Truncate at last directory.  */
      if (lastslash)
        {
	  *lastslash = '\0';
	  holy_translate_ieee1275_path (filename);

	  *path = filename;
	}
    }
  holy_free (bootpath);
}

/* Claim some available memory in the first /memory node. */
#ifdef __sparc__
static void 
holy_claim_heap (void)
{
  holy_mm_init_region ((void *) (holy_modules_get_end ()
				 + holy_KERNEL_MACHINE_STACK_SIZE), 0x200000);
}
#else
/* Helper for holy_claim_heap.  */
static int
heap_init (holy_uint64_t addr, holy_uint64_t len, holy_memory_type_t type,
	   void *data)
{
  unsigned long *total = data;

  if (type != holy_MEMORY_AVAILABLE)
    return 0;

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_NO_PRE1_5M_CLAIM))
    {
      if (addr + len <= 0x180000)
	return 0;

      if (addr < 0x180000)
	{
	  len = addr + len - 0x180000;
	  addr = 0x180000;
	}
    }
  len -= 1; /* Required for some firmware.  */

  /* Never exceed HEAP_MAX_SIZE  */
  if (*total + len > HEAP_MAX_SIZE)
    len = HEAP_MAX_SIZE - *total;

  /* Avoid claiming anything above HEAP_MAX_ADDR, if possible. */
  if ((addr < HEAP_MAX_ADDR) &&				/* if it's too late, don't bother */
      (addr + len > HEAP_MAX_ADDR) &&				/* if it wasn't available anyway, don't bother */
      (*total + (HEAP_MAX_ADDR - addr) > HEAP_MIN_SIZE))	/* only limit ourselves when we can afford to */
     len = HEAP_MAX_ADDR - addr;

  /* In theory, firmware should already prevent this from happening by not
     listing our own image in /memory/available.  The check below is intended
     as a safeguard in case that doesn't happen.  However, it doesn't protect
     us from corrupting our module area, which extends up to a
     yet-undetermined region above _end.  */
  if ((addr < (holy_addr_t) _end) && ((addr + len) > (holy_addr_t) _start))
    {
      holy_printf ("Warning: attempt to claim over our own code!\n");
      len = 0;
    }

  if (len)
    {
      holy_err_t err;
      /* Claim and use it.  */
      err = holy_claimmap (addr, len);
      if (err)
	return err;
      holy_mm_init_region ((void *) (holy_addr_t) addr, len);
    }

  *total += len;
  if (*total >= HEAP_MAX_SIZE)
    return 1;

  return 0;
}

static void 
holy_claim_heap (void)
{
  unsigned long total = 0;

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_FORCE_CLAIM))
    heap_init (holy_IEEE1275_STATIC_HEAP_START, holy_IEEE1275_STATIC_HEAP_LEN,
	       1, &total);
  else
    holy_machine_mmap_iterate (heap_init, &total);
}
#endif

static void
holy_parse_cmdline (void)
{
  holy_ssize_t actual;
  char args[256];

  if (holy_ieee1275_get_property (holy_ieee1275_chosen, "bootargs", &args,
				  sizeof args, &actual) == 0
      && actual > 1)
    {
      int i = 0;

      while (i < actual)
	{
	  char *command = &args[i];
	  char *end;
	  char *val;

	  end = holy_strchr (command, ';');
	  if (end == 0)
	    i = actual; /* No more commands after this one.  */
	  else
	    {
	      *end = '\0';
	      i += end - command + 1;
	      while (holy_isspace(args[i]))
		i++;
	    }

	  /* Process command.  */
	  val = holy_strchr (command, '=');
	  if (val)
	    {
	      *val = '\0';
	      holy_env_set (command, val + 1);
	    }
	}
    }
}

holy_addr_t holy_modbase;

void
holy_machine_init (void)
{
  holy_modbase = ALIGN_UP((holy_addr_t) _end
			  + holy_KERNEL_MACHINE_MOD_GAP,
			  holy_KERNEL_MACHINE_MOD_ALIGN);
  holy_ieee1275_init ();

  holy_console_init_early ();
  holy_claim_heap ();
  holy_console_init_lately ();
  holy_ofdisk_init ();

  holy_parse_cmdline ();

#ifdef __i386__
  holy_tsc_init ();
#else
  holy_install_get_time_ms (holy_rtc_get_time_ms);
#endif
}

void
holy_machine_fini (int flags)
{
  if (flags & holy_LOADER_FLAG_NORETURN)
    {
      holy_ofdisk_fini ();
      holy_console_fini ();
    }
}

holy_uint64_t
holy_rtc_get_time_ms (void)
{
  holy_uint32_t msecs = 0;

  holy_ieee1275_milliseconds (&msecs);

  return msecs;
}
