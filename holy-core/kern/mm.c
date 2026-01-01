/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/types.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/i18n.h>
#include <holy/mm_private.h>

#ifdef MM_DEBUG
# undef holy_malloc
# undef holy_zalloc
# undef holy_realloc
# undef holy_free
# undef holy_memalign
#endif



holy_mm_region_t holy_mm_base;

/* Get a header from the pointer PTR, and set *P and *R to a pointer
   to the header and a pointer to its region, respectively. PTR must
   be allocated.  */
static void
get_header_from_pointer (void *ptr, holy_mm_header_t *p, holy_mm_region_t *r)
{
  if ((holy_addr_t) ptr & (holy_MM_ALIGN - 1))
    holy_fatal ("unaligned pointer %p", ptr);

  for (*r = holy_mm_base; *r; *r = (*r)->next)
    if ((holy_addr_t) ptr > (holy_addr_t) ((*r) + 1)
	&& (holy_addr_t) ptr <= (holy_addr_t) ((*r) + 1) + (*r)->size)
      break;

  if (! *r)
    holy_fatal ("out of range pointer %p", ptr);

  *p = (holy_mm_header_t) ptr - 1;
  if ((*p)->magic == holy_MM_FREE_MAGIC)
    holy_fatal ("double free at %p", *p);
  if ((*p)->magic != holy_MM_ALLOC_MAGIC)
    holy_fatal ("alloc magic is broken at %p: %lx", *p,
		(unsigned long) (*p)->magic);
}

/* Initialize a region starting from ADDR and whose size is SIZE,
   to use it as free space.  */
void
holy_mm_init_region (void *addr, holy_size_t size)
{
  holy_mm_header_t h;
  holy_mm_region_t r, *p, q;

#if 0
  holy_printf ("Using memory for heap: start=%p, end=%p\n", addr, addr + (unsigned int) size);
#endif

  /* Exclude last 4K to avoid overflows. */
  /* If addr + 0x1000 overflows then whole region is in excluded zone.  */
  if ((holy_addr_t) addr > ~((holy_addr_t) 0x1000))
    return;

  /* If addr + 0x1000 + size overflows then decrease size.  */
  if (((holy_addr_t) addr + 0x1000) > ~(holy_addr_t) size)
    size = ((holy_addr_t) -0x1000) - (holy_addr_t) addr;

  for (p = &holy_mm_base, q = *p; q; p = &(q->next), q = *p)
    if ((holy_uint8_t *) addr + size + q->pre_size == (holy_uint8_t *) q)
      {
	r = (holy_mm_region_t) ALIGN_UP ((holy_addr_t) addr, holy_MM_ALIGN);
	*r = *q;
	r->pre_size += size;
	
	if (r->pre_size >> holy_MM_ALIGN_LOG2)
	  {
	    h = (holy_mm_header_t) (r + 1);
	    h->size = (r->pre_size >> holy_MM_ALIGN_LOG2);
	    h->magic = holy_MM_ALLOC_MAGIC;
	    r->size += h->size << holy_MM_ALIGN_LOG2;
	    r->pre_size &= (holy_MM_ALIGN - 1);
	    *p = r;
	    holy_free (h + 1);
	  }
	*p = r;
	return;
      }

  /* Allocate a region from the head.  */
  r = (holy_mm_region_t) ALIGN_UP ((holy_addr_t) addr, holy_MM_ALIGN);

  /* If this region is too small, ignore it.  */
  if (size < holy_MM_ALIGN + (char *) r - (char *) addr + sizeof (*r))
    return;

  size -= (char *) r - (char *) addr + sizeof (*r);

  h = (holy_mm_header_t) (r + 1);
  h->next = h;
  h->magic = holy_MM_FREE_MAGIC;
  h->size = (size >> holy_MM_ALIGN_LOG2);

  r->first = h;
  r->pre_size = (holy_addr_t) r - (holy_addr_t) addr;
  r->size = (h->size << holy_MM_ALIGN_LOG2);

  /* Find where to insert this region. Put a smaller one before bigger ones,
     to prevent fragmentation.  */
  for (p = &holy_mm_base, q = *p; q; p = &(q->next), q = *p)
    if (q->size > r->size)
      break;

  *p = r;
  r->next = q;
}

/* Allocate the number of units N with the alignment ALIGN from the ring
   buffer starting from *FIRST.  ALIGN must be a power of two. Both N and
   ALIGN are in units of holy_MM_ALIGN.  Return a non-NULL if successful,
   otherwise return NULL.  */
static void *
holy_real_malloc (holy_mm_header_t *first, holy_size_t n, holy_size_t align)
{
  holy_mm_header_t p, q;

  /* When everything is allocated side effect is that *first will have alloc
     magic marked, meaning that there is no room in this region.  */
  if ((*first)->magic == holy_MM_ALLOC_MAGIC)
    return 0;

  /* Try to search free slot for allocation in this memory region.  */
  for (q = *first, p = q->next; ; q = p, p = p->next)
    {
      holy_off_t extra;

      extra = ((holy_addr_t) (p + 1) >> holy_MM_ALIGN_LOG2) & (align - 1);
      if (extra)
	extra = align - extra;

      if (! p)
	holy_fatal ("null in the ring");

      if (p->magic != holy_MM_FREE_MAGIC)
	holy_fatal ("free magic is broken at %p: 0x%x", p, p->magic);

      if (p->size >= n + extra)
	{
	  extra += (p->size - extra - n) & (~(align - 1));
	  if (extra == 0 && p->size == n)
	    {
	      /* There is no special alignment requirement and memory block
	         is complete match.

	         1. Just mark memory block as allocated and remove it from
	            free list.

	         Result:
	         +---------------+ previous block's next
	         | alloc, size=n |          |
	         +---------------+          v
	       */
	      q->next = p->next;
	    }
	  else if (align == 1 || p->size == n + extra)
	    {
	      /* There might be alignment requirement, when taking it into
	         account memory block fits in.

	         1. Allocate new area at end of memory block.
	         2. Reduce size of available blocks from original node.
	         3. Mark new area as allocated and "remove" it from free
	            list.

	         Result:
	         +---------------+
	         | free, size-=n | next --+
	         +---------------+        |
	         | alloc, size=n |        |
	         +---------------+        v
	       */

	      p->size -= n;
	      p += p->size;
	    }
	  else if (extra == 0)
	    {
	      holy_mm_header_t r;
	      
	      r = p + extra + n;
	      r->magic = holy_MM_FREE_MAGIC;
	      r->size = p->size - extra - n;
	      r->next = p->next;
	      q->next = r;

	      if (q == p)
		{
		  q = r;
		  r->next = r;
		}
	    }
	  else
	    {
	      /* There is alignment requirement and there is room in memory
	         block.  Split memory block to three pieces.

	         1. Create new memory block right after section being
	            allocated.  Mark it as free.
	         2. Add new memory block to free chain.
	         3. Mark current memory block having only extra blocks.
	         4. Advance to aligned block and mark that as allocated and
	            "remove" it from free list.

	         Result:
	         +------------------------------+
	         | free, size=extra             | next --+
	         +------------------------------+        |
	         | alloc, size=n                |        |
	         +------------------------------+        |
	         | free, size=orig.size-extra-n | <------+, next --+
	         +------------------------------+                  v
	       */
	      holy_mm_header_t r;

	      r = p + extra + n;
	      r->magic = holy_MM_FREE_MAGIC;
	      r->size = p->size - extra - n;
	      r->next = p;

	      p->size = extra;
	      q->next = r;
	      p += extra;
	    }

	  p->magic = holy_MM_ALLOC_MAGIC;
	  p->size = n;

	  /* Mark find as a start marker for next allocation to fasten it.
	     This will have side effect of fragmenting memory as small
	     pieces before this will be un-used.  */
	  /* So do it only for chunks under 64K.  */
	  if (n < (0x8000 >> holy_MM_ALIGN_LOG2)
	      || *first == p)
	    *first = q;

	  return p + 1;
	}

      /* Search was completed without result.  */
      if (p == *first)
	break;
    }

  return 0;
}

/* Allocate SIZE bytes with the alignment ALIGN and return the pointer.  */
void *
holy_memalign (holy_size_t align, holy_size_t size)
{
  holy_mm_region_t r;
  holy_size_t n = ((size + holy_MM_ALIGN - 1) >> holy_MM_ALIGN_LOG2) + 1;
  int count = 0;

  if (!holy_mm_base)
    goto fail;

  if (size > ~(holy_size_t) align)
    goto fail;

  /* We currently assume at least a 32-bit holy_size_t,
     so limiting allocations to <adress space size> - 1MiB
     in name of sanity is beneficial. */
  if ((size + align) > ~(holy_size_t) 0x100000)
    goto fail;

  align = (align >> holy_MM_ALIGN_LOG2);
  if (align == 0)
    align = 1;

 again:

  for (r = holy_mm_base; r; r = r->next)
    {
      void *p;

      p = holy_real_malloc (&(r->first), n, align);
      if (p)
	return p;
    }

  /* If failed, increase free memory somehow.  */
  switch (count)
    {
    case 0:
      /* Invalidate disk caches.  */
      holy_disk_cache_invalidate_all ();
      count++;
      goto again;

#if 0
    case 1:
      /* Unload unneeded modules.  */
      holy_dl_unload_unneeded ();
      count++;
      goto again;
#endif

    default:
      break;
    }

 fail:
  holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
  return 0;
}

/* Allocate SIZE bytes and return the pointer.  */
void *
holy_malloc (holy_size_t size)
{
  return holy_memalign (0, size);
}

/* Allocate SIZE bytes, clear them and return the pointer.  */
void *
holy_zalloc (holy_size_t size)
{
  void *ret;

  ret = holy_memalign (0, size);
  if (ret)
    holy_memset (ret, 0, size);

  return ret;
}

/* Deallocate the pointer PTR.  */
void
holy_free (void *ptr)
{
  holy_mm_header_t p;
  holy_mm_region_t r;

  if (! ptr)
    return;

  get_header_from_pointer (ptr, &p, &r);

  if (r->first->magic == holy_MM_ALLOC_MAGIC)
    {
      p->magic = holy_MM_FREE_MAGIC;
      r->first = p->next = p;
    }
  else
    {
      holy_mm_header_t q, s;

#if 0
      q = r->first;
      do
	{
	  holy_printf ("%s:%d: q=%p, q->size=0x%x, q->magic=0x%x\n",
		       holy_FILE, __LINE__, q, q->size, q->magic);
	  q = q->next;
	}
      while (q != r->first);
#endif

      for (s = r->first, q = s->next; q <= p || q->next >= p; s = q, q = s->next)
	{
	  if (q->magic != holy_MM_FREE_MAGIC)
	    holy_fatal ("free magic is broken at %p: 0x%x", q, q->magic);

	  if (q <= q->next && (q > p || q->next < p))
	    break;
	}

      p->magic = holy_MM_FREE_MAGIC;
      p->next = q->next;
      q->next = p;

      if (p->next + p->next->size == p)
	{
	  p->magic = 0;

	  p->next->size += p->size;
	  q->next = p->next;
	  p = p->next;
	}

      r->first = q;

      if (q == p + p->size)
	{
	  q->magic = 0;
	  p->size += q->size;
	  if (q == s)
	    s = p;
	  s->next = p;
	  q = s;
	}

      r->first = q;
    }
}

/* Reallocate SIZE bytes and return the pointer. The contents will be
   the same as that of PTR.  */
void *
holy_realloc (void *ptr, holy_size_t size)
{
  holy_mm_header_t p;
  holy_mm_region_t r;
  void *q;
  holy_size_t n;

  if (! ptr)
    return holy_malloc (size);

  if (! size)
    {
      holy_free (ptr);
      return 0;
    }

  /* FIXME: Not optimal.  */
  n = ((size + holy_MM_ALIGN - 1) >> holy_MM_ALIGN_LOG2) + 1;
  get_header_from_pointer (ptr, &p, &r);

  if (p->size >= n)
    return ptr;

  q = holy_malloc (size);
  if (! q)
    return q;

  /* We've already checked that p->size < n.  */
  holy_memcpy (q, ptr, p->size << holy_MM_ALIGN_LOG2);
  holy_free (ptr);
  return q;
}

#ifdef MM_DEBUG
int holy_mm_debug = 0;

void
holy_mm_dump_free (void)
{
  holy_mm_region_t r;

  for (r = holy_mm_base; r; r = r->next)
    {
      holy_mm_header_t p;

      /* Follow the free list.  */
      p = r->first;
      do
	{
	  if (p->magic != holy_MM_FREE_MAGIC)
	    holy_fatal ("free magic is broken at %p: 0x%x", p, p->magic);

	  holy_printf ("F:%p:%u:%p\n",
		       p, (unsigned int) p->size << holy_MM_ALIGN_LOG2, p->next);
	  p = p->next;
	}
      while (p != r->first);
    }

  holy_printf ("\n");
}

void
holy_mm_dump (unsigned lineno)
{
  holy_mm_region_t r;

  holy_printf ("called at line %u\n", lineno);
  for (r = holy_mm_base; r; r = r->next)
    {
      holy_mm_header_t p;

      for (p = (holy_mm_header_t) ALIGN_UP ((holy_addr_t) (r + 1),
					    holy_MM_ALIGN);
	   (holy_addr_t) p < (holy_addr_t) (r+1) + r->size;
	   p++)
	{
	  switch (p->magic)
	    {
	    case holy_MM_FREE_MAGIC:
	      holy_printf ("F:%p:%u:%p\n",
			   p, (unsigned int) p->size << holy_MM_ALIGN_LOG2, p->next);
	      break;
	    case holy_MM_ALLOC_MAGIC:
	      holy_printf ("A:%p:%u\n", p, (unsigned int) p->size << holy_MM_ALIGN_LOG2);
	      break;
	    }
	}
    }

  holy_printf ("\n");
}

void *
holy_debug_malloc (const char *file, int line, holy_size_t size)
{
  void *ptr;

  if (holy_mm_debug)
    holy_printf ("%s:%d: malloc (0x%" PRIxholy_SIZE ") = ", file, line, size);
  ptr = holy_malloc (size);
  if (holy_mm_debug)
    holy_printf ("%p\n", ptr);
  return ptr;
}

void *
holy_debug_zalloc (const char *file, int line, holy_size_t size)
{
  void *ptr;

  if (holy_mm_debug)
    holy_printf ("%s:%d: zalloc (0x%" PRIxholy_SIZE ") = ", file, line, size);
  ptr = holy_zalloc (size);
  if (holy_mm_debug)
    holy_printf ("%p\n", ptr);
  return ptr;
}

void
holy_debug_free (const char *file, int line, void *ptr)
{
  if (holy_mm_debug)
    holy_printf ("%s:%d: free (%p)\n", file, line, ptr);
  holy_free (ptr);
}

void *
holy_debug_realloc (const char *file, int line, void *ptr, holy_size_t size)
{
  if (holy_mm_debug)
    holy_printf ("%s:%d: realloc (%p, 0x%" PRIxholy_SIZE ") = ", file, line, ptr, size);
  ptr = holy_realloc (ptr, size);
  if (holy_mm_debug)
    holy_printf ("%p\n", ptr);
  return ptr;
}

void *
holy_debug_memalign (const char *file, int line, holy_size_t align,
		    holy_size_t size)
{
  void *ptr;

  if (holy_mm_debug)
    holy_printf ("%s:%d: memalign (0x%" PRIxholy_SIZE  ", 0x%" PRIxholy_SIZE
		 ") = ", file, line, align, size);
  ptr = holy_memalign (align, size);
  if (holy_mm_debug)
    holy_printf ("%p\n", ptr);
  return ptr;
}

#endif /* MM_DEBUG */
