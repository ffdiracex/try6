/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/priority_queue.h>
#include <holy/mm.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_priority_queue
{
  holy_size_t elsize;
  holy_size_t allocated;
  holy_size_t used;
  holy_comparator_t cmp;
  void *els;
};

static inline void *
element (struct holy_priority_queue *pq, holy_size_t k)
{
  return ((holy_uint8_t *) pq->els) + k * pq->elsize;
}

static inline void
swap (struct holy_priority_queue *pq, holy_size_t m, holy_size_t n)
{
  holy_uint8_t *p1, *p2;
  holy_size_t l;
  p1 = (holy_uint8_t *) element (pq, m);
  p2 = (holy_uint8_t *) element (pq, n);
  for (l = pq->elsize; l; l--, p1++, p2++)
    {
      holy_uint8_t t;
      t = *p1;
      *p1 = *p2;
      *p2 = t;
    }
}

static inline holy_size_t
parent (holy_size_t v)
{
  return (v - 1) / 2;
}

static inline holy_size_t
left_child (holy_size_t v)
{
  return 2 * v + 1;
}

static inline holy_size_t
right_child (holy_size_t v)
{
  return 2 * v + 2;
}

void *
holy_priority_queue_top (holy_priority_queue_t pq)
{
  if (!pq->used)
    return 0;
  return element (pq, 0);
}

void
holy_priority_queue_destroy (holy_priority_queue_t pq)
{
  holy_free (pq->els);
  holy_free (pq);
}

holy_priority_queue_t
holy_priority_queue_new (holy_size_t elsize,
			 holy_comparator_t cmp)
{
  struct holy_priority_queue *ret;
  void *els;
  els = holy_malloc (elsize * 8);
  if (!els)
    return 0;
  ret = (struct holy_priority_queue *) holy_malloc (sizeof (*ret));
  if (!ret)
    {
      holy_free (els);
      return 0;
    }
  ret->elsize = elsize;
  ret->allocated = 8;
  ret->used = 0;
  ret->cmp = cmp;
  ret->els = els;
  return ret;
}

/* Heap property: pq->cmp (element (pq, p), element (pq, parent (p))) <= 0. */
holy_err_t
holy_priority_queue_push (holy_priority_queue_t pq, const void *el)
{
  holy_size_t p;
  if (pq->used == pq->allocated)
    {
      void *els;
      els = holy_realloc (pq->els, pq->elsize * 2 * pq->allocated);
      if (!els)
	return holy_errno;
      pq->allocated *= 2;
      pq->els = els;
    }
  pq->used++;
  holy_memcpy (element (pq, pq->used - 1), el, pq->elsize);
  for (p = pq->used - 1; p; p = parent (p))
    {
      if (pq->cmp (element (pq, p), element (pq, parent (p))) <= 0)
	break;
      swap (pq, p, parent (p));
    }

  return holy_ERR_NONE;
}

void
holy_priority_queue_pop (holy_priority_queue_t pq)
{
  holy_size_t p;
  
  swap (pq, 0, pq->used - 1);
  pq->used--;
  for (p = 0; left_child (p) < pq->used; )
    {
      holy_size_t c;
      if (pq->cmp (element (pq, left_child (p)), element (pq, p)) <= 0
	  && (right_child (p) >= pq->used
	      || pq->cmp (element (pq, right_child (p)), element (pq, p)) <= 0))
	break;
      if (right_child (p) >= pq->used
	  || pq->cmp (element (pq, left_child (p)),
		      element (pq, right_child (p))) > 0)
	c = left_child (p);
      else
	c = right_child (p);
      swap (pq, p, c);
      p = c;
    }
}


