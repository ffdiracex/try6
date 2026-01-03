/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/xnu.h>
#include <holy/cpu/xnu.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/loader.h>
#include <holy/machoload.h>
#include <holy/macho.h>
#include <holy/cpu/macho.h>
#include <holy/command.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/env.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#if defined (__i386) && !defined (holy_MACHINE_EFI)
#include <holy/autoefi.h>
#endif

struct holy_xnu_devtree_key *holy_xnu_devtree_root = 0;
static int driverspackagenum = 0;
static int driversnum = 0;
int holy_xnu_is_64bit = 0;
int holy_xnu_darwin_version = 0;

holy_addr_t holy_xnu_heap_target_start = 0;
holy_size_t holy_xnu_heap_size = 0;
struct holy_relocator *holy_xnu_relocator;

static holy_err_t
holy_xnu_register_memory (const char *prefix, int *suffix,
			  holy_addr_t addr, holy_size_t size);
holy_err_t
holy_xnu_heap_malloc (int size, void **src, holy_addr_t *target)
{
  holy_err_t err;
  holy_relocator_chunk_t ch;
  
  err = holy_relocator_alloc_chunk_addr (holy_xnu_relocator, &ch,
					 holy_xnu_heap_target_start
					 + holy_xnu_heap_size, size);
  if (err)
    return err;

  *src = get_virtual_current_address (ch);
  *target = holy_xnu_heap_target_start + holy_xnu_heap_size;
  holy_xnu_heap_size += size;
  holy_dprintf ("xnu", "val=%p\n", *src);
  return holy_ERR_NONE;
}

/* Make sure next block of the heap will be aligned.
   Please notice: aligned are pointers AFTER relocation
   and not the current ones. */
holy_err_t
holy_xnu_align_heap (int align)
{
  holy_xnu_heap_size
    = ALIGN_UP (holy_xnu_heap_target_start+ holy_xnu_heap_size, align)
    - holy_xnu_heap_target_start;
  return holy_ERR_NONE;
}

/* Free subtree pointed by CUR. */
void
holy_xnu_free_devtree (struct holy_xnu_devtree_key *cur)
{
  struct holy_xnu_devtree_key *d;
  while (cur)
    {
      holy_free (cur->name);
      if (cur->datasize == -1)
	holy_xnu_free_devtree (cur->first_child);
      else if (cur->data)
	holy_free (cur->data);
      d = cur->next;
      holy_free (cur);
      cur = d;
    }
}

/* Compute the size of device tree in xnu format. */
static holy_size_t
holy_xnu_writetree_get_size (struct holy_xnu_devtree_key *start,
			     const char *name)
{
  holy_size_t ret;
  struct holy_xnu_devtree_key *cur;

  /* Key header. */
  ret = 2 * sizeof (holy_uint32_t);

  /* "name" value. */
  ret += 32 + sizeof (holy_uint32_t)
    + holy_strlen (name) + 4
    - (holy_strlen (name) % 4);

  for (cur = start; cur; cur = cur->next)
    if (cur->datasize != -1)
      {
	int align_overhead;

	align_overhead = 4 - (cur->datasize % 4);
	if (align_overhead == 4)
	  align_overhead = 0;
	ret += 32 + sizeof (holy_uint32_t) + cur->datasize + align_overhead;
      }
    else
      ret += holy_xnu_writetree_get_size (cur->first_child, cur->name);
  return ret;
}

/* Write devtree in XNU format at curptr assuming the head is named NAME.*/
static void *
holy_xnu_writetree_toheap_real (void *curptr,
				struct holy_xnu_devtree_key *start,
				const char *name)
{
  struct holy_xnu_devtree_key *cur;
  int nkeys = 0, nvals = 0;
  for (cur = start; cur; cur = cur->next)
    {
      if (cur->datasize == -1)
	nkeys++;
      else
	nvals++;
    }
  /* For the name. */
  nvals++;

  *((holy_uint32_t *) curptr) = nvals;
  curptr = ((holy_uint32_t *) curptr) + 1;
  *((holy_uint32_t *) curptr) = nkeys;
  curptr = ((holy_uint32_t *) curptr) + 1;

  /* First comes "name" value. */
  holy_memset (curptr, 0, 32);
  holy_memcpy (curptr, "name", 4);
  curptr = ((holy_uint8_t *) curptr) + 32;
  *((holy_uint32_t *)curptr) = holy_strlen (name) + 1;
  curptr = ((holy_uint32_t *) curptr) + 1;
  holy_memcpy (curptr, name, holy_strlen (name));
  curptr = ((holy_uint8_t *) curptr) + holy_strlen (name);
  holy_memset (curptr, 0, 4 - (holy_strlen (name) % 4));
  curptr = ((holy_uint8_t *) curptr) + (4 - (holy_strlen (name) % 4));

  /* Then the other values. */
  for (cur = start; cur; cur = cur->next)
    if (cur->datasize != -1)
      {
	int align_overhead;

	align_overhead = 4 - (cur->datasize % 4);
	if (align_overhead == 4)
	  align_overhead = 0;
	holy_memset (curptr, 0, 32);
	holy_strncpy (curptr, cur->name, 31);
	curptr = ((holy_uint8_t *) curptr) + 32;
	*((holy_uint32_t *) curptr) = cur->datasize;
	curptr = ((holy_uint32_t *) curptr) + 1;
	holy_memcpy (curptr, cur->data, cur->datasize);
	curptr = ((holy_uint8_t *) curptr) + cur->datasize;
	holy_memset (curptr, 0, align_overhead);
	curptr = ((holy_uint8_t *) curptr) + align_overhead;
      }

  /* And then the keys. Recursively use this function. */
  for (cur = start; cur; cur = cur->next)
    if (cur->datasize == -1)
      {
	curptr = holy_xnu_writetree_toheap_real (curptr,
						 cur->first_child,
						 cur->name);
	if (!curptr)
	  return 0;
      }
  return curptr;
}

holy_err_t
holy_xnu_writetree_toheap (holy_addr_t *target, holy_size_t *size)
{
  struct holy_xnu_devtree_key *chosen;
  struct holy_xnu_devtree_key *memorymap;
  struct holy_xnu_devtree_key *driverkey;
  struct holy_xnu_extdesc *extdesc;
  holy_err_t err;
  void *src;

  err = holy_xnu_align_heap (holy_XNU_PAGESIZE);
  if (err)
    return err;

  /* Device tree itself is in the memory map of device tree. */
  /* Create a dummy value in memory-map. */
  chosen = holy_xnu_create_key (&holy_xnu_devtree_root, "chosen");
  if (! chosen)
    return holy_errno;
  memorymap = holy_xnu_create_key (&(chosen->first_child), "memory-map");
  if (! memorymap)
    return holy_errno;

  driverkey = (struct holy_xnu_devtree_key *) holy_malloc (sizeof (*driverkey));
  if (! driverkey)
    return holy_errno;
  driverkey->name = holy_strdup ("DeviceTree");
  if (! driverkey->name)
    return holy_errno;
  driverkey->datasize = sizeof (*extdesc);
  driverkey->next = memorymap->first_child;
  memorymap->first_child = driverkey;
  driverkey->data = extdesc
    = (struct holy_xnu_extdesc *) holy_malloc (sizeof (*extdesc));
  if (! driverkey->data)
    return holy_errno;

  /* Allocate the space based on the size with dummy value. */
  *size = holy_xnu_writetree_get_size (holy_xnu_devtree_root, "/");
  err = holy_xnu_heap_malloc (ALIGN_UP (*size + 1, holy_XNU_PAGESIZE),
			      &src, target);
  if (err)
    return err;

  /* Put real data in the dummy. */
  extdesc->addr = *target;
  extdesc->size = (holy_uint32_t) *size;

  /* Write the tree to heap. */
  holy_xnu_writetree_toheap_real (src, holy_xnu_devtree_root, "/");
  return holy_ERR_NONE;
}

/* Find a key or value in parent key. */
struct holy_xnu_devtree_key *
holy_xnu_find_key (struct holy_xnu_devtree_key *parent, const char *name)
{
  struct holy_xnu_devtree_key *cur;
  for (cur = parent; cur; cur = cur->next)
    if (holy_strcmp (cur->name, name) == 0)
      return cur;
  return 0;
}

struct holy_xnu_devtree_key *
holy_xnu_create_key (struct holy_xnu_devtree_key **parent, const char *name)
{
  struct holy_xnu_devtree_key *ret;
  ret = holy_xnu_find_key (*parent, name);
  if (ret)
    return ret;
  ret = (struct holy_xnu_devtree_key *) holy_zalloc (sizeof (*ret));
  if (! ret)
    return 0;
  ret->name = holy_strdup (name);
  if (! ret->name)
    {
      holy_free (ret);
      return 0;
    }
  ret->datasize = -1;
  ret->next = *parent;
  *parent = ret;
  return ret;
}

struct holy_xnu_devtree_key *
holy_xnu_create_value (struct holy_xnu_devtree_key **parent, const char *name)
{
  struct holy_xnu_devtree_key *ret;
  ret = holy_xnu_find_key (*parent, name);
  if (ret)
    {
      if (ret->datasize == -1)
	holy_xnu_free_devtree (ret->first_child);
      else if (ret->datasize)
	holy_free (ret->data);
      ret->datasize = 0;
      ret->data = 0;
      return ret;
    }
  ret = (struct holy_xnu_devtree_key *) holy_zalloc (sizeof (*ret));
  if (! ret)
    return 0;
  ret->name = holy_strdup (name);
  if (! ret->name)
    {
      holy_free (ret);
      return 0;
    }
  ret->next = *parent;
  *parent = ret;
  return ret;
}

static holy_err_t
holy_xnu_unload (void)
{
  holy_cpu_xnu_unload ();

  holy_xnu_free_devtree (holy_xnu_devtree_root);
  holy_xnu_devtree_root = 0;

  /* Free loaded image. */
  driversnum = 0;
  driverspackagenum = 0;
  holy_relocator_unload (holy_xnu_relocator);
  holy_xnu_relocator = NULL;
  holy_xnu_heap_target_start = 0;
  holy_xnu_heap_size = 0;
  holy_xnu_unlock ();
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_xnu_kernel (holy_command_t cmd __attribute__ ((unused)),
		     int argc, char *args[])
{
  holy_err_t err;
  holy_macho_t macho;
  holy_uint32_t startcode, endcode;
  int i;
  char *ptr;
  void *loadaddr;
  holy_addr_t loadaddr_target;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_xnu_unload ();

  macho = holy_macho_open (args[0], 0);
  if (! macho)
    return holy_errno;

  err = holy_macho_size32 (macho, &startcode, &endcode, holy_MACHO_NOBSS,
			   args[0]);
  if (err)
    {
      holy_macho_close (macho);
      holy_xnu_unload ();
      return err;
    }

  holy_dprintf ("xnu", "endcode = %lx, startcode = %lx\n",
		(unsigned long) endcode, (unsigned long) startcode);

  holy_xnu_relocator = holy_relocator_new ();
  if (!holy_xnu_relocator)
    return holy_errno;
  holy_xnu_heap_target_start = startcode;
  err = holy_xnu_heap_malloc (endcode - startcode, &loadaddr,
			      &loadaddr_target);

  if (err)
    {
      holy_macho_close (macho);
      holy_xnu_unload ();
      return err;
    }

  /* Load kernel. */
  err = holy_macho_load32 (macho, args[0], (char *) loadaddr - startcode,
			   holy_MACHO_NOBSS, &holy_xnu_darwin_version);
  if (err)
    {
      holy_macho_close (macho);
      holy_xnu_unload ();
      return err;
    }

  holy_xnu_entry_point = holy_macho_get_entry_point32 (macho, args[0]);
  if (! holy_xnu_entry_point)
    {
      holy_macho_close (macho);
      holy_xnu_unload ();
      return holy_error (holy_ERR_BAD_OS, "couldn't find entry point");
    }

  holy_macho_close (macho);

  err = holy_xnu_align_heap (holy_XNU_PAGESIZE);
  if (err)
    {
      holy_xnu_unload ();
      return err;
    }

  /* Copy parameters to kernel command line. */
  ptr = holy_xnu_cmdline;
  for (i = 1; i < argc; i++)
    {
      if (ptr + holy_strlen (args[i]) + 1
	  >= holy_xnu_cmdline + sizeof (holy_xnu_cmdline))
	break;
      holy_memcpy (ptr, args[i], holy_strlen (args[i]));
      ptr += holy_strlen (args[i]);
      *ptr = ' ';
      ptr++;
    }

  /* Replace last space by '\0'. */
  if (ptr != holy_xnu_cmdline)
    *(ptr - 1) = 0;

#if defined (__i386) && !defined (holy_MACHINE_EFI)
  err = holy_efiemu_autocore ();
  if (err)
    return err;
#endif

  holy_loader_set (holy_xnu_boot, holy_xnu_unload, 0);

  holy_xnu_lock ();
  holy_xnu_is_64bit = 0;

  return 0;
}

static holy_err_t
holy_cmd_xnu_kernel64 (holy_command_t cmd __attribute__ ((unused)),
		       int argc, char *args[])
{
  holy_err_t err;
  holy_macho_t macho;
  holy_uint64_t startcode, endcode;
  int i;
  char *ptr;
  void *loadaddr;
  holy_addr_t loadaddr_target;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_xnu_unload ();

  macho = holy_macho_open (args[0], 1);
  if (! macho)
    return holy_errno;

  err = holy_macho_size64 (macho, &startcode, &endcode, holy_MACHO_NOBSS,
			   args[0]);
  if (err)
    {
      holy_macho_close (macho);
      holy_xnu_unload ();
      return err;
    }

  startcode &= 0x0fffffff;
  endcode &= 0x0fffffff;

  holy_dprintf ("xnu", "endcode = %lx, startcode = %lx\n",
		(unsigned long) endcode, (unsigned long) startcode);

  holy_xnu_relocator = holy_relocator_new ();
  if (!holy_xnu_relocator)
    return holy_errno;
  holy_xnu_heap_target_start = startcode;
  err = holy_xnu_heap_malloc (endcode - startcode, &loadaddr,
			      &loadaddr_target);

  if (err)
    {
      holy_macho_close (macho);
      holy_xnu_unload ();
      return err;
    }

  /* Load kernel. */
  err = holy_macho_load64 (macho, args[0], (char *) loadaddr - startcode,
			   holy_MACHO_NOBSS, &holy_xnu_darwin_version);
  if (err)
    {
      holy_macho_close (macho);
      holy_xnu_unload ();
      return err;
    }

  holy_xnu_entry_point = holy_macho_get_entry_point64 (macho, args[0])
    & 0x0fffffff;
  if (! holy_xnu_entry_point)
    {
      holy_macho_close (macho);
      holy_xnu_unload ();
      return holy_error (holy_ERR_BAD_OS, "couldn't find entry point");
    }

  holy_macho_close (macho);

  err = holy_xnu_align_heap (holy_XNU_PAGESIZE);
  if (err)
    {
      holy_xnu_unload ();
      return err;
    }

  /* Copy parameters to kernel command line. */
  ptr = holy_xnu_cmdline;
  for (i = 1; i < argc; i++)
    {
      if (ptr + holy_strlen (args[i]) + 1
	  >= holy_xnu_cmdline + sizeof (holy_xnu_cmdline))
	break;
      holy_memcpy (ptr, args[i], holy_strlen (args[i]));
      ptr += holy_strlen (args[i]);
      *ptr = ' ';
      ptr++;
    }

  /* Replace last space by '\0'. */
  if (ptr != holy_xnu_cmdline)
    *(ptr - 1) = 0;

#if defined (__i386) && !defined (holy_MACHINE_EFI)
  err = holy_efiemu_autocore ();
  if (err)
    return err;
#endif

  holy_loader_set (holy_xnu_boot, holy_xnu_unload, 0);

  holy_xnu_lock ();
  holy_xnu_is_64bit = 1;

  return 0;
}

/* Register a memory in a memory map under name PREFIXSUFFIX
   and increment SUFFIX. */
static holy_err_t
holy_xnu_register_memory (const char *prefix, int *suffix,
			  holy_addr_t addr, holy_size_t size)
{
  struct holy_xnu_devtree_key *chosen;
  struct holy_xnu_devtree_key *memorymap;
  struct holy_xnu_devtree_key *driverkey;
  struct holy_xnu_extdesc *extdesc;

  if (! holy_xnu_heap_size)
    return holy_error (holy_ERR_BAD_OS, N_("you need to load the kernel first"));

  chosen = holy_xnu_create_key (&holy_xnu_devtree_root, "chosen");
  if (! chosen)
    return holy_errno;
  memorymap = holy_xnu_create_key (&(chosen->first_child), "memory-map");
  if (! memorymap)
    return holy_errno;

  driverkey = (struct holy_xnu_devtree_key *) holy_malloc (sizeof (*driverkey));
  if (! driverkey)
    return holy_errno;
  if (suffix)
    driverkey->name = holy_xasprintf ("%s%d", prefix, (*suffix)++);
  else
    driverkey->name = holy_strdup (prefix);
  if (!driverkey->name)
    {
      holy_free (driverkey);
      return holy_errno;
    }
  driverkey->datasize = sizeof (*extdesc);
  driverkey->next = memorymap->first_child;
  driverkey->data = extdesc
    = (struct holy_xnu_extdesc *) holy_malloc (sizeof (*extdesc));
  if (! driverkey->data)
    {
      holy_free (driverkey->name);
      holy_free (driverkey);
      return holy_errno;
    }
  memorymap->first_child = driverkey;
  extdesc->addr = addr;
  extdesc->size = (holy_uint32_t) size;
  return holy_ERR_NONE;
}

static inline char *
get_name_ptr (char *name)
{
  char *p = name, *p2;
  /* Skip Info.plist.  */
  p2 = holy_strrchr (p, '/');
  if (!p2)
    return name;
  if (p2 == name)
    return name + 1;
  p = p2 - 1;

  p2 = holy_strrchr (p, '/');
  if (!p2)
    return name;
  if (p2 == name)
    return name + 1;
  if (holy_memcmp (p2, "/Contents/", sizeof ("/Contents/") - 1) != 0)
    return p2 + 1;

  p = p2 - 1;

  p2 = holy_strrchr (p, '/');
  if (!p2)
    return name;
  return p2 + 1;
}

/* Load .kext. */
static holy_err_t
holy_xnu_load_driver (char *infoplistname, holy_file_t binaryfile,
		      const char *filename)
{
  holy_macho_t macho;
  holy_err_t err;
  holy_file_t infoplist;
  struct holy_xnu_extheader *exthead;
  int neededspace = sizeof (*exthead);
  holy_uint8_t *buf;
  void *buf0;
  holy_addr_t buf_target;
  holy_size_t infoplistsize = 0, machosize = 0;
  char *name, *nameend;
  int namelen;

  name = get_name_ptr (infoplistname);
  nameend = holy_strchr (name, '/');

  if (nameend)
    namelen = nameend - name;
  else
    namelen = holy_strlen (name);

  neededspace += namelen + 1;

  if (! holy_xnu_heap_size)
    return holy_error (holy_ERR_BAD_OS, N_("you need to load the kernel first"));

  /* Compute the needed space. */
  if (binaryfile)
    {
      macho = holy_macho_file (binaryfile, filename, holy_xnu_is_64bit);
      if (!macho)
	holy_file_close (binaryfile);
      else
	{
	  if (holy_xnu_is_64bit)
	    machosize = holy_macho_filesize64 (macho);
	  else
	    machosize = holy_macho_filesize32 (macho);
	}
      neededspace += machosize;
    }
  else
    macho = 0;

  if (infoplistname)
    infoplist = holy_file_open (infoplistname);
  else
    infoplist = 0;
  holy_errno = holy_ERR_NONE;
  if (infoplist)
    {
      infoplistsize = holy_file_size (infoplist);
      neededspace += infoplistsize + 1;
    }
  else
    infoplistsize = 0;

  /* Allocate the space. */
  err = holy_xnu_align_heap (holy_XNU_PAGESIZE);
  if (err)
    goto fail;
  err = holy_xnu_heap_malloc (neededspace, &buf0, &buf_target);
  if (err)
    goto fail;
  buf = buf0;

  exthead = (struct holy_xnu_extheader *) buf;
  holy_memset (exthead, 0, sizeof (*exthead));
  buf += sizeof (*exthead);

  /* Load the binary. */
  if (macho)
    {
      exthead->binaryaddr = buf_target + (buf - (holy_uint8_t *) buf0);
      exthead->binarysize = machosize;
      if (holy_xnu_is_64bit)
	err = holy_macho_readfile64 (macho, filename, buf);
      else
	err = holy_macho_readfile32 (macho, filename, buf);
      if (err)
	goto fail;
      holy_macho_close (macho);
      buf += machosize;
    }
  holy_errno = holy_ERR_NONE;

  /* Load the plist. */
  if (infoplist)
    {
      exthead->infoplistaddr = buf_target + (buf - (holy_uint8_t *) buf0);
      exthead->infoplistsize = infoplistsize + 1;
      if (holy_file_read (infoplist, buf, infoplistsize)
	  != (holy_ssize_t) (infoplistsize))
	{
	  holy_file_close (infoplist);
	  if (!holy_errno)
	    holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			infoplistname);
	  return holy_errno;
	}
      holy_file_close (infoplist);
      buf[infoplistsize] = 0;
      buf += infoplistsize + 1;
    }
  holy_errno = holy_ERR_NONE;

  exthead->nameaddr = (buf - (holy_uint8_t *) buf0) + buf_target;
  exthead->namesize = namelen + 1;
  holy_memcpy (buf, name, namelen);
  buf[namelen] = 0;
  buf += namelen + 1;

  /* Announce to kernel */
  return holy_xnu_register_memory ("Driver-", &driversnum, buf_target,
				   neededspace);
fail:
  if (macho)
    holy_macho_close (macho);
  return err;
}

/* Load mkext. */
static holy_err_t
holy_cmd_xnu_mkext (holy_command_t cmd __attribute__ ((unused)),
		    int argc, char *args[])
{
  holy_file_t file;
  void *loadto;
  holy_addr_t loadto_target;
  holy_err_t err;
  holy_off_t readoff = 0;
  holy_ssize_t readlen = -1;
  struct holy_macho_fat_header head;
  struct holy_macho_fat_arch *archs;
  int narchs, i;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (! holy_xnu_heap_size)
    return holy_error (holy_ERR_BAD_OS, N_("you need to load the kernel first"));

  file = holy_file_open (args[0]);
  if (! file)
    return holy_errno;

  /* Sometimes caches are fat binary. Errgh. */
  if (holy_file_read (file, &head, sizeof (head))
      != (holy_ssize_t) (sizeof (head)))
    {
      /* I don't know the internal structure of package but
	 can hardly imagine a valid package shorter than 20 bytes. */
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), args[0]);
      return holy_errno;
    }

  /* Find the corresponding architecture. */
  if (holy_be_to_cpu32 (head.magic) == holy_MACHO_FAT_MAGIC)
    {
      narchs = holy_be_to_cpu32 (head.nfat_arch);
      archs = holy_malloc (sizeof (struct holy_macho_fat_arch) * narchs);
      if (! archs)
	{
	  holy_file_close (file);
	  return holy_errno;

	}
      if (holy_file_read (file, archs,
			  sizeof (struct holy_macho_fat_arch) * narchs)
	  != (holy_ssize_t) sizeof(struct holy_macho_fat_arch) * narchs)
	{
	  holy_free (archs);
	  if (!holy_errno)
	    holy_error (holy_ERR_READ_ERROR, N_("premature end of file %s"),
			args[0]);
	  return holy_errno;
	}
      for (i = 0; i < narchs; i++)
	{
	  if (!holy_xnu_is_64bit && holy_MACHO_CPUTYPE_IS_HOST32
	      (holy_be_to_cpu32 (archs[i].cputype)))
	    {
	      readoff = holy_be_to_cpu32 (archs[i].offset);
	      readlen = holy_be_to_cpu32 (archs[i].size);
	    }
	  if (holy_xnu_is_64bit && holy_MACHO_CPUTYPE_IS_HOST64
	      (holy_be_to_cpu32 (archs[i].cputype)))
	    {
	      readoff = holy_be_to_cpu32 (archs[i].offset);
	      readlen = holy_be_to_cpu32 (archs[i].size);
	    }
	}
      holy_free (archs);
    }
  else
    {
      /* It's a flat file. Some sane people still exist. */
      readoff = 0;
      readlen = holy_file_size (file);
    }

  if (readlen == -1)
    {
      holy_file_close (file);
      return holy_error (holy_ERR_BAD_OS, "no suitable architecture is found");
    }

  /* Allocate space. */
  err = holy_xnu_align_heap (holy_XNU_PAGESIZE);
  if (err)
    {
      holy_file_close (file);
      return err;
    }

  err = holy_xnu_heap_malloc (readlen, &loadto, &loadto_target);
  if (err)
    {
      holy_file_close (file);
      return err;
    }

  /* Read the file. */
  holy_file_seek (file, readoff);
  if (holy_file_read (file, loadto, readlen) != (holy_ssize_t) (readlen))
    {
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), args[0]);
      return holy_errno;
    }
  holy_file_close (file);

  /* Pass it to kernel. */
  return holy_xnu_register_memory ("DriversPackage-", &driverspackagenum,
				   loadto_target, readlen);
}

static holy_err_t
holy_cmd_xnu_ramdisk (holy_command_t cmd __attribute__ ((unused)),
		      int argc, char *args[])
{
  holy_file_t file;
  void *loadto;
  holy_addr_t loadto_target;
  holy_err_t err;
  holy_size_t size;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (! holy_xnu_heap_size)
    return holy_error (holy_ERR_BAD_OS, N_("you need to load the kernel first"));

  file = holy_file_open (args[0]);
  if (! file)
    return holy_errno;

  err = holy_xnu_align_heap (holy_XNU_PAGESIZE);
  if (err)
    return err;

  size = holy_file_size (file);

  err = holy_xnu_heap_malloc (size, &loadto, &loadto_target);
  if (err)
    return err;
  if (holy_file_read (file, loadto, size) != (holy_ssize_t) (size))
    {
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), args[0]);
      return holy_errno;
    }
  return holy_xnu_register_memory ("RAMDisk", 0, loadto_target, size);
}

/* Returns true if the kext should be loaded according to plist
   and osbundlereq. Also fill BINNAME. */
static int
holy_xnu_check_os_bundle_required (char *plistname,
				   const char *osbundlereq,
				   char **binname)
{
  holy_file_t file;
  char *buf = 0, *tagstart = 0, *ptr1 = 0, *keyptr = 0;
  char *stringptr = 0, *ptr2 = 0;
  holy_size_t size;
  int depth = 0;
  int ret;
  int osbundlekeyfound = 0, binnamekeyfound = 0;
  if (binname)
    *binname = 0;

  file = holy_file_open (plistname);
  if (! file)
    return 0;

  size = holy_file_size (file);
  buf = holy_malloc (size);
  if (! buf)
    {
      holy_file_close (file);
      return 0;
    }
  if (holy_file_read (file, buf, size) != (holy_ssize_t) (size))
    {
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), plistname);
      return 0;
    }
  holy_file_close (file);

  /* Set the return value for the case when no OSBundleRequired tag is found. */
  if (osbundlereq)
    ret = holy_strword (osbundlereq, "all") || holy_strword (osbundlereq, "-");
  else
    ret = 1;

  /* Parse plist. It's quite dirty and inextensible but does its job. */
  for (ptr1 = buf; ptr1 < buf + size; ptr1++)
    switch (*ptr1)
      {
      case '<':
	tagstart = ptr1;
	*ptr1 = 0;
	if (keyptr && depth == 4
	    && holy_strcmp (keyptr, "OSBundleRequired") == 0)
	  osbundlekeyfound = 1;
	if (keyptr && depth == 4 &&
	    holy_strcmp (keyptr, "CFBundleExecutable") == 0)
	  binnamekeyfound = 1;
	if (stringptr && osbundlekeyfound && osbundlereq && depth == 4)
	  {
	    for (ptr2 = stringptr; *ptr2; ptr2++)
	      *ptr2 = holy_tolower (*ptr2);
	    ret = holy_strword (osbundlereq, stringptr)
	      || holy_strword (osbundlereq, "all");
	  }
	if (stringptr && binnamekeyfound && binname && depth == 4)
	  {
	    if (*binname)
	      holy_free (*binname);
	    *binname = holy_strdup (stringptr);
	  }

	*ptr1 = '<';
	keyptr = 0;
	stringptr = 0;
	break;
      case '>':
	if (! tagstart)
	  {
	    holy_free (buf);
	    holy_error (holy_ERR_BAD_OS, "can't parse %s", plistname);
	    return 0;
	  }
	*ptr1 = 0;
	if (tagstart[1] == '?' || ptr1[-1] == '/')
	  {
	    osbundlekeyfound = 0;
	    *ptr1 = '>';
	    break;
	  }
	if (depth == 3 && holy_strcmp (tagstart + 1, "key") == 0)
	  keyptr = ptr1 + 1;
	if (depth == 3 && holy_strcmp (tagstart + 1, "string") == 0)
	  stringptr = ptr1 + 1;
	else if (holy_strcmp (tagstart + 1, "/key") != 0)
	  {
	    osbundlekeyfound = 0;
	    binnamekeyfound = 0;
	  }
	*ptr1 = '>';

	if (tagstart[1] == '/')
	  depth--;
	else
	  depth++;
	break;
      }
  holy_free (buf);

  return ret;
}

/* Context for holy_xnu_scan_dir_for_kexts.  */
struct holy_xnu_scan_dir_for_kexts_ctx
{
  char *dirname;
  const char *osbundlerequired;
  int maxrecursion;
};

/* Helper for holy_xnu_scan_dir_for_kexts.  */
static int
holy_xnu_scan_dir_for_kexts_load (const char *filename,
				  const struct holy_dirhook_info *info,
				  void *data)
{
  struct holy_xnu_scan_dir_for_kexts_ctx *ctx = data;
  char *newdirname;

  if (! info->dir)
    return 0;
  if (filename[0] == '.')
    return 0;

  if (holy_strlen (filename) < 5 ||
      holy_memcmp (filename + holy_strlen (filename) - 5, ".kext", 5) != 0)
    return 0;

  newdirname
    = holy_malloc (holy_strlen (ctx->dirname) + holy_strlen (filename) + 2);

  /* It's a .kext. Try to load it. */
  if (newdirname)
    {
      holy_strcpy (newdirname, ctx->dirname);
      newdirname[holy_strlen (newdirname) + 1] = 0;
      newdirname[holy_strlen (newdirname)] = '/';
      holy_strcpy (newdirname + holy_strlen (newdirname), filename);
      holy_xnu_load_kext_from_dir (newdirname, ctx->osbundlerequired,
				   ctx->maxrecursion);
      if (holy_errno == holy_ERR_BAD_OS)
	holy_errno = holy_ERR_NONE;
      holy_free (newdirname);
    }
  return 0;
}

/* Load all loadable kexts placed under DIRNAME and matching OSBUNDLEREQUIRED */
holy_err_t
holy_xnu_scan_dir_for_kexts (char *dirname, const char *osbundlerequired,
			     int maxrecursion)
{
  struct holy_xnu_scan_dir_for_kexts_ctx ctx = {
    .dirname = dirname,
    .osbundlerequired = osbundlerequired,
    .maxrecursion = maxrecursion
  };
  holy_device_t dev;
  char *device_name;
  holy_fs_t fs;
  const char *path;

  if (! holy_xnu_heap_size)
    return holy_error (holy_ERR_BAD_OS, N_("you need to load the kernel first"));

  device_name = holy_file_get_device_name (dirname);
  dev = holy_device_open (device_name);
  if (dev)
    {
      fs = holy_fs_probe (dev);
      path = holy_strchr (dirname, ')');
      if (! path)
	path = dirname;
      else
	path++;

      if (fs)
	(fs->dir) (dev, path, holy_xnu_scan_dir_for_kexts_load, &ctx);
      holy_device_close (dev);
    }
  holy_free (device_name);

  return holy_ERR_NONE;
}

/* Context for holy_xnu_load_kext_from_dir.  */
struct holy_xnu_load_kext_from_dir_ctx
{
  char *dirname;
  const char *osbundlerequired;
  int maxrecursion;
  char *plistname;
  char *newdirname;
  int usemacos;
};

/* Helper for holy_xnu_load_kext_from_dir.  */
static int
holy_xnu_load_kext_from_dir_load (const char *filename,
				  const struct holy_dirhook_info *info,
				  void *data)
{
  struct holy_xnu_load_kext_from_dir_ctx *ctx = data;

  if (holy_strlen (filename) > 15)
    return 0;
  holy_strcpy (ctx->newdirname + holy_strlen (ctx->dirname) + 1, filename);

  /* If the kext contains directory "Contents" all real stuff is in
     this directory. */
  if (info->dir && holy_strcasecmp (filename, "Contents") == 0)
    holy_xnu_load_kext_from_dir (ctx->newdirname, ctx->osbundlerequired,
				 ctx->maxrecursion - 1);

  /* Directory "Plugins" contains nested kexts. */
  if (info->dir && holy_strcasecmp (filename, "Plugins") == 0)
    holy_xnu_scan_dir_for_kexts (ctx->newdirname, ctx->osbundlerequired,
				 ctx->maxrecursion - 1);

  /* Directory "MacOS" contains executable, otherwise executable is
     on the top. */
  if (info->dir && holy_strcasecmp (filename, "MacOS") == 0)
    ctx->usemacos = 1;

  /* Info.plist is the file which governs our future actions. */
  if (! info->dir && holy_strcasecmp (filename, "Info.plist") == 0
      && ! ctx->plistname)
    ctx->plistname = holy_strdup (ctx->newdirname);
  return 0;
}

/* Load extension DIRNAME. (extensions are directories in xnu) */
holy_err_t
holy_xnu_load_kext_from_dir (char *dirname, const char *osbundlerequired,
			     int maxrecursion)
{
  struct holy_xnu_load_kext_from_dir_ctx ctx = {
    .dirname = dirname,
    .osbundlerequired = osbundlerequired,
    .maxrecursion = maxrecursion,
    .plistname = 0,
    .usemacos = 0
  };
  holy_device_t dev;
  char *newpath;
  char *device_name;
  holy_fs_t fs;
  const char *path;
  char *binsuffix;
  holy_file_t binfile;

  ctx.newdirname = holy_malloc (holy_strlen (dirname) + 20);
  if (! ctx.newdirname)
    return holy_errno;
  holy_strcpy (ctx.newdirname, dirname);
  ctx.newdirname[holy_strlen (dirname)] = '/';
  ctx.newdirname[holy_strlen (dirname) + 1] = 0;
  device_name = holy_file_get_device_name (dirname);
  dev = holy_device_open (device_name);
  if (dev)
    {
      fs = holy_fs_probe (dev);
      path = holy_strchr (dirname, ')');
      if (! path)
	path = dirname;
      else
	path++;

      newpath = holy_strchr (ctx.newdirname, ')');
      if (! newpath)
	newpath = ctx.newdirname;
      else
	newpath++;

      /* Look at the directory. */
      if (fs)
	(fs->dir) (dev, path, holy_xnu_load_kext_from_dir_load, &ctx);

      if (ctx.plistname && holy_xnu_check_os_bundle_required
	  (ctx.plistname, osbundlerequired, &binsuffix))
	{
	  if (binsuffix)
	    {
	      /* Open the binary. */
	      char *binname = holy_malloc (holy_strlen (dirname)
					   + holy_strlen (binsuffix)
					   + sizeof ("/MacOS/"));
	      holy_strcpy (binname, dirname);
	      if (ctx.usemacos)
		holy_strcpy (binname + holy_strlen (binname), "/MacOS/");
	      else
		holy_strcpy (binname + holy_strlen (binname), "/");
	      holy_strcpy (binname + holy_strlen (binname), binsuffix);
	      holy_dprintf ("xnu", "%s:%s\n", ctx.plistname, binname);
	      binfile = holy_file_open (binname);
	      if (! binfile)
		holy_errno = holy_ERR_NONE;

	      /* Load the extension. */
	      holy_xnu_load_driver (ctx.plistname, binfile,
				    binname);
	      holy_free (binname);
	      holy_free (binsuffix);
	    }
	  else
	    {
	      holy_dprintf ("xnu", "%s:0\n", ctx.plistname);
	      holy_xnu_load_driver (ctx.plistname, 0, 0);
	    }
	}
      holy_free (ctx.plistname);
      holy_device_close (dev);
    }
  holy_free (device_name);

  return holy_ERR_NONE;
}


static int locked=0;
static holy_dl_t my_mod;

/* Load the kext. */
static holy_err_t
holy_cmd_xnu_kext (holy_command_t cmd __attribute__ ((unused)),
		   int argc, char *args[])
{
  holy_file_t binfile = 0;

  if (! holy_xnu_heap_size)
    return holy_error (holy_ERR_BAD_OS, N_("you need to load the kernel first"));

  if (argc == 2)
    {
      /* User explicitly specified plist and binary. */
      if (holy_strcmp (args[1], "-") != 0)
	{
	  binfile = holy_file_open (args[1]);
	  if (! binfile)
	    return holy_errno;
	}
      return holy_xnu_load_driver (holy_strcmp (args[0], "-") ? args[0] : 0,
				   binfile, args[1]);
    }

  /* load kext normally. */
  if (argc == 1)
    return holy_xnu_load_kext_from_dir (args[0], 0, 10);

  return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
}

/* Load a directory containing kexts. */
static holy_err_t
holy_cmd_xnu_kextdir (holy_command_t cmd __attribute__ ((unused)),
		      int argc, char *args[])
{
  if (argc != 1 && argc != 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, "directory name required");

  if (! holy_xnu_heap_size)
    return holy_error (holy_ERR_BAD_OS, N_("you need to load the kernel first"));

  if (argc == 1)
    return holy_xnu_scan_dir_for_kexts (args[0],
					"console,root,local-root,network-root",
					10);
  else
    {
      char *osbundlerequired = holy_strdup (args[1]), *ptr;
      holy_err_t err;
      if (! osbundlerequired)
	return holy_errno;
      for (ptr = osbundlerequired; *ptr; ptr++)
	*ptr = holy_tolower (*ptr);
      err = holy_xnu_scan_dir_for_kexts (args[0], osbundlerequired, 10);
      holy_free (osbundlerequired);
      return err;
    }
}

static inline int
hextoval (char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10;
  return 0;
}

static inline void
unescape (char *name, char *curdot, char *nextdot, int *len)
{
  char *ptr, *dptr;
  dptr = name;
  for (ptr = curdot; ptr < nextdot;)
    if (ptr + 2 < nextdot && *ptr == '%')
      {
	*dptr = (hextoval (ptr[1]) << 4) | (hextoval (ptr[2]));
	ptr += 3;
	dptr++;
      }
    else
      {
	*dptr = *ptr;
	ptr++;
	dptr++;
      }
  *len = dptr - name;
}

holy_err_t
holy_xnu_fill_devicetree (void)
{
  struct holy_env_var *var;
  FOR_SORTED_ENV (var)
  {
    char *nextdot = 0, *curdot;
    struct holy_xnu_devtree_key **curkey = &holy_xnu_devtree_root;
    struct holy_xnu_devtree_key *curvalue;
    char *name = 0, *data;
    int len;

    if (holy_memcmp (var->name, "XNU.DeviceTree.",
		     sizeof ("XNU.DeviceTree.") - 1) != 0)
      continue;

    curdot = var->name + sizeof ("XNU.DeviceTree.") - 1;
    nextdot = holy_strchr (curdot, '.');
    if (nextdot)
      nextdot++;
    while (nextdot)
      {
	name = holy_realloc (name, nextdot - curdot + 1);

	if (!name)
	  return holy_errno;

	unescape (name, curdot, nextdot, &len);
	name[len - 1] = 0;

	curkey = &(holy_xnu_create_key (curkey, name)->first_child);

	curdot = nextdot;
	nextdot = holy_strchr (nextdot, '.');
	if (nextdot)
	  nextdot++;
      }

    nextdot = curdot + holy_strlen (curdot) + 1;

    name = holy_realloc (name, nextdot - curdot + 1);
   
    if (!name)
      return holy_errno;
   
    unescape (name, curdot, nextdot, &len);
    name[len] = 0;

    curvalue = holy_xnu_create_value (curkey, name);
    if (!curvalue)
      return holy_errno;
    holy_free (name);
   
    data = holy_malloc (holy_strlen (var->value) + 1);
    if (!data)
      return holy_errno;
   
    unescape (data, var->value, var->value + holy_strlen (var->value),
	      &len);
    curvalue->datasize = len;
    curvalue->data = data;
  }

  return holy_errno;
}

struct holy_video_bitmap *holy_xnu_bitmap = 0;
holy_xnu_bitmap_mode_t holy_xnu_bitmap_mode;

/* Option array indices.  */
#define XNU_SPLASH_CMD_ARGINDEX_MODE 0

static const struct holy_arg_option xnu_splash_cmd_options[] =
  {
    {"mode", 'm', 0, N_("Background image mode."), N_("stretch|normal"),
     ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

static holy_err_t
holy_cmd_xnu_splash (holy_extcmd_context_t ctxt,
		     int argc, char *args[])
{
  holy_err_t err;
  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (! holy_xnu_heap_size)
    return holy_error (holy_ERR_BAD_OS, N_("you need to load the kernel first"));

  if (ctxt->state[XNU_SPLASH_CMD_ARGINDEX_MODE].set &&
      holy_strcmp (ctxt->state[XNU_SPLASH_CMD_ARGINDEX_MODE].arg,
		   "stretch") == 0)
    holy_xnu_bitmap_mode = holy_XNU_BITMAP_STRETCH;
  else
    holy_xnu_bitmap_mode = holy_XNU_BITMAP_CENTER;

  err = holy_video_bitmap_load (&holy_xnu_bitmap, args[0]);
  if (err)
    holy_xnu_bitmap = 0;

  return err;
}


#ifndef holy_MACHINE_EMU
static holy_err_t
holy_cmd_xnu_resume (holy_command_t cmd __attribute__ ((unused)),
		     int argc, char *args[])
{
  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  return holy_xnu_resume (args[0]);
}
#endif

void
holy_xnu_lock (void)
{
  if (!locked)
    holy_dl_ref (my_mod);
  locked = 1;
}

void
holy_xnu_unlock (void)
{
  if (locked)
    holy_dl_unref (my_mod);
  locked = 0;
}

static holy_command_t cmd_kernel64, cmd_kernel, cmd_mkext, cmd_kext;
static holy_command_t cmd_kextdir, cmd_ramdisk, cmd_resume;
static holy_extcmd_t cmd_splash;

holy_MOD_INIT(xnu)
{
  cmd_kernel = holy_register_command ("xnu_kernel", holy_cmd_xnu_kernel, 0,
				      N_("Load XNU image."));
  cmd_kernel64 = holy_register_command ("xnu_kernel64", holy_cmd_xnu_kernel64,
					0, N_("Load 64-bit XNU image."));
  cmd_mkext = holy_register_command ("xnu_mkext", holy_cmd_xnu_mkext, 0,
				     N_("Load XNU extension package."));
  cmd_kext = holy_register_command ("xnu_kext", holy_cmd_xnu_kext, 0,
				    N_("Load XNU extension."));
  cmd_kextdir = holy_register_command ("xnu_kextdir", holy_cmd_xnu_kextdir,
				       /* TRANSLATORS: OSBundleRequired is a
					  variable name in xnu extensions
					  manifests. It behaves mostly like
					  GNU/Linux runlevels.
				       */
				       N_("DIRECTORY [OSBundleRequired]"),
				       /* TRANSLATORS: There are many extensions
					  in extension directory.  */
				       N_("Load XNU extension directory."));
  cmd_ramdisk = holy_register_command ("xnu_ramdisk", holy_cmd_xnu_ramdisk, 0,
   /* TRANSLATORS: ramdisk here isn't identifier. It can be translated.  */
				       N_("Load XNU ramdisk. "
					  "It will be available in OS as md0."));
  cmd_splash = holy_register_extcmd ("xnu_splash",
				     holy_cmd_xnu_splash, 0, 0,
				     N_("Load a splash image for XNU."),
				     xnu_splash_cmd_options);

#ifndef holy_MACHINE_EMU
  cmd_resume = holy_register_command ("xnu_resume", holy_cmd_xnu_resume,
				      0, N_("Load an image of hibernated"
					    " XNU."));
#endif

  holy_cpu_xnu_init ();

  my_mod = mod;
}

holy_MOD_FINI(xnu)
{
#ifndef holy_MACHINE_EMU
  holy_unregister_command (cmd_resume);
#endif
  holy_unregister_command (cmd_mkext);
  holy_unregister_command (cmd_kext);
  holy_unregister_command (cmd_kextdir);
  holy_unregister_command (cmd_ramdisk);
  holy_unregister_command (cmd_kernel);
  holy_unregister_extcmd (cmd_splash);
  holy_unregister_command (cmd_kernel64);

  holy_cpu_xnu_fini ();
}
