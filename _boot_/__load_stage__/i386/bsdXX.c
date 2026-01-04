/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/i386/bsd.h>
#include <holy/mm.h>
#include <holy/elf.h>
#include <holy/misc.h>
#include <holy/i386/relocator.h>
#include <holy/i18n.h>

#define ALIGN_PAGE(a)	ALIGN_UP (a, 4096)

static inline holy_err_t
load (holy_file_t file, const char *filename, void *where, holy_off_t off, holy_size_t size)
{
  if (holy_file_seek (file, off) == (holy_off_t) -1)
    return holy_errno;
  if (holy_file_read (file, where, size) != (holy_ssize_t) size)
    {
      if (holy_errno)
	return holy_errno;
      return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			 filename);
    }
  return holy_ERR_NONE;
}

static inline holy_err_t
read_headers (holy_file_t file, const char *filename, Elf_Ehdr *e, char **shdr)
{
 if (holy_file_seek (file, 0) == (holy_off_t) -1)
    return holy_errno;

  if (holy_file_read (file, (char *) e, sizeof (*e)) != sizeof (*e))
    {
      if (holy_errno)
	return holy_errno;
      return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			 filename);
    }

  if (e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_version != EV_CURRENT)
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-independent ELF magic"));

  if (e->e_ident[EI_CLASS] != SUFFIX (ELFCLASS))
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  *shdr = holy_malloc ((holy_uint32_t) e->e_shnum * e->e_shentsize);
  if (! *shdr)
    return holy_errno;

  if (holy_file_seek (file, e->e_shoff) == (holy_off_t) -1)
    return holy_errno;

  if (holy_file_read (file, *shdr, (holy_uint32_t) e->e_shnum * e->e_shentsize)
      != (holy_ssize_t) ((holy_uint32_t) e->e_shnum * e->e_shentsize))
    {
      if (holy_errno)
	return holy_errno;
      return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			 filename);
    }

  return holy_ERR_NONE;
}

/* On i386 FreeBSD uses "elf module" approarch for 32-bit variant
   and "elf obj module" for 64-bit variant. However it may differ on other
   platforms. So I keep both versions.  */
#if OBJSYM
holy_err_t
SUFFIX (holy_freebsd_load_elfmodule_obj) (struct holy_relocator *relocator,
					  holy_file_t file, int argc,
					  char *argv[], holy_addr_t *kern_end)
{
  Elf_Ehdr e;
  Elf_Shdr *s;
  char *shdr = 0;
  holy_addr_t curload, module;
  holy_err_t err;
  holy_size_t chunk_size = 0;
  void *chunk_src;

  curload = module = ALIGN_PAGE (*kern_end);

  err = read_headers (file, argv[0], &e, &shdr);
  if (err)
    goto out;

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) ((char *) shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
    {
      if (s->sh_size == 0)
	continue;

      if (s->sh_addralign)
	chunk_size = ALIGN_UP (chunk_size + *kern_end, s->sh_addralign)
	  - *kern_end;

      chunk_size += s->sh_size;
    }

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   module, chunk_size);
    if (err)
      goto out;
    chunk_src = get_virtual_current_address (ch);
  }

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) ((char *) shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
    {
      if (s->sh_size == 0)
	continue;

      if (s->sh_addralign)
	curload = ALIGN_UP (curload, s->sh_addralign);
      s->sh_addr = curload;

      holy_dprintf ("bsd", "loading section to %x, size %d, align %d\n",
		    (unsigned) curload, (int) s->sh_size,
		    (int) s->sh_addralign);

      switch (s->sh_type)
	{
	default:
	case SHT_PROGBITS:
	  err = load (file, argv[0], (holy_uint8_t *) chunk_src + curload - *kern_end,
		      s->sh_offset, s->sh_size);
	  if (err)
	    goto out;
	  break;
	case SHT_NOBITS:
	  holy_memset ((holy_uint8_t *) chunk_src + curload - *kern_end, 0,
		       s->sh_size);
	  break;
	}
      curload += s->sh_size;
    }

  *kern_end = ALIGN_PAGE (curload);

  err = holy_freebsd_add_meta_module (argv[0], FREEBSD_MODTYPE_ELF_MODULE_OBJ,
				      argc - 1, argv + 1, module,
				      curload - module);
  if (! err)
    err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA
			     | FREEBSD_MODINFOMD_ELFHDR,
			     &e, sizeof (e));
  if (! err)
    err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA
			     | FREEBSD_MODINFOMD_SHDR,
			     shdr, e.e_shnum * e.e_shentsize);

out:
  holy_free (shdr);
  return err;
}

#else

holy_err_t
SUFFIX (holy_freebsd_load_elfmodule) (struct holy_relocator *relocator,
				      holy_file_t file, int argc, char *argv[],
				      holy_addr_t *kern_end)
{
  Elf_Ehdr e;
  Elf_Shdr *s;
  char *shdr = 0;
  holy_addr_t curload, module;
  holy_err_t err;
  holy_size_t chunk_size = 0;
  void *chunk_src;

  curload = module = ALIGN_PAGE (*kern_end);

  err = read_headers (file, argv[0], &e, &shdr);
  if (err)
    goto out;

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) ((char *) shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
    {
      if (s->sh_size == 0)
	continue;

      if (! (s->sh_flags & SHF_ALLOC))
	continue;
      if (chunk_size < s->sh_addr + s->sh_size)
	chunk_size = s->sh_addr + s->sh_size;
    }

  if (chunk_size < sizeof (e))
    chunk_size = sizeof (e);
  chunk_size += (holy_uint32_t) e.e_phnum * e.e_phentsize;
  chunk_size += (holy_uint32_t) e.e_shnum * e.e_shentsize;

  {
    holy_relocator_chunk_t ch;

    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   module, chunk_size);
    if (err)
      goto out;

    chunk_src = get_virtual_current_address (ch);
  }

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) ((char *) shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
    {
      if (s->sh_size == 0)
	continue;

      if (! (s->sh_flags & SHF_ALLOC))
	continue;

      holy_dprintf ("bsd", "loading section to %x, size %d, align %d\n",
		    (unsigned) curload, (int) s->sh_size,
		    (int) s->sh_addralign);

      switch (s->sh_type)
	{
	default:
	case SHT_PROGBITS:
	  err = load (file, argv[0],
		      (holy_uint8_t *) chunk_src + module
		      + s->sh_addr - *kern_end,
		      s->sh_offset, s->sh_size);
	  if (err)
	    goto out;
	  break;
	case SHT_NOBITS:
	  holy_memset ((holy_uint8_t *) chunk_src + module
		      + s->sh_addr - *kern_end, 0, s->sh_size);
	  break;
	}
      if (curload < module + s->sh_addr + s->sh_size)
	curload = module + s->sh_addr + s->sh_size;
    }

  load (file, argv[0], (holy_uint8_t *) chunk_src + module - *kern_end, 0, sizeof (e));
  if (curload < module + sizeof (e))
    curload = module + sizeof (e);

  load (file, argv[0], (holy_uint8_t *) chunk_src + curload - *kern_end, e.e_shoff,
	(holy_uint32_t) e.e_shnum * e.e_shentsize);
  e.e_shoff = curload - module;
  curload +=  (holy_uint32_t) e.e_shnum * e.e_shentsize;

  load (file, argv[0], (holy_uint8_t *) chunk_src + curload - *kern_end, e.e_phoff,
	(holy_uint32_t) e.e_phnum * e.e_phentsize);
  e.e_phoff = curload - module;
  curload +=  (holy_uint32_t) e.e_phnum * e.e_phentsize;

  *kern_end = curload;

  holy_freebsd_add_meta_module (argv[0], FREEBSD_MODTYPE_ELF_MODULE,
				argc - 1, argv + 1, module,
				curload - module);
out:
  holy_free (shdr);
  if (err)
    return err;
  return SUFFIX (holy_freebsd_load_elf_meta) (relocator, file, argv[0], kern_end);
}

#endif

holy_err_t
SUFFIX (holy_freebsd_load_elf_meta) (struct holy_relocator *relocator,
				     holy_file_t file,
				     const char *filename,
				     holy_addr_t *kern_end)
{
  holy_err_t err;
  Elf_Ehdr e;
  Elf_Shdr *s;
  char *shdr = 0;
  unsigned symoff, stroff, symsize, strsize;
  holy_freebsd_addr_t symstart, symend, symentsize, dynamic;
  Elf_Sym *sym;
  void *sym_chunk;
  holy_uint8_t *curload;
  holy_freebsd_addr_t symtarget;
  const char *str;
  unsigned i;
  holy_size_t chunk_size;

  err = read_headers (file, filename, &e, &shdr);
  if (err)
    goto out;

  err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA |
			   FREEBSD_MODINFOMD_ELFHDR, &e,
			   sizeof (e));
  if (err)
    goto out;

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) (shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
      if (s->sh_type == SHT_SYMTAB)
	break;
  if (s >= (Elf_Shdr *) ((char *) shdr
			+ e.e_shnum * e.e_shentsize))
    {
      err = holy_error (holy_ERR_BAD_OS, N_("no symbol table"));
      goto out;
    }
  symoff = s->sh_offset;
  symsize = s->sh_size;
  symentsize = s->sh_entsize;
  s = (Elf_Shdr *) (shdr + e.e_shentsize * s->sh_link);
  stroff = s->sh_offset;
  strsize = s->sh_size;

  chunk_size = ALIGN_UP (symsize + strsize, sizeof (holy_freebsd_addr_t))
    + 2 * sizeof (holy_freebsd_addr_t);

  symtarget = ALIGN_UP (*kern_end, sizeof (holy_freebsd_addr_t));

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   symtarget, chunk_size);
    if (err)
      goto out;
    sym_chunk = get_virtual_current_address (ch);
  }

  symstart = symtarget;
  symend = symstart + chunk_size;

  curload = sym_chunk;
  *((holy_freebsd_addr_t *) curload) = symsize;
  curload += sizeof (holy_freebsd_addr_t);

  if (holy_file_seek (file, symoff) == (holy_off_t) -1)
    {
      err = holy_errno;
      goto out;
    }
  sym = (Elf_Sym *) curload;
  if (holy_file_read (file, curload, symsize) != (holy_ssize_t) symsize)
    {
      if (! holy_errno)
	err = holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			  filename);
      else
	err = holy_errno;
      goto out;
    }
  curload += symsize;

  *((holy_freebsd_addr_t *) curload) = strsize;
  curload += sizeof (holy_freebsd_addr_t);
  if (holy_file_seek (file, stroff) == (holy_off_t) -1)
    {
      err = holy_errno;
      goto out;
    }
  str = (char *) curload;
  if (holy_file_read (file, curload, strsize) != (holy_ssize_t) strsize)
    {
      if (! holy_errno)
	err = holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			  filename);
      else
	err = holy_errno;
      goto out;
    }

  for (i = 0;
       i * symentsize < symsize;
       i++, sym = (Elf_Sym *) ((char *) sym + symentsize))
    {
      const char *name = str + sym->st_name;
      if (holy_strcmp (name, "_DYNAMIC") == 0)
	break;
    }

  if (i * symentsize < symsize)
    {
      dynamic = sym->st_value;
      holy_dprintf ("bsd", "dynamic = %llx\n", (unsigned long long) dynamic);
      err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA |
			       FREEBSD_MODINFOMD_DYNAMIC, &dynamic,
			       sizeof (dynamic));
      if (err)
	goto out;
    }

  err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA |
			   FREEBSD_MODINFOMD_SSYM, &symstart,
			   sizeof (symstart));
  if (err)
    goto out;

  err = holy_bsd_add_meta (FREEBSD_MODINFO_METADATA |
			   FREEBSD_MODINFOMD_ESYM, &symend,
			   sizeof (symend));
out:
  holy_free (shdr);
  if (err)
    return err;

  *kern_end = ALIGN_PAGE (symend);

  return holy_ERR_NONE;
}

holy_err_t
SUFFIX (holy_netbsd_load_elf_meta) (struct holy_relocator *relocator,
				    holy_file_t file, const char *filename,
				    holy_addr_t *kern_end)
{
  holy_err_t err;
  Elf_Ehdr e;
  Elf_Shdr *s, *symsh, *strsh;
  char *shdr = NULL;
  unsigned symsize, strsize;
  void *sym_chunk;
  holy_uint8_t *curload;
  holy_size_t chunk_size;
  Elf_Ehdr *e2;
  struct holy_netbsd_btinfo_symtab symtab;
  holy_addr_t symtarget;

  err = read_headers (file, filename, &e, &shdr);
  if (err)
    {
      holy_free (shdr);
      return holy_errno;
    }

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) (shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
      if (s->sh_type == SHT_SYMTAB)
	break;
  if (s >= (Elf_Shdr *) ((char *) shdr
			+ e.e_shnum * e.e_shentsize))
    {
      holy_free (shdr);
      return holy_ERR_NONE;
    }
  symsize = s->sh_size;
  symsh = s;
  s = (Elf_Shdr *) (shdr + e.e_shentsize * s->sh_link);
  strsize = s->sh_size;
  strsh = s;

  chunk_size = ALIGN_UP (symsize, sizeof (holy_freebsd_addr_t))
    + ALIGN_UP (strsize, sizeof (holy_freebsd_addr_t))
    + sizeof (e) + (holy_uint32_t) e.e_shnum * e.e_shentsize;

  symtarget = ALIGN_UP (*kern_end, sizeof (holy_freebsd_addr_t));
  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   symtarget, chunk_size);
    if (err)
      goto out;
    sym_chunk = get_virtual_current_address (ch);
  }

  symtab.nsyms = 1;
  symtab.ssyms = symtarget;
  symtab.esyms = symtarget + chunk_size;

  curload = sym_chunk;
  
  e2 = (Elf_Ehdr *) curload;
  holy_memcpy (curload, &e, sizeof (e));
  e2->e_phoff = 0;
  e2->e_phnum = 0;
  e2->e_phentsize = 0;
  e2->e_shstrndx = 0;
  e2->e_shoff = sizeof (e);

  curload += sizeof (e);

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) (shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
    {
      Elf_Shdr *s2;
      s2 = (Elf_Shdr *) curload;
      holy_memcpy (curload, s, e.e_shentsize);
      if (s == symsh)
	s2->sh_offset = sizeof (e) + (holy_uint32_t) e.e_shnum * e.e_shentsize;
      else if (s == strsh)
	s2->sh_offset = ALIGN_UP (symsize, sizeof (holy_freebsd_addr_t))
	  + sizeof (e) + (holy_uint32_t) e.e_shnum * e.e_shentsize;
      else
	s2->sh_offset = 0;
      s2->sh_addr = s2->sh_offset;
      curload += e.e_shentsize;
    }

  if (holy_file_seek (file, symsh->sh_offset) == (holy_off_t) -1)
    {
      err = holy_errno;
      goto out;
    }
  if (holy_file_read (file, curload, symsize) != (holy_ssize_t) symsize)
    {
      if (! holy_errno)
	err = holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			  filename);
      else
	err = holy_errno;
      goto out;
    }
  curload += ALIGN_UP (symsize, sizeof (holy_freebsd_addr_t));

  if (holy_file_seek (file, strsh->sh_offset) == (holy_off_t) -1)
    {
      err = holy_errno;
      goto out;
    }
  if (holy_file_read (file, curload, strsize) != (holy_ssize_t) strsize)
    {
      if (! holy_errno)
	err = holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			  filename);
      else
	err = holy_errno;
      goto out;
    }

  err = holy_bsd_add_meta (NETBSD_BTINFO_SYMTAB,
			   &symtab,
			   sizeof (symtab));
out:
  holy_free (shdr);
  if (err)
    return err;

  *kern_end = ALIGN_PAGE (symtarget + chunk_size);

  return holy_ERR_NONE;
}

holy_err_t
SUFFIX(holy_openbsd_find_ramdisk) (holy_file_t file,
				   const char *filename,
				   holy_addr_t kern_start,
				   void *kern_chunk_src,
				   struct holy_openbsd_ramdisk_descriptor *desc)
{
  unsigned symoff, stroff, symsize, strsize, symentsize;

  {
    holy_err_t err;
    Elf_Ehdr e;
    Elf_Shdr *s;
    char *shdr = NULL;
    
    err = read_headers (file, filename, &e, &shdr);
    if (err)
      {
	holy_free (shdr);
	return err;
      }

    for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) (shdr
						  + e.e_shnum * e.e_shentsize);
	 s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
      if (s->sh_type == SHT_SYMTAB)
	break;
    if (s >= (Elf_Shdr *) ((char *) shdr + e.e_shnum * e.e_shentsize))
      {
	holy_free (shdr);
	return holy_ERR_NONE;
      }

    symsize = s->sh_size;
    symentsize = s->sh_entsize;
    symoff = s->sh_offset;
    
    s = (Elf_Shdr *) (shdr + e.e_shentsize * s->sh_link);
    stroff = s->sh_offset;
    strsize = s->sh_size;
    holy_free (shdr);
  }
  {
    Elf_Sym *syms, *sym, *imagesym = NULL, *sizesym = NULL;
    unsigned i;
    char *strs;

    syms = holy_malloc (symsize);
    if (!syms)
      return holy_errno;

    if (holy_file_seek (file, symoff) == (holy_off_t) -1)
      {
	holy_free (syms);
	return holy_errno;
      }
    if (holy_file_read (file, syms, symsize) != (holy_ssize_t) symsize)
      {
	holy_free (syms);
	if (! holy_errno)
	  return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			     filename);
	return holy_errno;
      }

    strs = holy_malloc (strsize);
    if (!strs)
      {
	holy_free (syms);
	return holy_errno;
      }

    if (holy_file_seek (file, stroff) == (holy_off_t) -1)
      {
	holy_free (syms);
	holy_free (strs);
	return holy_errno;
      }
    if (holy_file_read (file, strs, strsize) != (holy_ssize_t) strsize)
      {
	holy_free (syms);
	holy_free (strs);
	if (! holy_errno)
	  return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			     filename);
	return holy_errno;
      }

    for (i = 0, sym = syms; i * symentsize < symsize;
       i++, sym = (Elf_Sym *) ((char *) sym + symentsize))
      {
	if (ELF_ST_TYPE (sym->st_info) != STT_OBJECT)
	  continue;
	if (!sym->st_name)
	  continue;
	if (holy_strcmp (strs + sym->st_name, "rd_root_image") == 0)
	  imagesym = sym;
	if (holy_strcmp (strs + sym->st_name, "rd_root_size") == 0)
	  sizesym = sym;
	if (imagesym && sizesym)
	  break;
      }
    if (!imagesym || !sizesym)
      {
	holy_free (syms);
	holy_free (strs);
	return holy_ERR_NONE;
      }
    if (sizeof (*desc->size) != sizesym->st_size)
      {
	holy_free (syms);
	holy_free (strs);
	return holy_error (holy_ERR_BAD_OS, "unexpected size of rd_root_size");
      }
    desc->max_size = imagesym->st_size;
    desc->target = (imagesym->st_value & 0xFFFFFF) - kern_start
      + (holy_uint8_t *) kern_chunk_src;
    desc->size = (holy_uint32_t *) ((sizesym->st_value & 0xFFFFFF) - kern_start
				    + (holy_uint8_t *) kern_chunk_src);
    holy_free (syms);
    holy_free (strs);

    return holy_ERR_NONE;
  }
}
