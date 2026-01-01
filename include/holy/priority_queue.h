/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_PRIORITY_QUEUE_HEADER
#define holy_PRIORITY_QUEUE_HEADER 1

#include <holy/misc.h>
#include <holy/err.h>

#ifdef __cplusplus
extern "C" {
#endif

struct holy_priority_queue;
typedef struct holy_priority_queue *holy_priority_queue_t;
typedef int (*holy_comparator_t) (const void *a, const void *b);

holy_priority_queue_t holy_priority_queue_new (holy_size_t elsize,
					       holy_comparator_t cmp);
void holy_priority_queue_destroy (holy_priority_queue_t pq);
void *holy_priority_queue_top (holy_priority_queue_t pq);
void holy_priority_queue_pop (holy_priority_queue_t pq);
holy_err_t holy_priority_queue_push (holy_priority_queue_t pq, const void *el);

#ifdef __cplusplus
}
#endif

#endif
