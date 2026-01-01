/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_TARGET_WORDSIZE (8 * holy_CPU_SIZEOF_VOID_P)

#include <config.h>
#include <holy/elf.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/types.h>
#include <holy/symbol.h>
#include <holy/file.h>
#include <holy/env.h>
#include <holy/cache.h>
#include <holy/i18n.h>
#include <holy/tpm.h>

/* Platforms where modules are in a readonly area of memory.  */
#if defined(holy_MACHINE_QEMU)
#define holy_MODULES_MACHINE_READONLY
#endif

#ifdef holy_MACHINE_EMU
#include <sys/mman.h>
#endif

#ifdef holy_MACHINE_EFI
#include <holy/efi/efi.h>
#endif



#pragma GCC diagnostic ignored "-Wcast-align"

holy_dl_t holy_dl_head = 0;

holy_err_t
holy_dl_add (holy_dl_t mod);

/* Keep global so that GDB scripts work.  */
holy_err_t
holy_dl_add (holy_dl_t mod)
{
  if (holy_dl_get (mod->name))
    return holy_error (holy_ERR_BAD_MODULE,
		       "`%s' is already loaded", mod->name);

  return holy_ERR_NONE;
}

static void
holy_dl_remove (holy_dl_t mod)
{
  holy_dl_t *p, q;

  for (p = &holy_dl_head, q = *p; q; p = &q->next, q = *p)
    if (q == mod)
      {
	*p = q->next;
	return;
      }
}



struct holy_symbol
{
  struct holy_symbol *next;
  const char *name;
  void *addr;
  int isfunc;
  holy_dl_t mod;	/* The module to which this symbol belongs.  */
};
typedef struct holy_symbol *holy_symbol_t;

/* The size of the symbol table.  */
#define holy_SYMTAB_SIZE	509

/* The symbol table (using an open-hash).  */
static struct holy_symbol *holy_symtab[holy_SYMTAB_SIZE];

/* Simple hash function.  */
static unsigned
holy_symbol_hash (const char *s)
{
  unsigned key = 0;

  while (*s)
    key = key * 65599 + *s++;

  return (key + (key >> 5)) % holy_SYMTAB_SIZE;
}

/* Resolve the symbol name NAME and return the address.
   Return NULL, if not found.  */
static holy_symbol_t
holy_dl_resolve_symbol (const char *name)
{
  holy_symbol_t sym;

  for (sym = holy_symtab[holy_symbol_hash (name)]; sym; sym = sym->next)
    if (holy_strcmp (sym->name, name) == 0)
      return sym;

  return 0;
}

/* Register a symbol with the name NAME and the address ADDR.  */
holy_err_t
holy_dl_register_symbol (const char *name, void *addr, int isfunc,
			 holy_dl_t mod)
{
  holy_symbol_t sym;
  unsigned k;

  sym = (holy_symbol_t) holy_malloc (sizeof (*sym));
  if (! sym)
    return holy_errno;

  if (mod)
    {
      sym->name = holy_strdup (name);
      if (! sym->name)
	{
	  holy_free (sym);
	  return holy_errno;
	}
    }
  else
    sym->name = name;

  sym->addr = addr;
  sym->mod = mod;
  sym->isfunc = isfunc;

  k = holy_symbol_hash (name);
  sym->next = holy_symtab[k];
  holy_symtab[k] = sym;

  return holy_ERR_NONE;
}

/* Unregister all the symbols defined in the module MOD.  */
static void
holy_dl_unregister_symbols (holy_dl_t mod)
{
  unsigned i;

  if (! mod)
    holy_fatal ("core symbols cannot be unregistered");

  for (i = 0; i < holy_SYMTAB_SIZE; i++)
    {
      holy_symbol_t sym, *p, q;

      for (p = &holy_symtab[i], sym = *p; sym; sym = q)
	{
	  q = sym->next;
	  if (sym->mod == mod)
	    {
	      *p = q;
	      holy_free ((void *) sym->name);
	      holy_free (sym);
	    }
	  else
	    p = &sym->next;
	}
    }
}

/* Return the address of a section whose index is N.  */
static void *
holy_dl_get_section_addr (holy_dl_t mod, unsigned n)
{
  holy_dl_segment_t seg;

  for (seg = mod->segment; seg; seg = seg->next)
    if (seg->section == n)
      return seg->addr;

  return 0;
}

/* Check if EHDR is a valid ELF header.  */
static holy_err_t
holy_dl_check_header (void *ehdr, holy_size_t size)
{
  Elf_Ehdr *e = ehdr;
  holy_err_t err;

  /* Check the header size.  */
  if (size < sizeof (Elf_Ehdr))
    return holy_error (holy_ERR_BAD_OS, "ELF header smaller than expected");

  /* Check the magic numbers.  */
  if (e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_version != EV_CURRENT)
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-independent ELF magic"));

  err = holy_arch_dl_check_header (ehdr);
  if (err)
    return err;

  return holy_ERR_NONE;
}

/* Load all segments from memory specified by E.  */
static holy_err_t
holy_dl_load_segments (holy_dl_t mod, const Elf_Ehdr *e)
{
  unsigned i;
  const Elf_Shdr *s;
  holy_size_t tsize = 0, talign = 1;
#if !defined (__i386__) && !defined (__x86_64__)
  holy_size_t tramp;
  holy_size_t got;
  holy_err_t err;
#endif
  char *ptr;

  for (i = 0, s = (const Elf_Shdr *)((const char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (const Elf_Shdr *)((const char *) s + e->e_shentsize))
    {
      tsize = ALIGN_UP (tsize, s->sh_addralign) + s->sh_size;
      if (talign < s->sh_addralign)
	talign = s->sh_addralign;
    }

#if !defined (__i386__) && !defined (__x86_64__)
  err = holy_arch_dl_get_tramp_got_size (e, &tramp, &got);
  if (err)
    return err;
  tsize += ALIGN_UP (tramp, holy_ARCH_DL_TRAMP_ALIGN);
  if (talign < holy_ARCH_DL_TRAMP_ALIGN)
    talign = holy_ARCH_DL_TRAMP_ALIGN;
  tsize += ALIGN_UP (got, holy_ARCH_DL_GOT_ALIGN);
  if (talign < holy_ARCH_DL_GOT_ALIGN)
    talign = holy_ARCH_DL_GOT_ALIGN;
#endif

#ifdef holy_MACHINE_EMU
  mod->base = holy_osdep_dl_memalign (talign, tsize);
#else
  mod->base = holy_memalign (talign, tsize);
#endif
  if (!mod->base)
    return holy_errno;
  mod->sz = tsize;
  ptr = mod->base;

  for (i = 0, s = (Elf_Shdr *)((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *)((char *) s + e->e_shentsize))
    {
      if (s->sh_flags & SHF_ALLOC)
	{
	  holy_dl_segment_t seg;

	  seg = (holy_dl_segment_t) holy_malloc (sizeof (*seg));
	  if (! seg)
	    return holy_errno;

	  if (s->sh_size)
	    {
	      void *addr;

	      ptr = (char *) ALIGN_UP ((holy_addr_t) ptr, s->sh_addralign);
	      addr = ptr;
	      ptr += s->sh_size;

	      switch (s->sh_type)
		{
		case SHT_PROGBITS:
		  holy_memcpy (addr, (char *) e + s->sh_offset, s->sh_size);
		  break;
		case SHT_NOBITS:
		  holy_memset (addr, 0, s->sh_size);
		  break;
		}

	      seg->addr = addr;
	    }
	  else
	    seg->addr = 0;

	  seg->size = s->sh_size;
	  seg->section = i;
	  seg->next = mod->segment;
	  mod->segment = seg;
	}
    }
#if !defined (__i386__) && !defined (__x86_64__)
  ptr = (char *) ALIGN_UP ((holy_addr_t) ptr, holy_ARCH_DL_TRAMP_ALIGN);
  mod->tramp = ptr;
  mod->trampptr = ptr;
  ptr += tramp;
  ptr = (char *) ALIGN_UP ((holy_addr_t) ptr, holy_ARCH_DL_GOT_ALIGN);
  mod->got = ptr;
  mod->gotptr = ptr;
  ptr += got;
#endif

  return holy_ERR_NONE;
}

static holy_err_t
holy_dl_resolve_symbols (holy_dl_t mod, Elf_Ehdr *e)
{
  unsigned i;
  Elf_Shdr *s;
  Elf_Sym *sym;
  const char *str;
  Elf_Word size, entsize;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_SYMTAB)
      break;

  /* Module without symbol table may still be used to pull in dependencies.
     We verify at build time that such modules do not contain any relocations
     that may reference symbol table. */
  if (i == e->e_shnum)
    return holy_ERR_NONE;

#ifdef holy_MODULES_MACHINE_READONLY
  mod->symtab = holy_malloc (s->sh_size);
  if (!mod->symtab)
    return holy_errno;
  holy_memcpy (mod->symtab, (char *) e + s->sh_offset, s->sh_size);
#else
  mod->symtab = (Elf_Sym *) ((char *) e + s->sh_offset);
#endif
  mod->symsize = s->sh_entsize;
  sym = mod->symtab;
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
      const char *name = str + sym->st_name;

      switch (type)
	{
	case STT_NOTYPE:
	case STT_OBJECT:
	  /* Resolve a global symbol.  */
	  if (sym->st_name != 0 && sym->st_shndx == 0)
	    {
	      holy_symbol_t nsym = holy_dl_resolve_symbol (name);
	      if (! nsym)
		return holy_error (holy_ERR_BAD_MODULE,
				   N_("symbol `%s' not found"), name);
	      sym->st_value = (Elf_Addr) nsym->addr;
	      if (nsym->isfunc)
		sym->st_info = ELF_ST_INFO (bind, STT_FUNC);
	    }
	  else
	    {
	      sym->st_value += (Elf_Addr) holy_dl_get_section_addr (mod,
								    sym->st_shndx);
	      if (bind != STB_LOCAL)
		if (holy_dl_register_symbol (name, (void *) sym->st_value, 0, mod))
		  return holy_errno;
	    }
	  break;

	case STT_FUNC:
	  sym->st_value += (Elf_Addr) holy_dl_get_section_addr (mod,
								sym->st_shndx);
#ifdef __ia64__
	  {
	      /* FIXME: free descriptor once it's not used anymore. */
	      char **desc;
	      desc = holy_malloc (2 * sizeof (char *));
	      if (!desc)
		return holy_errno;
	      desc[0] = (void *) sym->st_value;
	      desc[1] = mod->base;
	      sym->st_value = (holy_addr_t) desc;
	  }
#endif
	  if (bind != STB_LOCAL)
	    if (holy_dl_register_symbol (name, (void *) sym->st_value, 1, mod))
	      return holy_errno;
	  if (holy_strcmp (name, "holy_mod_init") == 0)
	    mod->init = (void (*) (holy_dl_t)) sym->st_value;
	  else if (holy_strcmp (name, "holy_mod_fini") == 0)
	    mod->fini = (void (*) (void)) sym->st_value;
	  break;

	case STT_SECTION:
	  sym->st_value = (Elf_Addr) holy_dl_get_section_addr (mod,
							       sym->st_shndx);
	  break;

	case STT_FILE:
	  sym->st_value = 0;
	  break;

	default:
	  return holy_error (holy_ERR_BAD_MODULE,
			     "unknown symbol type `%d'", (int) type);
	}
    }

  return holy_ERR_NONE;
}

static Elf_Shdr *
holy_dl_find_section (Elf_Ehdr *e, const char *name)
{
  Elf_Shdr *s;
  const char *str;
  unsigned i;

  s = (Elf_Shdr *) ((char *) e + e->e_shoff + e->e_shstrndx * e->e_shentsize);
  str = (char *) e + s->sh_offset;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (holy_strcmp (str + s->sh_name, name) == 0)
      return s;
  return NULL;
}

/* Me, Vladimir Serbinenko, hereby I add this module check as per new
   GNU module policy. Note that this license check is informative only.
   Modules have to be licensed under GPLv2 or GPLv2+ (optionally
   multi-licensed under other licences as well) independently of the
   presence of this check and solely by linking (module loading in holy
   constitutes linking) and holy core being licensed under GPLv2+.
   Be sure to understand your license obligations.
*/
static holy_err_t
holy_dl_check_license (Elf_Ehdr *e)
{
  Elf_Shdr *s = holy_dl_find_section (e, ".module_license");
  if (s && (holy_strcmp ((char *) e + s->sh_offset, "LICENSE=GPLv2") == 0
	    || holy_strcmp ((char *) e + s->sh_offset, "LICENSE=GPLv2+") == 0
	    || holy_strcmp ((char *) e + s->sh_offset, "LICENSE=GPLv2+") == 0))
    return holy_ERR_NONE;
  return holy_error (holy_ERR_BAD_MODULE, "incompatible license");
}

static holy_err_t
holy_dl_resolve_name (holy_dl_t mod, Elf_Ehdr *e)
{
  Elf_Shdr *s;

  s = holy_dl_find_section (e, ".modname");
  if (!s)
    return holy_error (holy_ERR_BAD_MODULE, "no module name found");
  
  mod->name = holy_strdup ((char *) e + s->sh_offset);
  if (! mod->name)
    return holy_errno;

  return holy_ERR_NONE;
}

static holy_err_t
holy_dl_resolve_dependencies (holy_dl_t mod, Elf_Ehdr *e)
{
  Elf_Shdr *s;

  s = holy_dl_find_section (e, ".moddeps");

  if (!s)
    return holy_ERR_NONE;

  const char *name = (char *) e + s->sh_offset;
  const char *max = name + s->sh_size;

  while ((name < max) && (*name))
    {
      holy_dl_t m;
      holy_dl_dep_t dep;

      m = holy_dl_load (name);
      if (! m)
	return holy_errno;

      holy_dl_ref (m);

      dep = (holy_dl_dep_t) holy_malloc (sizeof (*dep));
      if (! dep)
	return holy_errno;

      dep->mod = m;
      dep->next = mod->dep;
      mod->dep = dep;

      name += holy_strlen (name) + 1;
    }

  return holy_ERR_NONE;
}

int
holy_dl_ref (holy_dl_t mod)
{
  holy_dl_dep_t dep;

  if (!mod)
    return 0;

  for (dep = mod->dep; dep; dep = dep->next)
    holy_dl_ref (dep->mod);

  return ++mod->ref_count;
}

int
holy_dl_unref (holy_dl_t mod)
{
  holy_dl_dep_t dep;

  if (!mod)
    return 0;

  for (dep = mod->dep; dep; dep = dep->next)
    holy_dl_unref (dep->mod);

  return --mod->ref_count;
}

static void
holy_dl_flush_cache (holy_dl_t mod)
{
  holy_dprintf ("modules", "flushing 0x%lx bytes at %p\n",
		(unsigned long) mod->sz, mod->base);
  holy_arch_sync_caches (mod->base, mod->sz);
}

static holy_err_t
holy_dl_relocate_symbols (holy_dl_t mod, void *ehdr)
{
  Elf_Ehdr *e = ehdr;
  Elf_Shdr *s;
  unsigned i;

  for (i = 0, s = (Elf_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_REL || s->sh_type == SHT_RELA)
      {
	holy_dl_segment_t seg;
	holy_err_t err;

	/* Find the target segment.  */
	for (seg = mod->segment; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    if (!mod->symtab)
	      return holy_error (holy_ERR_BAD_MODULE, "relocation without symbol table");

	    err = holy_arch_dl_relocate_symbols (mod, ehdr, s, seg);
	    if (err)
	      return err;
	  }
      }

  return holy_ERR_NONE;
}

/* Load a module from core memory.  */
holy_dl_t
holy_dl_load_core_noinit (void *addr, holy_size_t size)
{
  Elf_Ehdr *e;
  holy_dl_t mod;

  holy_dprintf ("modules", "module at %p, size 0x%lx\n", addr,
		(unsigned long) size);
  e = addr;
  if (holy_dl_check_header (e, size))
    return 0;

  if (e->e_type != ET_REL)
    {
      holy_error (holy_ERR_BAD_MODULE, N_("this ELF file is not of the right type"));
      return 0;
    }

  /* Make sure that every section is within the core.  */
  if (size < e->e_shoff + (holy_uint32_t) e->e_shentsize * e->e_shnum)
    {
      holy_error (holy_ERR_BAD_OS, "ELF sections outside core");
      return 0;
    }

  mod = (holy_dl_t) holy_zalloc (sizeof (*mod));
  if (! mod)
    return 0;

  mod->ref_count = 1;

  holy_dprintf ("modules", "relocating to %p\n", mod);
  /* Me, Vladimir Serbinenko, hereby I add this module check as per new
     GNU module policy. Note that this license check is informative only.
     Modules have to be licensed under GPLv2 or GPLv2+ (optionally
     multi-licensed under other licences as well) independently of the
     presence of this check and solely by linking (module loading in holy
     constitutes linking) and holy core being licensed under GPLv2+.
     Be sure to understand your license obligations.
  */
  if (holy_dl_check_license (e)
      || holy_dl_resolve_name (mod, e)
      || holy_dl_resolve_dependencies (mod, e)
      || holy_dl_load_segments (mod, e)
      || holy_dl_resolve_symbols (mod, e)
      || holy_dl_relocate_symbols (mod, e))
    {
      mod->fini = 0;
      holy_dl_unload (mod);
      return 0;
    }

  holy_dl_flush_cache (mod);

  holy_dprintf ("modules", "module name: %s\n", mod->name);
  holy_dprintf ("modules", "init function: %p\n", mod->init);

  if (holy_dl_add (mod))
    {
      holy_dl_unload (mod);
      return 0;
    }

  return mod;
}

holy_dl_t
holy_dl_load_core (void *addr, holy_size_t size)
{
  holy_dl_t mod;

  holy_boot_time ("Parsing module");

  mod = holy_dl_load_core_noinit (addr, size);

  if (!mod)
    return NULL;

  holy_boot_time ("Initing module %s", mod->name);
  holy_dl_init (mod);
  holy_boot_time ("Module %s inited", mod->name);

  return mod;
}

/* Load a module from the file FILENAME.  */
holy_dl_t
holy_dl_load_file (const char *filename)
{
  holy_file_t file = NULL;
  holy_ssize_t size;
  void *core = 0;
  holy_dl_t mod = 0;

#ifdef holy_MACHINE_EFI
  if (holy_efi_secure_boot ())
    {
      holy_error (holy_ERR_ACCESS_DENIED,
		  "Secure Boot forbids loading module from %s", filename);
      return 0;
    }
#endif

  holy_boot_time ("Loading module %s", filename);

  file = holy_file_open (filename);
  if (! file)
    return 0;

  size = holy_file_size (file);
  core = holy_malloc (size);
  if (! core)
    {
      holy_file_close (file);
      return 0;
    }

  if (holy_file_read (file, core, size) != (int) size)
    {
      holy_file_close (file);
      holy_free (core);
      return 0;
    }

  /* We must close this before we try to process dependencies.
     Some disk backends do not handle gracefully multiple concurrent
     opens of the same device.  */
  holy_file_close (file);

  holy_tpm_measure(core, size, holy_BINARY_PCR, "holy_module", filename);
  holy_print_error();

  mod = holy_dl_load_core (core, size);
  holy_free (core);
  if (! mod)
    return 0;

  mod->ref_count--;
  return mod;
}

/* Load a module using a symbolic name.  */
holy_dl_t
holy_dl_load (const char *name)
{
  char *filename;
  holy_dl_t mod;
  const char *holy_dl_dir = holy_env_get ("prefix");

  mod = holy_dl_get (name);
  if (mod)
    return mod;

  if (holy_no_modules)
    return 0;

  if (! holy_dl_dir) {
    holy_error (holy_ERR_FILE_NOT_FOUND, N_("variable `%s' isn't set"), "prefix");
    return 0;
  }

  filename = holy_xasprintf ("%s/" holy_TARGET_CPU "-" holy_PLATFORM "/%s.mod",
			     holy_dl_dir, name);
  if (! filename)
    return 0;

  mod = holy_dl_load_file (filename);
  holy_free (filename);

  if (! mod)
    return 0;

  if (holy_strcmp (mod->name, name) != 0)
    holy_error (holy_ERR_BAD_MODULE, "mismatched names");

  return mod;
}

/* Unload the module MOD.  */
int
holy_dl_unload (holy_dl_t mod)
{
  holy_dl_dep_t dep, depn;

  if (mod->ref_count > 0)
    return 0;

  if (mod->fini)
    (mod->fini) ();

  holy_dl_remove (mod);
  holy_dl_unregister_symbols (mod);

  for (dep = mod->dep; dep; dep = depn)
    {
      depn = dep->next;

      holy_dl_unload (dep->mod);

      holy_free (dep);
    }

#ifdef holy_MACHINE_EMU
  holy_dl_osdep_dl_free (mod->base);
#else
  holy_free (mod->base);
#endif
  holy_free (mod->name);
#ifdef holy_MODULES_MACHINE_READONLY
  holy_free (mod->symtab);
#endif
  holy_free (mod);
  return 1;
}

/* Unload unneeded modules.  */
void
holy_dl_unload_unneeded (void)
{
  /* Because holy_dl_remove modifies the list of modules, this
     implementation is tricky.  */
  holy_dl_t p = holy_dl_head;

  while (p)
    {
      if (holy_dl_unload (p))
	{
	  p = holy_dl_head;
	  continue;
	}

      p = p->next;
    }
}
