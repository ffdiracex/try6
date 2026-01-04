/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/command.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/cache.h>
#include <holy/kernel.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/elf.h>
#include <holy/i18n.h>
#include <holy/env.h>
#include <holy/linux.h>

holy_MOD_LICENSE ("GPLv2+");

#pragma GCC diagnostic ignored "-Wcast-align"

#define ALIGN_MIN (256*1024*1024)

#define holy_ELF_SEARCH 1024

#define BOOT_PARAM_SIZE	16384

struct ia64_boot_param
{
  holy_uint64_t command_line;	/* physical address of command line. */
  holy_uint64_t efi_systab;	/* physical address of EFI system table */
  holy_uint64_t efi_memmap;	/* physical address of EFI memory map */
  holy_uint64_t efi_memmap_size;	/* size of EFI memory map */
  holy_uint64_t efi_memdesc_size; /* size of an EFI memory map descriptor */
  holy_uint32_t efi_memdesc_version;	/* memory descriptor version */
  struct
  {
    holy_uint16_t num_cols;	/* number of columns on console output dev */
    holy_uint16_t num_rows;	/* number of rows on console output device */
    holy_uint16_t orig_x;	/* cursor's x position */
    holy_uint16_t orig_y;	/* cursor's y position */
  } console_info;
  holy_uint64_t fpswa;		/* physical address of the fpswa interface */
  holy_uint64_t initrd_start;
  holy_uint64_t initrd_size;
};

typedef struct
{
  holy_uint32_t	revision;
  holy_uint32_t	reserved;
  void *fpswa;
} fpswa_interface_t;
static fpswa_interface_t *fpswa;

#define NEXT_MEMORY_DESCRIPTOR(desc, size)      \
  ((holy_efi_memory_descriptor_t *) ((char *) (desc) + (size)))

static holy_dl_t my_mod;

static int loaded;

/* Kernel base and size.  */
static void *kernel_mem;
static holy_efi_uintn_t kernel_pages;
static holy_uint64_t entry;

/* Initrd base and size.  */
static void *initrd_mem;
static holy_efi_uintn_t initrd_pages;
static holy_efi_uintn_t initrd_size;

static struct ia64_boot_param *boot_param;
static holy_efi_uintn_t boot_param_pages;

static inline holy_size_t
page_align (holy_size_t size)
{
  return (size + (1 << 12) - 1) & (~((1 << 12) - 1));
}

static void
query_fpswa (void)
{
  holy_efi_handle_t fpswa_image;
  holy_efi_boot_services_t *bs;
  holy_efi_status_t status;
  holy_efi_uintn_t size;
  static const holy_efi_guid_t fpswa_protocol =
    { 0xc41b6531, 0x97b9, 0x11d3,
      {0x9a, 0x29, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d} };

  if (fpswa != NULL)
    return;

  size = sizeof(holy_efi_handle_t);
  
  bs = holy_efi_system_table->boot_services;
  status = bs->locate_handle (holy_EFI_BY_PROTOCOL,
			      (void *) &fpswa_protocol,
			      NULL, &size, &fpswa_image);
  if (status != holy_EFI_SUCCESS)
    {
      holy_printf ("%s\n", _("Could not locate FPSWA driver"));
      return;
    }
  status = bs->handle_protocol (fpswa_image,
				(void *) &fpswa_protocol, (void *) &fpswa);
  if (status != holy_EFI_SUCCESS)
    {
      holy_printf ("%s\n",
		   _("FPSWA protocol wasn't able to find the interface"));
      return;
    } 
}

/* Find the optimal number of pages for the memory map. Is it better to
   move this code to efi/mm.c?  */
static holy_efi_uintn_t
find_mmap_size (void)
{
  static holy_efi_uintn_t mmap_size = 0;

  if (mmap_size != 0)
    return mmap_size;
  
  mmap_size = (1 << 12);
  while (1)
    {
      int ret;
      holy_efi_memory_descriptor_t *mmap;
      holy_efi_uintn_t desc_size;
      
      mmap = holy_malloc (mmap_size);
      if (! mmap)
	return 0;

      ret = holy_efi_get_memory_map (&mmap_size, mmap, 0, &desc_size, 0);
      holy_free (mmap);
      
      if (ret < 0)
	{
	  holy_error (holy_ERR_IO, "cannot get memory map");
	  return 0;
	}
      else if (ret > 0)
	break;

      mmap_size += (1 << 12);
    }

  /* Increase the size a bit for safety, because holy allocates more on
     later, and EFI itself may allocate more.  */
  mmap_size += (1 << 12);

  return page_align (mmap_size);
}

static void
free_pages (void)
{
  if (kernel_mem)
    {
      holy_efi_free_pages ((holy_addr_t) kernel_mem, kernel_pages);
      kernel_mem = 0;
    }

  if (initrd_mem)
    {
      holy_efi_free_pages ((holy_addr_t) initrd_mem, initrd_pages);
      initrd_mem = 0;
    }

  if (boot_param)
    {
      /* Free bootparam.  */
      holy_efi_free_pages ((holy_efi_physical_address_t) boot_param,
			   boot_param_pages);
      boot_param = 0;
    }
}

static void *
allocate_pages (holy_uint64_t align, holy_uint64_t size_pages,
		holy_uint64_t nobase)
{
  holy_uint64_t size;
  holy_efi_uintn_t desc_size;
  holy_efi_memory_descriptor_t *mmap, *mmap_end;
  holy_efi_uintn_t mmap_size, tmp_mmap_size;
  holy_efi_memory_descriptor_t *desc;
  void *mem = NULL;

  size = size_pages << 12;

  mmap_size = find_mmap_size ();
  if (!mmap_size)
    return 0;

    /* Read the memory map temporarily, to find free space.  */
  mmap = holy_malloc (mmap_size);
  if (! mmap)
    return 0;

  tmp_mmap_size = mmap_size;
  if (holy_efi_get_memory_map (&tmp_mmap_size, mmap, 0, &desc_size, 0) <= 0)
    {
      holy_error (holy_ERR_IO, "cannot get memory map");
      goto fail;
    }

  mmap_end = NEXT_MEMORY_DESCRIPTOR (mmap, tmp_mmap_size);
  
  /* First, find free pages for the real mode code
     and the memory map buffer.  */
  for (desc = mmap;
       desc < mmap_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      holy_uint64_t start, end;
      holy_uint64_t aligned_start;

      if (desc->type != holy_EFI_CONVENTIONAL_MEMORY)
	continue;

      start = desc->physical_start;
      end = start + (desc->num_pages << 12);
      /* Align is a power of 2.  */
      aligned_start = (start + align - 1) & ~(align - 1);
      if (aligned_start + size > end)
	continue;
      if (aligned_start == nobase)
	aligned_start += align;
      if (aligned_start + size > end)
	continue;
      mem = holy_efi_allocate_pages (aligned_start, size_pages);
      if (! mem)
	{
	  holy_error (holy_ERR_OUT_OF_MEMORY, "cannot allocate memory");
	  goto fail;
	}
      break;
    }

  if (! mem)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, "cannot allocate memory");
      goto fail;
    }

  holy_free (mmap);
  return mem;

 fail:
  holy_free (mmap);
  free_pages ();
  return 0;
}

static void
set_boot_param_console (void)
{
  holy_efi_simple_text_output_interface_t *conout;
  holy_efi_uintn_t cols, rows;
  
  conout = holy_efi_system_table->con_out;
  if (conout->query_mode (conout, conout->mode->mode, &cols, &rows)
      != holy_EFI_SUCCESS)
    return;

  holy_dprintf ("linux",
		"Console info: cols=%lu rows=%lu x=%u y=%u\n",
		cols, rows,
		conout->mode->cursor_column, conout->mode->cursor_row);
  
  boot_param->console_info.num_cols = cols;
  boot_param->console_info.num_rows = rows;
  boot_param->console_info.orig_x = conout->mode->cursor_column;
  boot_param->console_info.orig_y = conout->mode->cursor_row;
}

static holy_err_t
holy_linux_boot (void)
{
  holy_efi_uintn_t mmap_size;
  holy_efi_uintn_t map_key;
  holy_efi_uintn_t desc_size;
  holy_efi_uint32_t desc_version;
  holy_efi_memory_descriptor_t *mmap_buf;
  holy_err_t err;

  /* FPSWA.  */
  query_fpswa ();
  boot_param->fpswa = (holy_uint64_t)fpswa;

  /* Initrd.  */
  boot_param->initrd_start = (holy_uint64_t)initrd_mem;
  boot_param->initrd_size = (holy_uint64_t)initrd_size;

  set_boot_param_console ();

  holy_dprintf ("linux", "Jump to %016lx\n", entry);

  /* MDT.
     Must be done after holy_machine_fini because map_key is used by
     exit_boot_services.  */
  mmap_size = find_mmap_size ();
  if (! mmap_size)
    return holy_errno;
  mmap_buf = holy_efi_allocate_pages (0, page_align (mmap_size) >> 12);
  if (! mmap_buf)
    return holy_error (holy_ERR_IO, "cannot allocate memory map");
  err = holy_efi_finish_boot_services (&mmap_size, mmap_buf, &map_key,
				       &desc_size, &desc_version);
  if (err)
    return err;

  boot_param->efi_memmap = (holy_uint64_t)mmap_buf;
  boot_param->efi_memmap_size = mmap_size;
  boot_param->efi_memdesc_size = desc_size;
  boot_param->efi_memdesc_version = desc_version;

  /* See you next boot.  */
  asm volatile ("mov r28=%1; br.sptk.few %0" :: "b"(entry),"r"(boot_param));
  
  /* Never reach here.  */
  return holy_ERR_NONE;
}

static holy_err_t
holy_linux_unload (void)
{
  free_pages ();
  holy_dl_unref (my_mod);
  loaded = 0;
  return holy_ERR_NONE;
}

static holy_err_t
holy_load_elf64 (holy_file_t file, void *buffer, const char *filename)
{
  Elf64_Ehdr *ehdr = (Elf64_Ehdr *) buffer;
  Elf64_Phdr *phdr;
  int i;
  holy_uint64_t low_addr;
  holy_uint64_t high_addr;
  holy_uint64_t align;
  holy_uint64_t reloc_offset;
  const char *relocate;

  if (ehdr->e_ident[EI_MAG0] != ELFMAG0
      || ehdr->e_ident[EI_MAG1] != ELFMAG1
      || ehdr->e_ident[EI_MAG2] != ELFMAG2
      || ehdr->e_ident[EI_MAG3] != ELFMAG3
      || ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
    return holy_error(holy_ERR_UNKNOWN_OS,
		      N_("invalid arch-independent ELF magic"));

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS64
      || ehdr->e_version != EV_CURRENT
      || ehdr->e_machine != EM_IA_64)
    return holy_error (holy_ERR_UNKNOWN_OS,
		       N_("invalid arch-dependent ELF magic"));

  if (ehdr->e_type != ET_EXEC)
    return holy_error (holy_ERR_UNKNOWN_OS,
		       N_("this ELF file is not of the right type"));

  /* FIXME: Should we support program headers at strange locations?  */
  if (ehdr->e_phoff + ehdr->e_phnum * ehdr->e_phentsize > holy_ELF_SEARCH)
    return holy_error (holy_ERR_BAD_OS, "program header at a too high offset");

  entry = ehdr->e_entry;

  /* Compute low, high and align addresses.  */
  low_addr = ~0UL;
  high_addr = 0;
  align = 0;
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      phdr = (Elf64_Phdr *) ((char *) buffer + ehdr->e_phoff
			     + i * ehdr->e_phentsize);
      if (phdr->p_type == PT_LOAD)
	{
	  if (phdr->p_paddr < low_addr)
	    low_addr = phdr->p_paddr;
	  if (phdr->p_paddr + phdr->p_memsz > high_addr)
	    high_addr = phdr->p_paddr + phdr->p_memsz;
	  if (phdr->p_align > align)
	    align = phdr->p_align;
	}
    }

  if (align < ALIGN_MIN)
    align = ALIGN_MIN;

  if (high_addr == 0)
    return holy_error (holy_ERR_BAD_OS, "no program entries");

  kernel_pages = page_align (high_addr - low_addr) >> 12;

  /* Undocumented on purpose.  */
  relocate = holy_env_get ("linux_relocate");
  if (!relocate || holy_strcmp (relocate, "force") != 0)
    {
      kernel_mem = holy_efi_allocate_pages (low_addr, kernel_pages);
      reloc_offset = 0;
    }
  /* Try to relocate.  */
  if (! kernel_mem && (!relocate || holy_strcmp (relocate, "off") != 0))
    {
      kernel_mem = allocate_pages (align, kernel_pages, low_addr);
      if (kernel_mem)
	{
	  reloc_offset = (holy_uint64_t)kernel_mem - low_addr;
	  holy_dprintf ("linux", "  Relocated at %p (offset=%016lx)\n",
			kernel_mem, reloc_offset);
	  entry += reloc_offset;
	}
    }
  if (! kernel_mem)
    return holy_error (holy_ERR_OUT_OF_MEMORY,
		       "cannot allocate memory for OS");

  /* Load every loadable segment in memory.  */
  for (i = 0; i < ehdr->e_phnum; i++)
    {
      phdr = (Elf64_Phdr *) ((char *) buffer + ehdr->e_phoff
			     + i * ehdr->e_phentsize);
      if (phdr->p_type == PT_LOAD)
        {
	  holy_dprintf ("linux", "  [paddr=%lx load=%lx memsz=%08lx "
			"off=%lx flags=%x]\n",
			phdr->p_paddr, phdr->p_paddr + reloc_offset,
			phdr->p_memsz, phdr->p_offset, phdr->p_flags);
	  
	  if (holy_file_seek (file, phdr->p_offset) == (holy_off_t)-1)
	    return holy_errno;

	  if (holy_file_read (file, (void *) (phdr->p_paddr + reloc_offset),
			      phdr->p_filesz)
              != (holy_ssize_t) phdr->p_filesz)
	    {
	      if (!holy_errno)
		holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			    filename);
	      return holy_errno;
	    }
	  
          if (phdr->p_filesz < phdr->p_memsz)
	    holy_memset
	      ((char *)(phdr->p_paddr + reloc_offset + phdr->p_filesz),
	       0, phdr->p_memsz - phdr->p_filesz);

	  /* Sync caches if necessary.  */
	  if (phdr->p_flags & PF_X)
	    holy_arch_sync_caches
	      ((void *)(phdr->p_paddr + reloc_offset), phdr->p_memsz);
        }
    }
  loaded = 1;
  return 0;
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  char buffer[holy_ELF_SEARCH];
  char *cmdline, *p;
  holy_ssize_t len;
  int i;

  holy_dl_ref (my_mod);

  holy_loader_unset ();
    
  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  file = holy_file_open (argv[0]);
  if (! file)
    goto fail;

  len = holy_file_read (file, buffer, sizeof (buffer));
  if (len < (holy_ssize_t) sizeof (Elf64_Ehdr))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    argv[0]);
      goto fail;
    }

  holy_dprintf ("linux", "Loading linux: %s\n", argv[0]);

  if (holy_load_elf64 (file, buffer, argv[0]))
    goto fail;

  len = sizeof("BOOT_IMAGE=") + 8;
  for (i = 0; i < argc; i++)
    len += holy_strlen (argv[i]) + 1;
  len += sizeof (struct ia64_boot_param) + 512; /* Room for extensions.  */
  boot_param_pages = page_align (len) >> 12;
  boot_param = holy_efi_allocate_pages (0, boot_param_pages);
  if (boot_param == 0)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY,
		  "cannot allocate memory for bootparams");
      goto fail;
    }

  holy_memset (boot_param, 0, len);
  cmdline = ((char *)(boot_param + 1)) + 256;

  /* Build cmdline.  */
  p = holy_stpcpy (cmdline, "BOOT_IMAGE");
  for (i = 0; i < argc; i++)
    {
      *p++ = ' ';
      p = holy_stpcpy (p, argv[i]);
    }
  cmdline[10] = '=';
  
  boot_param->command_line = (holy_uint64_t) cmdline;
  boot_param->efi_systab = (holy_uint64_t) holy_efi_system_table;

  holy_errno = holy_ERR_NONE;

  holy_loader_set (holy_linux_boot, holy_linux_unload, 0);

 fail:
  if (file)
    holy_file_close (file);

  if (holy_errno != holy_ERR_NONE)
    {
      holy_efi_free_pages ((holy_efi_physical_address_t) boot_param,
			   boot_param_pages);
      holy_dl_unref (my_mod);
    }
  return holy_errno;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  struct holy_linux_initrd_context initrd_ctx = { 0, 0, 0 };

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }
  
  if (! loaded)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("you need to load the kernel first"));
      goto fail;
    }

  if (holy_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  initrd_size = holy_get_initrd_size (&initrd_ctx);
  holy_dprintf ("linux", "Loading initrd\n");

  initrd_pages = (page_align (initrd_size) >> 12);
  initrd_mem = holy_efi_allocate_pages (0, initrd_pages);
  if (! initrd_mem)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, "cannot allocate pages");
      goto fail;
    }
  
  holy_dprintf ("linux", "[addr=0x%lx, size=0x%lx]\n",
		(holy_uint64_t) initrd_mem, initrd_size);

  if (holy_initrd_load (&initrd_ctx, argv, initrd_mem))
    goto fail;
 fail:
  holy_initrd_close (&initrd_ctx);
  return holy_errno;
}

static holy_err_t
holy_cmd_fpswa (holy_command_t cmd __attribute__ ((unused)),
		int argc __attribute__((unused)),
		char *argv[] __attribute__((unused)))
{
  query_fpswa ();
  if (fpswa == NULL)
    holy_puts_ (N_("No FPSWA found"));
  else
    holy_printf (_("FPSWA revision: %x\n"), fpswa->revision);
  return holy_ERR_NONE;
}

static holy_command_t cmd_linux, cmd_initrd, cmd_fpswa;

holy_MOD_INIT(linux)
{
  cmd_linux = holy_register_command ("linux", holy_cmd_linux,
				     N_("FILE [ARGS...]"), N_("Load Linux."));
  
  cmd_initrd = holy_register_command ("initrd", holy_cmd_initrd,
				      N_("FILE"), N_("Load initrd."));

  cmd_fpswa = holy_register_command ("fpswa", holy_cmd_fpswa,
				     "", N_("Display FPSWA version."));

  my_mod = mod;
}

holy_MOD_FINI(linux)
{
  holy_unregister_command (cmd_linux);
  holy_unregister_command (cmd_initrd);
  holy_unregister_command (cmd_fpswa);
}
