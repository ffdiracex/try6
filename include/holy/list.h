/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LIST_HEADER
#define holy_LIST_HEADER 1

#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/compiler.h>

struct holy_list
{
  struct holy_list *next;
  struct holy_list **prev;
};
typedef struct holy_list *holy_list_t;

void EXPORT_FUNC(holy_list_push) (holy_list_t *head, holy_list_t item);
void EXPORT_FUNC(holy_list_remove) (holy_list_t item);

#define FOR_LIST_ELEMENTS(var, list) for ((var) = (list); (var); (var) = (var)->next)
#define FOR_LIST_ELEMENTS_SAFE(var, nxt, list) for ((var) = (list), (nxt) = ((var) ? (var)->next : 0); (var); (var) = (nxt), ((nxt) = (var) ? (var)->next : 0))

static inline void *
holy_bad_type_cast_real (int line, const char *file)
     ATTRIBUTE_ERROR ("bad type cast between incompatible holy types");

static inline void *
holy_bad_type_cast_real (int line, const char *file)
{
  holy_fatal ("error:%s:%u: bad type cast between incompatible holy types",
	      file, line);
}

#define holy_bad_type_cast() holy_bad_type_cast_real(__LINE__, holy_FILE)

#define holy_FIELD_MATCH(ptr, type, field) \
  ((char *) &(ptr)->field == (char *) &((type) (ptr))->field)

#define holy_AS_LIST(ptr) \
  (holy_FIELD_MATCH (ptr, holy_list_t, next) && holy_FIELD_MATCH (ptr, holy_list_t, prev) ? \
   (holy_list_t) ptr : (holy_list_t) holy_bad_type_cast ())

#define holy_AS_LIST_P(pptr) \
  (holy_FIELD_MATCH (*pptr, holy_list_t, next) && holy_FIELD_MATCH (*pptr, holy_list_t, prev) ? \
   (holy_list_t *) (void *) pptr : (holy_list_t *) holy_bad_type_cast ())

struct holy_named_list
{
  struct holy_named_list *next;
  struct holy_named_list **prev;
  char *name;
};
typedef struct holy_named_list *holy_named_list_t;

void * EXPORT_FUNC(holy_named_list_find) (holy_named_list_t head,
					  const char *name);

#define holy_AS_NAMED_LIST(ptr) \
  ((holy_FIELD_MATCH (ptr, holy_named_list_t, next) \
    && holy_FIELD_MATCH (ptr, holy_named_list_t, prev)	\
    && holy_FIELD_MATCH (ptr, holy_named_list_t, name))? \
   (holy_named_list_t) ptr : (holy_named_list_t) holy_bad_type_cast ())

#define holy_AS_NAMED_LIST_P(pptr) \
  ((holy_FIELD_MATCH (*pptr, holy_named_list_t, next) \
    && holy_FIELD_MATCH (*pptr, holy_named_list_t, prev)   \
    && holy_FIELD_MATCH (*pptr, holy_named_list_t, name))? \
   (holy_named_list_t *) (void *) pptr : (holy_named_list_t *) holy_bad_type_cast ())

#endif /* ! holy_LIST_HEADER */
