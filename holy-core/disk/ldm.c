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
#include <holy/msdos_partition.h>
#include <holy/gpt_partition.h>
#include <holy/i18n.h>

#ifdef holy_UTIL
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

#define LDM_GUID_STRLEN 64
#define LDM_NAME_STRLEN 32

typedef holy_uint8_t *holy_ldm_id_t;

enum { STRIPE = 1, SPANNED = 2, RAID5 = 3 };

#define LDM_LABEL_SECTOR 6
struct holy_ldm_vblk {
  char magic[4];
  holy_uint8_t unused1[12];
  holy_uint16_t update_status;
  holy_uint8_t flags;
  holy_uint8_t type;
  holy_uint32_t unused2;
  holy_uint8_t dynamic[104];
} holy_PACKED;
#define LDM_VBLK_MAGIC "VBLK"

enum
  {
    STATUS_CONSISTENT = 0,
    STATUS_STILL_ACTIVE = 1,
    STATUS_NOT_ACTIVE_YET = 2
  };

enum
  {
    ENTRY_COMPONENT = 0x32,
    ENTRY_PARTITION = 0x33,
    ENTRY_DISK = 0x34,
    ENTRY_VOLUME = 0x51,
  };

struct holy_ldm_label
{
  char magic[8];
  holy_uint32_t unused1;
  holy_uint16_t ver_major;
  holy_uint16_t ver_minor;
  holy_uint8_t unused2[32];
  char disk_guid[LDM_GUID_STRLEN];
  char host_guid[LDM_GUID_STRLEN];
  char group_guid[LDM_GUID_STRLEN];
  char group_name[LDM_NAME_STRLEN];
  holy_uint8_t unused3[11];
  holy_uint64_t pv_start;
  holy_uint64_t pv_size;
  holy_uint64_t config_start;
  holy_uint64_t config_size;
} holy_PACKED;


#define LDM_MAGIC "PRIVHEAD"



static inline holy_uint64_t
read_int (holy_uint8_t *in, holy_size_t s)
{
  holy_uint8_t *ptr2;
  holy_uint64_t ret;
  ret = 0;
  for (ptr2 = in; ptr2 < in + s; ptr2++)
    {
      ret <<= 8;
      ret |= *ptr2;
    }
  return ret;
}

static int
check_ldm_partition (holy_disk_t disk __attribute__ ((unused)), const holy_partition_t p, void *data)
{
  int *has_ldm = data;

  if (p->number >= 4)
    return 1;
  if (p->msdostype == holy_PC_PARTITION_TYPE_LDM)
    {
      *has_ldm = 1;
      return 1;
    }
  return 0;
}

static int
msdos_has_ldm_partition (holy_disk_t dsk)
{
  holy_err_t err;
  int has_ldm = 0;

  err = holy_partition_msdos_iterate (dsk, check_ldm_partition, &has_ldm);
  if (err)
    {
      holy_errno = holy_ERR_NONE;
      return 0;
    }

  return has_ldm;
}

static const holy_gpt_part_type_t ldm_type = holy_GPT_PARTITION_TYPE_LDM;

/* Helper for gpt_ldm_sector.  */
static int
gpt_ldm_sector_iter (holy_disk_t disk, const holy_partition_t p, void *data)
{
  holy_disk_addr_t *sector = data;
  struct holy_gpt_partentry gptdata;
  holy_partition_t p2;

  p2 = disk->partition;
  disk->partition = p->parent;
  if (holy_disk_read (disk, p->offset, p->index,
		      sizeof (gptdata), &gptdata))
    {
      disk->partition = p2;
      return 0;
    }
  disk->partition = p2;

  if (! holy_memcmp (&gptdata.type, &ldm_type, 16))
    {
      *sector = p->start + p->len - 1;
      return 1;
    }
  return 0;
}

static holy_disk_addr_t
gpt_ldm_sector (holy_disk_t dsk)
{
  holy_disk_addr_t sector = 0;
  holy_err_t err;

  err = holy_gpt_partition_map_iterate (dsk, gpt_ldm_sector_iter, &sector);
  if (err)
    {
      holy_errno = holy_ERR_NONE;
      return 0;
    }
  return sector;
}

static struct holy_diskfilter_vg *
make_vg (holy_disk_t disk,
	 const struct holy_ldm_label *label)
{
  holy_disk_addr_t startsec, endsec, cursec;
  struct holy_diskfilter_vg *vg;
  holy_err_t err;

  /* First time we see this volume group. We've to create the
     whole volume group structure. */
  vg = holy_malloc (sizeof (*vg));
  if (! vg)
    return NULL;
  vg->extent_size = 1;
  vg->name = holy_malloc (LDM_NAME_STRLEN + 1);
  vg->uuid = holy_malloc (LDM_GUID_STRLEN + 1);
  if (! vg->uuid || !vg->name)
    {
      holy_free (vg->uuid);
      holy_free (vg->name);
      return NULL;
    }
  holy_memcpy (vg->uuid, label->group_guid, LDM_GUID_STRLEN);
  holy_memcpy (vg->name, label->group_name, LDM_NAME_STRLEN);
  vg->name[LDM_NAME_STRLEN] = 0;
  vg->uuid[LDM_GUID_STRLEN] = 0;
  vg->uuid_len = holy_strlen (vg->uuid);

  vg->lvs = NULL;
  vg->pvs = NULL;

  startsec = holy_be_to_cpu64 (label->config_start);
  endsec = startsec + holy_be_to_cpu64 (label->config_size);

  /* First find disks.  */
  for (cursec = startsec + 0x12; cursec < endsec; cursec++)
    {
      struct holy_ldm_vblk vblk[holy_DISK_SECTOR_SIZE
				/ sizeof (struct holy_ldm_vblk)];
      unsigned i;
      err = holy_disk_read (disk, cursec, 0,
			    sizeof(vblk), &vblk);
      if (err)
	goto fail2;

      for (i = 0; i < ARRAY_SIZE (vblk); i++)
	{
	  struct holy_diskfilter_pv *pv;
	  holy_uint8_t *ptr;
	  if (holy_memcmp (vblk[i].magic, LDM_VBLK_MAGIC,
			   sizeof (vblk[i].magic)) != 0)
	    continue;
	  if (holy_be_to_cpu16 (vblk[i].update_status)
	      != STATUS_CONSISTENT
	      && holy_be_to_cpu16 (vblk[i].update_status)
	      != STATUS_STILL_ACTIVE)
	    continue;
	  if (vblk[i].type != ENTRY_DISK)
	    continue;
	  pv = holy_zalloc (sizeof (*pv));
	  if (!pv)
	    goto fail2;

	  pv->disk = 0;
	  ptr = vblk[i].dynamic;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic
	      + sizeof (vblk[i].dynamic))
	    {
	      holy_free (pv);
	      goto fail2;
	    }
	  pv->internal_id = holy_malloc (ptr[0] + 2);
	  if (!pv->internal_id)
	    {
	      holy_free (pv);
	      goto fail2;
	    }
	  holy_memcpy (pv->internal_id, ptr, (holy_size_t) ptr[0] + 1);
	  pv->internal_id[(holy_size_t) ptr[0] + 1] = 0;
	  
	  ptr += *ptr + 1;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic
	      + sizeof (vblk[i].dynamic))
	    {
	      holy_free (pv);
	      goto fail2;
	    }
	  /* ptr = name.  */
	  ptr += *ptr + 1;
	  if (ptr + *ptr + 1
	      >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (pv);
	      goto fail2;
	    }
	  pv->id.uuidlen = *ptr;
	  pv->id.uuid = holy_malloc (pv->id.uuidlen + 1);
	  holy_memcpy (pv->id.uuid, ptr + 1, pv->id.uuidlen);
	  pv->id.uuid[pv->id.uuidlen] = 0;

	  pv->next = vg->pvs;
	  vg->pvs = pv;
	}
    }

  /* Then find LVs.  */
  for (cursec = startsec + 0x12; cursec < endsec; cursec++)
    {
      struct holy_ldm_vblk vblk[holy_DISK_SECTOR_SIZE
				/ sizeof (struct holy_ldm_vblk)];
      unsigned i;
      err = holy_disk_read (disk, cursec, 0,
			    sizeof(vblk), &vblk);
      if (err)
	goto fail2;

      for (i = 0; i < ARRAY_SIZE (vblk); i++)
	{
	  struct holy_diskfilter_lv *lv;
	  holy_uint8_t *ptr;
	  if (holy_memcmp (vblk[i].magic, LDM_VBLK_MAGIC,
			   sizeof (vblk[i].magic)) != 0)
	    continue;
	  if (holy_be_to_cpu16 (vblk[i].update_status)
	      != STATUS_CONSISTENT
	      && holy_be_to_cpu16 (vblk[i].update_status)
	      != STATUS_STILL_ACTIVE)
	    continue;
	  if (vblk[i].type != ENTRY_VOLUME)
	    continue;
	  lv = holy_zalloc (sizeof (*lv));
	  if (!lv)
	    goto fail2;

	  lv->vg = vg;
	  lv->segment_count = 1;
	  lv->segment_alloc = 1;
	  lv->visible = 1;
	  lv->segments = holy_zalloc (sizeof (*lv->segments));
	  if (!lv->segments)
	    goto fail2;
	  lv->segments->start_extent = 0;
	  lv->segments->type = holy_DISKFILTER_MIRROR;
	  lv->segments->node_count = 0;
	  lv->segments->node_alloc = 8;
	  lv->segments->nodes = holy_zalloc (sizeof (*lv->segments->nodes)
					     * lv->segments->node_alloc);
	  if (!lv->segments->nodes)
	    goto fail2;
	  ptr = vblk[i].dynamic;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic
	      + sizeof (vblk[i].dynamic))
	    {
	      holy_free (lv);
	      goto fail2;
	    }
	  lv->internal_id = holy_malloc ((holy_size_t) ptr[0] + 2);
	  if (!lv->internal_id)
	    {
	      holy_free (lv);
	      goto fail2;
	    }
	  holy_memcpy (lv->internal_id, ptr, ptr[0] + 1);
	  lv->internal_id[ptr[0] + 1] = 0;

	  ptr += *ptr + 1;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic
	      + sizeof (vblk[i].dynamic))
	    {
	      holy_free (lv);
	      goto fail2;
	    }
	  lv->name = holy_malloc (*ptr + 1);
	  if (!lv->name)
	    {
	      holy_free (lv->internal_id);
	      holy_free (lv);
	      goto fail2;
	    }
	  holy_memcpy (lv->name, ptr + 1, *ptr);
	  lv->name[*ptr] = 0;
	  lv->fullname = holy_xasprintf ("ldm/%s/%s",
					 vg->uuid, lv->name);
	  if (!lv->fullname)
	    {
	      holy_free (lv->internal_id);
	      holy_free (lv->name);
	      holy_free (lv);
	      goto fail2;
	    }
	  ptr += *ptr + 1;
	  if (ptr + *ptr + 1
	      >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (lv->internal_id);
	      holy_free (lv->name);
	      holy_free (lv);
	      goto fail2;
	    }
	  /* ptr = volume type.  */
	  ptr += *ptr + 1;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (lv->internal_id);
	      holy_free (lv->name);
	      holy_free (lv);
	      goto fail2;
	    }
	  /* ptr = flags.  */
	  ptr += *ptr + 1;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (lv->internal_id);
	      holy_free (lv->name);
	      holy_free (lv);
	      goto fail2;
	    }

	  /* Skip state, type, unknown, volume number, zeros, flags. */
	  ptr += 14 + 1 + 1 + 1 + 3 + 1;
	  /* ptr = number of children.  */
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (lv->internal_id);
	      holy_free (lv->name);
	      holy_free (lv);
	      goto fail2;
	    }
	  ptr += *ptr + 1;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (lv->internal_id);
	      holy_free (lv->name);
	      holy_free (lv);
	      goto fail2;
	    }

	  /* Skip 2 more fields.  */
	  ptr += 8 + 8;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic)
	      || ptr + *ptr + 1>= vblk[i].dynamic
	      + sizeof (vblk[i].dynamic))
	    {
	      holy_free (lv->internal_id);
	      holy_free (lv->name);
	      holy_free (lv);
	      goto fail2;
	    }
	  lv->size = read_int (ptr + 1, *ptr);
	  lv->segments->extent_count = lv->size;

	  lv->next = vg->lvs;
	  vg->lvs = lv;
	}
    }

  /* Now the components.  */
  for (cursec = startsec + 0x12; cursec < endsec; cursec++)
    {
      struct holy_ldm_vblk vblk[holy_DISK_SECTOR_SIZE
				/ sizeof (struct holy_ldm_vblk)];
      unsigned i;
      err = holy_disk_read (disk, cursec, 0,
			    sizeof(vblk), &vblk);
      if (err)
	goto fail2;

      for (i = 0; i < ARRAY_SIZE (vblk); i++)
	{
	  struct holy_diskfilter_lv *comp;
	  struct holy_diskfilter_lv *lv;
	  holy_uint8_t type;

	  holy_uint8_t *ptr;
	  if (holy_memcmp (vblk[i].magic, LDM_VBLK_MAGIC,
			   sizeof (vblk[i].magic)) != 0)
	    continue;
	  if (holy_be_to_cpu16 (vblk[i].update_status)
	      != STATUS_CONSISTENT
	      && holy_be_to_cpu16 (vblk[i].update_status)
	      != STATUS_STILL_ACTIVE)
	    continue;
	  if (vblk[i].type != ENTRY_COMPONENT)
	    continue;
	  comp = holy_zalloc (sizeof (*comp));
	  if (!comp)
	    goto fail2;
	  comp->visible = 0;
	  comp->name = 0;
	  comp->fullname = 0;

	  ptr = vblk[i].dynamic;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      goto fail2;
	    }
	  comp->internal_id = holy_malloc ((holy_size_t) ptr[0] + 2);
	  if (!comp->internal_id)
	    {
	      holy_free (comp);
	      goto fail2;
	    }
	  holy_memcpy (comp->internal_id, ptr, ptr[0] + 1);
	  comp->internal_id[ptr[0] + 1] = 0;

	  ptr += *ptr + 1;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (comp->internal_id);
	      holy_free (comp);
	      goto fail2;
	    }
	  /* ptr = name.  */
	  ptr += *ptr + 1;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (comp->internal_id);
	      holy_free (comp);
	      goto fail2;
	    }
	  /* ptr = state.  */
	  ptr += *ptr + 1;
	  type = *ptr++;
	  /* skip zeros.  */
	  ptr += 4;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (comp->internal_id);
	      holy_free (comp);
	      goto fail2;
	    }

	  /* ptr = number of children. */
	  ptr += *ptr + 1;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      holy_free (comp->internal_id);
	      holy_free (comp);
	      goto fail2;
	    }
	  ptr += 8 + 8;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic
	      + sizeof (vblk[i].dynamic))
	    {
	      holy_free (comp->internal_id);
	      holy_free (comp);
	      goto fail2;
	    }
	  for (lv = vg->lvs; lv; lv = lv->next)
	    {
	      if (lv->internal_id[0] == ptr[0]
		  && holy_memcmp (lv->internal_id + 1, ptr + 1, ptr[0]) == 0)
		break;
	    }
	  if (!lv)
	    {
	      holy_free (comp->internal_id);
	      holy_free (comp);
	      continue;
	    }
	  comp->size = lv->size;
	  if (type == SPANNED)
	    {
	      comp->segment_alloc = 8;
	      comp->segment_count = 0;
	      comp->segments = holy_malloc (sizeof (*comp->segments)
					    * comp->segment_alloc);
	      if (!comp->segments)
		goto fail2;
	    }
	  else
	    {
	      comp->segment_alloc = 1;
	      comp->segment_count = 1;
	      comp->segments = holy_malloc (sizeof (*comp->segments));
	      if (!comp->segments)
		goto fail2;
	      comp->segments->start_extent = 0;
	      comp->segments->extent_count = lv->size;
	      comp->segments->layout = 0;
	      if (type == STRIPE)
		comp->segments->type = holy_DISKFILTER_STRIPED;
	      else if (type == RAID5)
		{
		  comp->segments->type = holy_DISKFILTER_RAID5;
		  comp->segments->layout = holy_RAID_LAYOUT_SYMMETRIC_MASK;
		}
	      else
		goto fail2;
	      ptr += *ptr + 1;
	      ptr++;
	      if (!(vblk[i].flags & 0x10))
		goto fail2;
	      if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic)
		  || ptr + *ptr + 1 >= vblk[i].dynamic
		  + sizeof (vblk[i].dynamic))
		{
		  holy_free (comp->internal_id);
		  holy_free (comp);
		  goto fail2;
		}
	      comp->segments->stripe_size = read_int (ptr + 1, *ptr);
	      ptr += *ptr + 1;
	      if (ptr + *ptr + 1 >= vblk[i].dynamic
		  + sizeof (vblk[i].dynamic))
		{
		  holy_free (comp->internal_id);
		  holy_free (comp);
		  goto fail2;
		}
	      comp->segments->node_count = read_int (ptr + 1, *ptr);
	      comp->segments->node_alloc = comp->segments->node_count;
	      comp->segments->nodes = holy_zalloc (sizeof (*comp->segments->nodes)
						   * comp->segments->node_alloc);
	      if (!lv->segments->nodes)
		goto fail2;
	    }

	  if (lv->segments->node_alloc == lv->segments->node_count)
	    {
	      void *t;
	      lv->segments->node_alloc *= 2; 
	      t = holy_realloc (lv->segments->nodes,
				sizeof (*lv->segments->nodes)
				* lv->segments->node_alloc);
	      if (!t)
		goto fail2;
	      lv->segments->nodes = t;
	    }
	  lv->segments->nodes[lv->segments->node_count].pv = 0;
	  lv->segments->nodes[lv->segments->node_count].start = 0;
	  lv->segments->nodes[lv->segments->node_count++].lv = comp;
	  comp->next = vg->lvs;
	  vg->lvs = comp;
	}
    }
  /* Partitions.  */
  for (cursec = startsec + 0x12; cursec < endsec; cursec++)
    {
      struct holy_ldm_vblk vblk[holy_DISK_SECTOR_SIZE
				/ sizeof (struct holy_ldm_vblk)];
      unsigned i;
      err = holy_disk_read (disk, cursec, 0,
			    sizeof(vblk), &vblk);
      if (err)
	goto fail2;

      for (i = 0; i < ARRAY_SIZE (vblk); i++)
	{
	  struct holy_diskfilter_lv *comp;
	  struct holy_diskfilter_node part;
	  holy_disk_addr_t start, size;

	  holy_uint8_t *ptr;
	  part.name = 0;
	  if (holy_memcmp (vblk[i].magic, LDM_VBLK_MAGIC,
			   sizeof (vblk[i].magic)) != 0)
	    continue;
	  if (holy_be_to_cpu16 (vblk[i].update_status)
	      != STATUS_CONSISTENT
	      && holy_be_to_cpu16 (vblk[i].update_status)
	      != STATUS_STILL_ACTIVE)
	    continue;
	  if (vblk[i].type != ENTRY_PARTITION)
	    continue;
	  part.lv = 0;
	  part.pv = 0;

	  ptr = vblk[i].dynamic;
	  if (ptr + *ptr + 1 >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      goto fail2;
	    }
	  /* ID */
	  ptr += *ptr + 1;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      goto fail2;
	    }
	  /* ptr = name.  */
	  ptr += *ptr + 1;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      goto fail2;
	    }

	  /* skip zeros and logcommit id.  */
	  ptr += 4 + 8;
	  if (ptr + 16 >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      goto fail2;
	    }
	  part.start = read_int (ptr, 8);
	  start = read_int (ptr + 8, 8);
	  ptr += 16;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic)
	      || ptr + *ptr + 1 >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      goto fail2;
	    }
	  size = read_int (ptr + 1, *ptr);
	  ptr += *ptr + 1;
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic)
	      || ptr + *ptr + 1 >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      goto fail2;
	    }

	  for (comp = vg->lvs; comp; comp = comp->next)
	    if (comp->internal_id[0] == ptr[0]
		&& holy_memcmp (ptr + 1, comp->internal_id + 1,
				comp->internal_id[0]) == 0)
	      goto out;
	  continue;
	out:
	  if (ptr >= vblk[i].dynamic + sizeof (vblk[i].dynamic)
	      || ptr + *ptr + 1 >= vblk[i].dynamic + sizeof (vblk[i].dynamic))
	    {
	      goto fail2;
	    }
	  ptr += *ptr + 1;
	  struct holy_diskfilter_pv *pv;
	  for (pv = vg->pvs; pv; pv = pv->next)
	    if (pv->internal_id[0] == ptr[0]
		&& holy_memcmp (pv->internal_id + 1, ptr + 1, ptr[0]) == 0)
	      part.pv = pv;

	  if (comp->segment_alloc == 1)
	    {
	      unsigned node_index;
	      ptr += *ptr + 1;
	      if (ptr + *ptr + 1 >= vblk[i].dynamic
		  + sizeof (vblk[i].dynamic))
		{
		  goto fail2;
		}
	      node_index = read_int (ptr + 1, *ptr);
	      if (node_index < comp->segments->node_count)
		comp->segments->nodes[node_index] = part;
	    }
	  else
	    {
	      if (comp->segment_alloc == comp->segment_count)
		{
		  void *t;
		  comp->segment_alloc *= 2;
		  t = holy_realloc (comp->segments,
				    comp->segment_alloc
				    * sizeof (*comp->segments));
		  if (!t)
		    goto fail2;
		  comp->segments = t;
		}
	      comp->segments[comp->segment_count].start_extent = start;
	      comp->segments[comp->segment_count].extent_count = size;
	      comp->segments[comp->segment_count].type = holy_DISKFILTER_STRIPED;
	      comp->segments[comp->segment_count].node_count = 1;
	      comp->segments[comp->segment_count].node_alloc = 1;
	      comp->segments[comp->segment_count].nodes
		= holy_malloc (sizeof (*comp->segments[comp->segment_count].nodes));
	      if (!comp->segments[comp->segment_count].nodes)
		goto fail2;
	      comp->segments[comp->segment_count].nodes[0] = part;
	      comp->segment_count++;
	    }
	}
    }
  if (holy_diskfilter_vg_register (vg))
    goto fail2;
  return vg;
 fail2:
  {
    struct holy_diskfilter_lv *lv, *next_lv;
    struct holy_diskfilter_pv *pv, *next_pv;
    for (lv = vg->lvs; lv; lv = next_lv)
      {
	unsigned i;
	for (i = 0; i < lv->segment_count; i++)
	  holy_free (lv->segments[i].nodes);

	next_lv = lv->next;
	holy_free (lv->segments);
	holy_free (lv->internal_id);
	holy_free (lv->name);
	holy_free (lv->fullname);
	holy_free (lv);
      }
    for (pv = vg->pvs; pv; pv = next_pv)
      {
	next_pv = pv->next;
	holy_free (pv->id.uuid);
	holy_free (pv);
      }
  }
  holy_free (vg->uuid);
  holy_free (vg);
  return NULL;
}

static struct holy_diskfilter_vg *
holy_ldm_detect (holy_disk_t disk,
		 struct holy_diskfilter_pv_id *id,
		 holy_disk_addr_t *start_sector)
{
  holy_err_t err;
  struct holy_ldm_label label;
  struct holy_diskfilter_vg *vg;

#ifdef holy_UTIL
  holy_util_info ("scanning %s for LDM", disk->name);
#endif

  {
    int i;
    int has_ldm = msdos_has_ldm_partition (disk);
    for (i = 0; i < 3; i++)
      {
	holy_disk_addr_t sector = LDM_LABEL_SECTOR;
	switch (i)
	  {
	  case 0:
	    if (!has_ldm)
	      continue;
	    sector = LDM_LABEL_SECTOR;
	    break;
	  case 1:
	    /* LDM is never inside a partition.  */
	    if (!has_ldm || disk->partition)
	      continue;
	    sector = holy_disk_get_size (disk);
	    if (sector == holy_DISK_SIZE_UNKNOWN)
	      continue;
	    sector--;
	    break;
	    /* FIXME: try the third copy.  */
	  case 2:
	    sector = gpt_ldm_sector (disk);
	    if (!sector)
	      continue;
	    break;
	  }
	err = holy_disk_read (disk, sector, 0,
			      sizeof(label), &label);
	if (err)
	  return NULL;
	if (holy_memcmp (label.magic, LDM_MAGIC, sizeof (label.magic)) == 0
	    && holy_be_to_cpu16 (label.ver_major) == 0x02
	    && holy_be_to_cpu16 (label.ver_minor) >= 0x0b
	    && holy_be_to_cpu16 (label.ver_minor) <= 0x0c)
	  break;
      }

    /* Return if we didn't find a label. */
    if (i == 3)
      {
#ifdef holy_UTIL
	holy_util_info ("no LDM signature found");
#endif
	return NULL;
      }
  }

  id->uuid = holy_malloc (LDM_GUID_STRLEN + 1);
  if (!id->uuid)
    return NULL;
  holy_memcpy (id->uuid, label.disk_guid, LDM_GUID_STRLEN);
  id->uuid[LDM_GUID_STRLEN] = 0;
  id->uuidlen = holy_strlen ((char *) id->uuid);
  *start_sector = holy_be_to_cpu64 (label.pv_start);

  {
    holy_size_t s;
    for (s = 0; s < LDM_GUID_STRLEN && label.group_guid[s]; s++);
    vg = holy_diskfilter_get_vg_by_uuid (s, label.group_guid);
    if (! vg)
      vg = make_vg (disk, &label);
  }

  if (!vg)
    {
      holy_free (id->uuid);
      return NULL;
    }
  return vg;
}

#ifdef holy_UTIL

char *
holy_util_get_ldm (holy_disk_t disk, holy_disk_addr_t start)
{
  struct holy_diskfilter_pv *pv = NULL;
  struct holy_diskfilter_vg *vg = NULL;
  struct holy_diskfilter_lv *res = 0, *lv, *res_lv = 0;

  pv = holy_diskfilter_get_pv_from_disk (disk, &vg);

  if (!pv)
    return NULL;

  for (lv = vg->lvs; lv; lv = lv->next)
    if (lv->segment_count == 1 && lv->segments->node_count == 1
	&& lv->segments->type == holy_DISKFILTER_STRIPED
	&& lv->segments->nodes->pv == pv
	&& lv->segments->nodes->start + pv->start_sector == start)
      {
	res_lv = lv;
	break;
      }
  if (!res_lv)
    return NULL;
  for (lv = vg->lvs; lv; lv = lv->next)
    if (lv->segment_count == 1 && lv->segments->node_count == 1
	&& lv->segments->type == holy_DISKFILTER_MIRROR
	&& lv->segments->nodes->lv == res_lv)
      {
	res = lv;
	break;
      }
  if (res && res->fullname)
    return holy_strdup (res->fullname);
  return NULL;
}

int
holy_util_is_ldm (holy_disk_t disk)
{
  int i;
  int has_ldm = msdos_has_ldm_partition (disk);
  for (i = 0; i < 3; i++)
    {
      holy_disk_addr_t sector = LDM_LABEL_SECTOR;
      holy_err_t err;
      struct holy_ldm_label label;

      switch (i)
	{
	case 0:
	  if (!has_ldm)
	    continue;
	  sector = LDM_LABEL_SECTOR;
	  break;
	case 1:
	  /* LDM is never inside a partition.  */
	  if (!has_ldm || disk->partition)
	    continue;
	  sector = holy_disk_get_size (disk);
	  if (sector == holy_DISK_SIZE_UNKNOWN)
	    continue;
	  sector--;
	  break;
	  /* FIXME: try the third copy.  */
	case 2:
	  sector = gpt_ldm_sector (disk);
	  if (!sector)
	    continue;
	  break;
	}
      err = holy_disk_read (disk, sector, 0, sizeof(label), &label);
      if (err)
	{
	  holy_errno = holy_ERR_NONE;
	  return 0;
	}
      /* This check is more relaxed on purpose.  */
      if (holy_memcmp (label.magic, LDM_MAGIC, sizeof (label.magic)) == 0)
	return 1;
    }

  return 0;
}

holy_err_t
holy_util_ldm_embed (struct holy_disk *disk, unsigned int *nsectors,
		     unsigned int max_nsectors,
		     holy_embed_type_t embed_type,
		     holy_disk_addr_t **sectors)
{
  struct holy_diskfilter_pv *pv = NULL;
  struct holy_diskfilter_vg *vg;
  struct holy_diskfilter_lv *lv;
  unsigned i;

  if (embed_type != holy_EMBED_PCBIOS)
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "LDM currently supports only PC-BIOS embedding");
  if (disk->partition)
    return holy_error (holy_ERR_BUG, "disk isn't LDM");
  pv = holy_diskfilter_get_pv_from_disk (disk, &vg);
  if (!pv)
    return holy_error (holy_ERR_BUG, "disk isn't LDM");
  for (lv = vg->lvs; lv; lv = lv->next)
    {
      struct holy_diskfilter_lv *comp;

      if (!lv->visible || !lv->fullname)
	continue;

      if (lv->segment_count != 1)
	continue;
      if (lv->segments->type != holy_DISKFILTER_MIRROR
	  || lv->segments->node_count != 1
	  || lv->segments->start_extent != 0
	  || lv->segments->extent_count != lv->size)
	continue;

      comp = lv->segments->nodes->lv;
      if (!comp)
	continue;

      if (comp->segment_count != 1 || comp->size != lv->size)
	continue;
      if (comp->segments->type != holy_DISKFILTER_STRIPED
	  || comp->segments->node_count != 1
	  || comp->segments->start_extent != 0
	  || comp->segments->extent_count != lv->size)
	continue;

      /* How to implement proper check is to be discussed.  */
#if 1
      if (1)
	continue;
#else
      if (holy_strcmp (lv->name, "Volume5") != 0)
	continue;
#endif
      if (lv->size < *nsectors)
	return holy_error (holy_ERR_OUT_OF_RANGE,
			   /* TRANSLATORS: it's a partition for embedding,
			      not a partition embed into something. holy
			      install tools put core.img into a place
			      usable for bootloaders (called generically
			      "embedding zone") and this operation is
			      called "embedding".  */
			   N_("your LDM Embedding Partition is too small;"
			      " embedding won't be possible"));
      *nsectors = lv->size;
      if (*nsectors > max_nsectors)
	*nsectors = max_nsectors;
      *sectors = holy_malloc (*nsectors * sizeof (**sectors));
      if (!*sectors)
	return holy_errno;
      for (i = 0; i < *nsectors; i++)
	(*sectors)[i] = (lv->segments->nodes->start
			 + comp->segments->nodes->start
			 + comp->segments->nodes->pv->start_sector + i);
      return holy_ERR_NONE;
    }

  return holy_error (holy_ERR_FILE_NOT_FOUND,
		     /* TRANSLATORS: it's a partition for embedding,
			not a partition embed into something.  */
		     N_("this LDM has no Embedding Partition;"
			" embedding won't be possible"));
}
#endif

static struct holy_diskfilter holy_ldm_dev = {
  .name = "ldm",
  .detect = holy_ldm_detect,
  .next = 0
};

holy_MOD_INIT (ldm)
{
  holy_diskfilter_register_back (&holy_ldm_dev);
}

holy_MOD_FINI (ldm)
{
  holy_diskfilter_unregister (&holy_ldm_dev);
}
