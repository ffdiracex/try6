/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#if defined(MULTIBOOT_LOAD_ELF32)
# define XX		32
# define E_MACHINE	MULTIBOOT_ELF32_MACHINE
# define ELFCLASSXX	ELFCLASS32
# define Elf_Ehdr	Elf32_Ehdr
# define Elf_Phdr	Elf32_Phdr
# define Elf_Shdr	Elf32_Shdr
#elif defined(MULTIBOOT_LOAD_ELF64)
# define XX		64
# define E_MACHINE	MULTIBOOT_ELF64_MACHINE
# define ELFCLASSXX	ELFCLASS64
# define Elf_Ehdr	Elf64_Ehdr
# define Elf_Phdr	Elf64_Phdr
# define Elf_Shdr	Elf64_Shdr
#else
#error "I'm confused"
#endif

#include <holy/i386/relocator.h>

#define CONCAT(a,b)	CONCAT_(a, b)
#define CONCAT_(a,b)	a ## b

#pragma GCC diagnostic ignored "-Wcast-align"

/* Check if BUFFER contains ELF32 (or ELF64).  */
static int
CONCAT(holy_multiboot_is_elf, XX) (void *buffer)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) buffer;

  return ehdr->e_ident[EI_CLASS] == ELFCLASSXX;
}

static holy_err_t
CONCAT(holy_multiboot_load_elf, XX) (mbi_load_data_t *mld)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) mld->buffer;
  char *phdr_base;
  holy_err_t err;
  holy_relocator_chunk_t ch;
  holy_uint32_t load_offset, load_size;
  int i;
  void *source;

  if (ehdr->e_ident[EI_MAG0] != ELFMAG0
      || ehdr->e_ident[EI_MAG1] != ELFMAG1
      || ehdr->e_ident[EI_MAG2] != ELFMAG2
      || ehdr->e_ident[EI_MAG3] != ELFMAG3
      || ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    return holy_error(holy_ERR_UNKNOWN_OS, N_("invalid arch-independent ELF magic"));

  if (ehdr->e_ident[EI_CLASS] != ELFCLASSXX || ehdr->e_machine != E_MACHINE
      || ehdr->e_version != EV_CURRENT)
    return holy_error (holy_ERR_UNKNOWN_OS, N_("invalid arch-dependent ELF magic"));

  if (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN)
    return holy_error (holy_ERR_UNKNOWN_OS, N_("this ELF file is not of the right type"));

  /* FIXME: Should we support program headers at strange locations?  */
  if (ehdr->e_phoff + (holy_uint32_t) ehdr->e_phnum * ehdr->e_phentsize > MULTIBOOT_SEARCH)
    return holy_error (holy_ERR_BAD_OS, "program header at a too high offset");

  phdr_base = (char *) mld->buffer + ehdr->e_phoff;
#define phdr(i)			((Elf_Phdr *) (phdr_base + (i) * ehdr->e_phentsize))

  mld->link_base_addr = ~0;

  /* Calculate lowest and highest load address.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    if (phdr(i)->p_type == PT_LOAD)
      {
	mld->link_base_addr = holy_min (mld->link_base_addr, phdr(i)->p_paddr);
	highest_load = holy_max (highest_load, phdr(i)->p_paddr + phdr(i)->p_memsz);
      }

#ifdef MULTIBOOT_LOAD_ELF64
  if (highest_load >= 0x100000000)
    return holy_error (holy_ERR_BAD_OS, "segment crosses 4 GiB border");
#endif

  load_size = highest_load - mld->link_base_addr;

  if (mld->relocatable)
    {
      if (load_size > mld->max_addr || mld->min_addr > mld->max_addr - load_size)
	return holy_error (holy_ERR_BAD_OS, "invalid min/max address and/or load size");

      err = holy_relocator_alloc_chunk_align (holy_multiboot_relocator, &ch,
					      mld->min_addr, mld->max_addr - load_size,
					      load_size, mld->align ? mld->align : 1,
					      mld->preference, mld->avoid_efi_boot_services);
    }
  else
    err = holy_relocator_alloc_chunk_addr (holy_multiboot_relocator, &ch,
					   mld->link_base_addr, load_size);

  if (err)
    {
      holy_dprintf ("multiboot_loader", "Cannot allocate memory for OS image\n");
      return err;
    }

  mld->load_base_addr = get_physical_target_address (ch);
  source = get_virtual_current_address (ch);

  holy_dprintf ("multiboot_loader", "link_base_addr=0x%x, load_base_addr=0x%x, "
		"load_size=0x%x, relocatable=%d\n", mld->link_base_addr,
		mld->load_base_addr, load_size, mld->relocatable);

  if (mld->relocatable)
    holy_dprintf ("multiboot_loader", "align=0x%lx, preference=0x%x, avoid_efi_boot_services=%d\n",
		  (long) mld->align, mld->preference, mld->avoid_efi_boot_services);

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      if (phdr(i)->p_type == PT_LOAD)
        {

	  holy_dprintf ("multiboot_loader", "segment %d: paddr=0x%lx, memsz=0x%lx, vaddr=0x%lx\n",
			i, (long) phdr(i)->p_paddr, (long) phdr(i)->p_memsz, (long) phdr(i)->p_vaddr);

	  load_offset = phdr(i)->p_paddr - mld->link_base_addr;

	  if (phdr(i)->p_filesz != 0)
	    {
	      if (holy_file_seek (mld->file, (holy_off_t) phdr(i)->p_offset)
		  == (holy_off_t) -1)
		return holy_errno;

	      if (holy_file_read (mld->file, (holy_uint8_t *) source + load_offset, phdr(i)->p_filesz)
		  != (holy_ssize_t) phdr(i)->p_filesz)
		{
		  if (!holy_errno)
		    holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
				mld->filename);
		  return holy_errno;
		}
	    }

          if (phdr(i)->p_filesz < phdr(i)->p_memsz)
            holy_memset ((holy_uint8_t *) source + load_offset + phdr(i)->p_filesz, 0,
			 phdr(i)->p_memsz - phdr(i)->p_filesz);
        }
    }

  for (i = 0; i < ehdr->e_phnum; i++)
    if (phdr(i)->p_vaddr <= ehdr->e_entry
	&& phdr(i)->p_vaddr + phdr(i)->p_memsz > ehdr->e_entry)
      {
	holy_multiboot_payload_eip = (ehdr->e_entry - phdr(i)->p_vaddr)
	  + phdr(i)->p_paddr;
#ifdef MULTIBOOT_LOAD_ELF64
# ifdef __mips
  /* We still in 32-bit mode.  */
  if ((ehdr->e_entry - phdr(i)->p_vaddr)
      + phdr(i)->p_paddr < 0xffffffff80000000ULL)
    return holy_error (holy_ERR_BAD_OS, "invalid entry point for ELF64");
# else
  /* We still in 32-bit mode.  */
  if ((ehdr->e_entry - phdr(i)->p_vaddr)
      + phdr(i)->p_paddr > 0xffffffff)
    return holy_error (holy_ERR_BAD_OS, "invalid entry point for ELF64");
# endif
#endif
	break;
      }

  if (i == ehdr->e_phnum)
    return holy_error (holy_ERR_BAD_OS, "entry point isn't in a segment");

#if defined (__i386__) || defined (__x86_64__)
  
#elif defined (__mips)
  holy_multiboot_payload_eip |= 0x80000000;
#else
#error Please complete this
#endif

  if (ehdr->e_shnum)
    {
      holy_uint8_t *shdr, *shdrptr;

      shdr = holy_malloc ((holy_uint32_t) ehdr->e_shnum * ehdr->e_shentsize);
      if (!shdr)
	return holy_errno;
      
      if (holy_file_seek (mld->file, ehdr->e_shoff) == (holy_off_t) -1)
	{
	  holy_free (shdr);
	  return holy_errno;
	}

      if (holy_file_read (mld->file, shdr, (holy_uint32_t) ehdr->e_shnum * ehdr->e_shentsize)
              != (holy_ssize_t) ehdr->e_shnum * ehdr->e_shentsize)
	{
	  if (!holy_errno)
	    holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			mld->filename);
	  return holy_errno;
	}
      
      for (shdrptr = shdr, i = 0; i < ehdr->e_shnum;
	   shdrptr += ehdr->e_shentsize, i++)
	{
	  Elf_Shdr *sh = (Elf_Shdr *) shdrptr;
	  void *src;
	  holy_addr_t target;

	  if (mld->mbi_ver >= 2 && (sh->sh_type == SHT_REL || sh->sh_type == SHT_RELA))
	    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET, "ELF files with relocs are not supported yet");

	  /* This section is a loaded section,
	     so we don't care.  */
	  if (sh->sh_addr != 0)
	    continue;
		      
	  /* This section is empty, so we don't care.  */
	  if (sh->sh_size == 0)
	    continue;

	  err = holy_relocator_alloc_chunk_align (holy_multiboot_relocator, &ch, 0,
						  (0xffffffff - sh->sh_size) + 1,
						  sh->sh_size, sh->sh_addralign,
						  holy_RELOCATOR_PREFERENCE_NONE,
						  mld->avoid_efi_boot_services);
	  if (err)
	    {
	      holy_dprintf ("multiboot_loader", "Error loading shdr %d\n", i);
	      return err;
	    }
	  src = get_virtual_current_address (ch);
	  target = get_physical_target_address (ch);

	  if (holy_file_seek (mld->file, sh->sh_offset) == (holy_off_t) -1)
	    return holy_errno;

          if (holy_file_read (mld->file, src, sh->sh_size)
              != (holy_ssize_t) sh->sh_size)
	    {
	      if (!holy_errno)
		holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			    mld->filename);
	      return holy_errno;
	    }
	  sh->sh_addr = target;
	}
      holy_multiboot_add_elfsyms (ehdr->e_shnum, ehdr->e_shentsize,
				  ehdr->e_shstrndx, shdr);
    }

#undef phdr

  return holy_errno;
}

#undef XX
#undef E_MACHINE
#undef ELFCLASSXX
#undef Elf_Ehdr
#undef Elf_Phdr
#undef Elf_Shdr
