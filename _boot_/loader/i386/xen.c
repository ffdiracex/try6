/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/memory.h>
#include <holy/normal.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/cpu/linux.h>
#include <holy/video.h>
#include <holy/video_fb.h>
#include <holy/command.h>
#include <holy/xen/relocator.h>
#include <holy/i18n.h>
#include <holy/elf.h>
#include <holy/elfload.h>
#include <holy/lib/cmdline.h>
#include <holy/xen.h>
#include <holy/xen_file.h>
#include <holy/linux.h>
#include <holy/i386/memory.h>

holy_MOD_LICENSE ("GPLv2+");

#ifdef __x86_64__
#define NUMBER_OF_LEVELS	4
#define INTERMEDIATE_OR		(holy_PAGE_PRESENT | holy_PAGE_RW | holy_PAGE_USER)
#define VIRT_MASK		0x0000ffffffffffffULL
#else
#define NUMBER_OF_LEVELS	3
#define INTERMEDIATE_OR		(holy_PAGE_PRESENT | holy_PAGE_RW)
#define VIRT_MASK		0x00000000ffffffffULL
#define HYPERVISOR_PUD_ADDRESS	0xc0000000ULL
#endif

struct holy_xen_mapping_lvl {
  holy_uint64_t virt_start;
  holy_uint64_t virt_end;
  holy_uint64_t pfn_start;
  holy_uint64_t n_pt_pages;
};

struct holy_xen_mapping {
  holy_uint64_t *where;
  struct holy_xen_mapping_lvl area;
  struct holy_xen_mapping_lvl lvls[NUMBER_OF_LEVELS];
};

struct xen_loader_state {
  struct holy_relocator *relocator;
  struct holy_relocator_xen_state state;
  struct start_info next_start;
  struct holy_xen_file_info xen_inf;
  holy_xen_mfn_t *virt_mfn_list;
  struct start_info *virt_start_info;
  holy_xen_mfn_t console_pfn;
  holy_uint64_t max_addr;
  holy_uint64_t pgtbl_end;
  struct xen_multiboot_mod_list *module_info_page;
  holy_uint64_t modules_target_start;
  holy_size_t n_modules;
  struct holy_xen_mapping *map_reloc;
  struct holy_xen_mapping mappings[XEN_MAX_MAPPINGS];
  int n_mappings;
  int loaded;
};

static struct xen_loader_state xen_state;

static holy_dl_t my_mod;

#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define MAX_MODULES (PAGE_SIZE / sizeof (struct xen_multiboot_mod_list))
#define STACK_SIZE 1048576
#define ADDITIONAL_SIZE (1 << 19)
#define ALIGN_SIZE (1 << 22)
#define LOG_POINTERS_PER_PAGE 9
#define POINTERS_PER_PAGE (1 << LOG_POINTERS_PER_PAGE)

static holy_uint64_t
page2offset (holy_uint64_t page)
{
  return page << PAGE_SHIFT;
}

static holy_err_t
get_pgtable_size (holy_uint64_t from, holy_uint64_t to, holy_uint64_t pfn)
{
  struct holy_xen_mapping *map, *map_cmp;
  holy_uint64_t mask, bits;
  int i, m;

  if (xen_state.n_mappings == XEN_MAX_MAPPINGS)
    return holy_error (holy_ERR_BUG, "too many mapped areas");

  holy_dprintf ("xen", "get_pgtable_size %d from=%llx, to=%llx, pfn=%llx\n",
		xen_state.n_mappings, (unsigned long long) from,
		(unsigned long long) to, (unsigned long long) pfn);

  map = xen_state.mappings + xen_state.n_mappings;
  holy_memset (map, 0, sizeof (*map));

  map->area.virt_start = from & VIRT_MASK;
  map->area.virt_end = (to - 1) & VIRT_MASK;
  map->area.n_pt_pages = 0;

  for (i = NUMBER_OF_LEVELS - 1; i >= 0; i--)
    {
      map->lvls[i].pfn_start = pfn + map->area.n_pt_pages;
      if (i == NUMBER_OF_LEVELS - 1)
	{
	  if (xen_state.n_mappings == 0)
	    {
	      map->lvls[i].virt_start = 0;
	      map->lvls[i].virt_end = VIRT_MASK;
	      map->lvls[i].n_pt_pages = 1;
	      map->area.n_pt_pages++;
	    }
	  continue;
	}

      bits = PAGE_SHIFT + (i + 1) * LOG_POINTERS_PER_PAGE;
      mask = (1ULL << bits) - 1;
      map->lvls[i].virt_start = map->area.virt_start & ~mask;
      map->lvls[i].virt_end = map->area.virt_end | mask;
#ifdef __i386__
      /* PAE wants last root directory present. */
      if (i == 1 && to <= HYPERVISOR_PUD_ADDRESS && xen_state.n_mappings == 0)
	map->lvls[i].virt_end = VIRT_MASK;
#endif
      for (m = 0; m < xen_state.n_mappings; m++)
	{
	  map_cmp = xen_state.mappings + m;
	  if (map_cmp->lvls[i].virt_start == map_cmp->lvls[i].virt_end)
	    continue;
	  if (map->lvls[i].virt_start >= map_cmp->lvls[i].virt_start &&
	      map->lvls[i].virt_end <= map_cmp->lvls[i].virt_end)
	   {
	     map->lvls[i].virt_start = 0;
	     map->lvls[i].virt_end = 0;
	     break;
	   }
	   if (map->lvls[i].virt_start >= map_cmp->lvls[i].virt_start &&
	       map->lvls[i].virt_start <= map_cmp->lvls[i].virt_end)
	     map->lvls[i].virt_start = map_cmp->lvls[i].virt_end + 1;
	   if (map->lvls[i].virt_end >= map_cmp->lvls[i].virt_start &&
	       map->lvls[i].virt_end <= map_cmp->lvls[i].virt_end)
	     map->lvls[i].virt_end = map_cmp->lvls[i].virt_start - 1;
	}
      if (map->lvls[i].virt_start < map->lvls[i].virt_end)
	map->lvls[i].n_pt_pages =
	  ((map->lvls[i].virt_end - map->lvls[i].virt_start) >> bits) + 1;
      map->area.n_pt_pages += map->lvls[i].n_pt_pages;
      holy_dprintf ("xen", "get_pgtable_size level %d: virt %llx-%llx %d pts\n",
		    i, (unsigned long long)  map->lvls[i].virt_start,
		    (unsigned long long)  map->lvls[i].virt_end,
		    (int) map->lvls[i].n_pt_pages);
    }

  holy_dprintf ("xen", "get_pgtable_size return: %d page tables\n",
		(int) map->area.n_pt_pages);

  xen_state.state.paging_start[xen_state.n_mappings] = pfn;
  xen_state.state.paging_size[xen_state.n_mappings] = map->area.n_pt_pages;

  return holy_ERR_NONE;
}

static holy_uint64_t *
get_pg_table_virt (int mapping, int level)
{
  holy_uint64_t pfn;
  struct holy_xen_mapping *map;

  map = xen_state.mappings + mapping;
  pfn = map->lvls[level].pfn_start - map->lvls[NUMBER_OF_LEVELS - 1].pfn_start;
  return map->where + pfn * POINTERS_PER_PAGE;
}

static holy_uint64_t
get_pg_table_prot (int level, holy_uint64_t pfn)
{
  int m;
  holy_uint64_t pfn_s, pfn_e;

  if (level > 0)
    return INTERMEDIATE_OR;
  for (m = 0; m < xen_state.n_mappings; m++)
    {
      pfn_s = xen_state.mappings[m].lvls[NUMBER_OF_LEVELS - 1].pfn_start;
      pfn_e = xen_state.mappings[m].area.n_pt_pages + pfn_s;
      if (pfn >= pfn_s && pfn < pfn_e)
	return holy_PAGE_PRESENT | holy_PAGE_USER;
    }
  return holy_PAGE_PRESENT | holy_PAGE_RW | holy_PAGE_USER;
}

static void
generate_page_table (holy_xen_mfn_t *mfn_list)
{
  int l, m1, m2;
  long p, p_s, p_e;
  holy_uint64_t start, end, pfn;
  holy_uint64_t *pg;
  struct holy_xen_mapping_lvl *lvl;

  for (m1 = 0; m1 < xen_state.n_mappings; m1++)
    holy_memset (xen_state.mappings[m1].where, 0,
		 xen_state.mappings[m1].area.n_pt_pages * PAGE_SIZE);

  for (l = NUMBER_OF_LEVELS - 1; l >= 0; l--)
    {
      for (m1 = 0; m1 < xen_state.n_mappings; m1++)
	{
	  start = xen_state.mappings[m1].lvls[l].virt_start;
	  end = xen_state.mappings[m1].lvls[l].virt_end;
	  pg = get_pg_table_virt(m1, l);
	  for (m2 = 0; m2 < xen_state.n_mappings; m2++)
	    {
	      lvl = (l > 0) ? xen_state.mappings[m2].lvls + l - 1
			    : &xen_state.mappings[m2].area;
	      if (l > 0 && lvl->n_pt_pages == 0)
		continue;
	      if (lvl->virt_start >= end || lvl->virt_end <= start)
		continue;
	      p_s = (holy_max (start, lvl->virt_start) - start) >>
		    (PAGE_SHIFT + l * LOG_POINTERS_PER_PAGE);
	      p_e = (holy_min (end, lvl->virt_end) - start) >>
		    (PAGE_SHIFT + l * LOG_POINTERS_PER_PAGE);
	      pfn = ((holy_max (start, lvl->virt_start) - lvl->virt_start) >>
		     (PAGE_SHIFT + l * LOG_POINTERS_PER_PAGE)) + lvl->pfn_start;
	      holy_dprintf ("xen", "write page table entries level %d pg %p "
			    "mapping %d/%d index %lx-%lx pfn %llx\n",
			    l, pg, m1, m2, p_s, p_e, (unsigned long long) pfn);
	      for (p = p_s; p <= p_e; p++)
		{
		  pg[p] = page2offset (mfn_list[pfn]) |
			  get_pg_table_prot (l, pfn);
		  pfn++;
		}
	    }
	}
    }
}

static holy_err_t
set_mfns (holy_xen_mfn_t pfn)
{
  holy_xen_mfn_t i, t;
  holy_xen_mfn_t cn_pfn = -1, st_pfn = -1;
  struct mmu_update m2p_updates[4];


  for (i = 0; i < holy_xen_start_page_addr->nr_pages; i++)
    {
      if (xen_state.virt_mfn_list[i] ==
	  holy_xen_start_page_addr->console.domU.mfn)
	cn_pfn = i;
      if (xen_state.virt_mfn_list[i] == holy_xen_start_page_addr->store_mfn)
	st_pfn = i;
    }
  if (cn_pfn == (holy_xen_mfn_t)-1)
    return holy_error (holy_ERR_BUG, "no console");
  if (st_pfn == (holy_xen_mfn_t)-1)
    return holy_error (holy_ERR_BUG, "no store");
  t = xen_state.virt_mfn_list[pfn];
  xen_state.virt_mfn_list[pfn] = xen_state.virt_mfn_list[cn_pfn];
  xen_state.virt_mfn_list[cn_pfn] = t;
  t = xen_state.virt_mfn_list[pfn + 1];
  xen_state.virt_mfn_list[pfn + 1] = xen_state.virt_mfn_list[st_pfn];
  xen_state.virt_mfn_list[st_pfn] = t;

  m2p_updates[0].ptr =
    page2offset (xen_state.virt_mfn_list[pfn]) | MMU_MACHPHYS_UPDATE;
  m2p_updates[0].val = pfn;
  m2p_updates[1].ptr =
    page2offset (xen_state.virt_mfn_list[pfn + 1]) | MMU_MACHPHYS_UPDATE;
  m2p_updates[1].val = pfn + 1;
  m2p_updates[2].ptr =
    page2offset (xen_state.virt_mfn_list[cn_pfn]) | MMU_MACHPHYS_UPDATE;
  m2p_updates[2].val = cn_pfn;
  m2p_updates[3].ptr =
    page2offset (xen_state.virt_mfn_list[st_pfn]) | MMU_MACHPHYS_UPDATE;
  m2p_updates[3].val = st_pfn;

  holy_xen_mmu_update (m2p_updates, 4, NULL, DOMID_SELF);

  return holy_ERR_NONE;
}

static holy_err_t
holy_xen_p2m_alloc (void)
{
  holy_relocator_chunk_t ch;
  holy_size_t p2msize, p2malloc;
  holy_err_t err;
  struct holy_xen_mapping *map;

  if (xen_state.virt_mfn_list)
    return holy_ERR_NONE;

  map = xen_state.mappings + xen_state.n_mappings;
  p2msize = ALIGN_UP (sizeof (holy_xen_mfn_t) *
		      holy_xen_start_page_addr->nr_pages, PAGE_SIZE);
  if (xen_state.xen_inf.has_p2m_base)
    {
      err = get_pgtable_size (xen_state.xen_inf.p2m_base,
			      xen_state.xen_inf.p2m_base + p2msize,
			      (xen_state.max_addr + p2msize) >> PAGE_SHIFT);
      if (err)
	return err;

      map->area.pfn_start = xen_state.max_addr >> PAGE_SHIFT;
      p2malloc = p2msize + page2offset (map->area.n_pt_pages);
      xen_state.n_mappings++;
      xen_state.next_start.mfn_list = xen_state.xen_inf.p2m_base;
      xen_state.next_start.first_p2m_pfn = map->area.pfn_start;
      xen_state.next_start.nr_p2m_frames = p2malloc >> PAGE_SHIFT;
    }
  else
    {
      xen_state.next_start.mfn_list =
	xen_state.max_addr + xen_state.xen_inf.virt_base;
      p2malloc = p2msize;
    }

  xen_state.state.mfn_list = xen_state.max_addr;
  err = holy_relocator_alloc_chunk_addr (xen_state.relocator, &ch,
					 xen_state.max_addr, p2malloc);
  if (err)
    return err;
  xen_state.virt_mfn_list = get_virtual_current_address (ch);
  if (xen_state.xen_inf.has_p2m_base)
    map->where = (holy_uint64_t *) xen_state.virt_mfn_list +
		 p2msize / sizeof (holy_uint64_t);
  holy_memcpy (xen_state.virt_mfn_list,
	       (void *) holy_xen_start_page_addr->mfn_list, p2msize);
  xen_state.max_addr += p2malloc;

  return holy_ERR_NONE;
}

static holy_err_t
holy_xen_special_alloc (void)
{
  holy_relocator_chunk_t ch;
  holy_err_t err;

  if (xen_state.virt_start_info)
    return holy_ERR_NONE;

  err = holy_relocator_alloc_chunk_addr (xen_state.relocator, &ch,
					 xen_state.max_addr,
					 sizeof (xen_state.next_start));
  if (err)
    return err;
  xen_state.state.start_info = xen_state.max_addr + xen_state.xen_inf.virt_base;
  xen_state.virt_start_info = get_virtual_current_address (ch);
  xen_state.max_addr =
    ALIGN_UP (xen_state.max_addr + sizeof (xen_state.next_start), PAGE_SIZE);
  xen_state.console_pfn = xen_state.max_addr >> PAGE_SHIFT;
  xen_state.max_addr += 2 * PAGE_SIZE;

  xen_state.next_start.nr_pages = holy_xen_start_page_addr->nr_pages;
  holy_memcpy (xen_state.next_start.magic, holy_xen_start_page_addr->magic,
	       sizeof (xen_state.next_start.magic));
  xen_state.next_start.store_mfn = holy_xen_start_page_addr->store_mfn;
  xen_state.next_start.store_evtchn = holy_xen_start_page_addr->store_evtchn;
  xen_state.next_start.console.domU = holy_xen_start_page_addr->console.domU;
  xen_state.next_start.shared_info = holy_xen_start_page_addr->shared_info;

  return holy_ERR_NONE;
}

static holy_err_t
holy_xen_pt_alloc (void)
{
  holy_relocator_chunk_t ch;
  holy_err_t err;
  holy_uint64_t nr_info_pages;
  holy_uint64_t nr_need_pages;
  holy_uint64_t try_virt_end;
  struct holy_xen_mapping *map;

  if (xen_state.pgtbl_end)
    return holy_ERR_NONE;

  map = xen_state.mappings + xen_state.n_mappings;
  xen_state.map_reloc = map + 1;

  xen_state.next_start.pt_base =
    xen_state.max_addr + xen_state.xen_inf.virt_base;
  nr_info_pages = xen_state.max_addr >> PAGE_SHIFT;
  nr_need_pages = nr_info_pages;

  while (1)
    {
      try_virt_end = ALIGN_UP (xen_state.xen_inf.virt_base +
			       page2offset (nr_need_pages) +
			       ADDITIONAL_SIZE + STACK_SIZE, ALIGN_SIZE);

      err = get_pgtable_size (xen_state.xen_inf.virt_base, try_virt_end,
			      nr_info_pages);
      if (err)
	return err;
      xen_state.n_mappings++;

      /* Map the relocator page either at virtual 0 or after end of area. */
      nr_need_pages = nr_info_pages + map->area.n_pt_pages;
      if (xen_state.xen_inf.virt_base)
	err = get_pgtable_size (0, PAGE_SIZE, nr_need_pages);
      else
	err = get_pgtable_size (try_virt_end, try_virt_end + PAGE_SIZE,
				nr_need_pages);
      if (err)
	return err;
      nr_need_pages += xen_state.map_reloc->area.n_pt_pages;

      if (xen_state.xen_inf.virt_base + page2offset (nr_need_pages) <=
	  try_virt_end)
	break;

      xen_state.n_mappings--;
    }

  xen_state.n_mappings++;
  nr_need_pages = map->area.n_pt_pages + xen_state.map_reloc->area.n_pt_pages;
  err = holy_relocator_alloc_chunk_addr (xen_state.relocator, &ch,
					 xen_state.max_addr,
					 page2offset (nr_need_pages));
  if (err)
    return err;

  map->where = get_virtual_current_address (ch);
  map->area.pfn_start = 0;
  xen_state.max_addr += page2offset (nr_need_pages);
  xen_state.state.stack =
    xen_state.max_addr + STACK_SIZE + xen_state.xen_inf.virt_base;
  xen_state.next_start.nr_pt_frames = nr_need_pages;
  xen_state.max_addr = try_virt_end - xen_state.xen_inf.virt_base;
  xen_state.pgtbl_end = xen_state.max_addr >> PAGE_SHIFT;
  xen_state.map_reloc->where = (holy_uint64_t *) ((char *) map->where +
					page2offset (map->area.n_pt_pages));

  return holy_ERR_NONE;
}

/* Allocate all not yet allocated areas mapped by initial page tables. */
static holy_err_t
holy_xen_alloc_boot_data (void)
{
  holy_err_t err;

  if (!xen_state.xen_inf.has_p2m_base)
    {
      err = holy_xen_p2m_alloc ();
      if (err)
	return err;
    }
  err = holy_xen_special_alloc ();
  if (err)
    return err;
  err = holy_xen_pt_alloc ();
  if (err)
    return err;

  return holy_ERR_NONE;
}

static holy_err_t
holy_xen_boot (void)
{
  holy_err_t err;
  holy_uint64_t nr_pages;
  struct gnttab_set_version gnttab_setver;
  holy_size_t i;

  if (holy_xen_n_allocated_shared_pages)
    return holy_error (holy_ERR_BUG, "active grants");

  err = holy_xen_alloc_boot_data ();
  if (err)
    return err;
  if (xen_state.xen_inf.has_p2m_base)
    {
      err = holy_xen_p2m_alloc ();
      if (err)
	return err;
    }

  err = set_mfns (xen_state.console_pfn);
  if (err)
    return err;

  nr_pages = xen_state.max_addr >> PAGE_SHIFT;

  holy_dprintf ("xen", "bootstrap domain %llx+%llx\n",
		(unsigned long long) xen_state.xen_inf.virt_base,
		(unsigned long long) page2offset (nr_pages));

  xen_state.map_reloc->area.pfn_start = nr_pages;
  generate_page_table (xen_state.virt_mfn_list);

  xen_state.state.entry_point = xen_state.xen_inf.entry_point;

  *xen_state.virt_start_info = xen_state.next_start;

  holy_memset (&gnttab_setver, 0, sizeof (gnttab_setver));

  gnttab_setver.version = 1;
  holy_xen_grant_table_op (GNTTABOP_set_version, &gnttab_setver, 1);

  for (i = 0; i < ARRAY_SIZE (holy_xen_shared_info->evtchn_pending); i++)
    holy_xen_shared_info->evtchn_pending[i] = 0;

  return holy_relocator_xen_boot (xen_state.relocator, xen_state.state, nr_pages,
				  xen_state.xen_inf.virt_base <
				  PAGE_SIZE ? page2offset (nr_pages) : 0,
				  xen_state.pgtbl_end - 1,
				  page2offset (xen_state.pgtbl_end - 1) +
				  xen_state.xen_inf.virt_base);
}

static void
holy_xen_reset (void)
{
  holy_relocator_unload (xen_state.relocator);

  holy_memset (&xen_state, 0, sizeof (xen_state));
}

static holy_err_t
holy_xen_unload (void)
{
  holy_xen_reset ();
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

#define HYPERCALL_INTERFACE_SIZE 32

#ifdef __x86_64__
static holy_uint8_t template[] =
  {
    0x51, /* push %rcx */
    0x41, 0x53, /* push %r11 */
    0x48, 0xc7, 0xc0, 0xbb, 0xaa, 0x00, 0x00, 	/* mov    $0xaabb,%rax */
    0x0f, 0x05,  /* syscall  */
    0x41, 0x5b, /* pop %r11 */
    0x59, /* pop %rcx  */
    0xc3 /* ret */
  };

static holy_uint8_t template_iret[] =
  {
    0x51, /* push   %rcx */
    0x41, 0x53, /* push   %r11 */
    0x50, /* push   %rax */
    0x48, 0xc7, 0xc0, 0x17, 0x00, 0x00, 0x00, /* mov    $0x17,%rax */
    0x0f, 0x05 /* syscall */
  };
#define CALLNO_OFFSET 6
#else

static holy_uint8_t template[] =
  {
    0xb8, 0xbb, 0xaa, 0x00, 0x00, /* mov imm32, %eax */
    0xcd, 0x82,  /* int $0x82  */
    0xc3 /* ret */
  };

static holy_uint8_t template_iret[] =
  {
    0x50, /* push   %eax */
    0xb8, 0x17, 0x00, 0x00, 0x00, /* mov    $0x17,%eax */
    0xcd, 0x82,  /* int $0x82  */
  };
#define CALLNO_OFFSET 1

#endif


static void
set_hypercall_interface (holy_uint8_t *tgt, unsigned callno)
{
  if (callno == 0x17)
    {
      holy_memcpy (tgt, template_iret, ARRAY_SIZE (template_iret));
      holy_memset (tgt + ARRAY_SIZE (template_iret), 0xcc,
		   HYPERCALL_INTERFACE_SIZE - ARRAY_SIZE (template_iret));
      return;
    }
  holy_memcpy (tgt, template, ARRAY_SIZE (template));
  holy_memset (tgt + ARRAY_SIZE (template), 0xcc,
	       HYPERCALL_INTERFACE_SIZE - ARRAY_SIZE (template));
  tgt[CALLNO_OFFSET] = callno & 0xff;
  tgt[CALLNO_OFFSET + 1] = callno >> 8;
}

#ifdef __x86_64__
#define holy_elfXX_load holy_elf64_load
#else
#define holy_elfXX_load holy_elf32_load
#endif

static holy_err_t
holy_cmd_xen (holy_command_t cmd __attribute__ ((unused)),
	      int argc, char *argv[])
{
  holy_file_t file;
  holy_elf_t elf;
  holy_err_t err;
  void *kern_chunk_src;
  holy_relocator_chunk_t ch;
  holy_addr_t kern_start;
  holy_addr_t kern_end;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  /* Call holy_loader_unset early to avoid it being called by holy_loader_set */
  holy_loader_unset ();

  holy_xen_reset ();

  holy_create_loader_cmdline (argc - 1, argv + 1,
			      (char *) xen_state.next_start.cmd_line,
			      sizeof (xen_state.next_start.cmd_line) - 1);

  file = holy_file_open (argv[0]);
  if (!file)
    return holy_errno;

  elf = holy_xen_file_and_cmdline (file,
		                   (char *) xen_state.next_start.cmd_line,
		                   sizeof (xen_state.next_start.cmd_line) - 1);
  if (!elf)
    goto fail;

  err = holy_xen_get_info (elf, &xen_state.xen_inf);
  if (err)
    goto fail;
#ifdef __x86_64__
  if (xen_state.xen_inf.arch != holy_XEN_FILE_X86_64)
#else
  if (xen_state.xen_inf.arch != holy_XEN_FILE_I386_PAE
      && xen_state.xen_inf.arch != holy_XEN_FILE_I386_PAE_BIMODE)
#endif
    {
      holy_error (holy_ERR_BAD_OS, "incompatible architecture: %d",
		  xen_state.xen_inf.arch);
      goto fail;
    }

  if (xen_state.xen_inf.virt_base & (PAGE_SIZE - 1))
    {
      holy_error (holy_ERR_BAD_OS, "unaligned virt_base");
      goto fail;
    }
  holy_dprintf ("xen", "virt_base = %llx, entry = %llx\n",
		(unsigned long long) xen_state.xen_inf.virt_base,
		(unsigned long long) xen_state.xen_inf.entry_point);

  xen_state.relocator = holy_relocator_new ();
  if (!xen_state.relocator)
    goto fail;

  kern_start = xen_state.xen_inf.kern_start - xen_state.xen_inf.paddr_offset;
  kern_end = xen_state.xen_inf.kern_end - xen_state.xen_inf.paddr_offset;

  if (xen_state.xen_inf.has_hypercall_page)
    {
      holy_dprintf ("xen", "hypercall page at 0x%llx\n",
		    (unsigned long long) xen_state.xen_inf.hypercall_page);
      kern_start = holy_min (kern_start, xen_state.xen_inf.hypercall_page -
					 xen_state.xen_inf.virt_base);
      kern_end = holy_max (kern_end, xen_state.xen_inf.hypercall_page -
				     xen_state.xen_inf.virt_base + PAGE_SIZE);
    }

  xen_state.max_addr = ALIGN_UP (kern_end, PAGE_SIZE);

  err = holy_relocator_alloc_chunk_addr (xen_state.relocator, &ch, kern_start,
					 kern_end - kern_start);
  if (err)
    goto fail;
  kern_chunk_src = get_virtual_current_address (ch);

  holy_dprintf ("xen", "paddr_offset = 0x%llx\n",
		(unsigned long long) xen_state.xen_inf.paddr_offset);
  holy_dprintf ("xen", "kern_start = 0x%llx, kern_end = 0x%llx\n",
		(unsigned long long) xen_state.xen_inf.kern_start,
		(unsigned long long) xen_state.xen_inf.kern_end);

  err = holy_elfXX_load (elf, argv[0],
			 (holy_uint8_t *) kern_chunk_src - kern_start
			 - xen_state.xen_inf.paddr_offset, 0, 0, 0);

  if (xen_state.xen_inf.has_hypercall_page)
    {
      unsigned i;
      for (i = 0; i < PAGE_SIZE / HYPERCALL_INTERFACE_SIZE; i++)
	set_hypercall_interface ((holy_uint8_t *) kern_chunk_src +
				 i * HYPERCALL_INTERFACE_SIZE +
				 xen_state.xen_inf.hypercall_page -
				 xen_state.xen_inf.virt_base - kern_start, i);
    }

  if (err)
    goto fail;

  holy_dl_ref (my_mod);
  xen_state.loaded = 1;

  holy_loader_set (holy_xen_boot, holy_xen_unload, 0);

  goto fail;

fail:
  /* holy_errno might be clobbered by further calls, save the error reason. */
  err = holy_errno;

  if (elf)
    holy_elf_close (elf);
  else if (file)
    holy_file_close (file);

  if (err != holy_ERR_NONE)
    holy_xen_reset ();

  return err;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  holy_size_t size = 0;
  holy_err_t err;
  struct holy_linux_initrd_context initrd_ctx = { 0, 0, 0 };
  holy_relocator_chunk_t ch;

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (!xen_state.loaded)
    {
      holy_error (holy_ERR_BAD_ARGUMENT,
		  N_("you need to load the kernel first"));
      goto fail;
    }

  if (xen_state.next_start.mod_start || xen_state.next_start.mod_len)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("initrd already loaded"));
      goto fail;
    }

  if (xen_state.xen_inf.unmapped_initrd)
    {
      err = holy_xen_alloc_boot_data ();
      if (err)
	goto fail;
    }

  if (holy_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  size = holy_get_initrd_size (&initrd_ctx);

  if (size)
    {
      err = holy_relocator_alloc_chunk_addr (xen_state.relocator, &ch,
					     xen_state.max_addr, size);
      if (err)
	goto fail;

      if (holy_initrd_load (&initrd_ctx, argv,
			    get_virtual_current_address (ch)))
	goto fail;
    }

  xen_state.next_start.mod_len = size;

  if (xen_state.xen_inf.unmapped_initrd)
    {
      xen_state.next_start.flags |= SIF_MOD_START_PFN;
      xen_state.next_start.mod_start = xen_state.max_addr >> PAGE_SHIFT;
    }
  else
    xen_state.next_start.mod_start =
      xen_state.max_addr + xen_state.xen_inf.virt_base;

  holy_dprintf ("xen", "Initrd, addr=0x%x, size=0x%x\n",
		(unsigned) (xen_state.max_addr + xen_state.xen_inf.virt_base),
		(unsigned) size);

  xen_state.max_addr = ALIGN_UP (xen_state.max_addr + size, PAGE_SIZE);

fail:
  holy_initrd_close (&initrd_ctx);

  return holy_errno;
}

static holy_err_t
holy_cmd_module (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  holy_size_t size = 0;
  holy_err_t err;
  holy_relocator_chunk_t ch;
  holy_size_t cmdline_len;
  int nounzip = 0;
  holy_file_t file;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (holy_strcmp (argv[0], "--nounzip") == 0)
    {
      argv++;
      argc--;
      nounzip = 1;
    }

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (!xen_state.loaded)
    {
      return holy_error (holy_ERR_BAD_ARGUMENT,
			 N_("you need to load the kernel first"));
    }

  if ((xen_state.next_start.mod_start || xen_state.next_start.mod_len) &&
      !xen_state.module_info_page)
    {
      return holy_error (holy_ERR_BAD_ARGUMENT, N_("initrd already loaded"));
    }

  /* Leave one space for terminator.  */
  if (xen_state.n_modules >= MAX_MODULES - 1)
    {
      return holy_error (holy_ERR_BAD_ARGUMENT, "too many modules");
    }

  if (!xen_state.module_info_page)
    {
      xen_state.xen_inf.unmapped_initrd = 0;
      xen_state.n_modules = 0;
      xen_state.max_addr = ALIGN_UP (xen_state.max_addr, PAGE_SIZE);
      xen_state.modules_target_start = xen_state.max_addr;
      xen_state.next_start.mod_start =
	xen_state.max_addr + xen_state.xen_inf.virt_base;
      xen_state.next_start.flags |= SIF_MULTIBOOT_MOD;

      err = holy_relocator_alloc_chunk_addr (xen_state.relocator, &ch,
					     xen_state.max_addr, MAX_MODULES
					     *
					     sizeof (xen_state.module_info_page
						     [0]));
      if (err)
	return err;
      xen_state.module_info_page = get_virtual_current_address (ch);
      holy_memset (xen_state.module_info_page, 0, MAX_MODULES
		   * sizeof (xen_state.module_info_page[0]));
      xen_state.max_addr +=
	MAX_MODULES * sizeof (xen_state.module_info_page[0]);
    }

  xen_state.max_addr = ALIGN_UP (xen_state.max_addr, PAGE_SIZE);

  if (nounzip)
    holy_file_filter_disable_compression ();
  file = holy_file_open (argv[0]);
  if (!file)
    return holy_errno;
  size = holy_file_size (file);

  cmdline_len = holy_loader_cmdline_size (argc - 1, argv + 1);

  err = holy_relocator_alloc_chunk_addr (xen_state.relocator, &ch,
					 xen_state.max_addr, cmdline_len);
  if (err)
    goto fail;

  holy_create_loader_cmdline (argc - 1, argv + 1,
			      get_virtual_current_address (ch), cmdline_len);

  xen_state.module_info_page[xen_state.n_modules].cmdline =
    xen_state.max_addr - xen_state.modules_target_start;
  xen_state.max_addr = ALIGN_UP (xen_state.max_addr + cmdline_len, PAGE_SIZE);

  if (size)
    {
      err = holy_relocator_alloc_chunk_addr (xen_state.relocator, &ch,
					     xen_state.max_addr, size);
      if (err)
	goto fail;
      if (holy_file_read (file, get_virtual_current_address (ch), size)
	  != (holy_ssize_t) size)
	{
	  if (!holy_errno)
	    holy_error (holy_ERR_FILE_READ_ERROR,
			N_("premature end of file %s"), argv[0]);
	  goto fail;
	}
    }
  xen_state.next_start.mod_len =
    xen_state.max_addr + size - xen_state.modules_target_start;
  xen_state.module_info_page[xen_state.n_modules].mod_start =
    xen_state.max_addr - xen_state.modules_target_start;
  xen_state.module_info_page[xen_state.n_modules].mod_end =
    xen_state.max_addr + size - xen_state.modules_target_start;

  xen_state.n_modules++;
  holy_dprintf ("xen", "module, addr=0x%x, size=0x%x\n",
		(unsigned) xen_state.max_addr, (unsigned) size);
  xen_state.max_addr = ALIGN_UP (xen_state.max_addr + size, PAGE_SIZE);


fail:
  holy_file_close (file);

  return holy_errno;
}

static holy_command_t cmd_xen, cmd_initrd, cmd_module, cmd_multiboot;

holy_MOD_INIT (xen)
{
  cmd_xen = holy_register_command ("linux", holy_cmd_xen,
				   0, N_("Load Linux."));
  cmd_multiboot = holy_register_command ("multiboot", holy_cmd_xen,
					 0, N_("Load Linux."));
  cmd_initrd = holy_register_command ("initrd", holy_cmd_initrd,
				      0, N_("Load initrd."));
  cmd_module = holy_register_command ("module", holy_cmd_module,
				      0, N_("Load module."));
  my_mod = mod;
}

holy_MOD_FINI (xen)
{
  holy_unregister_command (cmd_xen);
  holy_unregister_command (cmd_initrd);
  holy_unregister_command (cmd_multiboot);
  holy_unregister_command (cmd_module);
}
