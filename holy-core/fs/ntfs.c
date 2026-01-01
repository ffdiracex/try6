/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_fshelp_node holy_ntfs_file

#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/fshelp.h>
#include <holy/ntfs.h>
#include <holy/charset.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;

#define holy_fshelp_node holy_ntfs_file

static inline holy_uint16_t
u16at (void *ptr, holy_size_t ofs)
{
  return holy_le_to_cpu16 (holy_get_unaligned16 ((char *) ptr + ofs));
}

static inline holy_uint32_t
u32at (void *ptr, holy_size_t ofs)
{
  return holy_le_to_cpu32 (holy_get_unaligned32 ((char *) ptr + ofs));
}

static inline holy_uint64_t
u64at (void *ptr, holy_size_t ofs)
{
  return holy_le_to_cpu64 (holy_get_unaligned64 ((char *) ptr + ofs));
}

holy_ntfscomp_func_t holy_ntfscomp_func;

static holy_err_t
fixup (holy_uint8_t *buf, holy_size_t len, const holy_uint8_t *magic)
{
  holy_uint16_t ss;
  holy_uint8_t *pu;
  holy_uint16_t us;

  COMPILE_TIME_ASSERT ((1 << holy_NTFS_BLK_SHR) == holy_DISK_SECTOR_SIZE);

  if (holy_memcmp (buf, magic, 4))
    return holy_error (holy_ERR_BAD_FS, "%s label not found", magic);

  ss = u16at (buf, 6) - 1;
  if (ss != len)
    return holy_error (holy_ERR_BAD_FS, "size not match");
  pu = buf + u16at (buf, 4);
  us = u16at (pu, 0);
  buf -= 2;
  while (ss > 0)
    {
      buf += holy_DISK_SECTOR_SIZE;
      pu += 2;
      if (u16at (buf, 0) != us)
	return holy_error (holy_ERR_BAD_FS, "fixup signature not match");
      buf[0] = pu[0];
      buf[1] = pu[1];
      ss--;
    }

  return 0;
}

static holy_err_t read_mft (struct holy_ntfs_data *data, holy_uint8_t *buf,
			    holy_uint64_t mftno);
static holy_err_t read_attr (struct holy_ntfs_attr *at, holy_uint8_t *dest,
			     holy_disk_addr_t ofs, holy_size_t len,
			     int cached,
			     holy_disk_read_hook_t read_hook,
			     void *read_hook_data);

static holy_err_t read_data (struct holy_ntfs_attr *at, holy_uint8_t *pa,
			     holy_uint8_t *dest,
			     holy_disk_addr_t ofs, holy_size_t len,
			     int cached,
			     holy_disk_read_hook_t read_hook,
			     void *read_hook_data);

static void
init_attr (struct holy_ntfs_attr *at, struct holy_ntfs_file *mft)
{
  at->mft = mft;
  at->flags = (mft == &mft->data->mmft) ? holy_NTFS_AF_MMFT : 0;
  at->attr_nxt = mft->buf + u16at (mft->buf, 0x14);
  at->attr_end = at->emft_buf = at->edat_buf = at->sbuf = NULL;
}

static void
free_attr (struct holy_ntfs_attr *at)
{
  holy_free (at->emft_buf);
  holy_free (at->edat_buf);
  holy_free (at->sbuf);
}

static holy_uint8_t *
find_attr (struct holy_ntfs_attr *at, holy_uint8_t attr)
{
  if (at->flags & holy_NTFS_AF_ALST)
    {
    retry:
      while (at->attr_nxt < at->attr_end)
	{
	  at->attr_cur = at->attr_nxt;
	  at->attr_nxt += u16at (at->attr_cur, 4);
	  if ((*at->attr_cur == attr) || (attr == 0))
	    {
	      holy_uint8_t *new_pos;

	      if (at->flags & holy_NTFS_AF_MMFT)
		{
		  if ((holy_disk_read
		       (at->mft->data->disk, u32at (at->attr_cur, 0x10), 0,
			512, at->emft_buf))
		      ||
		      (holy_disk_read
		       (at->mft->data->disk, u32at (at->attr_cur, 0x14), 0,
			512, at->emft_buf + 512)))
		    return NULL;

		  if (fixup (at->emft_buf, at->mft->data->mft_size,
			     (const holy_uint8_t *) "FILE"))
		    return NULL;
		}
	      else
		{
		  if (read_mft (at->mft->data, at->emft_buf,
				u32at (at->attr_cur, 0x10)))
		    return NULL;
		}

	      new_pos = &at->emft_buf[u16at (at->emft_buf, 0x14)];
	      while (*new_pos != 0xFF)
		{
		  if ((*new_pos == *at->attr_cur)
		      && (u16at (new_pos, 0xE) == u16at (at->attr_cur, 0x18)))
		    {
		      return new_pos;
		    }
		  new_pos += u16at (new_pos, 4);
		}
	      holy_error (holy_ERR_BAD_FS,
			  "can\'t find 0x%X in attribute list",
			  (unsigned char) *at->attr_cur);
	      return NULL;
	    }
	}
      return NULL;
    }
  at->attr_cur = at->attr_nxt;
  while (*at->attr_cur != 0xFF)
    {
      at->attr_nxt += u16at (at->attr_cur, 4);
      if (*at->attr_cur == holy_NTFS_AT_ATTRIBUTE_LIST)
	at->attr_end = at->attr_cur;
      if ((*at->attr_cur == attr) || (attr == 0))
	return at->attr_cur;
      at->attr_cur = at->attr_nxt;
    }
  if (at->attr_end)
    {
      holy_uint8_t *pa;

      at->emft_buf = holy_malloc (at->mft->data->mft_size << holy_NTFS_BLK_SHR);
      if (at->emft_buf == NULL)
	return NULL;

      pa = at->attr_end;
      if (pa[8])
	{
          holy_uint32_t n;

          n = ((u32at (pa, 0x30) + holy_DISK_SECTOR_SIZE - 1)
               & (~(holy_DISK_SECTOR_SIZE - 1)));
	  at->attr_cur = at->attr_end;
	  at->edat_buf = holy_malloc (n);
	  if (!at->edat_buf)
	    return NULL;
	  if (read_data (at, pa, at->edat_buf, 0, n, 0, 0, 0))
	    {
	      holy_error (holy_ERR_BAD_FS,
			  "fail to read non-resident attribute list");
	      return NULL;
	    }
	  at->attr_nxt = at->edat_buf;
	  at->attr_end = at->edat_buf + u32at (pa, 0x30);
	}
      else
	{
	  at->attr_nxt = at->attr_end + u16at (pa, 0x14);
	  at->attr_end = at->attr_end + u32at (pa, 4);
	}
      at->flags |= holy_NTFS_AF_ALST;
      while (at->attr_nxt < at->attr_end)
	{
	  if ((*at->attr_nxt == attr) || (attr == 0))
	    break;
	  at->attr_nxt += u16at (at->attr_nxt, 4);
	}
      if (at->attr_nxt >= at->attr_end)
	return NULL;

      if ((at->flags & holy_NTFS_AF_MMFT) && (attr == holy_NTFS_AT_DATA))
	{
	  at->flags |= holy_NTFS_AF_GPOS;
	  at->attr_cur = at->attr_nxt;
	  pa = at->attr_cur;
	  holy_set_unaligned32 ((char *) pa + 0x10,
				holy_cpu_to_le32 (at->mft->data->mft_start));
	  holy_set_unaligned32 ((char *) pa + 0x14,
				holy_cpu_to_le32 (at->mft->data->mft_start
						  + 1));
	  pa = at->attr_nxt + u16at (pa, 4);
	  while (pa < at->attr_end)
	    {
	      if (*pa != attr)
		break;
	      if (read_attr
		  (at, pa + 0x10,
		   u32at (pa, 0x10) * (at->mft->data->mft_size << holy_NTFS_BLK_SHR),
		   at->mft->data->mft_size << holy_NTFS_BLK_SHR, 0, 0, 0))
		return NULL;
	      pa += u16at (pa, 4);
	    }
	  at->attr_nxt = at->attr_cur;
	  at->flags &= ~holy_NTFS_AF_GPOS;
	}
      goto retry;
    }
  return NULL;
}

static holy_uint8_t *
locate_attr (struct holy_ntfs_attr *at, struct holy_ntfs_file *mft,
	     holy_uint8_t attr)
{
  holy_uint8_t *pa;

  init_attr (at, mft);
  pa = find_attr (at, attr);
  if (pa == NULL)
    return NULL;
  if ((at->flags & holy_NTFS_AF_ALST) == 0)
    {
      while (1)
	{
	  pa = find_attr (at, attr);
	  if (pa == NULL)
	    break;
	  if (at->flags & holy_NTFS_AF_ALST)
	    return pa;
	}
      holy_errno = holy_ERR_NONE;
      free_attr (at);
      init_attr (at, mft);
      pa = find_attr (at, attr);
    }
  return pa;
}

static holy_disk_addr_t
read_run_data (const holy_uint8_t *run, int nn, int sig)
{
  holy_uint64_t r = 0;

  if (sig && nn && (run[nn - 1] & 0x80))
    r = -1;

  holy_memcpy (&r, run, nn);

  return holy_le_to_cpu64 (r);
}

holy_err_t
holy_ntfs_read_run_list (struct holy_ntfs_rlst * ctx)
{
  holy_uint8_t c1, c2;
  holy_disk_addr_t val;
  holy_uint8_t *run;

  run = ctx->cur_run;
retry:
  c1 = ((*run) & 0x7);
  c2 = ((*run) >> 4) & 0x7;
  run++;
  if (!c1)
    {
      if ((ctx->attr) && (ctx->attr->flags & holy_NTFS_AF_ALST))
	{
	  holy_disk_read_hook_t save_hook;

	  save_hook = ctx->comp.disk->read_hook;
	  ctx->comp.disk->read_hook = 0;
	  run = find_attr (ctx->attr, *ctx->attr->attr_cur);
	  ctx->comp.disk->read_hook = save_hook;
	  if (run)
	    {
	      if (run[8] == 0)
		return holy_error (holy_ERR_BAD_FS,
				   "$DATA should be non-resident");

	      run += u16at (run, 0x20);
	      ctx->curr_lcn = 0;
	      goto retry;
	    }
	}
      return holy_error (holy_ERR_BAD_FS, "run list overflown");
    }
  ctx->curr_vcn = ctx->next_vcn;
  ctx->next_vcn += read_run_data (run, c1, 0);	/* length of current VCN */
  run += c1;
  val = read_run_data (run, c2, 1);	/* offset to previous LCN */
  run += c2;
  ctx->curr_lcn += val;
  if (val == 0)
    ctx->flags |= holy_NTFS_RF_BLNK;
  else
    ctx->flags &= ~holy_NTFS_RF_BLNK;
  ctx->cur_run = run;
  return 0;
}

static holy_disk_addr_t
holy_ntfs_read_block (holy_fshelp_node_t node, holy_disk_addr_t block)
{
  struct holy_ntfs_rlst *ctx;

  ctx = (struct holy_ntfs_rlst *) node;
  if (block >= ctx->next_vcn)
    {
      if (holy_ntfs_read_run_list (ctx))
	return -1;
      return ctx->curr_lcn;
    }
  else
    return (ctx->flags & holy_NTFS_RF_BLNK) ? 0 : (block -
					 ctx->curr_vcn + ctx->curr_lcn);
}

static holy_err_t
read_data (struct holy_ntfs_attr *at, holy_uint8_t *pa, holy_uint8_t *dest,
	   holy_disk_addr_t ofs, holy_size_t len, int cached,
	   holy_disk_read_hook_t read_hook, void *read_hook_data)
{
  struct holy_ntfs_rlst cc, *ctx;

  if (len == 0)
    return 0;

  holy_memset (&cc, 0, sizeof (cc));
  ctx = &cc;
  ctx->attr = at;
  ctx->comp.log_spc = at->mft->data->log_spc;
  ctx->comp.disk = at->mft->data->disk;

  if (read_hook == holy_file_progress_hook)
    ctx->file = read_hook_data;

  if (pa[8] == 0)
    {
      if (ofs + len > u32at (pa, 0x10))
	return holy_error (holy_ERR_BAD_FS, "read out of range");
      holy_memcpy (dest, pa + u32at (pa, 0x14) + ofs, len);
      return 0;
    }

  ctx->cur_run = pa + u16at (pa, 0x20);

  ctx->next_vcn = u32at (pa, 0x10);
  ctx->curr_lcn = 0;

  if ((pa[0xC] & holy_NTFS_FLAG_COMPRESSED)
      && !(at->flags & holy_NTFS_AF_GPOS))
    {
      if (!cached)
	return holy_error (holy_ERR_BAD_FS, "attribute can\'t be compressed");

      return (holy_ntfscomp_func) ? holy_ntfscomp_func (dest, ofs, len, ctx)
	: holy_error (holy_ERR_BAD_FS, N_("module `%s' isn't loaded"),
		      "ntfscomp");
    }

  ctx->target_vcn = ofs >> (holy_NTFS_BLK_SHR + ctx->comp.log_spc);
  while (ctx->next_vcn <= ctx->target_vcn)
    {
      if (holy_ntfs_read_run_list (ctx))
	return holy_errno;
    }

  if (at->flags & holy_NTFS_AF_GPOS)
    {
      holy_disk_addr_t st0, st1;
      holy_uint64_t m;

      m = (ofs >> holy_NTFS_BLK_SHR) & ((1 << ctx->comp.log_spc) - 1);

      st0 =
	((ctx->target_vcn - ctx->curr_vcn + ctx->curr_lcn) << ctx->comp.log_spc) + m;
      st1 = st0 + 1;
      if (st1 ==
	  (ctx->next_vcn - ctx->curr_vcn + ctx->curr_lcn) << ctx->comp.log_spc)
	{
	  if (holy_ntfs_read_run_list (ctx))
	    return holy_errno;
	  st1 = ctx->curr_lcn << ctx->comp.log_spc;
	}
      holy_set_unaligned32 (dest, holy_cpu_to_le32 (st0));
      holy_set_unaligned32 (dest + 4, holy_cpu_to_le32 (st1));
      return 0;
    }

  holy_fshelp_read_file (ctx->comp.disk, (holy_fshelp_node_t) ctx,
			 read_hook, read_hook_data, ofs, len,
			 (char *) dest,
			 holy_ntfs_read_block, ofs + len,
			 ctx->comp.log_spc, 0);
  return holy_errno;
}

static holy_err_t
read_attr (struct holy_ntfs_attr *at, holy_uint8_t *dest, holy_disk_addr_t ofs,
	   holy_size_t len, int cached,
	   holy_disk_read_hook_t read_hook, void *read_hook_data)
{
  holy_uint8_t *save_cur;
  holy_uint8_t attr;
  holy_uint8_t *pp;
  holy_err_t ret;

  save_cur = at->attr_cur;
  at->attr_nxt = at->attr_cur;
  attr = *at->attr_nxt;
  if (at->flags & holy_NTFS_AF_ALST)
    {
      holy_uint8_t *pa;
      holy_disk_addr_t vcn;

      /* If compression is possible make sure that we include possible
	 compressed block size.  */
      if (holy_NTFS_LOG_COM_SEC >= at->mft->data->log_spc)
	vcn = ((ofs >> holy_NTFS_COM_LOG_LEN)
	       << (holy_NTFS_LOG_COM_SEC - at->mft->data->log_spc)) & ~0xFULL;
      else
	vcn = ofs >> (at->mft->data->log_spc + holy_NTFS_BLK_SHR);
      pa = at->attr_nxt + u16at (at->attr_nxt, 4);
      while (pa < at->attr_end)
	{
	  if (*pa != attr)
	    break;
	  if (u32at (pa, 8) > vcn)
	    break;
	  at->attr_nxt = pa;
	  pa += u16at (pa, 4);
	}
    }
  pp = find_attr (at, attr);
  if (pp)
    ret = read_data (at, pp, dest, ofs, len, cached,
		     read_hook, read_hook_data);
  else
    ret =
      (holy_errno) ? holy_errno : holy_error (holy_ERR_BAD_FS,
					      "attribute not found");
  at->attr_cur = save_cur;
  return ret;
}

static holy_err_t
read_mft (struct holy_ntfs_data *data, holy_uint8_t *buf, holy_uint64_t mftno)
{
  if (read_attr
      (&data->mmft.attr, buf, mftno * ((holy_disk_addr_t) data->mft_size << holy_NTFS_BLK_SHR),
       data->mft_size << holy_NTFS_BLK_SHR, 0, 0, 0))
    return holy_error (holy_ERR_BAD_FS, "read MFT 0x%llx fails", (unsigned long long) mftno);
  return fixup (buf, data->mft_size, (const holy_uint8_t *) "FILE");
}

static holy_err_t
init_file (struct holy_ntfs_file *mft, holy_uint64_t mftno)
{
  unsigned short flag;

  mft->inode_read = 1;

  mft->buf = holy_malloc (mft->data->mft_size << holy_NTFS_BLK_SHR);
  if (mft->buf == NULL)
    return holy_errno;

  if (read_mft (mft->data, mft->buf, mftno))
    return holy_errno;

  flag = u16at (mft->buf, 0x16);
  if ((flag & 1) == 0)
    return holy_error (holy_ERR_BAD_FS, "MFT 0x%llx is not in use",
		       (unsigned long long) mftno);

  if ((flag & 2) == 0)
    {
      holy_uint8_t *pa;

      pa = locate_attr (&mft->attr, mft, holy_NTFS_AT_DATA);
      if (pa == NULL)
	return holy_error (holy_ERR_BAD_FS, "no $DATA in MFT 0x%llx",
			   (unsigned long long) mftno);

      if (!pa[8])
	mft->size = u32at (pa, 0x10);
      else
	mft->size = u64at (pa, 0x30);

      if ((mft->attr.flags & holy_NTFS_AF_ALST) == 0)
	mft->attr.attr_end = 0;	/*  Don't jump to attribute list */
    }
  else
    init_attr (&mft->attr, mft);

  return 0;
}

static void
free_file (struct holy_ntfs_file *mft)
{
  free_attr (&mft->attr);
  holy_free (mft->buf);
}

static char *
get_utf8 (holy_uint8_t *in, holy_size_t len)
{
  holy_uint8_t *buf;
  holy_uint16_t *tmp;
  holy_size_t i;

  buf = holy_malloc (len * holy_MAX_UTF8_PER_UTF16 + 1);
  tmp = holy_malloc (len * sizeof (tmp[0]));
  if (!buf || !tmp)
    {
      holy_free (buf);
      holy_free (tmp);
      return NULL;
    }
  for (i = 0; i < len; i++)
    tmp[i] = holy_le_to_cpu16 (holy_get_unaligned16 (in + 2 * i));
  *holy_utf16_to_utf8 (buf, tmp, len) = '\0';
  holy_free (tmp);
  return (char *) buf;
}

static int
list_file (struct holy_ntfs_file *diro, holy_uint8_t *pos,
	   holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  holy_uint8_t *np;
  int ns;

  while (1)
    {
      holy_uint8_t namespace;
      char *ustr;

      if (pos[0xC] & 2)		/* end signature */
	break;

      np = pos + 0x50;
      ns = *(np++);
      namespace = *(np++);

      /*
       *  Ignore files in DOS namespace, as they will reappear as Win32
       *  names.
       */
      if ((ns) && (namespace != 2))
	{
	  enum holy_fshelp_filetype type;
	  struct holy_ntfs_file *fdiro;
	  holy_uint32_t attr;

	  attr = u32at (pos, 0x48);
	  if (attr & holy_NTFS_ATTR_REPARSE)
	    type = holy_FSHELP_SYMLINK;
	  else if (attr & holy_NTFS_ATTR_DIRECTORY)
	    type = holy_FSHELP_DIR;
	  else
	    type = holy_FSHELP_REG;

	  fdiro = holy_zalloc (sizeof (struct holy_ntfs_file));
	  if (!fdiro)
	    return 0;

	  fdiro->data = diro->data;
	  fdiro->ino = u64at (pos, 0) & 0xffffffffffffULL;
	  fdiro->mtime = u64at (pos, 0x20);

	  ustr = get_utf8 (np, ns);
	  if (ustr == NULL)
	    {
	      holy_free (fdiro);
	      return 0;
	    }
          if (namespace)
            type |= holy_FSHELP_CASE_INSENSITIVE;

	  if (hook (ustr, type, fdiro, hook_data))
	    {
	      holy_free (ustr);
	      return 1;
	    }

	  holy_free (ustr);
	}
      pos += u16at (pos, 8);
    }
  return 0;
}

struct symlink_descriptor
{
  holy_uint32_t type;
  holy_uint32_t total_len;
  holy_uint16_t off1;
  holy_uint16_t len1;
  holy_uint16_t off2;
  holy_uint16_t len2;
} holy_PACKED;

static char *
holy_ntfs_read_symlink (holy_fshelp_node_t node)
{
  struct holy_ntfs_file *mft;
  struct symlink_descriptor symdesc;
  holy_err_t err;
  holy_uint8_t *buf16;
  char *buf, *end;
  holy_size_t len;
  holy_uint8_t *pa;
  holy_size_t off;

  mft = (struct holy_ntfs_file *) node;

  mft->buf = holy_malloc (mft->data->mft_size << holy_NTFS_BLK_SHR);
  if (mft->buf == NULL)
    return NULL;

  if (read_mft (mft->data, mft->buf, mft->ino))
    return NULL;

  pa = locate_attr (&mft->attr, mft, holy_NTFS_AT_SYMLINK);
  if (pa == NULL)
    {
      holy_error (holy_ERR_BAD_FS, "no $SYMLINK in MFT 0x%llx",
		  (unsigned long long) mft->ino);
      return NULL;
    }

  err = read_attr (&mft->attr, (holy_uint8_t *) &symdesc, 0,
		   sizeof (struct symlink_descriptor), 1, 0, 0);
  if (err)
    return NULL;

  switch (holy_cpu_to_le32 (symdesc.type))
    {
    case 0xa000000c:
      off = (sizeof (struct symlink_descriptor) + 4
	     + holy_cpu_to_le32 (symdesc.off1));
      len = holy_cpu_to_le32 (symdesc.len1);
      break;
    case 0xa0000003:
      off = (sizeof (struct symlink_descriptor)
	     + holy_cpu_to_le32 (symdesc.off1));
      len = holy_cpu_to_le32 (symdesc.len1);
      break;
    default:
      holy_error (holy_ERR_BAD_FS, "symlink type invalid (%x)",
		  holy_cpu_to_le32 (symdesc.type));
      return NULL;
    }

  buf16 = holy_malloc (len);
  if (!buf16)
    return NULL;

  err = read_attr (&mft->attr, buf16, off, len, 1, 0, 0);
  if (err)
    return NULL;

  buf = get_utf8 (buf16, len / 2);
  if (!buf)
    {
      holy_free (buf16);
      return NULL;
    }
  holy_free (buf16);

  for (end = buf; *end; end++)
    if (*end == '\\')
      *end = '/';

  /* Split the sequence to avoid GCC thinking that this is a trigraph.  */
  if (holy_memcmp (buf, "/?" "?/", 4) == 0 && buf[5] == ':' && buf[6] == '/'
      && holy_isalpha (buf[4]))
    {
      holy_memmove (buf, buf + 6, end - buf + 1 - 6);
      end -= 6; 
    }
  return buf;
}

static int
holy_ntfs_iterate_dir (holy_fshelp_node_t dir,
		       holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  holy_uint8_t *bitmap;
  struct holy_ntfs_attr attr, *at;
  holy_uint8_t *cur_pos, *indx, *bmp;
  int ret = 0;
  holy_size_t bitmap_len;
  struct holy_ntfs_file *mft;

  mft = (struct holy_ntfs_file *) dir;

  if (!mft->inode_read)
    {
      if (init_file (mft, mft->ino))
	return 0;
    }

  indx = NULL;
  bmp = NULL;

  at = &attr;
  init_attr (at, mft);
  while (1)
    {
      cur_pos = find_attr (at, holy_NTFS_AT_INDEX_ROOT);
      if (cur_pos == NULL)
	{
	  holy_error (holy_ERR_BAD_FS, "no $INDEX_ROOT");
	  goto done;
	}

      /* Resident, Namelen=4, Offset=0x18, Flags=0x00, Name="$I30" */
      if ((u32at (cur_pos, 8) != 0x180400) ||
	  (u32at (cur_pos, 0x18) != 0x490024) ||
	  (u32at (cur_pos, 0x1C) != 0x300033))
	continue;
      cur_pos += u16at (cur_pos, 0x14);
      if (*cur_pos != 0x30)	/* Not filename index */
	continue;
      break;
    }

  cur_pos += 0x10;		/* Skip index root */
  ret = list_file (mft, cur_pos + u16at (cur_pos, 0), hook, hook_data);
  if (ret)
    goto done;

  bitmap = NULL;
  bitmap_len = 0;
  free_attr (at);
  init_attr (at, mft);
  while ((cur_pos = find_attr (at, holy_NTFS_AT_BITMAP)) != NULL)
    {
      int ofs;

      ofs = cur_pos[0xA];
      /* Namelen=4, Name="$I30" */
      if ((cur_pos[9] == 4) &&
	  (u32at (cur_pos, ofs) == 0x490024) &&
	  (u32at (cur_pos, ofs + 4) == 0x300033))
	{
          int is_resident = (cur_pos[8] == 0);

          bitmap_len = ((is_resident) ? u32at (cur_pos, 0x10) :
                        u32at (cur_pos, 0x28));

          bmp = holy_malloc (bitmap_len);
          if (bmp == NULL)
            goto done;

	  if (is_resident)
	    {
              holy_memcpy (bmp, cur_pos + u16at (cur_pos, 0x14),
                           bitmap_len);
	    }
          else
            {
              if (read_data (at, cur_pos, bmp, 0, bitmap_len, 0, 0, 0))
                {
                  holy_error (holy_ERR_BAD_FS,
                              "fails to read non-resident $BITMAP");
                  goto done;
                }
              bitmap_len = u32at (cur_pos, 0x30);
            }

          bitmap = bmp;
	  break;
	}
    }

  free_attr (at);
  cur_pos = locate_attr (at, mft, holy_NTFS_AT_INDEX_ALLOCATION);
  while (cur_pos != NULL)
    {
      /* Non-resident, Namelen=4, Offset=0x40, Flags=0, Name="$I30" */
      if ((u32at (cur_pos, 8) == 0x400401) &&
	  (u32at (cur_pos, 0x40) == 0x490024) &&
	  (u32at (cur_pos, 0x44) == 0x300033))
	break;
      cur_pos = find_attr (at, holy_NTFS_AT_INDEX_ALLOCATION);
    }

  if ((!cur_pos) && (bitmap))
    {
      holy_error (holy_ERR_BAD_FS, "$BITMAP without $INDEX_ALLOCATION");
      goto done;
    }

  if (bitmap)
    {
      holy_disk_addr_t i;
      holy_uint8_t v;

      indx = holy_malloc (mft->data->idx_size << holy_NTFS_BLK_SHR);
      if (indx == NULL)
	goto done;

      v = 1;
      for (i = 0; i < (holy_disk_addr_t)bitmap_len * 8; i++)
	{
	  if (*bitmap & v)
	    {
	      if ((read_attr
		   (at, indx, i * (mft->data->idx_size << holy_NTFS_BLK_SHR),
		    (mft->data->idx_size << holy_NTFS_BLK_SHR), 0, 0, 0))
		  || (fixup (indx, mft->data->idx_size,
			     (const holy_uint8_t *) "INDX")))
		goto done;
	      ret = list_file (mft, &indx[0x18 + u16at (indx, 0x18)],
			       hook, hook_data);
	      if (ret)
		goto done;
	    }
	  v <<= 1;
	  if (!v)
	    {
	      v = 1;
	      bitmap++;
	    }
	}
    }

done:
  free_attr (at);
  holy_free (indx);
  holy_free (bmp);

  return ret;
}

static struct holy_ntfs_data *
holy_ntfs_mount (holy_disk_t disk)
{
  struct holy_ntfs_bpb bpb;
  struct holy_ntfs_data *data = 0;
  holy_uint32_t spc;

  if (!disk)
    goto fail;

  data = (struct holy_ntfs_data *) holy_zalloc (sizeof (*data));
  if (!data)
    goto fail;

  data->disk = disk;

  /* Read the BPB.  */
  if (holy_disk_read (disk, 0, 0, sizeof (bpb), &bpb))
    goto fail;

  if (holy_memcmp ((char *) &bpb.oem_name, "NTFS", 4) != 0
      || bpb.sectors_per_cluster == 0
      || (bpb.sectors_per_cluster & (bpb.sectors_per_cluster - 1)) != 0
      || bpb.bytes_per_sector == 0
      || (bpb.bytes_per_sector & (bpb.bytes_per_sector - 1)) != 0)
    goto fail;

  spc = (((holy_uint32_t) bpb.sectors_per_cluster
	  * (holy_uint32_t) holy_le_to_cpu16 (bpb.bytes_per_sector))
	 >> holy_NTFS_BLK_SHR);
  if (spc == 0)
    goto fail;

  for (data->log_spc = 0; (1U << data->log_spc) < spc; data->log_spc++);

  if (bpb.clusters_per_mft > 0)
    data->mft_size = ((holy_disk_addr_t) bpb.clusters_per_mft) << data->log_spc;
  else if (-bpb.clusters_per_mft < holy_NTFS_BLK_SHR || -bpb.clusters_per_mft >= 31)
    goto fail;
  else
    data->mft_size = 1ULL << (-bpb.clusters_per_mft - holy_NTFS_BLK_SHR);

  if (bpb.clusters_per_index > 0)
    data->idx_size = (((holy_disk_addr_t) bpb.clusters_per_index)
		      << data->log_spc);
  else if (-bpb.clusters_per_index < holy_NTFS_BLK_SHR || -bpb.clusters_per_index >= 31)
    goto fail;
  else
    data->idx_size = 1ULL << (-bpb.clusters_per_index - holy_NTFS_BLK_SHR);

  data->mft_start = holy_le_to_cpu64 (bpb.mft_lcn) << data->log_spc;

  if ((data->mft_size > holy_NTFS_MAX_MFT) || (data->idx_size > holy_NTFS_MAX_IDX))
    goto fail;

  data->mmft.data = data;
  data->cmft.data = data;

  data->mmft.buf = holy_malloc (data->mft_size << holy_NTFS_BLK_SHR);
  if (!data->mmft.buf)
    goto fail;

  if (holy_disk_read
      (disk, data->mft_start, 0, data->mft_size << holy_NTFS_BLK_SHR, data->mmft.buf))
    goto fail;

  data->uuid = holy_le_to_cpu64 (bpb.num_serial);

  if (fixup (data->mmft.buf, data->mft_size, (const holy_uint8_t *) "FILE"))
    goto fail;

  if (!locate_attr (&data->mmft.attr, &data->mmft, holy_NTFS_AT_DATA))
    goto fail;

  if (init_file (&data->cmft, holy_NTFS_FILE_ROOT))
    goto fail;

  return data;

fail:
  holy_error (holy_ERR_BAD_FS, "not an ntfs filesystem");

  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      holy_free (data);
    }
  return 0;
}

/* Context for holy_ntfs_dir.  */
struct holy_ntfs_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_ntfs_dir.  */
static int
holy_ntfs_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		    holy_fshelp_node_t node, void *data)
{
  struct holy_ntfs_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  info.mtimeset = 1;
  info.mtime = holy_divmod64 (node->mtime, 10000000, 0)
    - 86400ULL * 365 * (1970 - 1601)
    - 86400ULL * ((1970 - 1601) / 4) + 86400ULL * ((1970 - 1601) / 100);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_ntfs_dir (holy_device_t device, const char *path,
	       holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_ntfs_dir_ctx ctx = { hook, hook_data };
  struct holy_ntfs_data *data = 0;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_ntfs_mount (device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file (path, &data->cmft, &fdiro, holy_ntfs_iterate_dir,
			 holy_ntfs_read_symlink, holy_FSHELP_DIR);

  if (holy_errno)
    goto fail;

  holy_ntfs_iterate_dir (fdiro, holy_ntfs_dir_iter, &ctx);

fail:
  if ((fdiro) && (fdiro != &data->cmft))
    {
      free_file (fdiro);
      holy_free (fdiro);
    }
  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      holy_free (data);
    }

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_ntfs_open (holy_file_t file, const char *name)
{
  struct holy_ntfs_data *data = 0;
  struct holy_fshelp_node *mft = 0;

  holy_dl_ref (my_mod);

  data = holy_ntfs_mount (file->device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file (name, &data->cmft, &mft, holy_ntfs_iterate_dir,
			 holy_ntfs_read_symlink, holy_FSHELP_REG);

  if (holy_errno)
    goto fail;

  if (mft != &data->cmft)
    {
      free_file (&data->cmft);
      holy_memcpy (&data->cmft, mft, sizeof (*mft));
      holy_free (mft);
      if (!data->cmft.inode_read)
	{
	  if (init_file (&data->cmft, data->cmft.ino))
	    goto fail;
	}
    }

  file->size = data->cmft.size;
  file->data = data;
  file->offset = 0;

  return 0;

fail:
  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      holy_free (data);
    }

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_ssize_t
holy_ntfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_ntfs_file *mft;

  mft = &((struct holy_ntfs_data *) file->data)->cmft;
  if (file->read_hook)
    mft->attr.save_pos = 1;

  read_attr (&mft->attr, (holy_uint8_t *) buf, file->offset, len, 1,
	     file->read_hook, file->read_hook_data);
  return (holy_errno) ? -1 : (holy_ssize_t) len;
}

static holy_err_t
holy_ntfs_close (holy_file_t file)
{
  struct holy_ntfs_data *data;

  data = file->data;

  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      holy_free (data);
    }

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_ntfs_label (holy_device_t device, char **label)
{
  struct holy_ntfs_data *data = 0;
  struct holy_fshelp_node *mft = 0;
  holy_uint8_t *pa;

  holy_dl_ref (my_mod);

  *label = 0;

  data = holy_ntfs_mount (device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file ("/$Volume", &data->cmft, &mft, holy_ntfs_iterate_dir,
			 0, holy_FSHELP_REG);

  if (holy_errno)
    goto fail;

  if (!mft->inode_read)
    {
      mft->buf = holy_malloc (mft->data->mft_size << holy_NTFS_BLK_SHR);
      if (mft->buf == NULL)
	goto fail;

      if (read_mft (mft->data, mft->buf, mft->ino))
	goto fail;
    }

  init_attr (&mft->attr, mft);
  pa = find_attr (&mft->attr, holy_NTFS_AT_VOLUME_NAME);
  if ((pa) && (pa[8] == 0) && (u32at (pa, 0x10)))
    {
      int len;

      len = u32at (pa, 0x10) / 2;
      pa += u16at (pa, 0x14);
      *label = get_utf8 (pa, len);
    }

fail:
  if ((mft) && (mft != &data->cmft))
    {
      free_file (mft);
      holy_free (mft);
    }
  if (data)
    {
      free_file (&data->mmft);
      free_file (&data->cmft);
      holy_free (data);
    }

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_ntfs_uuid (holy_device_t device, char **uuid)
{
  struct holy_ntfs_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_ntfs_mount (disk);
  if (data)
    {
      char *ptr;
      *uuid = holy_xasprintf ("%016llx", (unsigned long long) data->uuid);
      if (*uuid)
	for (ptr = *uuid; *ptr; ptr++)
	  *ptr = holy_toupper (*ptr);
      free_file (&data->mmft);
      free_file (&data->cmft);
      holy_free (data);
    }
  else
    *uuid = NULL;

  holy_dl_unref (my_mod);

  return holy_errno;
}

static struct holy_fs holy_ntfs_fs =
  {
    .name = "ntfs",
    .dir = holy_ntfs_dir,
    .open = holy_ntfs_open,
    .read = holy_ntfs_read,
    .close = holy_ntfs_close,
    .label = holy_ntfs_label,
    .uuid = holy_ntfs_uuid,
#ifdef holy_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
};

holy_MOD_INIT (ntfs)
{
  holy_fs_register (&holy_ntfs_fs);
  my_mod = mod;
}

holy_MOD_FINI (ntfs)
{
  holy_fs_unregister (&holy_ntfs_fs);
}
