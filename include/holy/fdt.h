/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FDT_HEADER
#define holy_FDT_HEADER	1

#include <holy/types.h>

#define FDT_MAGIC 0xD00DFEED

typedef struct {
	holy_uint32_t magic;
	holy_uint32_t totalsize;
	holy_uint32_t off_dt_struct;
	holy_uint32_t off_dt_strings;
	holy_uint32_t off_mem_rsvmap;
	holy_uint32_t version;
	holy_uint32_t last_comp_version;
	holy_uint32_t boot_cpuid_phys;
	holy_uint32_t size_dt_strings;
	holy_uint32_t size_dt_struct;
} holy_fdt_header_t;

struct holy_fdt_empty_tree {
  holy_fdt_header_t header;
  holy_uint64_t empty_rsvmap[2];
  struct {
    holy_uint32_t node_start;
    holy_uint8_t name[1];
    holy_uint32_t node_end;
    holy_uint32_t tree_end;
  } empty_node;
};

#define holy_FDT_EMPTY_TREE_SZ  sizeof (struct holy_fdt_empty_tree)

#define holy_fdt_get_header(fdt, field)	\
	holy_be_to_cpu32(((const holy_fdt_header_t *)(fdt))->field)
#define holy_fdt_set_header(fdt, field, value)	\
	((holy_fdt_header_t *)(fdt))->field = holy_cpu_to_be32(value)

#define holy_fdt_get_magic(fdt)	\
	holy_fdt_get_header(fdt, magic)
#define holy_fdt_set_magic(fdt, value)	\
	holy_fdt_set_header(fdt, magic, value)
#define holy_fdt_get_totalsize(fdt)	\
	holy_fdt_get_header(fdt, totalsize)
#define holy_fdt_set_totalsize(fdt, value)	\
	holy_fdt_set_header(fdt, totalsize, value)
#define holy_fdt_get_off_dt_struct(fdt)	\
	holy_fdt_get_header(fdt, off_dt_struct)
#define holy_fdt_set_off_dt_struct(fdt, value)	\
	holy_fdt_set_header(fdt, off_dt_struct, value)
#define holy_fdt_get_off_dt_strings(fdt)	\
	holy_fdt_get_header(fdt, off_dt_strings)
#define holy_fdt_set_off_dt_strings(fdt, value)	\
	holy_fdt_set_header(fdt, off_dt_strings, value)
#define holy_fdt_get_off_mem_rsvmap(fdt)	\
	holy_fdt_get_header(fdt, off_mem_rsvmap)
#define holy_fdt_set_off_mem_rsvmap(fdt, value)	\
	holy_fdt_set_header(fdt, off_mem_rsvmap, value)
#define holy_fdt_get_version(fdt)	\
	holy_fdt_get_header(fdt, version)
#define holy_fdt_set_version(fdt, value)	\
	holy_fdt_set_header(fdt, version, value)
#define holy_fdt_get_last_comp_version(fdt)	\
	holy_fdt_get_header(fdt, last_comp_version)
#define holy_fdt_set_last_comp_version(fdt, value)	\
	holy_fdt_set_header(fdt, last_comp_version, value)
#define holy_fdt_get_boot_cpuid_phys(fdt)	\
	holy_fdt_get_header(fdt, boot_cpuid_phys)
#define holy_fdt_set_boot_cpuid_phys(fdt, value)	\
	holy_fdt_set_header(fdt, boot_cpuid_phys, value)
#define holy_fdt_get_size_dt_strings(fdt)	\
	holy_fdt_get_header(fdt, size_dt_strings)
#define holy_fdt_set_size_dt_strings(fdt, value)	\
	holy_fdt_set_header(fdt, size_dt_strings, value)
#define holy_fdt_get_size_dt_struct(fdt)	\
	holy_fdt_get_header(fdt, size_dt_struct)
#define holy_fdt_set_size_dt_struct(fdt, value)	\
	holy_fdt_set_header(fdt, size_dt_struct, value)

int holy_fdt_create_empty_tree (void *fdt, unsigned int size);
int holy_fdt_check_header (void *fdt, unsigned int size);
int holy_fdt_check_header_nosize (void *fdt);
int holy_fdt_find_subnode (const void *fdt, unsigned int parentoffset,
			   const char *name);
int holy_fdt_add_subnode (void *fdt, unsigned int parentoffset,
			  const char *name);

int holy_fdt_set_prop (void *fdt, unsigned int nodeoffset, const char *name,
		      const void *val, holy_uint32_t len);
#define holy_fdt_set_prop32(fdt, nodeoffset, name, val)	\
({ \
  holy_uint32_t _val = holy_cpu_to_be32(val); \
  holy_fdt_set_prop ((fdt), (nodeoffset), (name), &_val, 4);	\
})

#define holy_fdt_set_prop64(fdt, nodeoffset, name, val)        \
({ \
  holy_uint64_t _val = holy_cpu_to_be64(val); \
  holy_fdt_set_prop ((fdt), (nodeoffset), (name), &_val, 8);   \
})

/* Setup "reg" property for
 * #address-cells = <0x2>
 * #size-cells = <0x2>
 */
#define holy_fdt_set_reg64(fdt, nodeoffset, addr, size)        \
({ \
  holy_uint64_t reg_64[2]; \
  reg_64[0] = holy_cpu_to_be64(addr); \
  reg_64[1] = holy_cpu_to_be64(size); \
  holy_fdt_set_prop ((fdt), (nodeoffset), "reg", reg_64, 16);  \
})

#endif	/* ! holy_FDT_HEADER */
