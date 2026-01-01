/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/xen_file.h>
#include <holy/misc.h>
#include <xen/elfnote.h>

static holy_err_t
parse_xen_guest (holy_elf_t elf, struct holy_xen_file_info *xi,
		 holy_off_t off, holy_size_t sz)
{
  char *buf;
  char *ptr;
  int has_paddr = 0;

  holy_errno = holy_ERR_NONE;
  if (holy_file_seek (elf->file, off) == (holy_off_t) -1)
    return holy_errno;
  buf = holy_malloc (sz);
  if (!buf)
    return holy_errno;

  if (holy_file_read (elf->file, buf, sz) != (holy_ssize_t) sz)
    {
      if (holy_errno)
	goto out;
      holy_free (buf);
      return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			 elf->file->name);
    }
  xi->has_xen_guest = 1;
  for (ptr = buf; ptr && ptr - buf < (holy_ssize_t) sz;
       ptr = holy_strchr (ptr, ','), (ptr ? ptr++ : 0))
    {
      if (holy_strncmp (ptr, "PAE=no,", sizeof ("PAE=no,") - 1) == 0)
	{
	  if (xi->arch != holy_XEN_FILE_I386
	      && xi->arch != holy_XEN_FILE_I386_PAE
	      && xi->arch != holy_XEN_FILE_I386_PAE_BIMODE)
	    continue;
	  xi->arch = holy_XEN_FILE_I386;
	  continue;
	}

      if (holy_strncmp (ptr, "PAE=yes,", sizeof ("PAE=yes,") - 1) == 0)
	{
	  if (xi->arch != holy_XEN_FILE_I386
	      && xi->arch != holy_XEN_FILE_I386_PAE
	      && xi->arch != holy_XEN_FILE_I386_PAE_BIMODE)
	    continue;
	  xi->arch = holy_XEN_FILE_I386_PAE;
	  continue;
	}

      if (holy_strncmp (ptr, "PAE=yes[extended-cr3],",
			sizeof ("PAE=yes[extended-cr3],") - 1) == 0)
	{
	  if (xi->arch != holy_XEN_FILE_I386
	      && xi->arch != holy_XEN_FILE_I386_PAE
	      && xi->arch != holy_XEN_FILE_I386_PAE_BIMODE)
	    continue;
	  xi->arch = holy_XEN_FILE_I386_PAE;
	  xi->extended_cr3 = 1;
	  continue;
	}

      if (holy_strncmp (ptr, "PAE=bimodal,", sizeof ("PAE=bimodal,") - 1) == 0)
	{
	  if (xi->arch != holy_XEN_FILE_I386
	      && xi->arch != holy_XEN_FILE_I386_PAE
	      && xi->arch != holy_XEN_FILE_I386_PAE_BIMODE)
	    continue;
	  xi->arch = holy_XEN_FILE_I386_PAE_BIMODE;
	  continue;
	}

      if (holy_strncmp (ptr, "PAE=bimodal[extended-cr3],",
			sizeof ("PAE=bimodal[extended-cr3],") - 1) == 0)
	{
	  if (xi->arch != holy_XEN_FILE_I386
	      && xi->arch != holy_XEN_FILE_I386_PAE
	      && xi->arch != holy_XEN_FILE_I386_PAE_BIMODE)
	    continue;
	  xi->arch = holy_XEN_FILE_I386_PAE_BIMODE;
	  xi->extended_cr3 = 1;
	  continue;
	}

      if (holy_strncmp (ptr, "PAE=yes,bimodal,", sizeof ("PAE=yes,bimodal,") - 1) == 0)
	{
	  if (xi->arch != holy_XEN_FILE_I386
	      && xi->arch != holy_XEN_FILE_I386_PAE
	      && xi->arch != holy_XEN_FILE_I386_PAE_BIMODE)
	    continue;
	  xi->arch = holy_XEN_FILE_I386_PAE_BIMODE;
	  continue;
	}

      if (holy_strncmp (ptr, "PAE=yes[extended-cr3],bimodal,",
			sizeof ("PAE=yes[extended-cr3],bimodal,") - 1) == 0)
	{
	  if (xi->arch != holy_XEN_FILE_I386
	      && xi->arch != holy_XEN_FILE_I386_PAE
	      && xi->arch != holy_XEN_FILE_I386_PAE_BIMODE)
	    continue;
	  xi->arch = holy_XEN_FILE_I386_PAE_BIMODE;
	  xi->extended_cr3 = 1;
	  continue;
	}

      if (holy_strncmp (ptr, "VIRT_BASE=", sizeof ("VIRT_BASE=") - 1) == 0)
	{
	  xi->virt_base = holy_strtoull (ptr + sizeof ("VIRT_BASE=") - 1, &ptr, 16);
	  if (holy_errno)
	    goto out;
	  continue;
	}
      if (holy_strncmp (ptr, "VIRT_ENTRY=", sizeof ("VIRT_ENTRY=") - 1) == 0)
	{
	  xi->entry_point = holy_strtoull (ptr + sizeof ("VIRT_ENTRY=") - 1, &ptr, 16);
	  if (holy_errno)
	    goto out;
	  continue;
	}
      if (holy_strncmp (ptr, "HYPERCALL_PAGE=", sizeof ("HYPERCALL_PAGE=") - 1) == 0)
	{
	  xi->hypercall_page = holy_strtoull (ptr + sizeof ("HYPERCALL_PAGE=") - 1, &ptr, 16);
	  xi->has_hypercall_page = 1;
	  if (holy_errno)
	    goto out;
	  continue;
	}
      if (holy_strncmp (ptr, "ELF_PADDR_OFFSET=", sizeof ("ELF_PADDR_OFFSET=") - 1) == 0)
	{
	  xi->paddr_offset = holy_strtoull (ptr + sizeof ("ELF_PADDR_OFFSET=") - 1, &ptr, 16);
	  has_paddr = 1;
	  if (holy_errno)
	    goto out;
	  continue;
	}
    }
  if (xi->has_hypercall_page)
    xi->hypercall_page = (xi->hypercall_page << 12) + xi->virt_base;
  if (!has_paddr)
    xi->paddr_offset = xi->virt_base;

out:
  holy_free (buf);

  return holy_errno;
}

#pragma GCC diagnostic ignored "-Wcast-align"

static holy_err_t
parse_note (holy_elf_t elf, struct holy_xen_file_info *xi,
	    holy_off_t off, holy_size_t sz)
{
  holy_uint32_t *buf;
  holy_uint32_t *ptr;
  if (holy_file_seek (elf->file, off) == (holy_off_t) -1)
    return holy_errno;
  buf = holy_malloc (sz);
  if (!buf)
    return holy_errno;

  if (holy_file_read (elf->file, buf, sz) != (holy_ssize_t) sz)
    {
      if (holy_errno)
	return holy_errno;
      return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			 elf->file->name);
    }
  for (ptr = buf; ptr - buf < (holy_ssize_t) (sz / sizeof (holy_uint32_t));)
    {
      Elf_Nhdr *nh = (Elf_Nhdr *) ptr;
      char *name;
      holy_uint32_t *desc;
      holy_uint32_t namesz, descsz;
      ptr += sizeof (*nh) / sizeof (holy_uint32_t);
      name = (char *) ptr;
      namesz = holy_le_to_cpu32 (nh->n_namesz);
      descsz = holy_le_to_cpu32 (nh->n_descsz);
      ptr += (namesz + 3) / 4;
      desc = ptr;
      ptr += (holy_le_to_cpu32 (nh->n_descsz) + 3) / 4;
      if ((namesz < 3) || holy_memcmp (name, "Xen", namesz == 3 ? 3 : 4) != 0)
	continue;
      xi->has_note = 1;
      switch (nh->n_type)
	{
	case XEN_ELFNOTE_ENTRY:
	  xi->entry_point = holy_le_to_cpu_addr (*(Elf_Addr *) desc);
	  break;
	case XEN_ELFNOTE_HYPERCALL_PAGE:
	  xi->hypercall_page = holy_le_to_cpu_addr (*(Elf_Addr *) desc);
	  xi->has_hypercall_page = 1;
	  break;
	case XEN_ELFNOTE_VIRT_BASE:
	  xi->virt_base = holy_le_to_cpu_addr (*(Elf_Addr *) desc);
	  break;
	case XEN_ELFNOTE_PADDR_OFFSET:
	  xi->paddr_offset = holy_le_to_cpu_addr (*(Elf_Addr *) desc);
	  break;
	case XEN_ELFNOTE_XEN_VERSION:
	  holy_dprintf ("xen", "xenversion = `%s'\n", (char *) desc);
	  break;
	case XEN_ELFNOTE_GUEST_OS:
	  holy_dprintf ("xen", "name = `%s'\n", (char *) desc);
	  break;
	case XEN_ELFNOTE_GUEST_VERSION:
	  holy_dprintf ("xen", "version = `%s'\n", (char *) desc);
	  break;
	case XEN_ELFNOTE_LOADER:
	  if (descsz < 7
	      || holy_memcmp (desc, "generic", descsz == 7 ? 7 : 8) != 0)
	    return holy_error (holy_ERR_BAD_OS, "invalid loader");
	  break;
	  /* PAE */
	case XEN_ELFNOTE_PAE_MODE:
	  holy_dprintf ("xen", "pae = `%s', %d, %d\n", (char *) desc,
			xi->arch, descsz);
	  if (xi->arch != holy_XEN_FILE_I386
	      && xi->arch != holy_XEN_FILE_I386_PAE
	      && xi->arch != holy_XEN_FILE_I386_PAE_BIMODE)
	    break;
	  if (descsz >= 3 && holy_memcmp (desc, "yes",
					  descsz == 3 ? 3 : 4) == 0)
	    {
	      xi->extended_cr3 = 1;
	      xi->arch = holy_XEN_FILE_I386_PAE;
	    }
	  if (descsz >= 7 && holy_memcmp (desc, "bimodal",
					  descsz == 7 ? 7 : 8) == 0)
	    {
	      xi->extended_cr3 = 1;
	      xi->arch = holy_XEN_FILE_I386_PAE_BIMODE;
	    }
	  if (descsz >= 11 && holy_memcmp (desc, "yes,bimodal",
					   descsz == 11 ? 11 : 12) == 0)
	    {
	      xi->extended_cr3 = 1;
	      xi->arch = holy_XEN_FILE_I386_PAE_BIMODE;
	    }
	  if (descsz >= 2 && holy_memcmp (desc, "no",
					  descsz == 2 ? 2 : 3) == 0)
	    xi->arch = holy_XEN_FILE_I386;
	  break;
	case XEN_ELFNOTE_INIT_P2M:
	  xi->p2m_base = holy_le_to_cpu_addr (*(Elf_Addr *) desc);
	  xi->has_p2m_base = 1;
	  break;
	case XEN_ELFNOTE_MOD_START_PFN:
	  xi->unmapped_initrd = !!holy_le_to_cpu32(*(holy_uint32_t *) desc);
	  break;
	default:
	  holy_dprintf ("xen", "unknown note type %d\n", nh->n_type);
	  break;
	}
    }
  return holy_ERR_NONE;
}

holy_err_t
holy_xen_get_infoXX (holy_elf_t elf, struct holy_xen_file_info *xi)
{
  Elf_Shdr *s, *s0;
  holy_size_t shnum = elf->ehdr.ehdrXX.e_shnum;
  holy_size_t shentsize = elf->ehdr.ehdrXX.e_shentsize;
  holy_size_t shsize = shnum * shentsize;
  holy_off_t stroff;
  holy_err_t err;
  Elf_Phdr *phdr;

  xi->kern_end = 0;
  xi->kern_start = ~0;
  xi->entry_point = elf->ehdr.ehdrXX.e_entry;

  /* FIXME: check note.  */
  FOR_ELF_PHDRS (elf, phdr)
    {
      Elf_Addr paddr;

      if (phdr->p_type == PT_NOTE)
	{
	  err = parse_note (elf, xi, phdr->p_offset, phdr->p_filesz);
	  if (err)
	    return err;
	}

      if (phdr->p_type != PT_LOAD)
	continue;

      paddr = phdr->p_paddr;

      if (paddr < xi->kern_start)
	xi->kern_start = paddr;

      if (paddr + phdr->p_memsz > xi->kern_end)
	xi->kern_end = paddr + phdr->p_memsz;
    }

  if (xi->has_note)
    return holy_ERR_NONE;

  if (!shnum || !shentsize)
    return holy_error (holy_ERR_BAD_OS, "no XEN note");

  s0 = holy_malloc (shsize);
  if (!s0)
    return holy_errno;

  if (holy_file_seek (elf->file, elf->ehdr.ehdrXX.e_shoff) == (holy_off_t) -1)
    {
      err = holy_errno;
      goto cleanup;
    }

  if (holy_file_read (elf->file, s0, shsize) != (holy_ssize_t) shsize)
    {
      if (holy_errno)
	err = holy_errno;
      else
	err = holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			 elf->file->name);
      goto cleanup;
    }

  s = (Elf_Shdr *) ((char *) s0 + elf->ehdr.ehdrXX.e_shstrndx * shentsize);
  stroff = s->sh_offset;

  for (s = s0; s < (Elf_Shdr *) ((char *) s0 + shnum * shentsize);
       s = (Elf_Shdr *) ((char *) s + shentsize))
    {
      char name[sizeof("__xen_guest")];
      holy_memset (name, 0, sizeof (name));
      if (holy_file_seek (elf->file, stroff + s->sh_name) == (holy_off_t) -1)
	{
	  err = holy_errno;
	  goto cleanup;
	}

      if (holy_file_read (elf->file, name, sizeof (name)) != (holy_ssize_t) sizeof (name))
	{
	  if (holy_errno)
	    {
	      err = holy_errno;
	      goto cleanup;
	    }
	  continue;
	}
      if (holy_memcmp (name, "__xen_guest",
		       sizeof("__xen_guest")) != 0)
	continue;
      err = parse_xen_guest (elf, xi, s->sh_offset, s->sh_size);
      goto cleanup;
    }
  err = holy_error (holy_ERR_BAD_OS, "no XEN note found");

cleanup:
  holy_free (s0);
  return err;
}
