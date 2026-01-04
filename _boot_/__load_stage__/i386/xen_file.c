/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/xen_file.h>
#include <holy/i386/linux.h>
#include <holy/misc.h>

#include <holy/verity-hash.h>

#define XZ_MAGIC "\3757zXZ\0"

holy_elf_t
holy_xen_file (holy_file_t file)
{
  return holy_xen_file_and_cmdline (file, NULL, 0);
}

holy_elf_t
holy_xen_file_and_cmdline (holy_file_t file,
			   char *cmdline,
			   holy_size_t cmdline_max_len)
{
  holy_elf_t elf;
  struct linux_kernel_header lh;
  holy_file_t off_file;
  holy_uint32_t payload_offset, payload_length;
  holy_uint8_t magic[6];

  elf = holy_elf_file (file, file->name);
  if (elf)
    return elf;
  holy_errno = holy_ERR_NONE;

  if (holy_file_seek (file, 0) == (holy_off_t) -1)
    goto fail;

  if (holy_file_read (file, &lh, sizeof (lh)) != sizeof (lh))
    goto fail;

  if (lh.boot_flag != holy_cpu_to_le16_compile_time (0xaa55)
      || lh.header != holy_cpu_to_le32_compile_time (holy_LINUX_MAGIC_SIGNATURE)
      || holy_le_to_cpu16 (lh.version) < 0x0208)
    {
      holy_error (holy_ERR_BAD_OS, "version too old for xen boot");
      return NULL;
    }

  payload_length = lh.payload_length;
  payload_offset = (lh.setup_sects + 1) * 512
    + lh.payload_offset;

  if (payload_length < sizeof (magic))
    {
      holy_error (holy_ERR_BAD_OS, "payload too short");
      return NULL;
    }

  holy_dprintf ("xen", "found bzimage payload 0x%llx-0x%llx\n",
		(unsigned long long) payload_offset,
		(unsigned long long) lh.payload_length);

  if (cmdline)
    holy_pass_verity_hash (&lh, cmdline, cmdline_max_len);

  holy_file_seek (file, payload_offset);

  if (holy_file_read (file, &magic, sizeof (magic)) != sizeof (magic))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    file->name);
      goto fail;
    }

  /* Kernel suffixes xz payload with their uncompressed size.
     Trim it.  */
  if (holy_memcmp (magic, XZ_MAGIC, sizeof (XZ_MAGIC) - 1) == 0)
    payload_length -= 4;
  off_file = holy_file_offset_open (file, payload_offset,
				    payload_length);
  if (!off_file)
    goto fail;

  elf = holy_elf_file (off_file, file->name);
  if (elf)
    return elf;
  holy_file_offset_close (off_file);

fail:
  holy_error (holy_ERR_BAD_OS, "not xen image");
  return NULL;
}

holy_err_t
holy_xen_get_info (holy_elf_t elf, struct holy_xen_file_info * xi)
{
  holy_memset (xi, 0, sizeof (*xi));

  if (holy_elf_is_elf64 (elf)
      && elf->ehdr.ehdr64.e_machine
      == holy_cpu_to_le16_compile_time (EM_X86_64)
      && elf->ehdr.ehdr64.e_ident[EI_DATA] == ELFDATA2LSB)
    {
      xi->arch = holy_XEN_FILE_X86_64;
      return holy_xen_get_info64 (elf, xi);
    }
  if (holy_elf_is_elf32 (elf)
      && elf->ehdr.ehdr32.e_machine == holy_cpu_to_le16_compile_time (EM_386)
      && elf->ehdr.ehdr32.e_ident[EI_DATA] == ELFDATA2LSB)
    {
      xi->arch = holy_XEN_FILE_I386;
      return holy_xen_get_info32 (elf, xi);
    }
  return holy_error (holy_ERR_BAD_OS, "unknown ELF type");
}
