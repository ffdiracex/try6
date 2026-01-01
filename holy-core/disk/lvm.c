/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/lvm.h>
#include <holy/partition.h>
#include <holy/i18n.h>

#ifdef holy_UTIL
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>
#endif

holy_MOD_LICENSE ("GPLv2+");


/* Go the string STR and return the number after STR.  *P will point
   at the number.  In case STR is not found, *P will be NULL and the
   return value will be 0.  */
static holy_uint64_t
holy_lvm_getvalue (char **p, const char *str)
{
  *p = holy_strstr (*p, str);
  if (! *p)
    return 0;
  *p += holy_strlen (str);
  return holy_strtoull (*p, p, 10);
}

#if 0
static int
holy_lvm_checkvalue (char **p, char *str, char *tmpl)
{
  int tmpllen = holy_strlen (tmpl);
  *p = holy_strstr (*p, str);
  if (! *p)
    return 0;
  *p += holy_strlen (str);
  if (**p != '"')
    return 0;
  return (holy_memcmp (*p + 1, tmpl, tmpllen) == 0 && (*p)[tmpllen + 1] == '"');
}
#endif

static int
holy_lvm_check_flag (char *p, const char *str, const char *flag)
{
  holy_size_t len_str = holy_strlen (str), len_flag = holy_strlen (flag);
  while (1)
    {
      char *q;
      p = holy_strstr (p, str);
      if (! p)
	return 0;
      p += len_str;
      if (holy_memcmp (p, " = [", sizeof (" = [") - 1) != 0)
	continue;
      q = p + sizeof (" = [") - 1;
      while (1)
	{
	  while (holy_isspace (*q))
	    q++;
	  if (*q != '"')
	    return 0;
	  q++;
	  if (holy_memcmp (q, flag, len_flag) == 0 && q[len_flag] == '"')
	    return 1;
	  while (*q != '"')
	    q++;
	  q++;
	  if (*q == ']')
	    return 0;
	  q++;
	}
    }
}

static struct holy_diskfilter_vg *
holy_lvm_detect (holy_disk_t disk,
		 struct holy_diskfilter_pv_id *id,
		 holy_disk_addr_t *start_sector)
{
  holy_err_t err;
  holy_uint64_t mda_offset, mda_size;
  char buf[holy_LVM_LABEL_SIZE];
  char vg_id[holy_LVM_ID_STRLEN+1];
  char pv_id[holy_LVM_ID_STRLEN+1];
  char *metadatabuf, *p, *q, *vgname;
  struct holy_lvm_label_header *lh = (struct holy_lvm_label_header *) buf;
  struct holy_lvm_pv_header *pvh;
  struct holy_lvm_disk_locn *dlocn;
  struct holy_lvm_mda_header *mdah;
  struct holy_lvm_raw_locn *rlocn;
  unsigned int i, j;
  holy_size_t vgname_len;
  struct holy_diskfilter_vg *vg;
  struct holy_diskfilter_pv *pv;

  /* Search for label. */
  for (i = 0; i < holy_LVM_LABEL_SCAN_SECTORS; i++)
    {
      err = holy_disk_read (disk, i, 0, sizeof(buf), buf);
      if (err)
	goto fail;

      if ((! holy_strncmp ((char *)lh->id, holy_LVM_LABEL_ID,
			   sizeof (lh->id)))
	  && (! holy_strncmp ((char *)lh->type, holy_LVM_LVM2_LABEL,
			      sizeof (lh->type))))
	break;
    }

  /* Return if we didn't find a label. */
  if (i == holy_LVM_LABEL_SCAN_SECTORS)
    {
#ifdef holy_UTIL
      holy_util_info ("no LVM signature found");
#endif
      goto fail;
    }

  pvh = (struct holy_lvm_pv_header *) (buf + holy_le_to_cpu32(lh->offset_xl));

  for (i = 0, j = 0; i < holy_LVM_ID_LEN; i++)
    {
      pv_id[j++] = pvh->pv_uuid[i];
      if ((i != 1) && (i != 29) && (i % 4 == 1))
	pv_id[j++] = '-';
    }
  pv_id[j] = '\0';

  dlocn = pvh->disk_areas_xl;

  dlocn++;
  /* Is it possible to have multiple data/metadata areas? I haven't
     seen devices that have it. */
  if (dlocn->offset)
    {
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "we don't support multiple LVM data areas");

#ifdef holy_UTIL
      holy_util_info ("we don't support multiple LVM data areas");
#endif
      goto fail;
    }

  dlocn++;
  mda_offset = holy_le_to_cpu64 (dlocn->offset);
  mda_size = holy_le_to_cpu64 (dlocn->size);

  /* It's possible to have multiple copies of metadata areas, we just use the
     first one.  */

  /* Allocate buffer space for the circular worst-case scenario. */
  metadatabuf = holy_malloc (2 * mda_size);
  if (! metadatabuf)
    goto fail;

  err = holy_disk_read (disk, 0, mda_offset, mda_size, metadatabuf);
  if (err)
    goto fail2;

  mdah = (struct holy_lvm_mda_header *) metadatabuf;
  if ((holy_strncmp ((char *)mdah->magic, holy_LVM_FMTT_MAGIC,
		     sizeof (mdah->magic)))
      || (holy_le_to_cpu32 (mdah->version) != holy_LVM_FMTT_VERSION))
    {
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "unknown LVM metadata header");
#ifdef holy_UTIL
      holy_util_info ("unknown LVM metadata header");
#endif
      goto fail2;
    }

  rlocn = mdah->raw_locns;
  if (holy_le_to_cpu64 (rlocn->offset) + holy_le_to_cpu64 (rlocn->size) >
      holy_le_to_cpu64 (mdah->size))
    {
      /* Metadata is circular. Copy the wrap in place. */
      holy_memcpy (metadatabuf + mda_size,
		   metadatabuf + holy_LVM_MDA_HEADER_SIZE,
		   holy_le_to_cpu64 (rlocn->offset) +
		   holy_le_to_cpu64 (rlocn->size) -
		   holy_le_to_cpu64 (mdah->size));
    }
  p = q = metadatabuf + holy_le_to_cpu64 (rlocn->offset);

  while (*q != ' ' && q < metadatabuf + mda_size)
    q++;

  if (q == metadatabuf + mda_size)
    {
#ifdef holy_UTIL
      holy_util_info ("error parsing metadata");
#endif
      goto fail2;
    }

  vgname_len = q - p;
  vgname = holy_malloc (vgname_len + 1);
  if (!vgname)
    goto fail2;

  holy_memcpy (vgname, p, vgname_len);
  vgname[vgname_len] = '\0';

  p = holy_strstr (q, "id = \"");
  if (p == NULL)
    {
#ifdef holy_UTIL
      holy_util_info ("couldn't find ID");
#endif
      goto fail3;
    }
  p += sizeof ("id = \"") - 1;
  holy_memcpy (vg_id, p, holy_LVM_ID_STRLEN);
  vg_id[holy_LVM_ID_STRLEN] = '\0';

  vg = holy_diskfilter_get_vg_by_uuid (holy_LVM_ID_STRLEN, vg_id);

  if (! vg)
    {
      /* First time we see this volume group. We've to create the
	 whole volume group structure. */
      vg = holy_malloc (sizeof (*vg));
      if (! vg)
	goto fail3;
      vg->name = vgname;
      vg->uuid = holy_malloc (holy_LVM_ID_STRLEN);
      if (! vg->uuid)
	goto fail3;
      holy_memcpy (vg->uuid, vg_id, holy_LVM_ID_STRLEN);
      vg->uuid_len = holy_LVM_ID_STRLEN;

      vg->extent_size = holy_lvm_getvalue (&p, "extent_size = ");
      if (p == NULL)
	{
#ifdef holy_UTIL
	  holy_util_info ("unknown extent size");
#endif
	  goto fail4;
	}

      vg->lvs = NULL;
      vg->pvs = NULL;

      p = holy_strstr (p, "physical_volumes {");
      if (p)
	{
	  p += sizeof ("physical_volumes {") - 1;

	  /* Add all the pvs to the volume group. */
	  while (1)
	    {
	      holy_ssize_t s;
	      while (holy_isspace (*p))
		p++;

	      if (*p == '}')
		break;

	      pv = holy_zalloc (sizeof (*pv));
	      q = p;
	      while (*q != ' ')
		q++;

	      s = q - p;
	      pv->name = holy_malloc (s + 1);
	      holy_memcpy (pv->name, p, s);
	      pv->name[s] = '\0';

	      p = holy_strstr (p, "id = \"");
	      if (p == NULL)
		goto pvs_fail;
	      p += sizeof("id = \"") - 1;

	      pv->id.uuid = holy_malloc (holy_LVM_ID_STRLEN);
	      if (!pv->id.uuid)
		goto pvs_fail;
	      holy_memcpy (pv->id.uuid, p, holy_LVM_ID_STRLEN);
	      pv->id.uuidlen = holy_LVM_ID_STRLEN;

	      pv->start_sector = holy_lvm_getvalue (&p, "pe_start = ");
	      if (p == NULL)
		{
#ifdef holy_UTIL
		  holy_util_info ("unknown pe_start");
#endif
		  goto pvs_fail;
		}

	      p = holy_strchr (p, '}');
	      if (p == NULL)
		{
#ifdef holy_UTIL
		  holy_util_info ("error parsing pe_start");
#endif
		  goto pvs_fail;
		}
	      p++;

	      pv->disk = NULL;
	      pv->next = vg->pvs;
	      vg->pvs = pv;

	      continue;
	    pvs_fail:
	      holy_free (pv->name);
	      holy_free (pv);
	      goto fail4;
	    }
	}

      p = holy_strstr (p, "logical_volumes {");
      if (p)
	{
	  p += sizeof ("logical_volumes {") - 1;

	  /* And add all the lvs to the volume group. */
	  while (1)
	    {
	      holy_ssize_t s;
	      int skip_lv = 0;
	      struct holy_diskfilter_lv *lv;
	      struct holy_diskfilter_segment *seg;
	      int is_pvmove;

	      while (holy_isspace (*p))
		p++;

	      if (*p == '}')
		break;

	      lv = holy_zalloc (sizeof (*lv));

	      q = p;
	      while (*q != ' ')
		q++;

	      s = q - p;
	      lv->name = holy_strndup (p, s);
	      if (!lv->name)
		goto lvs_fail;

	      {
		const char *iptr;
		char *optr;
		lv->fullname = holy_malloc (sizeof ("lvm/") - 1 + 2 * vgname_len
					    + 1 + 2 * s + 1);
		if (!lv->fullname)
		  goto lvs_fail;

		holy_memcpy (lv->fullname, "lvm/", sizeof ("lvm/") - 1);
		optr = lv->fullname + sizeof ("lvm/") - 1;
		for (iptr = vgname; iptr < vgname + vgname_len; iptr++)
		  {
		    *optr++ = *iptr;
		    if (*iptr == '-')
		      *optr++ = '-';
		  }
		*optr++ = '-';
		for (iptr = p; iptr < p + s; iptr++)
		  {
		    *optr++ = *iptr;
		    if (*iptr == '-')
		      *optr++ = '-';
		  }
		*optr++ = 0;
		lv->idname = holy_malloc (sizeof ("lvmid/")
					  + 2 * holy_LVM_ID_STRLEN + 1);
		if (!lv->idname)
		  goto lvs_fail;
		holy_memcpy (lv->idname, "lvmid/",
			     sizeof ("lvmid/") - 1);
		holy_memcpy (lv->idname + sizeof ("lvmid/") - 1,
			     vg_id, holy_LVM_ID_STRLEN);
		lv->idname[sizeof ("lvmid/") - 1 + holy_LVM_ID_STRLEN] = '/';

		p = holy_strstr (q, "id = \"");
		if (p == NULL)
		  {
#ifdef holy_UTIL
		    holy_util_info ("couldn't find ID");
#endif
		    goto lvs_fail;
		  }
		p += sizeof ("id = \"") - 1;
		holy_memcpy (lv->idname + sizeof ("lvmid/") - 1
			     + holy_LVM_ID_STRLEN + 1,
			     p, holy_LVM_ID_STRLEN);
		lv->idname[sizeof ("lvmid/") - 1 + 2 * holy_LVM_ID_STRLEN + 1] = '\0';
	      }

	      lv->size = 0;

	      lv->visible = holy_lvm_check_flag (p, "status", "VISIBLE");
	      is_pvmove = holy_lvm_check_flag (p, "status", "PVMOVE");

	      lv->segment_count = holy_lvm_getvalue (&p, "segment_count = ");
	      if (p == NULL)
		{
#ifdef holy_UTIL
		  holy_util_info ("unknown segment_count");
#endif
		  goto lvs_fail;
		}
	      lv->segments = holy_zalloc (sizeof (*seg) * lv->segment_count);
	      seg = lv->segments;

	      for (i = 0; i < lv->segment_count; i++)
		{

		  p = holy_strstr (p, "segment");
		  if (p == NULL)
		    {
#ifdef holy_UTIL
		      holy_util_info ("unknown segment");
#endif
		      goto lvs_segment_fail;
		    }

		  seg->start_extent = holy_lvm_getvalue (&p, "start_extent = ");
		  if (p == NULL)
		    {
#ifdef holy_UTIL
		      holy_util_info ("unknown start_extent");
#endif
		      goto lvs_segment_fail;
		    }
		  seg->extent_count = holy_lvm_getvalue (&p, "extent_count = ");
		  if (p == NULL)
		    {
#ifdef holy_UTIL
		      holy_util_info ("unknown extent_count");
#endif
		      goto lvs_segment_fail;
		    }

		  p = holy_strstr (p, "type = \"");
		  if (p == NULL)
		    goto lvs_segment_fail;
		  p += sizeof("type = \"") - 1;

		  lv->size += seg->extent_count * vg->extent_size;

		  if (holy_memcmp (p, "striped\"",
				   sizeof ("striped\"") - 1) == 0)
		    {
		      struct holy_diskfilter_node *stripe;

		      seg->type = holy_DISKFILTER_STRIPED;
		      seg->node_count = holy_lvm_getvalue (&p, "stripe_count = ");
		      if (p == NULL)
			{
#ifdef holy_UTIL
			  holy_util_info ("unknown stripe_count");
#endif
			  goto lvs_segment_fail;
			}

		      if (seg->node_count != 1)
			seg->stripe_size = holy_lvm_getvalue (&p, "stripe_size = ");

		      seg->nodes = holy_zalloc (sizeof (*stripe)
						* seg->node_count);
		      stripe = seg->nodes;

		      p = holy_strstr (p, "stripes = [");
		      if (p == NULL)
			{
#ifdef holy_UTIL
			  holy_util_info ("unknown stripes");
#endif
			  goto lvs_segment_fail2;
			}
		      p += sizeof("stripes = [") - 1;

		      for (j = 0; j < seg->node_count; j++)
			{
			  p = holy_strchr (p, '"');
			  if (p == NULL)
			    continue;
			  q = ++p;
			  while (*q != '"')
			    q++;

			  s = q - p;

			  stripe->name = holy_malloc (s + 1);
			  if (stripe->name == NULL)
			    goto lvs_segment_fail2;

			  holy_memcpy (stripe->name, p, s);
			  stripe->name[s] = '\0';

			  p = q + 1;

			  stripe->start = holy_lvm_getvalue (&p, ",")
			    * vg->extent_size;
			  if (p == NULL)
			    continue;

			  stripe++;
			}
		    }
		  else if (holy_memcmp (p, "mirror\"", sizeof ("mirror\"") - 1)
			   == 0)
		    {
		      seg->type = holy_DISKFILTER_MIRROR;
		      seg->node_count = holy_lvm_getvalue (&p, "mirror_count = ");
		      if (p == NULL)
			{
#ifdef holy_UTIL
			  holy_util_info ("unknown mirror_count");
#endif
			  goto lvs_segment_fail;
			}

		      seg->nodes = holy_zalloc (sizeof (seg->nodes[0])
						* seg->node_count);

		      p = holy_strstr (p, "mirrors = [");
		      if (p == NULL)
			{
#ifdef holy_UTIL
			  holy_util_info ("unknown mirrors");
#endif
			  goto lvs_segment_fail2;
			}
		      p += sizeof("mirrors = [") - 1;

		      for (j = 0; j < seg->node_count; j++)
			{
			  char *lvname;

			  p = holy_strchr (p, '"');
			  if (p == NULL)
			    continue;
			  q = ++p;
			  while (*q != '"')
			    q++;

			  s = q - p;

			  lvname = holy_malloc (s + 1);
			  if (lvname == NULL)
			    goto lvs_segment_fail2;

			  holy_memcpy (lvname, p, s);
			  lvname[s] = '\0';
			  seg->nodes[j].name = lvname;
			  p = q + 1;
			}
		      /* Only first (original) is ok with in progress pvmove.  */
		      if (is_pvmove)
			seg->node_count = 1;
		    }
		  else if (holy_memcmp (p, "raid", sizeof ("raid") - 1) == 0
			   && ((p[sizeof ("raid") - 1] >= '4'
				&& p[sizeof ("raid") - 1] <= '6')
			       || p[sizeof ("raid") - 1] == '1')
			   && p[sizeof ("raidX") - 1] == '"')
		    {
		      switch (p[sizeof ("raid") - 1])
			{
			case '1':
			  seg->type = holy_DISKFILTER_MIRROR;
			  break;
			case '4':
			  seg->type = holy_DISKFILTER_RAID4;
			  seg->layout = holy_RAID_LAYOUT_LEFT_ASYMMETRIC;
			  break;
			case '5':
			  seg->type = holy_DISKFILTER_RAID5;
			  seg->layout = holy_RAID_LAYOUT_LEFT_SYMMETRIC;
			  break;
			case '6':
			  seg->type = holy_DISKFILTER_RAID6;
			  seg->layout = (holy_RAID_LAYOUT_RIGHT_ASYMMETRIC
					 | holy_RAID_LAYOUT_MUL_FROM_POS);
			  break;
			}
		      seg->node_count = holy_lvm_getvalue (&p, "device_count = ");

		      if (p == NULL)
			{
#ifdef holy_UTIL
			  holy_util_info ("unknown device_count");
#endif
			  goto lvs_segment_fail;
			}

		      if (seg->type != holy_DISKFILTER_MIRROR)
			{
			  seg->stripe_size = holy_lvm_getvalue (&p, "stripe_size = ");
			  if (p == NULL)
			    {
#ifdef holy_UTIL
			      holy_util_info ("unknown stripe_size");
#endif
			      goto lvs_segment_fail;
			    }
			}

		      seg->nodes = holy_zalloc (sizeof (seg->nodes[0])
						* seg->node_count);

		      p = holy_strstr (p, "raids = [");
		      if (p == NULL)
			{
#ifdef holy_UTIL
			  holy_util_info ("unknown raids");
#endif
			  goto lvs_segment_fail2;
			}
		      p += sizeof("raids = [") - 1;

		      for (j = 0; j < seg->node_count; j++)
			{
			  char *lvname;

			  p = holy_strchr (p, '"');
			  p = p ? holy_strchr (p + 1, '"') : 0;
			  p = p ? holy_strchr (p + 1, '"') : 0;
			  if (p == NULL)
			    continue;
			  q = ++p;
			  while (*q != '"')
			    q++;

			  s = q - p;

			  lvname = holy_malloc (s + 1);
			  if (lvname == NULL)
			    goto lvs_segment_fail2;

			  holy_memcpy (lvname, p, s);
			  lvname[s] = '\0';
			  seg->nodes[j].name = lvname;
			  p = q + 1;
			}
		      if (seg->type == holy_DISKFILTER_RAID4)
			{
			  char *tmp;
			  tmp = seg->nodes[0].name;
			  holy_memmove (seg->nodes, seg->nodes + 1,
					sizeof (seg->nodes[0])
					* (seg->node_count - 1));
			  seg->nodes[seg->node_count - 1].name = tmp;
			}
		    }
		  else
		    {
#ifdef holy_UTIL
		      char *p2;
		      p2 = holy_strchr (p, '"');
		      if (p2)
			*p2 = 0;
		      holy_util_info ("unknown LVM type %s", p);
		      if (p2)
			*p2 ='"';
#endif
		      /* Found a non-supported type, give up and move on. */
		      skip_lv = 1;
		      break;
		    }

		  seg++;

		  continue;
		lvs_segment_fail2:
		  holy_free (seg->nodes);
		lvs_segment_fail:
		  goto fail4;
		}

	      if (p != NULL)
		p = holy_strchr (p, '}');
	      if (p == NULL)
		goto lvs_fail;
	      p += 3;

	      if (skip_lv)
		{
		  holy_free (lv->name);
		  holy_free (lv);
		  continue;
		}

	      lv->vg = vg;
	      lv->next = vg->lvs;
	      vg->lvs = lv;

	      continue;
	    lvs_fail:
	      holy_free (lv->name);
	      holy_free (lv);
	      goto fail4;
	    }
	}

      /* Match lvs.  */
      {
	struct holy_diskfilter_lv *lv1;
	struct holy_diskfilter_lv *lv2;
	for (lv1 = vg->lvs; lv1; lv1 = lv1->next)
	  for (i = 0; i < lv1->segment_count; i++)
	    for (j = 0; j < lv1->segments[i].node_count; j++)
	      {
		if (vg->pvs)
		  for (pv = vg->pvs; pv; pv = pv->next)
		    {
		      if (! holy_strcmp (pv->name,
					 lv1->segments[i].nodes[j].name))
			{
			  lv1->segments[i].nodes[j].pv = pv;
			  break;
			}
		    }
		if (lv1->segments[i].nodes[j].pv == NULL)
		  for (lv2 = vg->lvs; lv2; lv2 = lv2->next)
		    if (holy_strcmp (lv2->name,
				     lv1->segments[i].nodes[j].name) == 0)
		      lv1->segments[i].nodes[j].lv = lv2;
	      }
	
      }
      if (holy_diskfilter_vg_register (vg))
	goto fail4;
    }
  else
    {
      holy_free (vgname);
    }

  id->uuid = holy_malloc (holy_LVM_ID_STRLEN);
  if (!id->uuid)
    goto fail4;
  holy_memcpy (id->uuid, pv_id, holy_LVM_ID_STRLEN);
  id->uuidlen = holy_LVM_ID_STRLEN;
  holy_free (metadatabuf);
  *start_sector = -1;
  return vg;

  /* Failure path.  */
 fail4:
  holy_free (vg);
 fail3:
  holy_free (vgname);

 fail2:
  holy_free (metadatabuf);
 fail:
  return NULL;
}



static struct holy_diskfilter holy_lvm_dev = {
  .name = "lvm",
  .detect = holy_lvm_detect,
  .next = 0
};

holy_MOD_INIT (lvm)
{
  holy_diskfilter_register_back (&holy_lvm_dev);
}

holy_MOD_FINI (lvm)
{
  holy_diskfilter_unregister (&holy_lvm_dev);
}
