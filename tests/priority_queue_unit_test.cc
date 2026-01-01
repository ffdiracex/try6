// Copyright 2025 Felix P. A. Gillberg HolyBooter
// SPDX-License-Identifier: GPL-2.0

#include <stdio.h>
#include <string.h>
#include <holy/test.h>
#include <holy/misc.h>
#include <holy/priority_queue.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <queue>

using namespace std;

static int 
compar (const void *a_, const void *b_)
{
  int a = *(int *) a_;
  int b = *(int *) b_;
  if (a < b)
    return -1;
  if (a > b)
    return +1;
  return 0;
}

static void
priority_queue_test (void)
{
  priority_queue <int> pq;
  holy_priority_queue_t pq2;
  int counter;
  int s = 0;
  pq2 = holy_priority_queue_new (sizeof (int), compar);
  if (!pq2)
    {
      holy_test_assert (0,
			"priority queue: queue creating failed\n");
      return;
    }
  srand (1);

  for (counter = 0; counter < 1000000; counter++)
    {
      int op = rand () % 10;
      if (s && *(int *) holy_priority_queue_top (pq2) != pq.top ())
	{
	  printf ("Error at %d\n", counter);
	  holy_test_assert (0,
			    "priority queue: error at %d\n", counter);
	  return;
	}
      if (op < 3 && s)
	{
	  holy_priority_queue_pop (pq2);
	  pq.pop ();
	  s--;
	}
      else
	{
	  int v = rand ();
	  pq.push (v);
	  if (holy_priority_queue_push (pq2, &v) != 0)
	    {
	      holy_test_assert (0,
				"priority queue: push failed");
	      return;
	    }
	  s++;
	}
    }
  while (s)
    {
      if (*(int *) holy_priority_queue_top (pq2) != pq.top ())
	{
	  holy_test_assert (0,
			    "priority queue: Error at the end. %d elements remaining.\n", s);
	  return;
	}
      holy_priority_queue_pop (pq2);
      pq.pop ();
      s--;
    }
  printf ("priority_queue: passed successfully\n");
}

holy_UNIT_TEST ("priority_queue_unit_test", priority_queue_test);
