/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/ntfs.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
decomp_nextvcn (struct holy_ntfs_comp *cc)
{
  if (cc->comp_head >= cc->comp_tail)
    return holy_error (holy_ERR_BAD_FS, "compression block overflown");
  if (holy_disk_read
      (cc->disk,
       (cc->comp_table[cc->comp_head].next_lcn -
	(cc->comp_table[cc->comp_head].next_vcn - cc->cbuf_vcn)) << cc->log_spc,
       0,
       1 << (cc->log_spc + holy_NTFS_BLK_SHR), cc->cbuf))
    return holy_errno;
  cc->cbuf_vcn++;
  if ((cc->cbuf_vcn >= cc->comp_table[cc->comp_head].next_vcn))
    cc->comp_head++;
  cc->cbuf_ofs = 0;
  return 0;
}

static holy_err_t
decomp_getch (struct holy_ntfs_comp *cc, holy_uint8_t *res)
{
  if (cc->cbuf_ofs >= (1U << (cc->log_spc + holy_NTFS_BLK_SHR)))
    {
      if (decomp_nextvcn (cc))
	return holy_errno;
    }
  *res = cc->cbuf[cc->cbuf_ofs++];
  return 0;
}

static holy_err_t
decomp_get16 (struct holy_ntfs_comp *cc, holy_uint16_t * res)
{
  holy_uint8_t c1 = 0, c2 = 0;

  if ((decomp_getch (cc, &c1)) || (decomp_getch (cc, &c2)))
    return holy_errno;
  *res = ((holy_uint16_t) c2) * 256 + ((holy_uint16_t) c1);
  return 0;
}

/* Decompress a block (4096 bytes) */
static holy_err_t
decomp_block (struct holy_ntfs_comp *cc, holy_uint8_t *dest)
{
  holy_uint16_t flg, cnt;

  if (decomp_get16 (cc, &flg))
    return holy_errno;
  cnt = (flg & 0xFFF) + 1;

  if (dest)
    {
      if (flg & 0x8000)
	{
	  holy_uint8_t tag;
	  holy_uint32_t bits, copied;

	  bits = copied = tag = 0;
	  while (cnt > 0)
	    {
	      if (copied > holy_NTFS_COM_LEN)
		return holy_error (holy_ERR_BAD_FS,
				   "compression block too large");

	      if (!bits)
		{
		  if (decomp_getch (cc, &tag))
		    return holy_errno;

		  bits = 8;
		  cnt--;
		  if (cnt <= 0)
		    break;
		}
	      if (tag & 1)
		{
		  holy_uint32_t i, len, delta, code, lmask, dshift;
		  holy_uint16_t word = 0;

		  if (decomp_get16 (cc, &word))
		    return holy_errno;

		  code = word;
		  cnt -= 2;

		  if (!copied)
		    {
		      holy_error (holy_ERR_BAD_FS, "nontext window empty");
		      return 0;
		    }

		  for (i = copied - 1, lmask = 0xFFF, dshift = 12; i >= 0x10;
		       i >>= 1)
		    {
		      lmask >>= 1;
		      dshift--;
		    }

		  delta = code >> dshift;
		  len = (code & lmask) + 3;

		  for (i = 0; i < len; i++)
		    {
		      dest[copied] = dest[copied - delta - 1];
		      copied++;
		    }
		}
	      else
		{
		  holy_uint8_t ch = 0;

		  if (decomp_getch (cc, &ch))
		    return holy_errno;
		  dest[copied++] = ch;
		  cnt--;
		}
	      tag >>= 1;
	      bits--;
	    }
	  return 0;
	}
      else
	{
	  if (cnt != holy_NTFS_COM_LEN)
	    return holy_error (holy_ERR_BAD_FS,
			       "invalid compression block size");
	}
    }

  while (cnt > 0)
    {
      int n;

      n = (1 << (cc->log_spc + holy_NTFS_BLK_SHR)) - cc->cbuf_ofs;
      if (n > cnt)
	n = cnt;
      if ((dest) && (n))
	{
	  holy_memcpy (dest, &cc->cbuf[cc->cbuf_ofs], n);
	  dest += n;
	}
      cnt -= n;
      cc->cbuf_ofs += n;
      if ((cnt) && (decomp_nextvcn (cc)))
	return holy_errno;
    }
  return 0;
}

static holy_err_t
read_block (struct holy_ntfs_rlst *ctx, holy_uint8_t *buf, holy_size_t num)
{
  int log_cpb = holy_NTFS_LOG_COM_SEC - ctx->comp.log_spc;

  while (num)
    {
      holy_size_t nn;

      if ((ctx->target_vcn & 0xF) == 0)
	{

	  if (ctx->comp.comp_head != ctx->comp.comp_tail
	      && !(ctx->flags & holy_NTFS_RF_BLNK))
	    return holy_error (holy_ERR_BAD_FS, "invalid compression block");
	  ctx->comp.comp_head = ctx->comp.comp_tail = 0;
	  ctx->comp.cbuf_vcn = ctx->target_vcn;
	  ctx->comp.cbuf_ofs = (1 << (ctx->comp.log_spc + holy_NTFS_BLK_SHR));
	  if (ctx->target_vcn >= ctx->next_vcn)
	    {
	      if (holy_ntfs_read_run_list (ctx))
		return holy_errno;
	    }
	  while (ctx->target_vcn + 16 > ctx->next_vcn)
	    {
	      if (ctx->flags & holy_NTFS_RF_BLNK)
		break;
	      ctx->comp.comp_table[ctx->comp.comp_tail].next_vcn = ctx->next_vcn;
	      ctx->comp.comp_table[ctx->comp.comp_tail].next_lcn =
		ctx->curr_lcn + ctx->next_vcn - ctx->curr_vcn;
	      ctx->comp.comp_tail++;
	      if (holy_ntfs_read_run_list (ctx))
		return holy_errno;
	    }
	}

      nn = (16 - (unsigned) (ctx->target_vcn & 0xF)) >> log_cpb;
      if (nn > num)
	nn = num;
      num -= nn;

      if (ctx->flags & holy_NTFS_RF_BLNK)
	{
	  ctx->target_vcn += nn << log_cpb;
	  if (ctx->comp.comp_tail == 0)
	    {
	      if (buf)
		{
		  holy_memset (buf, 0, nn * holy_NTFS_COM_LEN);
		  buf += nn * holy_NTFS_COM_LEN;
		  if (holy_file_progress_hook && ctx->file)
		    holy_file_progress_hook (0, 0, nn * holy_NTFS_COM_LEN,
					     ctx->file);
		}
	    }
	  else
	    {
	      while (nn)
		{
		  if (decomp_block (&ctx->comp, buf))
		    return holy_errno;
		  if (buf)
		    buf += holy_NTFS_COM_LEN;
		  if (holy_file_progress_hook && ctx->file)
		    holy_file_progress_hook (0, 0, holy_NTFS_COM_LEN,
					     ctx->file);
		  nn--;
		}
	    }
	}
      else
	{
	  nn <<= log_cpb;
	  while ((ctx->comp.comp_head < ctx->comp.comp_tail) && (nn))
	    {
	      holy_disk_addr_t tt;

	      tt =
		ctx->comp.comp_table[ctx->comp.comp_head].next_vcn -
		ctx->target_vcn;
	      if (tt > nn)
		tt = nn;
	      ctx->target_vcn += tt;
	      if (buf)
		{
		  if (holy_disk_read
		      (ctx->comp.disk,
		       (ctx->comp.comp_table[ctx->comp.comp_head].next_lcn -
			(ctx->comp.comp_table[ctx->comp.comp_head].next_vcn -
			 ctx->target_vcn)) << ctx->comp.log_spc, 0,
		       tt << (ctx->comp.log_spc + holy_NTFS_BLK_SHR), buf))
		    return holy_errno;
		  if (holy_file_progress_hook && ctx->file)
		    holy_file_progress_hook (0, 0,
					     tt << (ctx->comp.log_spc
						    + holy_NTFS_BLK_SHR),
					     ctx->file);
		  buf += tt << (ctx->comp.log_spc + holy_NTFS_BLK_SHR);
		}
	      nn -= tt;
	      if (ctx->target_vcn >=
		  ctx->comp.comp_table[ctx->comp.comp_head].next_vcn)
		ctx->comp.comp_head++;
	    }
	  if (nn)
	    {
	      if (buf)
		{
		  if (holy_disk_read
		      (ctx->comp.disk,
		       (ctx->target_vcn - ctx->curr_vcn +
			ctx->curr_lcn) << ctx->comp.log_spc, 0,
		       nn << (ctx->comp.log_spc + holy_NTFS_BLK_SHR), buf))
		    return holy_errno;
		  buf += nn << (ctx->comp.log_spc + holy_NTFS_BLK_SHR);
		  if (holy_file_progress_hook && ctx->file)
		    holy_file_progress_hook (0, 0,
					     nn << (ctx->comp.log_spc
						    + holy_NTFS_BLK_SHR),
					     ctx->file);
		}
	      ctx->target_vcn += nn;
	    }
	}
    }
  return 0;
}

static holy_err_t
ntfscomp (holy_uint8_t *dest, holy_disk_addr_t ofs,
	  holy_size_t len, struct holy_ntfs_rlst *ctx)
{
  holy_err_t ret;
  holy_disk_addr_t vcn;

  if (ctx->attr->sbuf)
    {
      if ((ofs & (~(holy_NTFS_COM_LEN - 1))) == ctx->attr->save_pos)
	{
	  holy_disk_addr_t n;

	  n = holy_NTFS_COM_LEN - (ofs - ctx->attr->save_pos);
	  if (n > len)
	    n = len;

	  holy_memcpy (dest, ctx->attr->sbuf + ofs - ctx->attr->save_pos, n);
	  if (holy_file_progress_hook && ctx->file)
	    holy_file_progress_hook (0, 0, n, ctx->file);
	  if (n == len)
	    return 0;

	  dest += n;
	  len -= n;
	  ofs += n;
	}
    }
  else
    {
      ctx->attr->sbuf = holy_malloc (holy_NTFS_COM_LEN);
      if (ctx->attr->sbuf == NULL)
	return holy_errno;
      ctx->attr->save_pos = 1;
    }

  vcn = ctx->target_vcn = (ofs >> holy_NTFS_COM_LOG_LEN) * (holy_NTFS_COM_SEC >> ctx->comp.log_spc);
  ctx->target_vcn &= ~0xFULL;
  while (ctx->next_vcn <= ctx->target_vcn)
    {
      if (holy_ntfs_read_run_list (ctx))
	return holy_errno;
    }

  ctx->comp.comp_head = ctx->comp.comp_tail = 0;
  ctx->comp.cbuf = holy_malloc (1 << (ctx->comp.log_spc + holy_NTFS_BLK_SHR));
  if (!ctx->comp.cbuf)
    return 0;

  ret = 0;

  //ctx->comp.disk->read_hook = read_hook;
  //ctx->comp.disk->read_hook_data = read_hook_data;

  if ((vcn > ctx->target_vcn) &&
      (read_block
       (ctx, NULL, (vcn - ctx->target_vcn) >> (holy_NTFS_LOG_COM_SEC - ctx->comp.log_spc))))
    {
      ret = holy_errno;
      goto quit;
    }

  if (ofs % holy_NTFS_COM_LEN)
    {
      holy_uint32_t t, n, o;
      void *file = ctx->file;

      ctx->file = 0;

      t = ctx->target_vcn << (ctx->comp.log_spc + holy_NTFS_BLK_SHR);
      if (read_block (ctx, ctx->attr->sbuf, 1))
	{
	  ret = holy_errno;
	  goto quit;
	}

      ctx->file = file;

      ctx->attr->save_pos = t;

      o = ofs % holy_NTFS_COM_LEN;
      n = holy_NTFS_COM_LEN - o;
      if (n > len)
	n = len;
      holy_memcpy (dest, &ctx->attr->sbuf[o], n);
      if (holy_file_progress_hook && ctx->file)
	holy_file_progress_hook (0, 0, n, ctx->file);
      if (n == len)
	goto quit;
      dest += n;
      len -= n;
    }

  if (read_block (ctx, dest, len / holy_NTFS_COM_LEN))
    {
      ret = holy_errno;
      goto quit;
    }

  dest += (len / holy_NTFS_COM_LEN) * holy_NTFS_COM_LEN;
  len = len % holy_NTFS_COM_LEN;
  if (len)
    {
      holy_uint32_t t;
      void *file = ctx->file;

      ctx->file = 0;
      t = ctx->target_vcn << (ctx->comp.log_spc + holy_NTFS_BLK_SHR);
      if (read_block (ctx, ctx->attr->sbuf, 1))
	{
	  ret = holy_errno;
	  goto quit;
	}

      ctx->attr->save_pos = t;

      holy_memcpy (dest, ctx->attr->sbuf, len);
      if (holy_file_progress_hook && file)
	holy_file_progress_hook (0, 0, len, file);
    }

quit:
  //ctx->comp.disk->read_hook = 0;
  if (ctx->comp.cbuf)
    holy_free (ctx->comp.cbuf);
  return ret;
}

holy_MOD_INIT (ntfscomp)
{
  holy_ntfscomp_func = ntfscomp;
}

holy_MOD_FINI (ntfscomp)
{
  holy_ntfscomp_func = NULL;
}
