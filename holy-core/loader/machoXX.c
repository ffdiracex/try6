/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/i18n.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))

static int
SUFFIX (holy_macho_contains_macho) (holy_macho_t macho)
{
  return macho->offsetXX != -1;
}

void
SUFFIX (holy_macho_parse) (holy_macho_t macho, const char *filename)
{
  union {
    struct holy_macho_lzss_header lzss;
    holy_macho_header_t macho;
  } head;

  /* Is there any candidate at all? */
  if (macho->offsetXX == -1)
    return;

  /* Read header and check magic.  */
  if (holy_file_seek (macho->file, macho->offsetXX) == (holy_off_t) -1
      || holy_file_read (macho->file, &head, sizeof (head))
      != sizeof (head))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);
      macho->offsetXX = -1;
      return;
    }
  if (holy_memcmp (head.lzss.magic, holy_MACHO_LZSS_MAGIC,
		   sizeof (head.lzss.magic)) == 0)
    {
      macho->compressed_sizeXX = holy_be_to_cpu32 (head.lzss.compressed_size);
      macho->uncompressed_sizeXX
	= holy_be_to_cpu32 (head.lzss.uncompressed_size);
      if (macho->uncompressed_sizeXX < sizeof (head.macho))
	{
	  holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		      filename);
	  macho->offsetXX = -1;
	  return;
	}
      /* Skip header check.  */
      macho->compressedXX = 1;
      return;
    }

  if (head.macho.magic != holy_MACHO_MAGIC)
    {
      holy_error (holy_ERR_BAD_OS, "invalid Mach-O  header");
      macho->offsetXX = -1;
      return;
    }

  /* Read commands. */
  macho->ncmdsXX = head.macho.ncmds;
  macho->cmdsizeXX = head.macho.sizeofcmds;
  macho->cmdsXX = holy_malloc (macho->cmdsizeXX);
  if (! macho->cmdsXX)
    return;
  if (holy_file_seek (macho->file, macho->offsetXX
		      + sizeof (holy_macho_header_t)) == (holy_off_t) -1
      || holy_file_read (macho->file, macho->cmdsXX,
		      (holy_size_t) macho->cmdsizeXX)
      != (holy_ssize_t) macho->cmdsizeXX)
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);
      macho->offsetXX = -1;
    }
}

typedef int (*holy_macho_iter_hook_t)
(holy_macho_t , struct holy_macho_cmd *,
	       void *);

static holy_err_t
holy_macho_cmds_iterate (holy_macho_t macho,
			 holy_macho_iter_hook_t hook,
			 void *hook_arg,
			 const char *filename)
{
  holy_uint8_t *hdrs;
  int i;

  if (macho->compressedXX && !macho->uncompressedXX)
    {
      holy_uint8_t *tmp;
      holy_macho_header_t *head;
      macho->uncompressedXX = holy_malloc (macho->uncompressed_sizeXX);
      if (!macho->uncompressedXX)
	return holy_errno;
      tmp = holy_malloc (macho->compressed_sizeXX);
      if (!tmp)
	{
	  holy_free (macho->uncompressedXX);
	  macho->uncompressedXX = 0;
	  return holy_errno;
	}
      if (holy_file_seek (macho->file, macho->offsetXX
			  + holy_MACHO_LZSS_OFFSET) == (holy_off_t) -1
	  || holy_file_read (macho->file, tmp,
			     (holy_size_t) macho->compressed_sizeXX)
	  != (holy_ssize_t) macho->compressed_sizeXX)
	{
	  if (!holy_errno)
	    holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			filename);
	  holy_free (tmp);
	  holy_free (macho->uncompressedXX);
	  macho->uncompressedXX = 0;
	  macho->offsetXX = -1;
	  return holy_errno;
	}
      if (holy_decompress_lzss (macho->uncompressedXX,
				macho->uncompressedXX
				+ macho->uncompressed_sizeXX,
				tmp, tmp + macho->compressed_sizeXX)
	  != macho->uncompressed_sizeXX)
	{
	  if (!holy_errno)
	    holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			filename);
	  holy_free (tmp);
	  holy_free (macho->uncompressedXX);
	  macho->uncompressedXX = 0;
	  macho->offsetXX = -1;
	  return holy_errno;
	}
      holy_free (tmp);
      head = (holy_macho_header_t *) macho->uncompressedXX;
      macho->ncmdsXX = head->ncmds;
      macho->cmdsizeXX = head->sizeofcmds;
      macho->cmdsXX = macho->uncompressedXX + sizeof (holy_macho_header_t);
      if (sizeof (holy_macho_header_t) + macho->cmdsizeXX
	  >= macho->uncompressed_sizeXX)
	{
	  holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		      filename);
	  holy_free (macho->uncompressedXX);
	  macho->uncompressedXX = 0;
	  macho->offsetXX = -1;
	  return holy_errno;
	}
    }

  if (! macho->cmdsXX)
    return holy_error (holy_ERR_BAD_OS, "couldn't find Mach-O commands");
  hdrs = macho->cmdsXX;
  for (i = 0; i < macho->ncmdsXX; i++)
    {
      struct holy_macho_cmd *hdr = (struct holy_macho_cmd *) hdrs;
      if (hook (macho, hdr, hook_arg))
	break;
      hdrs += hdr->cmdsize;
    }

  return holy_errno;
}

holy_size_t
SUFFIX (holy_macho_filesize) (holy_macho_t macho)
{
  if (SUFFIX (holy_macho_contains_macho) (macho))
    return macho->endXX - macho->offsetXX;
  return 0;
}

holy_err_t
SUFFIX (holy_macho_readfile) (holy_macho_t macho,
			      const char *filename,
			      void *dest)
{
  holy_ssize_t read;
  if (! SUFFIX (holy_macho_contains_macho) (macho))
    return holy_error (holy_ERR_BAD_OS,
		       "couldn't read architecture-specific part");

  if (holy_file_seek (macho->file, macho->offsetXX) == (holy_off_t) -1)
    return holy_errno;

  read = holy_file_read (macho->file, dest,
			 macho->endXX - macho->offsetXX);
  if (read != (holy_ssize_t) (macho->endXX - macho->offsetXX))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);
      return holy_errno;
    }
  return holy_ERR_NONE;
}

struct calcsize_ctx
{
  int flags;
  int nr_phdrs;
  holy_macho_addr_t *segments_start;
  holy_macho_addr_t *segments_end;
};

/* Run through the program headers to calculate the total memory size we
   should claim.  */
static int
calcsize (holy_macho_t _macho __attribute__ ((unused)),
	  struct holy_macho_cmd *hdr0,
	  void *_arg)
{
  holy_macho_segment_t *hdr = (holy_macho_segment_t *) hdr0;
  struct calcsize_ctx *ctx = _arg;
  if (hdr->cmd != holy_MACHO_CMD_SEGMENT)
    return 0;

  if (! hdr->vmsize)
    return 0;

  if (! hdr->filesize && (ctx->flags & holy_MACHO_NOBSS))
    return 0;

  ctx->nr_phdrs++;
  if (hdr->vmaddr < *ctx->segments_start)
    *ctx->segments_start = hdr->vmaddr;
  if (hdr->vmaddr + hdr->vmsize > *ctx->segments_end)
    *ctx->segments_end = hdr->vmaddr + hdr->vmsize;
  return 0;
}

/* Calculate the amount of memory spanned by the segments. */
holy_err_t
SUFFIX (holy_macho_size) (holy_macho_t macho, holy_macho_addr_t *segments_start,
			  holy_macho_addr_t *segments_end, int flags,
			  const char *filename)
{
  struct calcsize_ctx ctx = {
    .flags = flags,
    .nr_phdrs = 0,
    .segments_start = segments_start,
    .segments_end = segments_end,
  };

  *segments_start = (holy_macho_addr_t) -1;
  *segments_end = 0;

  holy_macho_cmds_iterate (macho, calcsize, &ctx, filename);

  if (ctx.nr_phdrs == 0)
    return holy_error (holy_ERR_BAD_OS, "no program headers present");

  if (*segments_end < *segments_start)
    /* Very bad addresses.  */
    return holy_error (holy_ERR_BAD_OS, "bad program header load addresses");

  return holy_ERR_NONE;
}

struct do_load_ctx
{
  int flags;
  char *offset;
  const char *filename;
  int *darwin_version;
};

static int
do_load(holy_macho_t _macho,
	struct holy_macho_cmd *hdr0,
	void *_arg)
{
  holy_macho_segment_t *hdr = (holy_macho_segment_t *) hdr0;
  struct do_load_ctx *ctx = _arg;

  if (hdr->cmd != holy_MACHO_CMD_SEGMENT)
    return 0;

  if (! hdr->filesize && (ctx->flags & holy_MACHO_NOBSS))
    return 0;
  if (! hdr->vmsize)
    return 0;

  if (hdr->filesize)
    {
      holy_ssize_t read, toread = min (hdr->filesize, hdr->vmsize);
      if (_macho->uncompressedXX)
	{
	  if (hdr->fileoff + (holy_size_t) toread
	      > _macho->uncompressed_sizeXX)
	    read = -1;
	  else
	    {
	      read = toread;
	      holy_memcpy (ctx->offset + hdr->vmaddr,
			   _macho->uncompressedXX + hdr->fileoff, read);
	    }
	}
      else
	{
	  if (holy_file_seek (_macho->file, hdr->fileoff
			      + _macho->offsetXX) == (holy_off_t) -1)
	    return 1;
	  read = holy_file_read (_macho->file, ctx->offset + hdr->vmaddr,
				 toread);
	}

      if (read != toread)
	{
	  /* XXX How can we free memory from `load_hook'? */
	  if (!holy_errno)
	    holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			ctx->filename);

	  return 1;
	}
      if (ctx->darwin_version)
	{
	  const char *ptr = ctx->offset + hdr->vmaddr;
	  const char *end = ptr + min (hdr->filesize, hdr->vmsize)
	    - (sizeof ("Darwin Kernel Version ") - 1);
	  for (; ptr < end; ptr++)
	    if (holy_memcmp (ptr, "Darwin Kernel Version ",
			     sizeof ("Darwin Kernel Version ") - 1) == 0)
	      {
		ptr += sizeof ("Darwin Kernel Version ") - 1;
		*ctx->darwin_version = 0;
		end += (sizeof ("Darwin Kernel Version ") - 1);
		while (ptr < end && holy_isdigit (*ptr))
		  *ctx->darwin_version = (*ptr++ - '0') + *ctx->darwin_version * 10;
		break;
	      }
	}
    }

  if (hdr->filesize < hdr->vmsize)
    holy_memset (ctx->offset + hdr->vmaddr + hdr->filesize,
		 0, hdr->vmsize - hdr->filesize);
  return 0;
}

/* Load every loadable segment into memory specified by `_load_hook'.  */
holy_err_t
SUFFIX (holy_macho_load) (holy_macho_t macho, const char *filename,
			  char *offset, int flags, int *darwin_version)
{
  struct do_load_ctx ctx = {
    .flags = flags,
    .offset = offset,
    .filename = filename,
    .darwin_version = darwin_version
  };

  if (darwin_version)
    *darwin_version = 0;

  holy_macho_cmds_iterate (macho, do_load, &ctx, filename);

  return holy_errno;
}

static int
find_entry_point (holy_macho_t _macho __attribute__ ((unused)),
			    struct holy_macho_cmd *hdr,
			    void *_arg)
{
  holy_macho_addr_t *entry_point = _arg;
  if (hdr->cmd == holy_MACHO_CMD_THREAD)
    *entry_point = ((holy_macho_thread_t *) hdr)->entry_point;
  return 0;
}

holy_macho_addr_t
SUFFIX (holy_macho_get_entry_point) (holy_macho_t macho, const char *filename)
{
  holy_macho_addr_t entry_point = 0;
  holy_macho_cmds_iterate (macho, find_entry_point, &entry_point, filename);
  return entry_point;
}
