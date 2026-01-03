/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/memory.h>
#include <holy/i386/memory.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/elfload.h>
#include <holy/video.h>
#include <holy/relocator.h>
#include <holy/i386/relocator.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/cbfs_core.h>
#include <holy/lib/LzmaDec.h>
#include <holy/efi/pe32.h>
#include <holy/i386/cpuid.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_addr_t entry;
static struct holy_relocator *relocator = NULL;

static holy_err_t
holy_chain_boot (void)
{
  struct holy_relocator32_state state;

  holy_video_set_mode ("text", 0, 0);

  state.eip = entry;
  return holy_relocator32_boot (relocator, state, 0);
}

static holy_err_t
holy_chain_unload (void)
{
  holy_relocator_unload (relocator);
  relocator = NULL;

  return holy_ERR_NONE;
}

static holy_err_t
load_elf (holy_file_t file, const char *filename)
{
  holy_elf_t elf;
  Elf32_Phdr *phdr;
  holy_err_t err;

  elf = holy_elf_file (file, filename);
  if (!elf)
    return holy_errno;

  if (!holy_elf_is_elf32 (elf))
    return holy_error (holy_ERR_BAD_OS, "only ELF32 can be coreboot payload");

  entry = elf->ehdr.ehdr32.e_entry;

  FOR_ELF32_PHDRS(elf, phdr)
    {
      holy_uint8_t *load_addr;
      holy_relocator_chunk_t ch;

      if (phdr->p_type != PT_LOAD)
	continue;

      err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					     phdr->p_paddr, phdr->p_memsz);
      if (err)
	{
	  elf->file = 0;
	  holy_elf_close (elf);
	  return err;
	}

      load_addr = get_virtual_current_address (ch);

      if (holy_file_seek (elf->file, phdr->p_offset) == (holy_off_t) -1)
	{
	  elf->file = 0;
	  holy_elf_close (elf);
	  return holy_errno;
	}

      if (phdr->p_filesz)
	{
	  holy_ssize_t read;
	  read = holy_file_read (elf->file, load_addr, phdr->p_filesz);
	  if (read != (holy_ssize_t) phdr->p_filesz)
	    {
	      if (!holy_errno)
		holy_error (holy_ERR_FILE_READ_ERROR,
			    N_("premature end of file %s"),
			    filename);
	      elf->file = 0;
	      holy_elf_close (elf);
	      return holy_errno;
	    }
	}

      if (phdr->p_filesz < phdr->p_memsz)
	holy_memset ((load_addr + phdr->p_filesz),
		     0, phdr->p_memsz - phdr->p_filesz);
    }

  elf->file = 0;
  holy_elf_close (elf);
  return holy_ERR_NONE;
}

static void *SzAlloc(void *p __attribute__ ((unused)), size_t size) { return holy_malloc (size); }
static void SzFree(void *p __attribute__ ((unused)), void *address) { holy_free (address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };


static holy_err_t
load_segment (holy_file_t file, const char *filename,
	      void *load_addr, holy_uint32_t comp,
	      holy_size_t *size, holy_size_t max_size)
{
  switch (comp)
    {
    case holy_cpu_to_be32_compile_time (CBFS_COMPRESS_NONE):
      if (holy_file_read (file, load_addr, *size)
	  != (holy_ssize_t) *size)
	{
	  if (!holy_errno)
	    holy_error (holy_ERR_FILE_READ_ERROR,
			N_("premature end of file %s"),
			filename);
	      return holy_errno;
	}
      return holy_ERR_NONE;
    case holy_cpu_to_be32_compile_time (CBFS_COMPRESS_LZMA):
      {
	holy_uint8_t *buf;
	holy_size_t outsize, insize;
	SRes res;
	SizeT src_len, dst_len;
	ELzmaStatus status;
	if (*size < 13)
	  return holy_error (holy_ERR_BAD_OS, "invalid compressed chunk");
	buf = holy_malloc (*size);
	if (!buf)
	  return holy_errno;
	if (holy_file_read (file, buf, *size)
	    != (holy_ssize_t) *size)
	  {
	    if (!holy_errno)
	      holy_error (holy_ERR_FILE_READ_ERROR,
			  N_("premature end of file %s"),
			  filename);
	    holy_free (buf);
	    return holy_errno;
	  }
	outsize = holy_get_unaligned64 (buf + 5);
	if (outsize > max_size)
	  {
	    holy_free (buf);
	    return holy_error (holy_ERR_BAD_OS, "invalid compressed chunk");
	  }
	insize = *size - 13;

	src_len = insize;
	dst_len = outsize;
	res = LzmaDecode (load_addr, &dst_len, buf + 13, &src_len,
			  buf, 5, LZMA_FINISH_END, &status, &g_Alloc);
	/* ELzmaFinishMode finishMode,
	   ELzmaStatus *status, ISzAlloc *alloc)*/
	holy_free (buf);
	holy_dprintf ("chain", "%x, %x, %x, %x\n",
		      insize, src_len, outsize, dst_len);
	if (res != SZ_OK
	    || src_len != insize || dst_len != outsize)
	  return holy_error (holy_ERR_BAD_OS, "decompression failure %d", res);
	*size = outsize;
      }
      return holy_ERR_NONE;
    default:
      return holy_error (holy_ERR_BAD_OS, "unsupported compression %d",
			 holy_be_to_cpu32 (comp));
    }
}

static holy_err_t
load_tianocore (holy_file_t file)
{
  holy_uint16_t header_length;
  holy_uint32_t section_head;
  holy_uint8_t mz[2], pe[4];
  struct holy_pe32_coff_header coff_head;
  struct file_header
  {
    holy_uint8_t unused[18];
    holy_uint8_t type;
    holy_uint8_t unused2;
    holy_uint8_t size[3];
    holy_uint8_t unused3;
  } file_head;
  holy_relocator_chunk_t ch;

  if (holy_file_seek (file, 48) == (holy_off_t) -1
      || holy_file_read (file, &header_length, sizeof (header_length))
      != sizeof (header_length)
      || holy_file_seek (file, header_length) == (holy_off_t) -1)
    goto fail;

  while (1)
    {
      holy_off_t off;
      if (holy_file_read (file, &file_head, sizeof (file_head))
	  != sizeof (file_head))
	goto fail;
      if (file_head.type != 0xf0)
	break;
      off = holy_get_unaligned32 (file_head.size) & 0xffffff;
      if (off < sizeof (file_head))
	goto fail;
      if (holy_file_seek (file, holy_file_tell (file) + off
			  - sizeof (file_head)) == (holy_off_t) -1)
	goto fail;
    }

  if (file_head.type != 0x03)
    goto fail;

  while (1)
    {
      if (holy_file_read (file, &section_head, sizeof (section_head))
	  != sizeof (section_head))
	goto fail;
      if ((section_head >> 24) != 0x19)
	break;

      if ((section_head & 0xffffff) < sizeof (section_head))
	goto fail;

      if (holy_file_seek (file, holy_file_tell (file)
			  + (section_head & 0xffffff)
			  - sizeof (section_head)) == (holy_off_t) -1)
	goto fail;
    }

  if ((section_head >> 24) != 0x10)
    goto fail;

  holy_off_t exe_start = holy_file_tell (file);

  if (holy_file_read (file, &mz, sizeof (mz)) != sizeof (mz))
    goto fail;
  if (mz[0] != 'M' || mz[1] != 'Z')
    goto fail;

  if (holy_file_seek (file, holy_file_tell (file) + 0x3a) == (holy_off_t) -1)
    goto fail;

  if (holy_file_read (file, &section_head, sizeof (section_head))
      != sizeof (section_head))
    goto fail;
  if (section_head < 0x40)
    goto fail;

  if (holy_file_seek (file, holy_file_tell (file)
		      + section_head - 0x40) == (holy_off_t) -1)
    goto fail;

  if (holy_file_read (file, &pe, sizeof (pe))
      != sizeof (pe))
    goto fail;

  if (pe[0] != 'P' || pe[1] != 'E' || pe[2] != '\0' || pe[3] != '\0')
    goto fail;

  if (holy_file_read (file, &coff_head, sizeof (coff_head))
      != sizeof (coff_head))
    goto fail;

  holy_uint32_t loadaddr;

  switch (coff_head.machine)
    {
    case holy_PE32_MACHINE_I386:
      {
	struct holy_pe32_optional_header oh;
	if (holy_file_read (file, &oh, sizeof (oh))
	    != sizeof (oh))
	  goto fail;
	if (oh.magic != holy_PE32_PE32_MAGIC)
	  goto fail;
	loadaddr = oh.image_base - exe_start;
	entry = oh.image_base + oh.entry_addr;
	break;
      }
    case holy_PE32_MACHINE_X86_64:
      {
	struct holy_pe64_optional_header oh;
	if (! holy_cpuid_has_longmode)
	  {
	    holy_error (holy_ERR_BAD_OS, "your CPU does not implement AMD64 architecture");
	    goto fail;
	  }

	if (holy_file_read (file, &oh, sizeof (oh))
	    != sizeof (oh))
	  goto fail;
	if (oh.magic != holy_PE32_PE64_MAGIC)
	  goto fail;
	loadaddr = oh.image_base - exe_start;
	entry = oh.image_base + oh.entry_addr;
	break;
      }
    default:
      goto fail;
    }
  if (holy_file_seek (file, 0) == (holy_off_t) -1)
    goto fail;

  holy_size_t fz = holy_file_size (file);

  if (holy_relocator_alloc_chunk_addr (relocator, &ch,
				       loadaddr, fz))
    goto fail;

  if (holy_file_read (file, get_virtual_current_address (ch), fz)
      != (holy_ssize_t) fz)
    goto fail;

  return holy_ERR_NONE;

 fail:
  if (!holy_errno)
    holy_error (holy_ERR_BAD_OS, "fv volume is invalid");
  return holy_errno;
}

static holy_err_t
load_chewed (holy_file_t file, const char *filename)
{
  holy_size_t i;
  for (i = 0;; i++)
    {
      struct cbfs_payload_segment segment;
      holy_err_t err;

      if (holy_file_seek (file, sizeof (segment) * i) == (holy_off_t) -1
	  || holy_file_read (file, &segment, sizeof (segment))
	  != sizeof (segment))
	{
	  if (!holy_errno)
	    return holy_error (holy_ERR_BAD_OS,
			       "payload is too short");
	  return holy_errno;
	}

      switch (segment.type)
	{
	case PAYLOAD_SEGMENT_PARAMS:
	  break;

	case PAYLOAD_SEGMENT_ENTRY:
	  entry = holy_be_to_cpu64 (segment.load_addr);
	  return holy_ERR_NONE;

	case PAYLOAD_SEGMENT_BSS:
	  segment.len = 0;
	  segment.offset = 0;
	  segment.len = 0;
	  /* Fallthrough.  */
	case PAYLOAD_SEGMENT_CODE:
	case PAYLOAD_SEGMENT_DATA:
	  {
	    holy_uint32_t target = holy_be_to_cpu64 (segment.load_addr);
	    holy_uint32_t memsize = holy_be_to_cpu32 (segment.mem_len);
	    holy_uint32_t filesize = holy_be_to_cpu32 (segment.len);
	    holy_uint8_t *load_addr;
	    holy_relocator_chunk_t ch;

	    if (memsize < filesize)
	      memsize = filesize;

	    holy_dprintf ("chain", "%x+%x\n", target, memsize);

	    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
						   target, memsize);
	    if (err)
	      return err;

	    load_addr = get_virtual_current_address (ch);

	    if (filesize)
	      {
		if (holy_file_seek (file, holy_be_to_cpu32 (segment.offset))
		    == (holy_off_t) -1)
		  return holy_errno;

		err = load_segment (file, filename, load_addr,
				    segment.compression, &filesize, memsize);
		if (err)
		  return err;
	      }

	    if (filesize < memsize)
	      holy_memset ((load_addr + filesize),
			   0, memsize - filesize);
	  }
	}
    }
}

static holy_err_t
holy_cmd_chain (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_err_t err;
  holy_file_t file;
  holy_uint32_t head;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_loader_unset ();

  file = holy_file_open (argv[0]);
  if (!file)
    return holy_errno;

  relocator = holy_relocator_new ();
  if (!relocator)
    {
      holy_file_close (file);
      return holy_errno;
    }

  if (holy_file_read (file, &head, sizeof (head)) != sizeof (head)
      || holy_file_seek (file, 0) == (holy_off_t) -1)
    {
      holy_file_close (file);
      holy_relocator_unload (relocator);
      relocator = 0;
      if (!holy_errno)
	return holy_error (holy_ERR_BAD_OS,
			   "payload is too short");
      return holy_errno;
    }
      
  switch (head)
    {
    case ELFMAG0 | (ELFMAG1 << 8) | (ELFMAG2 << 16) | (ELFMAG3 << 24):
      err = load_elf (file, argv[0]);
      break;
    case PAYLOAD_SEGMENT_CODE:
    case PAYLOAD_SEGMENT_DATA:
    case PAYLOAD_SEGMENT_PARAMS:
    case PAYLOAD_SEGMENT_BSS:
    case PAYLOAD_SEGMENT_ENTRY:
      err = load_chewed (file, argv[0]);
      break;

    default:
      if (holy_file_seek (file, 40) == (holy_off_t) -1
	  || holy_file_read (file, &head, sizeof (head)) != sizeof (head)
	  || holy_file_seek (file, 0) == (holy_off_t) -1
	  || head != 0x4856465f)
	err = holy_error (holy_ERR_BAD_OS, "unrecognised payload type");
      else
	err = load_tianocore (file);
      break;
    }
  holy_file_close (file);
  if (err)
    {
      holy_relocator_unload (relocator);
      relocator = 0;
      return err;
    }

  holy_loader_set (holy_chain_boot, holy_chain_unload, 0);
  return holy_ERR_NONE;
}

static holy_command_t cmd_chain;

holy_MOD_INIT (chain)
{
  cmd_chain = holy_register_command ("chainloader", holy_cmd_chain,
				     N_("FILE"),
				     /* TRANSLATORS: "payload" is a term used
					by coreboot and must be translated in
					sync with coreboot. If unsure,
					let it untranslated.  */
				     N_("Load another coreboot payload"));
}

holy_MOD_FINI (chain)
{
  holy_unregister_command (cmd_chain);
  holy_chain_unload ();
}
