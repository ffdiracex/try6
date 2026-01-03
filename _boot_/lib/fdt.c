/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/fdt.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

#define FDT_SUPPORTED_VERSION	17

#define FDT_BEGIN_NODE	0x00000001
#define FDT_END_NODE	0x00000002
#define FDT_PROP	0x00000003
#define FDT_NOP		0x00000004
#define FDT_END		0x00000009

#define struct_end(fdt)	\
	((holy_addr_t) fdt + holy_fdt_get_off_dt_struct(fdt)	\
	 + holy_fdt_get_size_dt_struct(fdt))

/* Size needed by a node entry: 2 tokens (FDT_BEGIN_NODE and FDT_END_NODE), plus
   the NULL-terminated string containing the name, plus padding if needed. */
#define node_entry_size(node_name)	\
	(2 * sizeof(holy_uint32_t)	\
	+ ALIGN_UP (holy_strlen (name) + 1, sizeof(holy_uint32_t)))

/* Size needed by a property entry: 1 token (FDT_PROPERTY), plus len and nameoff
   fields, plus the property value, plus padding if needed. */
#define prop_entry_size(prop_len)	\
	(3 * sizeof(holy_uint32_t) + ALIGN_UP(prop_len, sizeof(holy_uint32_t)))

#define SKIP_NODE_NAME(name, token, end)	\
  name = (char *) ((token) + 1);	\
  while (name < (char *) end)	\
  {	\
    if (!*name++)	\
      break;	\
  }	\
  token = (holy_uint32_t *) ALIGN_UP((holy_addr_t) (name), sizeof(*token))


static holy_uint32_t *get_next_node (const void *fdt, char *node_name)
{
  holy_uint32_t *end = (void *) struct_end (fdt);
  holy_uint32_t *token;

  if (node_name >= (char *) end)
    return NULL;
  while (*node_name++)
  {
    if (node_name >= (char *) end)
  	  return NULL;
  }
  token = (holy_uint32_t *) ALIGN_UP ((holy_addr_t) node_name, 4);
  while (token < end)
  {
    switch (holy_be_to_cpu32(*token))
    {
      case FDT_BEGIN_NODE:
        token = get_next_node (fdt, (char *) (token + 1));
        if (!token)
          return NULL;
        break;
      case FDT_END_NODE:
        token++;
        if (token >= end)
          return NULL;
        return token;
      case FDT_PROP:
        /* Skip property token and following data (len, nameoff and property
           value). */
        token += prop_entry_size(holy_be_to_cpu32(*(token + 1)))
                 / sizeof(*token);
        break;
      case FDT_NOP:
        token++;
        break;
      default:
        return NULL;
    }
  }
  return NULL;
}

static int get_mem_rsvmap_size (const void *fdt)
{
  int size = 0;
  holy_uint64_t *ptr = (void *) ((holy_addr_t) fdt
                                 + holy_fdt_get_off_mem_rsvmap (fdt));

  do
  {
    size += 2 * sizeof(*ptr);
    if (!*ptr && !*(ptr + 1))
      return size;
    ptr += 2;
  } while ((holy_addr_t) ptr <= (holy_addr_t) fdt + holy_fdt_get_totalsize (fdt)
                                  - 2 * sizeof(holy_uint64_t));
  return -1;
}

static holy_uint32_t get_free_space (void *fdt)
{
  int mem_rsvmap_size = get_mem_rsvmap_size (fdt);

  if (mem_rsvmap_size < 0)
    /* invalid memory reservation block */
    return 0;
  return (holy_fdt_get_totalsize (fdt) - sizeof(holy_fdt_header_t)
          - mem_rsvmap_size - holy_fdt_get_size_dt_strings (fdt)
          - holy_fdt_get_size_dt_struct (fdt));
}

static int add_subnode (void *fdt, int parentoffset, const char *name)
{
  holy_uint32_t *token = (void *) ((holy_addr_t) fdt
                                   + holy_fdt_get_off_dt_struct(fdt)
                                   + parentoffset);
  holy_uint32_t *end = (void *) struct_end (fdt);
  unsigned int entry_size = node_entry_size (name);
  unsigned int struct_size = holy_fdt_get_size_dt_struct(fdt);
  char *node_name;

  SKIP_NODE_NAME(node_name, token, end);

  /* Insert the new subnode just after the properties of the parent node (if
     any).*/
  while (1)
  {
    if (token >= end)
      return -1;
    switch (holy_be_to_cpu32(*token))
    {
      case FDT_PROP:
        /* Skip len, nameoff and property value. */
        token += prop_entry_size(holy_be_to_cpu32(*(token + 1)))
                 / sizeof(*token);
        break;
      case FDT_BEGIN_NODE:
      case FDT_END_NODE:
        goto insert;
      case FDT_NOP:
        token++;
        break;
      default:
        /* invalid token */
        return -1;
    }
  }
insert:
  holy_memmove (token + entry_size / sizeof(*token), token,
                (holy_addr_t) end - (holy_addr_t) token);
  *token = holy_cpu_to_be32_compile_time(FDT_BEGIN_NODE);
  token[entry_size / sizeof(*token) - 2] = 0;	/* padding bytes */
  holy_strcpy((char *) (token + 1), name);
  token[entry_size / sizeof(*token) - 1] = holy_cpu_to_be32_compile_time(FDT_END_NODE);
  holy_fdt_set_size_dt_struct (fdt, struct_size + entry_size);
  return ((holy_addr_t) token - (holy_addr_t) fdt
          - holy_fdt_get_off_dt_struct(fdt));
}

/* Rearrange FDT blocks in the canonical order: first the memory reservation
   block (just after the FDT header), then the structure block and finally the
   strings block. No free space is left between the first and the second block,
   while the space between the second and the third block is given by the
   clearance argument. */
static int rearrange_blocks (void *fdt, unsigned int clearance)
{
  holy_uint32_t off_mem_rsvmap = ALIGN_UP(sizeof(holy_fdt_header_t), 8);
  holy_uint32_t off_dt_struct = off_mem_rsvmap + get_mem_rsvmap_size (fdt);
  holy_uint32_t off_dt_strings = off_dt_struct
                                 + holy_fdt_get_size_dt_struct (fdt)
                                 + clearance;
  holy_uint8_t *fdt_ptr = fdt;
  holy_uint8_t *tmp_fdt;

  if ((holy_fdt_get_off_mem_rsvmap (fdt) == off_mem_rsvmap)
      && (holy_fdt_get_off_dt_struct (fdt) == off_dt_struct))
    {
      /* No need to allocate memory for a temporary FDT, just move the strings
         block if needed. */
      if (holy_fdt_get_off_dt_strings (fdt) != off_dt_strings)
        {
          holy_memmove(fdt_ptr + off_dt_strings,
                       fdt_ptr + holy_fdt_get_off_dt_strings (fdt),
                       holy_fdt_get_size_dt_strings (fdt));
          holy_fdt_set_off_dt_strings (fdt, off_dt_strings);
        }
      return 0;
    }
  tmp_fdt = holy_malloc (holy_fdt_get_totalsize (fdt));
  if (!tmp_fdt)
    return -1;
  holy_memcpy (tmp_fdt + off_mem_rsvmap,
               fdt_ptr + holy_fdt_get_off_mem_rsvmap (fdt),
               get_mem_rsvmap_size (fdt));
  holy_fdt_set_off_mem_rsvmap (fdt, off_mem_rsvmap);
  holy_memcpy (tmp_fdt + off_dt_struct,
               fdt_ptr + holy_fdt_get_off_dt_struct (fdt),
               holy_fdt_get_size_dt_struct (fdt));
  holy_fdt_set_off_dt_struct (fdt, off_dt_struct);
  holy_memcpy (tmp_fdt + off_dt_strings,
               fdt_ptr + holy_fdt_get_off_dt_strings (fdt),
               holy_fdt_get_size_dt_strings (fdt));
  holy_fdt_set_off_dt_strings (fdt, off_dt_strings);

  /* Copy reordered blocks back to fdt. */
  holy_memcpy (fdt_ptr + off_mem_rsvmap, tmp_fdt + off_mem_rsvmap,
               holy_fdt_get_totalsize (fdt) - off_mem_rsvmap);

  holy_free(tmp_fdt);
  return 0;
}

static holy_uint32_t *find_prop (void *fdt, unsigned int nodeoffset,
				 const char *name)
{
  holy_uint32_t *prop = (void *) ((holy_addr_t) fdt
                                 + holy_fdt_get_off_dt_struct (fdt)
                                 + nodeoffset);
  holy_uint32_t *end = (void *) struct_end(fdt);
  holy_uint32_t nameoff;
  char *node_name;

  SKIP_NODE_NAME(node_name, prop, end);
  while (prop < end - 2)
  {
    if (holy_be_to_cpu32(*prop) == FDT_PROP)
      {
        nameoff = holy_be_to_cpu32(*(prop + 2));
        if ((nameoff + holy_strlen (name) < holy_fdt_get_size_dt_strings (fdt))
            && !holy_strcmp (name, (char *) fdt +
                             holy_fdt_get_off_dt_strings (fdt) + nameoff))
        {
          if (prop + prop_entry_size(holy_be_to_cpu32(*(prop + 1)))
              / sizeof (*prop) >= end)
            return NULL;
          return prop;
        }
        prop += prop_entry_size(holy_be_to_cpu32(*(prop + 1))) / sizeof (*prop);
      }
    else if (holy_be_to_cpu32(*prop) == FDT_NOP)
      prop++;
    else
      return NULL;
  }
  return NULL;
}

/* Check the FDT header for consistency and adjust the totalsize field to match
   the size allocated for the FDT; if this function is called before the other
   functions in this file and returns success, the other functions are
   guaranteed not to access memory locations outside the allocated memory. */
int holy_fdt_check_header_nosize (void *fdt)
{
  if (((holy_addr_t) fdt & 0x7) || (holy_fdt_get_magic (fdt) != FDT_MAGIC)
      || (holy_fdt_get_version (fdt) < FDT_SUPPORTED_VERSION)
      || (holy_fdt_get_last_comp_version (fdt) > FDT_SUPPORTED_VERSION)
      || (holy_fdt_get_off_dt_struct (fdt) & 0x00000003)
      || (holy_fdt_get_size_dt_struct (fdt) & 0x00000003)
      || (holy_fdt_get_off_dt_struct (fdt) + holy_fdt_get_size_dt_struct (fdt)
          > holy_fdt_get_totalsize (fdt))
      || (holy_fdt_get_off_dt_strings (fdt) + holy_fdt_get_size_dt_strings (fdt)
          > holy_fdt_get_totalsize (fdt))
      || (holy_fdt_get_off_mem_rsvmap (fdt) & 0x00000007)
      || (holy_fdt_get_off_mem_rsvmap (fdt)
          > holy_fdt_get_totalsize (fdt) - 2 * sizeof(holy_uint64_t)))
    return -1;
  return 0;
}

int holy_fdt_check_header (void *fdt, unsigned int size)
{
  if (size < sizeof (holy_fdt_header_t)
      || (holy_fdt_get_totalsize (fdt) > size)
      || holy_fdt_check_header_nosize (fdt) == -1)
    return -1;
  return 0;
}

/* Find a direct sub-node of a given parent node. */
int holy_fdt_find_subnode (const void *fdt, unsigned int parentoffset,
			   const char *name)
{
  holy_uint32_t *token, *end;
  char *node_name;

  if (parentoffset & 0x3)
    return -1;
  token = (void *) ((holy_addr_t) fdt + holy_fdt_get_off_dt_struct(fdt)
                    + parentoffset);
  end = (void *) struct_end (fdt);
  if ((token >= end) || (holy_be_to_cpu32(*token) != FDT_BEGIN_NODE))
    return -1;
  SKIP_NODE_NAME(node_name, token, end);
  while (token < end)
  {
    switch (holy_be_to_cpu32(*token))
    {
      case FDT_BEGIN_NODE:
        node_name = (char *) (token + 1);
        if (node_name + holy_strlen (name) >= (char *) end)
          return -1;
        if (!holy_strcmp (node_name, name))
          return (int) ((holy_addr_t) token - (holy_addr_t) fdt
                        - holy_fdt_get_off_dt_struct (fdt));
        token = get_next_node (fdt, node_name);
        if (!token)
          return -1;
        break;
      case FDT_PROP:
        /* Skip property token and following data (len, nameoff and property
           value). */
        if (token >= end - 1)
          return -1;
        token += prop_entry_size(holy_be_to_cpu32(*(token + 1)))
                 / sizeof(*token);
        break;
      case FDT_NOP:
        token++;
        break;
      default:
        return -1;
    }
  }
  return -1;
}

int holy_fdt_add_subnode (void *fdt, unsigned int parentoffset,
			  const char *name)
{
  unsigned int entry_size = node_entry_size(name);

  if ((parentoffset & 0x3) || (get_free_space (fdt) < entry_size))
    return -1;

  /* The new node entry will increase the size of the structure block: rearrange
     blocks such that there is sufficient free space between the structure and
     the strings block, then add the new node entry. */
  if (rearrange_blocks (fdt, entry_size) < 0)
    return -1;
  return add_subnode (fdt, parentoffset, name);
}

int holy_fdt_set_prop (void *fdt, unsigned int nodeoffset, const char *name,
		       const void *val, holy_uint32_t len)
{
  holy_uint32_t *prop;
  int prop_name_present = 0;
  holy_uint32_t nameoff = 0;

  if ((nodeoffset >= holy_fdt_get_size_dt_struct (fdt)) || (nodeoffset & 0x3)
      || (holy_be_to_cpu32(*(holy_uint32_t *) ((holy_addr_t) fdt
                           + holy_fdt_get_off_dt_struct (fdt) + nodeoffset))
          != FDT_BEGIN_NODE))
    return -1;
  prop = find_prop (fdt, nodeoffset, name);
  if (prop)
    {
	  holy_uint32_t prop_len = ALIGN_UP(holy_be_to_cpu32 (*(prop + 1)),
                                        sizeof(holy_uint32_t));
	  holy_uint32_t i;

      prop_name_present = 1;
	  for (i = 0; i < prop_len / sizeof(holy_uint32_t); i++)
        *(prop + 3 + i) = holy_cpu_to_be32_compile_time (FDT_NOP);
      if (len > ALIGN_UP(prop_len, sizeof(holy_uint32_t)))
        {
          /* Length of new property value is greater than the space allocated
             for the current value: a new entry needs to be created, so save the
             nameoff field of the current entry and replace the current entry
             with NOP tokens. */
          nameoff = holy_be_to_cpu32 (*(prop + 2));
          *prop = *(prop + 1) = *(prop + 2) = holy_cpu_to_be32_compile_time (FDT_NOP);
          prop = NULL;
        }
    }
  if (!prop || !prop_name_present) {
    unsigned int needed_space = 0;

    if (!prop)
      needed_space = prop_entry_size(len);
    if (!prop_name_present)
      needed_space += holy_strlen (name) + 1;
    if (needed_space > get_free_space (fdt))
      return -1;
    if (rearrange_blocks (fdt, !prop ? prop_entry_size(len) : 0) < 0)
      return -1;
  }
  if (!prop_name_present) {
    /* Append the property name at the end of the strings block. */
    nameoff = holy_fdt_get_size_dt_strings (fdt);
    holy_strcpy ((char *) fdt + holy_fdt_get_off_dt_strings (fdt) + nameoff,
                 name);
    holy_fdt_set_size_dt_strings (fdt, holy_fdt_get_size_dt_strings (fdt)
                                  + holy_strlen (name) + 1);
  }
  if (!prop) {
    char *node_name = (char *) ((holy_addr_t) fdt
                                + holy_fdt_get_off_dt_struct (fdt) + nodeoffset
                                + sizeof(holy_uint32_t));

    prop = (void *) (node_name + ALIGN_UP(holy_strlen(node_name) + 1, 4));
    holy_memmove (prop + prop_entry_size(len) / sizeof(*prop), prop,
                  struct_end(fdt) - (holy_addr_t) prop);
    holy_fdt_set_size_dt_struct (fdt, holy_fdt_get_size_dt_struct (fdt)
                                 + prop_entry_size(len));
    *prop = holy_cpu_to_be32_compile_time (FDT_PROP);
    *(prop + 2) = holy_cpu_to_be32 (nameoff);
  }
  *(prop + 1) = holy_cpu_to_be32 (len);

  /* Insert padding bytes at the end of the value; if they are not needed, they
     will be overwritten by the following memcpy. */
  *(prop + prop_entry_size(len) / sizeof(holy_uint32_t) - 1) = 0;

  holy_memcpy (prop + 3, val, len);
  return 0;
}

int
holy_fdt_create_empty_tree (void *fdt, unsigned int size)
{
  struct holy_fdt_empty_tree *et;

  if (size < holy_FDT_EMPTY_TREE_SZ)
    return -1;

  holy_memset (fdt, 0, size);
  et = fdt;

  et->empty_node.tree_end = holy_cpu_to_be32_compile_time (FDT_END);
  et->empty_node.node_end = holy_cpu_to_be32_compile_time (FDT_END_NODE);
  et->empty_node.node_start = holy_cpu_to_be32_compile_time (FDT_BEGIN_NODE);
  ((struct holy_fdt_empty_tree *) fdt)->header.off_mem_rsvmap =
    holy_cpu_to_be32_compile_time (ALIGN_UP (sizeof (holy_fdt_header_t), 8));

  holy_fdt_set_off_dt_strings (fdt, sizeof (*et));
  holy_fdt_set_off_dt_struct (fdt,
			      sizeof (et->header) + sizeof (et->empty_rsvmap));
  holy_fdt_set_version (fdt, FDT_SUPPORTED_VERSION);
  holy_fdt_set_last_comp_version (fdt, FDT_SUPPORTED_VERSION);
  holy_fdt_set_size_dt_struct (fdt, sizeof (et->empty_node));
  holy_fdt_set_totalsize (fdt, size);
  holy_fdt_set_magic (fdt, FDT_MAGIC);

  return 0;
}
