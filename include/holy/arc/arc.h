/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ARC_HEADER
#define holy_ARC_HEADER	1

#include <holy/types.h>
#include <holy/symbol.h>

typedef unsigned holy_arc_enum_t;
typedef holy_uint64_t holy_arc_ularge_t;
typedef unsigned long holy_arc_ulong_t;
typedef long holy_arc_long_t;
typedef unsigned short holy_arc_ushort_t;
typedef unsigned char holy_arc_uchar_t;

typedef holy_arc_long_t holy_arc_err_t;
typedef holy_arc_ulong_t holy_arc_fileno_t;

struct holy_arc_memory_descriptor
{
  holy_arc_enum_t type;
  holy_arc_ulong_t start_page;
  holy_arc_ulong_t num_pages;
};

enum holy_arc_memory_type
  {
    holy_ARC_MEMORY_EXCEPTION_BLOCK,
    holy_ARC_MEMORY_SYSTEM_PARAMETER_BLOCK,
#ifdef holy_CPU_WORDS_BIGENDIAN
    holy_ARC_MEMORY_FREE_CONTIGUOUS,
#endif
    holy_ARC_MEMORY_FREE,
    holy_ARC_MEMORY_BADRAM,
    holy_ARC_MEMORY_LOADED,    holy_ARC_MEMORY_FW_TEMPORARY,
    holy_ARC_MEMORY_FW_PERMANENT,
#ifndef holy_CPU_WORDS_BIGENDIAN
    holy_ARC_MEMORY_FREE_CONTIGUOUS,
#endif
  } holy_arc_memory_type_t;

struct holy_arc_timeinfo
{
  holy_arc_ushort_t y;
  holy_arc_ushort_t m;
  holy_arc_ushort_t d;
  holy_arc_ushort_t h;
  holy_arc_ushort_t min;
  holy_arc_ushort_t s;
  holy_arc_ushort_t ms;
};

struct holy_arc_display_status
{
  holy_arc_ushort_t x;
  holy_arc_ushort_t y;
  holy_arc_ushort_t w;
  holy_arc_ushort_t h;
  holy_arc_uchar_t fgcolor;
  holy_arc_uchar_t bgcolor;
  holy_arc_uchar_t high_intensity;
  holy_arc_uchar_t underscored;
  holy_arc_uchar_t reverse_video;
};

enum
  {
    holy_ARC_COMPONENT_FLAG_OUT = 0x40,
    holy_ARC_COMPONENT_FLAG_IN = 0x20,
  };

struct holy_arc_component
{
  holy_arc_enum_t class;
  holy_arc_enum_t type;
  holy_arc_enum_t flags;
  holy_arc_ushort_t version;
  holy_arc_ushort_t rev;
  holy_arc_ulong_t key;
  holy_arc_ulong_t affinity;
  holy_arc_ulong_t configdatasize;
  holy_arc_ulong_t idlen;
  const char *idstr;
};

enum
  {
#ifdef holy_CPU_WORDS_BIGENDIAN
    holy_ARC_COMPONENT_TYPE_ARC = 1,
#else
    holy_ARC_COMPONENT_TYPE_ARC,
#endif
    holy_ARC_COMPONENT_TYPE_CPU,
    holy_ARC_COMPONENT_TYPE_FPU,
    holy_ARC_COMPONENT_TYPE_PRI_I_CACHE,
    holy_ARC_COMPONENT_TYPE_PRI_D_CACHE,
    holy_ARC_COMPONENT_TYPE_SEC_I_CACHE,
    holy_ARC_COMPONENT_TYPE_SEC_D_CACHE,
    holy_ARC_COMPONENT_TYPE_SEC_CACHE,
    holy_ARC_COMPONENT_TYPE_EISA,
    holy_ARC_COMPONENT_TYPE_TCA,
    holy_ARC_COMPONENT_TYPE_SCSI,
    holy_ARC_COMPONENT_TYPE_DTIA,
    holy_ARC_COMPONENT_TYPE_MULTIFUNC,
    holy_ARC_COMPONENT_TYPE_DISK_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_TAPE_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_CDROM_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_WORM_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_SERIAL_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_NET_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_DISPLAY_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_PARALLEL_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_POINTER_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_KBD_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_AUDIO_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_OTHER_CONTROLLER,
    holy_ARC_COMPONENT_TYPE_DISK,
    holy_ARC_COMPONENT_TYPE_FLOPPY,
    holy_ARC_COMPONENT_TYPE_TAPE,
    holy_ARC_COMPONENT_TYPE_MODEM,
    holy_ARC_COMPONENT_TYPE_MONITOR,
    holy_ARC_COMPONENT_TYPE_PRINTER,
    holy_ARC_COMPONENT_TYPE_POINTER,
    holy_ARC_COMPONENT_TYPE_KBD,
    holy_ARC_COMPONENT_TYPE_TERMINAL,
#ifndef holy_CPU_WORDS_BIGENDIAN
    holy_ARC_COMPONENT_TYPE_OTHER_PERIPHERAL,
#endif
    holy_ARC_COMPONENT_TYPE_LINE,
    holy_ARC_COMPONENT_TYPE_NET,
    holy_ARC_COMPONENT_TYPE_MEMORY_UNIT,
  };

enum holy_arc_file_access
  {
    holy_ARC_FILE_ACCESS_OPEN_RO,
    holy_ARC_FILE_ACCESS_OPEN_WO,
    holy_ARC_FILE_ACCESS_OPEN_RW,
  };

struct holy_arc_fileinfo
{
  holy_arc_ularge_t start;
  holy_arc_ularge_t end;
  holy_arc_ularge_t current;
  holy_arc_enum_t type;
  holy_arc_ulong_t fnamelength;
  holy_arc_uchar_t attr;
  char filename[32];
};

struct holy_arc_firmware_vector
{
  /* 0x00. */
  void *load;
  void *invoke;
  void *execute;
  void *halt;

  /* 0x10. */
  void (*powerdown) (void);
  void (*restart) (void);
  void (*reboot) (void);
  void (*exit) (void);

  /* 0x20. */
  void *reserved1;
  const struct holy_arc_component * (*getpeer) (const struct holy_arc_component *comp);
  const struct holy_arc_component * (*getchild) (const struct holy_arc_component *comp);
  void *getparent;
  
  /* 0x30. */
  void *getconfigurationdata;
  void *addchild;
  void *deletecomponent;
  void *getcomponent;

  /* 0x40. */
  void *saveconfiguration;
  void *getsystemid;
  struct holy_arc_memory_descriptor *(*getmemorydescriptor) (struct holy_arc_memory_descriptor *current);
  void *reserved2;

  /* 0x50. */
  struct holy_arc_timeinfo *(*gettime) (void);
  void *getrelativetime;
  void *getdirectoryentry;
  holy_arc_err_t (*open) (const char *path, holy_arc_enum_t mode,
			  holy_arc_fileno_t *fileno);

  /* 0x60. */
  holy_arc_err_t (*close) (holy_arc_fileno_t fileno);
  holy_arc_err_t (*read) (holy_arc_fileno_t fileno, void *buf,
			  holy_arc_ulong_t n,
			  holy_arc_ulong_t *count);
  holy_arc_err_t (*get_read_status) (holy_arc_fileno_t fileno);
  holy_arc_err_t (*write) (holy_arc_fileno_t fileno, const void *buf,
			   holy_arc_ulong_t n,
			   holy_arc_ulong_t *count);

  /* 0x70. */
  holy_arc_err_t (*seek) (holy_arc_fileno_t fileno,
			  holy_arc_ularge_t *pos, holy_arc_enum_t mode);
  void *mount;
  const char * (*getenvironmentvariable) (const char *name);
  void *setenvironmentvariable;

  /* 0x80. */
  holy_arc_err_t (*getfileinformation) (holy_arc_fileno_t fileno,
					struct holy_arc_fileinfo *info);
  void *setfileinformation;
  void *flushallcaches;
  void *testunicodecharacter;
  
  /* 0x90. */
  struct holy_arc_display_status * (*getdisplaystatus) (holy_arc_fileno_t fileno);
};

struct holy_arc_adapter
{
  holy_arc_ulong_t adapter_type;
  holy_arc_ulong_t adapter_vector_length;
  void *adapter_vector;
};

struct holy_arc_system_parameter_block
{
  holy_arc_ulong_t signature;
  holy_arc_ulong_t length;
  holy_arc_ushort_t version;
  holy_arc_ushort_t revision;
  void *restartblock;
  void *debugblock;
  void *gevector;
  void *utlbmissvector;
  holy_arc_ulong_t firmware_vector_length;
  struct holy_arc_firmware_vector *firmwarevector;
  holy_arc_ulong_t private_vector_length;
  void *private_vector;
  holy_arc_ulong_t adapter_count;
  struct holy_arc_adapter adapters[0];
};


#define holy_ARC_SYSTEM_PARAMETER_BLOCK ((struct holy_arc_system_parameter_block *) 0xa0001000)
#define holy_ARC_FIRMWARE_VECTOR (holy_ARC_SYSTEM_PARAMETER_BLOCK->firmwarevector)
#define holy_ARC_STDIN 0
#define holy_ARC_STDOUT 1

typedef int (*holy_arc_iterate_devs_hook_t)
  (const char *name, const struct holy_arc_component *comp, void *data);

int EXPORT_FUNC (holy_arc_iterate_devs) (holy_arc_iterate_devs_hook_t hook,
					 void *hook_data,
					 int alt_names);

char *EXPORT_FUNC (holy_arc_alt_name_to_norm) (const char *name, const char *suffix);

int EXPORT_FUNC (holy_arc_is_device_serial) (const char *name, int alt_names);


#define FOR_ARC_CHILDREN(comp, parent) for (comp = holy_ARC_FIRMWARE_VECTOR->getchild (parent); comp; comp = holy_ARC_FIRMWARE_VECTOR->getpeer (comp))

extern void holy_arcdisk_init (void);
extern void holy_arcdisk_fini (void);


#endif
