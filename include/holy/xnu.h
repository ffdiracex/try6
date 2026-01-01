/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_XNU_H
#define holy_XNU_H 1

#include <holy/bitmap.h>

/* Header of a hibernation image. */
struct holy_xnu_hibernate_header
{
  /* Size of the image. Notice that file containing image is usually bigger. */
  holy_uint64_t image_size;
  holy_uint8_t unknown1[8];
  /* Where to copy launchcode? */
  holy_uint32_t launchcode_target_page;
  /* How many pages of launchcode? */
  holy_uint32_t launchcode_numpages;
  /* Where to jump? */
  holy_uint32_t entry_point;
  /* %esp at start. */
  holy_uint32_t stack;
  holy_uint8_t unknown2[44];
#define holy_XNU_HIBERNATE_MAGIC 0x73696d65
  holy_uint32_t magic;
  holy_uint8_t unknown3[28];
  /* This value is non-zero if page is encrypted. Unsupported. */
  holy_uint64_t encoffset;
  holy_uint8_t unknown4[360];
  /* The size of additional header used to locate image without parsing FS.
     Used only to skip it.
   */
  holy_uint32_t extmapsize;
} holy_PACKED;

/* In-memory structure for temporary keeping device tree. */
struct holy_xnu_devtree_key
{
  char *name;
  int datasize; /* -1 for not leaves. */
  union
  {
    struct holy_xnu_devtree_key *first_child;
    void *data;
  };
  struct holy_xnu_devtree_key *next;
};

/* A structure used in memory-map values. */
struct
holy_xnu_extdesc
{
  holy_uint32_t addr;
  holy_uint32_t size;
} holy_PACKED;

/* Header describing extension in the memory. */
struct holy_xnu_extheader
{
  holy_uint32_t infoplistaddr;
  holy_uint32_t infoplistsize;
  holy_uint32_t binaryaddr;
  holy_uint32_t binarysize;
  holy_uint32_t nameaddr;
  holy_uint32_t namesize;
} holy_PACKED;

struct holy_xnu_devtree_key *holy_xnu_create_key (struct holy_xnu_devtree_key **parent,
						  const char *name);

extern struct holy_xnu_devtree_key *holy_xnu_devtree_root;

void holy_xnu_free_devtree (struct holy_xnu_devtree_key *cur);

holy_err_t holy_xnu_writetree_toheap (holy_addr_t *target, holy_size_t *size);
struct holy_xnu_devtree_key *holy_xnu_create_value (struct holy_xnu_devtree_key **parent,
						    const char *name);

void holy_xnu_lock (void);
void holy_xnu_unlock (void);
holy_err_t holy_xnu_resume (char *imagename);
holy_err_t holy_xnu_boot_resume (void);
struct holy_xnu_devtree_key *holy_xnu_find_key (struct holy_xnu_devtree_key *parent,
						const char *name);
holy_err_t holy_xnu_align_heap (int align);
holy_err_t holy_xnu_scan_dir_for_kexts (char *dirname,
					const char *osbundlerequired,
					int maxrecursion);
holy_err_t holy_xnu_load_kext_from_dir (char *dirname,
					const char *osbundlerequired,
					int maxrecursion);
holy_err_t holy_xnu_heap_malloc (int size, void **src, holy_addr_t *target);
holy_err_t holy_xnu_fill_devicetree (void);
extern struct holy_relocator *holy_xnu_relocator;

extern holy_size_t holy_xnu_heap_size;
extern struct holy_video_bitmap *holy_xnu_bitmap;
typedef enum {holy_XNU_BITMAP_CENTER, holy_XNU_BITMAP_STRETCH}
  holy_xnu_bitmap_mode_t;
extern holy_xnu_bitmap_mode_t holy_xnu_bitmap_mode;
extern int holy_xnu_is_64bit;
extern holy_addr_t holy_xnu_heap_target_start;
extern int holy_xnu_darwin_version;
#endif
