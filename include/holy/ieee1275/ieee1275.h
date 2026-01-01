/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_IEEE1275_HEADER
#define holy_IEEE1275_HEADER	1

#include <holy/err.h>
#include <holy/types.h>
#include <holy/machine/ieee1275.h>

struct holy_ieee1275_mem_region
{
  unsigned int start;
  unsigned int size;
};

#define IEEE1275_MAX_PROP_LEN	8192
#define IEEE1275_MAX_PATH_LEN	256

#ifndef IEEE1275_CALL_ENTRY_FN
#define IEEE1275_CALL_ENTRY_FN(args) (*holy_ieee1275_entry_fn) (args)
#endif

/* All backcalls to the firmware is done by calling an entry function
   which was passed to us from the bootloader.  When doing the backcall,
   a structure is passed which specifies what the firmware should do.
   NAME is the requested service.  NR_INS and NR_OUTS is the number of
   passed arguments and the expected number of return values, resp. */
struct holy_ieee1275_common_hdr
{
  holy_ieee1275_cell_t name;
  holy_ieee1275_cell_t nr_ins;
  holy_ieee1275_cell_t nr_outs;
};

#define INIT_IEEE1275_COMMON(p, xname, xins, xouts) \
  (p)->name = (holy_ieee1275_cell_t) xname; \
  (p)->nr_ins = (holy_ieee1275_cell_t) xins; \
  (p)->nr_outs = (holy_ieee1275_cell_t) xouts

typedef holy_uint32_t holy_ieee1275_ihandle_t;
typedef holy_uint32_t holy_ieee1275_phandle_t;

#define holy_IEEE1275_PHANDLE_INVALID  ((holy_ieee1275_phandle_t) -1)

struct holy_ieee1275_devalias
{
  char *name;
  char *path;
  char *type;
  char *parent_path;
  holy_ieee1275_phandle_t phandle;
  holy_ieee1275_phandle_t parent_dev;
};

extern void (*EXPORT_VAR(holy_ieee1275_net_config)) (const char *dev,
                                                     char **device,
                                                     char **path,
                                                     char *bootargs);

/* Maps a device alias to a pathname.  */
extern holy_ieee1275_phandle_t EXPORT_VAR(holy_ieee1275_chosen);
extern holy_ieee1275_ihandle_t EXPORT_VAR(holy_ieee1275_mmu);
#ifdef __i386__
#define holy_IEEE1275_ENTRY_FN_ATTRIBUTE  __attribute__ ((regparm(3)))
#else
#define holy_IEEE1275_ENTRY_FN_ATTRIBUTE
#endif

extern int (* EXPORT_VAR(holy_ieee1275_entry_fn)) (void *) holy_IEEE1275_ENTRY_FN_ATTRIBUTE;

/* Static heap, used only if FORCE_CLAIM is set,
   happens on Open Hack'Ware. Should be in platform-specific
   header but is used only on PPC anyway.
*/
#define holy_IEEE1275_STATIC_HEAP_START 0x1000000
#define holy_IEEE1275_STATIC_HEAP_LEN   0x1000000


enum holy_ieee1275_flag
{
  /* Old World Macintosh firmware fails seek when "dev:0" is opened.  */
  holy_IEEE1275_FLAG_NO_PARTITION_0,

  /* Apple firmware runs in translated mode and requires use of the "map"
     method.  Other firmware runs in untranslated mode and doesn't like "map"
     calls.  */
  holy_IEEE1275_FLAG_REAL_MODE,

  /* CHRP specifies partitions are numbered from 1 (partition 0 refers to the
     whole disk). However, CodeGen firmware numbers partitions from 0.  */
  holy_IEEE1275_FLAG_0_BASED_PARTITIONS,

  /* CodeGen firmware does not correctly implement "output-device output" */
  holy_IEEE1275_FLAG_BROKEN_OUTPUT,

  /* OLPC / XO firmware hangs when accessing USB devices.  */
  holy_IEEE1275_FLAG_OFDISK_SDCARD_ONLY,

  /* Open Hack'Ware stops when trying to set colors */
  holy_IEEE1275_FLAG_CANNOT_SET_COLORS,

  /* Open Hack'Ware stops when holy_ieee1275_interpret is used.  */
  holy_IEEE1275_FLAG_CANNOT_INTERPRET,

  /* Open Hack'Ware has no memory map, just claim what we need.  */
  holy_IEEE1275_FLAG_FORCE_CLAIM,

  /* Open Hack'Ware don't support the ANSI sequence.  */
  holy_IEEE1275_FLAG_NO_ANSI,

  /* OpenFirmware hangs on qemu if one requests any memory below 1.5 MiB.  */
  holy_IEEE1275_FLAG_NO_PRE1_5M_CLAIM,

  /* OLPC / XO firmware has the cursor ON/OFF routines.  */
  holy_IEEE1275_FLAG_HAS_CURSORONOFF,

  /* Some PowerMacs claim to use 2 address cells but in fact use only 1. 
     Other PowerMacs claim to use only 1 and really do so. Always assume
     1 address cell is used on PowerMacs.
   */
  holy_IEEE1275_FLAG_BROKEN_ADDRESS_CELLS,

  holy_IEEE1275_FLAG_NO_TREE_SCANNING_FOR_DISKS,

  holy_IEEE1275_FLAG_NO_OFNET_SUFFIX,

  holy_IEEE1275_FLAG_VIRT_TO_REAL_BROKEN,

  holy_IEEE1275_FLAG_BROKEN_REPEAT,

  holy_IEEE1275_FLAG_CURSORONOFF_ANSI_BROKEN,
};

extern int EXPORT_FUNC(holy_ieee1275_test_flag) (enum holy_ieee1275_flag flag);
extern void EXPORT_FUNC(holy_ieee1275_set_flag) (enum holy_ieee1275_flag flag);




void EXPORT_FUNC(holy_ieee1275_init) (void);
int EXPORT_FUNC(holy_ieee1275_finddevice) (const char *name,
					   holy_ieee1275_phandle_t *phandlep);
int EXPORT_FUNC(holy_ieee1275_get_property) (holy_ieee1275_phandle_t phandle,
					     const char *property, void *buf,
					     holy_size_t size,
					     holy_ssize_t *actual);
int EXPORT_FUNC(holy_ieee1275_get_integer_property) (holy_ieee1275_phandle_t phandle,
						     const char *property, holy_uint32_t *buf,
						     holy_size_t size,
						     holy_ssize_t *actual);
int EXPORT_FUNC(holy_ieee1275_next_property) (holy_ieee1275_phandle_t phandle,
					      char *prev_prop, char *prop);
int EXPORT_FUNC(holy_ieee1275_get_property_length)
     (holy_ieee1275_phandle_t phandle, const char *prop, holy_ssize_t *length);
int EXPORT_FUNC(holy_ieee1275_instance_to_package)
     (holy_ieee1275_ihandle_t ihandle, holy_ieee1275_phandle_t *phandlep);
int EXPORT_FUNC(holy_ieee1275_package_to_path) (holy_ieee1275_phandle_t phandle,
						char *path, holy_size_t len,
						holy_ssize_t *actual);
int EXPORT_FUNC(holy_ieee1275_instance_to_path)
     (holy_ieee1275_ihandle_t ihandle, char *path, holy_size_t len,
      holy_ssize_t *actual);
int EXPORT_FUNC(holy_ieee1275_write) (holy_ieee1275_ihandle_t ihandle,
				      const void *buffer, holy_size_t len,
				      holy_ssize_t *actualp);
int EXPORT_FUNC(holy_ieee1275_read) (holy_ieee1275_ihandle_t ihandle,
				     void *buffer, holy_size_t len,
				     holy_ssize_t *actualp);
int EXPORT_FUNC(holy_ieee1275_seek) (holy_ieee1275_ihandle_t ihandle,
				     holy_disk_addr_t pos,
				     holy_ssize_t *result);
int EXPORT_FUNC(holy_ieee1275_peer) (holy_ieee1275_phandle_t node,
				     holy_ieee1275_phandle_t *result);
int EXPORT_FUNC(holy_ieee1275_child) (holy_ieee1275_phandle_t node,
				      holy_ieee1275_phandle_t *result);
int EXPORT_FUNC(holy_ieee1275_parent) (holy_ieee1275_phandle_t node,
				       holy_ieee1275_phandle_t *result);
int EXPORT_FUNC(holy_ieee1275_interpret) (const char *command,
					  holy_ieee1275_cell_t *catch);
int EXPORT_FUNC(holy_ieee1275_enter) (void);
void EXPORT_FUNC(holy_ieee1275_exit) (void) __attribute__ ((noreturn));
int EXPORT_FUNC(holy_ieee1275_open) (const char *node,
				     holy_ieee1275_ihandle_t *result);
int EXPORT_FUNC(holy_ieee1275_close) (holy_ieee1275_ihandle_t ihandle);
int EXPORT_FUNC(holy_ieee1275_claim) (holy_addr_t addr, holy_size_t size,
				      unsigned int align, holy_addr_t *result);
int EXPORT_FUNC(holy_ieee1275_release) (holy_addr_t addr, holy_size_t size);
int EXPORT_FUNC(holy_ieee1275_set_property) (holy_ieee1275_phandle_t phandle,
					     const char *propname,
					     const void *buf,
					     holy_size_t size,
					     holy_ssize_t *actual);
int EXPORT_FUNC(holy_ieee1275_set_color) (holy_ieee1275_ihandle_t ihandle,
					  int index, int r, int g, int b);
int EXPORT_FUNC(holy_ieee1275_milliseconds) (holy_uint32_t *msecs);


holy_err_t EXPORT_FUNC(holy_claimmap) (holy_addr_t addr, holy_size_t size);

int
EXPORT_FUNC(holy_ieee1275_map) (holy_addr_t phys, holy_addr_t virt,
				holy_size_t size, holy_uint32_t mode);

char *EXPORT_FUNC(holy_ieee1275_encode_devname) (const char *path);
char *EXPORT_FUNC(holy_ieee1275_get_filename) (const char *path);
int EXPORT_FUNC(holy_ieee1275_devices_iterate) (int (*hook)
						(struct holy_ieee1275_devalias *
						 alias));
char *EXPORT_FUNC(holy_ieee1275_get_aliasdevname) (const char *path);
char *EXPORT_FUNC(holy_ieee1275_canonicalise_devname) (const char *path);
char *EXPORT_FUNC(holy_ieee1275_get_device_type) (const char *path);
char *EXPORT_FUNC(holy_ieee1275_get_devname) (const char *path);

void EXPORT_FUNC(holy_ieee1275_devalias_init_iterator) (struct holy_ieee1275_devalias *alias);
void EXPORT_FUNC(holy_ieee1275_devalias_free) (struct holy_ieee1275_devalias *alias);
int EXPORT_FUNC(holy_ieee1275_devalias_next) (struct holy_ieee1275_devalias *alias);
void EXPORT_FUNC(holy_ieee1275_children_peer) (struct holy_ieee1275_devalias *alias);
void EXPORT_FUNC(holy_ieee1275_children_first) (const char *devpath,
						struct holy_ieee1275_devalias *alias);

#define FOR_IEEE1275_DEVALIASES(alias) for (holy_ieee1275_devalias_init_iterator (&(alias)); holy_ieee1275_devalias_next (&(alias));)

#define FOR_IEEE1275_DEVCHILDREN(devpath, alias) for (holy_ieee1275_children_first ((devpath), &(alias)); \
						      (alias).name;	\
						      holy_ieee1275_children_peer (&(alias)))

#endif /* ! holy_IEEE1275_HEADER */
