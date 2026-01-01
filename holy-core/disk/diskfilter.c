/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/diskfilter.h>
#include <holy/partition.h>
#ifdef holy_UTIL
#include <holy/i18n.h>
#include <holy/util/misc.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

/* Linked list of DISKFILTER arrays. */
static struct holy_diskfilter_vg *array_list;
holy_raid5_recover_func_t holy_raid5_recover_func;
holy_raid6_recover_func_t holy_raid6_recover_func;
holy_diskfilter_t holy_diskfilter_list;
static int inscnt = 0;
static int lv_num = 0;

static struct holy_diskfilter_lv *
find_lv (const char *name);
static int is_lv_readable (struct holy_diskfilter_lv *lv, int easily);



static holy_err_t
is_node_readable (const struct holy_diskfilter_node *node, int easily)
{
  /* Check whether we actually know the physical volume we want to
     read from.  */
  if (node->pv)
    return !!(node->pv->disk);
  if (node->lv)
    return is_lv_readable (node->lv, easily);
  return 0;
}

static int
is_lv_readable (struct holy_diskfilter_lv *lv, int easily)
{
  unsigned i, j;
  if (!lv)
    return 0;
  for (i = 0; i < lv->segment_count; i++)
    {
      int need = lv->segments[i].node_count, have = 0;
      switch (lv->segments[i].type)
	{
	case holy_DISKFILTER_RAID6:
	  if (!easily)
	    need--;
	  /* Fallthrough.  */
	case holy_DISKFILTER_RAID4:
	case holy_DISKFILTER_RAID5:
	  if (!easily)
	    need--;
	  /* Fallthrough.  */
	case holy_DISKFILTER_STRIPED:
	  break;

	case holy_DISKFILTER_MIRROR:
	  need = 1;
	  break;

	case holy_DISKFILTER_RAID10:
	  {
	    unsigned int n;
	    n = lv->segments[i].layout & 0xFF;
	    if (n == 1)
	      n = (lv->segments[i].layout >> 8) & 0xFF;
	    need = lv->segments[i].node_count - n + 1;
	  }
	  break;
	}
	for (j = 0; j < lv->segments[i].node_count; j++)
	  {
	    if (is_node_readable (lv->segments[i].nodes + j, easily))
	      have++;
	    if (have >= need)
	      break;
	  }
	if (have < need)
	  return 0;
    }

  return 1;
}

static holy_err_t
insert_array (holy_disk_t disk, const struct holy_diskfilter_pv_id *id,
	      struct holy_diskfilter_vg *array,
              holy_disk_addr_t start_sector,
	      holy_diskfilter_t diskfilter __attribute__ ((unused)));

static int
is_valid_diskfilter_name (const char *name)
{
  return (holy_memcmp (name, "md", sizeof ("md") - 1) == 0
	  || holy_memcmp (name, "lvm/", sizeof ("lvm/") - 1) == 0
	  || holy_memcmp (name, "lvmid/", sizeof ("lvmid/") - 1) == 0
	  || holy_memcmp (name, "ldm/", sizeof ("ldm/") - 1) == 0);
}

/* Helper for scan_disk.  */
static int
scan_disk_partition_iter (holy_disk_t disk, holy_partition_t p, void *data)
{
  const char *name = data;
  struct holy_diskfilter_vg *arr;
  holy_disk_addr_t start_sector;
  struct holy_diskfilter_pv_id id;
  holy_diskfilter_t diskfilter;

  holy_dprintf ("diskfilter", "Scanning for DISKFILTER devices on disk %s\n",
		name);
#ifdef holy_UTIL
  holy_util_info ("Scanning for DISKFILTER devices on disk %s", name);
#endif

  disk->partition = p;
  
  for (arr = array_list; arr != NULL; arr = arr->next)
    {
      struct holy_diskfilter_pv *m;
      for (m = arr->pvs; m; m = m->next)
	if (m->disk && m->disk->id == disk->id
	    && m->disk->dev->id == disk->dev->id
	    && m->part_start == holy_partition_get_start (disk->partition)
	    && m->part_size == holy_disk_get_size (disk))
	  return 0;
    }

  for (diskfilter = holy_diskfilter_list; diskfilter; diskfilter = diskfilter->next)
    {
#ifdef holy_UTIL
      holy_util_info ("Scanning for %s devices on disk %s",
		      diskfilter->name, name);
#endif
      id.uuid = 0;
      id.uuidlen = 0;
      arr = diskfilter->detect (disk, &id, &start_sector);
      if (arr &&
	  (! insert_array (disk, &id, arr, start_sector, diskfilter)))
	{
	  if (id.uuidlen)
	    holy_free (id.uuid);
	  return 0;
	}
      if (arr && id.uuidlen)
	holy_free (id.uuid);

      /* This error usually means it's not diskfilter, no need to display
	 it.  */
      if (holy_errno != holy_ERR_OUT_OF_RANGE)
	holy_print_error ();

      holy_errno = holy_ERR_NONE;
    }

  return 0;
}

static int
scan_disk (const char *name, int accept_diskfilter)
{
  holy_disk_t disk;
  static int scan_depth = 0;

  if (!accept_diskfilter && is_valid_diskfilter_name (name))
    return 0;

  if (scan_depth > 100)
    return 0;

  scan_depth++;
  disk = holy_disk_open (name);
  if (!disk)
    {
      holy_errno = holy_ERR_NONE;
      scan_depth--;
      return 0;
    }
  scan_disk_partition_iter (disk, 0, (void *) name);
  holy_partition_iterate (disk, scan_disk_partition_iter, (void *) name);
  holy_disk_close (disk);
  scan_depth--;
  return 0;
}

static int
scan_disk_hook (const char *name, void *data __attribute__ ((unused)))
{
  return scan_disk (name, 0);
}

static void
scan_devices (const char *arname)
{
  holy_disk_dev_t p;
  holy_disk_pull_t pull;
  struct holy_diskfilter_vg *vg;
  struct holy_diskfilter_lv *lv = NULL;
  int scan_depth;
  int need_rescan;

  for (pull = 0; pull < holy_DISK_PULL_MAX; pull++)
    for (p = holy_disk_dev_list; p; p = p->next)
      if (p->id != holy_DISK_DEVICE_DISKFILTER_ID
	  && p->iterate)
	{
	  if ((p->iterate) (scan_disk_hook, NULL, pull))
	    return;
	  if (arname && is_lv_readable (find_lv (arname), 1))
	    return;
	}

  scan_depth = 0;
  need_rescan = 1;
  while (need_rescan && scan_depth++ < 100)
    {
      need_rescan = 0;
      for (vg = array_list; vg; vg = vg->next)
	{
	  if (vg->lvs)
	    for (lv = vg->lvs; lv; lv = lv->next)
	      if (!lv->scanned && lv->fullname && lv->became_readable_at)
		{
		  scan_disk (lv->fullname, 1);
		  lv->scanned = 1;
		  need_rescan = 1;
		}
	}
    }

  if (need_rescan)
     holy_error (holy_ERR_UNKNOWN_DEVICE, "DISKFILTER scan depth exceeded");
}

static int
holy_diskfilter_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
			 holy_disk_pull_t pull)
{
  struct holy_diskfilter_vg *array;
  int islcnt = 0;

  if (pull == holy_DISK_PULL_RESCAN)
    {
      islcnt = inscnt + 1;
      scan_devices (NULL);
    }

  if (pull != holy_DISK_PULL_NONE && pull != holy_DISK_PULL_RESCAN)
    return 0;

  for (array = array_list; array; array = array->next)
    {
      struct holy_diskfilter_lv *lv;
      if (array->lvs)
	for (lv = array->lvs; lv; lv = lv->next)
	  if (lv->visible && lv->fullname && lv->became_readable_at >= islcnt)
	    {
	      if (hook (lv->fullname, hook_data))
		return 1;
	    }
    }

  return 0;
}

#ifdef holy_UTIL
static holy_disk_memberlist_t
holy_diskfilter_memberlist (holy_disk_t disk)
{
  struct holy_diskfilter_lv *lv = disk->data;
  holy_disk_memberlist_t list = NULL, tmp;
  struct holy_diskfilter_pv *pv;
  holy_disk_pull_t pull;
  holy_disk_dev_t p;
  struct holy_diskfilter_vg *vg;
  struct holy_diskfilter_lv *lv2 = NULL;

  if (!lv->vg->pvs)
    return NULL;

  pv = lv->vg->pvs;
  while (pv && pv->disk)
    pv = pv->next;

  for (pull = 0; pv && pull < holy_DISK_PULL_MAX; pull++)
    for (p = holy_disk_dev_list; pv && p; p = p->next)
      if (p->id != holy_DISK_DEVICE_DISKFILTER_ID
	  && p->iterate)
	{
	  (p->iterate) (scan_disk_hook, NULL, pull);
	  while (pv && pv->disk)
	    pv = pv->next;
	}

  for (vg = array_list; pv && vg; vg = vg->next)
    {
      if (vg->lvs)
	for (lv2 = vg->lvs; pv && lv2; lv2 = lv2->next)
	  if (!lv2->scanned && lv2->fullname && lv2->became_readable_at)
	    {
	      scan_disk (lv2->fullname, 1);
	      lv2->scanned = 1;
	      while (pv && pv->disk)
		pv = pv->next;
	    }
    }

  for (pv = lv->vg->pvs; pv; pv = pv->next)
    {
      if (!pv->disk)
	{
	  /* TRANSLATORS: This message kicks in during the detection of
	     which modules needs to be included in core image. This happens
	     in the case of degraded RAID and means that autodetection may
	     fail to include some of modules. It's an installation time
	     message, not runtime message.  */
	  holy_util_warn (_("Couldn't find physical volume `%s'."
			    " Some modules may be missing from core image."),
			  pv->name);
	  continue;
	}
      tmp = holy_malloc (sizeof (*tmp));
      tmp->disk = pv->disk;
      tmp->next = list;
      list = tmp;
    }

  return list;
}

void
holy_diskfilter_get_partmap (holy_disk_t disk,
			     void (*cb) (const char *pm, void *data),
			     void *data)
{
  struct holy_diskfilter_lv *lv = disk->data;
  struct holy_diskfilter_pv *pv;

  if (lv->vg->pvs)
    for (pv = lv->vg->pvs; pv; pv = pv->next)
      {
	holy_size_t s;
	if (!pv->disk)
	  {
	    /* TRANSLATORS: This message kicks in during the detection of
	       which modules needs to be included in core image. This happens
	       in the case of degraded RAID and means that autodetection may
	       fail to include some of modules. It's an installation time
	       message, not runtime message.  */
	    holy_util_warn (_("Couldn't find physical volume `%s'."
			      " Some modules may be missing from core image."),
			    pv->name);
	    continue;
	  }
	for (s = 0; pv->partmaps[s]; s++)
	  cb (pv->partmaps[s], data);
      }
}

static const char *
holy_diskfilter_getname (struct holy_disk *disk)
{
  struct holy_diskfilter_lv *array = disk->data;

  return array->vg->driver->name;
}
#endif

static inline char
hex2ascii (int c)
{
  if (c >= 10)
    return 'a' + c - 10;
  return c + '0';
}

static struct holy_diskfilter_lv *
find_lv (const char *name)
{
  struct holy_diskfilter_vg *vg;
  struct holy_diskfilter_lv *lv = NULL;

  for (vg = array_list; vg; vg = vg->next)
    {
      if (vg->lvs)
	for (lv = vg->lvs; lv; lv = lv->next)
	  if (((lv->fullname && holy_strcmp (lv->fullname, name) == 0)
	       || (lv->idname && holy_strcmp (lv->idname, name) == 0))
	      && is_lv_readable (lv, 0))
	    return lv;
    }
  return NULL;
}

static holy_err_t
holy_diskfilter_open (const char *name, holy_disk_t disk)
{
  struct holy_diskfilter_lv *lv;

  if (!is_valid_diskfilter_name (name))
     return holy_error (holy_ERR_UNKNOWN_DEVICE, "unknown DISKFILTER device %s",
			name);

  lv = find_lv (name);

  if (! lv)
    {
      scan_devices (name);
      if (holy_errno)
	{
	  holy_print_error ();
	  holy_errno = holy_ERR_NONE;
	}
      lv = find_lv (name);
    }

  if (!lv)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "unknown DISKFILTER device %s",
                       name);

  disk->id = lv->number;
  disk->data = lv;

  disk->total_sectors = lv->size;
  disk->max_agglomerate = holy_DISK_MAX_MAX_AGGLOMERATE;
  return 0;
}

static void
holy_diskfilter_close (holy_disk_t disk __attribute ((unused)))
{
  return;
}

static holy_err_t
read_lv (struct holy_diskfilter_lv *lv, holy_disk_addr_t sector,
	 holy_size_t size, char *buf);

holy_err_t
holy_diskfilter_read_node (const struct holy_diskfilter_node *node,
			   holy_disk_addr_t sector,
			   holy_size_t size, char *buf)
{
  /* Check whether we actually know the physical volume we want to
     read from.  */
  if (node->pv)
    {
      if (node->pv->disk)
	return holy_disk_read (node->pv->disk, sector + node->start
			       + node->pv->start_sector,
			       0, size << holy_DISK_SECTOR_BITS, buf);
      else
	return holy_error (holy_ERR_UNKNOWN_DEVICE,
			   N_("physical volume %s not found"), node->pv->name);

    }
  if (node->lv)
    return read_lv (node->lv, sector + node->start, size, buf);
  return holy_error (holy_ERR_UNKNOWN_DEVICE, "unknown node '%s'", node->name);
}


static holy_err_t
validate_segment (struct holy_diskfilter_segment *seg);

static holy_err_t
validate_lv (struct holy_diskfilter_lv *lv)
{
  unsigned int i;
  if (!lv)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "unknown volume");

  if (!lv->vg || lv->vg->extent_size == 0)
    return holy_error (holy_ERR_READ_ERROR, "invalid volume");

  for (i = 0; i < lv->segment_count; i++)
    {
      holy_err_t err;
      err = validate_segment (&lv->segments[i]);
      if (err)
	return err;
    }
  return holy_ERR_NONE;
}


static holy_err_t
validate_node (const struct holy_diskfilter_node *node)
{
  /* Check whether we actually know the physical volume we want to
     read from.  */
  if (node->pv)
    return holy_ERR_NONE;
  if (node->lv)
    return validate_lv (node->lv);
  return holy_error (holy_ERR_UNKNOWN_DEVICE, "unknown node '%s'", node->name);
}

static holy_err_t
validate_segment (struct holy_diskfilter_segment *seg)
{
  holy_err_t err;

  if (seg->stripe_size == 0 || seg->node_count == 0)
    return holy_error(holy_ERR_BAD_FS, "invalid segment");

  switch (seg->type)
    {
    case holy_DISKFILTER_RAID10:
      {
	holy_uint8_t near, far;
	near = seg->layout & 0xFF;
	far = (seg->layout >> 8) & 0xFF;
	if ((seg->layout >> 16) == 0 && far == 0)
	  return holy_error(holy_ERR_BAD_FS, "invalid segment");
	if (near > seg->node_count)
	  return holy_error(holy_ERR_BAD_FS, "invalid segment");
	break;
      }

    case holy_DISKFILTER_STRIPED:
    case holy_DISKFILTER_MIRROR:
	break;

    case holy_DISKFILTER_RAID4:
    case holy_DISKFILTER_RAID5:
      if (seg->node_count <= 1)
	return holy_error(holy_ERR_BAD_FS, "invalid segment");
      break;

    case holy_DISKFILTER_RAID6:
      if (seg->node_count <= 2)
	return holy_error(holy_ERR_BAD_FS, "invalid segment");
      break;

    default:
      return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			 "unsupported RAID level %d", seg->type);
    }

  unsigned i;
  for (i = 0; i < seg->node_count; i++)
    {
      err = validate_node (&seg->nodes[i]);
      if (err)
	return err;
    }
  return holy_ERR_NONE;

}

static holy_err_t
read_segment (struct holy_diskfilter_segment *seg, holy_disk_addr_t sector,
	      holy_size_t size, char *buf)
{
  holy_err_t err;
  switch (seg->type)
    {
    case holy_DISKFILTER_STRIPED:
      if (seg->node_count == 1)
	return holy_diskfilter_read_node (&seg->nodes[0],
					  sector, size, buf);
      /* Fallthrough.  */
    case holy_DISKFILTER_MIRROR:
    case holy_DISKFILTER_RAID10:
      {
	holy_disk_addr_t read_sector, far_ofs;
	holy_uint64_t disknr, b, near, far, ofs;
	unsigned int i, j;
	    
	read_sector = holy_divmod64 (sector, seg->stripe_size, &b);
	far = ofs = near = 1;
	far_ofs = 0;

	if (seg->type == 1)
	  near = seg->node_count;
	else if (seg->type == 10)
	  {
	    near = seg->layout & 0xFF;
	    far = (seg->layout >> 8) & 0xFF;
	    if (seg->layout >> 16)
	      {
		ofs = far;
		far_ofs = 1;
	      }
	    else
	      far_ofs = holy_divmod64 (seg->raid_member_size,
				       far * seg->stripe_size, 0);
		
	    far_ofs *= seg->stripe_size;
	  }

	read_sector = holy_divmod64 (read_sector * near,
				     seg->node_count,
				     &disknr);

	ofs *= seg->stripe_size;
	read_sector *= ofs;
	
	while (1)
	  {
	    holy_size_t read_size;

	    read_size = seg->stripe_size - b;
	    if (read_size > size)
	      read_size = size;

	    err = 0;
	    for (i = 0; i < near; i++)
	      {
		unsigned int k;

		k = disknr;
		err = 0;
		for (j = 0; j < far; j++)
		  {
		    if (holy_errno == holy_ERR_READ_ERROR
			|| holy_errno == holy_ERR_UNKNOWN_DEVICE)
		      holy_errno = holy_ERR_NONE;

		    err = holy_diskfilter_read_node (&seg->nodes[k],
						     read_sector
						     + j * far_ofs + b,
						     read_size,
						     buf);
		    if (! err)
		      break;
		    else if (err != holy_ERR_READ_ERROR
			     && err != holy_ERR_UNKNOWN_DEVICE)
		      return err;
		    k++;
		    if (k == seg->node_count)
		      k = 0;
		  }

		if (! err)
		  break;

		disknr++;
		if (disknr == seg->node_count)
		  {
		    disknr = 0;
		    read_sector += ofs;
		  }
	      }

	    if (err)
	      return err;

	    buf += read_size << holy_DISK_SECTOR_BITS;
	    size -= read_size;
	    if (! size)
	      return holy_ERR_NONE;
	    
	    b = 0;
	    disknr += (near - i);
	    while (disknr >= seg->node_count)
	      {
		disknr -= seg->node_count;
		read_sector += ofs;
	      }
	  }
      }

    case holy_DISKFILTER_RAID4:
    case holy_DISKFILTER_RAID5:
    case holy_DISKFILTER_RAID6:
      {
	holy_disk_addr_t read_sector;
	holy_uint64_t b, p, n, disknr, e;

	/* n = 1 for level 4 and 5, 2 for level 6.  */
	n = seg->type / 3;

	/* Find the first sector to read. */
	read_sector = holy_divmod64 (sector, seg->stripe_size, &b);
	read_sector = holy_divmod64 (read_sector, seg->node_count - n,
				     &disknr);
	if (seg->type >= 5)
	  {
	    holy_divmod64 (read_sector, seg->node_count, &p);

	    if (! (seg->layout & holy_RAID_LAYOUT_RIGHT_MASK))
	      p = seg->node_count - 1 - p;

	    if (seg->layout & holy_RAID_LAYOUT_SYMMETRIC_MASK)
	      {
		disknr += p + n;
	      }
	    else
	      {
		holy_uint32_t q;

		q = p + (n - 1);
		if (q >= seg->node_count)
		  q -= seg->node_count;

		if (disknr >= p)
		  disknr += n;
		else if (disknr >= q)
		  disknr += q + 1;
	      }

	    if (disknr >= seg->node_count)
	      disknr -= seg->node_count;
	  }
	else
	  p = seg->node_count - n;
	read_sector *= seg->stripe_size;

	while (1)
	  {
	    holy_size_t read_size;
	    int next_level;
	    
	    read_size = seg->stripe_size - b;
	    if (read_size > size)
	      read_size = size;

	    e = 0;
	    /* Reset read error.  */
	    if (holy_errno == holy_ERR_READ_ERROR
		|| holy_errno == holy_ERR_UNKNOWN_DEVICE)
	      holy_errno = holy_ERR_NONE;

	    err = holy_diskfilter_read_node (&seg->nodes[disknr],
					     read_sector + b,
					     read_size,
					     buf);

	    if ((err) && (err != holy_ERR_READ_ERROR
			  && err != holy_ERR_UNKNOWN_DEVICE))
	      return err;
	    e++;

	    if (err)
	      {
		holy_errno = holy_ERR_NONE;
		if (seg->type == holy_DISKFILTER_RAID6)
		  {
		    err = ((holy_raid6_recover_func) ?
			   (*holy_raid6_recover_func) (seg, disknr, p,
						       buf, read_sector + b,
						       read_size) :
			   holy_error (holy_ERR_BAD_DEVICE,
				       N_("module `%s' isn't loaded"),
				       "raid6rec"));
		  }
		else
		  {
		    err = ((holy_raid5_recover_func) ?
			   (*holy_raid5_recover_func) (seg, disknr,
						       buf, read_sector + b,
						       read_size) :
			   holy_error (holy_ERR_BAD_DEVICE,
				       N_("module `%s' isn't loaded"),
				       "raid5rec"));
		  }

		if (err)
		  return err;
	      }

	    buf += read_size << holy_DISK_SECTOR_BITS;
	    size -= read_size;
	    sector += read_size;
	    if (! size)
	      break;

	    b = 0;
	    disknr++;

	    if (seg->layout & holy_RAID_LAYOUT_SYMMETRIC_MASK)
	      {
		if (disknr == seg->node_count)
		  disknr = 0;

		next_level = (disknr == p);
	      }
	    else
	      {
		if (disknr == p)
		  disknr += n;

		next_level = (disknr >= seg->node_count);
	      }

	    if (next_level)
	      {
		read_sector += seg->stripe_size;

		if (seg->type >= 5)
		  {
		    if (seg->layout & holy_RAID_LAYOUT_RIGHT_MASK)
		      p = (p == seg->node_count - 1) ? 0 : p + 1;
		    else
		      p = (p == 0) ? seg->node_count - 1 : p - 1;

		    if (seg->layout & holy_RAID_LAYOUT_SYMMETRIC_MASK)
		      {
			disknr = p + n;
			if (disknr >= seg->node_count)
			  disknr -= seg->node_count;
		      }
		    else
		      {
			disknr -= seg->node_count;
			if ((disknr >= p && disknr < p + n)
			    || (disknr + seg->node_count >= p
				&& disknr + seg->node_count < p + n))
			  disknr = p + n;
			if (disknr >= seg->node_count)
			  disknr -= seg->node_count;
		      }
		  }
		else
		  disknr = 0;
	      }
	  }
      }   
      return holy_ERR_NONE;
    default:
      return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			 "unsupported RAID level %d", seg->type);
    }
}

static holy_err_t
read_lv (struct holy_diskfilter_lv *lv, holy_disk_addr_t sector,
	 holy_size_t size, char *buf)
{
  if (!lv)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "unknown volume");

  while (size)
    {
      holy_err_t err = 0;
      struct holy_diskfilter_vg *vg = lv->vg;
      struct holy_diskfilter_segment *seg = lv->segments;
      holy_uint64_t extent;
      holy_uint64_t to_read;

      extent = holy_divmod64 (sector, vg->extent_size, NULL);
      
      /* Find the right segment.  */
      {
	unsigned int i;
	for (i = 0; i < lv->segment_count; i++)
	  {
	    if ((seg->start_extent <= extent)
		&& ((seg->start_extent + seg->extent_count) > extent))
	      break;
	    seg++;
	  }
	if (i == lv->segment_count)
	  return holy_error (holy_ERR_READ_ERROR, "incorrect segment");
      }
      to_read = ((seg->start_extent + seg->extent_count)
		 * vg->extent_size) - sector;
      if (to_read > size)
	to_read = size;

      err = read_segment (seg, sector - seg->start_extent * vg->extent_size,
			  to_read, buf);
      if (err)
	return err;

      size -= to_read;
      sector += to_read;
      buf += to_read << holy_DISK_SECTOR_BITS;
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_diskfilter_read (holy_disk_t disk, holy_disk_addr_t sector,
		      holy_size_t size, char *buf)
{
  return read_lv (disk->data, sector, size, buf);
}

static holy_err_t
holy_diskfilter_write (holy_disk_t disk __attribute ((unused)),
		 holy_disk_addr_t sector __attribute ((unused)),
		 holy_size_t size __attribute ((unused)),
		 const char *buf __attribute ((unused)))
{
  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		     "diskfilter writes are not supported");
}

struct holy_diskfilter_vg *
holy_diskfilter_get_vg_by_uuid (holy_size_t uuidlen, char *uuid)
{
  struct holy_diskfilter_vg *p;

  for (p = array_list; p != NULL; p = p->next)
    if ((p->uuid_len == uuidlen) &&
        (! holy_memcmp (p->uuid, uuid, p->uuid_len)))
      return p;
  return NULL;
}

holy_err_t
holy_diskfilter_vg_register (struct holy_diskfilter_vg *vg)
{
  struct holy_diskfilter_lv *lv, *p;
  struct holy_diskfilter_vg *vgp;
  unsigned i;

  holy_dprintf ("diskfilter", "Found array %s\n", vg->name);
#ifdef holy_UTIL
  holy_util_info ("Found array %s", vg->name);
#endif

  for (lv = vg->lvs; lv; lv = lv->next)
    {
      holy_err_t err;

      /* RAID 1 and single-disk RAID 0 don't use a chunksize but code
         assumes one so set one. */
      for (i = 0; i < lv->segment_count; i++)
	{
	  if (lv->segments[i].type == 1)
	    lv->segments[i].stripe_size = 64;
	  if (lv->segments[i].type == holy_DISKFILTER_STRIPED
	      && lv->segments[i].node_count == 1
	      && lv->segments[i].stripe_size == 0)
	    lv->segments[i].stripe_size = 64;
	}

      err = validate_lv(lv);
      if (err)
	return err;
      lv->number = lv_num++;

      if (lv->fullname)
	{
	  holy_size_t len;
	  int max_used_number = 0, need_new_name = 0;
	  len = holy_strlen (lv->fullname);
	  for (vgp = array_list; vgp; vgp = vgp->next)
	    for (p = vgp->lvs; p; p = p->next)
	      {
		int cur_num;
		char *num, *end;
		if (!p->fullname)
		  continue;
		if (holy_strncmp (p->fullname, lv->fullname, len) != 0)
		  continue;
		if (p->fullname[len] == 0)
		  {
		    need_new_name = 1;
		    continue;
		  }
		num = p->fullname + len + 1;
		if (!holy_isdigit (num[0]))
		  continue;
		cur_num = holy_strtoul (num, &end, 10);
		if (end[0])
		  continue;
		if (cur_num > max_used_number)
		  max_used_number = cur_num;
	      }
	  if (need_new_name)
	    {
	      char *tmp;
	      tmp = holy_xasprintf ("%s_%d", lv->fullname, max_used_number + 1);
	      if (!tmp)
		return holy_errno;
	      holy_free (lv->fullname);
	      lv->fullname = tmp;
	    }
	}
    }
  /* Add our new array to the list.  */
  vg->next = array_list;
  array_list = vg;
  return holy_ERR_NONE;
}

struct holy_diskfilter_vg *
holy_diskfilter_make_raid (holy_size_t uuidlen, char *uuid, int nmemb,
			   const char *name, holy_uint64_t disk_size,
			   holy_uint64_t stripe_size,
			   int layout, int level)
{
  struct holy_diskfilter_vg *array;
  int i;
  holy_size_t j;
  holy_uint64_t totsize;
  struct holy_diskfilter_pv *pv;
  holy_err_t err;

  switch (level)
    {
    case 1:
      totsize = disk_size;
      break;

    case 10:
      {
	int n;
	n = layout & 0xFF;
	if (n == 1)
	  n = (layout >> 8) & 0xFF;
	if (n == 0)
	  {
	    holy_free (uuid);
	    return NULL;
	  }

	totsize = holy_divmod64 (nmemb * disk_size, n, 0);
      }
      break;

    case 0:
    case 4:
    case 5:
    case 6:
      totsize = (nmemb - ((unsigned) level / 3U)) * disk_size;
      break;

    default:
      holy_free (uuid);
      return NULL;
    }

  array = holy_diskfilter_get_vg_by_uuid (uuidlen, uuid);
  if (array)
    {
      if (array->lvs && array->lvs->size < totsize)
	{
	  array->lvs->size = totsize;
	  if (array->lvs->segments)
	    array->lvs->segments->extent_count = totsize;
	}

      if (array->lvs && array->lvs->segments
	  && array->lvs->segments->raid_member_size > disk_size)
	array->lvs->segments->raid_member_size = disk_size;

      holy_free (uuid);
      return array;
    }
  array = holy_zalloc (sizeof (*array));
  if (!array)
    {
      holy_free (uuid);
      return NULL;
    }
  array->uuid = uuid;
  array->uuid_len = uuidlen;
  if (name)
    {
      /* Strip off the homehost if present.  */
      char *colon = holy_strchr (name, ':');
      char *new_name = holy_xasprintf ("md/%s",
				       colon ? colon + 1 : name);

      if (! new_name)
	goto fail;

      array->name = new_name;
    }

  array->extent_size = 1;
  array->lvs = holy_zalloc (sizeof (*array->lvs));
  if (!array->lvs)
    goto fail;
  array->lvs->segment_count = 1;
  array->lvs->visible = 1;
  if (array->name)
    {
      array->lvs->name = holy_strdup (array->name);
      if (!array->lvs->name)
	goto fail;
      array->lvs->fullname = holy_strdup (array->name);
      if (!array->lvs->fullname)
	goto fail;
    }
  array->lvs->vg = array;

  array->lvs->idname = holy_malloc (sizeof ("mduuid/") + 2 * uuidlen);
  if (!array->lvs->idname)
    goto fail;

  holy_memcpy (array->lvs->idname, "mduuid/", sizeof ("mduuid/") - 1);
  for (j = 0; j < uuidlen; j++)
    {
      array->lvs->idname[sizeof ("mduuid/") - 1 + 2 * j]
	= hex2ascii (((unsigned char) uuid[j] >> 4));
      array->lvs->idname[sizeof ("mduuid/") - 1 + 2 * j + 1]
	= hex2ascii (((unsigned char) uuid[j] & 0xf));
    }
  array->lvs->idname[sizeof ("mduuid/") - 1 + 2 * uuidlen] = '\0';

  array->lvs->size = totsize;

  array->lvs->segments = holy_zalloc (sizeof (*array->lvs->segments));
  if (!array->lvs->segments)
    goto fail;
  array->lvs->segments->stripe_size = stripe_size;
  array->lvs->segments->layout = layout;
  array->lvs->segments->start_extent = 0;
  array->lvs->segments->extent_count = totsize;
  array->lvs->segments->type = level;
  array->lvs->segments->node_count = nmemb;
  array->lvs->segments->raid_member_size = disk_size;
  array->lvs->segments->nodes
    = holy_zalloc (nmemb * sizeof (array->lvs->segments->nodes[0]));
  array->lvs->segments->stripe_size = stripe_size;
  for (i = 0; i < nmemb; i++)
    {
      pv = holy_zalloc (sizeof (*pv));
      if (!pv)
	goto fail;
      pv->id.uuidlen = 0;
      pv->id.id = i;
      pv->next = array->pvs;
      array->pvs = pv;
      array->lvs->segments->nodes[i].pv = pv;
    }

  err = holy_diskfilter_vg_register (array);
  if (err)
    goto fail;

  return array;

 fail:
  if (array->lvs)
    {
      holy_free (array->lvs->name);
      holy_free (array->lvs->fullname);
      holy_free (array->lvs->idname);
      if (array->lvs->segments)
	{
	  holy_free (array->lvs->segments->nodes);
	  holy_free (array->lvs->segments);
	}
      holy_free (array->lvs);
    }
  while (array->pvs)
    {
      pv = array->pvs->next;
      holy_free (array->pvs);
      array->pvs = pv;
    }
  holy_free (array->name);
  holy_free (array->uuid);
  holy_free (array);
  return NULL;
}

static holy_err_t
insert_array (holy_disk_t disk, const struct holy_diskfilter_pv_id *id,
	      struct holy_diskfilter_vg *array,
              holy_disk_addr_t start_sector,
	      holy_diskfilter_t diskfilter __attribute__ ((unused)))
{
  struct holy_diskfilter_pv *pv;

  holy_dprintf ("diskfilter", "Inserting %s (+%lld,%lld) into %s (%s)\n", disk->name,
		(unsigned long long) holy_partition_get_start (disk->partition),
		(unsigned long long) holy_disk_get_size (disk),
		array->name, diskfilter->name);
#ifdef holy_UTIL
  holy_util_info ("Inserting %s (+%" holy_HOST_PRIuLONG_LONG ",%"
		  holy_HOST_PRIuLONG_LONG ") into %s (%s)\n", disk->name,
		  (unsigned long long) holy_partition_get_start (disk->partition),
		  (unsigned long long) holy_disk_get_size (disk),
		  array->name, diskfilter->name);
  array->driver = diskfilter;
#endif

  for (pv = array->pvs; pv; pv = pv->next)
    if (id->uuidlen == pv->id.uuidlen
	&& id->uuidlen 
	? (holy_memcmp (pv->id.uuid, id->uuid, id->uuidlen) == 0)
	: (pv->id.id == id->id))
      {
	struct holy_diskfilter_lv *lv;
	/* FIXME: Check whether the update time of the superblocks are
	   the same.  */
	if (pv->disk && holy_disk_get_size (disk) >= pv->part_size)
	  return holy_ERR_NONE;
	pv->disk = holy_disk_open (disk->name);
	if (!pv->disk)
	  return holy_errno;
	/* This could happen to LVM on RAID, pv->disk points to the
	   raid device, we shouldn't change it.  */
	pv->start_sector -= pv->part_start;
	pv->part_start = holy_partition_get_start (disk->partition);
	pv->part_size = holy_disk_get_size (disk);

#ifdef holy_UTIL
	{
	  holy_size_t s = 1;
	  holy_partition_t p;
	  for (p = disk->partition; p; p = p->parent)
	    s++;
	  pv->partmaps = xmalloc (s * sizeof (pv->partmaps[0]));
	  s = 0;
	  for (p = disk->partition; p; p = p->parent)
	    pv->partmaps[s++] = xstrdup (p->partmap->name);
	  pv->partmaps[s++] = 0;
	}
#endif
	if (start_sector != (holy_uint64_t)-1)
	  pv->start_sector = start_sector;
	pv->start_sector += pv->part_start;
	/* Add the device to the array. */
	for (lv = array->lvs; lv; lv = lv->next)
	  if (!lv->became_readable_at && lv->fullname && is_lv_readable (lv, 0))
	    lv->became_readable_at = ++inscnt;
	break;
      }

  return 0;
}

static void
free_array (void)
{
  while (array_list)
    {
      struct holy_diskfilter_vg *vg;
      struct holy_diskfilter_pv *pv;
      struct holy_diskfilter_lv *lv;

      vg = array_list;
      array_list = array_list->next;

      while ((pv = vg->pvs))
	{
	  vg->pvs = pv->next;
	  holy_free (pv->name);
	  if (pv->disk)
	    holy_disk_close (pv->disk);
	  if (pv->id.uuidlen)
	    holy_free (pv->id.uuid);
#ifdef holy_UTIL
	  holy_free (pv->partmaps);
#endif
	  holy_free (pv->internal_id);
	  holy_free (pv);
	}

      while ((lv = vg->lvs))
	{
	  unsigned i;
	  vg->lvs = lv->next;
	  holy_free (lv->fullname);
	  holy_free (lv->name);
	  holy_free (lv->idname);
	  for (i = 0; i < lv->segment_count; i++)
	    holy_free (lv->segments[i].nodes);
	  holy_free (lv->segments);
	  holy_free (lv->internal_id);
	  holy_free (lv);
	}

      holy_free (vg->uuid);
      holy_free (vg->name);
      holy_free (vg);
    }

  array_list = 0;
}

#ifdef holy_UTIL
struct holy_diskfilter_pv *
holy_diskfilter_get_pv_from_disk (holy_disk_t disk,
				  struct holy_diskfilter_vg **vg_out)
{
  struct holy_diskfilter_pv *pv;
  struct holy_diskfilter_vg *vg;

  scan_disk (disk->name, 1);
  for (vg = array_list; vg; vg = vg->next)
    for (pv = vg->pvs; pv; pv = pv->next)
      {
	if (pv->disk && pv->disk->id == disk->id
	    && pv->disk->dev->id == disk->dev->id
	    && pv->part_start == holy_partition_get_start (disk->partition)
	    && pv->part_size == holy_disk_get_size (disk))
	  {
	    if (vg_out)
	      *vg_out = vg;
	    return pv;
	  }
      }
  return NULL;
}
#endif

static struct holy_disk_dev holy_diskfilter_dev =
  {
    .name = "diskfilter",
    .id = holy_DISK_DEVICE_DISKFILTER_ID,
    .iterate = holy_diskfilter_iterate,
    .open = holy_diskfilter_open,
    .close = holy_diskfilter_close,
    .read = holy_diskfilter_read,
    .write = holy_diskfilter_write,
#ifdef holy_UTIL
    .memberlist = holy_diskfilter_memberlist,
    .raidname = holy_diskfilter_getname,
#endif
    .next = 0
  };


holy_MOD_INIT(diskfilter)
{
  holy_disk_dev_register (&holy_diskfilter_dev);
}

holy_MOD_FINI(diskfilter)
{
  holy_disk_dev_unregister (&holy_diskfilter_dev);
  free_array ();
}
