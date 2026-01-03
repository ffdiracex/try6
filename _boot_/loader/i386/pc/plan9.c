/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/partition.h>
#include <holy/msdos_partition.h>
#include <holy/scsi.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/video.h>
#include <holy/mm.h>
#include <holy/cpu/relocator.h>
#include <holy/extcmd.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;
static struct holy_relocator *rel;
static holy_uint32_t eip = 0xffffffff;

#define holy_PLAN9_TARGET            0x100000
#define holy_PLAN9_ALIGN             4096
#define holy_PLAN9_CONFIG_ADDR       0x001200
#define holy_PLAN9_CONFIG_PATH_SIZE  0x000040
#define holy_PLAN9_CONFIG_MAGIC      "ZORT 0\r\n"

static const struct holy_arg_option options[] =
  {
    {"map", 'm', holy_ARG_OPTION_REPEATABLE,
     /* TRANSLATORS: it's about guessing which holy disk
	is which Plan9 disk. If your language has no
	word "mapping" you can use another word which
	means that the holyDEVICE and PLAN9DEVICE are
	actually the same device, just named differently
	in OS and holy.  */
     N_("Override guessed mapping of Plan9 devices."), 
     N_("holyDEVICE=PLAN9DEVICE"),
     ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

struct holy_plan9_header
{
  holy_uint32_t magic;
#define holy_PLAN9_MAGIC 0x1eb
  holy_uint32_t text_size;
  holy_uint32_t data_size;
  holy_uint32_t bss_size;
  holy_uint32_t sectiona;
  holy_uint32_t entry_addr;
  holy_uint32_t zero;
  holy_uint32_t sectionb;
};

static holy_err_t
holy_plan9_boot (void)
{
  struct holy_relocator32_state state = {
    .eax = 0,
    .eip = eip,
    .ebx = 0,
    .ecx = 0,
    .edx = 0,
    .edi = 0,
    .esp = 0,
    .ebp = 0,
    .esi = 0
  };
  holy_video_set_mode ("text", 0, 0);

  return holy_relocator32_boot (rel, state, 0);
}

static holy_err_t
holy_plan9_unload (void)
{
  holy_relocator_unload (rel);
  rel = NULL;
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

/* Context for holy_cmd_plan9.  */
struct holy_cmd_plan9_ctx
{
  holy_extcmd_context_t ctxt;
  holy_file_t file;
  char *pmap;
  holy_size_t pmapalloc;
  holy_size_t pmapptr;
  int noslash;
  int prefixescnt[5];
  char *bootdisk, *bootpart;
};

static const char prefixes[5][10] = {
  "dos", "plan9", "ntfs", "linux", "linuxswap"
};

#include <holy/err.h>

static inline holy_err_t
holy_extend_alloc (holy_size_t sz, holy_size_t *allocated, char **ptr)
{
  void *n;
  if (sz < *allocated)
    return holy_ERR_NONE;

  *allocated = 2 * sz;
  n = holy_realloc (*ptr, *allocated);
  if (!n)
    return holy_errno;
  *ptr = n;
  return holy_ERR_NONE;
}

/* Helper for holy_cmd_plan9.  */
static int
fill_partition (holy_disk_t disk, const holy_partition_t partition, void *data)
{
  struct holy_cmd_plan9_ctx *fill_ctx = data;
  int file_disk = 0;
  int pstart, pend;

  if (!fill_ctx->noslash)
    {
      if (holy_extend_alloc (fill_ctx->pmapptr + 1, &fill_ctx->pmapalloc,
			     &fill_ctx->pmap))
	return 1;
      fill_ctx->pmap[fill_ctx->pmapptr++] = '/';
    }
  fill_ctx->noslash = 0;

  file_disk = fill_ctx->file->device->disk
    && disk->id == fill_ctx->file->device->disk->id
    && disk->dev->id == fill_ctx->file->device->disk->dev->id;

  pstart = fill_ctx->pmapptr;
  if (holy_strcmp (partition->partmap->name, "plan") == 0)
    {
      unsigned ptr = partition->index + sizeof ("part ") - 1;
      holy_err_t err;
      disk->partition = partition->parent;
      do
	{
	  if (holy_extend_alloc (fill_ctx->pmapptr + 1, &fill_ctx->pmapalloc,
				 &fill_ctx->pmap))
	    return 1;
	  err = holy_disk_read (disk, 1, ptr, 1,
				fill_ctx->pmap + fill_ctx->pmapptr);
	  if (err)
	    {
	      disk->partition = 0;
	      return err;
	    }
	  ptr++;
	  fill_ctx->pmapptr++;
	}
      while (holy_isalpha (fill_ctx->pmap[fill_ctx->pmapptr - 1])
	     || holy_isdigit (fill_ctx->pmap[fill_ctx->pmapptr - 1]));
      fill_ctx->pmapptr--;
    }
  else
    {
      char name[50];
      int c = 0;
      if (holy_strcmp (partition->partmap->name, "msdos") == 0)
	{
	  switch (partition->msdostype)
	    {
	    case holy_PC_PARTITION_TYPE_PLAN9:
	      c = 1;
	      break;
	    case holy_PC_PARTITION_TYPE_NTFS:
	      c = 2;
	      break;
	    case holy_PC_PARTITION_TYPE_MINIX:
	    case holy_PC_PARTITION_TYPE_LINUX_MINIX:
	    case holy_PC_PARTITION_TYPE_EXT2FS:
	      c = 3;
	      break;
	    case holy_PC_PARTITION_TYPE_LINUX_SWAP:
	      c = 4;
	      break;
	    }
	}

      if (fill_ctx->prefixescnt[c] == 0)
	holy_strcpy (name, prefixes[c]);
      else
	holy_snprintf (name, sizeof (name), "%s.%d", prefixes[c],
		       fill_ctx->prefixescnt[c]);
      fill_ctx->prefixescnt[c]++;
      if (holy_extend_alloc (fill_ctx->pmapptr + holy_strlen (name) + 1,
			     &fill_ctx->pmapalloc, &fill_ctx->pmap))
	return 1;
      holy_strcpy (fill_ctx->pmap + fill_ctx->pmapptr, name);
      fill_ctx->pmapptr += holy_strlen (name);
    }
  pend = fill_ctx->pmapptr;
  if (holy_extend_alloc (fill_ctx->pmapptr + 2 + 25 + 5 + 25,
			 &fill_ctx->pmapalloc, &fill_ctx->pmap))
    return 1;
  fill_ctx->pmap[fill_ctx->pmapptr++] = ' ';
  holy_snprintf (fill_ctx->pmap + fill_ctx->pmapptr, 25 + 5 + 25,
		 "%" PRIuholy_UINT64_T " %" PRIuholy_UINT64_T,
		 holy_partition_get_start (partition),
		 holy_partition_get_start (partition)
		 + holy_partition_get_len (partition));
  if (file_disk && holy_partition_get_start (partition)
      == holy_partition_get_start (fill_ctx->file->device->disk->partition)
      && holy_partition_get_len (partition)
      == holy_partition_get_len (fill_ctx->file->device->disk->partition))
    {
      holy_free (fill_ctx->bootpart);
      fill_ctx->bootpart = holy_strndup (fill_ctx->pmap + pstart,
					 pend - pstart);
    }

  fill_ctx->pmapptr += holy_strlen (fill_ctx->pmap + fill_ctx->pmapptr);
  return 0;
}

/* Helper for holy_cmd_plan9.  */
static int
fill_disk (const char *name, void *data)
{
  struct holy_cmd_plan9_ctx *fill_ctx = data;
  holy_device_t dev;
  char *plan9name = NULL;
  unsigned i;
  int file_disk = 0;

  dev = holy_device_open (name);
  if (!dev)
    {
      holy_print_error ();
      return 0;
    }
  if (!dev->disk)
    {
      holy_device_close (dev);
      return 0;
    }
  file_disk = fill_ctx->file->device->disk
    && dev->disk->id == fill_ctx->file->device->disk->id
    && dev->disk->dev->id == fill_ctx->file->device->disk->dev->id;
  for (i = 0;
       fill_ctx->ctxt->state[0].args && fill_ctx->ctxt->state[0].args[i]; i++)
    if (holy_strncmp (name, fill_ctx->ctxt->state[0].args[i],
		      holy_strlen (name)) == 0
	&& fill_ctx->ctxt->state[0].args[i][holy_strlen (name)] == '=')
      break;
  if (fill_ctx->ctxt->state[0].args && fill_ctx->ctxt->state[0].args[i])
    plan9name = holy_strdup (fill_ctx->ctxt->state[0].args[i]
			     + holy_strlen (name) + 1);
  else
    switch (dev->disk->dev->id)
      {
      case holy_DISK_DEVICE_BIOSDISK_ID:
	if (dev->disk->id & 0x80)
	  plan9name = holy_xasprintf ("sdB%u",
				      (unsigned) (dev->disk->id & 0x7f));
	else
	  plan9name = holy_xasprintf ("fd%u",
				      (unsigned) (dev->disk->id & 0x7f));
	break;
	/* Shouldn't happen as Plan9 doesn't work on these platforms.  */
      case holy_DISK_DEVICE_OFDISK_ID:
      case holy_DISK_DEVICE_EFIDISK_ID:

	/* Plan9 doesn't see those.  */
      default:

	/* Not sure how to handle those. */
      case holy_DISK_DEVICE_NAND_ID:
	if (!file_disk)
	  {
	    holy_device_close (dev);
	    return 0;
	  }
	
	/* if it's the disk the kernel is loaded from we need to name
	   it nevertheless.  */
	plan9name = holy_strdup ("sdZ0");
	break;

      case holy_DISK_DEVICE_ATA_ID:
	{
	  unsigned unit;
	  if (holy_strlen (dev->disk->name) < sizeof ("ata0") - 1)
	    unit = 0;
	  else
	    unit = holy_strtoul (dev->disk->name + sizeof ("ata0") - 1, 0, 0);
	  plan9name = holy_xasprintf ("sd%c%d", 'C' + unit / 2, unit % 2);
	}
	break;
      case holy_DISK_DEVICE_SCSI_ID:
	if (((dev->disk->id >> holy_SCSI_ID_SUBSYSTEM_SHIFT) & 0xff)
	    == holy_SCSI_SUBSYSTEM_PATA)
	  {
	    unsigned unit;
	    if (holy_strlen (dev->disk->name) < sizeof ("ata0") - 1)
	      unit = 0;
	    else
	      unit = holy_strtoul (dev->disk->name + sizeof ("ata0") - 1,
				   0, 0);
	    plan9name = holy_xasprintf ("sd%c%d", 'C' + unit / 2, unit % 2);
	    break;
	  }
	
	/* FIXME: how does Plan9 number controllers?
	   We probably need save the SCSI devices and sort them  */
	plan9name
	  = holy_xasprintf ("sd0%u", (unsigned)
			    ((dev->disk->id >> holy_SCSI_ID_BUS_SHIFT)
			     & 0xf));
	break;
      }
  if (!plan9name)
    {
      holy_print_error ();
      holy_device_close (dev);
      return 0;
    }
  if (holy_extend_alloc (fill_ctx->pmapptr + holy_strlen (plan9name)
			 + sizeof ("part="), &fill_ctx->pmapalloc,
			 &fill_ctx->pmap))
    {
      holy_free (plan9name);
      holy_device_close (dev);
      return 1;
    }
  holy_strcpy (fill_ctx->pmap + fill_ctx->pmapptr, plan9name);
  fill_ctx->pmapptr += holy_strlen (plan9name);
  if (!file_disk)
    holy_free (plan9name);
  else
    {
      holy_free (fill_ctx->bootdisk);
      fill_ctx->bootdisk = plan9name;
    }
  holy_strcpy (fill_ctx->pmap + fill_ctx->pmapptr, "part=");
  fill_ctx->pmapptr += sizeof ("part=") - 1;

  fill_ctx->noslash = 1;
  holy_memset (fill_ctx->prefixescnt, 0, sizeof (fill_ctx->prefixescnt));
  if (holy_partition_iterate (dev->disk, fill_partition, fill_ctx))
    {
      holy_device_close (dev);
      return 1;
    }
  if (holy_extend_alloc (fill_ctx->pmapptr + 1, &fill_ctx->pmapalloc,
			 &fill_ctx->pmap))
    {
      holy_device_close (dev);
      return 1;
    }
  fill_ctx->pmap[fill_ctx->pmapptr++] = '\n';

  holy_device_close (dev);
  return 0;
}

static holy_err_t
holy_cmd_plan9 (holy_extcmd_context_t ctxt, int argc, char *argv[])
{
  struct holy_cmd_plan9_ctx fill_ctx = {
    .ctxt = ctxt,
    .file = 0,
    .pmap = NULL,
    .pmapalloc = 256,
    .pmapptr = 0,
    .noslash = 1,
    .bootdisk = NULL,
    .bootpart = NULL
  };
  void *mem;
  holy_size_t memsize, padsize;
  struct holy_plan9_header hdr;
  char *config, *configptr;
  holy_size_t configsize;
  char *bootpath = NULL;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_dl_ref (my_mod);

  rel = holy_relocator_new ();
  if (!rel)
    goto fail;

  fill_ctx.file = holy_file_open (argv[0]);
  if (! fill_ctx.file)
    goto fail;

  fill_ctx.pmap = holy_malloc (fill_ctx.pmapalloc);
  if (!fill_ctx.pmap)
    goto fail;

  if (holy_disk_dev_iterate (fill_disk, &fill_ctx))
    goto fail;

  if (holy_extend_alloc (fill_ctx.pmapptr + 1, &fill_ctx.pmapalloc,
			 &fill_ctx.pmap))
    goto fail;
  fill_ctx.pmap[fill_ctx.pmapptr] = 0;

  {
    char *file_name = holy_strchr (argv[0], ')');
    if (file_name)
      file_name++;
    else
      file_name = argv[0];
    if (*file_name)
      file_name++;

    if (fill_ctx.bootpart)
      bootpath = holy_xasprintf ("%s!%s!%s", fill_ctx.bootdisk,
				 fill_ctx.bootpart, file_name);
    else
      bootpath = holy_xasprintf ("%s!%s", fill_ctx.bootdisk, file_name);
    holy_free (fill_ctx.bootdisk);
    holy_free (fill_ctx.bootpart);
  }
  if (!bootpath)
    goto fail;

  if (holy_file_read (fill_ctx.file, &hdr,
		      sizeof (hdr)) != (holy_ssize_t) sizeof (hdr))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    argv[0]);
      goto fail;
    }

  if (holy_be_to_cpu32 (hdr.magic) != holy_PLAN9_MAGIC
      || hdr.zero)
    {
      holy_error (holy_ERR_BAD_OS, "unsupported Plan9");
      goto fail;
    }

  memsize = ALIGN_UP (holy_be_to_cpu32 (hdr.text_size) + sizeof (hdr),
		       holy_PLAN9_ALIGN);
  memsize += ALIGN_UP (holy_be_to_cpu32 (hdr.data_size), holy_PLAN9_ALIGN);
  memsize += ALIGN_UP(holy_be_to_cpu32 (hdr.bss_size), holy_PLAN9_ALIGN);
  eip = holy_be_to_cpu32 (hdr.entry_addr) & 0xfffffff;

  /* path */
  configsize = holy_PLAN9_CONFIG_PATH_SIZE;
  /* magic */
  configsize += sizeof (holy_PLAN9_CONFIG_MAGIC) - 1;
  {
    int i;
    for (i = 1; i < argc; i++)
      configsize += holy_strlen (argv[i]) + 1;
  }
  configsize += (sizeof ("bootfile=") - 1) + holy_strlen (bootpath) + 1;
  configsize += fill_ctx.pmapptr;
  /* Terminating \0.  */
  configsize++;

  {
    holy_relocator_chunk_t ch;
    holy_err_t err;
    err = holy_relocator_alloc_chunk_addr (rel, &ch, holy_PLAN9_CONFIG_ADDR,
					   configsize);
    if (err)
      goto fail;
    config = get_virtual_current_address (ch);
  }

  holy_memset (config, 0, holy_PLAN9_CONFIG_PATH_SIZE);
  holy_strncpy (config, bootpath, holy_PLAN9_CONFIG_PATH_SIZE - 1);

  configptr = config + holy_PLAN9_CONFIG_PATH_SIZE;
  holy_memcpy (configptr, holy_PLAN9_CONFIG_MAGIC,
	       sizeof (holy_PLAN9_CONFIG_MAGIC) - 1);
  configptr += sizeof (holy_PLAN9_CONFIG_MAGIC) - 1;
  configptr = holy_stpcpy (configptr, "bootfile=");
  configptr = holy_stpcpy (configptr, bootpath);
  *configptr++ = '\n';
  {
    int i;
    for (i = 1; i < argc; i++)
      {
	configptr = holy_stpcpy (configptr, argv[i]);
	*configptr++ = '\n';
      }
  }
  configptr = holy_stpcpy (configptr, fill_ctx.pmap);

  {
    holy_relocator_chunk_t ch;
    holy_err_t err;

    err = holy_relocator_alloc_chunk_addr (rel, &ch, holy_PLAN9_TARGET,
					   memsize);
    if (err)
      goto fail;
    mem = get_virtual_current_address (ch);
  }

  {
    holy_uint8_t *ptr;
    ptr = mem;
    holy_memcpy (ptr, &hdr, sizeof (hdr));
    ptr += sizeof (hdr);

    if (holy_file_read (fill_ctx.file, ptr, holy_be_to_cpu32 (hdr.text_size))
	!= (holy_ssize_t) holy_be_to_cpu32 (hdr.text_size))
      {
	if (!holy_errno)
	  holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		      argv[0]);
	goto fail;
      }
    ptr += holy_be_to_cpu32 (hdr.text_size);
    padsize = ALIGN_UP (holy_be_to_cpu32 (hdr.text_size) + sizeof (hdr),
			holy_PLAN9_ALIGN) - holy_be_to_cpu32 (hdr.text_size)
      - sizeof (hdr);

    holy_memset (ptr, 0, padsize);
    ptr += padsize;

    if (holy_file_read (fill_ctx.file, ptr, holy_be_to_cpu32 (hdr.data_size))
	!= (holy_ssize_t) holy_be_to_cpu32 (hdr.data_size))
      {
	if (!holy_errno)
	  holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		      argv[0]);
	goto fail;
      }
    ptr += holy_be_to_cpu32 (hdr.data_size);
    padsize = ALIGN_UP (holy_be_to_cpu32 (hdr.data_size), holy_PLAN9_ALIGN)
      - holy_be_to_cpu32 (hdr.data_size);

    holy_memset (ptr, 0, padsize);
    ptr += padsize;
    holy_memset (ptr, 0, ALIGN_UP(holy_be_to_cpu32 (hdr.bss_size),
				  holy_PLAN9_ALIGN));
  }
  holy_loader_set (holy_plan9_boot, holy_plan9_unload, 1);
  return holy_ERR_NONE;

 fail:
  holy_free (fill_ctx.pmap);

  if (fill_ctx.file)
    holy_file_close (fill_ctx.file);

  holy_plan9_unload ();

  return holy_errno;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(plan9)
{
  cmd = holy_register_extcmd ("plan9", holy_cmd_plan9,
			      holy_COMMAND_OPTIONS_AT_START,
			      N_("KERNEL ARGS"), N_("Load Plan9 kernel."),
			      options);
  my_mod = mod;
}

holy_MOD_FINI(plan9)
{
  holy_unregister_extcmd (cmd);
}
