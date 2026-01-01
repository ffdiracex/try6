/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/list.h>
#include <holy/misc.h>
#include <holy/mm.h>

void *
holy_named_list_find (holy_named_list_t head, const char *name)
{
  holy_named_list_t item;

  FOR_LIST_ELEMENTS (item, head)
    if (holy_strcmp (item->name, name) == 0)
      return item;

  return NULL;
}

void
holy_list_push (holy_list_t *head, holy_list_t item)
{
  item->prev = head;
  if (*head)
    (*head)->prev = &item->next;
  item->next = *head;
  *head = item;
}

void
holy_list_remove (holy_list_t item)
{
  if (item->prev)
    *item->prev = item->next;
  if (item->next)
    item->next->prev = item->prev;
  item->next = 0;
  item->prev = 0;
}
