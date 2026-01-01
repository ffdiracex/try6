/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/term.h>
#include <holy/i18n.h>
#include <holy/xen.h>
#include <holy/time.h>
#include <xen/io/blkif.h>

struct virtdisk
{
  int handle;
  char *fullname;
  char *backend_dir;
  char *frontend_dir;
  struct blkif_sring *shared_page;
  struct blkif_front_ring ring;
  holy_xen_grant_t grant;
  holy_xen_evtchn_t evtchn;
  void *dma_page;
  holy_xen_grant_t dma_grant;
  struct virtdisk *compat_next;
};

#define xen_wmb() mb()
#define xen_mb() mb()

static struct virtdisk *virtdisks;
static holy_size_t vdiskcnt;
struct virtdisk *compat_head;

static int
holy_virtdisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		       holy_disk_pull_t pull)
{
  holy_size_t i;

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  for (i = 0; i < vdiskcnt; i++)
    if (hook (virtdisks[i].fullname, hook_data))
      return 1;
  return 0;
}

static holy_err_t
holy_virtdisk_open (const char *name, holy_disk_t disk)
{
  int i;
  holy_uint32_t secsize;
  char fdir[200];
  char *buf;
  int num = -1;
  struct virtdisk *vd;

  /* For compatibility with pv-holy legacy menu.lst accept hdX as disk name */
  if (name[0] == 'h' && name[1] == 'd' && name[2])
    {
      num = holy_strtoul (name + 2, 0, 10);
      if (holy_errno)
	{
	  holy_errno = 0;
	  num = -1;
	}
    }
  for (i = 0, vd = compat_head; vd; vd = vd->compat_next, i++)
    if (i == num || holy_strcmp (name, vd->fullname) == 0)
      break;
  if (!vd)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a virtdisk");
  disk->data = vd;
  disk->id = vd - virtdisks;

  holy_snprintf (fdir, sizeof (fdir), "%s/sectors", vd->backend_dir);
  buf = holy_xenstore_get_file (fdir, NULL);
  if (!buf)
    return holy_errno;
  disk->total_sectors = holy_strtoull (buf, 0, 10);
  if (holy_errno)
    return holy_errno;

  holy_snprintf (fdir, sizeof (fdir), "%s/sector-size", vd->backend_dir);
  buf = holy_xenstore_get_file (fdir, NULL);
  if (!buf)
    return holy_errno;
  secsize = holy_strtoull (buf, 0, 10);
  if (holy_errno)
    return holy_errno;

  if ((secsize & (secsize - 1)) || !secsize || secsize < 512
      || secsize > holy_XEN_PAGE_SIZE)
    return holy_error (holy_ERR_IO, "unsupported sector size %d", secsize);

  for (disk->log_sector_size = 0;
       (1U << disk->log_sector_size) < secsize; disk->log_sector_size++);

  disk->total_sectors >>= disk->log_sector_size - 9;

  return holy_ERR_NONE;
}

static void
holy_virtdisk_close (holy_disk_t disk __attribute__ ((unused)))
{
}

static holy_err_t
holy_virtdisk_read (holy_disk_t disk, holy_disk_addr_t sector,
		    holy_size_t size, char *buf)
{
  struct virtdisk *data = disk->data;

  while (size)
    {
      holy_size_t cur;
      struct blkif_request *req;
      struct blkif_response *resp;
      int sta = 0;
      struct evtchn_send send;
      cur = size;
      if (cur > (unsigned) (holy_XEN_PAGE_SIZE >> disk->log_sector_size))
	cur = holy_XEN_PAGE_SIZE >> disk->log_sector_size;
      while (RING_FULL (&data->ring))
	holy_xen_sched_op (SCHEDOP_yield, 0);
      req = RING_GET_REQUEST (&data->ring, data->ring.req_prod_pvt);
      req->operation = BLKIF_OP_READ;
      req->nr_segments = 1;
      req->handle = data->handle;
      req->id = 0;
      req->sector_number = sector << (disk->log_sector_size - 9);
      req->seg[0].gref = data->dma_grant;
      req->seg[0].first_sect = 0;
      req->seg[0].last_sect = (cur << (disk->log_sector_size - 9)) - 1;
      data->ring.req_prod_pvt++;
      RING_PUSH_REQUESTS (&data->ring);
      mb ();
      send.port = data->evtchn;
      holy_xen_event_channel_op (EVTCHNOP_send, &send);

      while (!RING_HAS_UNCONSUMED_RESPONSES (&data->ring))
	{
	  holy_xen_sched_op (SCHEDOP_yield, 0);
	  mb ();
	}
      while (1)
	{
	  int wtd;
	  RING_FINAL_CHECK_FOR_RESPONSES (&data->ring, wtd);
	  if (!wtd)
	    break;
	  resp = RING_GET_RESPONSE (&data->ring, data->ring.rsp_cons);
	  data->ring.rsp_cons++;
	  if (resp->status)
	    sta = resp->status;
	}
      if (sta)
	return holy_error (holy_ERR_IO, "read failed");
      holy_memcpy (buf, data->dma_page, cur << disk->log_sector_size);
      size -= cur;
      sector += cur;
      buf += cur << disk->log_sector_size;
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_virtdisk_write (holy_disk_t disk, holy_disk_addr_t sector,
		     holy_size_t size, const char *buf)
{
  struct virtdisk *data = disk->data;

  while (size)
    {
      holy_size_t cur;
      struct blkif_request *req;
      struct blkif_response *resp;
      int sta = 0;
      struct evtchn_send send;
      cur = size;
      if (cur > (unsigned) (holy_XEN_PAGE_SIZE >> disk->log_sector_size))
	cur = holy_XEN_PAGE_SIZE >> disk->log_sector_size;

      holy_memcpy (data->dma_page, buf, cur << disk->log_sector_size);

      while (RING_FULL (&data->ring))
	holy_xen_sched_op (SCHEDOP_yield, 0);
      req = RING_GET_REQUEST (&data->ring, data->ring.req_prod_pvt);
      req->operation = BLKIF_OP_WRITE;
      req->nr_segments = 1;
      req->handle = data->handle;
      req->id = 0;
      req->sector_number = sector << (disk->log_sector_size - 9);
      req->seg[0].gref = data->dma_grant;
      req->seg[0].first_sect = 0;
      req->seg[0].last_sect = (cur << (disk->log_sector_size - 9)) - 1;
      data->ring.req_prod_pvt++;
      RING_PUSH_REQUESTS (&data->ring);
      mb ();
      send.port = data->evtchn;
      holy_xen_event_channel_op (EVTCHNOP_send, &send);

      while (!RING_HAS_UNCONSUMED_RESPONSES (&data->ring))
	{
	  holy_xen_sched_op (SCHEDOP_yield, 0);
	  mb ();
	}
      while (1)
	{
	  int wtd;
	  RING_FINAL_CHECK_FOR_RESPONSES (&data->ring, wtd);
	  if (!wtd)
	    break;
	  resp = RING_GET_RESPONSE (&data->ring, data->ring.rsp_cons);
	  data->ring.rsp_cons++;
	  if (resp->status)
	    sta = resp->status;
	}
      if (sta)
	return holy_error (holy_ERR_IO, "write failed");
      size -= cur;
      sector += cur;
      buf += cur << disk->log_sector_size;
    }
  return holy_ERR_NONE;
}

static struct holy_disk_dev holy_virtdisk_dev = {
  .name = "xen",
  .id = holy_DISK_DEVICE_XEN,
  .iterate = holy_virtdisk_iterate,
  .open = holy_virtdisk_open,
  .close = holy_virtdisk_close,
  .read = holy_virtdisk_read,
  .write = holy_virtdisk_write,
  .next = 0
};

static int
count (const char *dir __attribute__ ((unused)), void *data)
{
  holy_size_t *ctr = data;
  (*ctr)++;

  return 0;
}

static int
fill (const char *dir, void *data)
{
  holy_size_t *ctr = data;
  domid_t dom;
  /* "dir" is just a number, at most 19 characters. */
  char fdir[200];
  char num[20];
  holy_err_t err;
  void *buf;
  struct evtchn_alloc_unbound alloc_unbound;
  struct virtdisk **prev = &compat_head, *vd = compat_head;

  /* Shouldn't happen unles some hotplug happened.  */
  if (vdiskcnt >= *ctr)
    return 1;
  virtdisks[vdiskcnt].handle = holy_strtoul (dir, 0, 10);
  if (holy_errno)
    {
      holy_errno = 0;
      return 0;
    }
  virtdisks[vdiskcnt].fullname = 0;
  virtdisks[vdiskcnt].backend_dir = 0;

  holy_snprintf (fdir, sizeof (fdir), "device/vbd/%s/backend", dir);
  virtdisks[vdiskcnt].backend_dir = holy_xenstore_get_file (fdir, NULL);
  if (!virtdisks[vdiskcnt].backend_dir)
    goto out_fail_1;

  holy_snprintf (fdir, sizeof (fdir), "%s/dev",
		 virtdisks[vdiskcnt].backend_dir);
  buf = holy_xenstore_get_file (fdir, NULL);
  if (!buf)
    {
      holy_errno = 0;
      virtdisks[vdiskcnt].fullname = holy_xasprintf ("xenid/%s", dir);
    }
  else
    {
      virtdisks[vdiskcnt].fullname = holy_xasprintf ("xen/%s", (char *) buf);
      holy_free (buf);
    }
  if (!virtdisks[vdiskcnt].fullname)
    goto out_fail_1;

  holy_snprintf (fdir, sizeof (fdir), "device/vbd/%s/backend-id", dir);
  buf = holy_xenstore_get_file (fdir, NULL);
  if (!buf)
    goto out_fail_1;

  dom = holy_strtoul (buf, 0, 10);
  holy_free (buf);
  if (holy_errno)
    goto out_fail_1;

  virtdisks[vdiskcnt].shared_page =
    holy_xen_alloc_shared_page (dom, &virtdisks[vdiskcnt].grant);
  if (!virtdisks[vdiskcnt].shared_page)
    goto out_fail_1;

  virtdisks[vdiskcnt].dma_page =
    holy_xen_alloc_shared_page (dom, &virtdisks[vdiskcnt].dma_grant);
  if (!virtdisks[vdiskcnt].dma_page)
    goto out_fail_2;

  alloc_unbound.dom = DOMID_SELF;
  alloc_unbound.remote_dom = dom;

  holy_xen_event_channel_op (EVTCHNOP_alloc_unbound, &alloc_unbound);
  virtdisks[vdiskcnt].evtchn = alloc_unbound.port;

  SHARED_RING_INIT (virtdisks[vdiskcnt].shared_page);
  FRONT_RING_INIT (&virtdisks[vdiskcnt].ring, virtdisks[vdiskcnt].shared_page,
		   holy_XEN_PAGE_SIZE);

  holy_snprintf (fdir, sizeof (fdir), "device/vbd/%s/ring-ref", dir);
  holy_snprintf (num, sizeof (num), "%u", virtdisks[vdiskcnt].grant);
  err = holy_xenstore_write_file (fdir, num, holy_strlen (num));
  if (err)
    goto out_fail_3;

  holy_snprintf (fdir, sizeof (fdir), "device/vbd/%s/event-channel", dir);
  holy_snprintf (num, sizeof (num), "%u", virtdisks[vdiskcnt].evtchn);
  err = holy_xenstore_write_file (fdir, num, holy_strlen (num));
  if (err)
    goto out_fail_3;

  holy_snprintf (fdir, sizeof (fdir), "device/vbd/%s/protocol", dir);
  err = holy_xenstore_write_file (fdir, XEN_IO_PROTO_ABI_NATIVE,
				  holy_strlen (XEN_IO_PROTO_ABI_NATIVE));
  if (err)
    goto out_fail_3;

  struct gnttab_dump_table dt;
  dt.dom = DOMID_SELF;
  holy_xen_grant_table_op (GNTTABOP_dump_table, (void *) &dt, 1);

  holy_snprintf (fdir, sizeof (fdir), "device/vbd/%s/state", dir);
  err = holy_xenstore_write_file (fdir, "3", 1);
  if (err)
    goto out_fail_3;

  while (1)
    {
      holy_snprintf (fdir, sizeof (fdir), "%s/state",
		     virtdisks[vdiskcnt].backend_dir);
      buf = holy_xenstore_get_file (fdir, NULL);
      if (!buf)
	goto out_fail_3;
      if (holy_strcmp (buf, "2") != 0)
	break;
      holy_free (buf);
      holy_xen_sched_op (SCHEDOP_yield, 0);
    }
  holy_dprintf ("xen", "state=%s\n", (char *) buf);
  holy_free (buf);

  holy_snprintf (fdir, sizeof (fdir), "device/vbd/%s", dir);

  virtdisks[vdiskcnt].frontend_dir = holy_strdup (fdir);

  /* For compatibility with pv-holy maintain linked list sorted by handle
     value in increasing order. This allows mapping of (hdX) disk names
     from legacy menu.lst */
  while (vd)
    {
      if (vd->handle > virtdisks[vdiskcnt].handle)
	break;
      prev = &vd->compat_next;
      vd = vd->compat_next;
    }
  virtdisks[vdiskcnt].compat_next = vd;
  *prev = &virtdisks[vdiskcnt];

  vdiskcnt++;
  return 0;

out_fail_3:
  holy_xen_free_shared_page (virtdisks[vdiskcnt].dma_page);
out_fail_2:
  holy_xen_free_shared_page (virtdisks[vdiskcnt].shared_page);
out_fail_1:
  holy_free (virtdisks[vdiskcnt].backend_dir);
  holy_free (virtdisks[vdiskcnt].fullname);

  holy_errno = 0;
  return 0;
}

void
holy_xendisk_init (void)
{
  holy_size_t ctr = 0;
  if (holy_xenstore_dir ("device/vbd", count, &ctr))
    holy_errno = 0;

  if (!ctr)
    return;

  virtdisks = holy_malloc (ctr * sizeof (virtdisks[0]));
  if (!virtdisks)
    return;
  if (holy_xenstore_dir ("device/vbd", fill, &ctr))
    holy_errno = 0;

  holy_disk_dev_register (&holy_virtdisk_dev);
}

void
holy_xendisk_fini (void)
{
  char fdir[200];
  unsigned i;

  for (i = 0; i < vdiskcnt; i++)
    {
      char *buf;
      struct evtchn_close close_op = {.port = virtdisks[i].evtchn };

      holy_snprintf (fdir, sizeof (fdir), "%s/state",
		     virtdisks[i].frontend_dir);
      holy_xenstore_write_file (fdir, "6", 1);

      while (1)
	{
	  holy_snprintf (fdir, sizeof (fdir), "%s/state",
			 virtdisks[i].backend_dir);
	  buf = holy_xenstore_get_file (fdir, NULL);
	  holy_dprintf ("xen", "state=%s\n", (char *) buf);

	  if (!buf || holy_strcmp (buf, "6") == 0)
	    break;
	  holy_free (buf);
	  holy_xen_sched_op (SCHEDOP_yield, 0);
	}
      holy_free (buf);

      holy_snprintf (fdir, sizeof (fdir), "%s/ring-ref",
		     virtdisks[i].frontend_dir);
      holy_xenstore_write_file (fdir, NULL, 0);

      holy_snprintf (fdir, sizeof (fdir), "%s/event-channel",
		     virtdisks[i].frontend_dir);
      holy_xenstore_write_file (fdir, NULL, 0);

      holy_xen_free_shared_page (virtdisks[i].dma_page);
      holy_xen_free_shared_page (virtdisks[i].shared_page);

      holy_xen_event_channel_op (EVTCHNOP_close, &close_op);

      /* Prepare for handoff.  */
      holy_snprintf (fdir, sizeof (fdir), "%s/state",
		     virtdisks[i].frontend_dir);
      holy_xenstore_write_file (fdir, "1", 1);
    }
}
