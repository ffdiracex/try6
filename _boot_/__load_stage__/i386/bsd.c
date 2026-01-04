/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/i386/bsd.h>
#include <holy/i386/cpuid.h>
#include <holy/memory.h>
#include <holy/i386/memory.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/elfload.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/aout.h>
#include <holy/command.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/ns8250.h>
#include <holy/bsdlabel.h>
#include <holy/crypto.h>
#ifdef holy_MACHINE_PCBIOS
#include <holy/machine/int.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

#include <holy/video.h>
#ifdef holy_MACHINE_PCBIOS
#include <holy/machine/biosnum.h>
#endif
#ifdef holy_MACHINE_EFI
#include <holy/efi/efi.h>
#define NETBSD_DEFAULT_VIDEO_MODE "800x600"
#else
#define NETBSD_DEFAULT_VIDEO_MODE "text"
#include <holy/i386/pc/vbe.h>
#endif
#include <holy/video.h>

#include <holy/disk.h>
#include <holy/device.h>
#include <holy/partition.h>
#include <holy/relocator.h>
#include <holy/i386/relocator.h>

#define ALIGN_DWORD(a)	ALIGN_UP (a, 4)
#define ALIGN_QWORD(a)	ALIGN_UP (a, 8)
#define ALIGN_VAR(a)	((is_64bit) ? (ALIGN_QWORD(a)) : (ALIGN_DWORD(a)))
#define ALIGN_PAGE(a)	ALIGN_UP (a, 4096)

static int kernel_type = KERNEL_TYPE_NONE;
static holy_dl_t my_mod;
static holy_addr_t entry, entry_hi, kern_start, kern_end;
static void *kern_chunk_src;
static holy_uint32_t bootflags;
static int is_elf_kernel, is_64bit;
static holy_uint32_t openbsd_root;
static struct holy_relocator *relocator = NULL;
static struct holy_openbsd_ramdisk_descriptor openbsd_ramdisk;

struct bsd_tag
{
  struct bsd_tag *next;
  holy_size_t len;
  holy_uint32_t type;
  union {
    holy_uint8_t a;
    holy_uint16_t b;
    holy_uint32_t c;
  } data[0];
};

static struct bsd_tag *tags, *tags_last;

struct netbsd_module
{
  struct netbsd_module *next;
  struct holy_netbsd_btinfo_module mod;
};

static struct netbsd_module *netbsd_mods, *netbsd_mods_last;

static const struct holy_arg_option freebsd_opts[] =
  {
    {"dual", 'D', 0, N_("Display output on all consoles."), 0, 0},
    {"serial", 'h', 0, N_("Use serial console."), 0, 0},
    {"askname", 'a', 0, N_("Ask for file name to reboot from."), 0, 0},
    {"cdrom", 'C', 0, N_("Use CD-ROM as root."), 0, 0},
    {"config", 'c', 0, N_("Invoke user configuration routing."), 0, 0},
    {"kdb", 'd', 0, N_("Enter in KDB on boot."), 0, 0},
    {"gdb", 'g', 0, N_("Use GDB remote debugger instead of DDB."), 0, 0},
    {"mute", 'm', 0, N_("Disable all boot output."), 0, 0},
    {"nointr", 'n', 0, "", 0, 0},
    {"pause", 'p', 0, N_("Wait for keypress after every line of output."), 0, 0},
    {"quiet", 'q', 0, "", 0, 0},
    {"dfltroot", 'r', 0, N_("Use compiled-in root device."), 0, 0},
    {"single", 's', 0, N_("Boot into single mode."), 0, 0},
    {"verbose", 'v', 0, N_("Boot with verbose messages."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static const holy_uint32_t freebsd_flags[] =
{
  FREEBSD_RB_DUAL, FREEBSD_RB_SERIAL, FREEBSD_RB_ASKNAME,
  FREEBSD_RB_CDROM, FREEBSD_RB_CONFIG, FREEBSD_RB_KDB,
  FREEBSD_RB_GDB, FREEBSD_RB_MUTE, FREEBSD_RB_NOINTR,
  FREEBSD_RB_PAUSE, FREEBSD_RB_QUIET, FREEBSD_RB_DFLTROOT,
  FREEBSD_RB_SINGLE, FREEBSD_RB_VERBOSE, 0
};

static const struct holy_arg_option openbsd_opts[] =
  {
    {"askname", 'a', 0, N_("Ask for file name to reboot from."), 0, 0},
    {"halt", 'b', 0, N_("Don't reboot, just halt."), 0, 0},
    {"config", 'c', 0, N_("Change configured devices."), 0, 0},
    {"single", 's', 0, N_("Boot into single mode."), 0, 0},
    {"kdb", 'd', 0, N_("Enter in KDB on boot."), 0, 0},
    {"root", 'r', 0, N_("Set root device."), "wdXY", ARG_TYPE_STRING},
    {"serial", 'h', holy_ARG_OPTION_OPTIONAL,
     N_("Use serial console."),
     /* TRANSLATORS: "com" is static and not to be translated. It refers to
	serial ports e.g. com1.
      */
     N_("comUNIT[,SPEED]"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static const holy_uint32_t openbsd_flags[] =
{
  OPENBSD_RB_ASKNAME, OPENBSD_RB_HALT, OPENBSD_RB_CONFIG,
  OPENBSD_RB_SINGLE, OPENBSD_RB_KDB, 0
};

#define OPENBSD_ROOT_ARG (ARRAY_SIZE (openbsd_flags) - 1)
#define OPENBSD_SERIAL_ARG (ARRAY_SIZE (openbsd_flags))

static const struct holy_arg_option netbsd_opts[] =
  {
    {"no-smp", '1', 0, N_("Disable SMP."), 0, 0},
    {"no-acpi", '2', 0, N_("Disable ACPI."), 0, 0},
    {"askname", 'a', 0, N_("Ask for file name to reboot from."), 0, 0},
    {"halt", 'b', 0, N_("Don't reboot, just halt."), 0, 0},
    {"config", 'c', 0, N_("Change configured devices."), 0, 0},
    {"kdb", 'd', 0, N_("Enter in KDB on boot."), 0, 0},
    {"miniroot", 'm', 0, "", 0, 0},
    {"quiet", 'q', 0, N_("Don't display boot diagnostic messages."), 0, 0},
    {"single", 's', 0, N_("Boot into single mode."), 0, 0},
    {"verbose", 'v', 0, N_("Boot with verbose messages."), 0, 0},
    {"debug", 'x', 0, N_("Boot with debug messages."), 0, 0},
    {"silent", 'z', 0, N_("Suppress normal output (warnings remain)."), 0, 0},
    {"root", 'r', 0, N_("Set root device."), N_("DEVICE"), ARG_TYPE_STRING},
    {"serial", 'h', holy_ARG_OPTION_OPTIONAL,
     N_("Use serial console."),
     /* TRANSLATORS: "com" is static and not to be translated. It refers to
	serial ports e.g. com1.
      */
     N_("[ADDR|comUNIT][,SPEED]"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static const holy_uint32_t netbsd_flags[] =
{
  NETBSD_AB_NOSMP, NETBSD_AB_NOACPI, NETBSD_RB_ASKNAME,
  NETBSD_RB_HALT, NETBSD_RB_USERCONFIG, NETBSD_RB_KDB,
  NETBSD_RB_MINIROOT, NETBSD_AB_QUIET, NETBSD_RB_SINGLE,
  NETBSD_AB_VERBOSE, NETBSD_AB_DEBUG, NETBSD_AB_SILENT, 0
};

#define NETBSD_ROOT_ARG (ARRAY_SIZE (netbsd_flags) - 1)
#define NETBSD_SERIAL_ARG (ARRAY_SIZE (netbsd_flags))

static void
holy_bsd_get_device (holy_uint32_t * biosdev,
		     holy_uint32_t * unit,
		     holy_uint32_t * slice, holy_uint32_t * part)
{
  holy_device_t dev;

#ifdef holy_MACHINE_PCBIOS
  *biosdev = holy_get_root_biosnumber () & 0xff;
#else
  *biosdev = 0xff;
#endif
  *unit = (*biosdev & 0x7f);
  *slice = 0xff;
  *part = 0xff;
  dev = holy_device_open (0);
  if (dev && dev->disk && dev->disk->partition)
    {
      if (dev->disk->partition->parent)
	{
	  *part = dev->disk->partition->number;
	  *slice = dev->disk->partition->parent->number + 1;
	}
      else
	*slice = dev->disk->partition->number + 1;
    }
  if (dev)
    holy_device_close (dev);
}

static holy_err_t
holy_bsd_add_meta_ptr (holy_uint32_t type, void **ptr, holy_uint32_t len)
{
  struct bsd_tag *newtag;

  newtag = holy_malloc (len + sizeof (struct bsd_tag));
  if (!newtag)
    return holy_errno;
  newtag->len = len;
  newtag->type = type;
  newtag->next = NULL;
  *ptr = newtag->data;

  if (kernel_type == KERNEL_TYPE_FREEBSD 
      && type == (FREEBSD_MODINFO_METADATA | FREEBSD_MODINFOMD_SMAP))
    {
      struct bsd_tag *p;
      for (p = tags;
	   p && p->type != (FREEBSD_MODINFO_METADATA
			    | FREEBSD_MODINFOMD_KERNEND);
	   p = p->next);

      if (p)
	{
	  newtag->next = p->next;
	  p->next = newtag;
	  if (newtag->next == NULL)
	    tags_last = newtag;
	  return holy_ERR_NONE;
	}
    }

  if (tags_last)
    tags_last->next = newtag;
  else
    tags = newtag;
  tags_last = newtag;

  return holy_ERR_NONE;
}

holy_err_t
holy_bsd_add_meta (holy_uint32_t type, const void *data, holy_uint32_t len)
{
  holy_err_t err;
  void *ptr;

  err = holy_bsd_add_meta_ptr (type, &ptr, len);
  if (err)
    return err;
  if (len)
    holy_memcpy (ptr, data, len);
  return holy_ERR_NONE;
}


struct holy_e820_mmap
{
  holy_uint64_t addr;
  holy_uint64_t size;
  holy_uint32_t type;
} holy_PACKED;
#define holy_E820_RAM        1
#define holy_E820_RESERVED   2
#define holy_E820_ACPI       3
#define holy_E820_NVS        4
#define holy_E820_BADRAM     5
#define holy_E820_COREBOOT_TABLES 0x10

/* Context for generate_e820_mmap.  */
struct generate_e820_mmap_ctx
{
  int count;
  struct holy_e820_mmap *mmap;
  struct holy_e820_mmap prev, cur;
};

/* Helper for generate_e820_mmap.  */
static int
generate_e820_mmap_iter (holy_uint64_t addr, holy_uint64_t size,
			 holy_memory_type_t type, void *data)
{
  struct generate_e820_mmap_ctx *ctx = data;

  ctx->cur.addr = addr;
  ctx->cur.size = size;

  if (type == holy_MEMORY_COREBOOT_TABLES
      && addr == 0)
      /* Nowadays the tables at 0 don't contain anything important but
       *BSD needs the memory at 0 for own needs.
       */
    type = holy_E820_RAM;

  ctx->cur.type = type;

  /* Merge regions if possible. */
  if (ctx->count && ctx->cur.type == ctx->prev.type
      && ctx->cur.addr == ctx->prev.addr + ctx->prev.size)
    {
      ctx->prev.size += ctx->cur.size;
      if (ctx->mmap)
	ctx->mmap[-1] = ctx->prev;
    }
  else
    {
      if (ctx->mmap)
	*ctx->mmap++ = ctx->cur;
      ctx->prev = ctx->cur;
      ctx->count++;
    }

  if (kernel_type == KERNEL_TYPE_OPENBSD && ctx->prev.addr < 0x100000
      && ctx->prev.addr + ctx->prev.size > 0x100000)
    {
      ctx->cur.addr = 0x100000;
      ctx->cur.size = ctx->prev.addr + ctx->prev.size - 0x100000;
      ctx->cur.type = ctx->prev.type;
      ctx->prev.size = 0x100000 - ctx->prev.addr;
      if (ctx->mmap)
	{
	  ctx->mmap[-1] = ctx->prev;
	  ctx->mmap[0] = ctx->cur;
	  ctx->mmap++;
	}
      ctx->prev = ctx->cur;
      ctx->count++;
    }

  return 0;
}

static void
generate_e820_mmap (holy_size_t *len, holy_size_t *cnt, void *buf)
{
  struct generate_e820_mmap_ctx ctx = {
    .count = 0,
    .mmap = buf
  };

  holy_mmap_iterate (generate_e820_mmap_iter, &ctx);

  if (len)
    *len = ctx.count * sizeof (struct holy_e820_mmap);
  *cnt = ctx.count;

  return;
}

static holy_err_t
holy_bsd_add_mmap (void)
{
  holy_size_t len, cnt;
  void *buf = NULL, *buf0;

  generate_e820_mmap (&len, &cnt, buf);

  if (kernel_type == KERNEL_TYPE_NETBSD)
    len += sizeof (holy_uint32_t);

  if (kernel_type == KERNEL_TYPE_OPENBSD)
    len += sizeof (struct holy_e820_mmap);

  buf = holy_malloc (len);
  if (!buf)
    return holy_errno;

  buf0 = buf;
  if (kernel_type == KERNEL_TYPE_NETBSD)
    {
      *(holy_uint32_t *) buf = cnt;
      buf = ((holy_uint32_t *) buf + 1);
    }

  generate_e820_mmap (NULL, &cnt, buf);

  if (kernel_type == KERNEL_TYPE_OPENBSD)
    holy_memset ((holy_uint8_t *) buf + len - sizeof (struct holy_e820_mmap), 0,
		 sizeof (struct holy_e820_mmap));

  holy_dprintf ("bsd", "%u entries in smap\n", (unsigned) cnt);
  if (kernel_type == KERNEL_TYPE_NETBSD)
    holy_bsd_add_meta (NETBSD_BTINFO_MEMMAP, buf0, len);
  else if (kernel_type == KERNEL_TYPE_OPENBSD)
    holy_bsd_add_meta (OPENBSD_BOOTARG_MMAP, buf0, len);
  else
    holy_bsd_add_meta (FREEBSD_MODINFO_METADATA |
		       FREEBSD_MODINFOMD_SMAP, buf0, len);

  holy_free (buf0);

  return holy_errno;
}

holy_err_t
holy_freebsd_add_meta_module (const char *filename, const char *type,
			      int argc, char **argv,
			      holy_addr_t addr, holy_uint32_t size)
{
  const char *name;
  name = holy_strrchr (filename, '/');
  if (name)
    name++;
  else
    name = filename;
  if (holy_strcmp (type, "/boot/zfs/zpool.cache") == 0)
    name = "/boot/zfs/zpool.cache";

  if (holy_bsd_add_meta (FREEBSD_MODINFO_NAME, name, holy_strlen (name) + 1))
    return holy_errno;

  if (is_64bit)
    {
      holy_uint64_t addr64 = addr, size64 = size;
      if (holy_bsd_add_meta (FREEBSD_MODINFO_TYPE, type, holy_strlen (type) + 1)
	  || holy_bsd_add_meta (FREEBSD_MODINFO_ADDR, &addr64, sizeof (addr64))
	  || holy_bsd_add_meta (FREEBSD_MODINFO_SIZE, &size64, sizeof (size64)))
	return holy_errno;
    }
  else
    {
      if (holy_bsd_add_meta (FREEBSD_MODINFO_TYPE, type, holy_strlen (type) + 1)
	  || holy_bsd_add_meta (FREEBSD_MODINFO_ADDR, &addr, sizeof (addr))
	  || holy_bsd_add_meta (FREEBSD_MODINFO_SIZE, &size, sizeof (size)))
	return holy_errno;
    }

  if (argc)
    {
      int i, n;

      n = 0;
      for (i = 0; i < argc; i++)
	{
	  n += holy_strlen (argv[i]) + 1;
	}

      if (n)
	{
	  void *cmdline;
	  char *p;

	  if (holy_bsd_add_meta_ptr (FREEBSD_MODINFO_ARGS, &cmdline, n))
	    return holy_errno;

	  p = cmdline;
	  for (i = 0; i < argc; i++)
	    {
	      holy_strcpy (p, argv[i]);
	      p += holy_strlen (argv[i]);
	      *(p++) = ' ';
	    }
	  *p = 0;
	}
    }

  return holy_ERR_NONE;
}

static void
holy_freebsd_list_modules (void)
{
  struct bsd_tag *tag;

  holy_printf ("  %-18s  %-18s%14s%14s\n", _("name"), _("type"), _("addr"),
	       _("size"));

  for (tag = tags; tag; tag = tag->next)
    {
      switch (tag->type)
	{
	case FREEBSD_MODINFO_NAME:
	case FREEBSD_MODINFO_TYPE:
	  holy_printf ("  %-18s", (char *) tag->data);
	  break;
	case FREEBSD_MODINFO_ADDR:
	  {
	    holy_uint32_t addr;

	    addr = *((holy_uint32_t *) tag->data);
	    holy_printf ("    0x%08x", addr);
	    break;
	  }
	case FREEBSD_MODINFO_SIZE:
	  {
	    holy_uint32_t len;

	    len = *((holy_uint32_t *) tag->data);
	    holy_printf ("    0x%08x\n", len);
	  }
	}
    }
}

static holy_err_t
holy_netbsd_add_meta_module (char *filename, holy_uint32_t type,
			     holy_addr_t addr, holy_uint32_t size)
{
  char *name;
  struct netbsd_module *mod;
  name = holy_strrchr (filename, '/');

  if (name)
    name++;
  else
    name = filename;

  mod = holy_zalloc (sizeof (*mod));
  if (!mod)
    return holy_errno;

  holy_strncpy (mod->mod.name, name, sizeof (mod->mod.name) - 1);
  mod->mod.addr = addr;
  mod->mod.type = type;
  mod->mod.size = size;

  if (netbsd_mods_last)
    netbsd_mods_last->next = mod;
  else
    netbsd_mods = mod;
  netbsd_mods_last = mod;

  return holy_ERR_NONE;
}

static void
holy_netbsd_list_modules (void)
{
  struct netbsd_module *mod;

  holy_printf ("  %-18s%14s%14s%14s\n", _("name"), _("type"), _("addr"),
	       _("size"));

  for (mod = netbsd_mods; mod; mod = mod->next)
    holy_printf ("  %-18s  0x%08x  0x%08x  0x%08x", mod->mod.name,
		 mod->mod.type, mod->mod.addr, mod->mod.size);
}

/* This function would be here but it's under different license. */
#include "bsd_pagetable.c"

static holy_uint32_t freebsd_bootdev, freebsd_biosdev;
static holy_uint64_t freebsd_zfsguid;

static void
freebsd_get_zfs (void)
{
  holy_device_t dev;
  holy_fs_t fs;
  char *uuid;
  holy_err_t err;

  dev = holy_device_open (0);
  if (!dev)
    return;
  fs = holy_fs_probe (dev);
  if (!fs)
    return;
  if (!fs->uuid || holy_strcmp (fs->name, "zfs") != 0)
    return;
  err = fs->uuid (dev, &uuid);
  if (err)
    return;
  if (!uuid)
    return;
  freebsd_zfsguid = holy_strtoull (uuid, 0, 16);
  holy_free (uuid);
}

static holy_err_t
holy_freebsd_boot (void)
{
  struct holy_freebsd_bootinfo bi;
  holy_uint8_t *p, *p0;
  holy_addr_t p_target;
  holy_size_t p_size = 0;
  holy_err_t err;
  holy_size_t tag_buf_len = 0;

  struct holy_env_var *var;

  holy_memset (&bi, 0, sizeof (bi));
  bi.version = FREEBSD_BOOTINFO_VERSION;
  bi.length = sizeof (bi);

  bi.boot_device = freebsd_biosdev;

  p_size = 0;
  FOR_SORTED_ENV (var)
    if ((holy_memcmp (var->name, "kFreeBSD.", sizeof("kFreeBSD.") - 1) == 0) && (var->name[sizeof("kFreeBSD.") - 1]))
      {
	p_size += holy_strlen (&var->name[sizeof("kFreeBSD.") - 1]);
	p_size++;
	p_size += holy_strlen (var->value) + 1;
      }

  if (p_size)
    p_size = ALIGN_PAGE (kern_end + p_size + 1) - kern_end;

  if (is_elf_kernel)
    {
      struct bsd_tag *tag;

      err = holy_bsd_add_mmap ();
      if (err)
	return err;

      err = holy_bsd_add_meta (FREEBSD_MODINFO_END, 0, 0);
      if (err)
	return err;
      
      tag_buf_len = 0;
      for (tag = tags; tag; tag = tag->next)
	tag_buf_len = ALIGN_VAR (tag_buf_len
				 + sizeof (struct freebsd_tag_header)
				 + tag->len);
      p_size = ALIGN_PAGE (kern_end + p_size + tag_buf_len) - kern_end;
    }

  if (is_64bit)
    p_size += 4096 * 3;

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   kern_end, p_size);
    if (err)
      return err;
    p = get_virtual_current_address (ch);
  }
  p_target = kern_end;
  p0 = p;
  kern_end += p_size;

  FOR_SORTED_ENV (var)
    if ((holy_memcmp (var->name, "kFreeBSD.", sizeof("kFreeBSD.") - 1) == 0) && (var->name[sizeof("kFreeBSD.") - 1]))
      {
	holy_strcpy ((char *) p, &var->name[sizeof("kFreeBSD.") - 1]);
	p += holy_strlen ((char *) p);
	*(p++) = '=';
	holy_strcpy ((char *) p, var->value);
	p += holy_strlen ((char *) p) + 1;
      }

  if (p != p0)
    {
      *(p++) = 0;

      bi.environment = p_target;
    }

  if (is_elf_kernel)
    {
      holy_uint8_t *p_tag = p;
      struct bsd_tag *tag;
      
      for (tag = tags; tag; tag = tag->next)
	{
	  struct freebsd_tag_header *head
	    = (struct freebsd_tag_header *) p_tag;
	  head->type = tag->type;
	  head->len = tag->len;
	  p_tag += sizeof (struct freebsd_tag_header);
	  switch (tag->type)
	    {
	    case FREEBSD_MODINFO_METADATA | FREEBSD_MODINFOMD_HOWTO:
	      if (is_64bit)
		*(holy_uint64_t *) p_tag = bootflags;
	      else
		*(holy_uint32_t *) p_tag = bootflags;
	      break;

	    case FREEBSD_MODINFO_METADATA | FREEBSD_MODINFOMD_ENVP:
	      if (is_64bit)
		*(holy_uint64_t *) p_tag = bi.environment;
	      else
		*(holy_uint32_t *) p_tag = bi.environment;
	      break;

	    case FREEBSD_MODINFO_METADATA | FREEBSD_MODINFOMD_KERNEND:
	      if (is_64bit)
		*(holy_uint64_t *) p_tag = kern_end;
	      else
		*(holy_uint32_t *) p_tag = kern_end;
	      break;

	    default:
	      holy_memcpy (p_tag, tag->data, tag->len);
	      break;
	    }
	  p_tag += tag->len;
	  p_tag = ALIGN_VAR (p_tag - p) + p;
	}

      bi.tags = (p - p0) + p_target;

      p = (ALIGN_PAGE ((p_tag - p0) + p_target) - p_target) + p0;
    }

  bi.kern_end = kern_end;

  holy_video_set_mode ("text", 0, 0);

  if (is_64bit)
    {
      struct holy_relocator64_state state;
      holy_uint8_t *pagetable;
      holy_uint32_t *stack;
      holy_addr_t stack_target;

      {
	holy_relocator_chunk_t ch;
	err = holy_relocator_alloc_chunk_align (relocator, &ch,
						0x10000, 0x90000,
						3 * sizeof (holy_uint32_t)
						+ sizeof (bi), 4,
						holy_RELOCATOR_PREFERENCE_NONE,
						0);
	if (err)
	  return err;
	stack = get_virtual_current_address (ch);
	stack_target = get_physical_target_address (ch);
      }

#ifdef holy_MACHINE_EFI
      err = holy_efi_finish_boot_services (NULL, NULL, NULL, NULL, NULL);
      if (err)
	return err;
#endif

      pagetable = p;
      fill_bsd64_pagetable (pagetable, (pagetable - p0) + p_target);

      state.cr3 = (pagetable - p0) + p_target;
      state.rsp = stack_target;
      state.rip = (((holy_uint64_t) entry_hi) << 32) | entry;

      stack[0] = entry;
      stack[1] = bi.tags;
      stack[2] = kern_end;
      return holy_relocator64_boot (relocator, state, 0, 0x40000000);
    }
  else
    {
      struct holy_relocator32_state state;
      holy_uint32_t *stack;
      holy_addr_t stack_target;

      {
	holy_relocator_chunk_t ch;
	err = holy_relocator_alloc_chunk_align (relocator, &ch,
						0x10000, 0x90000,
						9 * sizeof (holy_uint32_t)
						+ sizeof (bi), 4,
						holy_RELOCATOR_PREFERENCE_NONE,
						0);
	if (err)
	  return err;
	stack = get_virtual_current_address (ch);
	stack_target = get_physical_target_address (ch);
      }

#ifdef holy_MACHINE_EFI
      err = holy_efi_finish_boot_services (NULL, NULL, NULL, NULL, NULL);
      if (err)
	return err;
#endif

      holy_memcpy (&stack[9], &bi, sizeof (bi));
      state.eip = entry;
      state.esp = stack_target;
      state.ebp = stack_target;
      stack[0] = entry; /* "Return" address.  */
      stack[1] = bootflags | FREEBSD_RB_BOOTINFO;
      stack[2] = freebsd_bootdev;
      stack[3] = freebsd_zfsguid ? 4 : 0;
      stack[4] = freebsd_zfsguid;
      stack[5] = freebsd_zfsguid >> 32;
      stack[6] = stack_target + 9 * sizeof (holy_uint32_t);
      stack[7] = bi.tags;
      stack[8] = kern_end;
      return holy_relocator32_boot (relocator, state, 0);
    }

  /* Not reached.  */
  return holy_ERR_NONE;
}

static holy_err_t
holy_openbsd_boot (void)
{
  holy_uint32_t *stack;
  struct holy_relocator32_state state;
  void *curarg, *buf0, *arg0;
  holy_addr_t buf_target;
  holy_err_t err;
  holy_size_t tag_buf_len;

  err = holy_bsd_add_mmap ();
  if (err)
    return err;

#ifdef holy_MACHINE_PCBIOS
  {
    struct holy_bios_int_registers regs;

    regs.flags = holy_CPU_INT_FLAGS_DEFAULT;

    regs.ebx = 0;
    regs.ecx = 0;
    regs.eax = 0xb101;
    regs.es = 0;
    regs.edi = 0;
    regs.edx = 0;

    holy_bios_interrupt (0x1a, &regs);
    if (regs.edx == 0x20494350)
      {
	struct holy_openbsd_bootarg_pcibios pcibios;
	
	pcibios.characteristics = regs.eax & 0xff;
	pcibios.revision = regs.ebx & 0xffff;
	pcibios.pm_entry = regs.edi;
	pcibios.last_bus = regs.ecx & 0xff;

	holy_bsd_add_meta (OPENBSD_BOOTARG_PCIBIOS, &pcibios,
			   sizeof (pcibios));
      }
  }
#endif

  {
    struct bsd_tag *tag;
    tag_buf_len = 0;
    for (tag = tags; tag; tag = tag->next)
      tag_buf_len = ALIGN_VAR (tag_buf_len
			       + sizeof (struct holy_openbsd_bootargs)
			       + tag->len);
  }

  buf_target = holy_BSD_TEMP_BUFFER - 9 * sizeof (holy_uint32_t);
  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch, buf_target,
					   tag_buf_len
					   + sizeof (struct holy_openbsd_bootargs)
					   + 9 * sizeof (holy_uint32_t));
    if (err)
      return err;
    buf0 = get_virtual_current_address (ch);
  }

  stack = (holy_uint32_t *) buf0;
  arg0 = curarg = stack + 9;

  {
    struct bsd_tag *tag;
    struct holy_openbsd_bootargs *head;

    for (tag = tags; tag; tag = tag->next)
      {
	head = curarg;
	head->ba_type = tag->type;
	head->ba_size = tag->len + sizeof (*head);
	curarg = head + 1;
	holy_memcpy (curarg, tag->data, tag->len);
	curarg = (holy_uint8_t *) curarg + tag->len;
	head->ba_next = (holy_uint8_t *) curarg - (holy_uint8_t *) buf0
	  + buf_target;
      }
    head = curarg;
    head->ba_type = OPENBSD_BOOTARG_END;
    head->ba_size = 0;
    head->ba_next = 0;
  }

  holy_video_set_mode ("text", 0, 0);

#ifdef holy_MACHINE_EFI
  err = holy_efi_finish_boot_services (NULL, NULL, NULL, NULL, NULL);
  if (err)
    return err;
#endif

  state.eip = entry;
  state.ebp = state.esp
    = ((holy_uint8_t *) stack - (holy_uint8_t *) buf0) + buf_target;
  stack[0] = entry;
  stack[1] = bootflags;
  stack[2] = openbsd_root;
  stack[3] = OPENBSD_BOOTARG_APIVER;
  stack[4] = 0;
  stack[5] = holy_mmap_get_upper () >> 10;
  stack[6] = holy_mmap_get_lower () >> 10;
  stack[7] = (holy_uint8_t *) curarg - (holy_uint8_t *) arg0;
  stack[8] = ((holy_uint8_t *) arg0 - (holy_uint8_t *) buf0) + buf_target;

  return holy_relocator32_boot (relocator, state, 0);
}

static holy_err_t
holy_netbsd_setup_video (void)
{
  struct holy_video_mode_info mode_info;
  void *framebuffer;
  const char *modevar;
  struct holy_netbsd_btinfo_framebuf params;
  holy_err_t err;
  holy_video_driver_id_t driv_id;

  modevar = holy_env_get ("gfxpayload");

  /* Now all graphical modes are acceptable.
     May change in future if we have modes without framebuffer.  */
  if (modevar && *modevar != 0)
    {
      char *tmp;
      tmp = holy_xasprintf ("%s;" NETBSD_DEFAULT_VIDEO_MODE, modevar);
      if (! tmp)
	return holy_errno;
      err = holy_video_set_mode (tmp, 0, 0);
      holy_free (tmp);
    }
  else
    err = holy_video_set_mode (NETBSD_DEFAULT_VIDEO_MODE, 0, 0);

  if (err)
    return err;

  driv_id = holy_video_get_driver_id ();
  if (driv_id == holy_VIDEO_DRIVER_NONE)
    return holy_ERR_NONE;

  err = holy_video_get_info_and_fini (&mode_info, &framebuffer);

  if (err)
    return err;

  params.width = mode_info.width;
  params.height = mode_info.height;
  params.bpp = mode_info.bpp;
  params.pitch = mode_info.pitch;
  params.flags = 0;

  params.fbaddr = (holy_addr_t) framebuffer;

  params.red_mask_size = mode_info.red_mask_size;
  params.red_field_pos = mode_info.red_field_pos;
  params.green_mask_size = mode_info.green_mask_size;
  params.green_field_pos = mode_info.green_field_pos;
  params.blue_mask_size = mode_info.blue_mask_size;
  params.blue_field_pos = mode_info.blue_field_pos;

#ifdef holy_MACHINE_PCBIOS
  /* VESA packed modes may come with zeroed mask sizes, which need
     to be set here according to DAC Palette width.  If we don't,
     this results in Linux displaying a black screen.  */
  if (mode_info.bpp <= 8 && driv_id == holy_VIDEO_DRIVER_VBE)
    {
      struct holy_vbe_info_block controller_info;
      int status;
      int width = 8;

      status = holy_vbe_bios_get_controller_info (&controller_info);

      if (status == holy_VBE_STATUS_OK &&
	  (controller_info.capabilities & holy_VBE_CAPABILITY_DACWIDTH))
	status = holy_vbe_bios_set_dac_palette_width (&width);

      if (status != holy_VBE_STATUS_OK)
	/* 6 is default after mode reset.  */
	width = 6;

      params.red_mask_size = params.green_mask_size
	= params.blue_mask_size = width;
    }
#endif

  err = holy_bsd_add_meta (NETBSD_BTINFO_FRAMEBUF, &params, sizeof (params));
  return err;
}

static holy_err_t
holy_netbsd_add_modules (void)
{
  struct netbsd_module *mod;
  unsigned modcnt = 0;
  struct holy_netbsd_btinfo_modules *mods;
  unsigned i;
  holy_err_t err;

  for (mod = netbsd_mods; mod; mod = mod->next)
    modcnt++;

  mods = holy_malloc (sizeof (*mods) + sizeof (mods->mods[0]) * modcnt);
  if (!mods)
    return holy_errno;

  mods->num = modcnt;
  mods->last_addr = kern_end;
  for (mod = netbsd_mods, i = 0; mod; i++, mod = mod->next)
    mods->mods[i] = mod->mod;

  err = holy_bsd_add_meta (NETBSD_BTINFO_MODULES, mods,
			   sizeof (*mods) + sizeof (mods->mods[0]) * modcnt);
  holy_free (mods);
  return err;
}

/*
 * Adds NetBSD bootinfo bootdisk and bootwedge.  The partition identified
 * in these bootinfo fields is the root device.
 */
static void
holy_netbsd_add_boot_disk_and_wedge (void)
{
  holy_device_t dev;
  holy_disk_t disk;
  holy_partition_t part;
  holy_uint32_t biosdev;
  holy_uint32_t partmapsector;
  union {
    holy_uint64_t raw[holy_DISK_SECTOR_SIZE / 8];
    struct holy_partition_bsd_disk_label label;
  } buf;

  if (holy_MD_MD5->mdlen > holy_CRYPTO_MAX_MDLEN)
    {
      holy_error (holy_ERR_BUG, "mdlen too long");
      return;
    }

  dev = holy_device_open (0);
  if (! (dev && dev->disk && dev->disk->partition))
    goto fail;

  disk = dev->disk;
  part = disk->partition;

  if (disk->dev && disk->dev->id == holy_DISK_DEVICE_BIOSDISK_ID)
    biosdev = (holy_uint32_t) disk->id & 0xff;
  else
    biosdev = 0xff;

  /* Absolute sector of the partition map describing this partition.  */
  partmapsector = holy_partition_get_start (part->parent) + part->offset;

  disk->partition = part->parent;
  if (holy_disk_read (disk, part->offset, 0, holy_DISK_SECTOR_SIZE, buf.raw)
      != holy_ERR_NONE)
    goto fail;
  disk->partition = part;

  /* Fill bootwedge.  */
  {
    struct holy_netbsd_btinfo_bootwedge biw;
    holy_uint8_t hash[holy_CRYPTO_MAX_MDLEN];

    holy_memset (&biw, 0, sizeof (biw));
    biw.biosdev = biosdev;
    biw.startblk = holy_partition_get_start (part);
    biw.nblks = part->len;
    biw.matchblk = partmapsector;
    biw.matchnblks = 1;

    holy_crypto_hash (holy_MD_MD5, hash,
		      buf.raw, holy_DISK_SECTOR_SIZE);
    holy_memcpy (biw.matchhash, hash, 16);

    holy_bsd_add_meta (NETBSD_BTINFO_BOOTWEDGE, &biw, sizeof (biw));
  }

  /* Fill bootdisk.  */
  {
    struct holy_netbsd_btinfo_bootdisk bid;

    holy_memset (&bid, 0, sizeof (bid));
    /* Check for a NetBSD disk label.  */
    if (part->partmap != NULL &&
	(holy_strcmp (part->partmap->name, "netbsd") == 0 ||
	 (part->parent == NULL && holy_strcmp (part->partmap->name, "bsd") == 0)))
      {
	bid.labelsector = partmapsector;
	bid.label.type = buf.label.type;
	bid.label.checksum = buf.label.checksum;
	holy_memcpy (bid.label.packname, buf.label.packname, 16);
      }
    else
      {
	bid.labelsector = -1;
      }
    bid.biosdev = biosdev;
    bid.partition = part->number;

    holy_bsd_add_meta (NETBSD_BTINFO_BOOTDISK, &bid, sizeof (bid));
  }

fail:
  if (dev)
    holy_device_close (dev);
}

static holy_err_t
holy_netbsd_boot (void)
{
  struct holy_netbsd_bootinfo *bootinfo;
  void *curarg, *arg0;
  holy_addr_t arg_target, stack_target;
  holy_uint32_t *stack;
  holy_err_t err;
  struct holy_relocator32_state state;
  holy_size_t tag_buf_len = 0;
  int tag_count = 0;

  err = holy_bsd_add_mmap ();
  if (err)
    return err;

  err = holy_netbsd_setup_video ();
  if (err)
    {
      holy_print_error ();
      holy_puts_ (N_("Booting in blind mode"));
      holy_errno = holy_ERR_NONE;
    }

  err = holy_netbsd_add_modules ();
  if (err)
    return err;

#ifdef holy_MACHINE_EFI
  err = holy_bsd_add_meta (NETBSD_BTINFO_EFI,
			   &holy_efi_system_table,
			   sizeof (holy_efi_system_table));
  if (err)
    return err;
#endif

  {
    struct bsd_tag *tag;
    tag_buf_len = 0;
    for (tag = tags; tag; tag = tag->next)
      {
	tag_buf_len = ALIGN_VAR (tag_buf_len
				 + sizeof (struct holy_netbsd_btinfo_common)
				 + tag->len);
	tag_count++;
      }
  }

  arg_target = kern_end;
  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   arg_target, tag_buf_len
					   + sizeof (struct holy_netbsd_bootinfo)
					   + tag_count * sizeof (holy_uint32_t));
    if (err)
      return err;
    curarg = get_virtual_current_address (ch);
  }

  arg0 = curarg;
  bootinfo = (void *) ((holy_uint8_t *) arg0 + tag_buf_len);

  {
    struct bsd_tag *tag;
    unsigned i;

    bootinfo->bi_count = tag_count;
    for (tag = tags, i = 0; tag; i++, tag = tag->next)
      {
	struct holy_netbsd_btinfo_common *head = curarg;
	bootinfo->bi_data[i] = ((holy_uint8_t *) curarg - (holy_uint8_t *) arg0)
	  + arg_target;
	head->type = tag->type;
	head->len = tag->len + sizeof (*head);
	curarg = head + 1;
	holy_memcpy (curarg, tag->data, tag->len);
	curarg = (holy_uint8_t *) curarg + tag->len;
      }
  }

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_align (relocator, &ch, 0x10000, 0x90000,
					    7 * sizeof (holy_uint32_t), 4,
					    holy_RELOCATOR_PREFERENCE_NONE,
					    0);
    if (err)
      return err;
    stack = get_virtual_current_address (ch);
    stack_target = get_physical_target_address (ch);
  }

#ifdef holy_MACHINE_EFI
  err = holy_efi_finish_boot_services (NULL, NULL, NULL, NULL, NULL);
  if (err)
    return err;
#endif

  state.eip = entry;
  state.esp = stack_target;
  state.ebp = stack_target;
  stack[0] = entry;
  stack[1] = bootflags;
  stack[2] = 0;
  stack[3] = ((holy_uint8_t *) bootinfo - (holy_uint8_t *) arg0) + arg_target;
  stack[4] = 0;
  stack[5] = holy_mmap_get_upper () >> 10;
  stack[6] = holy_mmap_get_lower () >> 10;

  return holy_relocator32_boot (relocator, state, 0);
}

static holy_err_t
holy_bsd_unload (void)
{
  struct bsd_tag *tag, *next;
  for (tag = tags; tag; tag = next)
    {
      next = tag->next;
      holy_free (tag);
    }
  tags = NULL;
  tags_last = NULL;

  kernel_type = KERNEL_TYPE_NONE;
  holy_dl_unref (my_mod);

  holy_relocator_unload (relocator);
  relocator = NULL;

  return holy_ERR_NONE;
}

static holy_err_t
holy_bsd_load_aout (holy_file_t file, const char *filename)
{
  holy_addr_t load_addr, load_end;
  int ofs, align_page;
  union holy_aout_header ah;
  holy_err_t err;
  holy_size_t bss_size;

  if ((holy_file_seek (file, 0)) == (holy_off_t) - 1)
    return holy_errno;

  if (holy_file_read (file, &ah, sizeof (ah)) != sizeof (ah))
    {
      if (!holy_errno)
	holy_error (holy_ERR_READ_ERROR, N_("premature end of file %s"),
		    filename);
      return holy_errno;
    }

  if (holy_aout_get_type (&ah) != AOUT_TYPE_AOUT32)
    return holy_error (holy_ERR_BAD_OS, "invalid a.out header");

  entry = ah.aout32.a_entry & 0xFFFFFF;

  if (AOUT_GETMAGIC (ah.aout32) == AOUT32_ZMAGIC)
    {
      load_addr = entry;
      ofs = 0x1000;
      align_page = 0;
    }
  else
    {
      load_addr = entry & 0xF00000;
      ofs = sizeof (struct holy_aout32_header);
      align_page = 1;
    }

  if (load_addr < 0x100000)
    return holy_error (holy_ERR_BAD_OS, "load address below 1M");

  kern_start = load_addr;
  load_end = kern_end = load_addr + ah.aout32.a_text + ah.aout32.a_data;
  if (align_page)
    kern_end = ALIGN_PAGE (kern_end);

  if (ah.aout32.a_bss)
    {
      kern_end += ah.aout32.a_bss;
      if (align_page)
	kern_end = ALIGN_PAGE (kern_end);

      bss_size = kern_end - load_end;
    }
  else
    bss_size = 0;

  {
    holy_relocator_chunk_t ch;

    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   kern_start, kern_end - kern_start);
    if (err)
      return err;
    kern_chunk_src = get_virtual_current_address (ch);
  }

  return holy_aout_load (file, ofs, kern_chunk_src,
			 ah.aout32.a_text + ah.aout32.a_data,
			 bss_size);
}

static holy_err_t
holy_bsd_load_elf (holy_elf_t elf, const char *filename)
{
  holy_err_t err;

  kern_end = 0;
  kern_start = ~0;

  if (holy_elf_is_elf32 (elf))
    {
      holy_relocator_chunk_t ch;
      Elf32_Phdr *phdr;

      entry = elf->ehdr.ehdr32.e_entry & 0xFFFFFFF;

      FOR_ELF32_PHDRS (elf, phdr)
	{
	  Elf32_Addr paddr;

	  if (phdr->p_type != PT_LOAD
	      && phdr->p_type != PT_DYNAMIC)
	    continue;

	  paddr = phdr->p_paddr & 0xFFFFFFF;

	  if (paddr < kern_start)
	    kern_start = paddr;

	  if (paddr + phdr->p_memsz > kern_end)
	    kern_end = paddr + phdr->p_memsz;
	}

      if (holy_errno)
	return holy_errno;
      err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					     kern_start, kern_end - kern_start);
      if (err)
	return err;

      kern_chunk_src = get_virtual_current_address (ch);

      err = holy_elf32_load (elf, filename, (holy_uint8_t *) kern_chunk_src - kern_start, holy_ELF_LOAD_FLAGS_LOAD_PT_DYNAMIC | holy_ELF_LOAD_FLAGS_28BITS, 0, 0);
      if (err)
	return err;
      if (kernel_type != KERNEL_TYPE_OPENBSD)
	return holy_ERR_NONE;
      return holy_openbsd_find_ramdisk32 (elf->file, filename, kern_start,
					  kern_chunk_src, &openbsd_ramdisk);
    }
  else if (holy_elf_is_elf64 (elf))
    {
      Elf64_Phdr *phdr;

      is_64bit = 1;

      if (! holy_cpuid_has_longmode)
	return holy_error (holy_ERR_BAD_OS, "your CPU does not implement AMD64 architecture");

      /* FreeBSD has 64-bit entry point.  */
      if (kernel_type == KERNEL_TYPE_FREEBSD)
	{
	  entry = elf->ehdr.ehdr64.e_entry & 0xffffffff;
	  entry_hi = (elf->ehdr.ehdr64.e_entry >> 32) & 0xffffffff;
	}
      else
	{
	  entry = elf->ehdr.ehdr64.e_entry & 0x0fffffff;
	  entry_hi = 0;
	}

      FOR_ELF64_PHDRS (elf, phdr)
	{
	  Elf64_Addr paddr;

	  if (phdr->p_type != PT_LOAD
	      && phdr->p_type != PT_DYNAMIC)
	    continue;

	  paddr = phdr->p_paddr & 0xFFFFFFF;

	  if (paddr < kern_start)
	    kern_start = paddr;

	  if (paddr + phdr->p_memsz > kern_end)
	    kern_end = paddr + phdr->p_memsz;
	}

      if (holy_errno)
	return holy_errno;

      holy_dprintf ("bsd", "kern_start = %lx, kern_end = %lx\n",
		    (unsigned long) kern_start, (unsigned long) kern_end);
      {
	holy_relocator_chunk_t ch;

	err = holy_relocator_alloc_chunk_addr (relocator, &ch, kern_start,
					       kern_end - kern_start);
	if (err)
	  return err;
	kern_chunk_src = get_virtual_current_address (ch);
      }

      err = holy_elf64_load (elf, filename,
			     (holy_uint8_t *) kern_chunk_src - kern_start, holy_ELF_LOAD_FLAGS_LOAD_PT_DYNAMIC | holy_ELF_LOAD_FLAGS_28BITS, 0, 0);
      if (err)
	return err;
      if (kernel_type != KERNEL_TYPE_OPENBSD)
	return holy_ERR_NONE;
      return holy_openbsd_find_ramdisk64 (elf->file, filename, kern_start,
					  kern_chunk_src, &openbsd_ramdisk);
    }
  else
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));
}

static holy_err_t
holy_bsd_load (int argc, char *argv[])
{
  holy_file_t file;
  holy_elf_t elf;

  holy_dl_ref (my_mod);

  holy_loader_unset ();

  holy_memset (&openbsd_ramdisk, 0, sizeof (openbsd_ramdisk));

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  file = holy_file_open (argv[0]);
  if (!file)
    goto fail;

  relocator = holy_relocator_new ();
  if (!relocator)
    {
      holy_file_close (file);
      goto fail;
    }

  elf = holy_elf_file (file, argv[0]);
  if (elf)
    {
      is_elf_kernel = 1;
      holy_bsd_load_elf (elf, argv[0]);
      holy_elf_close (elf);
    }
  else
    {
      is_elf_kernel = 0;
      holy_errno = 0;
      holy_bsd_load_aout (file, argv[0]);
      holy_file_close (file);
    }

  kern_end = ALIGN_PAGE (kern_end);

fail:

  if (holy_errno != holy_ERR_NONE)
    holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_uint32_t
holy_bsd_parse_flags (const struct holy_arg_list *state,
		      const holy_uint32_t * flags)
{
  holy_uint32_t result = 0;
  unsigned i;

  for (i = 0; flags[i]; i++)
    if (state[i].set)
      result |= flags[i];

  return result;
}

static holy_err_t
holy_cmd_freebsd (holy_extcmd_context_t ctxt, int argc, char *argv[])
{
  kernel_type = KERNEL_TYPE_FREEBSD;
  bootflags = holy_bsd_parse_flags (ctxt->state, freebsd_flags);

  if (holy_bsd_load (argc, argv) == holy_ERR_NONE)
    {
      holy_uint32_t unit, slice, part;

      kern_end = ALIGN_PAGE (kern_end);
      if (is_elf_kernel)
	{
	  holy_err_t err;
	  holy_uint64_t data = 0;
	  holy_file_t file;
	  int len = is_64bit ? 8 : 4;

	  err = holy_freebsd_add_meta_module (argv[0], is_64bit
					      ? FREEBSD_MODTYPE_KERNEL64
					      : FREEBSD_MODTYPE_KERNEL,
					      argc - 1, argv + 1,
					      kern_start,
					      kern_end - kern_start);
	  if (err)
	    return err;

	  file = holy_file_open (argv[0]);
	  if (! file)
	    return holy_errno;

	  if (is_64bit)
	    err = holy_freebsd_load_elf_meta64 (relocator, file, argv[0],
						&kern_end);
	  else
	    err = holy_freebsd_load_elf_meta32 (relocator, file, argv[0],
						&kern_end);
	  if (err)
	    return err;

	  err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA |
				   FREEBSD_MODINFOMD_HOWTO, &data, 4);
	  if (err)
	    return err;

	  err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA |
				       FREEBSD_MODINFOMD_ENVP, &data, len);
	  if (err)
	    return err;

	  err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA |
				   FREEBSD_MODINFOMD_KERNEND, &data, len);
	  if (err)
	    return err;
	}
      holy_bsd_get_device (&freebsd_biosdev, &unit, &slice, &part);
      freebsd_zfsguid = 0;
      if (!is_64bit)
	freebsd_get_zfs ();
      holy_print_error ();
      freebsd_bootdev = (FREEBSD_B_DEVMAGIC + ((slice + 1) << FREEBSD_B_SLICESHIFT) +
			 (unit << FREEBSD_B_UNITSHIFT) + (part << FREEBSD_B_PARTSHIFT));

      holy_loader_set (holy_freebsd_boot, holy_bsd_unload, 0);
    }

  return holy_errno;
}

static const char *types[] = {
  [0] = "wd",
  [2] = "fd",
  [4] = "sd",
  [6] = "cd",
  [14] = "vnd",
  [17] = "rd"
};

static holy_err_t
holy_cmd_openbsd (holy_extcmd_context_t ctxt, int argc, char *argv[])
{
  holy_uint32_t bootdev;

  kernel_type = KERNEL_TYPE_OPENBSD;
  bootflags = holy_bsd_parse_flags (ctxt->state, openbsd_flags);

  if (ctxt->state[OPENBSD_ROOT_ARG].set)
    {
      const char *arg = ctxt->state[OPENBSD_ROOT_ARG].arg;
      unsigned type, unit, part;
      for (type = 0; type < ARRAY_SIZE (types); type++)
	if (types[type]
	    && holy_strncmp (arg, types[type],
			     holy_strlen (types[type])) == 0)
	  {
	    arg += holy_strlen (types[type]);
	    break;
	  }
      if (type == ARRAY_SIZE (types))
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   "unknown disk type name");

      unit = holy_strtoul (arg, (char **) &arg, 10);
      if (! (arg && *arg >= 'a' && *arg <= 'z'))
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   "only device specifications of form "
			   "<type><number><lowercase letter> are supported");

      part = *arg - 'a';

      bootdev = (OPENBSD_B_DEVMAGIC | (type << OPENBSD_B_TYPESHIFT)
		 | (unit << OPENBSD_B_UNITSHIFT)
		 | (part << OPENBSD_B_PARTSHIFT));
    }
  else
    bootdev = 0;

  if (ctxt->state[OPENBSD_SERIAL_ARG].set)
    {
      struct holy_openbsd_bootarg_console serial;
      char *ptr;
      unsigned port = 0;
      unsigned speed = 9600;

      holy_memset (&serial, 0, sizeof (serial));

      if (ctxt->state[OPENBSD_SERIAL_ARG].arg)
	{
	  ptr = ctxt->state[OPENBSD_SERIAL_ARG].arg;
	  if (holy_memcmp (ptr, "com", sizeof ("com") - 1) != 0)
	    return holy_error (holy_ERR_BAD_ARGUMENT,
			       "only com0-com3 are supported");
	  ptr += sizeof ("com") - 1;
	  port = holy_strtoul (ptr, &ptr, 0);
	  if (port >= 4)
	    return holy_error (holy_ERR_BAD_ARGUMENT,
			       "only com0-com3 are supported");
	  if (*ptr == ',')
	    {
	      ptr++; 
	      speed = holy_strtoul (ptr, &ptr, 0);
	      if (holy_errno)
		return holy_errno;
	    }
	}

      serial.device = (holy_OPENBSD_COM_MAJOR << 8) | port;
      serial.speed = speed;
      serial.addr = holy_ns8250_hw_get_port (port);
	  
      holy_bsd_add_meta (OPENBSD_BOOTARG_CONSOLE, &serial, sizeof (serial));
      bootflags |= OPENBSD_RB_SERCONS;
    }
  else
    {
      struct holy_openbsd_bootarg_console serial;

      holy_memset (&serial, 0, sizeof (serial));
      serial.device = (holy_OPENBSD_VGA_MAJOR << 8);
      serial.addr = 0xffffffff;
      holy_bsd_add_meta (OPENBSD_BOOTARG_CONSOLE, &serial, sizeof (serial));
      bootflags &= ~OPENBSD_RB_SERCONS;
    }

  if (holy_bsd_load (argc, argv) == holy_ERR_NONE)
    {
      holy_loader_set (holy_openbsd_boot, holy_bsd_unload, 0);
      openbsd_root = bootdev;
    }

  return holy_errno;
}

static holy_err_t
holy_cmd_netbsd (holy_extcmd_context_t ctxt, int argc, char *argv[])
{
  holy_err_t err;
  kernel_type = KERNEL_TYPE_NETBSD;
  bootflags = holy_bsd_parse_flags (ctxt->state, netbsd_flags);

  if (holy_bsd_load (argc, argv) == holy_ERR_NONE)
    {
      if (is_elf_kernel)
	{
	  holy_file_t file;

	  file = holy_file_open (argv[0]);
	  if (! file)
	    return holy_errno;

	  if (is_64bit)
	    err = holy_netbsd_load_elf_meta64 (relocator, file, argv[0], &kern_end);
	  else
	    err = holy_netbsd_load_elf_meta32 (relocator, file, argv[0], &kern_end);
	  if (err)
	    return err;
	}

      {
	char bootpath[holy_NETBSD_MAX_BOOTPATH_LEN];
	char *name;
	name = holy_strrchr (argv[0], '/');
	if (name)
	  name++;
	else
	  name = argv[0];
	holy_memset (bootpath, 0, sizeof (bootpath));
	holy_strncpy (bootpath, name, sizeof (bootpath) - 1);
	holy_bsd_add_meta (NETBSD_BTINFO_BOOTPATH, bootpath, sizeof (bootpath));
      }

      if (ctxt->state[NETBSD_ROOT_ARG].set)
	{
	  char root[holy_NETBSD_MAX_ROOTDEVICE_LEN];
	  holy_memset (root, 0, sizeof (root));
	  holy_strncpy (root, ctxt->state[NETBSD_ROOT_ARG].arg,
			sizeof (root) - 1);
	  holy_bsd_add_meta (NETBSD_BTINFO_ROOTDEVICE, root, sizeof (root));
	}
      if (ctxt->state[NETBSD_SERIAL_ARG].set)
	{
	  struct holy_netbsd_btinfo_serial serial;
	  char *ptr;

	  holy_memset (&serial, 0, sizeof (serial));
	  holy_strcpy (serial.devname, "com");

	  serial.addr = holy_ns8250_hw_get_port (0);
	  serial.speed = 9600;

	  if (ctxt->state[NETBSD_SERIAL_ARG].arg)
	    {
	      ptr = ctxt->state[NETBSD_SERIAL_ARG].arg;
	      if (holy_memcmp (ptr, "com", sizeof ("com") - 1) == 0)
		{
		  ptr += sizeof ("com") - 1;
		  serial.addr 
		    = holy_ns8250_hw_get_port (holy_strtoul (ptr, &ptr, 0));
		}
	      else
		serial.addr = holy_strtoul (ptr, &ptr, 0);
	      if (holy_errno)
		return holy_errno;

	      if (*ptr == ',')
		{
		  ptr++;
		  serial.speed = holy_strtoul (ptr, &ptr, 0);
		  if (holy_errno)
		    return holy_errno;
		}
	    }

 	  holy_bsd_add_meta (NETBSD_BTINFO_CONSOLE, &serial, sizeof (serial));
	}
      else
	{
	  struct holy_netbsd_btinfo_serial cons;

	  holy_memset (&cons, 0, sizeof (cons));
	  holy_strcpy (cons.devname, "pc");

	  holy_bsd_add_meta (NETBSD_BTINFO_CONSOLE, &cons, sizeof (cons));
	}

      holy_netbsd_add_boot_disk_and_wedge ();

      holy_loader_set (holy_netbsd_boot, holy_bsd_unload, 0);
    }

  return holy_errno;
}

static holy_err_t
holy_cmd_freebsd_loadenv (holy_command_t cmd __attribute__ ((unused)),
			  int argc, char *argv[])
{
  holy_file_t file = 0;
  char *buf = 0, *curr, *next;
  int len;

  if (! holy_loader_is_loaded ())
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("you need to load the kernel first"));

  if (kernel_type != KERNEL_TYPE_FREEBSD)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "only FreeBSD supports environment");

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  file = holy_file_open (argv[0]);
  if ((!file) || (!file->size))
    goto fail;

  len = file->size;
  buf = holy_malloc (len + 1);
  if (!buf)
    goto fail;

  if (holy_file_read (file, buf, len) != len)
    goto fail;

  buf[len] = 0;

  next = buf;
  while (next)
    {
      char *p;

      curr = next;
      next = holy_strchr (curr, '\n');
      if (next)
	{

	  p = next - 1;
	  while (p > curr)
	    {
	      if ((*p != '\r') && (*p != ' ') && (*p != '\t'))
		break;
	      p--;
	    }

	  if ((p > curr) && (*p == '"'))
	    p--;

	  *(p + 1) = 0;
	  next++;
	}

      if (*curr == '#')
	continue;

      p = holy_strchr (curr, '=');
      if (!p)
	continue;

      *(p++) = 0;

      if (*curr)
	{
	  char *name;

	  if (*p == '"')
	    p++;

	  name = holy_xasprintf ("kFreeBSD.%s", curr);
	  if (!name)
	    goto fail;
	  if (holy_env_set (name, p))
	    {
	      holy_free (name);
	      goto fail;
	    }
	  holy_free (name);
	}
    }

fail:
  holy_free (buf);

  if (file)
    holy_file_close (file);

  return holy_errno;
}

static holy_err_t
holy_cmd_freebsd_module (holy_command_t cmd __attribute__ ((unused)),
			 int argc, char *argv[])
{
  holy_file_t file = 0;
  int modargc;
  char **modargv;
  const char *type;
  holy_err_t err;
  void *src;

  if (! holy_loader_is_loaded ())
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("you need to load the kernel first"));

  if (kernel_type != KERNEL_TYPE_FREEBSD)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no FreeBSD loaded");

  if (!is_elf_kernel)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "only ELF kernel supports module");

  /* List the current modules if no parameter.  */
  if (!argc)
    {
      holy_freebsd_list_modules ();
      return 0;
    }

  file = holy_file_open (argv[0]);
  if ((!file) || (!file->size))
    goto fail;

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch, kern_end,
					   file->size);
    if (err)
      goto fail;
    src = get_virtual_current_address (ch);
  }


  holy_file_read (file, src, file->size);
  if (holy_errno)
    goto fail;

  modargc = argc - 1;
  modargv = argv + 1;

  if (modargc && (! holy_memcmp (modargv[0], "type=", 5)))
    {
      type = &modargv[0][5];
      modargc--;
      modargv++;
    }
  else
    type = FREEBSD_MODTYPE_RAW;

  err = holy_freebsd_add_meta_module (argv[0], type, modargc, modargv,
				      kern_end, file->size);
  if (err)
    goto fail;

  kern_end = ALIGN_PAGE (kern_end + file->size);

fail:
  if (file)
    holy_file_close (file);

  return holy_errno;
}

static holy_err_t
holy_netbsd_module_load (char *filename, holy_uint32_t type)
{
  holy_file_t file = 0;
  void *src;
  holy_err_t err;

  file = holy_file_open (filename);
  if ((!file) || (!file->size))
    goto fail;

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch, kern_end,
					   file->size);
    if (err)
      goto fail;

    src = get_virtual_current_address (ch);
  }

  holy_file_read (file, src, file->size);
  if (holy_errno)
    goto fail;

  err = holy_netbsd_add_meta_module (filename, type, kern_end, file->size);

  if (err)
    goto fail;

  kern_end = ALIGN_PAGE (kern_end + file->size);

fail:
  if (file)
    holy_file_close (file);

  return holy_errno;
}

static holy_err_t
holy_cmd_netbsd_module (holy_command_t cmd,
			int argc, char *argv[])
{
  holy_uint32_t type;

  if (! holy_loader_is_loaded ())
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("you need to load the kernel first"));

  if (kernel_type != KERNEL_TYPE_NETBSD)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no NetBSD loaded");

  if (!is_elf_kernel)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "only ELF kernel supports module");

  /* List the current modules if no parameter.  */
  if (!argc)
    {
      holy_netbsd_list_modules ();
      return 0;
    }

  if (holy_strcmp (cmd->name, "knetbsd_module_elf") == 0)
    type = holy_NETBSD_MODULE_ELF;
  else
    type = holy_NETBSD_MODULE_RAW;

  return holy_netbsd_module_load (argv[0], type);
}

static holy_err_t
holy_cmd_freebsd_module_elf (holy_command_t cmd __attribute__ ((unused)),
			     int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_err_t err;

  if (! holy_loader_is_loaded ())
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("you need to load the kernel first"));

  if (kernel_type != KERNEL_TYPE_FREEBSD)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "only FreeBSD supports module");

  if (! is_elf_kernel)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "only ELF kernel supports module");

  /* List the current modules if no parameter.  */
  if (! argc)
    {
      holy_freebsd_list_modules ();
      return 0;
    }

  file = holy_file_open (argv[0]);
  if (!file)
    return holy_errno;
  if (!file->size)
    {
      holy_file_close (file);
      return holy_errno;
    }

  if (is_64bit)
    err = holy_freebsd_load_elfmodule_obj64 (relocator, file,
					     argc, argv, &kern_end);
  else
    err = holy_freebsd_load_elfmodule32 (relocator, file,
					 argc, argv, &kern_end);
  holy_file_close (file);

  return err;
}

static holy_err_t
holy_cmd_openbsd_ramdisk (holy_command_t cmd __attribute__ ((unused)),
		      int argc, char *args[])
{
  holy_file_t file;
  holy_size_t size;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (! holy_loader_is_loaded ())
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("you need to load the kernel first"));

  if (kernel_type != KERNEL_TYPE_OPENBSD)
    return holy_error (holy_ERR_BAD_OS, "no kOpenBSD loaded");

  if (!openbsd_ramdisk.max_size)
    return holy_error (holy_ERR_BAD_OS, "your kOpenBSD doesn't support ramdisk");

  file = holy_file_open (args[0]);
  if (! file)
    return holy_errno;

  size = holy_file_size (file);

  if (size > openbsd_ramdisk.max_size)
    {
      holy_file_close (file);
      return holy_error (holy_ERR_BAD_OS, "your kOpenBSD supports ramdisk only"
			 " up to %u bytes, however you supplied a %u bytes one",
			 openbsd_ramdisk.max_size, size);
    }

  if (holy_file_read (file, openbsd_ramdisk.target, size)
      != (holy_ssize_t) (size))
    {
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), args[0]);
      return holy_errno;
    }
  holy_memset (openbsd_ramdisk.target + size, 0,
	       openbsd_ramdisk.max_size - size);
  *openbsd_ramdisk.size = ALIGN_UP (size, 512);

  return holy_ERR_NONE;
}

static holy_extcmd_t cmd_freebsd, cmd_openbsd, cmd_netbsd;
static holy_command_t cmd_freebsd_loadenv, cmd_freebsd_module;
static holy_command_t cmd_netbsd_module, cmd_freebsd_module_elf;
static holy_command_t cmd_netbsd_module_elf, cmd_openbsd_ramdisk;

holy_MOD_INIT (bsd)
{
  /* Net and OpenBSD kernels are often compressed.  */
  holy_dl_load ("gzio");

  cmd_freebsd = holy_register_extcmd ("kfreebsd", holy_cmd_freebsd, 0,
				      N_("FILE"), N_("Load kernel of FreeBSD."),
				      freebsd_opts);
  cmd_openbsd = holy_register_extcmd ("kopenbsd", holy_cmd_openbsd, 0,
				      N_("FILE"), N_("Load kernel of OpenBSD."),
				      openbsd_opts);
  cmd_netbsd = holy_register_extcmd ("knetbsd", holy_cmd_netbsd,  0,
				     N_("FILE"), N_("Load kernel of NetBSD."),
				     netbsd_opts);
  cmd_freebsd_loadenv =
    holy_register_command ("kfreebsd_loadenv", holy_cmd_freebsd_loadenv,
			   0, N_("Load FreeBSD env."));
  cmd_freebsd_module =
    holy_register_command ("kfreebsd_module", holy_cmd_freebsd_module,
			   0, N_("Load FreeBSD kernel module."));
  cmd_netbsd_module =
    holy_register_command ("knetbsd_module", holy_cmd_netbsd_module,
			   0, N_("Load NetBSD kernel module."));
  cmd_netbsd_module_elf =
    holy_register_command ("knetbsd_module_elf", holy_cmd_netbsd_module,
			   0, N_("Load NetBSD kernel module (ELF)."));
  cmd_freebsd_module_elf =
    holy_register_command ("kfreebsd_module_elf", holy_cmd_freebsd_module_elf,
			   0, N_("Load FreeBSD kernel module (ELF)."));

  cmd_openbsd_ramdisk = holy_register_command ("kopenbsd_ramdisk",
					       holy_cmd_openbsd_ramdisk, 0,
					       /* TRANSLATORS: ramdisk isn't identifier,
						it can be translated.  */
					       N_("Load kOpenBSD ramdisk."));

  my_mod = mod;
}

holy_MOD_FINI (bsd)
{
  holy_unregister_extcmd (cmd_freebsd);
  holy_unregister_extcmd (cmd_openbsd);
  holy_unregister_extcmd (cmd_netbsd);

  holy_unregister_command (cmd_freebsd_loadenv);
  holy_unregister_command (cmd_freebsd_module);
  holy_unregister_command (cmd_netbsd_module);
  holy_unregister_command (cmd_freebsd_module_elf);
  holy_unregister_command (cmd_netbsd_module_elf);
  holy_unregister_command (cmd_openbsd_ramdisk);

  holy_bsd_unload ();
}
