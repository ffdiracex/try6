/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_DISKFILTER_H
#define holy_DISKFILTER_H	1

#include <holy/types.h>
#include <holy/list.h>

enum
  {
    holy_RAID_LAYOUT_RIGHT_MASK	= 1,
    holy_RAID_LAYOUT_SYMMETRIC_MASK = 2,
    holy_RAID_LAYOUT_MUL_FROM_POS = 4,

    holy_RAID_LAYOUT_LEFT_ASYMMETRIC = 0,
    holy_RAID_LAYOUT_RIGHT_ASYMMETRIC = holy_RAID_LAYOUT_RIGHT_MASK,
    holy_RAID_LAYOUT_LEFT_SYMMETRIC = holy_RAID_LAYOUT_SYMMETRIC_MASK,
    holy_RAID_LAYOUT_RIGHT_SYMMETRIC = (holy_RAID_LAYOUT_RIGHT_MASK
					| holy_RAID_LAYOUT_SYMMETRIC_MASK)
  };


struct holy_diskfilter_vg {
  char *uuid;
  holy_size_t uuid_len;
  /* Optional.  */
  char *name;
  holy_uint64_t extent_size;
  struct holy_diskfilter_pv *pvs;
  struct holy_diskfilter_lv *lvs;
  struct holy_diskfilter_vg *next;

#ifdef holy_UTIL
  struct holy_diskfilter *driver;
#endif
};

struct holy_diskfilter_pv_id {
  union
  {
    char *uuid;
    int id;
  };
  holy_size_t uuidlen;
};

struct holy_diskfilter_pv {
  struct holy_diskfilter_pv_id id;
  /* Optional.  */
  char *name;
  holy_disk_t disk;
  holy_disk_addr_t part_start;
  holy_disk_addr_t part_size;
  holy_disk_addr_t start_sector; /* Sector number where the data area starts. */
  struct holy_diskfilter_pv *next;
  /* Optional.  */
  holy_uint8_t *internal_id;
#ifdef holy_UTIL
  char **partmaps;
#endif
};

struct holy_diskfilter_lv {
  /* Name used for disk.  */
  char *fullname;
  char *idname;
  /* Optional.  */
  char *name;
  int number;
  unsigned int segment_count;
  holy_size_t segment_alloc;
  holy_uint64_t size;
  int became_readable_at;
  int scanned;
  int visible;

  /* Pointer to segment_count segments. */
  struct holy_diskfilter_segment *segments;
  struct holy_diskfilter_vg *vg;
  struct holy_diskfilter_lv *next;

  /* Optional.  */
  char *internal_id;
};

struct holy_diskfilter_segment {
  holy_uint64_t start_extent;
  holy_uint64_t extent_count;
  enum 
    {
      holy_DISKFILTER_STRIPED = 0,
      holy_DISKFILTER_MIRROR = 1,
      holy_DISKFILTER_RAID4 = 4,
      holy_DISKFILTER_RAID5 = 5,
      holy_DISKFILTER_RAID6 = 6,
      holy_DISKFILTER_RAID10 = 10,
  } type;
  int layout;
  /* valid only for raid10.  */
  holy_uint64_t raid_member_size;

  unsigned int node_count;
  unsigned int node_alloc;
  struct holy_diskfilter_node *nodes;

  unsigned int stripe_size;
};

struct holy_diskfilter_node {
  holy_disk_addr_t start;
  /* Optional.  */
  char *name;
  struct holy_diskfilter_pv *pv;
  struct holy_diskfilter_lv *lv;
};

struct holy_diskfilter_vg *
holy_diskfilter_get_vg_by_uuid (holy_size_t uuidlen, char *uuid);

struct holy_diskfilter
{
  struct holy_diskfilter *next;
  struct holy_diskfilter **prev;

  const char *name;

  struct holy_diskfilter_vg * (*detect) (holy_disk_t disk,
					 struct holy_diskfilter_pv_id *id,
					 holy_disk_addr_t *start_sector);
};
typedef struct holy_diskfilter *holy_diskfilter_t;

extern holy_diskfilter_t holy_diskfilter_list;
static inline void
holy_diskfilter_register_front (holy_diskfilter_t diskfilter)
{
  holy_list_push (holy_AS_LIST_P (&holy_diskfilter_list),
		  holy_AS_LIST (diskfilter));
}

static inline void
holy_diskfilter_register_back (holy_diskfilter_t diskfilter)
{
  holy_diskfilter_t p, *q;
  for (q = &holy_diskfilter_list, p = *q; p; q = &p->next, p = *q);
  diskfilter->next = NULL;
  diskfilter->prev = q;
  *q = diskfilter;
}
static inline void
holy_diskfilter_unregister (holy_diskfilter_t diskfilter)
{
  holy_list_remove (holy_AS_LIST (diskfilter));
}

struct holy_diskfilter_vg *
holy_diskfilter_make_raid (holy_size_t uuidlen, char *uuid, int nmemb,
			   const char *name, holy_uint64_t disk_size,
			   holy_uint64_t stripe_size,
			   int layout, int level);

typedef holy_err_t (*holy_raid5_recover_func_t) (struct holy_diskfilter_segment *array,
                                                 int disknr, char *buf,
                                                 holy_disk_addr_t sector,
                                                 holy_size_t size);

typedef holy_err_t (*holy_raid6_recover_func_t) (struct holy_diskfilter_segment *array,
                                                 int disknr, int p, char *buf,
                                                 holy_disk_addr_t sector,
                                                 holy_size_t size);

extern holy_raid5_recover_func_t holy_raid5_recover_func;
extern holy_raid6_recover_func_t holy_raid6_recover_func;

holy_err_t holy_diskfilter_vg_register (struct holy_diskfilter_vg *vg);

holy_err_t
holy_diskfilter_read_node (const struct holy_diskfilter_node *node,
			   holy_disk_addr_t sector,
			   holy_size_t size, char *buf);

#ifdef holy_UTIL
struct holy_diskfilter_pv *
holy_diskfilter_get_pv_from_disk (holy_disk_t disk,
				  struct holy_diskfilter_vg **vg);
void
holy_diskfilter_get_partmap (holy_disk_t disk,
			     void (*cb) (const char *val, void *data),
			     void *data);
#endif

#endif /* ! holy_RAID_H */
