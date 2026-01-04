/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/xen.h>
#include <holy/term.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/mm.h>
#include <holy/kernel.h>
#include <holy/offsets.h>
#include <holy/memory.h>
#include <holy/i386/tsc.h>
#include <holy/term.h>
#include <holy/loader.h>

holy_addr_t holy_modbase;
struct start_info *holy_xen_start_page_addr;
volatile struct xencons_interface *holy_xen_xcons;
volatile struct shared_info *holy_xen_shared_info;
volatile struct xenstore_domain_interface *holy_xen_xenstore;
volatile grant_entry_v1_t *holy_xen_grant_table;
static const holy_size_t total_grants =
  holy_XEN_PAGE_SIZE / sizeof (holy_xen_grant_table[0]);
holy_size_t holy_xen_n_allocated_shared_pages;

static holy_xen_mfn_t
holy_xen_ptr2mfn (void *ptr)
{
  holy_xen_mfn_t *mfn_list =
    (holy_xen_mfn_t *) holy_xen_start_page_addr->mfn_list;
  return mfn_list[(holy_addr_t) ptr >> holy_XEN_LOG_PAGE_SIZE];
}

void *
holy_xen_alloc_shared_page (domid_t dom, holy_xen_grant_t * grnum)
{
  void *ret;
  holy_xen_mfn_t mfn;
  volatile grant_entry_v1_t *entry;

  /* Avoid 0.  */
  for (entry = holy_xen_grant_table;
       entry < holy_xen_grant_table + total_grants; entry++)
    if (!entry->flags)
      break;

  if (entry == holy_xen_grant_table + total_grants)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, "out of grant entries");
      return NULL;
    }
  ret = holy_memalign (holy_XEN_PAGE_SIZE, holy_XEN_PAGE_SIZE);
  if (!ret)
    return NULL;
  mfn = holy_xen_ptr2mfn (ret);
  entry->frame = mfn;
  entry->domid = dom;
  mb ();
  entry->flags = GTF_permit_access;
  mb ();
  *grnum = entry - holy_xen_grant_table;
  holy_xen_n_allocated_shared_pages++;
  return ret;
}

void
holy_xen_free_shared_page (void *ptr)
{
  holy_xen_mfn_t mfn;
  volatile grant_entry_v1_t *entry;

  mfn = holy_xen_ptr2mfn (ptr);
  for (entry = holy_xen_grant_table + 1;
       entry < holy_xen_grant_table + total_grants; entry++)
    if (entry->flags && entry->frame == mfn)
      {
	mb ();
	entry->flags = 0;
	mb ();
	entry->frame = 0;
	mb ();
      }
  holy_xen_n_allocated_shared_pages--;
}

void
holy_machine_get_bootlocation (char **device __attribute__ ((unused)),
			       char **path __attribute__ ((unused)))
{
}

static holy_uint8_t window[holy_XEN_PAGE_SIZE]
  __attribute__ ((aligned (holy_XEN_PAGE_SIZE)));

#ifdef __x86_64__
#define NUMBER_OF_LEVELS 4
#else
#define NUMBER_OF_LEVELS 3
#endif

#define LOG_POINTERS_PER_PAGE 9
#define POINTERS_PER_PAGE (1 << LOG_POINTERS_PER_PAGE)

void
holy_xen_store_send (const void *buf_, holy_size_t len)
{
  const holy_uint8_t *buf = buf_;
  struct evtchn_send send;
  int event_sent = 0;
  while (len)
    {
      holy_size_t avail, inbuf;
      holy_size_t prod, cons;
      mb ();
      prod = holy_xen_xenstore->req_prod;
      cons = holy_xen_xenstore->req_cons;
      if (prod >= cons + sizeof (holy_xen_xenstore->req))
	{
	  if (!event_sent)
	    {
	      send.port = holy_xen_start_page_addr->store_evtchn;
	      holy_xen_event_channel_op (EVTCHNOP_send, &send);
	      event_sent = 1;
	    }
	  holy_xen_sched_op (SCHEDOP_yield, 0);
	  continue;
	}
      event_sent = 0;
      avail = cons + sizeof (holy_xen_xenstore->req) - prod;
      inbuf = (~prod & (sizeof (holy_xen_xenstore->req) - 1)) + 1;
      if (avail > inbuf)
	avail = inbuf;
      if (avail > len)
	avail = len;
      holy_memcpy ((void *) &holy_xen_xenstore->req[prod & (sizeof (holy_xen_xenstore->req) - 1)],
		   buf, avail);
      buf += avail;
      len -= avail;
      mb ();
      holy_xen_xenstore->req_prod += avail;
      mb ();
      if (!event_sent)
	{
	  send.port = holy_xen_start_page_addr->store_evtchn;
	  holy_xen_event_channel_op (EVTCHNOP_send, &send);
	  event_sent = 1;
	}
      holy_xen_sched_op (SCHEDOP_yield, 0);
    }
}

void
holy_xen_store_recv (void *buf_, holy_size_t len)
{
  holy_uint8_t *buf = buf_;
  struct evtchn_send send;
  int event_sent = 0;
  while (len)
    {
      holy_size_t avail, inbuf;
      holy_size_t prod, cons;
      mb ();
      prod = holy_xen_xenstore->rsp_prod;
      cons = holy_xen_xenstore->rsp_cons;
      if (prod <= cons)
	{
	  if (!event_sent)
	    {
	      send.port = holy_xen_start_page_addr->store_evtchn;
	      holy_xen_event_channel_op (EVTCHNOP_send, &send);
	      event_sent = 1;
	    }
	  holy_xen_sched_op (SCHEDOP_yield, 0);
	  continue;
	}
      event_sent = 0;
      avail = prod - cons;
      inbuf = (~cons & (sizeof (holy_xen_xenstore->req) - 1)) + 1;
      if (avail > inbuf)
	avail = inbuf;
      if (avail > len)
	avail = len;
      holy_memcpy (buf,
		   (void *) &holy_xen_xenstore->rsp[cons & (sizeof (holy_xen_xenstore->rsp) - 1)],
		   avail);
      buf += avail;
      len -= avail;
      mb ();
      holy_xen_xenstore->rsp_cons += avail;
      mb ();
      if (!event_sent)
	{
	  send.port = holy_xen_start_page_addr->store_evtchn;
	  holy_xen_event_channel_op(EVTCHNOP_send, &send);
	  event_sent = 1;
	}
      holy_xen_sched_op(SCHEDOP_yield, 0);
    }
}

void *
holy_xenstore_get_file (const char *dir, holy_size_t *len)
{
  struct xsd_sockmsg msg;
  char *buf;
  holy_size_t dirlen = holy_strlen (dir) + 1;

  if (len)
    *len = 0;

  holy_memset (&msg, 0, sizeof (msg));
  msg.type = XS_READ;
  msg.len = dirlen;
  holy_xen_store_send (&msg, sizeof (msg));
  holy_xen_store_send (dir, dirlen);
  holy_xen_store_recv (&msg, sizeof (msg));
  buf = holy_malloc (msg.len + 1);
  if (!buf)
    return NULL;
  holy_dprintf ("xen", "msg type = %d, len = %d\n", msg.type, msg.len);
  holy_xen_store_recv (buf, msg.len);
  buf[msg.len] = '\0';
  if (msg.type == XS_ERROR)
    {
      holy_error (holy_ERR_IO, "couldn't read xenstorage `%s': %s", dir, buf);
      holy_free (buf);
      return NULL;
    }
  if (len)
    *len = msg.len;
  return buf;
}

holy_err_t
holy_xenstore_write_file (const char *dir, const void *buf, holy_size_t len)
{
  struct xsd_sockmsg msg;
  holy_size_t dirlen = holy_strlen (dir) + 1;
  char *resp;

  holy_memset (&msg, 0, sizeof (msg));
  msg.type = XS_WRITE;
  msg.len = dirlen + len;
  holy_xen_store_send (&msg, sizeof (msg));
  holy_xen_store_send (dir, dirlen);
  holy_xen_store_send (buf, len);
  holy_xen_store_recv (&msg, sizeof (msg));
  resp = holy_malloc (msg.len + 1);
  if (!resp)
    return holy_errno;
  holy_dprintf ("xen", "msg type = %d, len = %d\n", msg.type, msg.len);
  holy_xen_store_recv (resp, msg.len);
  resp[msg.len] = '\0';
  if (msg.type == XS_ERROR)
    {
      holy_dprintf ("xen", "error = %s\n", resp);
      holy_error (holy_ERR_IO, "couldn't read xenstorage `%s': %s",
		  dir, resp);
      holy_free (resp);
      return holy_errno;
    }
  holy_free (resp);
  return holy_ERR_NONE;
}

/* FIXME: error handling.  */
holy_err_t
holy_xenstore_dir (const char *dir,
		   int (*hook) (const char *dir, void *hook_data),
		   void *hook_data)
{
  struct xsd_sockmsg msg;
  char *buf;
  char *ptr;
  holy_size_t dirlen = holy_strlen (dir) + 1;

  holy_memset (&msg, 0, sizeof (msg));
  msg.type = XS_DIRECTORY;
  msg.len = dirlen;
  holy_xen_store_send (&msg, sizeof (msg));
  holy_xen_store_send (dir, dirlen);
  holy_xen_store_recv (&msg, sizeof (msg));
  buf = holy_malloc (msg.len + 1);
  if (!buf)
    return holy_errno;
  holy_dprintf ("xen", "msg type = %d, len = %d\n", msg.type, msg.len);
  holy_xen_store_recv (buf, msg.len);
  buf[msg.len] = '\0';
  if (msg.type == XS_ERROR)
    {
      holy_err_t err;
      err = holy_error (holy_ERR_IO, "couldn't read xenstorage `%s': %s",
			dir, buf);
      holy_free (buf);
      return err;
    }
  for (ptr = buf; ptr < buf + msg.len; ptr += holy_strlen (ptr) + 1)
    if (hook (ptr, hook_data))
      break;
  holy_free (buf);
  return holy_errno;
}

unsigned long gntframe = 0;

#define MAX_N_UNUSABLE_PAGES 4

static int
holy_xen_is_page_usable (holy_xen_mfn_t mfn)
{
  if (mfn == holy_xen_start_page_addr->console.domU.mfn)
    return 0;
  if (mfn == holy_xen_start_page_addr->shared_info)
    return 0;
  if (mfn == holy_xen_start_page_addr->store_mfn)
    return 0;
  if (mfn == gntframe)
    return 0;
  return 1;
}

static holy_uint64_t
page2offset (holy_uint64_t page)
{
  return page << 12;
}

#if defined (__x86_64__) && defined (__code_model_large__)
#define MAX_TOTAL_PAGES (1LL << (64 - 12))
#elif defined (__x86_64__)
#define MAX_TOTAL_PAGES (1LL << (31 - 12))
#else
#define MAX_TOTAL_PAGES (1LL << (32 - 12))
#endif

static void
map_all_pages (void)
{
  holy_uint64_t total_pages = holy_xen_start_page_addr->nr_pages;
  holy_uint64_t i, j;
  holy_xen_mfn_t *mfn_list =
    (holy_xen_mfn_t *) holy_xen_start_page_addr->mfn_list;
  holy_uint64_t *pg = (holy_uint64_t *) window;
  holy_uint64_t oldpgstart, oldpgend;
  struct gnttab_setup_table gnttab_setup;
  struct gnttab_set_version gnttab_setver;
  holy_size_t n_unusable_pages = 0;
  struct mmu_update m2p_updates[2 * MAX_N_UNUSABLE_PAGES];

  if (total_pages > MAX_TOTAL_PAGES - 4)
    total_pages = MAX_TOTAL_PAGES - 4;

  holy_memset (&gnttab_setver, 0, sizeof (gnttab_setver));

  gnttab_setver.version = 1;
  holy_xen_grant_table_op (GNTTABOP_set_version, &gnttab_setver, 1);

  holy_memset (&gnttab_setup, 0, sizeof (gnttab_setup));
  gnttab_setup.dom = DOMID_SELF;
  gnttab_setup.nr_frames = 1;
  gnttab_setup.frame_list.p = &gntframe;

  holy_xen_grant_table_op (GNTTABOP_setup_table, &gnttab_setup, 1);

  for (j = 0; j < total_pages - n_unusable_pages; j++)
    while (!holy_xen_is_page_usable (mfn_list[j]))
      {
	holy_xen_mfn_t t;
	if (n_unusable_pages >= MAX_N_UNUSABLE_PAGES)
	  {
	    struct sched_shutdown arg;
	    arg.reason = SHUTDOWN_crash;
	    holy_xen_sched_op (SCHEDOP_shutdown, &arg);
	    while (1);
	  }
	t = mfn_list[j];
	mfn_list[j] = mfn_list[total_pages - n_unusable_pages - 1];
	mfn_list[total_pages - n_unusable_pages - 1] = t;

	m2p_updates[2 * n_unusable_pages].ptr
	  = page2offset (mfn_list[j]) | MMU_MACHPHYS_UPDATE;
	m2p_updates[2 * n_unusable_pages].val = j;
	m2p_updates[2 * n_unusable_pages + 1].ptr
	  = page2offset (mfn_list[total_pages - n_unusable_pages - 1])
	  | MMU_MACHPHYS_UPDATE;
	m2p_updates[2 * n_unusable_pages + 1].val = total_pages
	  - n_unusable_pages - 1;

	n_unusable_pages++;
      }

  holy_xen_mmu_update (m2p_updates, 2 * n_unusable_pages, NULL, DOMID_SELF);

  total_pages += 4;

  holy_uint64_t lx[NUMBER_OF_LEVELS], nlx;
  holy_uint64_t paging_start = total_pages - 4 - n_unusable_pages, curpage;

  for (nlx = total_pages, i = 0; i < (unsigned) NUMBER_OF_LEVELS; i++)
    {
      nlx = (nlx + POINTERS_PER_PAGE - 1) >> LOG_POINTERS_PER_PAGE;
      /* PAE wants all 4 root directories present.  */
#ifdef __i386__
      if (i == 1)
	nlx = 4;
#endif
      lx[i] = nlx;
      paging_start -= nlx;
    }

  oldpgstart = holy_xen_start_page_addr->pt_base >> 12;
  oldpgend = oldpgstart + holy_xen_start_page_addr->nr_pt_frames;

  curpage = paging_start;

  int l;

  for (l = NUMBER_OF_LEVELS - 1; l >= 1; l--)
    {
      for (i = 0; i < lx[l]; i++)
	{
	  holy_xen_update_va_mapping (&window,
				      page2offset (mfn_list[curpage + i]) | 7,
				      UVMF_INVLPG);
	  holy_memset (&window, 0, sizeof (window));

	  for (j = i * POINTERS_PER_PAGE;
	       j < (i + 1) * POINTERS_PER_PAGE && j < lx[l - 1]; j++)
	    pg[j - i * POINTERS_PER_PAGE] =
	      page2offset (mfn_list[curpage + lx[l] + j])
#ifdef __x86_64__
	      | 4
#endif
	      | 3;
	}
      curpage += lx[l];
    }

  for (i = 0; i < lx[0]; i++)
    {
      holy_xen_update_va_mapping (&window,
				  page2offset (mfn_list[curpage + i]) | 7,
				  UVMF_INVLPG);
      holy_memset (&window, 0, sizeof (window));

      for (j = i * POINTERS_PER_PAGE;
	   j < (i + 1) * POINTERS_PER_PAGE && j < total_pages; j++)
	if (j < paging_start && !(j >= oldpgstart && j < oldpgend))
	  pg[j - i * POINTERS_PER_PAGE] = page2offset (mfn_list[j]) | 0x7;
	else if (j < holy_xen_start_page_addr->nr_pages)
	  pg[j - i * POINTERS_PER_PAGE] = page2offset (mfn_list[j]) | 5;
	else if (j == holy_xen_start_page_addr->nr_pages)
	  {
	    pg[j - i * POINTERS_PER_PAGE] =
	      page2offset (holy_xen_start_page_addr->console.domU.mfn) | 7;
	    holy_xen_xcons = (void *) (holy_addr_t) page2offset (j);
	  }
	else if (j == holy_xen_start_page_addr->nr_pages + 1)
	  {
	    pg[j - i * POINTERS_PER_PAGE] =
	      holy_xen_start_page_addr->shared_info | 7;
	    holy_xen_shared_info = (void *) (holy_addr_t) page2offset (j);
	  }
	else if (j == holy_xen_start_page_addr->nr_pages + 2)
	  {
	    pg[j - i * POINTERS_PER_PAGE] =
	      page2offset (holy_xen_start_page_addr->store_mfn) | 7;
	    holy_xen_xenstore = (void *) (holy_addr_t) page2offset (j);
	  }
	else if (j == holy_xen_start_page_addr->nr_pages + 3)
	  {
	    pg[j - i * POINTERS_PER_PAGE] = page2offset (gntframe) | 7;
	    holy_xen_grant_table = (void *) (holy_addr_t) page2offset (j);
	  }
    }

  holy_xen_update_va_mapping (&window, 0, UVMF_INVLPG);

  mmuext_op_t op[3];

  op[0].cmd = MMUEXT_PIN_L1_TABLE + (NUMBER_OF_LEVELS - 1);
  op[0].arg1.mfn = mfn_list[paging_start];
  op[1].cmd = MMUEXT_NEW_BASEPTR;
  op[1].arg1.mfn = mfn_list[paging_start];
  op[2].cmd = MMUEXT_UNPIN_TABLE;
  op[2].arg1.mfn = mfn_list[oldpgstart];

  holy_xen_mmuext_op (op, 3, NULL, DOMID_SELF);

  for (i = oldpgstart; i < oldpgend; i++)
    holy_xen_update_va_mapping ((void *) (holy_addr_t) page2offset (i),
				page2offset (mfn_list[i]) | 7, UVMF_INVLPG);
  void *new_start_page, *new_mfn_list;
  new_start_page = (void *) (holy_addr_t) page2offset (paging_start - 1);
  holy_memcpy (new_start_page, holy_xen_start_page_addr, 4096);
  holy_xen_start_page_addr = new_start_page;
  new_mfn_list = (void *) (holy_addr_t)
    page2offset (paging_start - 1
		 - ((holy_xen_start_page_addr->nr_pages
		     * sizeof (holy_uint64_t) + 4095) / 4096));
  holy_memcpy (new_mfn_list, mfn_list, holy_xen_start_page_addr->nr_pages
	       * sizeof (holy_uint64_t));
  holy_xen_start_page_addr->pt_base = page2offset (paging_start);
  holy_xen_start_page_addr->mfn_list = (holy_addr_t) new_mfn_list;

  holy_addr_t heap_start = holy_modules_get_end ();
  holy_addr_t heap_end = (holy_addr_t) new_mfn_list;

  holy_mm_init_region ((void *) heap_start, heap_end - heap_start);
}

extern char _end[];

void
holy_machine_init (void)
{
#ifdef __i386__
  holy_xen_vm_assist (VMASST_CMD_enable, VMASST_TYPE_pae_extended_cr3);
#endif

  holy_modbase = ALIGN_UP ((holy_addr_t) _end
			   + holy_KERNEL_MACHINE_MOD_GAP,
			   holy_KERNEL_MACHINE_MOD_ALIGN);

  map_all_pages ();

  holy_console_init ();

  holy_tsc_init ();

  holy_xendisk_init ();

  holy_boot_init ();
}

void
holy_exit (void)
{
  struct sched_shutdown arg;

  arg.reason = SHUTDOWN_poweroff;
  holy_xen_sched_op (SCHEDOP_shutdown, &arg);
  while (1);
}

void
holy_machine_fini (int flags __attribute__ ((unused)))
{
  holy_xendisk_fini ();
  holy_boot_fini ();
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  holy_uint64_t total_pages = holy_xen_start_page_addr->nr_pages;
  holy_uint64_t usable_pages = holy_xen_start_page_addr->pt_base >> 12;
  if (hook (0, page2offset (usable_pages), holy_MEMORY_AVAILABLE, hook_data))
    return holy_ERR_NONE;

  hook (page2offset (usable_pages), page2offset (total_pages - usable_pages),
	holy_MEMORY_RESERVED, hook_data);

  return holy_ERR_NONE;
}
