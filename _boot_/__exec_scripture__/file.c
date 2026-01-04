/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/command.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/file.h>
#include <holy/elf.h>
#include <holy/xen_file.h>
#include <holy/efi/pe32.h>
#include <holy/i386/linux.h>
#include <holy/xnu.h>
#include <holy/machoload.h>
#include <holy/fileid.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] = {
  {"is-i386-xen-pae-domu", 0, 0,
   N_("Check if FILE can be booted as i386 PAE Xen unprivileged guest kernel"),
   0, 0},
  {"is-x86_64-xen-domu", 0, 0,
   N_("Check if FILE can be booted as x86_64 Xen unprivileged guest kernel"), 0, 0},
  {"is-x86-xen-dom0", 0, 0,
   N_("Check if FILE can be used as Xen x86 privileged guest kernel"), 0, 0},
  {"is-x86-multiboot", 0, 0,
   N_("Check if FILE can be used as x86 multiboot kernel"), 0, 0},
  {"is-x86-multiboot2", 0, 0,
   N_("Check if FILE can be used as x86 multiboot2 kernel"), 0, 0},
  {"is-arm-linux", 0, 0,
   N_("Check if FILE is ARM Linux"), 0, 0},
  {"is-arm64-linux", 0, 0,
   N_("Check if FILE is ARM64 Linux"), 0, 0},
  {"is-ia64-linux", 0, 0,
   N_("Check if FILE is IA64 Linux"), 0, 0},
  {"is-mips-linux", 0, 0,
   N_("Check if FILE is MIPS Linux"), 0, 0},
  {"is-mipsel-linux", 0, 0,
   N_("Check if FILE is MIPSEL Linux"), 0, 0},
  {"is-sparc64-linux", 0, 0,
   N_("Check if FILE is SPARC64 Linux"), 0, 0},
  {"is-powerpc-linux", 0, 0,
   N_("Check if FILE is POWERPC Linux"), 0, 0},
  {"is-x86-linux", 0, 0,
   N_("Check if FILE is x86 Linux"), 0, 0},
  {"is-x86-linux32", 0, 0,
   N_("Check if FILE is x86 Linux supporting 32-bit protocol"), 0, 0},
  {"is-x86-kfreebsd", 0, 0,
   N_("Check if FILE is x86 kFreeBSD"), 0, 0},
  {"is-i386-kfreebsd", 0, 0,
   N_("Check if FILE is i386 kFreeBSD"), 0, 0},
  {"is-x86_64-kfreebsd", 0, 0,
   N_("Check if FILE is x86_64 kFreeBSD"), 0, 0},

  {"is-x86-knetbsd", 0, 0,
   N_("Check if FILE is x86 kNetBSD"), 0, 0},
  {"is-i386-knetbsd", 0, 0,
   N_("Check if FILE is i386 kNetBSD"), 0, 0},
  {"is-x86_64-knetbsd", 0, 0,
   N_("Check if FILE is x86_64 kNetBSD"), 0, 0},

  {"is-i386-efi", 0, 0,
   N_("Check if FILE is i386 EFI file"), 0, 0},
  {"is-x86_64-efi", 0, 0,
   N_("Check if FILE is x86_64 EFI file"), 0, 0},
  {"is-ia64-efi", 0, 0,
   N_("Check if FILE is IA64 EFI file"), 0, 0},
  {"is-arm64-efi", 0, 0,
   N_("Check if FILE is ARM64 EFI file"), 0, 0},
  {"is-arm-efi", 0, 0,
   N_("Check if FILE is ARM EFI file"), 0, 0},
  {"is-hibernated-hiberfil", 0, 0,
   N_("Check if FILE is hiberfil.sys in hibernated state"), 0, 0},
  {"is-x86_64-xnu", 0, 0,
   N_("Check if FILE is x86_64 XNU (Mac OS X kernel)"), 0, 0},
  {"is-i386-xnu", 0, 0,
   N_("Check if FILE is i386 XNU (Mac OS X kernel)"), 0, 0},
  {"is-xnu-hibr", 0, 0,
   N_("Check if FILE is XNU (Mac OS X kernel) hibernated image"), 0, 0},
  {"is-x86-bios-bootsector", 0, 0,
   N_("Check if FILE is BIOS bootsector"), 0, 0},
  {0, 0, 0, 0, 0, 0}
};

enum
{
  IS_PAE_DOMU,
  IS_64_DOMU,
  IS_DOM0,
  IS_MULTIBOOT,
  IS_MULTIBOOT2,
  IS_ARM_LINUX,
  IS_ARM64_LINUX,
  IS_IA64_LINUX,
  IS_MIPS_LINUX,
  IS_MIPSEL_LINUX,
  IS_SPARC64_LINUX,
  IS_POWERPC_LINUX,
  IS_X86_LINUX,
  IS_X86_LINUX32,
  IS_X86_KFREEBSD,
  IS_X86_KFREEBSD32,
  IS_X86_KFREEBSD64,
  IS_X86_KNETBSD,
  IS_X86_KNETBSD32,
  IS_X86_KNETBSD64,
  IS_32_EFI,
  IS_64_EFI,
  IS_IA_EFI,
  IS_ARM64_EFI,
  IS_ARM_EFI,
  IS_HIBERNATED,
  IS_XNU64,
  IS_XNU32,
  IS_XNU_HIBR,
  IS_BIOS_BOOTSECTOR,
  OPT_TYPE_MIN = IS_PAE_DOMU,
  OPT_TYPE_MAX = IS_BIOS_BOOTSECTOR
};


static holy_err_t
holy_cmd_file (holy_extcmd_context_t ctxt, int argc, char **args)
{
  holy_file_t file = 0;
  holy_elf_t elf = 0;
  holy_err_t err;
  int type = -1, i;
  int ret = 0;
  holy_macho_t macho = 0;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
  for (i = OPT_TYPE_MIN; i <= OPT_TYPE_MAX; i++)
    if (ctxt->state[i].set)
      {
	if (type == -1)
	  {
	    type = i;
	    continue;
	  }
	return holy_error (holy_ERR_BAD_ARGUMENT, "multiple types specified");
      }
  if (type == -1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no type specified");

  file = holy_file_open (args[0]);
  if (!file)
    return holy_errno;
  switch (type)
    {
    case IS_BIOS_BOOTSECTOR:
      {
	holy_uint16_t sig;
	if (holy_file_size (file) != 512)
	  break;
	if (holy_file_seek (file, 510) == (holy_size_t) -1)
	  break;
	if (holy_file_read (file, &sig, 2) != 2)
	  break;
	if (sig != holy_cpu_to_le16_compile_time (0xaa55))
	  break;
	ret = 1;
	break;
      }
    case IS_IA64_LINUX:
      {
	Elf64_Ehdr ehdr;

	if (holy_file_read (file, &ehdr, sizeof (ehdr)) != sizeof (ehdr))
	  break;

	if (ehdr.e_ident[EI_MAG0] != ELFMAG0
	    || ehdr.e_ident[EI_MAG1] != ELFMAG1
	    || ehdr.e_ident[EI_MAG2] != ELFMAG2
	    || ehdr.e_ident[EI_MAG3] != ELFMAG3
	    || ehdr.e_ident[EI_VERSION] != EV_CURRENT
	    || ehdr.e_version != EV_CURRENT)
	  break;

	if (ehdr.e_ident[EI_CLASS] != ELFCLASS64
	    || ehdr.e_ident[EI_DATA] != ELFDATA2LSB
	    || ehdr.e_machine != holy_cpu_to_le16_compile_time (EM_IA_64))
	  break;

	ret = 1;

	break;
      }

    case IS_SPARC64_LINUX:
      {
	Elf64_Ehdr ehdr;

	if (holy_file_read (file, &ehdr, sizeof (ehdr)) != sizeof (ehdr))
	  break;

	if (ehdr.e_ident[EI_MAG0] != ELFMAG0
	    || ehdr.e_ident[EI_MAG1] != ELFMAG1
	    || ehdr.e_ident[EI_MAG2] != ELFMAG2
	    || ehdr.e_ident[EI_MAG3] != ELFMAG3
	    || ehdr.e_ident[EI_VERSION] != EV_CURRENT
	    || ehdr.e_version != EV_CURRENT)
	  break;

	if (ehdr.e_ident[EI_CLASS] != ELFCLASS64
	    || ehdr.e_ident[EI_DATA] != ELFDATA2MSB)
	  break;

	if (ehdr.e_machine != holy_cpu_to_le16_compile_time (EM_SPARCV9)
	    || ehdr.e_type != holy_cpu_to_be16_compile_time (ET_EXEC))
	  break;

	ret = 1;

	break;
      }

    case IS_POWERPC_LINUX:
      {
	Elf32_Ehdr ehdr;

	if (holy_file_read (file, &ehdr, sizeof (ehdr)) != sizeof (ehdr))
	  break;

	if (ehdr.e_ident[EI_MAG0] != ELFMAG0
	    || ehdr.e_ident[EI_MAG1] != ELFMAG1
	    || ehdr.e_ident[EI_MAG2] != ELFMAG2
	    || ehdr.e_ident[EI_MAG3] != ELFMAG3
	    || ehdr.e_ident[EI_VERSION] != EV_CURRENT
	    || ehdr.e_version != EV_CURRENT)
	  break;

	if (ehdr.e_ident[EI_DATA] != ELFDATA2MSB
	    || (ehdr.e_machine != holy_cpu_to_le16_compile_time (EM_PPC)
		&& ehdr.e_machine !=
		holy_cpu_to_le16_compile_time (EM_PPC64)))
	  break;

	if (ehdr.e_type != holy_cpu_to_be16_compile_time (ET_EXEC)
	    && ehdr.e_type != holy_cpu_to_be16_compile_time (ET_DYN))
	  break;

	ret = 1;

	break;
      }

    case IS_MIPS_LINUX:
      {
	Elf32_Ehdr ehdr;

	if (holy_file_read (file, &ehdr, sizeof (ehdr)) != sizeof (ehdr))
	  break;

	if (ehdr.e_ident[EI_MAG0] != ELFMAG0
	    || ehdr.e_ident[EI_MAG1] != ELFMAG1
	    || ehdr.e_ident[EI_MAG2] != ELFMAG2
	    || ehdr.e_ident[EI_MAG3] != ELFMAG3
	    || ehdr.e_ident[EI_VERSION] != EV_CURRENT
	    || ehdr.e_version != EV_CURRENT)
	  break;

	if (ehdr.e_ident[EI_DATA] != ELFDATA2MSB
	    || ehdr.e_machine != holy_cpu_to_be16_compile_time (EM_MIPS)
	    || ehdr.e_type != holy_cpu_to_be16_compile_time (ET_EXEC))
	  break;

	ret = 1;

	break;
      }

    case IS_X86_KNETBSD:
    case IS_X86_KNETBSD32:
    case IS_X86_KNETBSD64:
      {
	int is32, is64;

	elf = holy_elf_file (file, file->name);

	if (elf->ehdr.ehdr32.e_type != holy_cpu_to_le16_compile_time (ET_EXEC)
	    || elf->ehdr.ehdr32.e_ident[EI_DATA] != ELFDATA2LSB)
	  break;

	is32 = holy_elf_is_elf32 (elf);
	is64 = holy_elf_is_elf64 (elf);
	if (!is32 && !is64)
	  break;
	if (!is32 && type == IS_X86_KNETBSD32)
	  break;
	if (!is64 && type == IS_X86_KNETBSD64)
	  break;
	if (is64)
	  ret = holy_file_check_netbsd64 (elf);
	if (is32)
	  ret = holy_file_check_netbsd32 (elf);
	break;
      }

    case IS_X86_KFREEBSD:
    case IS_X86_KFREEBSD32:
    case IS_X86_KFREEBSD64:
      {
	Elf32_Ehdr ehdr;
	int is32, is64;

	if (holy_file_read (file, &ehdr, sizeof (ehdr)) != sizeof (ehdr))
	  break;

	if (ehdr.e_ident[EI_MAG0] != ELFMAG0
	    || ehdr.e_ident[EI_MAG1] != ELFMAG1
	    || ehdr.e_ident[EI_MAG2] != ELFMAG2
	    || ehdr.e_ident[EI_MAG3] != ELFMAG3
	    || ehdr.e_ident[EI_VERSION] != EV_CURRENT
	    || ehdr.e_version != EV_CURRENT)
	  break;

	if (ehdr.e_type != holy_cpu_to_le16_compile_time (ET_EXEC)
	    || ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
	  break;

	if (ehdr.e_ident[EI_OSABI] != ELFOSABI_FREEBSD)
	  break;

	is32 = (ehdr.e_machine == holy_cpu_to_le16_compile_time (EM_386)
		&& ehdr.e_ident[EI_CLASS] == ELFCLASS32);
	is64 = (ehdr.e_machine == holy_cpu_to_le16_compile_time (EM_X86_64)
		&& ehdr.e_ident[EI_CLASS] == ELFCLASS64);
	if (!is32 && !is64)
	  break;
	if (!is32 && (type == IS_X86_KFREEBSD32 || type == IS_X86_KNETBSD32))
	  break;
	if (!is64 && (type == IS_X86_KFREEBSD64 || type == IS_X86_KNETBSD64))
	  break;
	ret = 1;

	break;
      }


    case IS_MIPSEL_LINUX:
      {
	Elf32_Ehdr ehdr;

	if (holy_file_read (file, &ehdr, sizeof (ehdr)) != sizeof (ehdr))
	  break;

	if (ehdr.e_ident[EI_MAG0] != ELFMAG0
	    || ehdr.e_ident[EI_MAG1] != ELFMAG1
	    || ehdr.e_ident[EI_MAG2] != ELFMAG2
	    || ehdr.e_ident[EI_MAG3] != ELFMAG3
	    || ehdr.e_ident[EI_VERSION] != EV_CURRENT
	    || ehdr.e_version != EV_CURRENT)
	  break;

	if (ehdr.e_machine != holy_cpu_to_le16_compile_time (EM_MIPS)
	    || ehdr.e_type != holy_cpu_to_le16_compile_time (ET_EXEC))
	  break;

	ret = 1;

	break;
      }
    case IS_ARM_LINUX:
      {
	holy_uint32_t sig, sig_pi;
	if (holy_file_read (file, &sig_pi, 4) != 4)
	  break;
	/* Raspberry pi.  */
	if (sig_pi == holy_cpu_to_le32_compile_time (0xea000006))
	  {
	    ret = 1;
	    break;
	  }

	if (holy_file_seek (file, 0x24) == (holy_size_t) -1)
	  break;
	if (holy_file_read (file, &sig, 4) != 4)
	  break;
	if (sig == holy_cpu_to_le32_compile_time (0x016f2818))
	  {
	    ret = 1;
	    break;
	  }
	break;
      }
    case IS_ARM64_LINUX:
      {
	holy_uint32_t sig;

	if (holy_file_seek (file, 0x38) == (holy_size_t) -1)
	  break;
	if (holy_file_read (file, &sig, 4) != 4)
	  break;
	if (sig == holy_cpu_to_le32_compile_time (0x644d5241))
	  {
	    ret = 1;
	    break;
	  }
	break;
      }
    case IS_PAE_DOMU ... IS_DOM0:
      {
	struct holy_xen_file_info xen_inf;
	elf = holy_xen_file (file);
	if (!elf)
	  break;
	err = holy_xen_get_info (elf, &xen_inf);
	if (err)
	  break;
	/* Unfortuntely no way to check if kernel supports dom0.  */
	if (type == IS_DOM0)
	  ret = 1;
	if (type == IS_PAE_DOMU)
	  ret = (xen_inf.arch == holy_XEN_FILE_I386_PAE
		 || xen_inf.arch == holy_XEN_FILE_I386_PAE_BIMODE);
	if (type == IS_64_DOMU)
	  ret = (xen_inf.arch == holy_XEN_FILE_X86_64);
	break;
      }
    case IS_MULTIBOOT:
    case IS_MULTIBOOT2:
      {
	holy_uint32_t *buffer;
	holy_ssize_t len;
	holy_size_t search_size;
	holy_uint32_t *header;
	holy_uint32_t magic;
	holy_size_t step;

	if (type == IS_MULTIBOOT2)
	  {
	    search_size = 32768;
	    magic = holy_cpu_to_le32_compile_time (0xe85250d6);
	    step = 2;
	  }
	else
	  {
	    search_size = 8192;
	    magic = holy_cpu_to_le32_compile_time (0x1BADB002);
	    step = 1;
	  }

	buffer = holy_malloc (search_size);
	if (!buffer)
	  break;

	len = holy_file_read (file, buffer, search_size);
	if (len < 32)
	  {
	    holy_free (buffer);
	    break;
	  }

	/* Look for the multiboot header in the buffer.  The header should
	   be at least 12 bytes and aligned on a 4-byte boundary.  */
	for (header = buffer;
	     ((char *) header <=
	      (char *) buffer + len - (type == IS_MULTIBOOT2 ? 16 : 12));
	     header += step)
	  {
	    if (header[0] == magic
		&& !(holy_le_to_cpu32 (header[0])
		     + holy_le_to_cpu32 (header[1])
		     + holy_le_to_cpu32 (header[2])
		     + (type == IS_MULTIBOOT2
			? holy_le_to_cpu32 (header[3]) : 0)))
	      {
		ret = 1;
		break;
	      }
	  }

	holy_free (buffer);
	break;
      }
    case IS_X86_LINUX32:
    case IS_X86_LINUX:
      {
	struct linux_kernel_header lh;
	if (holy_file_read (file, &lh, sizeof (lh)) != sizeof (lh))
	  break;
	if (lh.boot_flag != holy_cpu_to_le16_compile_time (0xaa55))
	  break;

	if (lh.setup_sects > holy_LINUX_MAX_SETUP_SECTS)
	  break;

	/* FIXME: some really old kernels (< 1.3.73) will fail this.  */
	if (lh.header !=
	    holy_cpu_to_le32_compile_time (holy_LINUX_MAGIC_SIGNATURE)
	    || holy_le_to_cpu16 (lh.version) < 0x0200)
	  break;

	if (type == IS_X86_LINUX)
	  {
	    ret = 1;
	    break;
	  }

	/* FIXME: 2.03 is not always good enough (Linux 2.4 can be 2.03 and
	   still not support 32-bit boot.  */
	if (lh.header !=
	    holy_cpu_to_le32_compile_time (holy_LINUX_MAGIC_SIGNATURE)
	    || holy_le_to_cpu16 (lh.version) < 0x0203)
	  break;

	if (!(lh.loadflags & holy_LINUX_FLAG_BIG_KERNEL))
	  break;
	ret = 1;
	break;
      }
    case IS_HIBERNATED:
      {
	holy_uint8_t hibr_file_magic[4];
	if (holy_file_read (file, &hibr_file_magic, sizeof (hibr_file_magic))
	    != sizeof (hibr_file_magic))
	  break;
	if (holy_memcmp ("hibr", hibr_file_magic, sizeof (hibr_file_magic)) ==
	    0
	    || holy_memcmp ("HIBR", hibr_file_magic,
			    sizeof (hibr_file_magic)) == 0)
	  ret = 1;
	break;
      }
    case IS_XNU64:
    case IS_XNU32:
      {
	macho = holy_macho_open (args[0], (type == IS_XNU64));
	if (!macho)
	  break;
	/* FIXME: more checks?  */
	ret = 1;
	break;
      }
    case IS_XNU_HIBR:
      {
	struct holy_xnu_hibernate_header hibhead;
	if (holy_file_read (file, &hibhead, sizeof (hibhead))
	    != sizeof (hibhead))
	  break;
	if (hibhead.magic !=
	    holy_cpu_to_le32_compile_time (holy_XNU_HIBERNATE_MAGIC))
	  break;
	ret = 1;
	break;
      }
    case IS_32_EFI:
    case IS_64_EFI:
    case IS_IA_EFI:
    case IS_ARM64_EFI:
    case IS_ARM_EFI:
      {
	char signature[4];
	holy_uint32_t pe_offset;
	struct holy_pe32_coff_header coff_head;

	if (holy_file_read (file, signature, 2) != 2)
	  break;
	if (signature[0] != 'M' || signature[1] != 'Z')
	  break;
	if ((holy_ssize_t) holy_file_seek (file, 0x3c) == -1)
	  break;
	if (holy_file_read (file, &pe_offset, 4) != 4)
	  break;
	if ((holy_ssize_t) holy_file_seek (file, holy_le_to_cpu32 (pe_offset))
	    == -1)
	  break;
	if (holy_file_read (file, signature, 4) != 4)
	  break;
	if (signature[0] != 'P' || signature[1] != 'E'
	    || signature[2] != '\0' || signature[3] != '\0')
	  break;

	if (holy_file_read (file, &coff_head, sizeof (coff_head))
	    != sizeof (coff_head))
	  break;
	if (type == IS_32_EFI
	    && coff_head.machine !=
	    holy_cpu_to_le16_compile_time (holy_PE32_MACHINE_I386))
	  break;
	if (type == IS_64_EFI
	    && coff_head.machine !=
	    holy_cpu_to_le16_compile_time (holy_PE32_MACHINE_X86_64))
	  break;
	if (type == IS_IA_EFI
	    && coff_head.machine !=
	    holy_cpu_to_le16_compile_time (holy_PE32_MACHINE_IA64))
	  break;
	if (type == IS_ARM64_EFI
	    && coff_head.machine !=
	    holy_cpu_to_le16_compile_time (holy_PE32_MACHINE_ARM64))
	  break;
	if (type == IS_ARM_EFI
	    && coff_head.machine !=
	    holy_cpu_to_le16_compile_time (holy_PE32_MACHINE_ARMTHUMB_MIXED))
	  break;
	if (type == IS_IA_EFI || type == IS_64_EFI || type == IS_ARM64_EFI)
	  {
	    struct holy_pe64_optional_header o64;
	    if (holy_file_read (file, &o64, sizeof (o64)) != sizeof (o64))
	      break;
	    if (o64.magic !=
		holy_cpu_to_le16_compile_time (holy_PE32_PE64_MAGIC))
	      break;
	    if (o64.subsystem !=
		holy_cpu_to_le16_compile_time
		(holy_PE32_SUBSYSTEM_EFI_APPLICATION))
	      break;
	    ret = 1;
	    break;
	  }
	if (type == IS_32_EFI || type == IS_ARM_EFI)
	  {
	    struct holy_pe32_optional_header o32;
	    if (holy_file_read (file, &o32, sizeof (o32)) != sizeof (o32))
	      break;
	    if (o32.magic !=
		holy_cpu_to_le16_compile_time (holy_PE32_PE32_MAGIC))
	      break;
	    if (o32.subsystem !=
		holy_cpu_to_le16_compile_time
		(holy_PE32_SUBSYSTEM_EFI_APPLICATION))
	      break;
	    ret = 1;
	    break;
	  }
	break;
      }
    }

  if (elf)
    holy_elf_close (elf);
  else if (macho)
    holy_macho_close (macho);
  else if (file)
    holy_file_close (file);

  if (!ret && (holy_errno == holy_ERR_BAD_OS || holy_errno == holy_ERR_NONE))
    /* TRANSLATORS: it's a standalone boolean value,
       opposite of "true".  */
    holy_error (holy_ERR_TEST_FAILURE, N_("false"));
  return holy_errno;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(file)
{
  cmd = holy_register_extcmd ("file", holy_cmd_file, 0,
			      N_("OPTIONS FILE"),
			      N_("Check if FILE is of specified type."),
			      options);
}

holy_MOD_FINI(file)
{
  holy_unregister_extcmd (cmd);
}
