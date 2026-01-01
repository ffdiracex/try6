/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/efiemu/efiemu.h>
#include <holy/cpu/efiemu.h>
#include <holy/elf.h>
#include <holy/i18n.h>

/* ELF symbols and their values */
static struct holy_efiemu_elf_sym *holy_efiemu_elfsyms = 0;
static int holy_efiemu_nelfsyms = 0;

/* Return the address of a section whose index is N.  */
static holy_err_t
holy_efiemu_get_section_addr (holy_efiemu_segment_t segs, unsigned n,
			      int *handle, holy_off_t *off)
{
  holy_efiemu_segment_t seg;

  for (seg = segs; seg; seg = seg->next)
    if (seg->section == n)
      {
	*handle = seg->handle;
	*off = seg->off;
	return holy_ERR_NONE;
      }

  return holy_error (holy_ERR_BAD_OS, "section %d not found", n);
}

holy_err_t
SUFFIX (holy_efiemu_loadcore_unload) (void)
{
  holy_free (holy_efiemu_elfsyms);
  holy_efiemu_elfsyms = 0;
  return holy_ERR_NONE;
}

/* Check if EHDR is a valid ELF header.  */
int
SUFFIX (holy_efiemu_check_header) (void *ehdr, holy_size_t size)
{
  Elf_Ehdr *e = ehdr;

  /* Check the header size.  */
  if (size < sizeof (Elf_Ehdr))
    return 0;

  /* Check the magic numbers.  */
  if (!SUFFIX (holy_arch_efiemu_check_header) (ehdr)
      || e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_version != EV_CURRENT)
    return 0;

  return 1;
}

/* Load all segments from memory specified by E.  */
static holy_err_t
holy_efiemu_load_segments (holy_efiemu_segment_t segs, const Elf_Ehdr *e)
{
  Elf_Shdr *s;
  holy_efiemu_segment_t cur;

  holy_dprintf ("efiemu", "loading segments\n");

  for (cur=segs; cur; cur = cur->next)
    {
      s = (Elf_Shdr *)cur->srcptr;

      if ((s->sh_flags & SHF_ALLOC) && s->sh_size)
	{
	  void *addr;

	  addr = (holy_uint8_t *) holy_efiemu_mm_obtain_request (cur->handle)
	    + cur->off;

	  switch (s->sh_type)
	    {
	    case SHT_PROGBITS:
	      holy_memcpy (addr, (char *) e + s->sh_offset, s->sh_size);
	      break;
	    case SHT_NOBITS:
	      holy_memset (addr, 0, s->sh_size);
	      break;
	    }
	}
    }

  return holy_ERR_NONE;
}

/* Get a string at offset OFFSET from strtab */
static char *
holy_efiemu_get_string (unsigned offset, const Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_STRTAB && offset < s->sh_size)
      return (char *) e + s->sh_offset + offset;
  return 0;
}

/* Request memory for segments and fill segments info */
static holy_err_t
holy_efiemu_init_segments (holy_efiemu_segment_t *segs, const Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    {
      if (s->sh_flags & SHF_ALLOC)
	{
	  holy_efiemu_segment_t seg;
	  seg = (holy_efiemu_segment_t) holy_malloc (sizeof (*seg));
	  if (! seg)
	    return holy_errno;

	  if (s->sh_size)
	    {
	      seg->handle
		= holy_efiemu_request_memalign
		(s->sh_addralign, s->sh_size,
		 s->sh_flags & SHF_EXECINSTR ? holy_EFI_RUNTIME_SERVICES_CODE
		 : holy_EFI_RUNTIME_SERVICES_DATA);
	      if (seg->handle < 0)
		{
		  holy_free (seg);
		  return holy_errno;
		}
	      seg->off = 0;
	    }

	  /*
	     .text-physical doesn't need to be relocated when switching to
	     virtual mode
	   */
	  if (!holy_strcmp (holy_efiemu_get_string (s->sh_name, e),
			    ".text-physical"))
	    seg->ptv_rel_needed = 0;
	  else
	    seg->ptv_rel_needed = 1;
	  seg->size = s->sh_size;
	  seg->section = i;
	  seg->next = *segs;
	  seg->srcptr = s;
	  *segs = seg;
	}
    }

  return holy_ERR_NONE;
}

/* Count symbols and relocators and allocate/request memory for them */
static holy_err_t
holy_efiemu_count_symbols (const Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;
  int num = 0;

  /* Symbols */
  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return holy_error (holy_ERR_BAD_OS, N_("no symbol table"));

  holy_efiemu_nelfsyms = (unsigned) s->sh_size / (unsigned) s->sh_entsize;
  holy_efiemu_elfsyms = (struct holy_efiemu_elf_sym *)
    holy_malloc (sizeof (struct holy_efiemu_elf_sym) * holy_efiemu_nelfsyms);

  /* Relocators */
  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_REL || s->sh_type == SHT_RELA)
      num += ((unsigned) s->sh_size) / ((unsigned) s->sh_entsize);

  holy_efiemu_request_symbols (num);

  return holy_ERR_NONE;
}

/* Fill holy_efiemu_elfsyms with symbol values */
static holy_err_t
holy_efiemu_resolve_symbols (holy_efiemu_segment_t segs, Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;
  Elf_Sym *sym;
  const char *str;
  Elf_Word size, entsize;

  holy_dprintf ("efiemu", "resolving symbols\n");

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  if (i == e->e_shnum)
    return holy_error (holy_ERR_BAD_OS, N_("no symbol table"));

  sym = (Elf_Sym *) ((char *) e + s->sh_offset);
  size = s->sh_size;
  entsize = s->sh_entsize;

  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shentsize * s->sh_link);
  str = (char *) e + s->sh_offset;

  for (i = 0;
       i < size / entsize;
       i++, sym = (Elf_Sym *) ((char *) sym + entsize))
    {
      unsigned char type = ELF_ST_TYPE (sym->st_info);
      unsigned char bind = ELF_ST_BIND (sym->st_info);
      int handle;
      holy_off_t off;
      holy_err_t err;
      const char *name = str + sym->st_name;
      holy_efiemu_elfsyms[i].section = sym->st_shndx;
      switch (type)
	{
	case STT_NOTYPE:
	  /* Resolve a global symbol.  */
	  if (sym->st_name != 0 && sym->st_shndx == 0)
	    {
	      err = holy_efiemu_resolve_symbol (name, &handle, &off);
	      if (err)
		return err;
	      holy_efiemu_elfsyms[i].handle = handle;
	      holy_efiemu_elfsyms[i].off = off;
	    }
	  else
	    sym->st_value = 0;
	  break;

	case STT_OBJECT:
	  err = holy_efiemu_get_section_addr (segs, sym->st_shndx,
					      &handle, &off);
	  if (err)
	    return err;

	  off += sym->st_value;
	  if (bind != STB_LOCAL)
	    {
	      err = holy_efiemu_register_symbol (name, handle, off);
	      if (err)
		return err;
	    }
	  holy_efiemu_elfsyms[i].handle = handle;
	  holy_efiemu_elfsyms[i].off = off;
	  break;

	case STT_FUNC:
	  err = holy_efiemu_get_section_addr (segs, sym->st_shndx,
					      &handle, &off);
	  if (err)
	    return err;

	  off += sym->st_value;
	  if (bind != STB_LOCAL)
	    {
	      err = holy_efiemu_register_symbol (name, handle, off);
	      if (err)
		return err;
	    }
	  holy_efiemu_elfsyms[i].handle = handle;
	  holy_efiemu_elfsyms[i].off = off;
	  break;

	case STT_SECTION:
	  err = holy_efiemu_get_section_addr (segs, sym->st_shndx,
					      &handle, &off);
	  if (err)
	    {
	      holy_efiemu_elfsyms[i].handle = 0;
	      holy_efiemu_elfsyms[i].off = 0;
	      holy_errno = holy_ERR_NONE;
	      break;
	    }

	  holy_efiemu_elfsyms[i].handle = handle;
	  holy_efiemu_elfsyms[i].off = off;
	  break;

	case STT_FILE:
	  holy_efiemu_elfsyms[i].handle = 0;
	  holy_efiemu_elfsyms[i].off = 0;
	  break;

	default:
	  return holy_error (holy_ERR_BAD_MODULE,
			     "unknown symbol type `%d'", (int) type);
	}
    }

  return holy_ERR_NONE;
}

/* Load runtime to the memory and request memory for definitive location*/
holy_err_t
SUFFIX (holy_efiemu_loadcore_init) (void *core, const char *filename,
				    holy_size_t core_size,
				    holy_efiemu_segment_t *segments)
{
  Elf_Ehdr *e = (Elf_Ehdr *) core;
  holy_err_t err;

  if (e->e_type != ET_REL)
    return holy_error (holy_ERR_BAD_MODULE, N_("this ELF file is not of the right type"));

  /* Make sure that every section is within the core.  */
  if ((holy_size_t) core_size < e->e_shoff + (holy_uint32_t) e->e_shentsize * e->e_shnum)
    return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		       filename);

  err = holy_efiemu_init_segments (segments, core);
  if (err)
    return err;
  err = holy_efiemu_count_symbols (core);
  if (err)
    return err;

  holy_efiemu_request_symbols (1);
  return holy_ERR_NONE;
}

/* Load runtime definitively */
holy_err_t
SUFFIX (holy_efiemu_loadcore_load) (void *core,
				    holy_size_t core_size
				    __attribute__ ((unused)),
				    holy_efiemu_segment_t segments)
{
  holy_err_t err;
  err = holy_efiemu_load_segments (segments, core);
  if (err)
    return err;

  err = holy_efiemu_resolve_symbols (segments, core);
  if (err)
    return err;

  err = SUFFIX (holy_arch_efiemu_relocate_symbols) (segments,
						    holy_efiemu_elfsyms,
						    core);
  if (err)
    return err;

  return holy_ERR_NONE;
}
