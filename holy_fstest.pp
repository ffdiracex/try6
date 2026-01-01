# 0 "util/holy-fstest.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "util/holy-fstest.c"





# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 7 "util/holy-fstest.c" 2
# 1 "./include/holy/types.h" 1
# 9 "./include/holy/types.h"
# 1 "./include/holy/emu/config.h" 1
# 9 "./include/holy/emu/config.h"
# 1 "./include/holy/disk.h" 1
# 9 "./include/holy/disk.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/disk.h" 2

# 1 "./include/holy/symbol.h" 1
# 9 "./include/holy/symbol.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/symbol.h" 2
# 12 "./include/holy/disk.h" 2
# 1 "./include/holy/err.h" 1
# 13 "./include/holy/err.h"
typedef enum
  {
    holy_ERR_NONE = 0,
    holy_ERR_TEST_FAILURE,
    holy_ERR_BAD_MODULE,
    holy_ERR_OUT_OF_MEMORY,
    holy_ERR_BAD_FILE_TYPE,
    holy_ERR_FILE_NOT_FOUND,
    holy_ERR_FILE_READ_ERROR,
    holy_ERR_BAD_FILENAME,
    holy_ERR_UNKNOWN_FS,
    holy_ERR_BAD_FS,
    holy_ERR_BAD_NUMBER,
    holy_ERR_OUT_OF_RANGE,
    holy_ERR_UNKNOWN_DEVICE,
    holy_ERR_BAD_DEVICE,
    holy_ERR_READ_ERROR,
    holy_ERR_WRITE_ERROR,
    holy_ERR_UNKNOWN_COMMAND,
    holy_ERR_INVALID_COMMAND,
    holy_ERR_BAD_ARGUMENT,
    holy_ERR_BAD_PART_TABLE,
    holy_ERR_UNKNOWN_OS,
    holy_ERR_BAD_OS,
    holy_ERR_NO_KERNEL,
    holy_ERR_BAD_FONT,
    holy_ERR_NOT_IMPLEMENTED_YET,
    holy_ERR_SYMLINK_LOOP,
    holy_ERR_BAD_COMPRESSED_DATA,
    holy_ERR_MENU,
    holy_ERR_TIMEOUT,
    holy_ERR_IO,
    holy_ERR_ACCESS_DENIED,
    holy_ERR_EXTRACTOR,
    holy_ERR_NET_BAD_ADDRESS,
    holy_ERR_NET_ROUTE_LOOP,
    holy_ERR_NET_NO_ROUTE,
    holy_ERR_NET_NO_ANSWER,
    holy_ERR_NET_NO_CARD,
    holy_ERR_WAIT,
    holy_ERR_BUG,
    holy_ERR_NET_PORT_CLOSED,
    holy_ERR_NET_INVALID_RESPONSE,
    holy_ERR_NET_UNKNOWN_ERROR,
    holy_ERR_NET_PACKET_TOO_BIG,
    holy_ERR_NET_NO_DOMAIN,
    holy_ERR_EOF,
    holy_ERR_BAD_SIGNATURE
  }
holy_err_t;

struct holy_error_saved
{
  holy_err_t holy_errno;
  char errmsg[256];
};

extern holy_err_t holy_errno;
extern char holy_errmsg[256];

holy_err_t holy_error (holy_err_t n, const char *fmt, ...);
void holy_fatal (const char *fmt, ...) __attribute__ ((noreturn));
void holy_error_push (void);
int holy_error_pop (void);
void holy_print_error (void);
extern int holy_err_printed_errors;
int holy_err_printf (const char *fmt, ...)
     __attribute__ ((format (__printf__, 1, 2)));
# 13 "./include/holy/disk.h" 2
# 1 "./include/holy/types.h" 1
# 14 "./include/holy/disk.h" 2
# 1 "./include/holy/device.h" 1
# 12 "./include/holy/device.h"
struct holy_disk;
struct holy_net;

struct holy_device
{
  struct holy_disk *disk;
  struct holy_net *net;
};
typedef struct holy_device *holy_device_t;

typedef int (*holy_device_iterate_hook_t) (const char *name, void *data);

holy_device_t holy_device_open (const char *name);
holy_err_t holy_device_close (holy_device_t device);
int holy_device_iterate (holy_device_iterate_hook_t hook,
          void *hook_data);
# 15 "./include/holy/disk.h" 2

# 1 "./include/holy/mm.h" 1
# 11 "./include/holy/mm.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 12 "./include/holy/mm.h" 2





typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long long int holy_int64_t;
typedef holy_int64_t holy_ssize;

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


void holy_mm_init_region (void *addr, holy_size_t size);
void *holy_malloc (holy_size_t size);
void *holy_zalloc (holy_size_t size);
void holy_free (void *ptr);
void *holy_realloc (void *ptr, holy_size_t size);

void *holy_memalign (holy_size_t align, holy_size_t size);


void holy_mm_check_real (const char *file, int line);
# 17 "./include/holy/disk.h" 2



enum holy_disk_dev_id
  {
    holy_DISK_DEVICE_BIOSDISK_ID,
    holy_DISK_DEVICE_OFDISK_ID,
    holy_DISK_DEVICE_LOOPBACK_ID,
    holy_DISK_DEVICE_EFIDISK_ID,
    holy_DISK_DEVICE_DISKFILTER_ID,
    holy_DISK_DEVICE_HOST_ID,
    holy_DISK_DEVICE_ATA_ID,
    holy_DISK_DEVICE_MEMDISK_ID,
    holy_DISK_DEVICE_NAND_ID,
    holy_DISK_DEVICE_SCSI_ID,
    holy_DISK_DEVICE_CRYPTODISK_ID,
    holy_DISK_DEVICE_ARCDISK_ID,
    holy_DISK_DEVICE_HOSTDISK_ID,
    holy_DISK_DEVICE_PROCFS_ID,
    holy_DISK_DEVICE_CBFSDISK_ID,
    holy_DISK_DEVICE_UBOOTDISK_ID,
    holy_DISK_DEVICE_XEN,
  };
# 95 "./include/holy/disk.h"
struct holy_disk;

struct holy_disk_memberlist;


typedef enum
  {
    holy_DISK_PULL_NONE,
    holy_DISK_PULL_REMOVABLE,
    holy_DISK_PULL_RESCAN,
    holy_DISK_PULL_MAX
  } holy_disk_pull_t;

typedef int (*holy_disk_dev_iterate_hook_t) (const char *name, void *data);


struct holy_disk_dev
{

  const char *name;


  enum holy_disk_dev_id id;


  int (*iterate) (holy_disk_dev_iterate_hook_t hook, void *hook_data,
    holy_disk_pull_t pull);


  holy_err_t (*open) (const char *name, struct holy_disk *disk);


  void (*close) (struct holy_disk *disk);


  holy_err_t (*read) (struct holy_disk *disk, holy_disk_addr_t sector,
        holy_size_t size, char *buf);


  holy_err_t (*write) (struct holy_disk *disk, holy_disk_addr_t sector,
         holy_size_t size, const char *buf);


  struct holy_disk_memberlist *(*memberlist) (struct holy_disk *disk);
  const char * (*raidname) (struct holy_disk *disk);



  struct holy_disk_dev *next;
};
typedef struct holy_disk_dev *holy_disk_dev_t;

extern holy_disk_dev_t holy_disk_dev_list;

struct holy_partition;

typedef long int holy_disk_addr_t;

typedef void (*holy_disk_read_hook_t) (holy_disk_addr_t sector,
           unsigned offset, unsigned length,
           void *data);


struct holy_disk
{

  const char *name;


  holy_disk_dev_t dev;


  holy_uint64_t total_sectors;


  unsigned int log_sector_size;


  unsigned int max_agglomerate;


  unsigned long id;


  struct holy_partition *partition;



  holy_disk_read_hook_t read_hook;


  void *read_hook_data;


  void *data;
};
typedef struct holy_disk *holy_disk_t;


struct holy_disk_memberlist
{
  holy_disk_t disk;
  struct holy_disk_memberlist *next;
};
typedef struct holy_disk_memberlist *holy_disk_memberlist_t;
# 220 "./include/holy/disk.h"
void holy_disk_cache_invalidate_all (void);

void holy_disk_dev_register (holy_disk_dev_t dev);
void holy_disk_dev_unregister (holy_disk_dev_t dev);
static inline int
holy_disk_dev_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data)
{
  holy_disk_dev_t p;
  holy_disk_pull_t pull;

  for (pull = 0; pull < holy_DISK_PULL_MAX; pull++)
    for (p = holy_disk_dev_list; p; p = p->next)
      if (p->iterate && (p->iterate) (hook, hook_data, pull))
 return 1;

  return 0;
}

holy_disk_t holy_disk_open (const char *name);
void holy_disk_close (holy_disk_t disk);
holy_err_t holy_disk_read (holy_disk_t disk,
     holy_disk_addr_t sector,
     holy_off_t offset,
     holy_size_t size,
     void *buf);
holy_err_t holy_disk_write (holy_disk_t disk,
       holy_disk_addr_t sector,
       holy_off_t offset,
       holy_size_t size,
       const void *buf);
extern holy_err_t (*holy_disk_write_weak) (holy_disk_t disk,
             holy_disk_addr_t sector,
             holy_off_t offset,
             holy_size_t size,
             const void *buf);


holy_uint64_t holy_disk_get_size (holy_disk_t disk);






extern void (* holy_disk_firmware_fini) (void);
extern int holy_disk_firmware_is_tainted;

static inline void
holy_stop_disk_firmware (void)
{

  holy_disk_firmware_is_tainted = 1;
  if (holy_disk_firmware_fini)
    {
      holy_disk_firmware_fini ();
      holy_disk_firmware_fini = ((void *) 0);
    }
}


struct holy_disk_cache
{
  enum holy_disk_dev_id dev_id;
  unsigned long disk_id;
  holy_disk_addr_t sector;
  char *data;
  int lock;
};

extern struct holy_disk_cache holy_disk_cache_table[1021];


void holy_lvm_init (void);
void holy_ldm_init (void);
void holy_mdraid09_init (void);
void holy_mdraid1x_init (void);
void holy_diskfilter_init (void);
void holy_lvm_fini (void);
void holy_ldm_fini (void);
void holy_mdraid09_fini (void);
void holy_mdraid1x_fini (void);
void holy_diskfilter_fini (void);
# 10 "./include/holy/emu/config.h" 2
# 1 "./include/holy/partition.h" 1
# 9 "./include/holy/partition.h"
# 1 "./include/holy/dl.h" 1
# 13 "./include/holy/dl.h"
# 1 "./include/holy/elf.h" 1
# 13 "./include/holy/elf.h"
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;

typedef holy_int64_t holy_ssize_t;





typedef holy_uint16_t Elf32_Half;
typedef holy_uint16_t Elf64_Half;


typedef holy_uint32_t Elf32_Word;
typedef holy_int32_t Elf32_Sword;
typedef holy_uint32_t Elf64_Word;
typedef holy_int32_t Elf64_Sword;


typedef holy_uint64_t Elf32_Xword;
typedef holy_int64_t Elf32_Sxword;
typedef holy_uint64_t Elf64_Xword;
typedef holy_int64_t Elf64_Sxword;


typedef holy_uint32_t Elf32_Addr;
typedef holy_uint64_t Elf64_Addr;


typedef holy_uint32_t Elf32_Off;
typedef holy_uint64_t Elf64_Off;


typedef holy_uint16_t Elf32_Section;
typedef holy_uint16_t Elf64_Section;


typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;






typedef struct
{
  unsigned char e_ident[(16)];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
  unsigned char e_ident[(16)];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;
# 268 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct
{
  Elf64_Word sh_name;
  Elf64_Word sh_type;
  Elf64_Xword sh_flags;
  Elf64_Addr sh_addr;
  Elf64_Off sh_offset;
  Elf64_Xword sh_size;
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Xword sh_addralign;
  Elf64_Xword sh_entsize;
} Elf64_Shdr;
# 367 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Section st_shndx;
} Elf32_Sym;

typedef struct
{
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Section st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
} Elf64_Sym;




typedef struct
{
  Elf32_Half si_boundto;
  Elf32_Half si_flags;
} Elf32_Syminfo;

typedef struct
{
  Elf64_Half si_boundto;
  Elf64_Half si_flags;
} Elf64_Syminfo;
# 481 "./include/holy/elf.h"
typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
} Elf32_Rel;






typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
} Elf64_Rel;



typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
  Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
  Elf64_Sxword r_addend;
} Elf64_Rela;
# 526 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;

typedef struct
{
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;
# 605 "./include/holy/elf.h"
typedef struct
{
  Elf32_Sword d_tag;
  union
    {
      Elf32_Word d_val;
      Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct
{
  Elf64_Sxword d_tag;
  union
    {
      Elf64_Xword d_val;
      Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;
# 769 "./include/holy/elf.h"
typedef struct
{
  Elf32_Half vd_version;
  Elf32_Half vd_flags;
  Elf32_Half vd_ndx;
  Elf32_Half vd_cnt;
  Elf32_Word vd_hash;
  Elf32_Word vd_aux;
  Elf32_Word vd_next;

} Elf32_Verdef;

typedef struct
{
  Elf64_Half vd_version;
  Elf64_Half vd_flags;
  Elf64_Half vd_ndx;
  Elf64_Half vd_cnt;
  Elf64_Word vd_hash;
  Elf64_Word vd_aux;
  Elf64_Word vd_next;

} Elf64_Verdef;
# 811 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word vda_name;
  Elf32_Word vda_next;

} Elf32_Verdaux;

typedef struct
{
  Elf64_Word vda_name;
  Elf64_Word vda_next;

} Elf64_Verdaux;




typedef struct
{
  Elf32_Half vn_version;
  Elf32_Half vn_cnt;
  Elf32_Word vn_file;

  Elf32_Word vn_aux;
  Elf32_Word vn_next;

} Elf32_Verneed;

typedef struct
{
  Elf64_Half vn_version;
  Elf64_Half vn_cnt;
  Elf64_Word vn_file;

  Elf64_Word vn_aux;
  Elf64_Word vn_next;

} Elf64_Verneed;
# 858 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word vna_hash;
  Elf32_Half vna_flags;
  Elf32_Half vna_other;
  Elf32_Word vna_name;
  Elf32_Word vna_next;

} Elf32_Vernaux;

typedef struct
{
  Elf64_Word vna_hash;
  Elf64_Half vna_flags;
  Elf64_Half vna_other;
  Elf64_Word vna_name;
  Elf64_Word vna_next;

} Elf64_Vernaux;
# 892 "./include/holy/elf.h"
typedef struct
{
  int a_type;
  union
    {
      long int a_val;
      void *a_ptr;
      void (*a_fcn) (void);
    } a_un;
} Elf32_auxv_t;

typedef struct
{
  long int a_type;
  union
    {
      long int a_val;
      void *a_ptr;
      void (*a_fcn) (void);
    } a_un;
} Elf64_auxv_t;
# 955 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
} Elf32_Nhdr;

typedef struct
{
  Elf64_Word n_namesz;
  Elf64_Word n_descsz;
  Elf64_Word n_type;
} Elf64_Nhdr;
# 1002 "./include/holy/elf.h"
typedef struct
{
  Elf32_Xword m_value;
  Elf32_Word m_info;
  Elf32_Word m_poffset;
  Elf32_Half m_repeat;
  Elf32_Half m_stride;
} Elf32_Move;

typedef struct
{
  Elf64_Xword m_value;
  Elf64_Xword m_info;
  Elf64_Xword m_poffset;
  Elf64_Half m_repeat;
  Elf64_Half m_stride;
} Elf64_Move;
# 1367 "./include/holy/elf.h"
typedef union
{
  struct
    {
      Elf32_Word gt_current_g_value;
      Elf32_Word gt_unused;
    } gt_header;
  struct
    {
      Elf32_Word gt_g_value;
      Elf32_Word gt_bytes;
    } gt_entry;
} Elf32_gptab;



typedef struct
{
  Elf32_Word ri_gprmask;
  Elf32_Word ri_cprmask[4];
  Elf32_Sword ri_gp_value;
} Elf32_RegInfo;



typedef struct
{
  unsigned char kind;

  unsigned char size;
  Elf32_Section section;

  Elf32_Word info;
} Elf_Options;
# 1443 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word hwp_flags1;
  Elf32_Word hwp_flags2;
} Elf_Options_Hw;
# 1582 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word l_name;
  Elf32_Word l_time_stamp;
  Elf32_Word l_checksum;
  Elf32_Word l_version;
  Elf32_Word l_flags;
} Elf32_Lib;

typedef struct
{
  Elf64_Word l_name;
  Elf64_Word l_time_stamp;
  Elf64_Word l_checksum;
  Elf64_Word l_version;
  Elf64_Word l_flags;
} Elf64_Lib;
# 1613 "./include/holy/elf.h"
typedef Elf32_Addr Elf32_Conflict;
# 14 "./include/holy/dl.h" 2
# 1 "./include/holy/list.h" 1
# 11 "./include/holy/list.h"
# 1 "./include/holy/compiler.h" 1
# 12 "./include/holy/list.h" 2

struct holy_list
{
  struct holy_list *next;
  struct holy_list **prev;
};
typedef struct holy_list *holy_list_t;

void holy_list_push (holy_list_t *head, holy_list_t item);
void holy_list_remove (holy_list_t item);




static inline void *
holy_bad_type_cast_real (int line, const char *file)
     __attribute__ ((__error__ ("bad type cast between incompatible holy types")));

static inline void *
holy_bad_type_cast_real (int line, const char *file)
{
  holy_fatal ("error:%s:%u: bad type cast between incompatible holy types",
       file, line);
}
# 50 "./include/holy/list.h"
struct holy_named_list
{
  struct holy_named_list *next;
  struct holy_named_list **prev;
  char *name;
};
typedef struct holy_named_list *holy_named_list_t;

void * holy_named_list_find (holy_named_list_t head,
       const char *name);
# 15 "./include/holy/dl.h" 2
# 1 "./include/holy/misc.h" 1
# 9 "./include/holy/misc.h"
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 1 3 4
# 40 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4

# 40 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 103 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
# 10 "./include/holy/misc.h" 2



# 1 "./include/holy/i18n.h" 1
# 9 "./include/holy/i18n.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/i18n.h" 2






# 15 "./include/holy/i18n.h"
extern const char *(*holy_gettext) (const char *s) __attribute__ ((format_arg (1)));



# 1 "/usr/include/locale.h" 1 3 4
# 25 "/usr/include/locale.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 415 "/usr/include/features.h" 3 4
# 1 "/usr/include/features-time64.h" 1 3 4
# 20 "/usr/include/features-time64.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 21 "/usr/include/features-time64.h" 2 3 4
# 1 "/usr/include/bits/timesize.h" 1 3 4
# 19 "/usr/include/bits/timesize.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 20 "/usr/include/bits/timesize.h" 2 3 4
# 22 "/usr/include/features-time64.h" 2 3 4
# 416 "/usr/include/features.h" 2 3 4
# 524 "/usr/include/features.h" 3 4
# 1 "/usr/include/sys/cdefs.h" 1 3 4
# 730 "/usr/include/sys/cdefs.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 731 "/usr/include/sys/cdefs.h" 2 3 4
# 1 "/usr/include/bits/long-double.h" 1 3 4
# 732 "/usr/include/sys/cdefs.h" 2 3 4
# 525 "/usr/include/features.h" 2 3 4
# 548 "/usr/include/features.h" 3 4
# 1 "/usr/include/gnu/stubs.h" 1 3 4
# 10 "/usr/include/gnu/stubs.h" 3 4
# 1 "/usr/include/gnu/stubs-64.h" 1 3 4
# 11 "/usr/include/gnu/stubs.h" 2 3 4
# 549 "/usr/include/features.h" 2 3 4
# 26 "/usr/include/locale.h" 2 3 4


# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 29 "/usr/include/locale.h" 2 3 4
# 1 "/usr/include/bits/locale.h" 1 3 4
# 30 "/usr/include/locale.h" 2 3 4


# 51 "/usr/include/locale.h" 3 4

# 51 "/usr/include/locale.h" 3 4
struct lconv
{


  char *decimal_point;
  char *thousands_sep;





  char *grouping;





  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char int_frac_digits;
  char frac_digits;

  char p_cs_precedes;

  char p_sep_by_space;

  char n_cs_precedes;

  char n_sep_by_space;






  char p_sign_posn;
  char n_sign_posn;


  char int_p_cs_precedes;

  char int_p_sep_by_space;

  char int_n_cs_precedes;

  char int_n_sep_by_space;






  char int_p_sign_posn;
  char int_n_sign_posn;
# 118 "/usr/include/locale.h" 3 4
};



extern char *setlocale (int __category, const char *__locale) __attribute__ ((__nothrow__ , __leaf__));


extern struct lconv *localeconv (void) __attribute__ ((__nothrow__ , __leaf__));
# 135 "/usr/include/locale.h" 3 4
# 1 "/usr/include/bits/types/locale_t.h" 1 3 4
# 22 "/usr/include/bits/types/locale_t.h" 3 4
# 1 "/usr/include/bits/types/__locale_t.h" 1 3 4
# 27 "/usr/include/bits/types/__locale_t.h" 3 4
struct __locale_struct
{

  struct __locale_data *__locales[13];


  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;


  const char *__names[13];
};

typedef struct __locale_struct *__locale_t;
# 23 "/usr/include/bits/types/locale_t.h" 2 3 4

typedef __locale_t locale_t;
# 136 "/usr/include/locale.h" 2 3 4





extern locale_t newlocale (int __category_mask, const char *__locale,
      locale_t __base) __attribute__ ((__nothrow__ , __leaf__));
# 176 "/usr/include/locale.h" 3 4
extern locale_t duplocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));



extern void freelocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));






extern locale_t uselocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));








# 20 "./include/holy/i18n.h" 2
# 1 "/usr/include/libintl.h" 1 3 4
# 34 "/usr/include/libintl.h" 3 4





extern char *gettext (const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (1)));



extern char *dgettext (const char *__domainname, const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));
extern char *__dgettext (const char *__domainname, const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));



extern char *dcgettext (const char *__domainname,
   const char *__msgid, int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));
extern char *__dcgettext (const char *__domainname,
     const char *__msgid, int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));




extern char *ngettext (const char *__msgid1, const char *__msgid2,
         unsigned long int __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (1))) __attribute__ ((__format_arg__ (2)));



extern char *dngettext (const char *__domainname, const char *__msgid1,
   const char *__msgid2, unsigned long int __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2))) __attribute__ ((__format_arg__ (3)));



extern char *dcngettext (const char *__domainname, const char *__msgid1,
    const char *__msgid2, unsigned long int __n,
    int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2))) __attribute__ ((__format_arg__ (3)));





extern char *textdomain (const char *__domainname) __attribute__ ((__nothrow__ , __leaf__));



extern char *bindtextdomain (const char *__domainname,
        const char *__dirname) __attribute__ ((__nothrow__ , __leaf__));



extern char *bind_textdomain_codeset (const char *__domainname,
          const char *__codeset) __attribute__ ((__nothrow__ , __leaf__));
# 121 "/usr/include/libintl.h" 3 4

# 21 "./include/holy/i18n.h" 2
# 40 "./include/holy/i18n.h"

# 40 "./include/holy/i18n.h"
static inline const char * __attribute__ ((always_inline,format_arg (1)))
_ (const char *str)
{
  return gettext(str);
}
# 14 "./include/holy/misc.h" 2
# 32 "./include/holy/misc.h"
typedef unsigned long long int holy_size_t;
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;
typedef holy_int64_t holy_ssize_t;


void *holy_memmove (void *dest, const void *src, holy_size_t n);
char *holy_strcpy (char *dest, const char *src);

static inline char *
holy_strncpy (char *dest, const char *src, int c)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0' && --c)
    ;

  return dest;
}

static inline char *
holy_stpcpy (char *dest, const char *src)
{
  char *d = dest;
  const char *s = src;

  do
    *d++ = *s;
  while (*s++ != '\0');

  return d - 1;
}


static inline void *
holy_memcpy (void *dest, const void *src, holy_size_t n)
{
  return holy_memmove (dest, src, n);
}
# 87 "./include/holy/misc.h"
int holy_memcmp (const void *s1, const void *s2, holy_size_t n);
int holy_strcmp (const char *s1, const char *s2);
int holy_strncmp (const char *s1, const char *s2, holy_size_t n);

char *holy_strchr (const char *s, int c);
char *holy_strrchr (const char *s, int c);
int holy_strword (const char *s, const char *w);



static inline char *
holy_strstr (const char *haystack, const char *needle)
{





  if (*needle != '\0')
    {


      char b = *needle++;

      for (;; haystack++)
 {
   if (*haystack == '\0')

     return 0;
   if (*haystack == b)

     {
       const char *rhaystack = haystack + 1;
       const char *rneedle = needle;

       for (;; rhaystack++, rneedle++)
  {
    if (*rneedle == '\0')

      return (char *) haystack;
    if (*rhaystack == '\0')

      return 0;
    if (*rhaystack != *rneedle)

      break;
  }
     }
 }
    }
  else
    return (char *) haystack;
}

int holy_isspace (int c);

static inline int
holy_isprint (int c)
{
  return (c >= ' ' && c <= '~');
}

static inline int
holy_iscntrl (int c)
{
  return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

static inline int
holy_isalpha (int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int
holy_islower (int c)
{
  return (c >= 'a' && c <= 'z');
}

static inline int
holy_isupper (int c)
{
  return (c >= 'A' && c <= 'Z');
}

static inline int
holy_isgraph (int c)
{
  return (c >= '!' && c <= '~');
}

static inline int
holy_isdigit (int c)
{
  return (c >= '0' && c <= '9');
}

static inline int
holy_isxdigit (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int
holy_isalnum (int c)
{
  return holy_isalpha (c) || holy_isdigit (c);
}

static inline int
holy_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';

  return c;
}

static inline int
holy_toupper (int c)
{
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 'A';

  return c;
}

static inline int
holy_strcasecmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (holy_tolower ((holy_uint8_t) *s1)
   != holy_tolower ((holy_uint8_t) *s2))
 break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

static inline int
holy_strncasecmp (const char *s1, const char *s2, holy_size_t n)
{
  if (n == 0)
    return 0;

  while (*s1 && *s2 && --n)
    {
      if (holy_tolower (*s1) != holy_tolower (*s2))
 break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

unsigned long holy_strtoul (const char *str, char **end, int base);
unsigned long long holy_strtoull (const char *str, char **end, int base);

static inline long
holy_strtol (const char *str, char **end, int base)
{
  int negative = 0;
  unsigned long long magnitude;

  while (*str && holy_isspace (*str))
    str++;

  if (*str == '-')
    {
      negative = 1;
      str++;
    }

  magnitude = holy_strtoull (str, end, base);
  if (negative)
    {
      if (magnitude > (unsigned long) (9223372036854775807L) + 1)
        {
          holy_error (holy_ERR_OUT_OF_RANGE, "overflow is detected");
          return (-9223372036854775807L - 1);
        }
      return -((long) magnitude);
    }
  else
    {
      if (magnitude > (9223372036854775807L))
        {
          holy_error (holy_ERR_OUT_OF_RANGE, "overflow is detected");
          return (9223372036854775807L);
        }
      return (long) magnitude;
    }
}

char *holy_strdup (const char *s) __attribute__ ((warn_unused_result));
char *holy_strndup (const char *s, holy_size_t n) __attribute__ ((warn_unused_result));
void *holy_memset (void *s, int c, holy_size_t n);
holy_size_t holy_strlen (const char *s) __attribute__ ((warn_unused_result));
int holy_printf (const char *fmt, ...) __attribute__ ((format (gnu_printf, 1, 2)));
int holy_printf_ (const char *fmt, ...) __attribute__ ((format (gnu_printf, 1, 2)));



static inline char *
holy_strchrsub (char *output, const char *input, char ch, const char *with)
{
  while (*input)
    {
      if (*input == ch)
 {
   holy_strcpy (output, with);
   output += holy_strlen (with);
   input++;
   continue;
 }
      *output++ = *input++;
    }
  *output = '\0';
  return output;
}

extern void (*holy_xputs) (const char *str);

static inline int
holy_puts (const char *s)
{
  const char nl[2] = "\n";
  holy_xputs (s);
  holy_xputs (nl);

  return 1;
}

int holy_puts_ (const char *s);
void holy_real_dprintf (const char *file,
                                     const int line,
                                     const char *condition,
                                     const char *fmt, ...) __attribute__ ((format (gnu_printf, 4, 5)));
int holy_vprintf (const char *fmt, va_list args);
int holy_snprintf (char *str, holy_size_t n, const char *fmt, ...)
     __attribute__ ((format (gnu_printf, 3, 4)));
int holy_vsnprintf (char *str, holy_size_t n, const char *fmt,
     va_list args);
char *holy_xasprintf (const char *fmt, ...)
     __attribute__ ((format (gnu_printf, 1, 2))) __attribute__ ((warn_unused_result));
char *holy_xvasprintf (const char *fmt, va_list args) __attribute__ ((warn_unused_result));
void holy_exit (void) __attribute__ ((noreturn));
holy_uint64_t holy_divmod64 (holy_uint64_t n,
       holy_uint64_t d,
       holy_uint64_t *r);
# 363 "./include/holy/misc.h"
holy_int64_t
holy_divmod64s (holy_int64_t n,
     holy_int64_t d,
     holy_int64_t *r);

holy_uint32_t
holy_divmod32 (holy_uint32_t n,
     holy_uint32_t d,
     holy_uint32_t *r);

holy_int32_t
holy_divmod32s (holy_int32_t n,
      holy_int32_t d,
      holy_int32_t *r);



static inline char *
holy_memchr (const void *p, int c, holy_size_t len)
{
  const char *s = (const char *) p;
  const char *e = s + len;

  for (; s < e; s++)
    if (*s == c)
      return (char *) s;

  return 0;
}


static inline unsigned int
holy_abs (int x)
{
  if (x < 0)
    return (unsigned int) (-x);
  else
    return (unsigned int) x;
}





void holy_reboot (void) __attribute__ ((noreturn));
# 421 "./include/holy/misc.h"
void holy_halt (void) __attribute__ ((noreturn));
# 431 "./include/holy/misc.h"
static inline void
holy_error_save (struct holy_error_saved *save)
{
  holy_memcpy (save->errmsg, holy_errmsg, sizeof (save->errmsg));
  save->holy_errno = holy_errno;
  holy_errno = holy_ERR_NONE;
}

static inline void
holy_error_load (const struct holy_error_saved *save)
{
  holy_memcpy (holy_errmsg, save->errmsg, sizeof (holy_errmsg));
  holy_errno = save->holy_errno;
}
# 16 "./include/holy/dl.h" 2
# 141 "./include/holy/dl.h"
struct holy_dl_segment
{
  struct holy_dl_segment *next;
  void *addr;
  holy_size_t size;
  unsigned section;
};
typedef struct holy_dl_segment *holy_dl_segment_t;

struct holy_dl;

struct holy_dl_dep
{
  struct holy_dl_dep *next;
  struct holy_dl *mod;
};
typedef struct holy_dl_dep *holy_dl_dep_t;
# 184 "./include/holy/dl.h"
typedef struct holy_dl *holy_dl_t;

holy_dl_t holy_dl_load_file (const char *filename);
holy_dl_t holy_dl_load (const char *name);
holy_dl_t holy_dl_load_core (void *addr, holy_size_t size);
holy_dl_t holy_dl_load_core_noinit (void *addr, holy_size_t size);
int holy_dl_unload (holy_dl_t mod);
void holy_dl_unload_unneeded (void);
int holy_dl_ref (holy_dl_t mod);
int holy_dl_unref (holy_dl_t mod);
extern holy_dl_t holy_dl_head;
# 231 "./include/holy/dl.h"
holy_err_t holy_dl_register_symbol (const char *name, void *addr,
        int isfunc, holy_dl_t mod);

holy_err_t holy_arch_dl_check_header (void *ehdr);
# 249 "./include/holy/dl.h"
holy_err_t
holy_ia64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
     holy_size_t *got);
holy_err_t
holy_arm64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
      holy_size_t *got);
# 263 "./include/holy/dl.h"
holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
     holy_size_t *got);
# 10 "./include/holy/partition.h" 2



typedef unsigned long long int holy_uint64_t;
# 68 "./include/holy/partition.h"
struct holy_disk;

typedef struct holy_partition *holy_partition_t;


typedef enum
{
  holy_EMBED_PCBIOS
} holy_embed_type_t;


typedef int (*holy_partition_iterate_hook_t) (struct holy_disk *disk,
           const holy_partition_t partition,
           void *data);


struct holy_partition_map
{

  struct holy_partition_map *next;
  struct holy_partition_map **prev;


  const char *name;


  holy_err_t (*iterate) (struct holy_disk *disk,
    holy_partition_iterate_hook_t hook, void *hook_data);


  holy_err_t (*embed) (struct holy_disk *disk, unsigned int *nsectors,
         unsigned int max_nsectors,
         holy_embed_type_t embed_type,
         holy_disk_addr_t **sectors);

};
typedef struct holy_partition_map *holy_partition_map_t;


struct holy_partition
{

  int number;


  unsigned long long int start;


  unsigned long long int len;


  unsigned long long int offset;


  int index;


  struct holy_partition *parent;


  holy_partition_map_t partmap;



  unsigned char msdostype;
};

holy_partition_t holy_partition_probe (struct holy_disk *disk,
          const char *str);
int holy_partition_iterate (struct holy_disk *disk,
      holy_partition_iterate_hook_t hook,
      void *hook_data);
char *holy_partition_get_name (const holy_partition_t partition);


extern holy_partition_map_t holy_partition_map_list;


static inline void
holy_partition_map_register (holy_partition_map_t partmap)
{
  holy_list_push ((((char *) &(*&holy_partition_map_list)->next == (char *) &((holy_list_t) (*&holy_partition_map_list))->next) && ((char *) &(*&holy_partition_map_list)->prev == (char *) &((holy_list_t) (*&holy_partition_map_list))->prev) ? (holy_list_t *) (void *) &holy_partition_map_list : (holy_list_t *) holy_bad_type_cast_real(149, "util/holy-fstest.c")),
    (((char *) &(partmap)->next == (char *) &((holy_list_t) (partmap))->next) && ((char *) &(partmap)->prev == (char *) &((holy_list_t) (partmap))->prev) ? (holy_list_t) partmap : (holy_list_t) holy_bad_type_cast_real(150, "util/holy-fstest.c")));
}


static inline void
holy_partition_map_unregister (holy_partition_map_t partmap)
{
  holy_list_remove ((((char *) &(partmap)->next == (char *) &((holy_list_t) (partmap))->next) && ((char *) &(partmap)->prev == (char *) &((holy_list_t) (partmap))->prev) ? (holy_list_t) partmap : (holy_list_t) holy_bad_type_cast_real(157, "util/holy-fstest.c")));
}




static inline holy_disk_addr_t
holy_partition_get_start (const holy_partition_t p)
{
  holy_partition_t part;
  holy_uint64_t part_start = 0;

  for (part = p; part; part = part->parent)
    part_start += part->start;

  return part_start;
}

static inline holy_uint64_t
holy_partition_get_len (const holy_partition_t p)
{
  return p->len;
}
# 11 "./include/holy/emu/config.h" 2
# 1 "./include/holy/emu/hostfile.h" 1
# 11 "./include/holy/emu/hostfile.h"
# 1 "/usr/include/sys/types.h" 1 3 4
# 27 "/usr/include/sys/types.h" 3 4


# 1 "/usr/include/bits/types.h" 1 3 4
# 27 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 28 "/usr/include/bits/types.h" 2 3 4
# 1 "/usr/include/bits/timesize.h" 1 3 4
# 19 "/usr/include/bits/timesize.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 20 "/usr/include/bits/timesize.h" 2 3 4
# 29 "/usr/include/bits/types.h" 2 3 4



# 31 "/usr/include/bits/types.h" 3 4
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;






typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;



typedef long int __quad_t;
typedef unsigned long int __u_quad_t;







typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
# 141 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/typesizes.h" 1 3 4
# 142 "/usr/include/bits/types.h" 2 3 4
# 1 "/usr/include/bits/time64.h" 1 3 4
# 143 "/usr/include/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;




typedef int __sig_atomic_t;
# 30 "/usr/include/sys/types.h" 2 3 4



typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;


typedef __loff_t loff_t;






typedef __ino64_t ino_t;




typedef __ino64_t ino64_t;




typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;







typedef __off64_t off_t;




typedef __off64_t off64_t;




typedef __pid_t pid_t;





typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;




# 1 "/usr/include/bits/types/clock_t.h" 1 3 4






typedef __clock_t clock_t;
# 127 "/usr/include/sys/types.h" 2 3 4

# 1 "/usr/include/bits/types/clockid_t.h" 1 3 4






typedef __clockid_t clockid_t;
# 129 "/usr/include/sys/types.h" 2 3 4
# 1 "/usr/include/bits/types/time_t.h" 1 3 4
# 10 "/usr/include/bits/types/time_t.h" 3 4
typedef __time_t time_t;
# 130 "/usr/include/sys/types.h" 2 3 4
# 1 "/usr/include/bits/types/timer_t.h" 1 3 4






typedef __timer_t timer_t;
# 131 "/usr/include/sys/types.h" 2 3 4



typedef __useconds_t useconds_t;



typedef __suseconds_t suseconds_t;





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 229 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 145 "/usr/include/sys/types.h" 2 3 4



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;




# 1 "/usr/include/bits/stdint-intn.h" 1 3 4
# 24 "/usr/include/bits/stdint-intn.h" 3 4
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
# 156 "/usr/include/sys/types.h" 2 3 4


typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;


typedef int register_t __attribute__ ((__mode__ (__word__)));
# 176 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 24 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/endian.h" 1 3 4
# 35 "/usr/include/bits/endian.h" 3 4
# 1 "/usr/include/bits/endianness.h" 1 3 4
# 36 "/usr/include/bits/endian.h" 2 3 4
# 25 "/usr/include/endian.h" 2 3 4
# 35 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/byteswap.h" 1 3 4
# 33 "/usr/include/bits/byteswap.h" 3 4
static __inline __uint16_t
__bswap_16 (__uint16_t __bsx)
{

  return __builtin_bswap16 (__bsx);



}






static __inline __uint32_t
__bswap_32 (__uint32_t __bsx)
{

  return __builtin_bswap32 (__bsx);



}
# 69 "/usr/include/bits/byteswap.h" 3 4
__extension__ static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{

  return __builtin_bswap64 (__bsx);



}
# 36 "/usr/include/endian.h" 2 3 4
# 1 "/usr/include/bits/uintn-identity.h" 1 3 4
# 32 "/usr/include/bits/uintn-identity.h" 3 4
static __inline __uint16_t
__uint16_identity (__uint16_t __x)
{
  return __x;
}

static __inline __uint32_t
__uint32_identity (__uint32_t __x)
{
  return __x;
}

static __inline __uint64_t
__uint64_identity (__uint64_t __x)
{
  return __x;
}
# 37 "/usr/include/endian.h" 2 3 4
# 177 "/usr/include/sys/types.h" 2 3 4


# 1 "/usr/include/sys/select.h" 1 3 4
# 30 "/usr/include/sys/select.h" 3 4
# 1 "/usr/include/bits/select.h" 1 3 4
# 31 "/usr/include/sys/select.h" 2 3 4


# 1 "/usr/include/bits/types/sigset_t.h" 1 3 4



# 1 "/usr/include/bits/types/__sigset_t.h" 1 3 4




typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
# 5 "/usr/include/bits/types/sigset_t.h" 2 3 4


typedef __sigset_t sigset_t;
# 34 "/usr/include/sys/select.h" 2 3 4



# 1 "/usr/include/bits/types/struct_timeval.h" 1 3 4







struct timeval
{




  __time_t tv_sec;
  __suseconds_t tv_usec;

};
# 38 "/usr/include/sys/select.h" 2 3 4

# 1 "/usr/include/bits/types/struct_timespec.h" 1 3 4
# 11 "/usr/include/bits/types/struct_timespec.h" 3 4
struct timespec
{



  __time_t tv_sec;




  __syscall_slong_t tv_nsec;
# 31 "/usr/include/bits/types/struct_timespec.h" 3 4
};
# 40 "/usr/include/sys/select.h" 2 3 4
# 49 "/usr/include/sys/select.h" 3 4
typedef long int __fd_mask;
# 59 "/usr/include/sys/select.h" 3 4
typedef struct
  {



    __fd_mask fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];





  } fd_set;






typedef __fd_mask fd_mask;
# 91 "/usr/include/sys/select.h" 3 4

# 102 "/usr/include/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 127 "/usr/include/sys/select.h" 3 4
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);
# 153 "/usr/include/sys/select.h" 3 4

# 180 "/usr/include/sys/types.h" 2 3 4





typedef __blksize_t blksize_t;
# 205 "/usr/include/sys/types.h" 3 4
typedef __blkcnt64_t blkcnt_t;



typedef __fsblkcnt64_t fsblkcnt_t;



typedef __fsfilcnt64_t fsfilcnt_t;





typedef __blkcnt64_t blkcnt64_t;
typedef __fsblkcnt64_t fsblkcnt64_t;
typedef __fsfilcnt64_t fsfilcnt64_t;





# 1 "/usr/include/bits/pthreadtypes.h" 1 3 4
# 23 "/usr/include/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/bits/thread-shared-types.h" 1 3 4
# 44 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/pthreadtypes-arch.h" 1 3 4
# 21 "/usr/include/bits/pthreadtypes-arch.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 22 "/usr/include/bits/pthreadtypes-arch.h" 2 3 4
# 45 "/usr/include/bits/thread-shared-types.h" 2 3 4

# 1 "/usr/include/bits/atomic_wide_counter.h" 1 3 4
# 25 "/usr/include/bits/atomic_wide_counter.h" 3 4
typedef union
{
  __extension__ unsigned long long int __value64;
  struct
  {
    unsigned int __low;
    unsigned int __high;
  } __value32;
} __atomic_wide_counter;
# 47 "/usr/include/bits/thread-shared-types.h" 2 3 4




typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;

typedef struct __pthread_internal_slist
{
  struct __pthread_internal_slist *__next;
} __pthread_slist_t;
# 76 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/struct_mutex.h" 1 3 4
# 22 "/usr/include/bits/struct_mutex.h" 3 4
struct __pthread_mutex_s
{
  int __lock;
  unsigned int __count;
  int __owner;

  unsigned int __nusers;



  int __kind;

  short __spins;
  short __elision;
  __pthread_list_t __list;
# 53 "/usr/include/bits/struct_mutex.h" 3 4
};
# 77 "/usr/include/bits/thread-shared-types.h" 2 3 4
# 89 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/struct_rwlock.h" 1 3 4
# 23 "/usr/include/bits/struct_rwlock.h" 3 4
struct __pthread_rwlock_arch_t
{
  unsigned int __readers;
  unsigned int __writers;
  unsigned int __wrphase_futex;
  unsigned int __writers_futex;
  unsigned int __pad3;
  unsigned int __pad4;

  int __cur_writer;
  int __shared;
  signed char __rwelision;




  unsigned char __pad1[7];


  unsigned long int __pad2;


  unsigned int __flags;
# 55 "/usr/include/bits/struct_rwlock.h" 3 4
};
# 90 "/usr/include/bits/thread-shared-types.h" 2 3 4




struct __pthread_cond_s
{
  __atomic_wide_counter __wseq;
  __atomic_wide_counter __g1_start;
  unsigned int __g_size[2] ;
  unsigned int __g1_orig_size;
  unsigned int __wrefs;
  unsigned int __g_signals[2];
  unsigned int __unused_initialized_1;
  unsigned int __unused_initialized_2;
};

typedef unsigned int __tss_t;
typedef unsigned long int __thrd_t;

typedef struct
{
  int __data ;
} __once_flag;
# 24 "/usr/include/bits/pthreadtypes.h" 2 3 4



typedef unsigned long int pthread_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;



typedef unsigned int pthread_key_t;



typedef int pthread_once_t;


union pthread_attr_t
{
  char __size[56];
  long int __align;
};

typedef union pthread_attr_t pthread_attr_t;




typedef union
{
  struct __pthread_mutex_s __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;


typedef union
{
  struct __pthread_cond_s __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;





typedef union
{
  struct __pthread_rwlock_arch_t __data;
  char __size[56];
  long int __align;
} pthread_rwlock_t;

typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;





typedef volatile int pthread_spinlock_t;




typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;
# 228 "/usr/include/sys/types.h" 2 3 4



# 12 "./include/holy/emu/hostfile.h" 2
# 1 "./include/holy/osdep/hostfile.h" 1
# 11 "./include/holy/osdep/hostfile.h"
# 1 "./include/holy/osdep/hostfile_unix.h" 1
# 9 "./include/holy/osdep/hostfile_unix.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/osdep/hostfile_unix.h" 2






# 1 "/usr/include/sys/stat.h" 1 3 4
# 99 "/usr/include/sys/stat.h" 3 4


# 1 "/usr/include/bits/stat.h" 1 3 4
# 25 "/usr/include/bits/stat.h" 3 4
# 1 "/usr/include/bits/struct_stat.h" 1 3 4
# 26 "/usr/include/bits/struct_stat.h" 3 4
struct stat
  {



    __dev_t st_dev;




    __ino_t st_ino;







    __nlink_t st_nlink;
    __mode_t st_mode;

    __uid_t st_uid;
    __gid_t st_gid;

    int __pad0;

    __dev_t st_rdev;




    __off_t st_size;



    __blksize_t st_blksize;

    __blkcnt_t st_blocks;
# 74 "/usr/include/bits/struct_stat.h" 3 4
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 89 "/usr/include/bits/struct_stat.h" 3 4
    __syscall_slong_t __glibc_reserved[3];
# 99 "/usr/include/bits/struct_stat.h" 3 4
  };



struct stat64
  {



    __dev_t st_dev;

    __ino64_t st_ino;
    __nlink_t st_nlink;
    __mode_t st_mode;






    __uid_t st_uid;
    __gid_t st_gid;

    int __pad0;
    __dev_t st_rdev;
    __off_t st_size;





    __blksize_t st_blksize;
    __blkcnt64_t st_blocks;







    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 151 "/usr/include/bits/struct_stat.h" 3 4
    __syscall_slong_t __glibc_reserved[3];




  };
# 26 "/usr/include/bits/stat.h" 2 3 4
# 102 "/usr/include/sys/stat.h" 2 3 4
# 227 "/usr/include/sys/stat.h" 3 4
extern int stat (const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "stat64") __attribute__ ((__nothrow__ , __leaf__))

     __attribute__ ((__nonnull__ (1, 2)));
extern int fstat (int __fd, struct stat *__buf) __asm__ ("" "fstat64") __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__nonnull__ (2)));
# 240 "/usr/include/sys/stat.h" 3 4
extern int stat64 (const char *__restrict __file,
     struct stat64 *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int fstat64 (int __fd, struct stat64 *__buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
# 279 "/usr/include/sys/stat.h" 3 4
extern int fstatat (int __fd, const char *__restrict __file, struct stat *__restrict __buf, int __flag) __asm__ ("" "fstatat64") __attribute__ ((__nothrow__ , __leaf__))


                 __attribute__ ((__nonnull__ (3)));
# 291 "/usr/include/sys/stat.h" 3 4
extern int fstatat64 (int __fd, const char *__restrict __file,
        struct stat64 *__restrict __buf, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 327 "/usr/include/sys/stat.h" 3 4
extern int lstat (const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "lstat64") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1, 2)));







extern int lstat64 (const char *__restrict __file,
      struct stat64 *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
# 352 "/usr/include/sys/stat.h" 3 4
extern int chmod (const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int lchmod (const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fchmod (int __fd, __mode_t __mode) __attribute__ ((__nothrow__ , __leaf__));





extern int fchmodat (int __fd, const char *__file, __mode_t __mode,
       int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;






extern __mode_t umask (__mode_t __mask) __attribute__ ((__nothrow__ , __leaf__));




extern __mode_t getumask (void) __attribute__ ((__nothrow__ , __leaf__));



extern int mkdir (const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mkdirat (int __fd, const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));






extern int mknod (const char *__path, __mode_t __mode, __dev_t __dev)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mknodat (int __fd, const char *__path, __mode_t __mode,
      __dev_t __dev) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));





extern int mkfifo (const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mkfifoat (int __fd, const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));






extern int utimensat (int __fd, const char *__path,
        const struct timespec __times[2],
        int __flags)
     __attribute__ ((__nothrow__ , __leaf__));
# 452 "/usr/include/sys/stat.h" 3 4
extern int futimens (int __fd, const struct timespec __times[2]) __attribute__ ((__nothrow__ , __leaf__));
# 465 "/usr/include/sys/stat.h" 3 4
# 1 "/usr/include/bits/statx.h" 1 3 4
# 31 "/usr/include/bits/statx.h" 3 4
# 1 "/usr/include/linux/stat.h" 1 3 4




# 1 "/usr/include/linux/types.h" 1 3 4




# 1 "/usr/include/asm/types.h" 1 3 4
# 1 "/usr/include/asm-generic/types.h" 1 3 4






# 1 "/usr/include/asm-generic/int-ll64.h" 1 3 4
# 12 "/usr/include/asm-generic/int-ll64.h" 3 4
# 1 "/usr/include/asm/bitsperlong.h" 1 3 4
# 11 "/usr/include/asm/bitsperlong.h" 3 4
# 1 "/usr/include/asm-generic/bitsperlong.h" 1 3 4
# 12 "/usr/include/asm/bitsperlong.h" 2 3 4
# 13 "/usr/include/asm-generic/int-ll64.h" 2 3 4







typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;


__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
# 8 "/usr/include/asm-generic/types.h" 2 3 4
# 2 "/usr/include/asm/types.h" 2 3 4
# 6 "/usr/include/linux/types.h" 2 3 4



# 1 "/usr/include/linux/posix_types.h" 1 3 4




# 1 "/usr/include/linux/stddef.h" 1 3 4
# 6 "/usr/include/linux/posix_types.h" 2 3 4
# 25 "/usr/include/linux/posix_types.h" 3 4
typedef struct {
 unsigned long fds_bits[1024 / (8 * sizeof(long))];
} __kernel_fd_set;


typedef void (*__kernel_sighandler_t)(int);


typedef int __kernel_key_t;
typedef int __kernel_mqd_t;

# 1 "/usr/include/asm/posix_types.h" 1 3 4






# 1 "/usr/include/asm/posix_types_64.h" 1 3 4
# 11 "/usr/include/asm/posix_types_64.h" 3 4
typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;


typedef unsigned long __kernel_old_dev_t;


# 1 "/usr/include/asm-generic/posix_types.h" 1 3 4
# 15 "/usr/include/asm-generic/posix_types.h" 3 4
typedef long __kernel_long_t;
typedef unsigned long __kernel_ulong_t;



typedef __kernel_ulong_t __kernel_ino_t;



typedef unsigned int __kernel_mode_t;



typedef int __kernel_pid_t;



typedef int __kernel_ipc_pid_t;



typedef unsigned int __kernel_uid_t;
typedef unsigned int __kernel_gid_t;



typedef __kernel_long_t __kernel_suseconds_t;



typedef int __kernel_daddr_t;



typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;
# 72 "/usr/include/asm-generic/posix_types.h" 3 4
typedef __kernel_ulong_t __kernel_size_t;
typedef __kernel_long_t __kernel_ssize_t;
typedef __kernel_long_t __kernel_ptrdiff_t;




typedef struct {
 int val[2];
} __kernel_fsid_t;





typedef __kernel_long_t __kernel_off_t;
typedef long long __kernel_loff_t;
typedef __kernel_long_t __kernel_old_time_t;
typedef __kernel_long_t __kernel_time_t;
typedef long long __kernel_time64_t;
typedef __kernel_long_t __kernel_clock_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
# 19 "/usr/include/asm/posix_types_64.h" 2 3 4
# 8 "/usr/include/asm/posix_types.h" 2 3 4
# 37 "/usr/include/linux/posix_types.h" 2 3 4
# 10 "/usr/include/linux/types.h" 2 3 4


typedef __signed__ __int128 __s128 __attribute__((aligned(16)));
typedef unsigned __int128 __u128 __attribute__((aligned(16)));
# 31 "/usr/include/linux/types.h" 3 4
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef __u16 __sum16;
typedef __u32 __wsum;
# 55 "/usr/include/linux/types.h" 3 4
typedef unsigned __poll_t;
# 6 "/usr/include/linux/stat.h" 2 3 4
# 56 "/usr/include/linux/stat.h" 3 4
struct statx_timestamp {
 __s64 tv_sec;
 __u32 tv_nsec;
 __s32 __reserved;
};
# 99 "/usr/include/linux/stat.h" 3 4
struct statx {


 __u32 stx_mask;


 __u32 stx_blksize;


 __u64 stx_attributes;



 __u32 stx_nlink;


 __u32 stx_uid;


 __u32 stx_gid;


 __u16 stx_mode;
 __u16 __spare0[1];



 __u64 stx_ino;


 __u64 stx_size;


 __u64 stx_blocks;


 __u64 stx_attributes_mask;



 struct statx_timestamp stx_atime;


 struct statx_timestamp stx_btime;


 struct statx_timestamp stx_ctime;


 struct statx_timestamp stx_mtime;



 __u32 stx_rdev_major;
 __u32 stx_rdev_minor;


 __u32 stx_dev_major;
 __u32 stx_dev_minor;


 __u64 stx_mnt_id;


 __u32 stx_dio_mem_align;


 __u32 stx_dio_offset_align;



 __u64 stx_subvol;


 __u32 stx_atomic_write_unit_min;


 __u32 stx_atomic_write_unit_max;



 __u32 stx_atomic_write_segments_max;


 __u32 stx_dio_read_offset_align;


 __u32 stx_atomic_write_unit_max_opt;
 __u32 __spare2[1];


 __u64 __spare3[8];


};
# 32 "/usr/include/bits/statx.h" 2 3 4







# 1 "/usr/include/bits/statx-generic.h" 1 3 4
# 25 "/usr/include/bits/statx-generic.h" 3 4
# 1 "/usr/include/bits/types/struct_statx_timestamp.h" 1 3 4
# 26 "/usr/include/bits/statx-generic.h" 2 3 4
# 1 "/usr/include/bits/types/struct_statx.h" 1 3 4
# 27 "/usr/include/bits/statx-generic.h" 2 3 4
# 62 "/usr/include/bits/statx-generic.h" 3 4



int statx (int __dirfd, const char *__restrict __path, int __flags,
           unsigned int __mask, struct statx *__restrict __buf)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (5)));


# 40 "/usr/include/bits/statx.h" 2 3 4
# 466 "/usr/include/sys/stat.h" 2 3 4



# 17 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/fcntl.h" 1 3 4
# 28 "/usr/include/fcntl.h" 3 4







# 1 "/usr/include/bits/fcntl.h" 1 3 4
# 35 "/usr/include/bits/fcntl.h" 3 4
struct flock
  {
    short int l_type;
    short int l_whence;




    __off64_t l_start;
    __off64_t l_len;

    __pid_t l_pid;
  };


struct flock64
  {
    short int l_type;
    short int l_whence;
    __off64_t l_start;
    __off64_t l_len;
    __pid_t l_pid;
  };



# 1 "/usr/include/bits/fcntl-linux.h" 1 3 4
# 38 "/usr/include/bits/fcntl-linux.h" 3 4
# 1 "/usr/include/bits/types/struct_iovec.h" 1 3 4
# 23 "/usr/include/bits/types/struct_iovec.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 24 "/usr/include/bits/types/struct_iovec.h" 2 3 4


struct iovec
  {
    void *iov_base;
    size_t iov_len;
  };
# 39 "/usr/include/bits/fcntl-linux.h" 2 3 4
# 267 "/usr/include/bits/fcntl-linux.h" 3 4
enum __pid_type
  {
    F_OWNER_TID = 0,
    F_OWNER_PID,
    F_OWNER_PGRP,
    F_OWNER_GID = F_OWNER_PGRP
  };


struct f_owner_ex
  {
    enum __pid_type type;
    __pid_t pid;
  };
# 357 "/usr/include/bits/fcntl-linux.h" 3 4
# 1 "/usr/include/linux/falloc.h" 1 3 4
# 358 "/usr/include/bits/fcntl-linux.h" 2 3 4



struct file_handle
{
  unsigned int handle_bytes;
  int handle_type;

  unsigned char f_handle[0];
};
# 386 "/usr/include/bits/fcntl-linux.h" 3 4





extern __ssize_t readahead (int __fd, __off64_t __offset, size_t __count)
    __attribute__ ((__nothrow__ , __leaf__));






extern int sync_file_range (int __fd, __off64_t __offset, __off64_t __count,
       unsigned int __flags);






extern __ssize_t vmsplice (int __fdout, const struct iovec *__iov,
      size_t __count, unsigned int __flags);





extern __ssize_t splice (int __fdin, __off64_t *__offin, int __fdout,
    __off64_t *__offout, size_t __len,
    unsigned int __flags);





extern __ssize_t tee (int __fdin, int __fdout, size_t __len,
        unsigned int __flags);
# 433 "/usr/include/bits/fcntl-linux.h" 3 4
extern int fallocate (int __fd, int __mode, __off64_t __offset, __off64_t __len) __asm__ ("" "fallocate64")

                     ;





extern int fallocate64 (int __fd, int __mode, __off64_t __offset,
   __off64_t __len);




extern int name_to_handle_at (int __dfd, const char *__name,
         struct file_handle *__handle, int *__mnt_id,
         int __flags) __attribute__ ((__nothrow__ , __leaf__));





extern int open_by_handle_at (int __mountdirfd, struct file_handle *__handle,
         int __flags);




# 62 "/usr/include/bits/fcntl.h" 2 3 4
# 36 "/usr/include/fcntl.h" 2 3 4
# 78 "/usr/include/fcntl.h" 3 4
# 1 "/usr/include/bits/stat.h" 1 3 4
# 79 "/usr/include/fcntl.h" 2 3 4
# 180 "/usr/include/fcntl.h" 3 4
extern int fcntl (int __fd, int __cmd, ...) __asm__ ("" "fcntl64");





extern int fcntl64 (int __fd, int __cmd, ...);
# 212 "/usr/include/fcntl.h" 3 4
extern int open (const char *__file, int __oflag, ...) __asm__ ("" "open64")
     __attribute__ ((__nonnull__ (1)));





extern int open64 (const char *__file, int __oflag, ...) __attribute__ ((__nonnull__ (1)));
# 237 "/usr/include/fcntl.h" 3 4
extern int openat (int __fd, const char *__file, int __oflag, ...) __asm__ ("" "openat64")
                    __attribute__ ((__nonnull__ (2)));





extern int openat64 (int __fd, const char *__file, int __oflag, ...)
     __attribute__ ((__nonnull__ (2)));
# 258 "/usr/include/fcntl.h" 3 4
extern int creat (const char *__file, mode_t __mode) __asm__ ("" "creat64")
                  __attribute__ ((__nonnull__ (1)));





extern int creat64 (const char *__file, mode_t __mode) __attribute__ ((__nonnull__ (1)));
# 287 "/usr/include/fcntl.h" 3 4
extern int lockf (int __fd, int __cmd, __off64_t __len) __asm__ ("" "lockf64")
                                     ;





extern int lockf64 (int __fd, int __cmd, off64_t __len) ;
# 306 "/usr/include/fcntl.h" 3 4
extern int posix_fadvise (int __fd, __off64_t __offset, __off64_t __len, int __advise) __asm__ ("" "posix_fadvise64") __attribute__ ((__nothrow__ , __leaf__))

                      ;





extern int posix_fadvise64 (int __fd, off64_t __offset, off64_t __len,
       int __advise) __attribute__ ((__nothrow__ , __leaf__));
# 327 "/usr/include/fcntl.h" 3 4
extern int posix_fallocate (int __fd, __off64_t __offset, __off64_t __len) __asm__ ("" "posix_fallocate64")

                           ;





extern int posix_fallocate64 (int __fd, off64_t __offset, off64_t __len);
# 345 "/usr/include/fcntl.h" 3 4

# 18 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/dirent.h" 1 3 4
# 27 "/usr/include/dirent.h" 3 4

# 61 "/usr/include/dirent.h" 3 4
# 1 "/usr/include/bits/dirent.h" 1 3 4
# 22 "/usr/include/bits/dirent.h" 3 4
struct dirent
  {




    __ino64_t d_ino;
    __off64_t d_off;

    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
  };


struct dirent64
  {
    __ino64_t d_ino;
    __off64_t d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
  };
# 62 "/usr/include/dirent.h" 2 3 4
# 97 "/usr/include/dirent.h" 3 4
enum
  {
    DT_UNKNOWN = 0,

    DT_FIFO = 1,

    DT_CHR = 2,

    DT_DIR = 4,

    DT_BLK = 6,

    DT_REG = 8,

    DT_LNK = 10,

    DT_SOCK = 12,

    DT_WHT = 14

  };
# 127 "/usr/include/dirent.h" 3 4
typedef struct __dirstream DIR;






extern int closedir (DIR *__dirp) __attribute__ ((__nonnull__ (1)));






extern DIR *opendir (const char *__name) __attribute__ ((__nonnull__ (1)))
 __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (closedir, 1)));






extern DIR *fdopendir (int __fd)
 __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (closedir, 1)));
# 167 "/usr/include/dirent.h" 3 4
extern struct dirent *readdir (DIR *__dirp) __asm__ ("" "readdir64")
     __attribute__ ((__nonnull__ (1)));






extern struct dirent64 *readdir64 (DIR *__dirp) __attribute__ ((__nonnull__ (1)));
# 191 "/usr/include/dirent.h" 3 4
extern int readdir_r (DIR *__restrict __dirp, struct dirent *__restrict __entry, struct dirent **__restrict __result) __asm__ ("" "readdir64_r")




  __attribute__ ((__nonnull__ (1, 2, 3))) __attribute__ ((__deprecated__));






extern int readdir64_r (DIR *__restrict __dirp,
   struct dirent64 *__restrict __entry,
   struct dirent64 **__restrict __result)
  __attribute__ ((__nonnull__ (1, 2, 3))) __attribute__ ((__deprecated__));




extern void rewinddir (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern void seekdir (DIR *__dirp, long int __pos) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int telldir (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int dirfd (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 235 "/usr/include/dirent.h" 3 4
# 1 "/usr/include/bits/posix1_lim.h" 1 3 4
# 27 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 28 "/usr/include/bits/posix1_lim.h" 2 3 4
# 161 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/local_lim.h" 1 3 4
# 38 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/linux/limits.h" 1 3 4
# 39 "/usr/include/bits/local_lim.h" 2 3 4
# 81 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/bits/pthread_stack_min-dynamic.h" 1 3 4
# 23 "/usr/include/bits/pthread_stack_min-dynamic.h" 3 4

extern long int __sysconf (int __name) __attribute__ ((__nothrow__ , __leaf__));

# 82 "/usr/include/bits/local_lim.h" 2 3 4
# 162 "/usr/include/bits/posix1_lim.h" 2 3 4
# 236 "/usr/include/dirent.h" 2 3 4
# 247 "/usr/include/dirent.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 248 "/usr/include/dirent.h" 2 3 4
# 265 "/usr/include/dirent.h" 3 4
extern int scandir (const char *__restrict __dir, struct dirent ***__restrict __namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **)) __asm__ ("" "scandir64")





                    __attribute__ ((__nonnull__ (1, 2)));
# 280 "/usr/include/dirent.h" 3 4
extern int scandir64 (const char *__restrict __dir,
        struct dirent64 ***__restrict __namelist,
        int (*__selector) (const struct dirent64 *),
        int (*__cmp) (const struct dirent64 **,
        const struct dirent64 **))
     __attribute__ ((__nonnull__ (1, 2)));
# 303 "/usr/include/dirent.h" 3 4
extern int scandirat (int __dfd, const char *__restrict __dir, struct dirent ***__restrict __namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **)) __asm__ ("" "scandirat64")





                      __attribute__ ((__nonnull__ (2, 3)));







extern int scandirat64 (int __dfd, const char *__restrict __dir,
   struct dirent64 ***__restrict __namelist,
   int (*__selector) (const struct dirent64 *),
   int (*__cmp) (const struct dirent64 **,
          const struct dirent64 **))
     __attribute__ ((__nonnull__ (2, 3)));
# 332 "/usr/include/dirent.h" 3 4
extern int alphasort (const struct dirent **__e1, const struct dirent **__e2) __asm__ ("" "alphasort64") __attribute__ ((__nothrow__ , __leaf__))


                   __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int alphasort64 (const struct dirent64 **__e1,
   const struct dirent64 **__e2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 361 "/usr/include/dirent.h" 3 4
extern __ssize_t getdirentries (int __fd, char *__restrict __buf, size_t __nbytes, __off64_t *__restrict __basep) __asm__ ("" "getdirentries64") __attribute__ ((__nothrow__ , __leaf__))



                      __attribute__ ((__nonnull__ (2, 4)));






extern __ssize_t getdirentries64 (int __fd, char *__restrict __buf,
      size_t __nbytes,
      __off64_t *__restrict __basep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));
# 387 "/usr/include/dirent.h" 3 4
extern int versionsort (const struct dirent **__e1, const struct dirent **__e2) __asm__ ("" "versionsort64") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int versionsort64 (const struct dirent64 **__e1,
     const struct dirent64 **__e2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));





# 1 "/usr/include/bits/dirent_ext.h" 1 3 4
# 23 "/usr/include/bits/dirent_ext.h" 3 4






extern __ssize_t getdents64 (int __fd, void *__buffer, size_t __length)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



# 407 "/usr/include/dirent.h" 2 3 4
# 19 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/unistd.h" 1 3 4
# 27 "/usr/include/unistd.h" 3 4

# 202 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/posix_opt.h" 1 3 4
# 203 "/usr/include/unistd.h" 2 3 4



# 1 "/usr/include/bits/environments.h" 1 3 4
# 22 "/usr/include/bits/environments.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 23 "/usr/include/bits/environments.h" 2 3 4
# 207 "/usr/include/unistd.h" 2 3 4
# 226 "/usr/include/unistd.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 227 "/usr/include/unistd.h" 2 3 4
# 267 "/usr/include/unistd.h" 3 4
typedef __intptr_t intptr_t;






typedef __socklen_t socklen_t;
# 287 "/usr/include/unistd.h" 3 4
extern int access (const char *__name, int __type) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int euidaccess (const char *__name, int __type)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int eaccess (const char *__name, int __type)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int execveat (int __fd, const char *__path, char *const __argv[],
                     char *const __envp[], int __flags)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));






extern int faccessat (int __fd, const char *__file, int __type, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;
# 342 "/usr/include/unistd.h" 3 4
extern __off64_t lseek (int __fd, __off64_t __offset, int __whence) __asm__ ("" "lseek64") __attribute__ ((__nothrow__ , __leaf__))

             ;





extern __off64_t lseek64 (int __fd, __off64_t __offset, int __whence)
     __attribute__ ((__nothrow__ , __leaf__));






extern int close (int __fd);




extern void closefrom (int __lowfd) __attribute__ ((__nothrow__ , __leaf__));







extern ssize_t read (int __fd, void *__buf, size_t __nbytes)
    __attribute__ ((__access__ (__write_only__, 2, 3)));





extern ssize_t write (int __fd, const void *__buf, size_t __n)
    __attribute__ ((__access__ (__read_only__, 2, 3)));
# 404 "/usr/include/unistd.h" 3 4
extern ssize_t pread (int __fd, void *__buf, size_t __nbytes, __off64_t __offset) __asm__ ("" "pread64")


    __attribute__ ((__access__ (__write_only__, 2, 3)));
extern ssize_t pwrite (int __fd, const void *__buf, size_t __nbytes, __off64_t __offset) __asm__ ("" "pwrite64")


    __attribute__ ((__access__ (__read_only__, 2, 3)));
# 422 "/usr/include/unistd.h" 3 4
extern ssize_t pread64 (int __fd, void *__buf, size_t __nbytes,
   __off64_t __offset)
    __attribute__ ((__access__ (__write_only__, 2, 3)));


extern ssize_t pwrite64 (int __fd, const void *__buf, size_t __n,
    __off64_t __offset)
    __attribute__ ((__access__ (__read_only__, 2, 3)));







extern int pipe (int __pipedes[2]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int pipe2 (int __pipedes[2], int __flags) __attribute__ ((__nothrow__ , __leaf__)) ;
# 452 "/usr/include/unistd.h" 3 4
extern unsigned int alarm (unsigned int __seconds) __attribute__ ((__nothrow__ , __leaf__));
# 464 "/usr/include/unistd.h" 3 4
extern unsigned int sleep (unsigned int __seconds);







extern __useconds_t ualarm (__useconds_t __value, __useconds_t __interval)
     __attribute__ ((__nothrow__ , __leaf__));






extern int usleep (__useconds_t __useconds);
# 489 "/usr/include/unistd.h" 3 4
extern int pause (void);



extern int chown (const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchown (int __fd, __uid_t __owner, __gid_t __group) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int lchown (const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;






extern int fchownat (int __fd, const char *__file, __uid_t __owner,
       __gid_t __group, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;



extern int chdir (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchdir (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;
# 531 "/usr/include/unistd.h" 3 4
extern char *getcwd (char *__buf, size_t __size) __attribute__ ((__nothrow__ , __leaf__)) ;





extern char *get_current_dir_name (void) __attribute__ ((__nothrow__ , __leaf__));







extern char *getwd (char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__deprecated__))
    __attribute__ ((__access__ (__write_only__, 1)));




extern int dup (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;


extern int dup2 (int __fd, int __fd2) __attribute__ ((__nothrow__ , __leaf__));




extern int dup3 (int __fd, int __fd2, int __flags) __attribute__ ((__nothrow__ , __leaf__));



extern char **__environ;

extern char **environ;





extern int execve (const char *__path, char *const __argv[],
     char *const __envp[]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int fexecve (int __fd, char *const __argv[], char *const __envp[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));




extern int execv (const char *__path, char *const __argv[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execle (const char *__path, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execl (const char *__path, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execvp (const char *__file, char *const __argv[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int execlp (const char *__file, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int execvpe (const char *__file, char *const __argv[],
      char *const __envp[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern int nice (int __inc) __attribute__ ((__nothrow__ , __leaf__)) ;




extern void _exit (int __status) __attribute__ ((__noreturn__));





# 1 "/usr/include/bits/confname.h" 1 3 4
# 24 "/usr/include/bits/confname.h" 3 4
enum
  {
    _PC_LINK_MAX,

    _PC_MAX_CANON,

    _PC_MAX_INPUT,

    _PC_NAME_MAX,

    _PC_PATH_MAX,

    _PC_PIPE_BUF,

    _PC_CHOWN_RESTRICTED,

    _PC_NO_TRUNC,

    _PC_VDISABLE,

    _PC_SYNC_IO,

    _PC_ASYNC_IO,

    _PC_PRIO_IO,

    _PC_SOCK_MAXBUF,

    _PC_FILESIZEBITS,

    _PC_REC_INCR_XFER_SIZE,

    _PC_REC_MAX_XFER_SIZE,

    _PC_REC_MIN_XFER_SIZE,

    _PC_REC_XFER_ALIGN,

    _PC_ALLOC_SIZE_MIN,

    _PC_SYMLINK_MAX,

    _PC_2_SYMLINKS

  };


enum
  {
    _SC_ARG_MAX,

    _SC_CHILD_MAX,

    _SC_CLK_TCK,

    _SC_NGROUPS_MAX,

    _SC_OPEN_MAX,

    _SC_STREAM_MAX,

    _SC_TZNAME_MAX,

    _SC_JOB_CONTROL,

    _SC_SAVED_IDS,

    _SC_REALTIME_SIGNALS,

    _SC_PRIORITY_SCHEDULING,

    _SC_TIMERS,

    _SC_ASYNCHRONOUS_IO,

    _SC_PRIORITIZED_IO,

    _SC_SYNCHRONIZED_IO,

    _SC_FSYNC,

    _SC_MAPPED_FILES,

    _SC_MEMLOCK,

    _SC_MEMLOCK_RANGE,

    _SC_MEMORY_PROTECTION,

    _SC_MESSAGE_PASSING,

    _SC_SEMAPHORES,

    _SC_SHARED_MEMORY_OBJECTS,

    _SC_AIO_LISTIO_MAX,

    _SC_AIO_MAX,

    _SC_AIO_PRIO_DELTA_MAX,

    _SC_DELAYTIMER_MAX,

    _SC_MQ_OPEN_MAX,

    _SC_MQ_PRIO_MAX,

    _SC_VERSION,

    _SC_PAGESIZE,


    _SC_RTSIG_MAX,

    _SC_SEM_NSEMS_MAX,

    _SC_SEM_VALUE_MAX,

    _SC_SIGQUEUE_MAX,

    _SC_TIMER_MAX,




    _SC_BC_BASE_MAX,

    _SC_BC_DIM_MAX,

    _SC_BC_SCALE_MAX,

    _SC_BC_STRING_MAX,

    _SC_COLL_WEIGHTS_MAX,

    _SC_EQUIV_CLASS_MAX,

    _SC_EXPR_NEST_MAX,

    _SC_LINE_MAX,

    _SC_RE_DUP_MAX,

    _SC_CHARCLASS_NAME_MAX,


    _SC_2_VERSION,

    _SC_2_C_BIND,

    _SC_2_C_DEV,

    _SC_2_FORT_DEV,

    _SC_2_FORT_RUN,

    _SC_2_SW_DEV,

    _SC_2_LOCALEDEF,


    _SC_PII,

    _SC_PII_XTI,

    _SC_PII_SOCKET,

    _SC_PII_INTERNET,

    _SC_PII_OSI,

    _SC_POLL,

    _SC_SELECT,

    _SC_UIO_MAXIOV,

    _SC_IOV_MAX = _SC_UIO_MAXIOV,

    _SC_PII_INTERNET_STREAM,

    _SC_PII_INTERNET_DGRAM,

    _SC_PII_OSI_COTS,

    _SC_PII_OSI_CLTS,

    _SC_PII_OSI_M,

    _SC_T_IOV_MAX,



    _SC_THREADS,

    _SC_THREAD_SAFE_FUNCTIONS,

    _SC_GETGR_R_SIZE_MAX,

    _SC_GETPW_R_SIZE_MAX,

    _SC_LOGIN_NAME_MAX,

    _SC_TTY_NAME_MAX,

    _SC_THREAD_DESTRUCTOR_ITERATIONS,

    _SC_THREAD_KEYS_MAX,

    _SC_THREAD_STACK_MIN,

    _SC_THREAD_THREADS_MAX,

    _SC_THREAD_ATTR_STACKADDR,

    _SC_THREAD_ATTR_STACKSIZE,

    _SC_THREAD_PRIORITY_SCHEDULING,

    _SC_THREAD_PRIO_INHERIT,

    _SC_THREAD_PRIO_PROTECT,

    _SC_THREAD_PROCESS_SHARED,


    _SC_NPROCESSORS_CONF,

    _SC_NPROCESSORS_ONLN,

    _SC_PHYS_PAGES,

    _SC_AVPHYS_PAGES,

    _SC_ATEXIT_MAX,

    _SC_PASS_MAX,


    _SC_XOPEN_VERSION,

    _SC_XOPEN_XCU_VERSION,

    _SC_XOPEN_UNIX,

    _SC_XOPEN_CRYPT,

    _SC_XOPEN_ENH_I18N,

    _SC_XOPEN_SHM,


    _SC_2_CHAR_TERM,

    _SC_2_C_VERSION,

    _SC_2_UPE,


    _SC_XOPEN_XPG2,

    _SC_XOPEN_XPG3,

    _SC_XOPEN_XPG4,


    _SC_CHAR_BIT,

    _SC_CHAR_MAX,

    _SC_CHAR_MIN,

    _SC_INT_MAX,

    _SC_INT_MIN,

    _SC_LONG_BIT,

    _SC_WORD_BIT,

    _SC_MB_LEN_MAX,

    _SC_NZERO,

    _SC_SSIZE_MAX,

    _SC_SCHAR_MAX,

    _SC_SCHAR_MIN,

    _SC_SHRT_MAX,

    _SC_SHRT_MIN,

    _SC_UCHAR_MAX,

    _SC_UINT_MAX,

    _SC_ULONG_MAX,

    _SC_USHRT_MAX,


    _SC_NL_ARGMAX,

    _SC_NL_LANGMAX,

    _SC_NL_MSGMAX,

    _SC_NL_NMAX,

    _SC_NL_SETMAX,

    _SC_NL_TEXTMAX,


    _SC_XBS5_ILP32_OFF32,

    _SC_XBS5_ILP32_OFFBIG,

    _SC_XBS5_LP64_OFF64,

    _SC_XBS5_LPBIG_OFFBIG,


    _SC_XOPEN_LEGACY,

    _SC_XOPEN_REALTIME,

    _SC_XOPEN_REALTIME_THREADS,


    _SC_ADVISORY_INFO,

    _SC_BARRIERS,

    _SC_BASE,

    _SC_C_LANG_SUPPORT,

    _SC_C_LANG_SUPPORT_R,

    _SC_CLOCK_SELECTION,

    _SC_CPUTIME,

    _SC_THREAD_CPUTIME,

    _SC_DEVICE_IO,

    _SC_DEVICE_SPECIFIC,

    _SC_DEVICE_SPECIFIC_R,

    _SC_FD_MGMT,

    _SC_FIFO,

    _SC_PIPE,

    _SC_FILE_ATTRIBUTES,

    _SC_FILE_LOCKING,

    _SC_FILE_SYSTEM,

    _SC_MONOTONIC_CLOCK,

    _SC_MULTI_PROCESS,

    _SC_SINGLE_PROCESS,

    _SC_NETWORKING,

    _SC_READER_WRITER_LOCKS,

    _SC_SPIN_LOCKS,

    _SC_REGEXP,

    _SC_REGEX_VERSION,

    _SC_SHELL,

    _SC_SIGNALS,

    _SC_SPAWN,

    _SC_SPORADIC_SERVER,

    _SC_THREAD_SPORADIC_SERVER,

    _SC_SYSTEM_DATABASE,

    _SC_SYSTEM_DATABASE_R,

    _SC_TIMEOUTS,

    _SC_TYPED_MEMORY_OBJECTS,

    _SC_USER_GROUPS,

    _SC_USER_GROUPS_R,

    _SC_2_PBS,

    _SC_2_PBS_ACCOUNTING,

    _SC_2_PBS_LOCATE,

    _SC_2_PBS_MESSAGE,

    _SC_2_PBS_TRACK,

    _SC_SYMLOOP_MAX,

    _SC_STREAMS,

    _SC_2_PBS_CHECKPOINT,


    _SC_V6_ILP32_OFF32,

    _SC_V6_ILP32_OFFBIG,

    _SC_V6_LP64_OFF64,

    _SC_V6_LPBIG_OFFBIG,


    _SC_HOST_NAME_MAX,

    _SC_TRACE,

    _SC_TRACE_EVENT_FILTER,

    _SC_TRACE_INHERIT,

    _SC_TRACE_LOG,


    _SC_LEVEL1_ICACHE_SIZE,

    _SC_LEVEL1_ICACHE_ASSOC,

    _SC_LEVEL1_ICACHE_LINESIZE,

    _SC_LEVEL1_DCACHE_SIZE,

    _SC_LEVEL1_DCACHE_ASSOC,

    _SC_LEVEL1_DCACHE_LINESIZE,

    _SC_LEVEL2_CACHE_SIZE,

    _SC_LEVEL2_CACHE_ASSOC,

    _SC_LEVEL2_CACHE_LINESIZE,

    _SC_LEVEL3_CACHE_SIZE,

    _SC_LEVEL3_CACHE_ASSOC,

    _SC_LEVEL3_CACHE_LINESIZE,

    _SC_LEVEL4_CACHE_SIZE,

    _SC_LEVEL4_CACHE_ASSOC,

    _SC_LEVEL4_CACHE_LINESIZE,



    _SC_IPV6 = _SC_LEVEL1_ICACHE_SIZE + 50,

    _SC_RAW_SOCKETS,


    _SC_V7_ILP32_OFF32,

    _SC_V7_ILP32_OFFBIG,

    _SC_V7_LP64_OFF64,

    _SC_V7_LPBIG_OFFBIG,


    _SC_SS_REPL_MAX,


    _SC_TRACE_EVENT_NAME_MAX,

    _SC_TRACE_NAME_MAX,

    _SC_TRACE_SYS_MAX,

    _SC_TRACE_USER_EVENT_MAX,


    _SC_XOPEN_STREAMS,


    _SC_THREAD_ROBUST_PRIO_INHERIT,

    _SC_THREAD_ROBUST_PRIO_PROTECT,


    _SC_MINSIGSTKSZ,


    _SC_SIGSTKSZ

  };


enum
  {
    _CS_PATH,


    _CS_V6_WIDTH_RESTRICTED_ENVS,



    _CS_GNU_LIBC_VERSION,

    _CS_GNU_LIBPTHREAD_VERSION,


    _CS_V5_WIDTH_RESTRICTED_ENVS,



    _CS_V7_WIDTH_RESTRICTED_ENVS,



    _CS_LFS_CFLAGS = 1000,

    _CS_LFS_LDFLAGS,

    _CS_LFS_LIBS,

    _CS_LFS_LINTFLAGS,

    _CS_LFS64_CFLAGS,

    _CS_LFS64_LDFLAGS,

    _CS_LFS64_LIBS,

    _CS_LFS64_LINTFLAGS,


    _CS_XBS5_ILP32_OFF32_CFLAGS = 1100,

    _CS_XBS5_ILP32_OFF32_LDFLAGS,

    _CS_XBS5_ILP32_OFF32_LIBS,

    _CS_XBS5_ILP32_OFF32_LINTFLAGS,

    _CS_XBS5_ILP32_OFFBIG_CFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LDFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LIBS,

    _CS_XBS5_ILP32_OFFBIG_LINTFLAGS,

    _CS_XBS5_LP64_OFF64_CFLAGS,

    _CS_XBS5_LP64_OFF64_LDFLAGS,

    _CS_XBS5_LP64_OFF64_LIBS,

    _CS_XBS5_LP64_OFF64_LINTFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_CFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LDFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LIBS,

    _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V6_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LIBS,

    _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V6_LP64_OFF64_CFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LIBS,

    _CS_POSIX_V6_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V7_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LIBS,

    _CS_POSIX_V7_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V7_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V7_LP64_OFF64_CFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LIBS,

    _CS_POSIX_V7_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LINTFLAGS,


    _CS_V6_ENV,

    _CS_V7_ENV

  };
# 631 "/usr/include/unistd.h" 2 3 4


extern long int pathconf (const char *__path, int __name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int fpathconf (int __fd, int __name) __attribute__ ((__nothrow__ , __leaf__));


extern long int sysconf (int __name) __attribute__ ((__nothrow__ , __leaf__));



extern size_t confstr (int __name, char *__buf, size_t __len) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__write_only__, 2, 3)));




extern __pid_t getpid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t getppid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t getpgrp (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t __getpgid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));

extern __pid_t getpgid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));






extern int setpgid (__pid_t __pid, __pid_t __pgid) __attribute__ ((__nothrow__ , __leaf__));
# 682 "/usr/include/unistd.h" 3 4
extern int setpgrp (void) __attribute__ ((__nothrow__ , __leaf__));






extern __pid_t setsid (void) __attribute__ ((__nothrow__ , __leaf__));



extern __pid_t getsid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));



extern __uid_t getuid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __uid_t geteuid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __gid_t getgid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __gid_t getegid (void) __attribute__ ((__nothrow__ , __leaf__));




extern int getgroups (int __size, __gid_t __list[]) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__write_only__, 2, 1)));


extern int group_member (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__));






extern int setuid (__uid_t __uid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setreuid (__uid_t __ruid, __uid_t __euid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int seteuid (__uid_t __uid) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int setgid (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setregid (__gid_t __rgid, __gid_t __egid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setegid (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__)) ;





extern int getresuid (__uid_t *__ruid, __uid_t *__euid, __uid_t *__suid)
     __attribute__ ((__nothrow__ , __leaf__));



extern int getresgid (__gid_t *__rgid, __gid_t *__egid, __gid_t *__sgid)
     __attribute__ ((__nothrow__ , __leaf__));



extern int setresuid (__uid_t __ruid, __uid_t __euid, __uid_t __suid)
     __attribute__ ((__nothrow__ , __leaf__)) ;



extern int setresgid (__gid_t __rgid, __gid_t __egid, __gid_t __sgid)
     __attribute__ ((__nothrow__ , __leaf__)) ;






extern __pid_t fork (void) __attribute__ ((__nothrow__));







extern __pid_t vfork (void) __attribute__ ((__nothrow__ , __leaf__));






extern __pid_t _Fork (void) __attribute__ ((__nothrow__ , __leaf__));





extern char *ttyname (int __fd) __attribute__ ((__nothrow__ , __leaf__));



extern int ttyname_r (int __fd, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)))
     __attribute__ ((__access__ (__write_only__, 2, 3)));



extern int isatty (int __fd) __attribute__ ((__nothrow__ , __leaf__));




extern int ttyslot (void) __attribute__ ((__nothrow__ , __leaf__));




extern int link (const char *__from, const char *__to)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern int linkat (int __fromfd, const char *__from, int __tofd,
     const char *__to, int __flags)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4))) ;




extern int symlink (const char *__from, const char *__to)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern ssize_t readlink (const char *__restrict __path,
    char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)))
     __attribute__ ((__access__ (__write_only__, 2, 3)));





extern int symlinkat (const char *__from, int __tofd,
        const char *__to) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3))) ;


extern ssize_t readlinkat (int __fd, const char *__restrict __path,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)))
     __attribute__ ((__access__ (__write_only__, 3, 4)));



extern int unlink (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int unlinkat (int __fd, const char *__name, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern int rmdir (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern __pid_t tcgetpgrp (int __fd) __attribute__ ((__nothrow__ , __leaf__));


extern int tcsetpgrp (int __fd, __pid_t __pgrp_id) __attribute__ ((__nothrow__ , __leaf__));






extern char *getlogin (void);







extern int getlogin_r (char *__name, size_t __name_len) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));




extern int setlogin (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







# 1 "/usr/include/bits/getopt_posix.h" 1 3 4
# 27 "/usr/include/bits/getopt_posix.h" 3 4
# 1 "/usr/include/bits/getopt_core.h" 1 3 4
# 28 "/usr/include/bits/getopt_core.h" 3 4








extern char *optarg;
# 50 "/usr/include/bits/getopt_core.h" 3 4
extern int optind;




extern int opterr;



extern int optopt;
# 91 "/usr/include/bits/getopt_core.h" 3 4
extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));


# 28 "/usr/include/bits/getopt_posix.h" 2 3 4


# 49 "/usr/include/bits/getopt_posix.h" 3 4

# 904 "/usr/include/unistd.h" 2 3 4







extern int gethostname (char *__name, size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));






extern int sethostname (const char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__read_only__, 1, 2)));



extern int sethostid (long int __id) __attribute__ ((__nothrow__ , __leaf__)) ;





extern int getdomainname (char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
     __attribute__ ((__access__ (__write_only__, 1, 2)));
extern int setdomainname (const char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__read_only__, 1, 2)));




extern int vhangup (void) __attribute__ ((__nothrow__ , __leaf__));


extern int revoke (const char *__file) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;







extern int profil (unsigned short int *__sample_buffer, size_t __size,
     size_t __offset, unsigned int __scale)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int acct (const char *__name) __attribute__ ((__nothrow__ , __leaf__));



extern char *getusershell (void) __attribute__ ((__nothrow__ , __leaf__));
extern void endusershell (void) __attribute__ ((__nothrow__ , __leaf__));
extern void setusershell (void) __attribute__ ((__nothrow__ , __leaf__));





extern int daemon (int __nochdir, int __noclose) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int chroot (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern char *getpass (const char *__prompt) __attribute__ ((__nonnull__ (1)));







extern int fsync (int __fd);





extern int syncfs (int __fd) __attribute__ ((__nothrow__ , __leaf__));






extern long int gethostid (void);


extern void sync (void) __attribute__ ((__nothrow__ , __leaf__));





extern int getpagesize (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern int getdtablesize (void) __attribute__ ((__nothrow__ , __leaf__));
# 1030 "/usr/include/unistd.h" 3 4
extern int truncate (const char *__file, __off64_t __length) __asm__ ("" "truncate64") __attribute__ ((__nothrow__ , __leaf__))

                  __attribute__ ((__nonnull__ (1))) ;





extern int truncate64 (const char *__file, __off64_t __length)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 1052 "/usr/include/unistd.h" 3 4
extern int ftruncate (int __fd, __off64_t __length) __asm__ ("" "ftruncate64") __attribute__ ((__nothrow__ , __leaf__))
                        ;





extern int ftruncate64 (int __fd, __off64_t __length) __attribute__ ((__nothrow__ , __leaf__)) ;
# 1070 "/usr/include/unistd.h" 3 4
extern int brk (void *__addr) __attribute__ ((__nothrow__ , __leaf__)) ;





extern void *sbrk (intptr_t __delta) __attribute__ ((__nothrow__ , __leaf__));
# 1091 "/usr/include/unistd.h" 3 4
extern long int syscall (long int __sysno, ...) __attribute__ ((__nothrow__ , __leaf__));
# 1142 "/usr/include/unistd.h" 3 4
ssize_t copy_file_range (int __infd, __off64_t *__pinoff,
    int __outfd, __off64_t *__poutoff,
    size_t __length, unsigned int __flags);





extern int fdatasync (int __fildes);
# 1162 "/usr/include/unistd.h" 3 4
extern char *crypt (const char *__key, const char *__salt)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));







extern void swab (const void *__restrict __from, void *__restrict __to,
    ssize_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)))
    __attribute__ ((__access__ (__read_only__, 1, 3)))
    __attribute__ ((__access__ (__write_only__, 2, 3)));
# 1201 "/usr/include/unistd.h" 3 4
int getentropy (void *__buffer, size_t __length)
    __attribute__ ((__access__ (__write_only__, 1, 2)));
# 1211 "/usr/include/unistd.h" 3 4
extern int close_range (unsigned int __fd, unsigned int __max_fd,
   int __flags) __attribute__ ((__nothrow__ , __leaf__));
# 1221 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/unistd_ext.h" 1 3 4
# 34 "/usr/include/bits/unistd_ext.h" 3 4
extern __pid_t gettid (void) __attribute__ ((__nothrow__ , __leaf__));



# 1 "/usr/include/linux/close_range.h" 1 3 4
# 39 "/usr/include/bits/unistd_ext.h" 2 3 4
# 1222 "/usr/include/unistd.h" 2 3 4


# 20 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/stdio.h" 1 3 4
# 28 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 29 "/usr/include/stdio.h" 2 3 4





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 35 "/usr/include/stdio.h" 2 3 4





# 1 "/usr/include/bits/types/__fpos_t.h" 1 3 4




# 1 "/usr/include/bits/types/__mbstate_t.h" 1 3 4
# 13 "/usr/include/bits/types/__mbstate_t.h" 3 4
typedef struct
{
  int __count;
  union
  {
    unsigned int __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 6 "/usr/include/bits/types/__fpos_t.h" 2 3 4




typedef struct _G_fpos_t
{
  __off_t __pos;
  __mbstate_t __state;
} __fpos_t;
# 41 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/__fpos64_t.h" 1 3 4
# 10 "/usr/include/bits/types/__fpos64_t.h" 3 4
typedef struct _G_fpos64_t
{
  __off64_t __pos;
  __mbstate_t __state;
} __fpos64_t;
# 42 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/__FILE.h" 1 3 4



struct _IO_FILE;
typedef struct _IO_FILE __FILE;
# 43 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/FILE.h" 1 3 4



struct _IO_FILE;


typedef struct _IO_FILE FILE;
# 44 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/struct_FILE.h" 1 3 4
# 35 "/usr/include/bits/types/struct_FILE.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 36 "/usr/include/bits/types/struct_FILE.h" 2 3 4

struct _IO_FILE;
struct _IO_marker;
struct _IO_codecvt;
struct _IO_wide_data;




typedef void _IO_lock_t;





struct _IO_FILE
{
  int _flags;


  char *_IO_read_ptr;
  char *_IO_read_end;
  char *_IO_read_base;
  char *_IO_write_base;
  char *_IO_write_ptr;
  char *_IO_write_end;
  char *_IO_buf_base;
  char *_IO_buf_end;


  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;
  int _flags2:24;

  char _short_backupbuf[1];
  __off_t _old_offset;


  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

  _IO_lock_t *_lock;







  __off64_t _offset;

  struct _IO_codecvt *_codecvt;
  struct _IO_wide_data *_wide_data;
  struct _IO_FILE *_freeres_list;
  void *_freeres_buf;
  struct _IO_FILE **_prevchain;
  int _mode;

  int _unused3;

  __uint64_t _total_written;




  char _unused2[12 * sizeof (int) - 5 * sizeof (void *)];
};
# 45 "/usr/include/stdio.h" 2 3 4


# 1 "/usr/include/bits/types/cookie_io_functions_t.h" 1 3 4
# 27 "/usr/include/bits/types/cookie_io_functions_t.h" 3 4
typedef __ssize_t cookie_read_function_t (void *__cookie, char *__buf,
                                          size_t __nbytes);







typedef __ssize_t cookie_write_function_t (void *__cookie, const char *__buf,
                                           size_t __nbytes);







typedef int cookie_seek_function_t (void *__cookie, __off64_t *__pos, int __w);


typedef int cookie_close_function_t (void *__cookie);






typedef struct _IO_cookie_io_functions_t
{
  cookie_read_function_t *read;
  cookie_write_function_t *write;
  cookie_seek_function_t *seek;
  cookie_close_function_t *close;
} cookie_io_functions_t;
# 48 "/usr/include/stdio.h" 2 3 4
# 87 "/usr/include/stdio.h" 3 4
typedef __fpos64_t fpos_t;


typedef __fpos64_t fpos64_t;
# 129 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/stdio_lim.h" 1 3 4
# 130 "/usr/include/stdio.h" 2 3 4
# 149 "/usr/include/stdio.h" 3 4
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;






extern int remove (const char *__filename) __attribute__ ((__nothrow__ , __leaf__));

extern int rename (const char *__old, const char *__new) __attribute__ ((__nothrow__ , __leaf__));



extern int renameat (int __oldfd, const char *__old, int __newfd,
       const char *__new) __attribute__ ((__nothrow__ , __leaf__));
# 179 "/usr/include/stdio.h" 3 4
extern int renameat2 (int __oldfd, const char *__old, int __newfd,
        const char *__new, unsigned int __flags) __attribute__ ((__nothrow__ , __leaf__));






extern int fclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 201 "/usr/include/stdio.h" 3 4
extern FILE *tmpfile (void) __asm__ ("" "tmpfile64")
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;






extern FILE *tmpfile64 (void)
   __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;



extern char *tmpnam (char[20]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern char *tmpnam_r (char __s[20]) __attribute__ ((__nothrow__ , __leaf__)) ;
# 231 "/usr/include/stdio.h" 3 4
extern char *tempnam (const char *__dir, const char *__pfx)
   __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (__builtin_free, 1)));






extern int fflush (FILE *__stream);
# 248 "/usr/include/stdio.h" 3 4
extern int fflush_unlocked (FILE *__stream);
# 258 "/usr/include/stdio.h" 3 4
extern int fcloseall (void);
# 279 "/usr/include/stdio.h" 3 4
extern FILE *fopen (const char *__restrict __filename, const char *__restrict __modes) __asm__ ("" "fopen64")

  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
extern FILE *freopen (const char *__restrict __filename, const char *__restrict __modes, FILE *__restrict __stream) __asm__ ("" "freopen64")


  __attribute__ ((__nonnull__ (3)));






extern FILE *fopen64 (const char *__restrict __filename,
        const char *__restrict __modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
extern FILE *freopen64 (const char *__restrict __filename,
   const char *__restrict __modes,
   FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));




extern FILE *fdopen (int __fd, const char *__modes) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;





extern FILE *fopencookie (void *__restrict __magic_cookie,
     const char *__restrict __modes,
     cookie_io_functions_t __io_funcs) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *fmemopen (void *__s, size_t __len, const char *__modes)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *open_memstream (char **__bufloc, size_t *__sizeloc) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
# 337 "/usr/include/stdio.h" 3 4
extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__nonnull__ (1)));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int fprintf (FILE *__restrict __stream,
      const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int printf (const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nonnull__ (1)));




extern int vprintf (const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));



extern int snprintf (char *__restrict __s, size_t __maxlen,
       const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));





extern int vasprintf (char **__restrict __ptr, const char *__restrict __f,
        __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0))) ;
extern int __asprintf (char **__restrict __ptr,
         const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;
extern int asprintf (char **__restrict __ptr,
       const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;




extern int vdprintf (int __fd, const char *__restrict __fmt,
       __gnuc_va_list __arg)
     __attribute__ ((__format__ (__printf__, 2, 0)));
extern int dprintf (int __fd, const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));







extern int fscanf (FILE *__restrict __stream,
     const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int scanf (const char *__restrict __format, ...) ;

extern int sscanf (const char *__restrict __s,
     const char *__restrict __format, ...) __attribute__ ((__nothrow__ , __leaf__));





# 1 "/usr/include/bits/floatn.h" 1 3 4
# 131 "/usr/include/bits/floatn.h" 3 4
# 1 "/usr/include/bits/floatn-common.h" 1 3 4
# 24 "/usr/include/bits/floatn-common.h" 3 4
# 1 "/usr/include/bits/long-double.h" 1 3 4
# 25 "/usr/include/bits/floatn-common.h" 2 3 4
# 132 "/usr/include/bits/floatn.h" 2 3 4
# 441 "/usr/include/stdio.h" 2 3 4




extern int fscanf (FILE *__restrict __stream, const char *__restrict __format, ...) __asm__ ("" "__isoc23_fscanf")

                                __attribute__ ((__nonnull__ (1)));
extern int scanf (const char *__restrict __format, ...) __asm__ ("" "__isoc23_scanf")
                              ;
extern int sscanf (const char *__restrict __s, const char *__restrict __format, ...) __asm__ ("" "__isoc23_sscanf") __attribute__ ((__nothrow__ , __leaf__))

                      ;
# 493 "/usr/include/stdio.h" 3 4
extern int vfscanf (FILE *__restrict __s, const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));





extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (const char *__restrict __s,
      const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format__ (__scanf__, 2, 0)));






extern int vfscanf (FILE *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vfscanf")



     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));
extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vscanf")

     __attribute__ ((__format__ (__scanf__, 1, 0))) ;
extern int vsscanf (const char *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vsscanf") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__format__ (__scanf__, 2, 0)));
# 578 "/usr/include/stdio.h" 3 4
extern int fgetc (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getc (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern int getchar (void);






extern int getc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getchar_unlocked (void);
# 603 "/usr/include/stdio.h" 3 4
extern int fgetc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));







extern int fputc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));





extern int putchar (int __c);
# 627 "/usr/include/stdio.h" 3 4
extern int fputc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern int putc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream) __attribute__ ((__nonnull__ (1)));


extern int putw (int __w, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 677 "/usr/include/stdio.h" 3 4
extern char *fgets_unlocked (char *__restrict __s, int __n,
        FILE *__restrict __stream)
    __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 689 "/usr/include/stdio.h" 3 4
extern __ssize_t __getdelim (char **__restrict __lineptr,
                             size_t *__restrict __n, int __delimiter,
                             FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));
extern __ssize_t getdelim (char **__restrict __lineptr,
                           size_t *__restrict __n, int __delimiter,
                           FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));


extern __ssize_t getline (char **__restrict __lineptr,
                          size_t *__restrict __n,
                          FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));







extern int fputs (const char *__restrict __s, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (2)));





extern int puts (const char *__s);






extern int ungetc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));




extern size_t fwrite (const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s) __attribute__ ((__nonnull__ (4)));
# 745 "/usr/include/stdio.h" 3 4
extern int fputs_unlocked (const char *__restrict __s,
      FILE *__restrict __stream) __attribute__ ((__nonnull__ (2)));
# 756 "/usr/include/stdio.h" 3 4
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));
extern size_t fwrite_unlocked (const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));







extern int fseek (FILE *__stream, long int __off, int __whence)
  __attribute__ ((__nonnull__ (1)));




extern long int ftell (FILE *__stream) __attribute__ ((__nonnull__ (1)));




extern void rewind (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 802 "/usr/include/stdio.h" 3 4
extern int fseeko (FILE *__stream, __off64_t __off, int __whence) __asm__ ("" "fseeko64")

                   __attribute__ ((__nonnull__ (1)));
extern __off64_t ftello (FILE *__stream) __asm__ ("" "ftello64")
  __attribute__ ((__nonnull__ (1)));
# 828 "/usr/include/stdio.h" 3 4
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos) __asm__ ("" "fgetpos64")

  __attribute__ ((__nonnull__ (1)));
extern int fsetpos (FILE *__stream, const fpos_t *__pos) __asm__ ("" "fsetpos64")

  __attribute__ ((__nonnull__ (1)));







extern int fseeko64 (FILE *__stream, __off64_t __off, int __whence)
  __attribute__ ((__nonnull__ (1)));
extern __off64_t ftello64 (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int fgetpos64 (FILE *__restrict __stream, fpos64_t *__restrict __pos)
  __attribute__ ((__nonnull__ (1)));
extern int fsetpos64 (FILE *__stream, const fpos64_t *__pos) __attribute__ ((__nonnull__ (1)));



extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern void perror (const char *__s) __attribute__ ((__cold__));




extern int fileno (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 887 "/usr/include/stdio.h" 3 4
extern int pclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern FILE *popen (const char *__command, const char *__modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (pclose, 1))) ;






extern char *ctermid (char *__s) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1)));





extern char *cuserid (char *__s)
  __attribute__ ((__access__ (__write_only__, 1)));




struct obstack;


extern int obstack_printf (struct obstack *__restrict __obstack,
      const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3)));
extern int obstack_vprintf (struct obstack *__restrict __obstack,
       const char *__restrict __format,
       __gnuc_va_list __args)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0)));







extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 949 "/usr/include/stdio.h" 3 4
extern int __uflow (FILE *);
extern int __overflow (FILE *, int);
# 973 "/usr/include/stdio.h" 3 4

# 21 "./include/holy/osdep/hostfile_unix.h" 2


# 22 "./include/holy/osdep/hostfile_unix.h"
typedef struct dirent *holy_util_fd_dirent_t;
typedef DIR *holy_util_fd_dir_t;

static inline holy_util_fd_dir_t
holy_util_fd_opendir (const char *name)
{
  return opendir (name);
}

static inline void
holy_util_fd_closedir (holy_util_fd_dir_t dirp)
{
  closedir (dirp);
}

static inline holy_util_fd_dirent_t
holy_util_fd_readdir (holy_util_fd_dir_t dirp)
{
  return readdir (dirp);
}

static inline int
holy_util_unlink (const char *pathname)
{
  return unlink (pathname);
}

static inline int
holy_util_rmdir (const char *pathname)
{
  return rmdir (pathname);
}

static inline int
holy_util_rename (const char *from, const char *to)
{
  return rename (from, to);
}
# 70 "./include/holy/osdep/hostfile_unix.h"
enum holy_util_fd_open_flags_t
  {
    holy_UTIL_FD_O_RDONLY = 
# 72 "./include/holy/osdep/hostfile_unix.h" 3 4
                           00
# 72 "./include/holy/osdep/hostfile_unix.h"
                                   ,
    holy_UTIL_FD_O_WRONLY = 
# 73 "./include/holy/osdep/hostfile_unix.h" 3 4
                           01
# 73 "./include/holy/osdep/hostfile_unix.h"
                                   ,
    holy_UTIL_FD_O_RDWR = 
# 74 "./include/holy/osdep/hostfile_unix.h" 3 4
                         02
# 74 "./include/holy/osdep/hostfile_unix.h"
                               ,
    holy_UTIL_FD_O_CREATTRUNC = 
# 75 "./include/holy/osdep/hostfile_unix.h" 3 4
                               0100 
# 75 "./include/holy/osdep/hostfile_unix.h"
                                       | 
# 75 "./include/holy/osdep/hostfile_unix.h" 3 4
                                         01000
# 75 "./include/holy/osdep/hostfile_unix.h"
                                                ,
    holy_UTIL_FD_O_SYNC = (0

      | 
# 78 "./include/holy/osdep/hostfile_unix.h" 3 4
       04010000


      
# 81 "./include/holy/osdep/hostfile_unix.h"
     | 
# 81 "./include/holy/osdep/hostfile_unix.h" 3 4
       04010000

      
# 83 "./include/holy/osdep/hostfile_unix.h"
     )
  };



typedef int holy_util_fd_t;
# 12 "./include/holy/osdep/hostfile.h" 2
# 13 "./include/holy/emu/hostfile.h" 2

int
holy_util_is_directory (const char *path);
int
holy_util_is_special_file (const char *path);
int
holy_util_is_regular (const char *path);

char *
holy_util_path_concat (size_t n, ...);
char *
holy_util_path_concat_ext (size_t n, ...);

int
holy_util_fd_seek (holy_util_fd_t fd, holy_uint64_t off);
ssize_t
holy_util_fd_read (holy_util_fd_t fd, char *buf, size_t len);
ssize_t
holy_util_fd_write (holy_util_fd_t fd, const char *buf, size_t len);

holy_util_fd_t
holy_util_fd_open (const char *os_dev, int flags);
const char *
holy_util_fd_strerror (void);
void
holy_util_fd_sync (holy_util_fd_t fd);
void
holy_util_disable_fd_syncs (void);
void
holy_util_fd_close (holy_util_fd_t fd);

holy_uint64_t
holy_util_get_fd_size (holy_util_fd_t fd, const char *name, unsigned *log_secsize);
char *
holy_util_make_temporary_file (void);
char *
holy_util_make_temporary_dir (void);
void
holy_util_unlink_recursive (const char *name);
holy_uint32_t
holy_util_get_mtime (const char *name);
# 12 "./include/holy/emu/config.h" 2


const char *
holy_util_get_config_filename (void);
const char *
holy_util_get_pkgdatadir (void);
const char *
holy_util_get_pkglibdir (void);
const char *
holy_util_get_localedir (void);

struct holy_util_config
{
  int is_cryptodisk_enabled;
  char *holy_distributor;
};

void
holy_util_load_config (struct holy_util_config *cfg);

void
holy_util_parse_config (FILE *f, struct holy_util_config *cfg, int simple);
# 10 "./include/holy/types.h" 2
# 63 "./include/holy/types.h"
typedef signed char holy_int8_t;
typedef short holy_int16_t;
typedef int holy_int32_t;

typedef long holy_int64_t;




typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned holy_uint32_t;



typedef unsigned long holy_uint64_t;
# 90 "./include/holy/types.h"
typedef holy_uint64_t holy_addr_t;
typedef holy_uint64_t holy_size_t;
typedef holy_int64_t holy_ssize_t;
# 138 "./include/holy/types.h"
typedef holy_uint64_t holy_properly_aligned_t;




typedef holy_uint64_t holy_off_t;


typedef holy_uint64_t holy_disk_addr_t;


static inline holy_uint16_t holy_swap_bytes16(holy_uint16_t _x)
{
   return (holy_uint16_t) ((_x << 8) | (_x >> 8));
}
# 170 "./include/holy/types.h"
static inline holy_uint32_t holy_swap_bytes32(holy_uint32_t x)
{
 return __builtin_bswap32(x);
}

static inline holy_uint64_t holy_swap_bytes64(holy_uint64_t x)
{
 return __builtin_bswap64(x);
}
# 244 "./include/holy/types.h"
struct holy_unaligned_uint16
{
  holy_uint16_t val;
} __attribute__ ((packed));
struct holy_unaligned_uint32
{
  holy_uint32_t val;
} __attribute__ ((packed));
struct holy_unaligned_uint64
{
  holy_uint64_t val;
} __attribute__ ((packed));

typedef struct holy_unaligned_uint16 holy_unaligned_uint16_t;
typedef struct holy_unaligned_uint32 holy_unaligned_uint32_t;
typedef struct holy_unaligned_uint64 holy_unaligned_uint64_t;

static inline holy_uint16_t holy_get_unaligned16 (const void *ptr)
{
  const struct holy_unaligned_uint16 *dd
    = (const struct holy_unaligned_uint16 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned16 (void *ptr, holy_uint16_t val)
{
  struct holy_unaligned_uint16 *dd = (struct holy_unaligned_uint16 *) ptr;
  dd->val = val;
}

static inline holy_uint32_t holy_get_unaligned32 (const void *ptr)
{
  const struct holy_unaligned_uint32 *dd
    = (const struct holy_unaligned_uint32 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned32 (void *ptr, holy_uint32_t val)
{
  struct holy_unaligned_uint32 *dd = (struct holy_unaligned_uint32 *) ptr;
  dd->val = val;
}

static inline holy_uint64_t holy_get_unaligned64 (const void *ptr)
{
  const struct holy_unaligned_uint64 *dd
    = (const struct holy_unaligned_uint64 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned64 (void *ptr, holy_uint64_t val)
{
  struct holy_unaligned_uint64_t
  {
    holy_uint64_t d;
  } __attribute__ ((packed));
  struct holy_unaligned_uint64_t *dd = (struct holy_unaligned_uint64_t *) ptr;
  dd->d = val;
}
# 8 "util/holy-fstest.c" 2
# 1 "./include/holy/emu/misc.h" 1
# 10 "./include/holy/emu/misc.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 11 "./include/holy/emu/misc.h" 2



# 1 "./include/holy/gcrypt/gcrypt.h" 1





# 1 "./include/holy/gcrypt/gpg-error.h" 1
# 9 "./include/holy/gcrypt/gpg-error.h"
# 1 "./include/holy/crypto.h" 1
# 14 "./include/holy/crypto.h"
typedef enum
  {
    GPG_ERR_NO_ERROR,
    GPG_ERR_BAD_MPI,
    GPG_ERR_BAD_SECKEY,
    GPG_ERR_BAD_SIGNATURE,
    GPG_ERR_CIPHER_ALGO,
    GPG_ERR_CONFLICT,
    GPG_ERR_DECRYPT_FAILED,
    GPG_ERR_DIGEST_ALGO,
    GPG_ERR_GENERAL,
    GPG_ERR_INTERNAL,
    GPG_ERR_INV_ARG,
    GPG_ERR_INV_CIPHER_MODE,
    GPG_ERR_INV_FLAG,
    GPG_ERR_INV_KEYLEN,
    GPG_ERR_INV_OBJ,
    GPG_ERR_INV_OP,
    GPG_ERR_INV_SEXP,
    GPG_ERR_INV_VALUE,
    GPG_ERR_MISSING_VALUE,
    GPG_ERR_NO_ENCRYPTION_SCHEME,
    GPG_ERR_NO_OBJ,
    GPG_ERR_NO_PRIME,
    GPG_ERR_NO_SIGNATURE_SCHEME,
    GPG_ERR_NOT_FOUND,
    GPG_ERR_NOT_IMPLEMENTED,
    GPG_ERR_NOT_SUPPORTED,
    GPG_ERROR_CFLAGS,
    GPG_ERR_PUBKEY_ALGO,
    GPG_ERR_SELFTEST_FAILED,
    GPG_ERR_TOO_SHORT,
    GPG_ERR_UNSUPPORTED,
    GPG_ERR_WEAK_KEY,
    GPG_ERR_WRONG_KEY_USAGE,
    GPG_ERR_WRONG_PUBKEY_ALGO,
    GPG_ERR_OUT_OF_MEMORY,
    GPG_ERR_TOO_LARGE,
    GPG_ERR_ENOMEM
  } gpg_err_code_t;
typedef gpg_err_code_t gpg_error_t;
typedef gpg_error_t gcry_error_t;
typedef gpg_err_code_t gcry_err_code_t;
# 77 "./include/holy/crypto.h"
typedef gcry_err_code_t (*gcry_cipher_setkey_t) (void *c,
       const unsigned char *key,
       unsigned keylen);


typedef void (*gcry_cipher_encrypt_t) (void *c,
           unsigned char *outbuf,
           const unsigned char *inbuf);


typedef void (*gcry_cipher_decrypt_t) (void *c,
           unsigned char *outbuf,
           const unsigned char *inbuf);


typedef void (*gcry_cipher_stencrypt_t) (void *c,
      unsigned char *outbuf,
      const unsigned char *inbuf,
      unsigned int n);


typedef void (*gcry_cipher_stdecrypt_t) (void *c,
      unsigned char *outbuf,
      const unsigned char *inbuf,
      unsigned int n);

typedef struct gcry_cipher_oid_spec
{
  const char *oid;
  int mode;
} gcry_cipher_oid_spec_t;


typedef struct gcry_cipher_spec
{
  const char *name;
  const char **aliases;
  gcry_cipher_oid_spec_t *oids;
  holy_size_t blocksize;
  holy_size_t keylen;
  holy_size_t contextsize;
  gcry_cipher_setkey_t setkey;
  gcry_cipher_encrypt_t encrypt;
  gcry_cipher_decrypt_t decrypt;
  gcry_cipher_stencrypt_t stencrypt;
  gcry_cipher_stdecrypt_t stdecrypt;

  const char *modname;

  struct gcry_cipher_spec *next;
} gcry_cipher_spec_t;


typedef void (*gcry_md_init_t) (void *c);


typedef void (*gcry_md_write_t) (void *c, const void *buf, holy_size_t nbytes);


typedef void (*gcry_md_final_t) (void *c);


typedef unsigned char *(*gcry_md_read_t) (void *c);

typedef struct gcry_md_oid_spec
{
  const char *oidstring;
} gcry_md_oid_spec_t;


typedef struct gcry_md_spec
{
  const char *name;
  unsigned char *asnoid;
  int asnlen;
  gcry_md_oid_spec_t *oids;
  holy_size_t mdlen;
  gcry_md_init_t init;
  gcry_md_write_t write;
  gcry_md_final_t final;
  gcry_md_read_t read;
  holy_size_t contextsize;

  holy_size_t blocksize;

  const char *modname;

  struct gcry_md_spec *next;
} gcry_md_spec_t;

struct gcry_mpi;
typedef struct gcry_mpi *gcry_mpi_t;


typedef gcry_err_code_t (*gcry_pk_generate_t) (int algo,
            unsigned int nbits,
            unsigned long use_e,
            gcry_mpi_t *skey,
            gcry_mpi_t **retfactors);


typedef gcry_err_code_t (*gcry_pk_check_secret_key_t) (int algo,
             gcry_mpi_t *skey);


typedef gcry_err_code_t (*gcry_pk_encrypt_t) (int algo,
           gcry_mpi_t *resarr,
           gcry_mpi_t data,
           gcry_mpi_t *pkey,
           int flags);


typedef gcry_err_code_t (*gcry_pk_decrypt_t) (int algo,
           gcry_mpi_t *result,
           gcry_mpi_t *data,
           gcry_mpi_t *skey,
           int flags);


typedef gcry_err_code_t (*gcry_pk_sign_t) (int algo,
        gcry_mpi_t *resarr,
        gcry_mpi_t data,
        gcry_mpi_t *skey);


typedef gcry_err_code_t (*gcry_pk_verify_t) (int algo,
          gcry_mpi_t hash,
          gcry_mpi_t *data,
          gcry_mpi_t *pkey,
          int (*cmp) (void *, gcry_mpi_t),
          void *opaquev);


typedef unsigned (*gcry_pk_get_nbits_t) (int algo, gcry_mpi_t *pkey);


typedef struct gcry_pk_spec
{
  const char *name;
  const char **aliases;
  const char *elements_pkey;
  const char *elements_skey;
  const char *elements_enc;
  const char *elements_sig;
  const char *elements_grip;
  int use;
  gcry_pk_generate_t generate;
  gcry_pk_check_secret_key_t check_secret_key;
  gcry_pk_encrypt_t encrypt;
  gcry_pk_decrypt_t decrypt;
  gcry_pk_sign_t sign;
  gcry_pk_verify_t verify;
  gcry_pk_get_nbits_t get_nbits;

  const char *modname;

} gcry_pk_spec_t;

struct holy_crypto_cipher_handle
{
  const struct gcry_cipher_spec *cipher;
  char ctx[0];
};

typedef struct holy_crypto_cipher_handle *holy_crypto_cipher_handle_t;

struct holy_crypto_hmac_handle;

const gcry_cipher_spec_t *
holy_crypto_lookup_cipher_by_name (const char *name);

holy_crypto_cipher_handle_t
holy_crypto_cipher_open (const struct gcry_cipher_spec *cipher);

gcry_err_code_t
holy_crypto_cipher_set_key (holy_crypto_cipher_handle_t cipher,
       const unsigned char *key,
       unsigned keylen);

static inline void
holy_crypto_cipher_close (holy_crypto_cipher_handle_t cipher)
{
  holy_free (cipher);
}

static inline void
holy_crypto_xor (void *out, const void *in1, const void *in2, holy_size_t size)
{
  const holy_uint8_t *in1ptr = in1, *in2ptr = in2;
  holy_uint8_t *outptr = out;
  while (size && (((holy_addr_t) in1ptr & (sizeof (holy_uint64_t) - 1))
    || ((holy_addr_t) in2ptr & (sizeof (holy_uint64_t) - 1))
    || ((holy_addr_t) outptr & (sizeof (holy_uint64_t) - 1))))
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
      size--;
    }
  while (size >= sizeof (holy_uint64_t))
    {

      *(holy_uint64_t *) (void *) outptr
 = (*(const holy_uint64_t *) (const void *) in1ptr
    ^ *(const holy_uint64_t *) (const void *) in2ptr);
      in1ptr += sizeof (holy_uint64_t);
      in2ptr += sizeof (holy_uint64_t);
      outptr += sizeof (holy_uint64_t);
      size -= sizeof (holy_uint64_t);
    }
  while (size)
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
      size--;
    }
}

gcry_err_code_t
holy_crypto_ecb_decrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size);

gcry_err_code_t
holy_crypto_ecb_encrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size);
gcry_err_code_t
holy_crypto_cbc_encrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size,
    void *iv_in);
gcry_err_code_t
holy_crypto_cbc_decrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size,
    void *iv);
void
holy_cipher_register (gcry_cipher_spec_t *cipher);
void
holy_cipher_unregister (gcry_cipher_spec_t *cipher);
void
holy_md_register (gcry_md_spec_t *digest);
void
holy_md_unregister (gcry_md_spec_t *cipher);

extern struct gcry_pk_spec *holy_crypto_pk_dsa;
extern struct gcry_pk_spec *holy_crypto_pk_ecdsa;
extern struct gcry_pk_spec *holy_crypto_pk_ecdh;
extern struct gcry_pk_spec *holy_crypto_pk_rsa;

void
holy_crypto_hash (const gcry_md_spec_t *hash, void *out, const void *in,
    holy_size_t inlen);
const gcry_md_spec_t *
holy_crypto_lookup_md_by_name (const char *name);

holy_err_t
holy_crypto_gcry_error (gcry_err_code_t in);

void holy_burn_stack (holy_size_t size);

struct holy_crypto_hmac_handle *
holy_crypto_hmac_init (const struct gcry_md_spec *md,
         const void *key, holy_size_t keylen);
void
holy_crypto_hmac_write (struct holy_crypto_hmac_handle *hnd,
   const void *data,
   holy_size_t datalen);
gcry_err_code_t
holy_crypto_hmac_fini (struct holy_crypto_hmac_handle *hnd, void *out);

gcry_err_code_t
holy_crypto_hmac_buffer (const struct gcry_md_spec *md,
    const void *key, holy_size_t keylen,
    const void *data, holy_size_t datalen, void *out);

extern gcry_md_spec_t _gcry_digest_spec_md5;
extern gcry_md_spec_t _gcry_digest_spec_sha1;
extern gcry_md_spec_t _gcry_digest_spec_sha256;
extern gcry_md_spec_t _gcry_digest_spec_sha512;
extern gcry_md_spec_t _gcry_digest_spec_crc32;
extern gcry_cipher_spec_t _gcry_cipher_spec_aes;
# 372 "./include/holy/crypto.h"
gcry_err_code_t
holy_crypto_pbkdf2 (const struct gcry_md_spec *md,
      const holy_uint8_t *P, holy_size_t Plen,
      const holy_uint8_t *S, holy_size_t Slen,
      unsigned int c,
      holy_uint8_t *DK, holy_size_t dkLen);

int
holy_crypto_memcmp (const void *a, const void *b, holy_size_t n);

int
holy_password_get (char buf[], unsigned buf_size);




extern void (*holy_crypto_autoload_hook) (const char *name);

void _gcry_assert_failed (const char *expr, const char *file, int line,
                          const char *func) __attribute__ ((noreturn));

void _gcry_burn_stack (int bytes);
void _gcry_log_error( const char *fmt, ... ) __attribute__ ((format (__printf__, 1, 2)));



void holy_gcry_init_all (void);
void holy_gcry_fini_all (void);

int
holy_get_random (void *out, holy_size_t len);
# 10 "./include/holy/gcrypt/gpg-error.h" 2
typedef enum
  {
    GPG_ERR_SOURCE_USER_1
  }
  gpg_err_source_t;

static inline int
gpg_err_make (gpg_err_source_t source __attribute__ ((unused)), gpg_err_code_t code)
{
  return code;
}

static inline gpg_err_code_t
gpg_err_code (gpg_error_t err)
{
  return err;
}

static inline gpg_err_source_t
gpg_err_source (gpg_error_t err __attribute__ ((unused)))
{
  return GPG_ERR_SOURCE_USER_1;
}

gcry_err_code_t
gpg_error_from_syserror (void);
# 7 "./include/holy/gcrypt/gcrypt.h" 2
# 90 "./include/holy/gcrypt/gcrypt.h"
typedef gpg_err_source_t gcry_err_source_t;

static inline gcry_err_code_t
gcry_err_make (gcry_err_source_t source, gcry_err_code_t code)
{
  return gpg_err_make (source, code);
}







static inline gcry_err_code_t
gcry_error (gcry_err_code_t code)
{
  return gcry_err_make (GPG_ERR_SOURCE_USER_1, code);
}

static inline gcry_err_code_t
gcry_err_code (gcry_err_code_t err)
{
  return gpg_err_code (err);
}


static inline gcry_err_source_t
gcry_err_source (gcry_err_code_t err)
{
  return gpg_err_source (err);
}



const char *gcry_strerror (gcry_err_code_t err);



const char *gcry_strsource (gcry_err_code_t err);




gcry_err_code_t gcry_err_code_from_errno (int err);



int gcry_err_code_to_errno (gcry_err_code_t code);



gcry_err_code_t gcry_err_make_from_errno (gcry_err_source_t source, int err);


gcry_err_code_t gcry_error_from_errno (int err);



struct gcry_mpi;
# 159 "./include/holy/gcrypt/gcrypt.h"
const char *gcry_check_version (const char *req_version);




enum gcry_ctl_cmds
  {
    GCRYCTL_SET_KEY = 1,
    GCRYCTL_SET_IV = 2,
    GCRYCTL_CFB_SYNC = 3,
    GCRYCTL_RESET = 4,
    GCRYCTL_FINALIZE = 5,
    GCRYCTL_GET_KEYLEN = 6,
    GCRYCTL_GET_BLKLEN = 7,
    GCRYCTL_TEST_ALGO = 8,
    GCRYCTL_IS_SECURE = 9,
    GCRYCTL_GET_ASNOID = 10,
    GCRYCTL_ENABLE_ALGO = 11,
    GCRYCTL_DISABLE_ALGO = 12,
    GCRYCTL_DUMP_RANDOM_STATS = 13,
    GCRYCTL_DUMP_SECMEM_STATS = 14,
    GCRYCTL_GET_ALGO_NPKEY = 15,
    GCRYCTL_GET_ALGO_NSKEY = 16,
    GCRYCTL_GET_ALGO_NSIGN = 17,
    GCRYCTL_GET_ALGO_NENCR = 18,
    GCRYCTL_SET_VERBOSITY = 19,
    GCRYCTL_SET_DEBUG_FLAGS = 20,
    GCRYCTL_CLEAR_DEBUG_FLAGS = 21,
    GCRYCTL_USE_SECURE_RNDPOOL= 22,
    GCRYCTL_DUMP_MEMORY_STATS = 23,
    GCRYCTL_INIT_SECMEM = 24,
    GCRYCTL_TERM_SECMEM = 25,
    GCRYCTL_DISABLE_SECMEM_WARN = 27,
    GCRYCTL_SUSPEND_SECMEM_WARN = 28,
    GCRYCTL_RESUME_SECMEM_WARN = 29,
    GCRYCTL_DROP_PRIVS = 30,
    GCRYCTL_ENABLE_M_GUARD = 31,
    GCRYCTL_START_DUMP = 32,
    GCRYCTL_STOP_DUMP = 33,
    GCRYCTL_GET_ALGO_USAGE = 34,
    GCRYCTL_IS_ALGO_ENABLED = 35,
    GCRYCTL_DISABLE_INTERNAL_LOCKING = 36,
    GCRYCTL_DISABLE_SECMEM = 37,
    GCRYCTL_INITIALIZATION_FINISHED = 38,
    GCRYCTL_INITIALIZATION_FINISHED_P = 39,
    GCRYCTL_ANY_INITIALIZATION_P = 40,
    GCRYCTL_SET_CBC_CTS = 41,
    GCRYCTL_SET_CBC_MAC = 42,
    GCRYCTL_SET_CTR = 43,
    GCRYCTL_ENABLE_QUICK_RANDOM = 44,
    GCRYCTL_SET_RANDOM_SEED_FILE = 45,
    GCRYCTL_UPDATE_RANDOM_SEED_FILE = 46,
    GCRYCTL_SET_THREAD_CBS = 47,
    GCRYCTL_FAST_POLL = 48,
    GCRYCTL_SET_RANDOM_DAEMON_SOCKET = 49,
    GCRYCTL_USE_RANDOM_DAEMON = 50,
    GCRYCTL_FAKED_RANDOM_P = 51,
    GCRYCTL_SET_RNDEGD_SOCKET = 52,
    GCRYCTL_PRINT_CONFIG = 53,
    GCRYCTL_OPERATIONAL_P = 54,
    GCRYCTL_FIPS_MODE_P = 55,
    GCRYCTL_FORCE_FIPS_MODE = 56,
    GCRYCTL_SELFTEST = 57,

    GCRYCTL_DISABLE_HWF = 63,
    GCRYCTL_SET_ENFORCED_FIPS_FLAG = 64
  };


gcry_err_code_t gcry_control (enum gcry_ctl_cmds CMD, ...);






struct gcry_sexp;
typedef struct gcry_sexp *gcry_sexp_t;







enum gcry_sexp_format
  {
    GCRYSEXP_FMT_DEFAULT = 0,
    GCRYSEXP_FMT_CANON = 1,
    GCRYSEXP_FMT_BASE64 = 2,
    GCRYSEXP_FMT_ADVANCED = 3
  };




gcry_err_code_t gcry_sexp_new (gcry_sexp_t *retsexp,
                            const void *buffer, size_t length,
                            int autodetect);



gcry_err_code_t gcry_sexp_create (gcry_sexp_t *retsexp,
                               void *buffer, size_t length,
                               int autodetect, void (*freefnc) (void *));



gcry_err_code_t gcry_sexp_sscan (gcry_sexp_t *retsexp, size_t *erroff,
                              const char *buffer, size_t length);



gcry_err_code_t gcry_sexp_build (gcry_sexp_t *retsexp, size_t *erroff,
                              const char *format, ...);



gcry_err_code_t gcry_sexp_build_array (gcry_sexp_t *retsexp, size_t *erroff,
        const char *format, void **arg_list);


void gcry_sexp_release (gcry_sexp_t sexp);



size_t gcry_sexp_canon_len (const unsigned char *buffer, size_t length,
                            size_t *erroff, gcry_err_code_t *errcode);



size_t gcry_sexp_sprint (gcry_sexp_t sexp, int mode, void *buffer,
                         size_t maxlength);



void gcry_sexp_dump (const gcry_sexp_t a);

gcry_sexp_t gcry_sexp_cons (const gcry_sexp_t a, const gcry_sexp_t b);
gcry_sexp_t gcry_sexp_alist (const gcry_sexp_t *array);
gcry_sexp_t gcry_sexp_vlist (const gcry_sexp_t a, ...);
gcry_sexp_t gcry_sexp_append (const gcry_sexp_t a, const gcry_sexp_t n);
gcry_sexp_t gcry_sexp_prepend (const gcry_sexp_t a, const gcry_sexp_t n);






gcry_sexp_t gcry_sexp_find_token (gcry_sexp_t list,
                                const char *tok, size_t toklen);


int gcry_sexp_length (const gcry_sexp_t list);




gcry_sexp_t gcry_sexp_nth (const gcry_sexp_t list, int number);




gcry_sexp_t gcry_sexp_car (const gcry_sexp_t list);






gcry_sexp_t gcry_sexp_cdr (const gcry_sexp_t list);

gcry_sexp_t gcry_sexp_cadr (const gcry_sexp_t list);
# 340 "./include/holy/gcrypt/gcrypt.h"
const char *gcry_sexp_nth_data (const gcry_sexp_t list, int number,
                                size_t *datalen);






char *gcry_sexp_nth_string (gcry_sexp_t list, int number);







gcry_mpi_t gcry_sexp_nth_mpi (gcry_sexp_t list, int number, int mpifmt);
# 367 "./include/holy/gcrypt/gcrypt.h"
enum gcry_mpi_format
  {
    GCRYMPI_FMT_NONE= 0,
    GCRYMPI_FMT_STD = 1,
    GCRYMPI_FMT_PGP = 2,
    GCRYMPI_FMT_SSH = 3,
    GCRYMPI_FMT_HEX = 4,
    GCRYMPI_FMT_USG = 5
  };


enum gcry_mpi_flag
  {
    GCRYMPI_FLAG_SECURE = 1,
    GCRYMPI_FLAG_OPAQUE = 2


  };




gcry_mpi_t gcry_mpi_new (unsigned int nbits);


gcry_mpi_t gcry_mpi_snew (unsigned int nbits);


void gcry_mpi_release (gcry_mpi_t a);


gcry_mpi_t gcry_mpi_copy (const gcry_mpi_t a);


gcry_mpi_t gcry_mpi_set (gcry_mpi_t w, const gcry_mpi_t u);


gcry_mpi_t gcry_mpi_set_ui (gcry_mpi_t w, unsigned long u);


void gcry_mpi_swap (gcry_mpi_t a, gcry_mpi_t b);



int gcry_mpi_cmp (const gcry_mpi_t u, const gcry_mpi_t v);




int gcry_mpi_cmp_ui (const gcry_mpi_t u, unsigned long v);





gcry_err_code_t gcry_mpi_scan (gcry_mpi_t *ret_mpi, enum gcry_mpi_format format,
                            const void *buffer, size_t buflen,
                            size_t *nscanned);






gcry_err_code_t gcry_mpi_print (enum gcry_mpi_format format,
                             unsigned char *buffer, size_t buflen,
                             size_t *nwritten,
                             const gcry_mpi_t a);





gcry_err_code_t gcry_mpi_aprint (enum gcry_mpi_format format,
                              unsigned char **buffer, size_t *nwritten,
                              const gcry_mpi_t a);





void gcry_mpi_dump (const gcry_mpi_t a);



void gcry_mpi_add (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_add_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v);


void gcry_mpi_addm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_sub (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_sub_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v );


void gcry_mpi_subm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_mul (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_mul_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v );


void gcry_mpi_mulm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_mul_2exp (gcry_mpi_t w, gcry_mpi_t u, unsigned long cnt);



void gcry_mpi_div (gcry_mpi_t q, gcry_mpi_t r,
                   gcry_mpi_t dividend, gcry_mpi_t divisor, int round);


void gcry_mpi_mod (gcry_mpi_t r, gcry_mpi_t dividend, gcry_mpi_t divisor);


void gcry_mpi_powm (gcry_mpi_t w,
                    const gcry_mpi_t b, const gcry_mpi_t e,
                    const gcry_mpi_t m);



int gcry_mpi_gcd (gcry_mpi_t g, gcry_mpi_t a, gcry_mpi_t b);



int gcry_mpi_invm (gcry_mpi_t x, gcry_mpi_t a, gcry_mpi_t m);



unsigned int gcry_mpi_get_nbits (gcry_mpi_t a);


int gcry_mpi_test_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_set_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_clear_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_set_highbit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_clear_highbit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_rshift (gcry_mpi_t x, gcry_mpi_t a, unsigned int n);


void gcry_mpi_lshift (gcry_mpi_t x, gcry_mpi_t a, unsigned int n);




gcry_mpi_t gcry_mpi_set_opaque (gcry_mpi_t a, void *p, unsigned int nbits);




void *gcry_mpi_get_opaque (gcry_mpi_t a, unsigned int *nbits);




void gcry_mpi_set_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);



void gcry_mpi_clear_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);


int gcry_mpi_get_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);
# 607 "./include/holy/gcrypt/gcrypt.h"
struct gcry_cipher_handle;
typedef struct gcry_cipher_handle *gcry_cipher_hd_t;
# 617 "./include/holy/gcrypt/gcrypt.h"
enum gcry_cipher_algos
  {
    GCRY_CIPHER_NONE = 0,
    GCRY_CIPHER_IDEA = 1,
    GCRY_CIPHER_3DES = 2,
    GCRY_CIPHER_CAST5 = 3,
    GCRY_CIPHER_BLOWFISH = 4,
    GCRY_CIPHER_SAFER_SK128 = 5,
    GCRY_CIPHER_DES_SK = 6,
    GCRY_CIPHER_AES = 7,
    GCRY_CIPHER_AES192 = 8,
    GCRY_CIPHER_AES256 = 9,
    GCRY_CIPHER_TWOFISH = 10,


    GCRY_CIPHER_ARCFOUR = 301,
    GCRY_CIPHER_DES = 302,
    GCRY_CIPHER_TWOFISH128 = 303,
    GCRY_CIPHER_SERPENT128 = 304,
    GCRY_CIPHER_SERPENT192 = 305,
    GCRY_CIPHER_SERPENT256 = 306,
    GCRY_CIPHER_RFC2268_40 = 307,
    GCRY_CIPHER_RFC2268_128 = 308,
    GCRY_CIPHER_SEED = 309,
    GCRY_CIPHER_CAMELLIA128 = 310,
    GCRY_CIPHER_CAMELLIA192 = 311,
    GCRY_CIPHER_CAMELLIA256 = 312
  };
# 655 "./include/holy/gcrypt/gcrypt.h"
enum gcry_cipher_modes
  {
    GCRY_CIPHER_MODE_NONE = 0,
    GCRY_CIPHER_MODE_ECB = 1,
    GCRY_CIPHER_MODE_CFB = 2,
    GCRY_CIPHER_MODE_CBC = 3,
    GCRY_CIPHER_MODE_STREAM = 4,
    GCRY_CIPHER_MODE_OFB = 5,
    GCRY_CIPHER_MODE_CTR = 6,
    GCRY_CIPHER_MODE_AESWRAP= 7
  };


enum gcry_cipher_flags
  {
    GCRY_CIPHER_SECURE = 1,
    GCRY_CIPHER_ENABLE_SYNC = 2,
    GCRY_CIPHER_CBC_CTS = 4,
    GCRY_CIPHER_CBC_MAC = 8
  };




gcry_err_code_t gcry_cipher_open (gcry_cipher_hd_t *handle,
                              int algo, int mode, unsigned int flags);


void gcry_cipher_close (gcry_cipher_hd_t h);


gcry_err_code_t gcry_cipher_ctl (gcry_cipher_hd_t h, int cmd, void *buffer,
                             size_t buflen);


gcry_err_code_t gcry_cipher_info (gcry_cipher_hd_t h, int what, void *buffer,
                              size_t *nbytes);


gcry_err_code_t gcry_cipher_algo_info (int algo, int what, void *buffer,
                                   size_t *nbytes);




const char *gcry_cipher_algo_name (int algorithm) __attribute__ ((__pure__));



int gcry_cipher_map_name (const char *name) __attribute__ ((__pure__));




int gcry_cipher_mode_from_oid (const char *string) __attribute__ ((__pure__));





gcry_err_code_t gcry_cipher_encrypt (gcry_cipher_hd_t h,
                                  void *out, size_t outsize,
                                  const void *in, size_t inlen);


gcry_err_code_t gcry_cipher_decrypt (gcry_cipher_hd_t h,
                                  void *out, size_t outsize,
                                  const void *in, size_t inlen);


gcry_err_code_t gcry_cipher_setkey (gcry_cipher_hd_t hd,
                                 const void *key, size_t keylen);



gcry_err_code_t gcry_cipher_setiv (gcry_cipher_hd_t hd,
                                const void *iv, size_t ivlen);
# 747 "./include/holy/gcrypt/gcrypt.h"
gpg_error_t gcry_cipher_setctr (gcry_cipher_hd_t hd,
                                const void *ctr, size_t ctrlen);


size_t gcry_cipher_get_algo_keylen (int algo);


size_t gcry_cipher_get_algo_blklen (int algo);
# 766 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_cipher_list (int *list, int *list_length);
# 776 "./include/holy/gcrypt/gcrypt.h"
enum gcry_pk_algos
  {
    GCRY_PK_RSA = 1,
    GCRY_PK_RSA_E = 2,
    GCRY_PK_RSA_S = 3,
    GCRY_PK_ELG_E = 16,
    GCRY_PK_DSA = 17,
    GCRY_PK_ELG = 20,
    GCRY_PK_ECDSA = 301,
    GCRY_PK_ECDH = 302
  };
# 797 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_pk_encrypt (gcry_sexp_t *result,
                              gcry_sexp_t data, gcry_sexp_t pkey);



gcry_err_code_t gcry_pk_decrypt (gcry_sexp_t *result,
                              gcry_sexp_t data, gcry_sexp_t skey);



gcry_err_code_t gcry_pk_sign (gcry_sexp_t *result,
                           gcry_sexp_t data, gcry_sexp_t skey);


gcry_err_code_t gcry_pk_verify (gcry_sexp_t sigval,
                             gcry_sexp_t data, gcry_sexp_t pkey);


gcry_err_code_t gcry_pk_testkey (gcry_sexp_t key);




gcry_err_code_t gcry_pk_genkey (gcry_sexp_t *r_key, gcry_sexp_t s_parms);


gcry_err_code_t gcry_pk_ctl (int cmd, void *buffer, size_t buflen);


gcry_err_code_t gcry_pk_algo_info (int algo, int what,
                                void *buffer, size_t *nbytes);




const char *gcry_pk_algo_name (int algorithm) __attribute__ ((__pure__));



int gcry_pk_map_name (const char* name) __attribute__ ((__pure__));



unsigned int gcry_pk_get_nbits (gcry_sexp_t key) __attribute__ ((__pure__));



unsigned char *gcry_pk_get_keygrip (gcry_sexp_t key, unsigned char *array);


const char *gcry_pk_get_curve (gcry_sexp_t key, int iterator,
                               unsigned int *r_nbits);



gcry_sexp_t gcry_pk_get_param (int algo, const char *name);
# 864 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_pk_list (int *list, int *list_length);
# 876 "./include/holy/gcrypt/gcrypt.h"
enum gcry_md_algos
  {
    GCRY_MD_NONE = 0,
    GCRY_MD_MD5 = 1,
    GCRY_MD_SHA1 = 2,
    GCRY_MD_RMD160 = 3,
    GCRY_MD_MD2 = 5,
    GCRY_MD_TIGER = 6,
    GCRY_MD_HAVAL = 7,
    GCRY_MD_SHA256 = 8,
    GCRY_MD_SHA384 = 9,
    GCRY_MD_SHA512 = 10,
    GCRY_MD_SHA224 = 11,
    GCRY_MD_MD4 = 301,
    GCRY_MD_CRC32 = 302,
    GCRY_MD_CRC32_RFC1510 = 303,
    GCRY_MD_CRC24_RFC2440 = 304,
    GCRY_MD_WHIRLPOOL = 305,
    GCRY_MD_TIGER1 = 306,
    GCRY_MD_TIGER2 = 307
  };


enum gcry_md_flags
  {
    GCRY_MD_FLAG_SECURE = 1,
    GCRY_MD_FLAG_HMAC = 2
  };


struct gcry_md_context;




typedef struct gcry_md_handle
{

  struct gcry_md_context *ctx;


  int bufpos;
  int bufsize;
  unsigned char buf[1];
} *gcry_md_hd_t;
# 932 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_md_open (gcry_md_hd_t *h, int algo, unsigned int flags);


void gcry_md_close (gcry_md_hd_t hd);


gcry_err_code_t gcry_md_enable (gcry_md_hd_t hd, int algo);


gcry_err_code_t gcry_md_copy (gcry_md_hd_t *bhd, gcry_md_hd_t ahd);


void gcry_md_reset (gcry_md_hd_t hd);


gcry_err_code_t gcry_md_ctl (gcry_md_hd_t hd, int cmd,
                          void *buffer, size_t buflen);




void gcry_md_write (gcry_md_hd_t hd, const void *buffer, size_t length);



unsigned char *gcry_md_read (gcry_md_hd_t hd, int algo);






void gcry_md_hash_buffer (int algo, void *digest,
                          const void *buffer, size_t length);



int gcry_md_get_algo (gcry_md_hd_t hd);



unsigned int gcry_md_get_algo_dlen (int algo);



int gcry_md_is_enabled (gcry_md_hd_t a, int algo);


int gcry_md_is_secure (gcry_md_hd_t a);


gcry_err_code_t gcry_md_info (gcry_md_hd_t h, int what, void *buffer,
                          size_t *nbytes);


gcry_err_code_t gcry_md_algo_info (int algo, int what, void *buffer,
                               size_t *nbytes);




const char *gcry_md_algo_name (int algo) __attribute__ ((__pure__));



int gcry_md_map_name (const char* name) __attribute__ ((__pure__));



gcry_err_code_t gcry_md_setkey (gcry_md_hd_t hd, const void *key, size_t keylen);




void gcry_md_debug (gcry_md_hd_t hd, const char *suffix);
# 1055 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_md_list (int *list, int *list_length);
# 1476 "./include/holy/gcrypt/gcrypt.h"
enum gcry_kdf_algos
  {
    GCRY_KDF_NONE = 0,
    GCRY_KDF_SIMPLE_S2K = 16,
    GCRY_KDF_SALTED_S2K = 17,
    GCRY_KDF_ITERSALTED_S2K = 19,
    GCRY_KDF_PBKDF1 = 33,
    GCRY_KDF_PBKDF2 = 34
  };


gpg_error_t gcry_kdf_derive (const void *passphrase, size_t passphraselen,
                             int algo, int subalgo,
                             const void *salt, size_t saltlen,
                             unsigned long iterations,
                             size_t keysize, void *keybuffer);
# 1506 "./include/holy/gcrypt/gcrypt.h"
typedef enum gcry_random_level
  {
    GCRY_WEAK_RANDOM = 0,
    GCRY_STRONG_RANDOM = 1,
    GCRY_VERY_STRONG_RANDOM = 2
  }
gcry_random_level_t;



void gcry_randomize (void *buffer, size_t length,
                     enum gcry_random_level level);




gcry_err_code_t gcry_random_add_bytes (const void *buffer, size_t length,
                                    int quality);
# 1533 "./include/holy/gcrypt/gcrypt.h"
void *gcry_random_bytes (size_t nbytes, enum gcry_random_level level)
                         __attribute__ ((__malloc__));




void *gcry_random_bytes_secure (size_t nbytes, enum gcry_random_level level)
                                __attribute__ ((__malloc__));





void gcry_mpi_randomize (gcry_mpi_t w,
                         unsigned int nbits, enum gcry_random_level level);



void gcry_create_nonce (void *buffer, size_t length);
# 1570 "./include/holy/gcrypt/gcrypt.h"
typedef int (*gcry_prime_check_func_t) (void *arg, int mode,
                                        gcry_mpi_t candidate);
# 1588 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_prime_generate (gcry_mpi_t *prime,
                                  unsigned int prime_bits,
                                  unsigned int factor_bits,
                                  gcry_mpi_t **factors,
                                  gcry_prime_check_func_t cb_func,
                                  void *cb_arg,
                                  gcry_random_level_t random_level,
                                  unsigned int flags);





gcry_err_code_t gcry_prime_group_generator (gcry_mpi_t *r_g,
                                         gcry_mpi_t prime,
                                         gcry_mpi_t *factors,
                                         gcry_mpi_t start_g);



void gcry_prime_release_factors (gcry_mpi_t *factors);



gcry_err_code_t gcry_prime_check (gcry_mpi_t x, unsigned int flags);
# 1623 "./include/holy/gcrypt/gcrypt.h"
enum gcry_log_levels
  {
    GCRY_LOG_CONT = 0,
    GCRY_LOG_INFO = 10,
    GCRY_LOG_WARN = 20,
    GCRY_LOG_ERROR = 30,
    GCRY_LOG_FATAL = 40,
    GCRY_LOG_BUG = 50,
    GCRY_LOG_DEBUG = 100
  };


typedef void (*gcry_handler_progress_t) (void *, const char *, int, int, int);


typedef void *(*gcry_handler_alloc_t) (size_t n);


typedef int (*gcry_handler_secure_check_t) (const void *);


typedef void *(*gcry_handler_realloc_t) (void *p, size_t n);


typedef void (*gcry_handler_free_t) (void *);


typedef int (*gcry_handler_no_mem_t) (void *, size_t, unsigned int);


typedef void (*gcry_handler_error_t) (void *, int, const char *);


typedef void (*gcry_handler_log_t) (void *, int, const char *, va_list);



void gcry_set_progress_handler (gcry_handler_progress_t cb, void *cb_data);



void gcry_set_allocation_handler (
                             gcry_handler_alloc_t func_alloc,
                             gcry_handler_alloc_t func_alloc_secure,
                             gcry_handler_secure_check_t func_secure_check,
                             gcry_handler_realloc_t func_realloc,
                             gcry_handler_free_t func_free);



void gcry_set_outofcore_handler (gcry_handler_no_mem_t h, void *opaque);



void gcry_set_fatalerror_handler (gcry_handler_error_t fnc, void *opaque);



void gcry_set_log_handler (gcry_handler_log_t f, void *opaque);


void gcry_set_gettext_handler (const char *(*f)(const char*));



void *gcry_malloc (size_t n) __attribute__ ((__malloc__));
void *gcry_calloc (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_malloc_secure (size_t n) __attribute__ ((__malloc__));
void *gcry_calloc_secure (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_realloc (void *a, size_t n);
char *gcry_strdup (const char *string) __attribute__ ((__malloc__));
void *gcry_xmalloc (size_t n) __attribute__ ((__malloc__));
void *gcry_xcalloc (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_xmalloc_secure (size_t n) __attribute__ ((__malloc__));
void *gcry_xcalloc_secure (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_xrealloc (void *a, size_t n);
char *gcry_xstrdup (const char * a) __attribute__ ((__malloc__));
void gcry_free (void *a);


int gcry_is_secure (const void *a) __attribute__ ((__pure__));






# 1 "./holy-core/lib/libgcrypt-holy/src/gcrypt-module.h" 1
# 1711 "./include/holy/gcrypt/gcrypt.h" 2
# 15 "./include/holy/emu/misc.h" 2



# 1 "./include/holy/util/misc.h" 1
# 9 "./include/holy/util/misc.h"
# 1 "/usr/include/stdlib.h" 1 3 4
# 26 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/stdlib.h" 2 3 4





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 344 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4

# 344 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4
typedef int wchar_t;
# 33 "/usr/include/stdlib.h" 2 3 4







# 1 "/usr/include/bits/waitflags.h" 1 3 4
# 41 "/usr/include/stdlib.h" 2 3 4
# 1 "/usr/include/bits/waitstatus.h" 1 3 4
# 42 "/usr/include/stdlib.h" 2 3 4
# 59 "/usr/include/stdlib.h" 3 4
typedef struct
  {
    int quot;
    int rem;
  } div_t;



typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;





__extension__ typedef struct
  {
    long long int quot;
    long long int rem;
  } lldiv_t;
# 98 "/usr/include/stdlib.h" 3 4
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__ , __leaf__)) ;



extern double atof (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern int atoi (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern long int atol (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;



__extension__ extern long long int atoll (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;



extern double strtod (const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern float strtof (const char *__restrict __nptr,
       char **__restrict __endptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern long double strtold (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 141 "/usr/include/stdlib.h" 3 4
extern _Float32 strtof32 (const char *__restrict __nptr,
     char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float64 strtof64 (const char *__restrict __nptr,
     char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float128 strtof128 (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float32x strtof32x (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float64x strtof64x (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 177 "/usr/include/stdlib.h" 3 4
extern long int strtol (const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern unsigned long int strtoul (const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



__extension__
extern long long int strtoq (const char *__restrict __nptr,
        char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr,
           char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




__extension__
extern long long int strtoll (const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern long int strtol (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtol") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
extern unsigned long int strtoul (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoul") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));

__extension__
extern long long int strtoq (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoll") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoull") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));

__extension__
extern long long int strtoll (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoll") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoull") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));
# 278 "/usr/include/stdlib.h" 3 4
extern int strfromd (char *__dest, size_t __size, const char *__format,
       double __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));

extern int strfromf (char *__dest, size_t __size, const char *__format,
       float __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));

extern int strfroml (char *__dest, size_t __size, const char *__format,
       long double __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 298 "/usr/include/stdlib.h" 3 4
extern int strfromf32 (char *__dest, size_t __size, const char * __format,
         _Float32 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf64 (char *__dest, size_t __size, const char * __format,
         _Float64 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf128 (char *__dest, size_t __size, const char * __format,
   _Float128 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf32x (char *__dest, size_t __size, const char * __format,
   _Float32x __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf64x (char *__dest, size_t __size, const char * __format,
   _Float64x __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 340 "/usr/include/stdlib.h" 3 4
extern long int strtol_l (const char *__restrict __nptr,
     char **__restrict __endptr, int __base,
     locale_t __loc) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

extern unsigned long int strtoul_l (const char *__restrict __nptr,
        char **__restrict __endptr,
        int __base, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

__extension__
extern long long int strtoll_l (const char *__restrict __nptr,
    char **__restrict __endptr, int __base,
    locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

__extension__
extern unsigned long long int strtoull_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       int __base, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));





extern long int strtol_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtol_l") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1, 4)));
extern unsigned long int strtoul_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoul_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
__extension__
extern long long int strtoll_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoll_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
__extension__
extern unsigned long long int strtoull_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoull_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
# 415 "/usr/include/stdlib.h" 3 4
extern double strtod_l (const char *__restrict __nptr,
   char **__restrict __endptr, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));

extern float strtof_l (const char *__restrict __nptr,
         char **__restrict __endptr, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));

extern long double strtold_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));
# 436 "/usr/include/stdlib.h" 3 4
extern _Float32 strtof32_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float64 strtof64_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float128 strtof128_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float32x strtof32x_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float64x strtof64x_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));
# 505 "/usr/include/stdlib.h" 3 4
extern char *l64a (long int __n) __attribute__ ((__nothrow__ , __leaf__)) ;


extern long int a64l (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;
# 521 "/usr/include/stdlib.h" 3 4
extern long int random (void) __attribute__ ((__nothrow__ , __leaf__));


extern void srandom (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));





extern char *initstate (unsigned int __seed, char *__statebuf,
   size_t __statelen) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern char *setstate (char *__statebuf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







struct random_data
  {
    int32_t *fptr;
    int32_t *rptr;
    int32_t *state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t *end_ptr;
  };

extern int random_r (struct random_data *__restrict __buf,
       int32_t *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern int srandom_r (unsigned int __seed, struct random_data *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
   size_t __statelen,
   struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));

extern int setstate_r (char *__restrict __statebuf,
         struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern int rand (void) __attribute__ ((__nothrow__ , __leaf__));

extern void srand (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));



extern int rand_r (unsigned int *__seed) __attribute__ ((__nothrow__ , __leaf__));







extern double drand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern double erand48 (unsigned short int __xsubi[3]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int lrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int nrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int mrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int jrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void srand48 (long int __seedval) __attribute__ ((__nothrow__ , __leaf__));
extern unsigned short int *seed48 (unsigned short int __seed16v[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern void lcong48 (unsigned short int __param[7]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





struct drand48_data
  {
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    __extension__ unsigned long long int __a;

  };


extern int drand48_r (struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int erand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int lrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int nrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int mrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int jrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

extern int seed48_r (unsigned short int __seed16v[3],
       struct drand48_data *__buffer) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern int lcong48_r (unsigned short int __param[7],
        struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern __uint32_t arc4random (void)
     __attribute__ ((__nothrow__ , __leaf__)) ;


extern void arc4random_buf (void *__buf, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern __uint32_t arc4random_uniform (__uint32_t __upper_bound)
     __attribute__ ((__nothrow__ , __leaf__)) ;




extern void *malloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__))
     __attribute__ ((__alloc_size__ (1))) ;

extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_size__ (1, 2))) ;






extern void *realloc (void *__ptr, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__)) __attribute__ ((__alloc_size__ (2)));


extern void free (void *__ptr) __attribute__ ((__nothrow__ , __leaf__));







extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__))
     __attribute__ ((__alloc_size__ (2, 3)))
    __attribute__ ((__malloc__ (__builtin_free, 1)));


extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__ (reallocarray, 1)));



# 1 "/usr/include/alloca.h" 1 3 4
# 24 "/usr/include/alloca.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 25 "/usr/include/alloca.h" 2 3 4







extern void *alloca (size_t __size) __attribute__ ((__nothrow__ , __leaf__));






# 707 "/usr/include/stdlib.h" 2 3 4





extern void *valloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__))
     __attribute__ ((__alloc_size__ (1))) ;




extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;




extern void *aligned_alloc (size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_align__ (1)))
     __attribute__ ((__alloc_size__ (2))) ;



extern void abort (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));



extern int atexit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int at_quick_exit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern void exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));





extern void quick_exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));





extern void _Exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));




extern char *getenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;




extern char *secure_getenv (const char *__name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;






extern int putenv (char *__string) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int setenv (const char *__name, const char *__value, int __replace)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));


extern int unsetenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern int clearenv (void) __attribute__ ((__nothrow__ , __leaf__));
# 814 "/usr/include/stdlib.h" 3 4
extern char *mktemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 830 "/usr/include/stdlib.h" 3 4
extern int mkstemp (char *__template) __asm__ ("" "mkstemp64")
     __attribute__ ((__nonnull__ (1))) ;





extern int mkstemp64 (char *__template) __attribute__ ((__nonnull__ (1))) ;
# 852 "/usr/include/stdlib.h" 3 4
extern int mkstemps (char *__template, int __suffixlen) __asm__ ("" "mkstemps64")
                     __attribute__ ((__nonnull__ (1))) ;





extern int mkstemps64 (char *__template, int __suffixlen)
     __attribute__ ((__nonnull__ (1))) ;
# 870 "/usr/include/stdlib.h" 3 4
extern char *mkdtemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 884 "/usr/include/stdlib.h" 3 4
extern int mkostemp (char *__template, int __flags) __asm__ ("" "mkostemp64")
     __attribute__ ((__nonnull__ (1))) ;





extern int mkostemp64 (char *__template, int __flags) __attribute__ ((__nonnull__ (1))) ;
# 905 "/usr/include/stdlib.h" 3 4
extern int mkostemps (char *__template, int __suffixlen, int __flags) __asm__ ("" "mkostemps64")

     __attribute__ ((__nonnull__ (1))) ;





extern int mkostemps64 (char *__template, int __suffixlen, int __flags)
     __attribute__ ((__nonnull__ (1))) ;
# 923 "/usr/include/stdlib.h" 3 4
extern int system (const char *__command) ;





extern char *canonicalize_file_name (const char *__name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__malloc__))
     __attribute__ ((__malloc__ (__builtin_free, 1))) ;
# 940 "/usr/include/stdlib.h" 3 4
extern char *realpath (const char *__restrict __name,
         char *__restrict __resolved) __attribute__ ((__nothrow__ , __leaf__)) ;






typedef int (*__compar_fn_t) (const void *, const void *);


typedef __compar_fn_t comparison_fn_t;



typedef int (*__compar_d_fn_t) (const void *, const void *, void *);




extern void *bsearch (const void *__key, const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;







extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));

extern void qsort_r (void *__base, size_t __nmemb, size_t __size,
       __compar_d_fn_t __compar, void *__arg)
  __attribute__ ((__nonnull__ (1, 4)));




extern int abs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern long int labs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;


__extension__ extern long long int llabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;



extern unsigned int uabs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern unsigned long int ulabs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
__extension__ extern unsigned long long int ullabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;





extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;


__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
# 1018 "/usr/include/stdlib.h" 3 4
extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *gcvt (double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;




extern char *qecvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qfcvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;




extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));

extern int qecvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qfcvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));





extern int mblen (const char *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));


extern int mbtowc (wchar_t *__restrict __pwc,
     const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));


extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__ , __leaf__));



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__read_only__, 2)));

extern size_t wcstombs (char *__restrict __s,
   const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1, 3)))
  __attribute__ ((__access__ (__read_only__, 2)));






extern int rpmatch (const char *__response) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 1105 "/usr/include/stdlib.h" 3 4
extern int getsubopt (char **__restrict __optionp,
        char *const *__restrict __tokens,
        char **__restrict __valuep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2, 3))) ;







extern int posix_openpt (int __oflag) ;







extern int grantpt (int __fd) __attribute__ ((__nothrow__ , __leaf__));



extern int unlockpt (int __fd) __attribute__ ((__nothrow__ , __leaf__));




extern char *ptsname (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int ptsname_r (int __fd, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 2, 3)));


extern int getpt (void);






extern int getloadavg (double __loadavg[], int __nelem)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 1161 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/bits/stdlib-float.h" 1 3 4
# 1162 "/usr/include/stdlib.h" 2 3 4
# 1173 "/usr/include/stdlib.h" 3 4

# 10 "./include/holy/util/misc.h" 2


# 1 "/usr/include/setjmp.h" 1 3 4
# 27 "/usr/include/setjmp.h" 3 4


# 1 "/usr/include/bits/setjmp.h" 1 3 4
# 26 "/usr/include/bits/setjmp.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 27 "/usr/include/bits/setjmp.h" 2 3 4




typedef long int __jmp_buf[8];
# 30 "/usr/include/setjmp.h" 2 3 4
# 1 "/usr/include/bits/types/struct___jmp_buf_tag.h" 1 3 4
# 26 "/usr/include/bits/types/struct___jmp_buf_tag.h" 3 4
struct __jmp_buf_tag
  {




    __jmp_buf __jmpbuf;
    int __mask_was_saved;
    __sigset_t __saved_mask;
  };
# 31 "/usr/include/setjmp.h" 2 3 4

typedef struct __jmp_buf_tag jmp_buf[1];



extern int setjmp (jmp_buf __env) __attribute__ ((__nothrow__));




extern int __sigsetjmp (struct __jmp_buf_tag __env[1], int __savemask) __attribute__ ((__nothrow__));



extern int _setjmp (struct __jmp_buf_tag __env[1]) __attribute__ ((__nothrow__));
# 54 "/usr/include/setjmp.h" 3 4
extern void longjmp (struct __jmp_buf_tag __env[1], int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));





extern void _longjmp (struct __jmp_buf_tag __env[1], int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));







typedef struct __jmp_buf_tag sigjmp_buf[1];
# 80 "/usr/include/setjmp.h" 3 4
extern void siglongjmp (sigjmp_buf __env, int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));
# 90 "/usr/include/setjmp.h" 3 4

# 13 "./include/holy/util/misc.h" 2


# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 16 "./include/holy/util/misc.h" 2


# 1 "./include/holy/emu/misc.h" 1
# 19 "./include/holy/util/misc.h" 2


# 20 "./include/holy/util/misc.h"
char *holy_util_get_path (const char *dir, const char *file);
size_t holy_util_get_image_size (const char *path);
char *holy_util_read_image (const char *path);
void holy_util_load_image (const char *path, char *buf);
void holy_util_write_image (const char *img, size_t size, FILE *out,
       const char *name);
void holy_util_write_image_at (const void *img, size_t size, off_t offset,
          FILE *out, const char *name);

char *make_system_path_relative_to_its_root (const char *path);

char *holy_canonicalize_file_name (const char *path);

void holy_util_init_nls (void);

void holy_util_host_init (int *argc, char ***argv);

int holy_qsort_strcmp (const void *, const void *);
# 19 "./include/holy/emu/misc.h" 2

extern int verbosity;
extern const char *program_name;


extern const unsigned long long int holy_size_t;
void holy_init_all (void);
void holy_fini_all (void);

void holy_find_zpool_from_dir (const char *dir,
          char **poolname, char **poolfs);

char *holy_make_system_path_relative_to_its_root (const char *path)
 __attribute__ ((warn_unused_result));
int
holy_util_device_is_mapped (const char *dev);
# 44 "./include/holy/emu/misc.h"
void * xmalloc (holy_size_t size) __attribute__ ((warn_unused_result));
void * xrealloc (void *ptr, holy_size_t size) __attribute__ ((warn_unused_result));
char * xstrdup (const char *str) __attribute__ ((warn_unused_result));
char * xasprintf (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2))) __attribute__ ((warn_unused_result));

void holy_util_warn (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void holy_util_info (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void holy_util_error (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2), noreturn));

holy_uint64_t holy_util_get_cpu_time_ms (void);


int holy_device_mapper_supported (void);





FILE *
holy_util_fopen (const char *path, const char *mode);


void holy_util_file_sync (FILE *f);
# 9 "util/holy-fstest.c" 2




# 1 "./include/holy/file.h" 1
# 12 "./include/holy/file.h"
# 1 "./include/holy/fs.h" 1
# 20 "./include/holy/fs.h"
struct holy_file;

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;
typedef holy_int64_t holy_ssize_t;



struct holy_dirhook_info
{
  unsigned dir:1;
  unsigned mtimeset:1;
  unsigned case_insensitive:1;
  unsigned inodeset:1;
  holy_int32_t mtime;
  holy_uint64_t inode;
};

typedef int (*holy_fs_dir_hook_t) (const char *filename,
       const struct holy_dirhook_info *info,
       void *data);


struct holy_fs
{

  struct holy_fs *next;
  struct holy_fs **prev;


  const char *name;


  holy_err_t (*dir) (holy_device_t device, const char *path,
       holy_fs_dir_hook_t hook, void *hook_data);


  holy_err_t (*open) (struct holy_file *file, const char *name);


  holy_ssize_t (*read) (struct holy_file *file, char *buf, holy_size_t len);


  holy_err_t (*close) (struct holy_file *file);




  holy_err_t (*label) (holy_device_t device, char **label);




  holy_err_t (*uuid) (holy_device_t device, char **uuid);


  holy_err_t (*mtime) (holy_device_t device, holy_int32_t *timebuf);



  holy_err_t (*embed) (holy_device_t device, unsigned int *nsectors,
         unsigned int max_nsectors,
         holy_embed_type_t embed_type,
         holy_disk_addr_t **sectors);


  int reserved_first_sector;


  int blocklist_install;

};
typedef struct holy_fs *holy_fs_t;


extern struct holy_fs holy_fs_blocklist;





typedef int (*holy_fs_autoload_hook_t) (void);
extern holy_fs_autoload_hook_t holy_fs_autoload_hook;
extern holy_fs_t holy_fs_list;


static inline void
holy_fs_register (holy_fs_t fs)
{
  holy_list_push ((((char *) &(*&holy_fs_list)->next == (char *) &((holy_list_t) (*&holy_fs_list))->next) && ((char *) &(*&holy_fs_list)->prev == (char *) &((holy_list_t) (*&holy_fs_list))->prev) ? (holy_list_t *) (void *) &holy_fs_list : (holy_list_t *) holy_bad_type_cast_real(117, "util/holy-fstest.c")), (((char *) &(fs)->next == (char *) &((holy_list_t) (fs))->next) && ((char *) &(fs)->prev == (char *) &((holy_list_t) (fs))->prev) ? (holy_list_t) fs : (holy_list_t) holy_bad_type_cast_real(117, "util/holy-fstest.c")));
}


static inline void
holy_fs_unregister (holy_fs_t fs)
{
  holy_list_remove ((((char *) &(fs)->next == (char *) &((holy_list_t) (fs))->next) && ((char *) &(fs)->prev == (char *) &((holy_list_t) (fs))->prev) ? (holy_list_t) fs : (holy_list_t) holy_bad_type_cast_real(124, "util/holy-fstest.c")));
}



holy_fs_t holy_fs_probe (holy_device_t device);
# 13 "./include/holy/file.h" 2


typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long long int holy_int64_t;

typedef holy_int64_t holy_ssize_t;
typedef holy_uint64_t holy_size_t;

typedef holy_uint64_t holy_off_t;
typedef holy_uint64_t holy_disk_addr_t;


struct holy_disk;
struct holy_net;

struct holy_device {
    struct holy_disk *disk;
    struct holy_net *net;
}

typedef struct holy_device *holy_device_t;



struct holy_file
{

  char *name;


  holy_device_t device;


  holy_fs_t fs;


  holy_off_t offset;
  holy_off_t progress_offset;


  holy_uint64_t last_progress_time;
  holy_off_t last_progress_offset;
  holy_uint64_t estimated_speed;


  holy_off_t size;


  int not_easily_seekable;


  void *data;


  holy_disk_read_hook_t read_hook;


  void *read_hook_data;
};
typedef struct holy_file *holy_file_t;

extern holy_disk_read_hook_t holy_file_progress_hook;


typedef enum holy_file_filter_id
  {
    holy_FILE_FILTER_PUBKEY,
    holy_FILE_FILTER_GZIO,
    holy_FILE_FILTER_XZIO,
    holy_FILE_FILTER_LZOPIO,
    holy_FILE_FILTER_MAX,
    holy_FILE_FILTER_COMPRESSION_FIRST = holy_FILE_FILTER_GZIO,
    holy_FILE_FILTER_COMPRESSION_LAST = holy_FILE_FILTER_LZOPIO,
  } holy_file_filter_id_t;

typedef holy_file_t (*holy_file_filter_t) (holy_file_t in, const char *filename);

extern holy_file_filter_t holy_file_filters_all[holy_FILE_FILTER_MAX];
extern holy_file_filter_t holy_file_filters_enabled[holy_FILE_FILTER_MAX];

static inline void
holy_file_filter_register (holy_file_filter_id_t id, holy_file_filter_t filter)
{
  holy_file_filters_all[id] = filter;
  holy_file_filters_enabled[id] = filter;
}

static inline void
holy_file_filter_unregister (holy_file_filter_id_t id)
{
  holy_file_filters_all[id] = 0;
  holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable (holy_file_filter_id_t id)
{
  holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_compression (void)
{
  holy_file_filter_id_t id;

  for (id = holy_FILE_FILTER_COMPRESSION_FIRST;
       id <= holy_FILE_FILTER_COMPRESSION_LAST; id++)
    holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_all (void)
{
  holy_file_filter_id_t id;

  for (id = 0;
       id < holy_FILE_FILTER_MAX; id++)
    holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_pubkey (void)
{
  holy_file_filters_enabled[holy_FILE_FILTER_PUBKEY] = 0;
}


char *holy_file_get_device_name (const char *name);

holy_file_t holy_file_open (const char *name);
holy_ssize_t holy_file_read (holy_file_t file, void *buf,
       holy_size_t len);
holy_off_t holy_file_seek (holy_file_t file, holy_off_t offset);
holy_err_t holy_file_close (holy_file_t file);




static inline holy_off_t
holy_file_size (const holy_file_t file)
{
  return file->size;
}

static inline holy_off_t
holy_file_tell (const holy_file_t file)
{
  return file->offset;
}

static inline int
holy_file_seekable (const holy_file_t file)
{
  return !file->not_easily_seekable;
}

holy_file_t
holy_file_offset_open (holy_file_t parent, holy_off_t start,
         holy_off_t size);
void
holy_file_offset_close (holy_file_t file);
# 14 "util/holy-fstest.c" 2

# 1 "./include/holy/env.h" 1
# 12 "./include/holy/env.h"
# 1 "./include/holy/menu.h" 1
# 9 "./include/holy/menu.h"
struct holy_menu_entry_class
{
  char *name;
  struct holy_menu_entry_class *next;
};


struct holy_menu_entry
{

  const char *title;


  const char *id;


  int restricted;


  const char *users;





  struct holy_menu_entry_class *classes;


  const char *sourcecode;


  int argc;
  char **args;

  int hotkey;

  int submenu;


  struct holy_menu_entry *next;
};
typedef struct holy_menu_entry *holy_menu_entry_t;


struct holy_menu
{

  int size;


  holy_menu_entry_t entry_list;
};
typedef struct holy_menu *holy_menu_t;



typedef struct holy_menu_execute_callback
{

  void (*notify_booting) (holy_menu_entry_t entry, void *userdata);






  void (*notify_fallback) (holy_menu_entry_t entry, void *userdata);



  void (*notify_failure) (void *userdata);
}
*holy_menu_execute_callback_t;

holy_menu_entry_t holy_menu_get_entry (holy_menu_t menu, int no);
int holy_menu_get_timeout (void);
void holy_menu_set_timeout (int timeout);
void holy_menu_entry_run (holy_menu_entry_t entry);
int holy_menu_get_default_entry_index (holy_menu_t menu);

void holy_menu_init (void);
void holy_menu_fini (void);
# 13 "./include/holy/env.h" 2

struct holy_env_var;

typedef const char *(*holy_env_read_hook_t) (struct holy_env_var *var,
          const char *val);
typedef char *(*holy_env_write_hook_t) (struct holy_env_var *var,
     const char *val);

struct holy_env_var
{
  char *name;
  char *value;
  holy_env_read_hook_t read_hook;
  holy_env_write_hook_t write_hook;
  struct holy_env_var *next;
  struct holy_env_var **prevp;
  struct holy_env_var *sorted_next;
  int global;
};

holy_err_t holy_env_set (const char *name, const char *val);
const char *holy_env_get (const char *name);
void holy_env_unset (const char *name);
struct holy_env_var *holy_env_update_get_sorted (void);



holy_err_t holy_register_variable_hook (const char *name,
           holy_env_read_hook_t read_hook,
           holy_env_write_hook_t write_hook);

holy_err_t holy_env_context_open (void);
holy_err_t holy_env_context_close (void);
holy_err_t holy_env_export (const char *name);

void holy_env_unset_menu (void);
holy_menu_t holy_env_get_menu (void);
void holy_env_set_menu (holy_menu_t nmenu);

holy_err_t
holy_env_extractor_open (int source);

holy_err_t
holy_env_extractor_close (int source);
# 16 "util/holy-fstest.c" 2
# 1 "./include/holy/term.h" 1
# 58 "./include/holy/term.h"
# 1 "./include/holy/unicode.h" 1
# 13 "./include/holy/unicode.h"
struct holy_unicode_bidi_pair
{
  holy_uint32_t key;
  holy_uint32_t replace;
};

struct holy_unicode_compact_range
{
  unsigned start:21;
  unsigned len:9;
  unsigned bidi_type:5;
  unsigned comb_type:8;
  unsigned bidi_mirror:1;
  unsigned join_type:3;
} __attribute__ ((packed));



struct holy_unicode_arabic_shape
{
  holy_uint32_t code;
  holy_uint32_t isolated;
  holy_uint32_t right_linked;
  holy_uint32_t both_linked;
  holy_uint32_t left_linked;
};

extern struct holy_unicode_arabic_shape holy_unicode_arabic_shapes[];

enum holy_bidi_type
  {
    holy_BIDI_TYPE_L = 0,
    holy_BIDI_TYPE_LRE,
    holy_BIDI_TYPE_LRO,
    holy_BIDI_TYPE_R,
    holy_BIDI_TYPE_AL,
    holy_BIDI_TYPE_RLE,
    holy_BIDI_TYPE_RLO,
    holy_BIDI_TYPE_PDF,
    holy_BIDI_TYPE_EN,
    holy_BIDI_TYPE_ES,
    holy_BIDI_TYPE_ET,
    holy_BIDI_TYPE_AN,
    holy_BIDI_TYPE_CS,
    holy_BIDI_TYPE_NSM,
    holy_BIDI_TYPE_BN,
    holy_BIDI_TYPE_B,
    holy_BIDI_TYPE_S,
    holy_BIDI_TYPE_WS,
    holy_BIDI_TYPE_ON
  };

enum holy_join_type
  {
    holy_JOIN_TYPE_NONJOINING = 0,
    holy_JOIN_TYPE_LEFT = 1,
    holy_JOIN_TYPE_RIGHT = 2,
    holy_JOIN_TYPE_DUAL = 3,
    holy_JOIN_TYPE_CAUSING = 4,
    holy_JOIN_TYPE_TRANSPARENT = 5
  };

enum holy_comb_type
  {
    holy_UNICODE_COMB_NONE = 0,
    holy_UNICODE_COMB_OVERLAY = 1,
    holy_UNICODE_COMB_HEBREW_SHEVA = 10,
    holy_UNICODE_COMB_HEBREW_HATAF_SEGOL = 11,
    holy_UNICODE_COMB_HEBREW_HATAF_PATAH = 12,
    holy_UNICODE_COMB_HEBREW_HATAF_QAMATS = 13,
    holy_UNICODE_COMB_HEBREW_HIRIQ = 14,
    holy_UNICODE_COMB_HEBREW_TSERE = 15,
    holy_UNICODE_COMB_HEBREW_SEGOL = 16,
    holy_UNICODE_COMB_HEBREW_PATAH = 17,
    holy_UNICODE_COMB_HEBREW_QAMATS = 18,
    holy_UNICODE_COMB_HEBREW_HOLAM = 19,
    holy_UNICODE_COMB_HEBREW_QUBUTS = 20,
    holy_UNICODE_COMB_HEBREW_DAGESH = 21,
    holy_UNICODE_COMB_HEBREW_METEG = 22,
    holy_UNICODE_COMB_HEBREW_RAFE = 23,
    holy_UNICODE_COMB_HEBREW_SHIN_DOT = 24,
    holy_UNICODE_COMB_HEBREW_SIN_DOT = 25,
    holy_UNICODE_COMB_HEBREW_VARIKA = 26,
    holy_UNICODE_COMB_ARABIC_FATHATAN = 27,
    holy_UNICODE_COMB_ARABIC_DAMMATAN = 28,
    holy_UNICODE_COMB_ARABIC_KASRATAN = 29,
    holy_UNICODE_COMB_ARABIC_FATHAH = 30,
    holy_UNICODE_COMB_ARABIC_DAMMAH = 31,
    holy_UNICODE_COMB_ARABIC_KASRA = 32,
    holy_UNICODE_COMB_ARABIC_SHADDA = 33,
    holy_UNICODE_COMB_ARABIC_SUKUN = 34,
    holy_UNICODE_COMB_ARABIC_SUPERSCRIPT_ALIF = 35,
    holy_UNICODE_COMB_SYRIAC_SUPERSCRIPT_ALAPH = 36,
    holy_UNICODE_STACK_ATTACHED_BELOW = 202,
    holy_UNICODE_STACK_ATTACHED_ABOVE = 214,
    holy_UNICODE_COMB_ATTACHED_ABOVE_RIGHT = 216,
    holy_UNICODE_STACK_BELOW = 220,
    holy_UNICODE_COMB_BELOW_RIGHT = 222,
    holy_UNICODE_COMB_ABOVE_LEFT = 228,
    holy_UNICODE_STACK_ABOVE = 230,
    holy_UNICODE_COMB_ABOVE_RIGHT = 232,
    holy_UNICODE_COMB_YPOGEGRAMMENI = 240,


    holy_UNICODE_COMB_ME = 253,
    holy_UNICODE_COMB_MC = 254,
    holy_UNICODE_COMB_MN = 255,
  };

struct holy_unicode_combining
{
  holy_uint32_t code:21;
  enum holy_comb_type type:8;
};

struct holy_unicode_glyph
{
  holy_uint32_t base:23;
  holy_uint16_t variant:9;

  holy_uint8_t attributes:5;
  holy_uint8_t bidi_level:6;
  enum holy_bidi_type bidi_type:5;

  unsigned ncomb:8;


  int estimated_width:8;

  holy_size_t orig_pos;
  union
  {
    struct holy_unicode_combining combining_inline[sizeof (void *)
         / sizeof (struct holy_unicode_combining)];
    struct holy_unicode_combining *combining_ptr;
  };
};
# 168 "./include/holy/unicode.h"
enum
  {
    holy_UNICODE_DOTLESS_LOWERCASE_I = 0x0131,
    holy_UNICODE_DOTLESS_LOWERCASE_J = 0x0237,
    holy_UNICODE_COMBINING_GRAPHEME_JOINER = 0x034f,
    holy_UNICODE_HEBREW_WAW = 0x05d5,
    holy_UNICODE_ARABIC_START = 0x0600,
    holy_UNICODE_ARABIC_END = 0x0700,
    holy_UNICODE_THAANA_ABAFILI = 0x07a6,
    holy_UNICODE_THAANA_AABAAFILI = 0x07a7,
    holy_UNICODE_THAANA_IBIFILI = 0x07a8,
    holy_UNICODE_THAANA_EEBEEFILI = 0x07a9,
    holy_UNICODE_THAANA_UBUFILI = 0x07aa,
    holy_UNICODE_THAANA_OOBOOFILI = 0x07ab,
    holy_UNICODE_THAANA_EBEFILI = 0x07ac,
    holy_UNICODE_THAANA_EYBEYFILI = 0x07ad,
    holy_UNICODE_THAANA_OBOFILI = 0x07ae,
    holy_UNICODE_THAANA_OABOAFILI = 0x07af,
    holy_UNICODE_THAANA_SUKUN = 0x07b0,
    holy_UNICODE_ZWNJ = 0x200c,
    holy_UNICODE_ZWJ = 0x200d,
    holy_UNICODE_LRM = 0x200e,
    holy_UNICODE_RLM = 0x200f,
    holy_UNICODE_LRE = 0x202a,
    holy_UNICODE_RLE = 0x202b,
    holy_UNICODE_PDF = 0x202c,
    holy_UNICODE_LRO = 0x202d,
    holy_UNICODE_RLO = 0x202e,
    holy_UNICODE_LEFTARROW = 0x2190,
    holy_UNICODE_UPARROW = 0x2191,
    holy_UNICODE_RIGHTARROW = 0x2192,
    holy_UNICODE_DOWNARROW = 0x2193,
    holy_UNICODE_UPDOWNARROW = 0x2195,
    holy_UNICODE_LIGHT_HLINE = 0x2500,
    holy_UNICODE_HLINE = 0x2501,
    holy_UNICODE_LIGHT_VLINE = 0x2502,
    holy_UNICODE_VLINE = 0x2503,
    holy_UNICODE_LIGHT_CORNER_UL = 0x250c,
    holy_UNICODE_CORNER_UL = 0x250f,
    holy_UNICODE_LIGHT_CORNER_UR = 0x2510,
    holy_UNICODE_CORNER_UR = 0x2513,
    holy_UNICODE_LIGHT_CORNER_LL = 0x2514,
    holy_UNICODE_CORNER_LL = 0x2517,
    holy_UNICODE_LIGHT_CORNER_LR = 0x2518,
    holy_UNICODE_CORNER_LR = 0x251b,
    holy_UNICODE_BLACK_UP_TRIANGLE = 0x25b2,
    holy_UNICODE_BLACK_RIGHT_TRIANGLE = 0x25ba,
    holy_UNICODE_BLACK_DOWN_TRIANGLE = 0x25bc,
    holy_UNICODE_BLACK_LEFT_TRIANGLE = 0x25c4,
    holy_UNICODE_VARIATION_SELECTOR_1 = 0xfe00,
    holy_UNICODE_VARIATION_SELECTOR_16 = 0xfe0f,
    holy_UNICODE_TAG_START = 0xe0000,
    holy_UNICODE_TAG_END = 0xe007f,
    holy_UNICODE_VARIATION_SELECTOR_17 = 0xe0100,
    holy_UNICODE_VARIATION_SELECTOR_256 = 0xe01ef,
    holy_UNICODE_LAST_VALID = 0x10ffff
  };

extern struct holy_unicode_compact_range holy_unicode_compact[];
extern struct holy_unicode_bidi_pair holy_unicode_bidi_pairs[];





struct holy_term_pos
{
  unsigned valid:1;
  unsigned x:15, y:16;
};

holy_ssize_t
holy_bidi_logical_to_visual (const holy_uint32_t *logical,
        holy_size_t logical_len,
        struct holy_unicode_glyph **visual_out,
        holy_size_t (*getcharwidth) (const struct holy_unicode_glyph *visual, void *getcharwidth_arg),
        void *getcharwidth_arg,
        holy_size_t max_width,
        holy_size_t start_width, holy_uint32_t codechar,
        struct holy_term_pos *pos,
        int primitive_wrap);

enum holy_comb_type
holy_unicode_get_comb_type (holy_uint32_t c);
holy_size_t
holy_unicode_aglomerate_comb (const holy_uint32_t *in, holy_size_t inlen,
         struct holy_unicode_glyph *out);

static inline const struct holy_unicode_combining *
holy_unicode_get_comb (const struct holy_unicode_glyph *in)
{
  if (in->ncomb == 0)
    return 
# 260 "./include/holy/unicode.h" 3 4
          ((void *)0)
# 260 "./include/holy/unicode.h"
              ;
  if (in->ncomb > (sizeof (in->combining_inline) / sizeof (in->combining_inline[0])))
    return in->combining_ptr;
  return in->combining_inline;
}

static inline void
holy_unicode_destroy_glyph (struct holy_unicode_glyph *glyph)
{
  if (glyph->ncomb > (sizeof (glyph->combining_inline) / sizeof (glyph->combining_inline[0])))
    holy_free (glyph->combining_ptr);
  glyph->ncomb = 0;
}

static inline struct holy_unicode_glyph *
holy_unicode_glyph_dup (const struct holy_unicode_glyph *in)
{
  struct holy_unicode_glyph *out = holy_malloc (sizeof (*out));
  if (!out)
    return 
# 279 "./include/holy/unicode.h" 3 4
          ((void *)0)
# 279 "./include/holy/unicode.h"
              ;
  holy_memcpy (out, in, sizeof (*in));
  if (in->ncomb > (sizeof (out->combining_inline) / sizeof (out->combining_inline[0])))
    {
      out->combining_ptr = holy_malloc (in->ncomb * sizeof (out->combining_ptr[0]));
      if (!out->combining_ptr)
 {
   holy_free (out);
   return 
# 287 "./include/holy/unicode.h" 3 4
         ((void *)0)
# 287 "./include/holy/unicode.h"
             ;
 }
      holy_memcpy (out->combining_ptr, in->combining_ptr,
     in->ncomb * sizeof (out->combining_ptr[0]));
    }
  else
    holy_memcpy (&out->combining_inline, &in->combining_inline,
   sizeof (out->combining_inline));
  return out;
}

static inline void
holy_unicode_set_glyph (struct holy_unicode_glyph *out,
   const struct holy_unicode_glyph *in)
{
  holy_memcpy (out, in, sizeof (*in));
  if (in->ncomb > (sizeof (out->combining_inline) / sizeof (out->combining_inline[0])))
    {
      out->combining_ptr = holy_malloc (in->ncomb * sizeof (out->combining_ptr[0]));
      if (!out->combining_ptr)
 return;
      holy_memcpy (out->combining_ptr, in->combining_ptr,
     in->ncomb * sizeof (out->combining_ptr[0]));
    }
  else
    holy_memcpy (&out->combining_inline, &in->combining_inline,
   sizeof (out->combining_inline));
}

static inline struct holy_unicode_glyph *
holy_unicode_glyph_from_code (holy_uint32_t code)
{
  struct holy_unicode_glyph *ret;
  ret = holy_zalloc (sizeof (*ret));
  if (!ret)
    return 
# 322 "./include/holy/unicode.h" 3 4
          ((void *)0)
# 322 "./include/holy/unicode.h"
              ;

  ret->base = code;

  return ret;
}

static inline void
holy_unicode_set_glyph_from_code (struct holy_unicode_glyph *glyph,
      holy_uint32_t code)
{
  holy_memset (glyph, 0, sizeof (*glyph));

  glyph->base = code;
}

holy_uint32_t
holy_unicode_mirror_code (holy_uint32_t in);
holy_uint32_t
holy_unicode_shape_code (holy_uint32_t in, holy_uint8_t attr);

const holy_uint32_t *
holy_unicode_get_comb_end (const holy_uint32_t *end,
      const holy_uint32_t *cur);
# 59 "./include/holy/term.h" 2



typedef enum
  {


    holy_TERM_COLOR_STANDARD,

    holy_TERM_COLOR_NORMAL,

    holy_TERM_COLOR_HIGHLIGHT
  }
holy_term_color_state;
# 124 "./include/holy/term.h"
struct holy_term_input
{

  struct holy_term_input *next;
  struct holy_term_input **prev;


  const char *name;


  holy_err_t (*init) (struct holy_term_input *term);


  holy_err_t (*fini) (struct holy_term_input *term);


  int (*getkey) (struct holy_term_input *term);


  int (*getkeystatus) (struct holy_term_input *term);

  void *data;
};
typedef struct holy_term_input *holy_term_input_t;


struct holy_term_coordinate
{
  holy_uint16_t x;
  holy_uint16_t y;
};

struct holy_term_output
{

  struct holy_term_output *next;
  struct holy_term_output **prev;


  const char *name;


  holy_err_t (*init) (struct holy_term_output *term);


  holy_err_t (*fini) (struct holy_term_output *term);


  void (*putchar) (struct holy_term_output *term,
     const struct holy_unicode_glyph *c);



  holy_size_t (*getcharwidth) (struct holy_term_output *term,
          const struct holy_unicode_glyph *c);


  struct holy_term_coordinate (*getwh) (struct holy_term_output *term);


  struct holy_term_coordinate (*getxy) (struct holy_term_output *term);


  void (*gotoxy) (struct holy_term_output *term,
    struct holy_term_coordinate pos);


  void (*cls) (struct holy_term_output *term);


  void (*setcolorstate) (struct holy_term_output *term,
    holy_term_color_state state);


  void (*setcursor) (struct holy_term_output *term, int on);


  void (*refresh) (struct holy_term_output *term);


  holy_err_t (*fullscreen) (void);


  holy_uint32_t flags;


  holy_uint32_t progress_update_divisor;
  holy_uint32_t progress_update_counter;

  void *data;
};
typedef struct holy_term_output *holy_term_output_t;






extern holy_uint8_t holy_term_normal_color;
extern holy_uint8_t holy_term_highlight_color;

extern struct holy_term_output *holy_term_outputs_disabled;
extern struct holy_term_input *holy_term_inputs_disabled;
extern struct holy_term_output *holy_term_outputs;
extern struct holy_term_input *holy_term_inputs;

static inline void
holy_term_register_input (const char *name __attribute__ ((unused)),
     holy_term_input_t term)
{
  if (holy_term_inputs)
    holy_list_push ((((char *) &(*&holy_term_inputs_disabled)->next == (char *) &((holy_list_t) (*&holy_term_inputs_disabled))->next) && ((char *) &(*&holy_term_inputs_disabled)->prev == (char *) &((holy_list_t) (*&holy_term_inputs_disabled))->prev) ? (holy_list_t *) (void *) &holy_term_inputs_disabled : (holy_list_t *) holy_bad_type_cast_real(235, "util/holy-fstest.c")),
      (((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(236, "util/holy-fstest.c")));
  else
    {

      if (! term->init || term->init (term) == holy_ERR_NONE)
 holy_list_push ((((char *) &(*&holy_term_inputs)->next == (char *) &((holy_list_t) (*&holy_term_inputs))->next) && ((char *) &(*&holy_term_inputs)->prev == (char *) &((holy_list_t) (*&holy_term_inputs))->prev) ? (holy_list_t *) (void *) &holy_term_inputs : (holy_list_t *) holy_bad_type_cast_real(241, "util/holy-fstest.c")), (((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(241, "util/holy-fstest.c")));
    }
}

static inline void
holy_term_register_input_inactive (const char *name __attribute__ ((unused)),
       holy_term_input_t term)
{
  holy_list_push ((((char *) &(*&holy_term_inputs_disabled)->next == (char *) &((holy_list_t) (*&holy_term_inputs_disabled))->next) && ((char *) &(*&holy_term_inputs_disabled)->prev == (char *) &((holy_list_t) (*&holy_term_inputs_disabled))->prev) ? (holy_list_t *) (void *) &holy_term_inputs_disabled : (holy_list_t *) holy_bad_type_cast_real(249, "util/holy-fstest.c")),
    (((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(250, "util/holy-fstest.c")));
}

static inline void
holy_term_register_input_active (const char *name __attribute__ ((unused)),
     holy_term_input_t term)
{
  if (! term->init || term->init (term) == holy_ERR_NONE)
    holy_list_push ((((char *) &(*&holy_term_inputs)->next == (char *) &((holy_list_t) (*&holy_term_inputs))->next) && ((char *) &(*&holy_term_inputs)->prev == (char *) &((holy_list_t) (*&holy_term_inputs))->prev) ? (holy_list_t *) (void *) &holy_term_inputs : (holy_list_t *) holy_bad_type_cast_real(258, "util/holy-fstest.c")), (((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(258, "util/holy-fstest.c")));
}

static inline void
holy_term_register_output (const char *name __attribute__ ((unused)),
      holy_term_output_t term)
{
  if (holy_term_outputs)
    holy_list_push ((((char *) &(*&holy_term_outputs_disabled)->next == (char *) &((holy_list_t) (*&holy_term_outputs_disabled))->next) && ((char *) &(*&holy_term_outputs_disabled)->prev == (char *) &((holy_list_t) (*&holy_term_outputs_disabled))->prev) ? (holy_list_t *) (void *) &holy_term_outputs_disabled : (holy_list_t *) holy_bad_type_cast_real(266, "util/holy-fstest.c")),
      (((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(267, "util/holy-fstest.c")));
  else
    {

      if (! term->init || term->init (term) == holy_ERR_NONE)
 holy_list_push ((((char *) &(*&holy_term_outputs)->next == (char *) &((holy_list_t) (*&holy_term_outputs))->next) && ((char *) &(*&holy_term_outputs)->prev == (char *) &((holy_list_t) (*&holy_term_outputs))->prev) ? (holy_list_t *) (void *) &holy_term_outputs : (holy_list_t *) holy_bad_type_cast_real(272, "util/holy-fstest.c")),
   (((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(273, "util/holy-fstest.c")));
    }
}

static inline void
holy_term_register_output_inactive (const char *name __attribute__ ((unused)),
        holy_term_output_t term)
{
  holy_list_push ((((char *) &(*&holy_term_outputs_disabled)->next == (char *) &((holy_list_t) (*&holy_term_outputs_disabled))->next) && ((char *) &(*&holy_term_outputs_disabled)->prev == (char *) &((holy_list_t) (*&holy_term_outputs_disabled))->prev) ? (holy_list_t *) (void *) &holy_term_outputs_disabled : (holy_list_t *) holy_bad_type_cast_real(281, "util/holy-fstest.c")),
    (((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(282, "util/holy-fstest.c")));
}

static inline void
holy_term_register_output_active (const char *name __attribute__ ((unused)),
      holy_term_output_t term)
{
  if (! term->init || term->init (term) == holy_ERR_NONE)
    holy_list_push ((((char *) &(*&holy_term_outputs)->next == (char *) &((holy_list_t) (*&holy_term_outputs))->next) && ((char *) &(*&holy_term_outputs)->prev == (char *) &((holy_list_t) (*&holy_term_outputs))->prev) ? (holy_list_t *) (void *) &holy_term_outputs : (holy_list_t *) holy_bad_type_cast_real(290, "util/holy-fstest.c")),
      (((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(291, "util/holy-fstest.c")));
}

static inline void
holy_term_unregister_input (holy_term_input_t term)
{
  holy_list_remove ((((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(297, "util/holy-fstest.c")));
  holy_list_remove ((((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(298, "util/holy-fstest.c")));
}

static inline void
holy_term_unregister_output (holy_term_output_t term)
{
  holy_list_remove ((((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(304, "util/holy-fstest.c")));
  holy_list_remove ((((char *) &(term)->next == (char *) &((holy_list_t) (term))->next) && ((char *) &(term)->prev == (char *) &((holy_list_t) (term))->prev) ? (holy_list_t) term : (holy_list_t) holy_bad_type_cast_real(305, "util/holy-fstest.c")));
}






void holy_putcode (holy_uint32_t code, struct holy_term_output *term);
int holy_getkey (void);
int holy_getkey_noblock (void);
void holy_cls (void);
void holy_refresh (void);
void holy_puts_terminal (const char *str, struct holy_term_output *term);
struct holy_term_coordinate *holy_term_save_pos (void);
void holy_term_restore_pos (struct holy_term_coordinate *pos);

static inline unsigned holy_term_width (struct holy_term_output *term)
{
  return term->getwh(term).x ? : 80;
}

static inline unsigned holy_term_height (struct holy_term_output *term)
{
  return term->getwh(term).y ? : 24;
}

static inline struct holy_term_coordinate
holy_term_getxy (struct holy_term_output *term)
{
  return term->getxy (term);
}

static inline void
holy_term_refresh (struct holy_term_output *term)
{
  if (term->refresh)
    term->refresh (term);
}

static inline void
holy_term_gotoxy (struct holy_term_output *term, struct holy_term_coordinate pos)
{
  term->gotoxy (term, pos);
}

static inline void
holy_term_setcolorstate (struct holy_term_output *term,
    holy_term_color_state state)
{
  if (term->setcolorstate)
    term->setcolorstate (term, state);
}

static inline void
holy_setcolorstate (holy_term_color_state state)
{
  struct holy_term_output *term;

  for (((term)) = ((holy_term_outputs)); ((term)); ((term)) = ((term))->next)
    holy_term_setcolorstate (term, state);
}


static inline void
holy_term_setcursor (struct holy_term_output *term, int on)
{
  if (term->setcursor)
    term->setcursor (term, on);
}

static inline void
holy_term_cls (struct holy_term_output *term)
{
  if (term->cls)
    (term->cls) (term);
  else
    {
      holy_putcode ('\n', term);
      holy_term_refresh (term);
    }
}
# 395 "./include/holy/term.h"
static inline holy_size_t
holy_unicode_estimate_width (const struct holy_unicode_glyph *c __attribute__ ((unused)))
{
  if (holy_unicode_get_comb_type (c->base))
    return 0;
  return 1;
}





static inline holy_size_t
holy_term_getcharwidth (struct holy_term_output *term,
   const struct holy_unicode_glyph *c)
{
  if (c->base == '\t')
    return 8;

  if (term->getcharwidth)
    return term->getcharwidth (term, c);
  else if (((term->flags & (7 << 3))
     == (2 << 3))
    || ((term->flags & (7 << 3))
        == (3 << 3))
    || ((term->flags & (7 << 3))
        == (4 << 3)))
    return holy_unicode_estimate_width (c);
  else
    return 1;
}

struct holy_term_autoload
{
  struct holy_term_autoload *next;
  char *name;
  char *modname;
};

extern struct holy_term_autoload *holy_term_input_autoload;
extern struct holy_term_autoload *holy_term_output_autoload;

static inline void
holy_print_spaces (struct holy_term_output *term, int number_spaces)
{
  while (--number_spaces >= 0)
    holy_putcode (' ', term);
}

extern void (*holy_term_poll_usb) (int wait_for_completion);
# 17 "util/holy-fstest.c" 2

# 1 "./include/holy/lib/hexdump.h" 1
# 9 "./include/holy/lib/hexdump.h"
void hexdump (unsigned long bse,char* buf,int len);
# 19 "util/holy-fstest.c" 2

# 1 "./include/holy/command.h" 1
# 14 "./include/holy/command.h"
typedef enum holy_command_flags
  {

    holy_COMMAND_FLAG_EXTCMD = 0x10,

    holy_COMMAND_FLAG_DYNCMD = 0x20,

    holy_COMMAND_FLAG_BLOCKS = 0x40,

    holy_COMMAND_ACCEPT_DASH = 0x80,

    holy_COMMAND_OPTIONS_AT_START = 0x100,

    holy_COMMAND_FLAG_EXTRACTOR = 0x200
  } holy_command_flags_t;

struct holy_command;

typedef holy_err_t (*holy_command_func_t) (struct holy_command *cmd,
        int argc, char **argv);





struct holy_command
{

  struct holy_command *next;
  struct holy_command **prev;


  const char *name;


  int prio;


  holy_command_func_t func;


  holy_command_flags_t flags;


  const char *summary;


  const char *description;


  void *data;
};
typedef struct holy_command *holy_command_t;

extern holy_command_t holy_command_list;

holy_command_t
holy_register_command_prio (const char *name,
      holy_command_func_t func,
      const char *summary,
      const char *description,
      int prio);
void holy_unregister_command (holy_command_t cmd);

static inline holy_command_t
holy_register_command (const char *name,
         holy_command_func_t func,
         const char *summary,
         const char *description)
{
  return holy_register_command_prio (name, func, summary, description, 0);
}

static inline holy_command_t
holy_register_command_p1 (const char *name,
     holy_command_func_t func,
     const char *summary,
     const char *description)
{
  return holy_register_command_prio (name, func, summary, description, 1);
}

static inline holy_command_t
holy_command_find (const char *name)
{
  return holy_named_list_find (((((char *) &(holy_command_list)->next == (char *) &((holy_named_list_t) (holy_command_list))->next) && ((char *) &(holy_command_list)->prev == (char *) &((holy_named_list_t) (holy_command_list))->prev) && ((char *) &(holy_command_list)->name == (char *) &((holy_named_list_t) (holy_command_list))->name))? (holy_named_list_t) holy_command_list : (holy_named_list_t) holy_bad_type_cast_real(99, "util/holy-fstest.c")), name);
}

static inline holy_err_t
holy_command_execute (const char *name, int argc, char **argv)
{
  holy_command_t cmd;

  cmd = holy_command_find (name);
  return (cmd) ? cmd->func (cmd, argc, argv) : holy_ERR_FILE_NOT_FOUND;
}




void holy_register_core_commands (void);
# 21 "util/holy-fstest.c" 2

# 1 "./include/holy/zfs/zfs.h" 1
# 13 "./include/holy/zfs/zfs.h"
typedef enum holy_zfs_endian
  {
    holy_ZFS_UNKNOWN_ENDIAN = -2,
    holy_ZFS_LITTLE_ENDIAN = -1,
    holy_ZFS_BIG_ENDIAN = 0
  } holy_zfs_endian_t;
# 95 "./include/holy/zfs/zfs.h"
typedef enum pool_state {
 POOL_STATE_ACTIVE = 0,
 POOL_STATE_EXPORTED,
 POOL_STATE_DESTROYED,
 POOL_STATE_SPARE,
 POOL_STATE_L2CACHE,
 POOL_STATE_UNINITIALIZED,
 POOL_STATE_UNAVAIL,
 POOL_STATE_POTENTIALLY_ACTIVE
} pool_state_t;

struct holy_zfs_data;

holy_err_t holy_zfs_fetch_nvlist (holy_device_t dev, char **nvlist);
holy_err_t holy_zfs_getmdnobj (holy_device_t dev, const char *fsfilename,
          holy_uint64_t *mdnobj);

char *holy_zfs_nvlist_lookup_string (const char *nvlist, const char *name);
char *holy_zfs_nvlist_lookup_nvlist (const char *nvlist, const char *name);
int holy_zfs_nvlist_lookup_uint64 (const char *nvlist, const char *name,
       holy_uint64_t *out);
char *holy_zfs_nvlist_lookup_nvlist_array (const char *nvlist,
        const char *name,
        holy_size_t array_index);
int holy_zfs_nvlist_lookup_nvlist_array_get_nelm (const char *nvlist,
        const char *name);
holy_err_t
holy_zfs_add_key (holy_uint8_t *key_in,
    holy_size_t keylen,
    int passphrase);

extern holy_err_t (*holy_zfs_decrypt) (holy_crypto_cipher_handle_t cipher,
           holy_uint64_t algo,
           void *nonce,
           char *buf, holy_size_t size,
           const holy_uint32_t *expected_mac,
           holy_zfs_endian_t endian);

struct holy_zfs_key;

extern holy_crypto_cipher_handle_t (*holy_zfs_load_key) (const struct holy_zfs_key *key,
        holy_size_t keysize,
        holy_uint64_t salt,
        holy_uint64_t algo);
# 23 "util/holy-fstest.c" 2



# 1 "/usr/include/errno.h" 1 3 4
# 28 "/usr/include/errno.h" 3 4
# 1 "/usr/include/bits/errno.h" 1 3 4
# 26 "/usr/include/bits/errno.h" 3 4
# 1 "/usr/include/linux/errno.h" 1 3 4
# 1 "/usr/include/asm/errno.h" 1 3 4
# 1 "/usr/include/asm-generic/errno.h" 1 3 4




# 1 "/usr/include/asm-generic/errno-base.h" 1 3 4
# 6 "/usr/include/asm-generic/errno.h" 2 3 4
# 2 "/usr/include/asm/errno.h" 2 3 4
# 2 "/usr/include/linux/errno.h" 2 3 4
# 27 "/usr/include/bits/errno.h" 2 3 4
# 29 "/usr/include/errno.h" 2 3 4









# 37 "/usr/include/errno.h" 3 4
extern int *__errno_location (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));







extern char *program_invocation_name;
extern char *program_invocation_short_name;

# 1 "/usr/include/bits/types/error_t.h" 1 3 4
# 22 "/usr/include/bits/types/error_t.h" 3 4
typedef int error_t;
# 49 "/usr/include/errno.h" 2 3 4




# 27 "util/holy-fstest.c" 2
# 1 "/usr/include/string.h" 1 3 4
# 26 "/usr/include/string.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/string.h" 2 3 4






# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 34 "/usr/include/string.h" 2 3 4
# 43 "/usr/include/string.h" 3 4
extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
       size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern void *memmove (void *__dest, const void *__src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern void *memccpy (void *__restrict __dest, const void *__restrict __src,
        int __c, size_t __n)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__write_only__, 1, 4)));




extern void *memset (void *__s, int __c, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int memcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 80 "/usr/include/string.h" 3 4
extern int __memcmpeq (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 107 "/usr/include/string.h" 3 4
extern void *memchr (const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 120 "/usr/include/string.h" 3 4
extern void *rawmemchr (const void *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 133 "/usr/include/string.h" 3 4
extern void *memrchr (const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)))
      __attribute__ ((__access__ (__read_only__, 1, 3)));





extern char *strcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern char *strcat (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncat (char *__restrict __dest, const char *__restrict __src,
        size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern int strncmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcoll (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern size_t strxfrm (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 1, 3)));






extern int strcoll_l (const char *__s1, const char *__s2, locale_t __l)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));


extern size_t strxfrm_l (char *__dest, const char *__src, size_t __n,
    locale_t __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)))
     __attribute__ ((__access__ (__write_only__, 1, 3)));





extern char *strdup (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));






extern char *strndup (const char *__string, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));
# 246 "/usr/include/string.h" 3 4
extern char *strchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 273 "/usr/include/string.h" 3 4
extern char *strrchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 286 "/usr/include/string.h" 3 4
extern char *strchrnul (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));





extern size_t strcspn (const char *__s, const char *__reject)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern size_t strspn (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 323 "/usr/include/string.h" 3 4
extern char *strpbrk (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 350 "/usr/include/string.h" 3 4
extern char *strstr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));




extern char *strtok (char *__restrict __s, const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern char *__strtok_r (char *__restrict __s,
    const char *__restrict __delim,
    char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));

extern char *strtok_r (char *__restrict __s, const char *__restrict __delim,
         char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
# 380 "/usr/include/string.h" 3 4
extern char *strcasestr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));







extern void *memmem (const void *__haystack, size_t __haystacklen,
       const void *__needle, size_t __needlelen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 3)))
    __attribute__ ((__access__ (__read_only__, 1, 2)))
    __attribute__ ((__access__ (__read_only__, 3, 4)));



extern void *__mempcpy (void *__restrict __dest,
   const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern void *mempcpy (void *__restrict __dest,
        const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern size_t strlen (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));




extern size_t strnlen (const char *__string, size_t __maxlen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));




extern char *strerror (int __errnum) __attribute__ ((__nothrow__ , __leaf__));
# 444 "/usr/include/string.h" 3 4
extern char *strerror_r (int __errnum, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 2, 3)));




extern const char *strerrordesc_np (int __err) __attribute__ ((__nothrow__ , __leaf__));

extern const char *strerrorname_np (int __err) __attribute__ ((__nothrow__ , __leaf__));





extern char *strerror_l (int __errnum, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));



# 1 "/usr/include/strings.h" 1 3 4
# 23 "/usr/include/strings.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 24 "/usr/include/strings.h" 2 3 4










extern int bcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bcopy (const void *__src, void *__dest, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 68 "/usr/include/strings.h" 3 4
extern char *index (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 96 "/usr/include/strings.h" 3 4
extern char *rindex (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));






extern int ffs (int __i) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));





extern int ffsl (long int __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
__extension__ extern int ffsll (long long int __ll)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern int strcasecmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strncasecmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int strcasecmp_l (const char *__s1, const char *__s2, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));



extern int strncasecmp_l (const char *__s1, const char *__s2,
     size_t __n, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 4)));



# 463 "/usr/include/string.h" 2 3 4



extern void explicit_bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));



extern char *strsep (char **__restrict __stringp,
       const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern char *strsignal (int __sig) __attribute__ ((__nothrow__ , __leaf__));



extern const char *sigabbrev_np (int __sig) __attribute__ ((__nothrow__ , __leaf__));


extern const char *sigdescr_np (int __sig) __attribute__ ((__nothrow__ , __leaf__));



extern char *__stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern char *__stpncpy (char *__restrict __dest,
   const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern size_t strlcpy (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__write_only__, 1, 3)));



extern size_t strlcat (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__read_write__, 1, 3)));




extern int strverscmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern char *strfry (char *__string) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void *memfrob (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__read_write__, 1, 2)));
# 540 "/usr/include/string.h" 3 4
extern char *basename (const char *__filename) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 552 "/usr/include/string.h" 3 4

# 28 "util/holy-fstest.c" 2

# 1 "./holy-core/gnulib/progname.h" 1
# 20 "./holy-core/gnulib/progname.h"

# 20 "./holy-core/gnulib/progname.h"
extern const char *program_name;




extern void set_program_name (const char *argv0);
# 30 "util/holy-fstest.c" 2
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
# 1 "./holy-core/gnulib/argp.h" 1
# 10 "./holy-core/gnulib/argp.h"
# 1 "/usr/include/ctype.h" 1 3 4
# 28 "/usr/include/ctype.h" 3 4

# 46 "/usr/include/ctype.h" 3 4

# 46 "/usr/include/ctype.h" 3 4
enum
{
  _ISupper = ((0) < 8 ? ((1 << (0)) << 8) : ((1 << (0)) >> 8)),
  _ISlower = ((1) < 8 ? ((1 << (1)) << 8) : ((1 << (1)) >> 8)),
  _ISalpha = ((2) < 8 ? ((1 << (2)) << 8) : ((1 << (2)) >> 8)),
  _ISdigit = ((3) < 8 ? ((1 << (3)) << 8) : ((1 << (3)) >> 8)),
  _ISxdigit = ((4) < 8 ? ((1 << (4)) << 8) : ((1 << (4)) >> 8)),
  _ISspace = ((5) < 8 ? ((1 << (5)) << 8) : ((1 << (5)) >> 8)),
  _ISprint = ((6) < 8 ? ((1 << (6)) << 8) : ((1 << (6)) >> 8)),
  _ISgraph = ((7) < 8 ? ((1 << (7)) << 8) : ((1 << (7)) >> 8)),
  _ISblank = ((8) < 8 ? ((1 << (8)) << 8) : ((1 << (8)) >> 8)),
  _IScntrl = ((9) < 8 ? ((1 << (9)) << 8) : ((1 << (9)) >> 8)),
  _ISpunct = ((10) < 8 ? ((1 << (10)) << 8) : ((1 << (10)) >> 8)),
  _ISalnum = ((11) < 8 ? ((1 << (11)) << 8) : ((1 << (11)) >> 8))
};
# 79 "/usr/include/ctype.h" 3 4
extern const unsigned short int **__ctype_b_loc (void)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern const __int32_t **__ctype_tolower_loc (void)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
extern const __int32_t **__ctype_toupper_loc (void)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
# 108 "/usr/include/ctype.h" 3 4
extern int isalnum (int) __attribute__ ((__nothrow__ , __leaf__));
extern int isalpha (int) __attribute__ ((__nothrow__ , __leaf__));
extern int iscntrl (int) __attribute__ ((__nothrow__ , __leaf__));
extern int isdigit (int) __attribute__ ((__nothrow__ , __leaf__));
extern int islower (int) __attribute__ ((__nothrow__ , __leaf__));
extern int isgraph (int) __attribute__ ((__nothrow__ , __leaf__));
extern int isprint (int) __attribute__ ((__nothrow__ , __leaf__));
extern int ispunct (int) __attribute__ ((__nothrow__ , __leaf__));
extern int isspace (int) __attribute__ ((__nothrow__ , __leaf__));
extern int isupper (int) __attribute__ ((__nothrow__ , __leaf__));
extern int isxdigit (int) __attribute__ ((__nothrow__ , __leaf__));



extern int tolower (int __c) __attribute__ ((__nothrow__ , __leaf__));


extern int toupper (int __c) __attribute__ ((__nothrow__ , __leaf__));




extern int isblank (int) __attribute__ ((__nothrow__ , __leaf__));




extern int isctype (int __c, int __mask) __attribute__ ((__nothrow__ , __leaf__));






extern int isascii (int __c) __attribute__ ((__nothrow__ , __leaf__));



extern int toascii (int __c) __attribute__ ((__nothrow__ , __leaf__));



extern int _toupper (int) __attribute__ ((__nothrow__ , __leaf__));
extern int _tolower (int) __attribute__ ((__nothrow__ , __leaf__));
# 251 "/usr/include/ctype.h" 3 4
extern int isalnum_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int isalpha_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int iscntrl_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int isdigit_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int islower_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int isgraph_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int isprint_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int ispunct_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int isspace_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int isupper_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));
extern int isxdigit_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));

extern int isblank_l (int, locale_t) __attribute__ ((__nothrow__ , __leaf__));



extern int __tolower_l (int __c, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));
extern int tolower_l (int __c, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));


extern int __toupper_l (int __c, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));
extern int toupper_l (int __c, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));
# 327 "/usr/include/ctype.h" 3 4

# 11 "./holy-core/gnulib/argp.h" 2
# 1 "/usr/include/getopt.h" 1 3 4
# 36 "/usr/include/getopt.h" 3 4
# 1 "/usr/include/bits/getopt_ext.h" 1 3 4
# 27 "/usr/include/bits/getopt_ext.h" 3 4

# 50 "/usr/include/bits/getopt_ext.h" 3 4
struct option
{
  const char *name;


  int has_arg;
  int *flag;
  int val;
};







extern int getopt_long (int ___argc, char *const *___argv,
   const char *__shortopts,
          const struct option *__longopts, int *__longind)
       __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
extern int getopt_long_only (int ___argc, char *const *___argv,
        const char *__shortopts,
               const struct option *__longopts, int *__longind)
       __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));


# 37 "/usr/include/getopt.h" 2 3 4
# 12 "./holy-core/gnulib/argp.h" 2
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/limits.h" 1 3 4
# 34 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/limits.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/syslimits.h" 1 3 4






 
# 7 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/syslimits.h" 3 4
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/limits.h" 1 3 4
# 210 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/limits.h" 3 4
# 1 "/usr/include/limits.h" 1 3 4
# 26 "/usr/include/limits.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/limits.h" 2 3 4
# 199 "/usr/include/limits.h" 3 4
# 1 "/usr/include/bits/posix2_lim.h" 1 3 4
# 200 "/usr/include/limits.h" 2 3 4



# 1 "/usr/include/bits/xopen_lim.h" 1 3 4
# 64 "/usr/include/bits/xopen_lim.h" 3 4
# 1 "/usr/include/bits/uio_lim.h" 1 3 4
# 65 "/usr/include/bits/xopen_lim.h" 2 3 4
# 204 "/usr/include/limits.h" 2 3 4
# 211 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/limits.h" 2 3 4
# 10 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/syslimits.h" 2 3 4
#pragma GCC diagnostic pop
# 35 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/limits.h" 2 3 4
# 13 "./holy-core/gnulib/argp.h" 2
# 64 "./holy-core/gnulib/argp.h"

# 64 "./holy-core/gnulib/argp.h"
struct argp_option
{


  const char *name;



  int key;



  const char *arg;


  int flags;
# 88 "./holy-core/gnulib/argp.h"
  const char *doc;
# 97 "./holy-core/gnulib/argp.h"
  int group;
};
# 138 "./holy-core/gnulib/argp.h"
struct argp;
struct argp_state;
struct argp_child;


typedef error_t (*argp_parser_t) (int key, char *arg,
                                  struct argp_state *state);
# 212 "./holy-core/gnulib/argp.h"
struct argp
{


  const struct argp_option *options;
# 225 "./holy-core/gnulib/argp.h"
  argp_parser_t parser;






  const char *args_doc;






  const char *doc;







  const struct argp_child *children;
# 259 "./holy-core/gnulib/argp.h"
  char *(*help_filter) (int __key, const char *__text, void *__input);




  const char *argp_domain;
};
# 280 "./holy-core/gnulib/argp.h"
struct argp_child
{

  const struct argp *argp;


  int flags;





  const char *header;







  int group;
};



struct argp_state
{

  const struct argp *root_argp;


  int argc;
  char **argv;


  int next;


  unsigned flags;





  unsigned arg_num;




  int quoted;


  void *input;


  void **child_inputs;


  void *hook;



  char *name;


  FILE *err_stream;
  FILE *out_stream;

  void *pstate;
};
# 402 "./holy-core/gnulib/argp.h"
extern error_t argp_parse (const struct argp *__restrict __argp,
                           int , char **__restrict ,
                           unsigned __flags, int *__restrict __arg_index,
                           void *__restrict __input);
extern error_t __argp_parse (const struct argp *__restrict __argp,
                             int , char **__restrict ,
                             unsigned __flags, int *__restrict __arg_index,
                             void *__restrict __input);
# 431 "./holy-core/gnulib/argp.h"
extern const char *argp_program_version;






extern void (*argp_program_version_hook) (FILE *__restrict __stream,
                                          struct argp_state *__restrict
                                          __state);






extern const char *argp_program_bug_address;




extern error_t argp_err_exit_status;
# 485 "./holy-core/gnulib/argp.h"
extern void argp_help (const struct argp *__restrict __argp,
                       FILE *__restrict __stream,
                       unsigned __flags, char *__restrict __name);
extern void __argp_help (const struct argp *__restrict __argp,
                         FILE *__restrict __stream, unsigned __flags,
                         char *__name);
# 502 "./holy-core/gnulib/argp.h"
extern void argp_state_help (const struct argp_state *__restrict __state,
                             FILE *__restrict __stream,
                             unsigned int __flags);
extern void __argp_state_help (const struct argp_state *__restrict __state,
                               FILE *__restrict __stream,
                               unsigned int __flags);
# 518 "./holy-core/gnulib/argp.h"
extern void argp_error (const struct argp_state *__restrict __state,
                        const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));
extern void __argp_error (const struct argp_state *__restrict __state,
                          const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));
# 533 "./holy-core/gnulib/argp.h"
extern void argp_failure (const struct argp_state *__restrict __state,
                          int __status, int __errnum,
                          const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 4, 5)));
extern void __argp_failure (const struct argp_state *__restrict __state,
                            int __status, int __errnum,
                            const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 4, 5)));
# 555 "./holy-core/gnulib/argp.h"
extern void *_argp_input (const struct argp *__restrict __argp,
                          const struct argp_state *__restrict __state)
     
# 557 "./holy-core/gnulib/argp.h" 3 4
    __attribute__ ((__nothrow__ , __leaf__))
# 557 "./holy-core/gnulib/argp.h"
           ;
extern void *__argp_input (const struct argp *__restrict __argp,
                           const struct argp_state *__restrict __state)
     
# 560 "./holy-core/gnulib/argp.h" 3 4
    __attribute__ ((__nothrow__ , __leaf__))
# 560 "./holy-core/gnulib/argp.h"
           ;
# 569 "./holy-core/gnulib/argp.h"

# 569 "./holy-core/gnulib/argp.h"
#pragma GCC diagnostic push
# 569 "./holy-core/gnulib/argp.h"
 
# 569 "./holy-core/gnulib/argp.h"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
# 569 "./holy-core/gnulib/argp.h"
 
# 569 "./holy-core/gnulib/argp.h"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
# 600 "./holy-core/gnulib/argp.h"
inline void
argp_usage (const struct argp_state *__state)
{
  argp_state_help (__state, 
# 603 "./holy-core/gnulib/argp.h" 3 4
                             stderr
# 603 "./holy-core/gnulib/argp.h"
                                   , (0x02 | 0x04 | 0x100));
}

inline int

# 607 "./holy-core/gnulib/argp.h" 3 4
__attribute__ ((__nothrow__ , __leaf__)) 
# 607 "./holy-core/gnulib/argp.h"
_option_is_short (const struct argp_option *__opt)
{
  if (__opt->flags & 0x8)
    return 0;
  else
    {
      int __key = __opt->key;
      return __key > 0 && __key <= 
# 614 "./holy-core/gnulib/argp.h" 3 4
                                  (0x7f * 2 + 1) 
# 614 "./holy-core/gnulib/argp.h"
                                            && 
# 614 "./holy-core/gnulib/argp.h" 3 4
                                               ((*__ctype_b_loc ())[(int) ((
# 614 "./holy-core/gnulib/argp.h"
                                               __key
# 614 "./holy-core/gnulib/argp.h" 3 4
                                               ))] & (unsigned short int) _ISprint)
# 614 "./holy-core/gnulib/argp.h"
                                                              ;
    }
}

inline int

# 619 "./holy-core/gnulib/argp.h" 3 4
__attribute__ ((__nothrow__ , __leaf__)) 
# 619 "./holy-core/gnulib/argp.h"
_option_is_end (const struct argp_option *__opt)
{
  return !__opt->key && !__opt->name && !__opt->doc && !__opt->group;
}







# 629 "./holy-core/gnulib/argp.h"
#pragma GCC diagnostic pop
# 33 "util/holy-fstest.c" 2
 
# 33 "util/holy-fstest.c"
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

static holy_err_t
execute_command (const char *name, int n, char **args)
{
  holy_command_t cmd;

  cmd = holy_command_find (name);
  if (! cmd)
    holy_util_error (_("can't find command `%s'"), name);

  return (cmd->func) (cmd, n, args);
}

enum {
  CMD_LS = 1,
  CMD_CP,
  CMD_CAT,
  CMD_CMP,
  CMD_HEX,
  CMD_CRC,
  CMD_BLOCKLIST,
  CMD_TESTLOAD,
  CMD_ZFSINFO,
  CMD_XNU_UUID
};


static holy_disk_addr_t skip, leng;
static int uncompress = 0;

static void
read_file (char *pathname, int (*hook) (holy_off_t ofs, char *buf, int len, void *hook_arg), void *hook_arg)
{
  static char buf[32256];
  holy_file_t file;

  if ((pathname[0] == '-') && (pathname[1] == 0))
    {
      holy_device_t dev;

      dev = holy_device_open (0);
      if ((! dev) || (! dev->disk))
        holy_util_error ("%s", holy_errmsg);

      holy_util_info ("total sectors : %" "llu",
                      (unsigned long long) dev->disk->total_sectors);

      if (! leng)
        leng = (dev->disk->total_sectors << 9) - skip;

      while (leng)
        {
          holy_size_t len;

          len = (leng > 32256) ? 32256 : leng;

          if (holy_disk_read (dev->disk, 0, skip, len, buf))
     {
       char *msg = holy_xasprintf (_("disk read fails at offset %lld, length %lld"),
       (long long) skip, (long long) len);
       holy_util_error ("%s", msg);
     }

          if (hook (skip, buf, len, hook_arg))
            break;

          skip += len;
          leng -= len;
        }

      holy_device_close (dev);
      return;
    }

  if (uncompress == 0)
    holy_file_filter_disable_compression ();
  file = holy_file_open (pathname);
  if (!file)
    {
      holy_util_error (_("cannot open `%s': %s"), pathname,
         holy_errmsg);
      return;
    }

  holy_util_info ("file size : %" "llu",
    (unsigned long long) file->size);

  if (skip > file->size)
    {
      char *msg = holy_xasprintf (_("invalid skip value %lld"),
      (unsigned long long) skip);
      holy_util_error ("%s", msg);
      return;
    }

  {
    holy_off_t ofs, len;
    ofs = skip;
    len = file->size - skip;
    if ((leng) && (leng < len))
      len = leng;

    file->offset = skip;

    while (len)
      {
 holy_ssize_t sz;

 sz = holy_file_read (file, buf, (len > 32256) ? 32256 : len);
 if (sz < 0)
   {
     char *msg = holy_xasprintf (_("read error at offset %llu: %s"),
     (unsigned long long) ofs, holy_errmsg);
     holy_util_error ("%s", msg);
     break;
   }

 if ((sz == 0) || (hook (ofs, buf, sz, hook_arg)))
   break;

 ofs += sz;
 len -= sz;
      }
  }

  holy_file_close (file);
}

struct cp_hook_ctx
{
  FILE *ff;
  const char *dest;
};

static int
cp_hook (holy_off_t ofs, char *buf, int len, void *_ctx)
{
  struct cp_hook_ctx *ctx = _ctx;
  (void) ofs;

  if ((int) fwrite (buf, 1, len, ctx->ff) != len)
    {
      holy_util_error (_("cannot write to `%s': %s"),
         ctx->dest, strerror (
# 178 "util/holy-fstest.c" 3 4
                             (*__errno_location ())
# 178 "util/holy-fstest.c"
                                  ));
      return 1;
    }

  return 0;
}

static void
cmd_cp (char *src, const char *dest)
{
  struct cp_hook_ctx ctx =
    {
      .dest = dest
    };

  ctx.ff = holy_util_fopen (dest, "wb");
  if (ctx.ff == 
# 194 "util/holy-fstest.c" 3 4
               ((void *)0)
# 194 "util/holy-fstest.c"
                   )
    {
      holy_util_error (_("cannot open OS file `%s': %s"), dest,
         strerror (
# 197 "util/holy-fstest.c" 3 4
                  (*__errno_location ())
# 197 "util/holy-fstest.c"
                       ));
      return;
    }
  read_file (src, cp_hook, &ctx);
  fclose (ctx.ff);
}

static int
cat_hook (holy_off_t ofs, char *buf, int len, void *_arg __attribute__ ((unused)))
{
  (void) ofs;

  if ((int) fwrite (buf, 1, len, 
# 209 "util/holy-fstest.c" 3 4
                                stdout
# 209 "util/holy-fstest.c"
                                      ) != len)
    {
      holy_util_error (_("cannot write to the stdout: %s"),
         strerror (
# 212 "util/holy-fstest.c" 3 4
                  (*__errno_location ())
# 212 "util/holy-fstest.c"
                       ));
      return 1;
    }

  return 0;
}

static void
cmd_cat (char *src)
{
  read_file (src, cat_hook, 0);
}

static int
cmp_hook (holy_off_t ofs, char *buf, int len, void *ff_in)
{
  FILE *ff = ff_in;
  static char buf_1[32256];
  if ((int) fread (buf_1, 1, len, ff) != len)
    {
      char *msg = holy_xasprintf (_("read error at offset %llu: %s"),
      (unsigned long long) ofs, holy_errmsg);
      holy_util_error ("%s", msg);
      return 1;
    }

  if (holy_memcmp (buf, buf_1, len) != 0)
    {
      int i;

      for (i = 0; i < len; i++, ofs++)
 if (buf_1[i] != buf[i])
   {
     char *msg = holy_xasprintf (_("compare fail at offset %llu"),
     (unsigned long long) ofs);
     holy_util_error ("%s", msg);
     return 1;
   }
    }
  return 0;
}


static void
cmd_cmp (char *src, char *dest)
{
  FILE *ff;

  if (holy_util_is_directory (dest))
    {
      holy_util_fd_dir_t dir = holy_util_fd_opendir (dest);
      holy_util_fd_dirent_t entry;
      if (dir == 
# 264 "util/holy-fstest.c" 3 4
                ((void *)0)
# 264 "util/holy-fstest.c"
                    )
 {
   holy_util_error (_("OS file %s open error: %s"), dest,
      holy_util_fd_strerror ());
   return;
 }
      while ((entry = holy_util_fd_readdir (dir)))
 {
   char *srcnew, *destnew;
   char *ptr;
   if (strcmp (entry->d_name, ".") == 0
       || strcmp (entry->d_name, "..") == 0)
     continue;
   srcnew = xmalloc (strlen (src) + sizeof ("/")
       + strlen (entry->d_name));
   destnew = xmalloc (strlen (dest) + sizeof ("/")
       + strlen (entry->d_name));
   ptr = holy_stpcpy (srcnew, src);
   *ptr++ = '/';
   strcpy (ptr, entry->d_name);
   ptr = holy_stpcpy (destnew, dest);
   *ptr++ = '/';
   strcpy (ptr, entry->d_name);

   if (holy_util_is_special_file (destnew))
     continue;

   cmd_cmp (srcnew, destnew);
 }
      holy_util_fd_closedir (dir);
      return;
    }

  ff = holy_util_fopen (dest, "rb");
  if (ff == 
# 298 "util/holy-fstest.c" 3 4
           ((void *)0)
# 298 "util/holy-fstest.c"
               )
    {
      holy_util_error (_("OS file %s open error: %s"), dest,
         strerror (
# 301 "util/holy-fstest.c" 3 4
                  (*__errno_location ())
# 301 "util/holy-fstest.c"
                       ));
      return;
    }

  if ((skip) && (fseeko (ff, skip, 
# 305 "util/holy-fstest.c" 3 4
                                  0
# 305 "util/holy-fstest.c"
                                          )))
    holy_util_error (_("cannot seek `%s': %s"), dest,
       strerror (
# 307 "util/holy-fstest.c" 3 4
                (*__errno_location ())
# 307 "util/holy-fstest.c"
                     ));

  read_file (src, cmp_hook, ff);

  {
    holy_uint64_t pre;
    pre = ftell (ff);
    fseek (ff, 0, 
# 314 "util/holy-fstest.c" 3 4
                 2
# 314 "util/holy-fstest.c"
                         );
    if (pre != ftell (ff))
      holy_util_error ("%s", _("unexpected end of file"));
  }
  fclose (ff);
}

static int
hex_hook (holy_off_t ofs, char *buf, int len, void *arg __attribute__ ((unused)))
{
  hexdump (ofs, buf, len);
  return 0;
}

static void
cmd_hex (char *pathname)
{
  read_file (pathname, hex_hook, 0);
}

static int
crc_hook (holy_off_t ofs, char *buf, int len, void *crc_ctx)
{
  (void) ofs;

  ((const gcry_md_spec_t *) &_gcry_digest_spec_crc32)->write(crc_ctx, buf, len);
  return 0;
}

static void
cmd_crc (char *pathname)
{
  holy_uint8_t *crc32_context = xmalloc (((const gcry_md_spec_t *) &_gcry_digest_spec_crc32)->contextsize);
  ((const gcry_md_spec_t *) &_gcry_digest_spec_crc32)->init(crc32_context);

  read_file (pathname, crc_hook, crc32_context);
  ((const gcry_md_spec_t *) &_gcry_digest_spec_crc32)->final(crc32_context);
  printf ("%08x\n",
   holy_swap_bytes32(holy_get_unaligned32 (((const gcry_md_spec_t *) &_gcry_digest_spec_crc32)->read (crc32_context))));
  free (crc32_context);
}

static const char *root = 
# 356 "util/holy-fstest.c" 3 4
                         ((void *)0)
# 356 "util/holy-fstest.c"
                             ;
static int args_count = 0;
static int nparm = 0;
static int num_disks = 1;
static char **images = 
# 360 "util/holy-fstest.c" 3 4
                      ((void *)0)
# 360 "util/holy-fstest.c"
                          ;
static int cmd = 0;
static char *debug_str = 
# 362 "util/holy-fstest.c" 3 4
                        ((void *)0)
# 362 "util/holy-fstest.c"
                            ;
static char **args = 
# 363 "util/holy-fstest.c" 3 4
                    ((void *)0)
# 363 "util/holy-fstest.c"
                        ;
static int mount_crypt = 0;

static void
fstest (int n)
{
  char *host_file;
  char *loop_name;
  int i;

  for (i = 0; i < num_disks; i++)
    {
      char *argv[2];
      loop_name = holy_xasprintf ("loop%d", i);
      if (!loop_name)
 holy_util_error ("%s", holy_errmsg);

      host_file = holy_xasprintf ("(host)%s", images[i]);
      if (!host_file)
 holy_util_error ("%s", holy_errmsg);

      argv[0] = loop_name;
      argv[1] = host_file;

      if (execute_command ("loopback", 2, argv))
        holy_util_error (_("`loopback' command fails: %s"), holy_errmsg);

      holy_free (loop_name);
      holy_free (host_file);
    }

  {
    if (mount_crypt)
      {
 char *argv[2] = { xstrdup ("-a"), 
# 397 "util/holy-fstest.c" 3 4
                                  ((void *)0)
# 397 "util/holy-fstest.c"
                                      };
 if (execute_command ("cryptomount", 1, argv))
   holy_util_error (_("`cryptomount' command fails: %s"),
      holy_errmsg);
 free (argv[0]);
      }
  }

  holy_ldm_fini ();
  holy_lvm_fini ();
  holy_mdraid09_fini ();
  holy_mdraid1x_fini ();
  holy_diskfilter_fini ();
  holy_diskfilter_init ();
  holy_mdraid09_init ();
  holy_mdraid1x_init ();
  holy_lvm_init ();
  holy_ldm_init ();

  switch (cmd)
    {
    case CMD_LS:
      execute_command ("ls", n, args);
      break;
    case CMD_ZFSINFO:
      execute_command ("zfsinfo", n, args);
      break;
    case CMD_CP:
      cmd_cp (args[0], args[1]);
      break;
    case CMD_CAT:
      cmd_cat (args[0]);
      break;
    case CMD_CMP:
      cmd_cmp (args[0], args[1]);
      break;
    case CMD_HEX:
      cmd_hex (args[0]);
      break;
    case CMD_CRC:
      cmd_crc (args[0]);
      break;
    case CMD_BLOCKLIST:
      execute_command ("blocklist", n, args);
      holy_printf ("\n");
      break;
    case CMD_TESTLOAD:
      execute_command ("testload", n, args);
      holy_printf ("\n");
      break;
    case CMD_XNU_UUID:
      {
 holy_device_t dev;
 holy_fs_t fs;
 char *uuid = 0;
 char *argv[3] = { xstrdup ("-l"), 
# 452 "util/holy-fstest.c" 3 4
                                  ((void *)0)
# 452 "util/holy-fstest.c"
                                      , 
# 452 "util/holy-fstest.c" 3 4
                                        ((void *)0)
# 452 "util/holy-fstest.c"
                                            };
 dev = holy_device_open (n ? args[0] : 0);
 if (!dev)
   holy_util_error ("%s", holy_errmsg);
 fs = holy_fs_probe (dev);
 if (!fs)
   holy_util_error ("%s", holy_errmsg);
 if (!fs->uuid)
   holy_util_error ("%s", _("couldn't retrieve UUID"));
 if (fs->uuid (dev, &uuid))
   holy_util_error ("%s", holy_errmsg);
 if (!uuid)
   holy_util_error ("%s", _("couldn't retrieve UUID"));
 argv[1] = uuid;
 execute_command ("xnu_uuid", 2, argv);
 holy_free (argv[0]);
 holy_free (uuid);
 holy_device_close (dev);
      }
    }

  for (i = 0; i < num_disks; i++)
    {
      char *argv[2];

      loop_name = holy_xasprintf ("loop%d", i);
      if (!loop_name)
 holy_util_error ("%s", holy_errmsg);

      argv[0] = xstrdup ("-d");
      argv[1] = loop_name;

      execute_command ("loopback", 2, argv);

      holy_free (loop_name);
      holy_free (argv[0]);
    }
}

static struct argp_option options[] = {
  {0, 0, 0 , 0x8, "Commands:", 1},
  {"ls PATH", 0, 0 , 0x8, "List files in PATH.", 1},
  {"cp FILE LOCAL", 0, 0, 0x8, "Copy FILE to local file LOCAL.", 1},
  {"cat FILE", 0, 0 , 0x8, "Copy FILE to standard output.", 1},
  {"cmp FILE LOCAL", 0, 0, 0x8, "Compare FILE with local file LOCAL.", 1},
  {"hex FILE", 0, 0 , 0x8, "Show contents of FILE in hex.", 1},
  {"crc FILE", 0, 0 , 0x8, "Get crc32 checksum of FILE.", 1},
  {"blocklist FILE", 0, 0, 0x8, "Display blocklist of FILE.", 1},
  {"xnu_uuid DEVICE", 0, 0, 0x8, "Compute XNU UUID of the device.", 1},

  {"root", 'r', "DEVICE_NAME", 0, "Set root device.", 2},
  {"skip", 's', "NUM", 0, "Skip N bytes from output file.", 2},
  {"length", 'n', "NUM", 0, "Handle N bytes in output file.", 2},
  {"diskcount", 'c', "NUM", 0, "Specify the number of input files.", 2},
  {"debug", 'd', "STRING", 0, "Set debug environment variable.", 2},
  {"crypto", 'C', 
# 507 "util/holy-fstest.c" 3 4
                   ((void *)0)
# 507 "util/holy-fstest.c"
                       , 0, "Mount crypto devices.", 2},
  {"zfs-key", 'K',

   "FILE|prompt", 0, "Load zfs crypto key.", 2},
  {"verbose", 'v', 
# 511 "util/holy-fstest.c" 3 4
                    ((void *)0)
# 511 "util/holy-fstest.c"
                        , 0, "print verbose messages.", 2},
  {"uncompress", 'u', 
# 512 "util/holy-fstest.c" 3 4
                     ((void *)0)
# 512 "util/holy-fstest.c"
                         , 0, "Uncompress data.", 2},
  {0, 0, 0, 0, 0, 0}
};


static void
print_version (FILE *stream, struct argp_state *state)
{
  fprintf (stream, "%s (%s) %s\n", program_name, "holy", "2.02");
}
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  char *p;

  switch (key)
    {
    case 'r':
      root = arg;
      return 0;

    case 'K':
      if (strcmp (arg, "prompt") == 0)
 {
   char buf[1024];
   holy_puts_ ("Enter ZFS password: ");
   if (holy_password_get (buf, 1023))
     {
       holy_zfs_add_key ((holy_uint8_t *) buf, holy_strlen (buf), 1);
     }
 }
      else
      {
 FILE *f;
 ssize_t real_size;
 holy_uint8_t buf[1024];
 f = holy_util_fopen (arg, "rb");
 if (!f)
   {
     printf (_("%s: error:"), program_name);
     printf (_("cannot open `%s': %s"), arg, strerror (
# 554 "util/holy-fstest.c" 3 4
                                                      (*__errno_location ())
# 554 "util/holy-fstest.c"
                                                           ));
     printf ("\n");
     return 0;
   }
 real_size = fread (buf, 1, 1024, f);
 fclose (f);
 if (real_size < 0)
   {
     printf (_("%s: error:"), program_name);
     printf (_("cannot read `%s': %s"), arg, strerror (
# 563 "util/holy-fstest.c" 3 4
                                                      (*__errno_location ())
# 563 "util/holy-fstest.c"
                                                           ));
     printf ("\n");
     return 0;
   }
 holy_zfs_add_key (buf, real_size, 0);
      }
      return 0;

    case 'C':
      mount_crypt = 1;
      return 0;

    case 's':
      skip = holy_strtoul (arg, &p, 0);
      if (*p == 's')
 skip <<= 9;
      return 0;

    case 'n':
      leng = holy_strtoul (arg, &p, 0);
      if (*p == 's')
 leng <<= 9;
      return 0;

    case 'c':
      num_disks = holy_strtoul (arg, 
# 588 "util/holy-fstest.c" 3 4
                                    ((void *)0)
# 588 "util/holy-fstest.c"
                                        , 0);
      if (num_disks < 1)
 {
   fprintf (
# 591 "util/holy-fstest.c" 3 4
           stderr
# 591 "util/holy-fstest.c"
                 , "%s", _("Invalid disk count.\n"));
   argp_usage (state);
 }
      if (args_count != 0)
 {



   fprintf (
# 599 "util/holy-fstest.c" 3 4
           stderr
# 599 "util/holy-fstest.c"
                 , "%s", _("Disk count must precede disks list.\n"));
   argp_usage (state);
 }
      return 0;

    case 'd':
      debug_str = arg;
      return 0;

    case 'v':
      verbosity++;
      return 0;

    case 'u':
      uncompress = 1;
      return 0;

    case 0x1000001:
      if (args_count < num_disks)
 {
   fprintf (
# 619 "util/holy-fstest.c" 3 4
           stderr
# 619 "util/holy-fstest.c"
                 , "%s", _("No command is specified.\n"));
   argp_usage (state);
 }
      if (args_count - 1 - num_disks < nparm)
 {
   fprintf (
# 624 "util/holy-fstest.c" 3 4
           stderr
# 624 "util/holy-fstest.c"
                 , "%s", _("Not enough parameters to command.\n"));
   argp_usage (state);
 }
      return 0;

    case 0:
      break;

    default:
      return 
# 633 "util/holy-fstest.c" 3 4
            7
# 633 "util/holy-fstest.c"
                            ;
    }

  if (args_count < num_disks)
    {
      if (args_count == 0)
 images = xmalloc (num_disks * sizeof (images[0]));
      images[args_count] = holy_canonicalize_file_name (arg);
      args_count++;
      return 0;
    }

  if (args_count == num_disks)
    {
      if (!holy_strcmp (arg, "ls"))
        {
          cmd = CMD_LS;
        }
      else if (!holy_strcmp (arg, "zfsinfo"))
        {
          cmd = CMD_ZFSINFO;
        }
      else if (!holy_strcmp (arg, "cp"))
 {
   cmd = CMD_CP;
          nparm = 2;
 }
      else if (!holy_strcmp (arg, "cat"))
 {
   cmd = CMD_CAT;
   nparm = 1;
 }
      else if (!holy_strcmp (arg, "cmp"))
 {
   cmd = CMD_CMP;
          nparm = 2;
 }
      else if (!holy_strcmp (arg, "hex"))
 {
   cmd = CMD_HEX;
          nparm = 1;
 }
      else if (!holy_strcmp (arg, "crc"))
 {
   cmd = CMD_CRC;
          nparm = 1;
 }
      else if (!holy_strcmp (arg, "blocklist"))
 {
   cmd = CMD_BLOCKLIST;
          nparm = 1;
 }
      else if (!holy_strcmp (arg, "testload"))
 {
   cmd = CMD_TESTLOAD;
          nparm = 1;
 }
      else if (holy_strcmp (arg, "xnu_uuid") == 0)
 {
   cmd = CMD_XNU_UUID;
   nparm = 0;
 }
      else
 {
   fprintf (
# 697 "util/holy-fstest.c" 3 4
           stderr
# 697 "util/holy-fstest.c"
                 , _("Invalid command %s.\n"), arg);
   argp_usage (state);
 }
      args_count++;
      return 0;
    }

  args[args_count - 1 - num_disks] = xstrdup (arg);
  args_count++;
  return 0;
}

struct argp argp = {
  options, argp_parser, "IMAGE_PATH COMMANDS",
  "Debug tool for filesystem driver.",
  
# 712 "util/holy-fstest.c" 3 4
 ((void *)0)
# 712 "util/holy-fstest.c"
     , 
# 712 "util/holy-fstest.c" 3 4
       ((void *)0)
# 712 "util/holy-fstest.c"
           , 
# 712 "util/holy-fstest.c" 3 4
             ((void *)0)

# 713 "util/holy-fstest.c"
};

int
main (int argc, char *argv[])
{
  const char *default_root;
  char *alloc_root;

  holy_util_host_init (&argc, &argv);

  args = xmalloc (argc * sizeof (args[0]));

  argp_parse (&argp, argc, argv, 0, 0, 0);


  holy_init_all ();
  holy_gcry_init_all ();

  if (debug_str)
    holy_env_set ("debug", debug_str);

  default_root = (num_disks == 1) ? "loop0" : "md0";
  alloc_root = 0;
  if (root)
    {
      if ((*root >= '0') && (*root <= '9'))
        {
          alloc_root = xmalloc (strlen (default_root) + strlen (root) + 2);

          sprintf (alloc_root, "%s,%s", default_root, root);
          root = alloc_root;
        }
    }
  else
    root = default_root;

  holy_env_set ("root", root);

  if (alloc_root)
    free (alloc_root);


  fstest (args_count - 1 - num_disks);


  holy_gcry_fini_all ();
  holy_fini_all ();

  return 0;
}
# 0 "holy-core/kern/emu/hostfs.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "holy-core/kern/emu/hostfs.c"





# 1 "./config-util.h" 1
# 7 "holy-core/kern/emu/hostfs.c" 2

# 1 "./include/holy/fs.h" 1
# 9 "./include/holy/fs.h"
# 1 "./include/holy/device.h" 1
# 9 "./include/holy/device.h"
# 1 "./include/holy/symbol.h" 1
# 9 "./include/holy/symbol.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/symbol.h" 2
# 10 "./include/holy/device.h" 2
# 1 "./include/holy/err.h" 1
# 13 "./include/holy/err.h"
typedef enum
  {
    holy_ERR_NONE = 0,
    holy_ERR_TEST_FAILURE,
    holy_ERR_BAD_MODULE,
    holy_ERR_OUT_OF_MEMORY,
    holy_ERR_BAD_FILE_TYPE,
    holy_ERR_FILE_NOT_FOUND,
    holy_ERR_FILE_READ_ERROR,
    holy_ERR_BAD_FILENAME,
    holy_ERR_UNKNOWN_FS,
    holy_ERR_BAD_FS,
    holy_ERR_BAD_NUMBER,
    holy_ERR_OUT_OF_RANGE,
    holy_ERR_UNKNOWN_DEVICE,
    holy_ERR_BAD_DEVICE,
    holy_ERR_READ_ERROR,
    holy_ERR_WRITE_ERROR,
    holy_ERR_UNKNOWN_COMMAND,
    holy_ERR_INVALID_COMMAND,
    holy_ERR_BAD_ARGUMENT,
    holy_ERR_BAD_PART_TABLE,
    holy_ERR_UNKNOWN_OS,
    holy_ERR_BAD_OS,
    holy_ERR_NO_KERNEL,
    holy_ERR_BAD_FONT,
    holy_ERR_NOT_IMPLEMENTED_YET,
    holy_ERR_SYMLINK_LOOP,
    holy_ERR_BAD_COMPRESSED_DATA,
    holy_ERR_MENU,
    holy_ERR_TIMEOUT,
    holy_ERR_IO,
    holy_ERR_ACCESS_DENIED,
    holy_ERR_EXTRACTOR,
    holy_ERR_NET_BAD_ADDRESS,
    holy_ERR_NET_ROUTE_LOOP,
    holy_ERR_NET_NO_ROUTE,
    holy_ERR_NET_NO_ANSWER,
    holy_ERR_NET_NO_CARD,
    holy_ERR_WAIT,
    holy_ERR_BUG,
    holy_ERR_NET_PORT_CLOSED,
    holy_ERR_NET_INVALID_RESPONSE,
    holy_ERR_NET_UNKNOWN_ERROR,
    holy_ERR_NET_PACKET_TOO_BIG,
    holy_ERR_NET_NO_DOMAIN,
    holy_ERR_EOF,
    holy_ERR_BAD_SIGNATURE
  }
holy_err_t;

struct holy_error_saved
{
  holy_err_t holy_errno;
  char errmsg[256];
};

extern holy_err_t holy_errno;
extern char holy_errmsg[256];

holy_err_t holy_error (holy_err_t n, const char *fmt, ...);
void holy_fatal (const char *fmt, ...) __attribute__ ((noreturn));
void holy_error_push (void);
int holy_error_pop (void);
void holy_print_error (void);
extern int holy_err_printed_errors;
int holy_err_printf (const char *fmt, ...)
     __attribute__ ((format (__printf__, 1, 2)));
# 11 "./include/holy/device.h" 2

struct holy_disk;
struct holy_net;

struct holy_device
{
  struct holy_disk *disk;
  struct holy_net *net;
};
typedef struct holy_device *holy_device_t;

typedef int (*holy_device_iterate_hook_t) (const char *name, void *data);

holy_device_t holy_device_open (const char *name);
holy_err_t holy_device_close (holy_device_t device);
int holy_device_iterate (holy_device_iterate_hook_t hook,
          void *hook_data);
# 10 "./include/holy/fs.h" 2

# 1 "./include/holy/types.h" 1
# 9 "./include/holy/types.h"
# 1 "./include/holy/emu/config.h" 1
# 9 "./include/holy/emu/config.h"
# 1 "./include/holy/disk.h" 1
# 9 "./include/holy/disk.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/disk.h" 2



# 1 "./include/holy/types.h" 1
# 14 "./include/holy/disk.h" 2


# 1 "./include/holy/mm.h" 1
# 11 "./include/holy/mm.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 12 "./include/holy/mm.h" 2





typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long long int holy_int64_t;
typedef holy_int64_t holy_ssize;

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


void holy_mm_init_region (void *addr, holy_size_t size);
void *holy_malloc (holy_size_t size);
void *holy_zalloc (holy_size_t size);
void holy_free (void *ptr);
void *holy_realloc (void *ptr, holy_size_t size);

void *holy_memalign (holy_size_t align, holy_size_t size);


void holy_mm_check_real (const char *file, int line);
# 17 "./include/holy/disk.h" 2



enum holy_disk_dev_id
  {
    holy_DISK_DEVICE_BIOSDISK_ID,
    holy_DISK_DEVICE_OFDISK_ID,
    holy_DISK_DEVICE_LOOPBACK_ID,
    holy_DISK_DEVICE_EFIDISK_ID,
    holy_DISK_DEVICE_DISKFILTER_ID,
    holy_DISK_DEVICE_HOST_ID,
    holy_DISK_DEVICE_ATA_ID,
    holy_DISK_DEVICE_MEMDISK_ID,
    holy_DISK_DEVICE_NAND_ID,
    holy_DISK_DEVICE_SCSI_ID,
    holy_DISK_DEVICE_CRYPTODISK_ID,
    holy_DISK_DEVICE_ARCDISK_ID,
    holy_DISK_DEVICE_HOSTDISK_ID,
    holy_DISK_DEVICE_PROCFS_ID,
    holy_DISK_DEVICE_CBFSDISK_ID,
    holy_DISK_DEVICE_UBOOTDISK_ID,
    holy_DISK_DEVICE_XEN,
  };
# 95 "./include/holy/disk.h"
struct holy_disk;

struct holy_disk_memberlist;


typedef enum
  {
    holy_DISK_PULL_NONE,
    holy_DISK_PULL_REMOVABLE,
    holy_DISK_PULL_RESCAN,
    holy_DISK_PULL_MAX
  } holy_disk_pull_t;

typedef int (*holy_disk_dev_iterate_hook_t) (const char *name, void *data);


struct holy_disk_dev
{

  const char *name;


  enum holy_disk_dev_id id;


  int (*iterate) (holy_disk_dev_iterate_hook_t hook, void *hook_data,
    holy_disk_pull_t pull);


  holy_err_t (*open) (const char *name, struct holy_disk *disk);


  void (*close) (struct holy_disk *disk);


  holy_err_t (*read) (struct holy_disk *disk, holy_disk_addr_t sector,
        holy_size_t size, char *buf);


  holy_err_t (*write) (struct holy_disk *disk, holy_disk_addr_t sector,
         holy_size_t size, const char *buf);


  struct holy_disk_memberlist *(*memberlist) (struct holy_disk *disk);
  const char * (*raidname) (struct holy_disk *disk);



  struct holy_disk_dev *next;
};
typedef struct holy_disk_dev *holy_disk_dev_t;

extern holy_disk_dev_t holy_disk_dev_list;

struct holy_partition;

typedef long int holy_disk_addr_t;

typedef void (*holy_disk_read_hook_t) (holy_disk_addr_t sector,
           unsigned offset, unsigned length,
           void *data);


struct holy_disk
{

  const char *name;


  holy_disk_dev_t dev;


  holy_uint64_t total_sectors;


  unsigned int log_sector_size;


  unsigned int max_agglomerate;


  unsigned long id;


  struct holy_partition *partition;



  holy_disk_read_hook_t read_hook;


  void *read_hook_data;


  void *data;
};
typedef struct holy_disk *holy_disk_t;


struct holy_disk_memberlist
{
  holy_disk_t disk;
  struct holy_disk_memberlist *next;
};
typedef struct holy_disk_memberlist *holy_disk_memberlist_t;
# 220 "./include/holy/disk.h"
void holy_disk_cache_invalidate_all (void);

void holy_disk_dev_register (holy_disk_dev_t dev);
void holy_disk_dev_unregister (holy_disk_dev_t dev);
static inline int
holy_disk_dev_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data)
{
  holy_disk_dev_t p;
  holy_disk_pull_t pull;

  for (pull = 0; pull < holy_DISK_PULL_MAX; pull++)
    for (p = holy_disk_dev_list; p; p = p->next)
      if (p->iterate && (p->iterate) (hook, hook_data, pull))
 return 1;

  return 0;
}

holy_disk_t holy_disk_open (const char *name);
void holy_disk_close (holy_disk_t disk);
holy_err_t holy_disk_read (holy_disk_t disk,
     holy_disk_addr_t sector,
     holy_off_t offset,
     holy_size_t size,
     void *buf);
holy_err_t holy_disk_write (holy_disk_t disk,
       holy_disk_addr_t sector,
       holy_off_t offset,
       holy_size_t size,
       const void *buf);
extern holy_err_t (*holy_disk_write_weak) (holy_disk_t disk,
             holy_disk_addr_t sector,
             holy_off_t offset,
             holy_size_t size,
             const void *buf);


holy_uint64_t holy_disk_get_size (holy_disk_t disk);






extern void (* holy_disk_firmware_fini) (void);
extern int holy_disk_firmware_is_tainted;

static inline void
holy_stop_disk_firmware (void)
{

  holy_disk_firmware_is_tainted = 1;
  if (holy_disk_firmware_fini)
    {
      holy_disk_firmware_fini ();
      holy_disk_firmware_fini = ((void *) 0);
    }
}


struct holy_disk_cache
{
  enum holy_disk_dev_id dev_id;
  unsigned long disk_id;
  holy_disk_addr_t sector;
  char *data;
  int lock;
};

extern struct holy_disk_cache holy_disk_cache_table[1021];


void holy_lvm_init (void);
void holy_ldm_init (void);
void holy_mdraid09_init (void);
void holy_mdraid1x_init (void);
void holy_diskfilter_init (void);
void holy_lvm_fini (void);
void holy_ldm_fini (void);
void holy_mdraid09_fini (void);
void holy_mdraid1x_fini (void);
void holy_diskfilter_fini (void);
# 10 "./include/holy/emu/config.h" 2
# 1 "./include/holy/partition.h" 1
# 9 "./include/holy/partition.h"
# 1 "./include/holy/dl.h" 1
# 13 "./include/holy/dl.h"
# 1 "./include/holy/elf.h" 1
# 13 "./include/holy/elf.h"
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;

typedef holy_int64_t holy_ssize_t;





typedef holy_uint16_t Elf32_Half;
typedef holy_uint16_t Elf64_Half;


typedef holy_uint32_t Elf32_Word;
typedef holy_int32_t Elf32_Sword;
typedef holy_uint32_t Elf64_Word;
typedef holy_int32_t Elf64_Sword;


typedef holy_uint64_t Elf32_Xword;
typedef holy_int64_t Elf32_Sxword;
typedef holy_uint64_t Elf64_Xword;
typedef holy_int64_t Elf64_Sxword;


typedef holy_uint32_t Elf32_Addr;
typedef holy_uint64_t Elf64_Addr;


typedef holy_uint32_t Elf32_Off;
typedef holy_uint64_t Elf64_Off;


typedef holy_uint16_t Elf32_Section;
typedef holy_uint16_t Elf64_Section;


typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;






typedef struct
{
  unsigned char e_ident[(16)];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
  unsigned char e_ident[(16)];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;
# 268 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct
{
  Elf64_Word sh_name;
  Elf64_Word sh_type;
  Elf64_Xword sh_flags;
  Elf64_Addr sh_addr;
  Elf64_Off sh_offset;
  Elf64_Xword sh_size;
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Xword sh_addralign;
  Elf64_Xword sh_entsize;
} Elf64_Shdr;
# 367 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Section st_shndx;
} Elf32_Sym;

typedef struct
{
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Section st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
} Elf64_Sym;




typedef struct
{
  Elf32_Half si_boundto;
  Elf32_Half si_flags;
} Elf32_Syminfo;

typedef struct
{
  Elf64_Half si_boundto;
  Elf64_Half si_flags;
} Elf64_Syminfo;
# 481 "./include/holy/elf.h"
typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
} Elf32_Rel;






typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
} Elf64_Rel;



typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
  Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
  Elf64_Sxword r_addend;
} Elf64_Rela;
# 526 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;

typedef struct
{
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;
# 605 "./include/holy/elf.h"
typedef struct
{
  Elf32_Sword d_tag;
  union
    {
      Elf32_Word d_val;
      Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct
{
  Elf64_Sxword d_tag;
  union
    {
      Elf64_Xword d_val;
      Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;
# 769 "./include/holy/elf.h"
typedef struct
{
  Elf32_Half vd_version;
  Elf32_Half vd_flags;
  Elf32_Half vd_ndx;
  Elf32_Half vd_cnt;
  Elf32_Word vd_hash;
  Elf32_Word vd_aux;
  Elf32_Word vd_next;

} Elf32_Verdef;

typedef struct
{
  Elf64_Half vd_version;
  Elf64_Half vd_flags;
  Elf64_Half vd_ndx;
  Elf64_Half vd_cnt;
  Elf64_Word vd_hash;
  Elf64_Word vd_aux;
  Elf64_Word vd_next;

} Elf64_Verdef;
# 811 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word vda_name;
  Elf32_Word vda_next;

} Elf32_Verdaux;

typedef struct
{
  Elf64_Word vda_name;
  Elf64_Word vda_next;

} Elf64_Verdaux;




typedef struct
{
  Elf32_Half vn_version;
  Elf32_Half vn_cnt;
  Elf32_Word vn_file;

  Elf32_Word vn_aux;
  Elf32_Word vn_next;

} Elf32_Verneed;

typedef struct
{
  Elf64_Half vn_version;
  Elf64_Half vn_cnt;
  Elf64_Word vn_file;

  Elf64_Word vn_aux;
  Elf64_Word vn_next;

} Elf64_Verneed;
# 858 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word vna_hash;
  Elf32_Half vna_flags;
  Elf32_Half vna_other;
  Elf32_Word vna_name;
  Elf32_Word vna_next;

} Elf32_Vernaux;

typedef struct
{
  Elf64_Word vna_hash;
  Elf64_Half vna_flags;
  Elf64_Half vna_other;
  Elf64_Word vna_name;
  Elf64_Word vna_next;

} Elf64_Vernaux;
# 892 "./include/holy/elf.h"
typedef struct
{
  int a_type;
  union
    {
      long int a_val;
      void *a_ptr;
      void (*a_fcn) (void);
    } a_un;
} Elf32_auxv_t;

typedef struct
{
  long int a_type;
  union
    {
      long int a_val;
      void *a_ptr;
      void (*a_fcn) (void);
    } a_un;
} Elf64_auxv_t;
# 955 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
} Elf32_Nhdr;

typedef struct
{
  Elf64_Word n_namesz;
  Elf64_Word n_descsz;
  Elf64_Word n_type;
} Elf64_Nhdr;
# 1002 "./include/holy/elf.h"
typedef struct
{
  Elf32_Xword m_value;
  Elf32_Word m_info;
  Elf32_Word m_poffset;
  Elf32_Half m_repeat;
  Elf32_Half m_stride;
} Elf32_Move;

typedef struct
{
  Elf64_Xword m_value;
  Elf64_Xword m_info;
  Elf64_Xword m_poffset;
  Elf64_Half m_repeat;
  Elf64_Half m_stride;
} Elf64_Move;
# 1367 "./include/holy/elf.h"
typedef union
{
  struct
    {
      Elf32_Word gt_current_g_value;
      Elf32_Word gt_unused;
    } gt_header;
  struct
    {
      Elf32_Word gt_g_value;
      Elf32_Word gt_bytes;
    } gt_entry;
} Elf32_gptab;



typedef struct
{
  Elf32_Word ri_gprmask;
  Elf32_Word ri_cprmask[4];
  Elf32_Sword ri_gp_value;
} Elf32_RegInfo;



typedef struct
{
  unsigned char kind;

  unsigned char size;
  Elf32_Section section;

  Elf32_Word info;
} Elf_Options;
# 1443 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word hwp_flags1;
  Elf32_Word hwp_flags2;
} Elf_Options_Hw;
# 1582 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word l_name;
  Elf32_Word l_time_stamp;
  Elf32_Word l_checksum;
  Elf32_Word l_version;
  Elf32_Word l_flags;
} Elf32_Lib;

typedef struct
{
  Elf64_Word l_name;
  Elf64_Word l_time_stamp;
  Elf64_Word l_checksum;
  Elf64_Word l_version;
  Elf64_Word l_flags;
} Elf64_Lib;
# 1613 "./include/holy/elf.h"
typedef Elf32_Addr Elf32_Conflict;
# 14 "./include/holy/dl.h" 2
# 1 "./include/holy/list.h" 1
# 11 "./include/holy/list.h"
# 1 "./include/holy/compiler.h" 1
# 12 "./include/holy/list.h" 2

struct holy_list
{
  struct holy_list *next;
  struct holy_list **prev;
};
typedef struct holy_list *holy_list_t;

void holy_list_push (holy_list_t *head, holy_list_t item);
void holy_list_remove (holy_list_t item);




static inline void *
holy_bad_type_cast_real (int line, const char *file)
     __attribute__ ((__error__ ("bad type cast between incompatible holy types")));

static inline void *
holy_bad_type_cast_real (int line, const char *file)
{
  holy_fatal ("error:%s:%u: bad type cast between incompatible holy types",
       file, line);
}
# 50 "./include/holy/list.h"
struct holy_named_list
{
  struct holy_named_list *next;
  struct holy_named_list **prev;
  char *name;
};
typedef struct holy_named_list *holy_named_list_t;

void * holy_named_list_find (holy_named_list_t head,
       const char *name);
# 15 "./include/holy/dl.h" 2
# 1 "./include/holy/misc.h" 1
# 9 "./include/holy/misc.h"
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 1 3 4
# 40 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4

# 40 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 103 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
# 10 "./include/holy/misc.h" 2



# 1 "./include/holy/i18n.h" 1
# 9 "./include/holy/i18n.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/i18n.h" 2






# 15 "./include/holy/i18n.h"
extern const char *(*holy_gettext) (const char *s) __attribute__ ((format_arg (1)));



# 1 "/usr/include/locale.h" 1 3 4
# 25 "/usr/include/locale.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 415 "/usr/include/features.h" 3 4
# 1 "/usr/include/features-time64.h" 1 3 4
# 20 "/usr/include/features-time64.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 21 "/usr/include/features-time64.h" 2 3 4
# 1 "/usr/include/bits/timesize.h" 1 3 4
# 19 "/usr/include/bits/timesize.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 20 "/usr/include/bits/timesize.h" 2 3 4
# 22 "/usr/include/features-time64.h" 2 3 4
# 416 "/usr/include/features.h" 2 3 4
# 524 "/usr/include/features.h" 3 4
# 1 "/usr/include/sys/cdefs.h" 1 3 4
# 730 "/usr/include/sys/cdefs.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 731 "/usr/include/sys/cdefs.h" 2 3 4
# 1 "/usr/include/bits/long-double.h" 1 3 4
# 732 "/usr/include/sys/cdefs.h" 2 3 4
# 525 "/usr/include/features.h" 2 3 4
# 548 "/usr/include/features.h" 3 4
# 1 "/usr/include/gnu/stubs.h" 1 3 4
# 10 "/usr/include/gnu/stubs.h" 3 4
# 1 "/usr/include/gnu/stubs-64.h" 1 3 4
# 11 "/usr/include/gnu/stubs.h" 2 3 4
# 549 "/usr/include/features.h" 2 3 4
# 26 "/usr/include/locale.h" 2 3 4


# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 29 "/usr/include/locale.h" 2 3 4
# 1 "/usr/include/bits/locale.h" 1 3 4
# 30 "/usr/include/locale.h" 2 3 4


# 51 "/usr/include/locale.h" 3 4

# 51 "/usr/include/locale.h" 3 4
struct lconv
{


  char *decimal_point;
  char *thousands_sep;





  char *grouping;





  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char int_frac_digits;
  char frac_digits;

  char p_cs_precedes;

  char p_sep_by_space;

  char n_cs_precedes;

  char n_sep_by_space;






  char p_sign_posn;
  char n_sign_posn;


  char int_p_cs_precedes;

  char int_p_sep_by_space;

  char int_n_cs_precedes;

  char int_n_sep_by_space;






  char int_p_sign_posn;
  char int_n_sign_posn;
# 118 "/usr/include/locale.h" 3 4
};



extern char *setlocale (int __category, const char *__locale) __attribute__ ((__nothrow__ , __leaf__));


extern struct lconv *localeconv (void) __attribute__ ((__nothrow__ , __leaf__));
# 135 "/usr/include/locale.h" 3 4
# 1 "/usr/include/bits/types/locale_t.h" 1 3 4
# 22 "/usr/include/bits/types/locale_t.h" 3 4
# 1 "/usr/include/bits/types/__locale_t.h" 1 3 4
# 27 "/usr/include/bits/types/__locale_t.h" 3 4
struct __locale_struct
{

  struct __locale_data *__locales[13];


  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;


  const char *__names[13];
};

typedef struct __locale_struct *__locale_t;
# 23 "/usr/include/bits/types/locale_t.h" 2 3 4

typedef __locale_t locale_t;
# 136 "/usr/include/locale.h" 2 3 4





extern locale_t newlocale (int __category_mask, const char *__locale,
      locale_t __base) __attribute__ ((__nothrow__ , __leaf__));
# 176 "/usr/include/locale.h" 3 4
extern locale_t duplocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));



extern void freelocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));






extern locale_t uselocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));








# 20 "./include/holy/i18n.h" 2
# 1 "/usr/include/libintl.h" 1 3 4
# 34 "/usr/include/libintl.h" 3 4





extern char *gettext (const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (1)));



extern char *dgettext (const char *__domainname, const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));
extern char *__dgettext (const char *__domainname, const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));



extern char *dcgettext (const char *__domainname,
   const char *__msgid, int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));
extern char *__dcgettext (const char *__domainname,
     const char *__msgid, int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));




extern char *ngettext (const char *__msgid1, const char *__msgid2,
         unsigned long int __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (1))) __attribute__ ((__format_arg__ (2)));



extern char *dngettext (const char *__domainname, const char *__msgid1,
   const char *__msgid2, unsigned long int __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2))) __attribute__ ((__format_arg__ (3)));



extern char *dcngettext (const char *__domainname, const char *__msgid1,
    const char *__msgid2, unsigned long int __n,
    int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2))) __attribute__ ((__format_arg__ (3)));





extern char *textdomain (const char *__domainname) __attribute__ ((__nothrow__ , __leaf__));



extern char *bindtextdomain (const char *__domainname,
        const char *__dirname) __attribute__ ((__nothrow__ , __leaf__));



extern char *bind_textdomain_codeset (const char *__domainname,
          const char *__codeset) __attribute__ ((__nothrow__ , __leaf__));
# 121 "/usr/include/libintl.h" 3 4

# 21 "./include/holy/i18n.h" 2
# 40 "./include/holy/i18n.h"

# 40 "./include/holy/i18n.h"
static inline const char * __attribute__ ((always_inline,format_arg (1)))
_ (const char *str)
{
  return gettext(str);
}
# 14 "./include/holy/misc.h" 2
# 32 "./include/holy/misc.h"
typedef unsigned long long int holy_size_t;
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;
typedef holy_int64_t holy_ssize_t;


void *holy_memmove (void *dest, const void *src, holy_size_t n);
char *holy_strcpy (char *dest, const char *src);

static inline char *
holy_strncpy (char *dest, const char *src, int c)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0' && --c)
    ;

  return dest;
}

static inline char *
holy_stpcpy (char *dest, const char *src)
{
  char *d = dest;
  const char *s = src;

  do
    *d++ = *s;
  while (*s++ != '\0');

  return d - 1;
}


static inline void *
holy_memcpy (void *dest, const void *src, holy_size_t n)
{
  return holy_memmove (dest, src, n);
}
# 87 "./include/holy/misc.h"
int holy_memcmp (const void *s1, const void *s2, holy_size_t n);
int holy_strcmp (const char *s1, const char *s2);
int holy_strncmp (const char *s1, const char *s2, holy_size_t n);

char *holy_strchr (const char *s, int c);
char *holy_strrchr (const char *s, int c);
int holy_strword (const char *s, const char *w);



static inline char *
holy_strstr (const char *haystack, const char *needle)
{





  if (*needle != '\0')
    {


      char b = *needle++;

      for (;; haystack++)
 {
   if (*haystack == '\0')

     return 0;
   if (*haystack == b)

     {
       const char *rhaystack = haystack + 1;
       const char *rneedle = needle;

       for (;; rhaystack++, rneedle++)
  {
    if (*rneedle == '\0')

      return (char *) haystack;
    if (*rhaystack == '\0')

      return 0;
    if (*rhaystack != *rneedle)

      break;
  }
     }
 }
    }
  else
    return (char *) haystack;
}

int holy_isspace (int c);

static inline int
holy_isprint (int c)
{
  return (c >= ' ' && c <= '~');
}

static inline int
holy_iscntrl (int c)
{
  return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

static inline int
holy_isalpha (int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int
holy_islower (int c)
{
  return (c >= 'a' && c <= 'z');
}

static inline int
holy_isupper (int c)
{
  return (c >= 'A' && c <= 'Z');
}

static inline int
holy_isgraph (int c)
{
  return (c >= '!' && c <= '~');
}

static inline int
holy_isdigit (int c)
{
  return (c >= '0' && c <= '9');
}

static inline int
holy_isxdigit (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int
holy_isalnum (int c)
{
  return holy_isalpha (c) || holy_isdigit (c);
}

static inline int
holy_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';

  return c;
}

static inline int
holy_toupper (int c)
{
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 'A';

  return c;
}

static inline int
holy_strcasecmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (holy_tolower ((holy_uint8_t) *s1)
   != holy_tolower ((holy_uint8_t) *s2))
 break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

static inline int
holy_strncasecmp (const char *s1, const char *s2, holy_size_t n)
{
  if (n == 0)
    return 0;

  while (*s1 && *s2 && --n)
    {
      if (holy_tolower (*s1) != holy_tolower (*s2))
 break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

unsigned long holy_strtoul (const char *str, char **end, int base);
unsigned long long holy_strtoull (const char *str, char **end, int base);

static inline long
holy_strtol (const char *str, char **end, int base)
{
  int negative = 0;
  unsigned long long magnitude;

  while (*str && holy_isspace (*str))
    str++;

  if (*str == '-')
    {
      negative = 1;
      str++;
    }

  magnitude = holy_strtoull (str, end, base);
  if (negative)
    {
      if (magnitude > (unsigned long) (9223372036854775807L) + 1)
        {
          holy_error (holy_ERR_OUT_OF_RANGE, "overflow is detected");
          return (-9223372036854775807L - 1);
        }
      return -((long) magnitude);
    }
  else
    {
      if (magnitude > (9223372036854775807L))
        {
          holy_error (holy_ERR_OUT_OF_RANGE, "overflow is detected");
          return (9223372036854775807L);
        }
      return (long) magnitude;
    }
}

char *holy_strdup (const char *s) __attribute__ ((warn_unused_result));
char *holy_strndup (const char *s, holy_size_t n) __attribute__ ((warn_unused_result));
void *holy_memset (void *s, int c, holy_size_t n);
holy_size_t holy_strlen (const char *s) __attribute__ ((warn_unused_result));
int holy_printf (const char *fmt, ...) __attribute__ ((format (gnu_printf, 1, 2)));
int holy_printf_ (const char *fmt, ...) __attribute__ ((format (gnu_printf, 1, 2)));



static inline char *
holy_strchrsub (char *output, const char *input, char ch, const char *with)
{
  while (*input)
    {
      if (*input == ch)
 {
   holy_strcpy (output, with);
   output += holy_strlen (with);
   input++;
   continue;
 }
      *output++ = *input++;
    }
  *output = '\0';
  return output;
}

extern void (*holy_xputs) (const char *str);

static inline int
holy_puts (const char *s)
{
  const char nl[2] = "\n";
  holy_xputs (s);
  holy_xputs (nl);

  return 1;
}

int holy_puts_ (const char *s);
void holy_real_dprintf (const char *file,
                                     const int line,
                                     const char *condition,
                                     const char *fmt, ...) __attribute__ ((format (gnu_printf, 4, 5)));
int holy_vprintf (const char *fmt, va_list args);
int holy_snprintf (char *str, holy_size_t n, const char *fmt, ...)
     __attribute__ ((format (gnu_printf, 3, 4)));
int holy_vsnprintf (char *str, holy_size_t n, const char *fmt,
     va_list args);
char *holy_xasprintf (const char *fmt, ...)
     __attribute__ ((format (gnu_printf, 1, 2))) __attribute__ ((warn_unused_result));
char *holy_xvasprintf (const char *fmt, va_list args) __attribute__ ((warn_unused_result));
void holy_exit (void) __attribute__ ((noreturn));
holy_uint64_t holy_divmod64 (holy_uint64_t n,
       holy_uint64_t d,
       holy_uint64_t *r);
# 363 "./include/holy/misc.h"
holy_int64_t
holy_divmod64s (holy_int64_t n,
     holy_int64_t d,
     holy_int64_t *r);

holy_uint32_t
holy_divmod32 (holy_uint32_t n,
     holy_uint32_t d,
     holy_uint32_t *r);

holy_int32_t
holy_divmod32s (holy_int32_t n,
      holy_int32_t d,
      holy_int32_t *r);



static inline char *
holy_memchr (const void *p, int c, holy_size_t len)
{
  const char *s = (const char *) p;
  const char *e = s + len;

  for (; s < e; s++)
    if (*s == c)
      return (char *) s;

  return 0;
}


static inline unsigned int
holy_abs (int x)
{
  if (x < 0)
    return (unsigned int) (-x);
  else
    return (unsigned int) x;
}





void holy_reboot (void) __attribute__ ((noreturn));
# 421 "./include/holy/misc.h"
void holy_halt (void) __attribute__ ((noreturn));
# 431 "./include/holy/misc.h"
static inline void
holy_error_save (struct holy_error_saved *save)
{
  holy_memcpy (save->errmsg, holy_errmsg, sizeof (save->errmsg));
  save->holy_errno = holy_errno;
  holy_errno = holy_ERR_NONE;
}

static inline void
holy_error_load (const struct holy_error_saved *save)
{
  holy_memcpy (holy_errmsg, save->errmsg, sizeof (holy_errmsg));
  holy_errno = save->holy_errno;
}
# 16 "./include/holy/dl.h" 2
# 141 "./include/holy/dl.h"
struct holy_dl_segment
{
  struct holy_dl_segment *next;
  void *addr;
  holy_size_t size;
  unsigned section;
};
typedef struct holy_dl_segment *holy_dl_segment_t;

struct holy_dl;

struct holy_dl_dep
{
  struct holy_dl_dep *next;
  struct holy_dl *mod;
};
typedef struct holy_dl_dep *holy_dl_dep_t;
# 184 "./include/holy/dl.h"
typedef struct holy_dl *holy_dl_t;

holy_dl_t holy_dl_load_file (const char *filename);
holy_dl_t holy_dl_load (const char *name);
holy_dl_t holy_dl_load_core (void *addr, holy_size_t size);
holy_dl_t holy_dl_load_core_noinit (void *addr, holy_size_t size);
int holy_dl_unload (holy_dl_t mod);
void holy_dl_unload_unneeded (void);
int holy_dl_ref (holy_dl_t mod);
int holy_dl_unref (holy_dl_t mod);
extern holy_dl_t holy_dl_head;
# 231 "./include/holy/dl.h"
holy_err_t holy_dl_register_symbol (const char *name, void *addr,
        int isfunc, holy_dl_t mod);

holy_err_t holy_arch_dl_check_header (void *ehdr);
# 249 "./include/holy/dl.h"
holy_err_t
holy_ia64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
     holy_size_t *got);
holy_err_t
holy_arm64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
      holy_size_t *got);
# 263 "./include/holy/dl.h"
holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
     holy_size_t *got);
# 10 "./include/holy/partition.h" 2



typedef unsigned long long int holy_uint64_t;
# 68 "./include/holy/partition.h"
struct holy_disk;

typedef struct holy_partition *holy_partition_t;


typedef enum
{
  holy_EMBED_PCBIOS
} holy_embed_type_t;


typedef int (*holy_partition_iterate_hook_t) (struct holy_disk *disk,
           const holy_partition_t partition,
           void *data);


struct holy_partition_map
{

  struct holy_partition_map *next;
  struct holy_partition_map **prev;


  const char *name;


  holy_err_t (*iterate) (struct holy_disk *disk,
    holy_partition_iterate_hook_t hook, void *hook_data);


  holy_err_t (*embed) (struct holy_disk *disk, unsigned int *nsectors,
         unsigned int max_nsectors,
         holy_embed_type_t embed_type,
         holy_disk_addr_t **sectors);

};
typedef struct holy_partition_map *holy_partition_map_t;


struct holy_partition
{

  int number;


  unsigned long long int start;


  unsigned long long int len;


  unsigned long long int offset;


  int index;


  struct holy_partition *parent;


  holy_partition_map_t partmap;



  unsigned char msdostype;
};

holy_partition_t holy_partition_probe (struct holy_disk *disk,
          const char *str);
int holy_partition_iterate (struct holy_disk *disk,
      holy_partition_iterate_hook_t hook,
      void *hook_data);
char *holy_partition_get_name (const holy_partition_t partition);


extern holy_partition_map_t holy_partition_map_list;


static inline void
holy_partition_map_register (holy_partition_map_t partmap)
{
  holy_list_push ((((char *) &(*&holy_partition_map_list)->next == (char *) &((holy_list_t) (*&holy_partition_map_list))->next) && ((char *) &(*&holy_partition_map_list)->prev == (char *) &((holy_list_t) (*&holy_partition_map_list))->prev) ? (holy_list_t *) (void *) &holy_partition_map_list : (holy_list_t *) holy_bad_type_cast_real(149, "util/holy-fstest.c")),
    (((char *) &(partmap)->next == (char *) &((holy_list_t) (partmap))->next) && ((char *) &(partmap)->prev == (char *) &((holy_list_t) (partmap))->prev) ? (holy_list_t) partmap : (holy_list_t) holy_bad_type_cast_real(150, "util/holy-fstest.c")));
}


static inline void
holy_partition_map_unregister (holy_partition_map_t partmap)
{
  holy_list_remove ((((char *) &(partmap)->next == (char *) &((holy_list_t) (partmap))->next) && ((char *) &(partmap)->prev == (char *) &((holy_list_t) (partmap))->prev) ? (holy_list_t) partmap : (holy_list_t) holy_bad_type_cast_real(157, "util/holy-fstest.c")));
}




static inline holy_disk_addr_t
holy_partition_get_start (const holy_partition_t p)
{
  holy_partition_t part;
  holy_uint64_t part_start = 0;

  for (part = p; part; part = part->parent)
    part_start += part->start;

  return part_start;
}

static inline holy_uint64_t
holy_partition_get_len (const holy_partition_t p)
{
  return p->len;
}
# 11 "./include/holy/emu/config.h" 2
# 1 "./include/holy/emu/hostfile.h" 1
# 11 "./include/holy/emu/hostfile.h"
# 1 "/usr/include/sys/types.h" 1 3 4
# 27 "/usr/include/sys/types.h" 3 4


# 1 "/usr/include/bits/types.h" 1 3 4
# 27 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 28 "/usr/include/bits/types.h" 2 3 4
# 1 "/usr/include/bits/timesize.h" 1 3 4
# 19 "/usr/include/bits/timesize.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 20 "/usr/include/bits/timesize.h" 2 3 4
# 29 "/usr/include/bits/types.h" 2 3 4



# 31 "/usr/include/bits/types.h" 3 4
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;






typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;



typedef long int __quad_t;
typedef unsigned long int __u_quad_t;







typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
# 141 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/typesizes.h" 1 3 4
# 142 "/usr/include/bits/types.h" 2 3 4
# 1 "/usr/include/bits/time64.h" 1 3 4
# 143 "/usr/include/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;




typedef int __sig_atomic_t;
# 30 "/usr/include/sys/types.h" 2 3 4



typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;


typedef __loff_t loff_t;






typedef __ino64_t ino_t;




typedef __ino64_t ino64_t;




typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;







typedef __off64_t off_t;




typedef __off64_t off64_t;




typedef __pid_t pid_t;





typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;




# 1 "/usr/include/bits/types/clock_t.h" 1 3 4






typedef __clock_t clock_t;
# 127 "/usr/include/sys/types.h" 2 3 4

# 1 "/usr/include/bits/types/clockid_t.h" 1 3 4






typedef __clockid_t clockid_t;
# 129 "/usr/include/sys/types.h" 2 3 4
# 1 "/usr/include/bits/types/time_t.h" 1 3 4
# 10 "/usr/include/bits/types/time_t.h" 3 4
typedef __time_t time_t;
# 130 "/usr/include/sys/types.h" 2 3 4
# 1 "/usr/include/bits/types/timer_t.h" 1 3 4






typedef __timer_t timer_t;
# 131 "/usr/include/sys/types.h" 2 3 4



typedef __useconds_t useconds_t;



typedef __suseconds_t suseconds_t;





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 229 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 145 "/usr/include/sys/types.h" 2 3 4



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;




# 1 "/usr/include/bits/stdint-intn.h" 1 3 4
# 24 "/usr/include/bits/stdint-intn.h" 3 4
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
# 156 "/usr/include/sys/types.h" 2 3 4


typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;


typedef int register_t __attribute__ ((__mode__ (__word__)));
# 176 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 24 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/endian.h" 1 3 4
# 35 "/usr/include/bits/endian.h" 3 4
# 1 "/usr/include/bits/endianness.h" 1 3 4
# 36 "/usr/include/bits/endian.h" 2 3 4
# 25 "/usr/include/endian.h" 2 3 4
# 35 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/byteswap.h" 1 3 4
# 33 "/usr/include/bits/byteswap.h" 3 4
static __inline __uint16_t
__bswap_16 (__uint16_t __bsx)
{

  return __builtin_bswap16 (__bsx);



}






static __inline __uint32_t
__bswap_32 (__uint32_t __bsx)
{

  return __builtin_bswap32 (__bsx);



}
# 69 "/usr/include/bits/byteswap.h" 3 4
__extension__ static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{

  return __builtin_bswap64 (__bsx);



}
# 36 "/usr/include/endian.h" 2 3 4
# 1 "/usr/include/bits/uintn-identity.h" 1 3 4
# 32 "/usr/include/bits/uintn-identity.h" 3 4
static __inline __uint16_t
__uint16_identity (__uint16_t __x)
{
  return __x;
}

static __inline __uint32_t
__uint32_identity (__uint32_t __x)
{
  return __x;
}

static __inline __uint64_t
__uint64_identity (__uint64_t __x)
{
  return __x;
}
# 37 "/usr/include/endian.h" 2 3 4
# 177 "/usr/include/sys/types.h" 2 3 4


# 1 "/usr/include/sys/select.h" 1 3 4
# 30 "/usr/include/sys/select.h" 3 4
# 1 "/usr/include/bits/select.h" 1 3 4
# 31 "/usr/include/sys/select.h" 2 3 4


# 1 "/usr/include/bits/types/sigset_t.h" 1 3 4



# 1 "/usr/include/bits/types/__sigset_t.h" 1 3 4




typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
# 5 "/usr/include/bits/types/sigset_t.h" 2 3 4


typedef __sigset_t sigset_t;
# 34 "/usr/include/sys/select.h" 2 3 4



# 1 "/usr/include/bits/types/struct_timeval.h" 1 3 4







struct timeval
{




  __time_t tv_sec;
  __suseconds_t tv_usec;

};
# 38 "/usr/include/sys/select.h" 2 3 4

# 1 "/usr/include/bits/types/struct_timespec.h" 1 3 4
# 11 "/usr/include/bits/types/struct_timespec.h" 3 4
struct timespec
{



  __time_t tv_sec;




  __syscall_slong_t tv_nsec;
# 31 "/usr/include/bits/types/struct_timespec.h" 3 4
};
# 40 "/usr/include/sys/select.h" 2 3 4
# 49 "/usr/include/sys/select.h" 3 4
typedef long int __fd_mask;
# 59 "/usr/include/sys/select.h" 3 4
typedef struct
  {



    __fd_mask fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];





  } fd_set;






typedef __fd_mask fd_mask;
# 91 "/usr/include/sys/select.h" 3 4

# 102 "/usr/include/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 127 "/usr/include/sys/select.h" 3 4
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);
# 153 "/usr/include/sys/select.h" 3 4

# 180 "/usr/include/sys/types.h" 2 3 4





typedef __blksize_t blksize_t;
# 205 "/usr/include/sys/types.h" 3 4
typedef __blkcnt64_t blkcnt_t;



typedef __fsblkcnt64_t fsblkcnt_t;



typedef __fsfilcnt64_t fsfilcnt_t;





typedef __blkcnt64_t blkcnt64_t;
typedef __fsblkcnt64_t fsblkcnt64_t;
typedef __fsfilcnt64_t fsfilcnt64_t;





# 1 "/usr/include/bits/pthreadtypes.h" 1 3 4
# 23 "/usr/include/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/bits/thread-shared-types.h" 1 3 4
# 44 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/pthreadtypes-arch.h" 1 3 4
# 21 "/usr/include/bits/pthreadtypes-arch.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 22 "/usr/include/bits/pthreadtypes-arch.h" 2 3 4
# 45 "/usr/include/bits/thread-shared-types.h" 2 3 4

# 1 "/usr/include/bits/atomic_wide_counter.h" 1 3 4
# 25 "/usr/include/bits/atomic_wide_counter.h" 3 4
typedef union
{
  __extension__ unsigned long long int __value64;
  struct
  {
    unsigned int __low;
    unsigned int __high;
  } __value32;
} __atomic_wide_counter;
# 47 "/usr/include/bits/thread-shared-types.h" 2 3 4




typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;

typedef struct __pthread_internal_slist
{
  struct __pthread_internal_slist *__next;
} __pthread_slist_t;
# 76 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/struct_mutex.h" 1 3 4
# 22 "/usr/include/bits/struct_mutex.h" 3 4
struct __pthread_mutex_s
{
  int __lock;
  unsigned int __count;
  int __owner;

  unsigned int __nusers;



  int __kind;

  short __spins;
  short __elision;
  __pthread_list_t __list;
# 53 "/usr/include/bits/struct_mutex.h" 3 4
};
# 77 "/usr/include/bits/thread-shared-types.h" 2 3 4
# 89 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/struct_rwlock.h" 1 3 4
# 23 "/usr/include/bits/struct_rwlock.h" 3 4
struct __pthread_rwlock_arch_t
{
  unsigned int __readers;
  unsigned int __writers;
  unsigned int __wrphase_futex;
  unsigned int __writers_futex;
  unsigned int __pad3;
  unsigned int __pad4;

  int __cur_writer;
  int __shared;
  signed char __rwelision;




  unsigned char __pad1[7];


  unsigned long int __pad2;


  unsigned int __flags;
# 55 "/usr/include/bits/struct_rwlock.h" 3 4
};
# 90 "/usr/include/bits/thread-shared-types.h" 2 3 4




struct __pthread_cond_s
{
  __atomic_wide_counter __wseq;
  __atomic_wide_counter __g1_start;
  unsigned int __g_size[2] ;
  unsigned int __g1_orig_size;
  unsigned int __wrefs;
  unsigned int __g_signals[2];
  unsigned int __unused_initialized_1;
  unsigned int __unused_initialized_2;
};

typedef unsigned int __tss_t;
typedef unsigned long int __thrd_t;

typedef struct
{
  int __data ;
} __once_flag;
# 24 "/usr/include/bits/pthreadtypes.h" 2 3 4



typedef unsigned long int pthread_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;



typedef unsigned int pthread_key_t;



typedef int pthread_once_t;


union pthread_attr_t
{
  char __size[56];
  long int __align;
};

typedef union pthread_attr_t pthread_attr_t;




typedef union
{
  struct __pthread_mutex_s __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;


typedef union
{
  struct __pthread_cond_s __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;





typedef union
{
  struct __pthread_rwlock_arch_t __data;
  char __size[56];
  long int __align;
} pthread_rwlock_t;

typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;





typedef volatile int pthread_spinlock_t;




typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;
# 228 "/usr/include/sys/types.h" 2 3 4



# 12 "./include/holy/emu/hostfile.h" 2
# 1 "./include/holy/osdep/hostfile.h" 1
# 11 "./include/holy/osdep/hostfile.h"
# 1 "./include/holy/osdep/hostfile_unix.h" 1
# 9 "./include/holy/osdep/hostfile_unix.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/osdep/hostfile_unix.h" 2






# 1 "/usr/include/sys/stat.h" 1 3 4
# 99 "/usr/include/sys/stat.h" 3 4


# 1 "/usr/include/bits/stat.h" 1 3 4
# 25 "/usr/include/bits/stat.h" 3 4
# 1 "/usr/include/bits/struct_stat.h" 1 3 4
# 26 "/usr/include/bits/struct_stat.h" 3 4
struct stat
  {



    __dev_t st_dev;




    __ino_t st_ino;







    __nlink_t st_nlink;
    __mode_t st_mode;

    __uid_t st_uid;
    __gid_t st_gid;

    int __pad0;

    __dev_t st_rdev;




    __off_t st_size;



    __blksize_t st_blksize;

    __blkcnt_t st_blocks;
# 74 "/usr/include/bits/struct_stat.h" 3 4
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 89 "/usr/include/bits/struct_stat.h" 3 4
    __syscall_slong_t __glibc_reserved[3];
# 99 "/usr/include/bits/struct_stat.h" 3 4
  };



struct stat64
  {



    __dev_t st_dev;

    __ino64_t st_ino;
    __nlink_t st_nlink;
    __mode_t st_mode;






    __uid_t st_uid;
    __gid_t st_gid;

    int __pad0;
    __dev_t st_rdev;
    __off_t st_size;





    __blksize_t st_blksize;
    __blkcnt64_t st_blocks;







    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 151 "/usr/include/bits/struct_stat.h" 3 4
    __syscall_slong_t __glibc_reserved[3];




  };
# 26 "/usr/include/bits/stat.h" 2 3 4
# 102 "/usr/include/sys/stat.h" 2 3 4
# 227 "/usr/include/sys/stat.h" 3 4
extern int stat (const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "stat64") __attribute__ ((__nothrow__ , __leaf__))

     __attribute__ ((__nonnull__ (1, 2)));
extern int fstat (int __fd, struct stat *__buf) __asm__ ("" "fstat64") __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__nonnull__ (2)));
# 240 "/usr/include/sys/stat.h" 3 4
extern int stat64 (const char *__restrict __file,
     struct stat64 *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int fstat64 (int __fd, struct stat64 *__buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
# 279 "/usr/include/sys/stat.h" 3 4
extern int fstatat (int __fd, const char *__restrict __file, struct stat *__restrict __buf, int __flag) __asm__ ("" "fstatat64") __attribute__ ((__nothrow__ , __leaf__))


                 __attribute__ ((__nonnull__ (3)));
# 291 "/usr/include/sys/stat.h" 3 4
extern int fstatat64 (int __fd, const char *__restrict __file,
        struct stat64 *__restrict __buf, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 327 "/usr/include/sys/stat.h" 3 4
extern int lstat (const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "lstat64") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1, 2)));







extern int lstat64 (const char *__restrict __file,
      struct stat64 *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
# 352 "/usr/include/sys/stat.h" 3 4
extern int chmod (const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int lchmod (const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fchmod (int __fd, __mode_t __mode) __attribute__ ((__nothrow__ , __leaf__));





extern int fchmodat (int __fd, const char *__file, __mode_t __mode,
       int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;






extern __mode_t umask (__mode_t __mask) __attribute__ ((__nothrow__ , __leaf__));




extern __mode_t getumask (void) __attribute__ ((__nothrow__ , __leaf__));



extern int mkdir (const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mkdirat (int __fd, const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));






extern int mknod (const char *__path, __mode_t __mode, __dev_t __dev)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mknodat (int __fd, const char *__path, __mode_t __mode,
      __dev_t __dev) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));





extern int mkfifo (const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mkfifoat (int __fd, const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));






extern int utimensat (int __fd, const char *__path,
        const struct timespec __times[2],
        int __flags)
     __attribute__ ((__nothrow__ , __leaf__));
# 452 "/usr/include/sys/stat.h" 3 4
extern int futimens (int __fd, const struct timespec __times[2]) __attribute__ ((__nothrow__ , __leaf__));
# 465 "/usr/include/sys/stat.h" 3 4
# 1 "/usr/include/bits/statx.h" 1 3 4
# 31 "/usr/include/bits/statx.h" 3 4
# 1 "/usr/include/linux/stat.h" 1 3 4




# 1 "/usr/include/linux/types.h" 1 3 4




# 1 "/usr/include/asm/types.h" 1 3 4
# 1 "/usr/include/asm-generic/types.h" 1 3 4






# 1 "/usr/include/asm-generic/int-ll64.h" 1 3 4
# 12 "/usr/include/asm-generic/int-ll64.h" 3 4
# 1 "/usr/include/asm/bitsperlong.h" 1 3 4
# 11 "/usr/include/asm/bitsperlong.h" 3 4
# 1 "/usr/include/asm-generic/bitsperlong.h" 1 3 4
# 12 "/usr/include/asm/bitsperlong.h" 2 3 4
# 13 "/usr/include/asm-generic/int-ll64.h" 2 3 4







typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;


__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
# 8 "/usr/include/asm-generic/types.h" 2 3 4
# 2 "/usr/include/asm/types.h" 2 3 4
# 6 "/usr/include/linux/types.h" 2 3 4



# 1 "/usr/include/linux/posix_types.h" 1 3 4




# 1 "/usr/include/linux/stddef.h" 1 3 4
# 6 "/usr/include/linux/posix_types.h" 2 3 4
# 25 "/usr/include/linux/posix_types.h" 3 4
typedef struct {
 unsigned long fds_bits[1024 / (8 * sizeof(long))];
} __kernel_fd_set;


typedef void (*__kernel_sighandler_t)(int);


typedef int __kernel_key_t;
typedef int __kernel_mqd_t;

# 1 "/usr/include/asm/posix_types.h" 1 3 4






# 1 "/usr/include/asm/posix_types_64.h" 1 3 4
# 11 "/usr/include/asm/posix_types_64.h" 3 4
typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;


typedef unsigned long __kernel_old_dev_t;


# 1 "/usr/include/asm-generic/posix_types.h" 1 3 4
# 15 "/usr/include/asm-generic/posix_types.h" 3 4
typedef long __kernel_long_t;
typedef unsigned long __kernel_ulong_t;



typedef __kernel_ulong_t __kernel_ino_t;



typedef unsigned int __kernel_mode_t;



typedef int __kernel_pid_t;



typedef int __kernel_ipc_pid_t;



typedef unsigned int __kernel_uid_t;
typedef unsigned int __kernel_gid_t;



typedef __kernel_long_t __kernel_suseconds_t;



typedef int __kernel_daddr_t;



typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;
# 72 "/usr/include/asm-generic/posix_types.h" 3 4
typedef __kernel_ulong_t __kernel_size_t;
typedef __kernel_long_t __kernel_ssize_t;
typedef __kernel_long_t __kernel_ptrdiff_t;




typedef struct {
 int val[2];
} __kernel_fsid_t;





typedef __kernel_long_t __kernel_off_t;
typedef long long __kernel_loff_t;
typedef __kernel_long_t __kernel_old_time_t;
typedef __kernel_long_t __kernel_time_t;
typedef long long __kernel_time64_t;
typedef __kernel_long_t __kernel_clock_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
# 19 "/usr/include/asm/posix_types_64.h" 2 3 4
# 8 "/usr/include/asm/posix_types.h" 2 3 4
# 37 "/usr/include/linux/posix_types.h" 2 3 4
# 10 "/usr/include/linux/types.h" 2 3 4


typedef __signed__ __int128 __s128 __attribute__((aligned(16)));
typedef unsigned __int128 __u128 __attribute__((aligned(16)));
# 31 "/usr/include/linux/types.h" 3 4
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef __u16 __sum16;
typedef __u32 __wsum;
# 55 "/usr/include/linux/types.h" 3 4
typedef unsigned __poll_t;
# 6 "/usr/include/linux/stat.h" 2 3 4
# 56 "/usr/include/linux/stat.h" 3 4
struct statx_timestamp {
 __s64 tv_sec;
 __u32 tv_nsec;
 __s32 __reserved;
};
# 99 "/usr/include/linux/stat.h" 3 4
struct statx {


 __u32 stx_mask;


 __u32 stx_blksize;


 __u64 stx_attributes;



 __u32 stx_nlink;


 __u32 stx_uid;


 __u32 stx_gid;


 __u16 stx_mode;
 __u16 __spare0[1];



 __u64 stx_ino;


 __u64 stx_size;


 __u64 stx_blocks;


 __u64 stx_attributes_mask;



 struct statx_timestamp stx_atime;


 struct statx_timestamp stx_btime;


 struct statx_timestamp stx_ctime;


 struct statx_timestamp stx_mtime;



 __u32 stx_rdev_major;
 __u32 stx_rdev_minor;


 __u32 stx_dev_major;
 __u32 stx_dev_minor;


 __u64 stx_mnt_id;


 __u32 stx_dio_mem_align;


 __u32 stx_dio_offset_align;



 __u64 stx_subvol;


 __u32 stx_atomic_write_unit_min;


 __u32 stx_atomic_write_unit_max;



 __u32 stx_atomic_write_segments_max;


 __u32 stx_dio_read_offset_align;


 __u32 stx_atomic_write_unit_max_opt;
 __u32 __spare2[1];


 __u64 __spare3[8];


};
# 32 "/usr/include/bits/statx.h" 2 3 4







# 1 "/usr/include/bits/statx-generic.h" 1 3 4
# 25 "/usr/include/bits/statx-generic.h" 3 4
# 1 "/usr/include/bits/types/struct_statx_timestamp.h" 1 3 4
# 26 "/usr/include/bits/statx-generic.h" 2 3 4
# 1 "/usr/include/bits/types/struct_statx.h" 1 3 4
# 27 "/usr/include/bits/statx-generic.h" 2 3 4
# 62 "/usr/include/bits/statx-generic.h" 3 4



int statx (int __dirfd, const char *__restrict __path, int __flags,
           unsigned int __mask, struct statx *__restrict __buf)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (5)));


# 40 "/usr/include/bits/statx.h" 2 3 4
# 466 "/usr/include/sys/stat.h" 2 3 4



# 17 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/fcntl.h" 1 3 4
# 28 "/usr/include/fcntl.h" 3 4







# 1 "/usr/include/bits/fcntl.h" 1 3 4
# 35 "/usr/include/bits/fcntl.h" 3 4
struct flock
  {
    short int l_type;
    short int l_whence;




    __off64_t l_start;
    __off64_t l_len;

    __pid_t l_pid;
  };


struct flock64
  {
    short int l_type;
    short int l_whence;
    __off64_t l_start;
    __off64_t l_len;
    __pid_t l_pid;
  };



# 1 "/usr/include/bits/fcntl-linux.h" 1 3 4
# 38 "/usr/include/bits/fcntl-linux.h" 3 4
# 1 "/usr/include/bits/types/struct_iovec.h" 1 3 4
# 23 "/usr/include/bits/types/struct_iovec.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 24 "/usr/include/bits/types/struct_iovec.h" 2 3 4


struct iovec
  {
    void *iov_base;
    size_t iov_len;
  };
# 39 "/usr/include/bits/fcntl-linux.h" 2 3 4
# 267 "/usr/include/bits/fcntl-linux.h" 3 4
enum __pid_type
  {
    F_OWNER_TID = 0,
    F_OWNER_PID,
    F_OWNER_PGRP,
    F_OWNER_GID = F_OWNER_PGRP
  };


struct f_owner_ex
  {
    enum __pid_type type;
    __pid_t pid;
  };
# 357 "/usr/include/bits/fcntl-linux.h" 3 4
# 1 "/usr/include/linux/falloc.h" 1 3 4
# 358 "/usr/include/bits/fcntl-linux.h" 2 3 4



struct file_handle
{
  unsigned int handle_bytes;
  int handle_type;

  unsigned char f_handle[0];
};
# 386 "/usr/include/bits/fcntl-linux.h" 3 4





extern __ssize_t readahead (int __fd, __off64_t __offset, size_t __count)
    __attribute__ ((__nothrow__ , __leaf__));






extern int sync_file_range (int __fd, __off64_t __offset, __off64_t __count,
       unsigned int __flags);






extern __ssize_t vmsplice (int __fdout, const struct iovec *__iov,
      size_t __count, unsigned int __flags);





extern __ssize_t splice (int __fdin, __off64_t *__offin, int __fdout,
    __off64_t *__offout, size_t __len,
    unsigned int __flags);





extern __ssize_t tee (int __fdin, int __fdout, size_t __len,
        unsigned int __flags);
# 433 "/usr/include/bits/fcntl-linux.h" 3 4
extern int fallocate (int __fd, int __mode, __off64_t __offset, __off64_t __len) __asm__ ("" "fallocate64")

                     ;





extern int fallocate64 (int __fd, int __mode, __off64_t __offset,
   __off64_t __len);




extern int name_to_handle_at (int __dfd, const char *__name,
         struct file_handle *__handle, int *__mnt_id,
         int __flags) __attribute__ ((__nothrow__ , __leaf__));





extern int open_by_handle_at (int __mountdirfd, struct file_handle *__handle,
         int __flags);




# 62 "/usr/include/bits/fcntl.h" 2 3 4
# 36 "/usr/include/fcntl.h" 2 3 4
# 78 "/usr/include/fcntl.h" 3 4
# 1 "/usr/include/bits/stat.h" 1 3 4
# 79 "/usr/include/fcntl.h" 2 3 4
# 180 "/usr/include/fcntl.h" 3 4
extern int fcntl (int __fd, int __cmd, ...) __asm__ ("" "fcntl64");





extern int fcntl64 (int __fd, int __cmd, ...);
# 212 "/usr/include/fcntl.h" 3 4
extern int open (const char *__file, int __oflag, ...) __asm__ ("" "open64")
     __attribute__ ((__nonnull__ (1)));





extern int open64 (const char *__file, int __oflag, ...) __attribute__ ((__nonnull__ (1)));
# 237 "/usr/include/fcntl.h" 3 4
extern int openat (int __fd, const char *__file, int __oflag, ...) __asm__ ("" "openat64")
                    __attribute__ ((__nonnull__ (2)));





extern int openat64 (int __fd, const char *__file, int __oflag, ...)
     __attribute__ ((__nonnull__ (2)));
# 258 "/usr/include/fcntl.h" 3 4
extern int creat (const char *__file, mode_t __mode) __asm__ ("" "creat64")
                  __attribute__ ((__nonnull__ (1)));





extern int creat64 (const char *__file, mode_t __mode) __attribute__ ((__nonnull__ (1)));
# 287 "/usr/include/fcntl.h" 3 4
extern int lockf (int __fd, int __cmd, __off64_t __len) __asm__ ("" "lockf64")
                                     ;





extern int lockf64 (int __fd, int __cmd, off64_t __len) ;
# 306 "/usr/include/fcntl.h" 3 4
extern int posix_fadvise (int __fd, __off64_t __offset, __off64_t __len, int __advise) __asm__ ("" "posix_fadvise64") __attribute__ ((__nothrow__ , __leaf__))

                      ;





extern int posix_fadvise64 (int __fd, off64_t __offset, off64_t __len,
       int __advise) __attribute__ ((__nothrow__ , __leaf__));
# 327 "/usr/include/fcntl.h" 3 4
extern int posix_fallocate (int __fd, __off64_t __offset, __off64_t __len) __asm__ ("" "posix_fallocate64")

                           ;





extern int posix_fallocate64 (int __fd, off64_t __offset, off64_t __len);
# 345 "/usr/include/fcntl.h" 3 4

# 18 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/dirent.h" 1 3 4
# 27 "/usr/include/dirent.h" 3 4

# 61 "/usr/include/dirent.h" 3 4
# 1 "/usr/include/bits/dirent.h" 1 3 4
# 22 "/usr/include/bits/dirent.h" 3 4
struct dirent
  {




    __ino64_t d_ino;
    __off64_t d_off;

    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
  };


struct dirent64
  {
    __ino64_t d_ino;
    __off64_t d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
  };
# 62 "/usr/include/dirent.h" 2 3 4
# 97 "/usr/include/dirent.h" 3 4
enum
  {
    DT_UNKNOWN = 0,

    DT_FIFO = 1,

    DT_CHR = 2,

    DT_DIR = 4,

    DT_BLK = 6,

    DT_REG = 8,

    DT_LNK = 10,

    DT_SOCK = 12,

    DT_WHT = 14

  };
# 127 "/usr/include/dirent.h" 3 4
typedef struct __dirstream DIR;






extern int closedir (DIR *__dirp) __attribute__ ((__nonnull__ (1)));






extern DIR *opendir (const char *__name) __attribute__ ((__nonnull__ (1)))
 __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (closedir, 1)));






extern DIR *fdopendir (int __fd)
 __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (closedir, 1)));
# 167 "/usr/include/dirent.h" 3 4
extern struct dirent *readdir (DIR *__dirp) __asm__ ("" "readdir64")
     __attribute__ ((__nonnull__ (1)));






extern struct dirent64 *readdir64 (DIR *__dirp) __attribute__ ((__nonnull__ (1)));
# 191 "/usr/include/dirent.h" 3 4
extern int readdir_r (DIR *__restrict __dirp, struct dirent *__restrict __entry, struct dirent **__restrict __result) __asm__ ("" "readdir64_r")




  __attribute__ ((__nonnull__ (1, 2, 3))) __attribute__ ((__deprecated__));






extern int readdir64_r (DIR *__restrict __dirp,
   struct dirent64 *__restrict __entry,
   struct dirent64 **__restrict __result)
  __attribute__ ((__nonnull__ (1, 2, 3))) __attribute__ ((__deprecated__));




extern void rewinddir (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern void seekdir (DIR *__dirp, long int __pos) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int telldir (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int dirfd (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 235 "/usr/include/dirent.h" 3 4
# 1 "/usr/include/bits/posix1_lim.h" 1 3 4
# 27 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 28 "/usr/include/bits/posix1_lim.h" 2 3 4
# 161 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/local_lim.h" 1 3 4
# 38 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/linux/limits.h" 1 3 4
# 39 "/usr/include/bits/local_lim.h" 2 3 4
# 81 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/bits/pthread_stack_min-dynamic.h" 1 3 4
# 23 "/usr/include/bits/pthread_stack_min-dynamic.h" 3 4

extern long int __sysconf (int __name) __attribute__ ((__nothrow__ , __leaf__));

# 82 "/usr/include/bits/local_lim.h" 2 3 4
# 162 "/usr/include/bits/posix1_lim.h" 2 3 4
# 236 "/usr/include/dirent.h" 2 3 4
# 247 "/usr/include/dirent.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 248 "/usr/include/dirent.h" 2 3 4
# 265 "/usr/include/dirent.h" 3 4
extern int scandir (const char *__restrict __dir, struct dirent ***__restrict __namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **)) __asm__ ("" "scandir64")





                    __attribute__ ((__nonnull__ (1, 2)));
# 280 "/usr/include/dirent.h" 3 4
extern int scandir64 (const char *__restrict __dir,
        struct dirent64 ***__restrict __namelist,
        int (*__selector) (const struct dirent64 *),
        int (*__cmp) (const struct dirent64 **,
        const struct dirent64 **))
     __attribute__ ((__nonnull__ (1, 2)));
# 303 "/usr/include/dirent.h" 3 4
extern int scandirat (int __dfd, const char *__restrict __dir, struct dirent ***__restrict __namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **)) __asm__ ("" "scandirat64")





                      __attribute__ ((__nonnull__ (2, 3)));







extern int scandirat64 (int __dfd, const char *__restrict __dir,
   struct dirent64 ***__restrict __namelist,
   int (*__selector) (const struct dirent64 *),
   int (*__cmp) (const struct dirent64 **,
          const struct dirent64 **))
     __attribute__ ((__nonnull__ (2, 3)));
# 332 "/usr/include/dirent.h" 3 4
extern int alphasort (const struct dirent **__e1, const struct dirent **__e2) __asm__ ("" "alphasort64") __attribute__ ((__nothrow__ , __leaf__))


                   __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int alphasort64 (const struct dirent64 **__e1,
   const struct dirent64 **__e2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 361 "/usr/include/dirent.h" 3 4
extern __ssize_t getdirentries (int __fd, char *__restrict __buf, size_t __nbytes, __off64_t *__restrict __basep) __asm__ ("" "getdirentries64") __attribute__ ((__nothrow__ , __leaf__))



                      __attribute__ ((__nonnull__ (2, 4)));






extern __ssize_t getdirentries64 (int __fd, char *__restrict __buf,
      size_t __nbytes,
      __off64_t *__restrict __basep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));
# 387 "/usr/include/dirent.h" 3 4
extern int versionsort (const struct dirent **__e1, const struct dirent **__e2) __asm__ ("" "versionsort64") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int versionsort64 (const struct dirent64 **__e1,
     const struct dirent64 **__e2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));





# 1 "/usr/include/bits/dirent_ext.h" 1 3 4
# 23 "/usr/include/bits/dirent_ext.h" 3 4






extern __ssize_t getdents64 (int __fd, void *__buffer, size_t __length)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



# 407 "/usr/include/dirent.h" 2 3 4
# 19 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/unistd.h" 1 3 4
# 27 "/usr/include/unistd.h" 3 4

# 202 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/posix_opt.h" 1 3 4
# 203 "/usr/include/unistd.h" 2 3 4



# 1 "/usr/include/bits/environments.h" 1 3 4
# 22 "/usr/include/bits/environments.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 23 "/usr/include/bits/environments.h" 2 3 4
# 207 "/usr/include/unistd.h" 2 3 4
# 226 "/usr/include/unistd.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 227 "/usr/include/unistd.h" 2 3 4
# 267 "/usr/include/unistd.h" 3 4
typedef __intptr_t intptr_t;






typedef __socklen_t socklen_t;
# 287 "/usr/include/unistd.h" 3 4
extern int access (const char *__name, int __type) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int euidaccess (const char *__name, int __type)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int eaccess (const char *__name, int __type)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int execveat (int __fd, const char *__path, char *const __argv[],
                     char *const __envp[], int __flags)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));






extern int faccessat (int __fd, const char *__file, int __type, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;
# 342 "/usr/include/unistd.h" 3 4
extern __off64_t lseek (int __fd, __off64_t __offset, int __whence) __asm__ ("" "lseek64") __attribute__ ((__nothrow__ , __leaf__))

             ;





extern __off64_t lseek64 (int __fd, __off64_t __offset, int __whence)
     __attribute__ ((__nothrow__ , __leaf__));






extern int close (int __fd);




extern void closefrom (int __lowfd) __attribute__ ((__nothrow__ , __leaf__));







extern ssize_t read (int __fd, void *__buf, size_t __nbytes)
    __attribute__ ((__access__ (__write_only__, 2, 3)));





extern ssize_t write (int __fd, const void *__buf, size_t __n)
    __attribute__ ((__access__ (__read_only__, 2, 3)));
# 404 "/usr/include/unistd.h" 3 4
extern ssize_t pread (int __fd, void *__buf, size_t __nbytes, __off64_t __offset) __asm__ ("" "pread64")


    __attribute__ ((__access__ (__write_only__, 2, 3)));
extern ssize_t pwrite (int __fd, const void *__buf, size_t __nbytes, __off64_t __offset) __asm__ ("" "pwrite64")


    __attribute__ ((__access__ (__read_only__, 2, 3)));
# 422 "/usr/include/unistd.h" 3 4
extern ssize_t pread64 (int __fd, void *__buf, size_t __nbytes,
   __off64_t __offset)
    __attribute__ ((__access__ (__write_only__, 2, 3)));


extern ssize_t pwrite64 (int __fd, const void *__buf, size_t __n,
    __off64_t __offset)
    __attribute__ ((__access__ (__read_only__, 2, 3)));







extern int pipe (int __pipedes[2]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int pipe2 (int __pipedes[2], int __flags) __attribute__ ((__nothrow__ , __leaf__)) ;
# 452 "/usr/include/unistd.h" 3 4
extern unsigned int alarm (unsigned int __seconds) __attribute__ ((__nothrow__ , __leaf__));
# 464 "/usr/include/unistd.h" 3 4
extern unsigned int sleep (unsigned int __seconds);







extern __useconds_t ualarm (__useconds_t __value, __useconds_t __interval)
     __attribute__ ((__nothrow__ , __leaf__));






extern int usleep (__useconds_t __useconds);
# 489 "/usr/include/unistd.h" 3 4
extern int pause (void);



extern int chown (const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchown (int __fd, __uid_t __owner, __gid_t __group) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int lchown (const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;






extern int fchownat (int __fd, const char *__file, __uid_t __owner,
       __gid_t __group, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;



extern int chdir (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchdir (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;
# 531 "/usr/include/unistd.h" 3 4
extern char *getcwd (char *__buf, size_t __size) __attribute__ ((__nothrow__ , __leaf__)) ;





extern char *get_current_dir_name (void) __attribute__ ((__nothrow__ , __leaf__));







extern char *getwd (char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__deprecated__))
    __attribute__ ((__access__ (__write_only__, 1)));




extern int dup (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;


extern int dup2 (int __fd, int __fd2) __attribute__ ((__nothrow__ , __leaf__));




extern int dup3 (int __fd, int __fd2, int __flags) __attribute__ ((__nothrow__ , __leaf__));



extern char **__environ;

extern char **environ;





extern int execve (const char *__path, char *const __argv[],
     char *const __envp[]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int fexecve (int __fd, char *const __argv[], char *const __envp[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));




extern int execv (const char *__path, char *const __argv[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execle (const char *__path, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execl (const char *__path, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execvp (const char *__file, char *const __argv[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int execlp (const char *__file, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int execvpe (const char *__file, char *const __argv[],
      char *const __envp[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern int nice (int __inc) __attribute__ ((__nothrow__ , __leaf__)) ;




extern void _exit (int __status) __attribute__ ((__noreturn__));





# 1 "/usr/include/bits/confname.h" 1 3 4
# 24 "/usr/include/bits/confname.h" 3 4
enum
  {
    _PC_LINK_MAX,

    _PC_MAX_CANON,

    _PC_MAX_INPUT,

    _PC_NAME_MAX,

    _PC_PATH_MAX,

    _PC_PIPE_BUF,

    _PC_CHOWN_RESTRICTED,

    _PC_NO_TRUNC,

    _PC_VDISABLE,

    _PC_SYNC_IO,

    _PC_ASYNC_IO,

    _PC_PRIO_IO,

    _PC_SOCK_MAXBUF,

    _PC_FILESIZEBITS,

    _PC_REC_INCR_XFER_SIZE,

    _PC_REC_MAX_XFER_SIZE,

    _PC_REC_MIN_XFER_SIZE,

    _PC_REC_XFER_ALIGN,

    _PC_ALLOC_SIZE_MIN,

    _PC_SYMLINK_MAX,

    _PC_2_SYMLINKS

  };


enum
  {
    _SC_ARG_MAX,

    _SC_CHILD_MAX,

    _SC_CLK_TCK,

    _SC_NGROUPS_MAX,

    _SC_OPEN_MAX,

    _SC_STREAM_MAX,

    _SC_TZNAME_MAX,

    _SC_JOB_CONTROL,

    _SC_SAVED_IDS,

    _SC_REALTIME_SIGNALS,

    _SC_PRIORITY_SCHEDULING,

    _SC_TIMERS,

    _SC_ASYNCHRONOUS_IO,

    _SC_PRIORITIZED_IO,

    _SC_SYNCHRONIZED_IO,

    _SC_FSYNC,

    _SC_MAPPED_FILES,

    _SC_MEMLOCK,

    _SC_MEMLOCK_RANGE,

    _SC_MEMORY_PROTECTION,

    _SC_MESSAGE_PASSING,

    _SC_SEMAPHORES,

    _SC_SHARED_MEMORY_OBJECTS,

    _SC_AIO_LISTIO_MAX,

    _SC_AIO_MAX,

    _SC_AIO_PRIO_DELTA_MAX,

    _SC_DELAYTIMER_MAX,

    _SC_MQ_OPEN_MAX,

    _SC_MQ_PRIO_MAX,

    _SC_VERSION,

    _SC_PAGESIZE,


    _SC_RTSIG_MAX,

    _SC_SEM_NSEMS_MAX,

    _SC_SEM_VALUE_MAX,

    _SC_SIGQUEUE_MAX,

    _SC_TIMER_MAX,




    _SC_BC_BASE_MAX,

    _SC_BC_DIM_MAX,

    _SC_BC_SCALE_MAX,

    _SC_BC_STRING_MAX,

    _SC_COLL_WEIGHTS_MAX,

    _SC_EQUIV_CLASS_MAX,

    _SC_EXPR_NEST_MAX,

    _SC_LINE_MAX,

    _SC_RE_DUP_MAX,

    _SC_CHARCLASS_NAME_MAX,


    _SC_2_VERSION,

    _SC_2_C_BIND,

    _SC_2_C_DEV,

    _SC_2_FORT_DEV,

    _SC_2_FORT_RUN,

    _SC_2_SW_DEV,

    _SC_2_LOCALEDEF,


    _SC_PII,

    _SC_PII_XTI,

    _SC_PII_SOCKET,

    _SC_PII_INTERNET,

    _SC_PII_OSI,

    _SC_POLL,

    _SC_SELECT,

    _SC_UIO_MAXIOV,

    _SC_IOV_MAX = _SC_UIO_MAXIOV,

    _SC_PII_INTERNET_STREAM,

    _SC_PII_INTERNET_DGRAM,

    _SC_PII_OSI_COTS,

    _SC_PII_OSI_CLTS,

    _SC_PII_OSI_M,

    _SC_T_IOV_MAX,



    _SC_THREADS,

    _SC_THREAD_SAFE_FUNCTIONS,

    _SC_GETGR_R_SIZE_MAX,

    _SC_GETPW_R_SIZE_MAX,

    _SC_LOGIN_NAME_MAX,

    _SC_TTY_NAME_MAX,

    _SC_THREAD_DESTRUCTOR_ITERATIONS,

    _SC_THREAD_KEYS_MAX,

    _SC_THREAD_STACK_MIN,

    _SC_THREAD_THREADS_MAX,

    _SC_THREAD_ATTR_STACKADDR,

    _SC_THREAD_ATTR_STACKSIZE,

    _SC_THREAD_PRIORITY_SCHEDULING,

    _SC_THREAD_PRIO_INHERIT,

    _SC_THREAD_PRIO_PROTECT,

    _SC_THREAD_PROCESS_SHARED,


    _SC_NPROCESSORS_CONF,

    _SC_NPROCESSORS_ONLN,

    _SC_PHYS_PAGES,

    _SC_AVPHYS_PAGES,

    _SC_ATEXIT_MAX,

    _SC_PASS_MAX,


    _SC_XOPEN_VERSION,

    _SC_XOPEN_XCU_VERSION,

    _SC_XOPEN_UNIX,

    _SC_XOPEN_CRYPT,

    _SC_XOPEN_ENH_I18N,

    _SC_XOPEN_SHM,


    _SC_2_CHAR_TERM,

    _SC_2_C_VERSION,

    _SC_2_UPE,


    _SC_XOPEN_XPG2,

    _SC_XOPEN_XPG3,

    _SC_XOPEN_XPG4,


    _SC_CHAR_BIT,

    _SC_CHAR_MAX,

    _SC_CHAR_MIN,

    _SC_INT_MAX,

    _SC_INT_MIN,

    _SC_LONG_BIT,

    _SC_WORD_BIT,

    _SC_MB_LEN_MAX,

    _SC_NZERO,

    _SC_SSIZE_MAX,

    _SC_SCHAR_MAX,

    _SC_SCHAR_MIN,

    _SC_SHRT_MAX,

    _SC_SHRT_MIN,

    _SC_UCHAR_MAX,

    _SC_UINT_MAX,

    _SC_ULONG_MAX,

    _SC_USHRT_MAX,


    _SC_NL_ARGMAX,

    _SC_NL_LANGMAX,

    _SC_NL_MSGMAX,

    _SC_NL_NMAX,

    _SC_NL_SETMAX,

    _SC_NL_TEXTMAX,


    _SC_XBS5_ILP32_OFF32,

    _SC_XBS5_ILP32_OFFBIG,

    _SC_XBS5_LP64_OFF64,

    _SC_XBS5_LPBIG_OFFBIG,


    _SC_XOPEN_LEGACY,

    _SC_XOPEN_REALTIME,

    _SC_XOPEN_REALTIME_THREADS,


    _SC_ADVISORY_INFO,

    _SC_BARRIERS,

    _SC_BASE,

    _SC_C_LANG_SUPPORT,

    _SC_C_LANG_SUPPORT_R,

    _SC_CLOCK_SELECTION,

    _SC_CPUTIME,

    _SC_THREAD_CPUTIME,

    _SC_DEVICE_IO,

    _SC_DEVICE_SPECIFIC,

    _SC_DEVICE_SPECIFIC_R,

    _SC_FD_MGMT,

    _SC_FIFO,

    _SC_PIPE,

    _SC_FILE_ATTRIBUTES,

    _SC_FILE_LOCKING,

    _SC_FILE_SYSTEM,

    _SC_MONOTONIC_CLOCK,

    _SC_MULTI_PROCESS,

    _SC_SINGLE_PROCESS,

    _SC_NETWORKING,

    _SC_READER_WRITER_LOCKS,

    _SC_SPIN_LOCKS,

    _SC_REGEXP,

    _SC_REGEX_VERSION,

    _SC_SHELL,

    _SC_SIGNALS,

    _SC_SPAWN,

    _SC_SPORADIC_SERVER,

    _SC_THREAD_SPORADIC_SERVER,

    _SC_SYSTEM_DATABASE,

    _SC_SYSTEM_DATABASE_R,

    _SC_TIMEOUTS,

    _SC_TYPED_MEMORY_OBJECTS,

    _SC_USER_GROUPS,

    _SC_USER_GROUPS_R,

    _SC_2_PBS,

    _SC_2_PBS_ACCOUNTING,

    _SC_2_PBS_LOCATE,

    _SC_2_PBS_MESSAGE,

    _SC_2_PBS_TRACK,

    _SC_SYMLOOP_MAX,

    _SC_STREAMS,

    _SC_2_PBS_CHECKPOINT,


    _SC_V6_ILP32_OFF32,

    _SC_V6_ILP32_OFFBIG,

    _SC_V6_LP64_OFF64,

    _SC_V6_LPBIG_OFFBIG,


    _SC_HOST_NAME_MAX,

    _SC_TRACE,

    _SC_TRACE_EVENT_FILTER,

    _SC_TRACE_INHERIT,

    _SC_TRACE_LOG,


    _SC_LEVEL1_ICACHE_SIZE,

    _SC_LEVEL1_ICACHE_ASSOC,

    _SC_LEVEL1_ICACHE_LINESIZE,

    _SC_LEVEL1_DCACHE_SIZE,

    _SC_LEVEL1_DCACHE_ASSOC,

    _SC_LEVEL1_DCACHE_LINESIZE,

    _SC_LEVEL2_CACHE_SIZE,

    _SC_LEVEL2_CACHE_ASSOC,

    _SC_LEVEL2_CACHE_LINESIZE,

    _SC_LEVEL3_CACHE_SIZE,

    _SC_LEVEL3_CACHE_ASSOC,

    _SC_LEVEL3_CACHE_LINESIZE,

    _SC_LEVEL4_CACHE_SIZE,

    _SC_LEVEL4_CACHE_ASSOC,

    _SC_LEVEL4_CACHE_LINESIZE,



    _SC_IPV6 = _SC_LEVEL1_ICACHE_SIZE + 50,

    _SC_RAW_SOCKETS,


    _SC_V7_ILP32_OFF32,

    _SC_V7_ILP32_OFFBIG,

    _SC_V7_LP64_OFF64,

    _SC_V7_LPBIG_OFFBIG,


    _SC_SS_REPL_MAX,


    _SC_TRACE_EVENT_NAME_MAX,

    _SC_TRACE_NAME_MAX,

    _SC_TRACE_SYS_MAX,

    _SC_TRACE_USER_EVENT_MAX,


    _SC_XOPEN_STREAMS,


    _SC_THREAD_ROBUST_PRIO_INHERIT,

    _SC_THREAD_ROBUST_PRIO_PROTECT,


    _SC_MINSIGSTKSZ,


    _SC_SIGSTKSZ

  };


enum
  {
    _CS_PATH,


    _CS_V6_WIDTH_RESTRICTED_ENVS,



    _CS_GNU_LIBC_VERSION,

    _CS_GNU_LIBPTHREAD_VERSION,


    _CS_V5_WIDTH_RESTRICTED_ENVS,



    _CS_V7_WIDTH_RESTRICTED_ENVS,



    _CS_LFS_CFLAGS = 1000,

    _CS_LFS_LDFLAGS,

    _CS_LFS_LIBS,

    _CS_LFS_LINTFLAGS,

    _CS_LFS64_CFLAGS,

    _CS_LFS64_LDFLAGS,

    _CS_LFS64_LIBS,

    _CS_LFS64_LINTFLAGS,


    _CS_XBS5_ILP32_OFF32_CFLAGS = 1100,

    _CS_XBS5_ILP32_OFF32_LDFLAGS,

    _CS_XBS5_ILP32_OFF32_LIBS,

    _CS_XBS5_ILP32_OFF32_LINTFLAGS,

    _CS_XBS5_ILP32_OFFBIG_CFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LDFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LIBS,

    _CS_XBS5_ILP32_OFFBIG_LINTFLAGS,

    _CS_XBS5_LP64_OFF64_CFLAGS,

    _CS_XBS5_LP64_OFF64_LDFLAGS,

    _CS_XBS5_LP64_OFF64_LIBS,

    _CS_XBS5_LP64_OFF64_LINTFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_CFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LDFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LIBS,

    _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V6_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LIBS,

    _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V6_LP64_OFF64_CFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LIBS,

    _CS_POSIX_V6_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V7_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LIBS,

    _CS_POSIX_V7_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V7_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V7_LP64_OFF64_CFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LIBS,

    _CS_POSIX_V7_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LINTFLAGS,


    _CS_V6_ENV,

    _CS_V7_ENV

  };
# 631 "/usr/include/unistd.h" 2 3 4


extern long int pathconf (const char *__path, int __name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int fpathconf (int __fd, int __name) __attribute__ ((__nothrow__ , __leaf__));


extern long int sysconf (int __name) __attribute__ ((__nothrow__ , __leaf__));



extern size_t confstr (int __name, char *__buf, size_t __len) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__write_only__, 2, 3)));




extern __pid_t getpid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t getppid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t getpgrp (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t __getpgid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));

extern __pid_t getpgid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));






extern int setpgid (__pid_t __pid, __pid_t __pgid) __attribute__ ((__nothrow__ , __leaf__));
# 682 "/usr/include/unistd.h" 3 4
extern int setpgrp (void) __attribute__ ((__nothrow__ , __leaf__));






extern __pid_t setsid (void) __attribute__ ((__nothrow__ , __leaf__));



extern __pid_t getsid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));



extern __uid_t getuid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __uid_t geteuid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __gid_t getgid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __gid_t getegid (void) __attribute__ ((__nothrow__ , __leaf__));




extern int getgroups (int __size, __gid_t __list[]) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__write_only__, 2, 1)));


extern int group_member (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__));






extern int setuid (__uid_t __uid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setreuid (__uid_t __ruid, __uid_t __euid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int seteuid (__uid_t __uid) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int setgid (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setregid (__gid_t __rgid, __gid_t __egid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setegid (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__)) ;





extern int getresuid (__uid_t *__ruid, __uid_t *__euid, __uid_t *__suid)
     __attribute__ ((__nothrow__ , __leaf__));



extern int getresgid (__gid_t *__rgid, __gid_t *__egid, __gid_t *__sgid)
     __attribute__ ((__nothrow__ , __leaf__));



extern int setresuid (__uid_t __ruid, __uid_t __euid, __uid_t __suid)
     __attribute__ ((__nothrow__ , __leaf__)) ;



extern int setresgid (__gid_t __rgid, __gid_t __egid, __gid_t __sgid)
     __attribute__ ((__nothrow__ , __leaf__)) ;






extern __pid_t fork (void) __attribute__ ((__nothrow__));







extern __pid_t vfork (void) __attribute__ ((__nothrow__ , __leaf__));






extern __pid_t _Fork (void) __attribute__ ((__nothrow__ , __leaf__));





extern char *ttyname (int __fd) __attribute__ ((__nothrow__ , __leaf__));



extern int ttyname_r (int __fd, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)))
     __attribute__ ((__access__ (__write_only__, 2, 3)));



extern int isatty (int __fd) __attribute__ ((__nothrow__ , __leaf__));




extern int ttyslot (void) __attribute__ ((__nothrow__ , __leaf__));




extern int link (const char *__from, const char *__to)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern int linkat (int __fromfd, const char *__from, int __tofd,
     const char *__to, int __flags)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4))) ;




extern int symlink (const char *__from, const char *__to)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern ssize_t readlink (const char *__restrict __path,
    char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)))
     __attribute__ ((__access__ (__write_only__, 2, 3)));





extern int symlinkat (const char *__from, int __tofd,
        const char *__to) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3))) ;


extern ssize_t readlinkat (int __fd, const char *__restrict __path,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)))
     __attribute__ ((__access__ (__write_only__, 3, 4)));



extern int unlink (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int unlinkat (int __fd, const char *__name, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern int rmdir (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern __pid_t tcgetpgrp (int __fd) __attribute__ ((__nothrow__ , __leaf__));


extern int tcsetpgrp (int __fd, __pid_t __pgrp_id) __attribute__ ((__nothrow__ , __leaf__));






extern char *getlogin (void);







extern int getlogin_r (char *__name, size_t __name_len) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));




extern int setlogin (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







# 1 "/usr/include/bits/getopt_posix.h" 1 3 4
# 27 "/usr/include/bits/getopt_posix.h" 3 4
# 1 "/usr/include/bits/getopt_core.h" 1 3 4
# 28 "/usr/include/bits/getopt_core.h" 3 4








extern char *optarg;
# 50 "/usr/include/bits/getopt_core.h" 3 4
extern int optind;




extern int opterr;



extern int optopt;
# 91 "/usr/include/bits/getopt_core.h" 3 4
extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));


# 28 "/usr/include/bits/getopt_posix.h" 2 3 4


# 49 "/usr/include/bits/getopt_posix.h" 3 4

# 904 "/usr/include/unistd.h" 2 3 4







extern int gethostname (char *__name, size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));






extern int sethostname (const char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__read_only__, 1, 2)));



extern int sethostid (long int __id) __attribute__ ((__nothrow__ , __leaf__)) ;





extern int getdomainname (char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
     __attribute__ ((__access__ (__write_only__, 1, 2)));
extern int setdomainname (const char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__read_only__, 1, 2)));




extern int vhangup (void) __attribute__ ((__nothrow__ , __leaf__));


extern int revoke (const char *__file) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;







extern int profil (unsigned short int *__sample_buffer, size_t __size,
     size_t __offset, unsigned int __scale)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int acct (const char *__name) __attribute__ ((__nothrow__ , __leaf__));



extern char *getusershell (void) __attribute__ ((__nothrow__ , __leaf__));
extern void endusershell (void) __attribute__ ((__nothrow__ , __leaf__));
extern void setusershell (void) __attribute__ ((__nothrow__ , __leaf__));





extern int daemon (int __nochdir, int __noclose) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int chroot (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern char *getpass (const char *__prompt) __attribute__ ((__nonnull__ (1)));







extern int fsync (int __fd);





extern int syncfs (int __fd) __attribute__ ((__nothrow__ , __leaf__));






extern long int gethostid (void);


extern void sync (void) __attribute__ ((__nothrow__ , __leaf__));





extern int getpagesize (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern int getdtablesize (void) __attribute__ ((__nothrow__ , __leaf__));
# 1030 "/usr/include/unistd.h" 3 4
extern int truncate (const char *__file, __off64_t __length) __asm__ ("" "truncate64") __attribute__ ((__nothrow__ , __leaf__))

                  __attribute__ ((__nonnull__ (1))) ;





extern int truncate64 (const char *__file, __off64_t __length)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 1052 "/usr/include/unistd.h" 3 4
extern int ftruncate (int __fd, __off64_t __length) __asm__ ("" "ftruncate64") __attribute__ ((__nothrow__ , __leaf__))
                        ;





extern int ftruncate64 (int __fd, __off64_t __length) __attribute__ ((__nothrow__ , __leaf__)) ;
# 1070 "/usr/include/unistd.h" 3 4
extern int brk (void *__addr) __attribute__ ((__nothrow__ , __leaf__)) ;





extern void *sbrk (intptr_t __delta) __attribute__ ((__nothrow__ , __leaf__));
# 1091 "/usr/include/unistd.h" 3 4
extern long int syscall (long int __sysno, ...) __attribute__ ((__nothrow__ , __leaf__));
# 1142 "/usr/include/unistd.h" 3 4
ssize_t copy_file_range (int __infd, __off64_t *__pinoff,
    int __outfd, __off64_t *__poutoff,
    size_t __length, unsigned int __flags);





extern int fdatasync (int __fildes);
# 1162 "/usr/include/unistd.h" 3 4
extern char *crypt (const char *__key, const char *__salt)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));







extern void swab (const void *__restrict __from, void *__restrict __to,
    ssize_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)))
    __attribute__ ((__access__ (__read_only__, 1, 3)))
    __attribute__ ((__access__ (__write_only__, 2, 3)));
# 1201 "/usr/include/unistd.h" 3 4
int getentropy (void *__buffer, size_t __length)
    __attribute__ ((__access__ (__write_only__, 1, 2)));
# 1211 "/usr/include/unistd.h" 3 4
extern int close_range (unsigned int __fd, unsigned int __max_fd,
   int __flags) __attribute__ ((__nothrow__ , __leaf__));
# 1221 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/unistd_ext.h" 1 3 4
# 34 "/usr/include/bits/unistd_ext.h" 3 4
extern __pid_t gettid (void) __attribute__ ((__nothrow__ , __leaf__));



# 1 "/usr/include/linux/close_range.h" 1 3 4
# 39 "/usr/include/bits/unistd_ext.h" 2 3 4
# 1222 "/usr/include/unistd.h" 2 3 4


# 20 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/stdio.h" 1 3 4
# 28 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 29 "/usr/include/stdio.h" 2 3 4





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 35 "/usr/include/stdio.h" 2 3 4





# 1 "/usr/include/bits/types/__fpos_t.h" 1 3 4




# 1 "/usr/include/bits/types/__mbstate_t.h" 1 3 4
# 13 "/usr/include/bits/types/__mbstate_t.h" 3 4
typedef struct
{
  int __count;
  union
  {
    unsigned int __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 6 "/usr/include/bits/types/__fpos_t.h" 2 3 4




typedef struct _G_fpos_t
{
  __off_t __pos;
  __mbstate_t __state;
} __fpos_t;
# 41 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/__fpos64_t.h" 1 3 4
# 10 "/usr/include/bits/types/__fpos64_t.h" 3 4
typedef struct _G_fpos64_t
{
  __off64_t __pos;
  __mbstate_t __state;
} __fpos64_t;
# 42 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/__FILE.h" 1 3 4



struct _IO_FILE;
typedef struct _IO_FILE __FILE;
# 43 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/FILE.h" 1 3 4



struct _IO_FILE;


typedef struct _IO_FILE FILE;
# 44 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/struct_FILE.h" 1 3 4
# 35 "/usr/include/bits/types/struct_FILE.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 36 "/usr/include/bits/types/struct_FILE.h" 2 3 4

struct _IO_FILE;
struct _IO_marker;
struct _IO_codecvt;
struct _IO_wide_data;




typedef void _IO_lock_t;





struct _IO_FILE
{
  int _flags;


  char *_IO_read_ptr;
  char *_IO_read_end;
  char *_IO_read_base;
  char *_IO_write_base;
  char *_IO_write_ptr;
  char *_IO_write_end;
  char *_IO_buf_base;
  char *_IO_buf_end;


  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;
  int _flags2:24;

  char _short_backupbuf[1];
  __off_t _old_offset;


  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

  _IO_lock_t *_lock;







  __off64_t _offset;

  struct _IO_codecvt *_codecvt;
  struct _IO_wide_data *_wide_data;
  struct _IO_FILE *_freeres_list;
  void *_freeres_buf;
  struct _IO_FILE **_prevchain;
  int _mode;

  int _unused3;

  __uint64_t _total_written;




  char _unused2[12 * sizeof (int) - 5 * sizeof (void *)];
};
# 45 "/usr/include/stdio.h" 2 3 4


# 1 "/usr/include/bits/types/cookie_io_functions_t.h" 1 3 4
# 27 "/usr/include/bits/types/cookie_io_functions_t.h" 3 4
typedef __ssize_t cookie_read_function_t (void *__cookie, char *__buf,
                                          size_t __nbytes);







typedef __ssize_t cookie_write_function_t (void *__cookie, const char *__buf,
                                           size_t __nbytes);







typedef int cookie_seek_function_t (void *__cookie, __off64_t *__pos, int __w);


typedef int cookie_close_function_t (void *__cookie);






typedef struct _IO_cookie_io_functions_t
{
  cookie_read_function_t *read;
  cookie_write_function_t *write;
  cookie_seek_function_t *seek;
  cookie_close_function_t *close;
} cookie_io_functions_t;
# 48 "/usr/include/stdio.h" 2 3 4
# 87 "/usr/include/stdio.h" 3 4
typedef __fpos64_t fpos_t;


typedef __fpos64_t fpos64_t;
# 129 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/stdio_lim.h" 1 3 4
# 130 "/usr/include/stdio.h" 2 3 4
# 149 "/usr/include/stdio.h" 3 4
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;






extern int remove (const char *__filename) __attribute__ ((__nothrow__ , __leaf__));

extern int rename (const char *__old, const char *__new) __attribute__ ((__nothrow__ , __leaf__));



extern int renameat (int __oldfd, const char *__old, int __newfd,
       const char *__new) __attribute__ ((__nothrow__ , __leaf__));
# 179 "/usr/include/stdio.h" 3 4
extern int renameat2 (int __oldfd, const char *__old, int __newfd,
        const char *__new, unsigned int __flags) __attribute__ ((__nothrow__ , __leaf__));






extern int fclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 201 "/usr/include/stdio.h" 3 4
extern FILE *tmpfile (void) __asm__ ("" "tmpfile64")
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;






extern FILE *tmpfile64 (void)
   __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;



extern char *tmpnam (char[20]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern char *tmpnam_r (char __s[20]) __attribute__ ((__nothrow__ , __leaf__)) ;
# 231 "/usr/include/stdio.h" 3 4
extern char *tempnam (const char *__dir, const char *__pfx)
   __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (__builtin_free, 1)));






extern int fflush (FILE *__stream);
# 248 "/usr/include/stdio.h" 3 4
extern int fflush_unlocked (FILE *__stream);
# 258 "/usr/include/stdio.h" 3 4
extern int fcloseall (void);
# 279 "/usr/include/stdio.h" 3 4
extern FILE *fopen (const char *__restrict __filename, const char *__restrict __modes) __asm__ ("" "fopen64")

  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
extern FILE *freopen (const char *__restrict __filename, const char *__restrict __modes, FILE *__restrict __stream) __asm__ ("" "freopen64")


  __attribute__ ((__nonnull__ (3)));






extern FILE *fopen64 (const char *__restrict __filename,
        const char *__restrict __modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
extern FILE *freopen64 (const char *__restrict __filename,
   const char *__restrict __modes,
   FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));




extern FILE *fdopen (int __fd, const char *__modes) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;





extern FILE *fopencookie (void *__restrict __magic_cookie,
     const char *__restrict __modes,
     cookie_io_functions_t __io_funcs) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *fmemopen (void *__s, size_t __len, const char *__modes)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *open_memstream (char **__bufloc, size_t *__sizeloc) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
# 337 "/usr/include/stdio.h" 3 4
extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__nonnull__ (1)));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int fprintf (FILE *__restrict __stream,
      const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int printf (const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nonnull__ (1)));




extern int vprintf (const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));



extern int snprintf (char *__restrict __s, size_t __maxlen,
       const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));





extern int vasprintf (char **__restrict __ptr, const char *__restrict __f,
        __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0))) ;
extern int __asprintf (char **__restrict __ptr,
         const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;
extern int asprintf (char **__restrict __ptr,
       const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;




extern int vdprintf (int __fd, const char *__restrict __fmt,
       __gnuc_va_list __arg)
     __attribute__ ((__format__ (__printf__, 2, 0)));
extern int dprintf (int __fd, const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));







extern int fscanf (FILE *__restrict __stream,
     const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int scanf (const char *__restrict __format, ...) ;

extern int sscanf (const char *__restrict __s,
     const char *__restrict __format, ...) __attribute__ ((__nothrow__ , __leaf__));





# 1 "/usr/include/bits/floatn.h" 1 3 4
# 131 "/usr/include/bits/floatn.h" 3 4
# 1 "/usr/include/bits/floatn-common.h" 1 3 4
# 24 "/usr/include/bits/floatn-common.h" 3 4
# 1 "/usr/include/bits/long-double.h" 1 3 4
# 25 "/usr/include/bits/floatn-common.h" 2 3 4
# 132 "/usr/include/bits/floatn.h" 2 3 4
# 441 "/usr/include/stdio.h" 2 3 4




extern int fscanf (FILE *__restrict __stream, const char *__restrict __format, ...) __asm__ ("" "__isoc23_fscanf")

                                __attribute__ ((__nonnull__ (1)));
extern int scanf (const char *__restrict __format, ...) __asm__ ("" "__isoc23_scanf")
                              ;
extern int sscanf (const char *__restrict __s, const char *__restrict __format, ...) __asm__ ("" "__isoc23_sscanf") __attribute__ ((__nothrow__ , __leaf__))

                      ;
# 493 "/usr/include/stdio.h" 3 4
extern int vfscanf (FILE *__restrict __s, const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));





extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (const char *__restrict __s,
      const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format__ (__scanf__, 2, 0)));






extern int vfscanf (FILE *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vfscanf")



     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));
extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vscanf")

     __attribute__ ((__format__ (__scanf__, 1, 0))) ;
extern int vsscanf (const char *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vsscanf") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__format__ (__scanf__, 2, 0)));
# 578 "/usr/include/stdio.h" 3 4
extern int fgetc (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getc (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern int getchar (void);






extern int getc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getchar_unlocked (void);
# 603 "/usr/include/stdio.h" 3 4
extern int fgetc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));







extern int fputc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));





extern int putchar (int __c);
# 627 "/usr/include/stdio.h" 3 4
extern int fputc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern int putc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream) __attribute__ ((__nonnull__ (1)));


extern int putw (int __w, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 677 "/usr/include/stdio.h" 3 4
extern char *fgets_unlocked (char *__restrict __s, int __n,
        FILE *__restrict __stream)
    __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 689 "/usr/include/stdio.h" 3 4
extern __ssize_t __getdelim (char **__restrict __lineptr,
                             size_t *__restrict __n, int __delimiter,
                             FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));
extern __ssize_t getdelim (char **__restrict __lineptr,
                           size_t *__restrict __n, int __delimiter,
                           FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));


extern __ssize_t getline (char **__restrict __lineptr,
                          size_t *__restrict __n,
                          FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));







extern int fputs (const char *__restrict __s, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (2)));





extern int puts (const char *__s);






extern int ungetc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));




extern size_t fwrite (const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s) __attribute__ ((__nonnull__ (4)));
# 745 "/usr/include/stdio.h" 3 4
extern int fputs_unlocked (const char *__restrict __s,
      FILE *__restrict __stream) __attribute__ ((__nonnull__ (2)));
# 756 "/usr/include/stdio.h" 3 4
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));
extern size_t fwrite_unlocked (const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));







extern int fseek (FILE *__stream, long int __off, int __whence)
  __attribute__ ((__nonnull__ (1)));




extern long int ftell (FILE *__stream) __attribute__ ((__nonnull__ (1)));




extern void rewind (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 802 "/usr/include/stdio.h" 3 4
extern int fseeko (FILE *__stream, __off64_t __off, int __whence) __asm__ ("" "fseeko64")

                   __attribute__ ((__nonnull__ (1)));
extern __off64_t ftello (FILE *__stream) __asm__ ("" "ftello64")
  __attribute__ ((__nonnull__ (1)));
# 828 "/usr/include/stdio.h" 3 4
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos) __asm__ ("" "fgetpos64")

  __attribute__ ((__nonnull__ (1)));
extern int fsetpos (FILE *__stream, const fpos_t *__pos) __asm__ ("" "fsetpos64")

  __attribute__ ((__nonnull__ (1)));







extern int fseeko64 (FILE *__stream, __off64_t __off, int __whence)
  __attribute__ ((__nonnull__ (1)));
extern __off64_t ftello64 (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int fgetpos64 (FILE *__restrict __stream, fpos64_t *__restrict __pos)
  __attribute__ ((__nonnull__ (1)));
extern int fsetpos64 (FILE *__stream, const fpos64_t *__pos) __attribute__ ((__nonnull__ (1)));



extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern void perror (const char *__s) __attribute__ ((__cold__));




extern int fileno (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 887 "/usr/include/stdio.h" 3 4
extern int pclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern FILE *popen (const char *__command, const char *__modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (pclose, 1))) ;






extern char *ctermid (char *__s) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1)));





extern char *cuserid (char *__s)
  __attribute__ ((__access__ (__write_only__, 1)));




struct obstack;


extern int obstack_printf (struct obstack *__restrict __obstack,
      const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3)));
extern int obstack_vprintf (struct obstack *__restrict __obstack,
       const char *__restrict __format,
       __gnuc_va_list __args)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0)));







extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 949 "/usr/include/stdio.h" 3 4
extern int __uflow (FILE *);
extern int __overflow (FILE *, int);
# 973 "/usr/include/stdio.h" 3 4

# 21 "./include/holy/osdep/hostfile_unix.h" 2


# 22 "./include/holy/osdep/hostfile_unix.h"
typedef struct dirent *holy_util_fd_dirent_t;
typedef DIR *holy_util_fd_dir_t;

static inline holy_util_fd_dir_t
holy_util_fd_opendir (const char *name)
{
  return opendir (name);
}

static inline void
holy_util_fd_closedir (holy_util_fd_dir_t dirp)
{
  closedir (dirp);
}

static inline holy_util_fd_dirent_t
holy_util_fd_readdir (holy_util_fd_dir_t dirp)
{
  return readdir (dirp);
}

static inline int
holy_util_unlink (const char *pathname)
{
  return unlink (pathname);
}

static inline int
holy_util_rmdir (const char *pathname)
{
  return rmdir (pathname);
}

static inline int
holy_util_rename (const char *from, const char *to)
{
  return rename (from, to);
}
# 70 "./include/holy/osdep/hostfile_unix.h"
enum holy_util_fd_open_flags_t
  {
    holy_UTIL_FD_O_RDONLY = 
# 72 "./include/holy/osdep/hostfile_unix.h" 3 4
                           00
# 72 "./include/holy/osdep/hostfile_unix.h"
                                   ,
    holy_UTIL_FD_O_WRONLY = 
# 73 "./include/holy/osdep/hostfile_unix.h" 3 4
                           01
# 73 "./include/holy/osdep/hostfile_unix.h"
                                   ,
    holy_UTIL_FD_O_RDWR = 
# 74 "./include/holy/osdep/hostfile_unix.h" 3 4
                         02
# 74 "./include/holy/osdep/hostfile_unix.h"
                               ,
    holy_UTIL_FD_O_CREATTRUNC = 
# 75 "./include/holy/osdep/hostfile_unix.h" 3 4
                               0100 
# 75 "./include/holy/osdep/hostfile_unix.h"
                                       | 
# 75 "./include/holy/osdep/hostfile_unix.h" 3 4
                                         01000
# 75 "./include/holy/osdep/hostfile_unix.h"
                                                ,
    holy_UTIL_FD_O_SYNC = (0

      | 
# 78 "./include/holy/osdep/hostfile_unix.h" 3 4
       04010000


      
# 81 "./include/holy/osdep/hostfile_unix.h"
     | 
# 81 "./include/holy/osdep/hostfile_unix.h" 3 4
       04010000

      
# 83 "./include/holy/osdep/hostfile_unix.h"
     )
  };



typedef int holy_util_fd_t;
# 12 "./include/holy/osdep/hostfile.h" 2
# 13 "./include/holy/emu/hostfile.h" 2

int
holy_util_is_directory (const char *path);
int
holy_util_is_special_file (const char *path);
int
holy_util_is_regular (const char *path);

char *
holy_util_path_concat (size_t n, ...);
char *
holy_util_path_concat_ext (size_t n, ...);

int
holy_util_fd_seek (holy_util_fd_t fd, holy_uint64_t off);
ssize_t
holy_util_fd_read (holy_util_fd_t fd, char *buf, size_t len);
ssize_t
holy_util_fd_write (holy_util_fd_t fd, const char *buf, size_t len);

holy_util_fd_t
holy_util_fd_open (const char *os_dev, int flags);
const char *
holy_util_fd_strerror (void);
void
holy_util_fd_sync (holy_util_fd_t fd);
void
holy_util_disable_fd_syncs (void);
void
holy_util_fd_close (holy_util_fd_t fd);

holy_uint64_t
holy_util_get_fd_size (holy_util_fd_t fd, const char *name, unsigned *log_secsize);
char *
holy_util_make_temporary_file (void);
char *
holy_util_make_temporary_dir (void);
void
holy_util_unlink_recursive (const char *name);
holy_uint32_t
holy_util_get_mtime (const char *name);
# 12 "./include/holy/emu/config.h" 2


const char *
holy_util_get_config_filename (void);
const char *
holy_util_get_pkgdatadir (void);
const char *
holy_util_get_pkglibdir (void);
const char *
holy_util_get_localedir (void);

struct holy_util_config
{
  int is_cryptodisk_enabled;
  char *holy_distributor;
};

void
holy_util_load_config (struct holy_util_config *cfg);

void
holy_util_parse_config (FILE *f, struct holy_util_config *cfg, int simple);
# 10 "./include/holy/types.h" 2
# 63 "./include/holy/types.h"
typedef signed char holy_int8_t;
typedef short holy_int16_t;
typedef int holy_int32_t;

typedef long holy_int64_t;




typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned holy_uint32_t;



typedef unsigned long holy_uint64_t;
# 90 "./include/holy/types.h"
typedef holy_uint64_t holy_addr_t;
typedef holy_uint64_t holy_size_t;
typedef holy_int64_t holy_ssize_t;
# 138 "./include/holy/types.h"
typedef holy_uint64_t holy_properly_aligned_t;




typedef holy_uint64_t holy_off_t;


typedef holy_uint64_t holy_disk_addr_t;


static inline holy_uint16_t holy_swap_bytes16(holy_uint16_t _x)
{
   return (holy_uint16_t) ((_x << 8) | (_x >> 8));
}
# 170 "./include/holy/types.h"
static inline holy_uint32_t holy_swap_bytes32(holy_uint32_t x)
{
 return __builtin_bswap32(x);
}

static inline holy_uint64_t holy_swap_bytes64(holy_uint64_t x)
{
 return __builtin_bswap64(x);
}
# 244 "./include/holy/types.h"
struct holy_unaligned_uint16
{
  holy_uint16_t val;
} __attribute__ ((packed));
struct holy_unaligned_uint32
{
  holy_uint32_t val;
} __attribute__ ((packed));
struct holy_unaligned_uint64
{
  holy_uint64_t val;
} __attribute__ ((packed));

typedef struct holy_unaligned_uint16 holy_unaligned_uint16_t;
typedef struct holy_unaligned_uint32 holy_unaligned_uint32_t;
typedef struct holy_unaligned_uint64 holy_unaligned_uint64_t;

static inline holy_uint16_t holy_get_unaligned16 (const void *ptr)
{
  const struct holy_unaligned_uint16 *dd
    = (const struct holy_unaligned_uint16 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned16 (void *ptr, holy_uint16_t val)
{
  struct holy_unaligned_uint16 *dd = (struct holy_unaligned_uint16 *) ptr;
  dd->val = val;
}

static inline holy_uint32_t holy_get_unaligned32 (const void *ptr)
{
  const struct holy_unaligned_uint32 *dd
    = (const struct holy_unaligned_uint32 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned32 (void *ptr, holy_uint32_t val)
{
  struct holy_unaligned_uint32 *dd = (struct holy_unaligned_uint32 *) ptr;
  dd->val = val;
}

static inline holy_uint64_t holy_get_unaligned64 (const void *ptr)
{
  const struct holy_unaligned_uint64 *dd
    = (const struct holy_unaligned_uint64 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned64 (void *ptr, holy_uint64_t val)
{
  struct holy_unaligned_uint64_t
  {
    holy_uint64_t d;
  } __attribute__ ((packed));
  struct holy_unaligned_uint64_t *dd = (struct holy_unaligned_uint64_t *) ptr;
  dd->d = val;
}
# 12 "./include/holy/fs.h" 2
# 20 "./include/holy/fs.h"
struct holy_file;

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;
typedef holy_int64_t holy_ssize_t;



struct holy_dirhook_info
{
  unsigned dir:1;
  unsigned mtimeset:1;
  unsigned case_insensitive:1;
  unsigned inodeset:1;
  holy_int32_t mtime;
  holy_uint64_t inode;
};

typedef int (*holy_fs_dir_hook_t) (const char *filename,
       const struct holy_dirhook_info *info,
       void *data);


struct holy_fs
{

  struct holy_fs *next;
  struct holy_fs **prev;


  const char *name;


  holy_err_t (*dir) (holy_device_t device, const char *path,
       holy_fs_dir_hook_t hook, void *hook_data);


  holy_err_t (*open) (struct holy_file *file, const char *name);


  holy_ssize_t (*read) (struct holy_file *file, char *buf, holy_size_t len);


  holy_err_t (*close) (struct holy_file *file);




  holy_err_t (*label) (holy_device_t device, char **label);




  holy_err_t (*uuid) (holy_device_t device, char **uuid);


  holy_err_t (*mtime) (holy_device_t device, holy_int32_t *timebuf);



  holy_err_t (*embed) (holy_device_t device, unsigned int *nsectors,
         unsigned int max_nsectors,
         holy_embed_type_t embed_type,
         holy_disk_addr_t **sectors);


  int reserved_first_sector;


  int blocklist_install;

};
typedef struct holy_fs *holy_fs_t;


extern struct holy_fs holy_fs_blocklist;





typedef int (*holy_fs_autoload_hook_t) (void);
extern holy_fs_autoload_hook_t holy_fs_autoload_hook;
extern holy_fs_t holy_fs_list;


static inline void
holy_fs_register (holy_fs_t fs)
{
  holy_list_push ((((char *) &(*&holy_fs_list)->next == (char *) &((holy_list_t) (*&holy_fs_list))->next) && ((char *) &(*&holy_fs_list)->prev == (char *) &((holy_list_t) (*&holy_fs_list))->prev) ? (holy_list_t *) (void *) &holy_fs_list : (holy_list_t *) holy_bad_type_cast_real(117, "util/holy-fstest.c")), (((char *) &(fs)->next == (char *) &((holy_list_t) (fs))->next) && ((char *) &(fs)->prev == (char *) &((holy_list_t) (fs))->prev) ? (holy_list_t) fs : (holy_list_t) holy_bad_type_cast_real(117, "util/holy-fstest.c")));
}


static inline void
holy_fs_unregister (holy_fs_t fs)
{
  holy_list_remove ((((char *) &(fs)->next == (char *) &((holy_list_t) (fs))->next) && ((char *) &(fs)->prev == (char *) &((holy_list_t) (fs))->prev) ? (holy_list_t) fs : (holy_list_t) holy_bad_type_cast_real(124, "util/holy-fstest.c")));
}



holy_fs_t holy_fs_probe (holy_device_t device);
# 9 "holy-core/kern/emu/hostfs.c" 2
# 1 "./include/holy/file.h" 1
# 15 "./include/holy/file.h"
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long long int holy_int64_t;

typedef holy_int64_t holy_ssize_t;
typedef holy_uint64_t holy_size_t;

typedef holy_uint64_t holy_off_t;
typedef holy_uint64_t holy_disk_addr_t;


struct holy_disk;
struct holy_net;

struct holy_device {
    struct holy_disk *disk;
    struct holy_net *net;
}

typedef struct holy_device *holy_device_t;



struct holy_file
{

  char *name;


  holy_device_t device;


  holy_fs_t fs;


  holy_off_t offset;
  holy_off_t progress_offset;


  holy_uint64_t last_progress_time;
  holy_off_t last_progress_offset;
  holy_uint64_t estimated_speed;


  holy_off_t size;


  int not_easily_seekable;


  void *data;


  holy_disk_read_hook_t read_hook;


  void *read_hook_data;
};
typedef struct holy_file *holy_file_t;

extern holy_disk_read_hook_t holy_file_progress_hook;


typedef enum holy_file_filter_id
  {
    holy_FILE_FILTER_PUBKEY,
    holy_FILE_FILTER_GZIO,
    holy_FILE_FILTER_XZIO,
    holy_FILE_FILTER_LZOPIO,
    holy_FILE_FILTER_MAX,
    holy_FILE_FILTER_COMPRESSION_FIRST = holy_FILE_FILTER_GZIO,
    holy_FILE_FILTER_COMPRESSION_LAST = holy_FILE_FILTER_LZOPIO,
  } holy_file_filter_id_t;

typedef holy_file_t (*holy_file_filter_t) (holy_file_t in, const char *filename);

extern holy_file_filter_t holy_file_filters_all[holy_FILE_FILTER_MAX];
extern holy_file_filter_t holy_file_filters_enabled[holy_FILE_FILTER_MAX];

static inline void
holy_file_filter_register (holy_file_filter_id_t id, holy_file_filter_t filter)
{
  holy_file_filters_all[id] = filter;
  holy_file_filters_enabled[id] = filter;
}

static inline void
holy_file_filter_unregister (holy_file_filter_id_t id)
{
  holy_file_filters_all[id] = 0;
  holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable (holy_file_filter_id_t id)
{
  holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_compression (void)
{
  holy_file_filter_id_t id;

  for (id = holy_FILE_FILTER_COMPRESSION_FIRST;
       id <= holy_FILE_FILTER_COMPRESSION_LAST; id++)
    holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_all (void)
{
  holy_file_filter_id_t id;

  for (id = 0;
       id < holy_FILE_FILTER_MAX; id++)
    holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_pubkey (void)
{
  holy_file_filters_enabled[holy_FILE_FILTER_PUBKEY] = 0;
}


char *holy_file_get_device_name (const char *name);

holy_file_t holy_file_open (const char *name);
holy_ssize_t holy_file_read (holy_file_t file, void *buf,
       holy_size_t len);
holy_off_t holy_file_seek (holy_file_t file, holy_off_t offset);
holy_err_t holy_file_close (holy_file_t file);




static inline holy_off_t
holy_file_size (const holy_file_t file)
{
  return file->size;
}

static inline holy_off_t
holy_file_tell (const holy_file_t file)
{
  return file->offset;
}

static inline int
holy_file_seekable (const holy_file_t file)
{
  return !file->not_easily_seekable;
}

holy_file_t
holy_file_offset_open (holy_file_t parent, holy_off_t start,
         holy_off_t size);
void
holy_file_offset_close (holy_file_t file);
# 10 "holy-core/kern/emu/hostfs.c" 2




# 1 "./include/holy/util/misc.h" 1
# 9 "./include/holy/util/misc.h"
# 1 "/usr/include/stdlib.h" 1 3 4
# 26 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/stdlib.h" 2 3 4





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 344 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4

# 344 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4
typedef int wchar_t;
# 33 "/usr/include/stdlib.h" 2 3 4







# 1 "/usr/include/bits/waitflags.h" 1 3 4
# 41 "/usr/include/stdlib.h" 2 3 4
# 1 "/usr/include/bits/waitstatus.h" 1 3 4
# 42 "/usr/include/stdlib.h" 2 3 4
# 59 "/usr/include/stdlib.h" 3 4
typedef struct
  {
    int quot;
    int rem;
  } div_t;



typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;





__extension__ typedef struct
  {
    long long int quot;
    long long int rem;
  } lldiv_t;
# 98 "/usr/include/stdlib.h" 3 4
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__ , __leaf__)) ;



extern double atof (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern int atoi (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern long int atol (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;



__extension__ extern long long int atoll (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;



extern double strtod (const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern float strtof (const char *__restrict __nptr,
       char **__restrict __endptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern long double strtold (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 141 "/usr/include/stdlib.h" 3 4
extern _Float32 strtof32 (const char *__restrict __nptr,
     char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float64 strtof64 (const char *__restrict __nptr,
     char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float128 strtof128 (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float32x strtof32x (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float64x strtof64x (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 177 "/usr/include/stdlib.h" 3 4
extern long int strtol (const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern unsigned long int strtoul (const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



__extension__
extern long long int strtoq (const char *__restrict __nptr,
        char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr,
           char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




__extension__
extern long long int strtoll (const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern long int strtol (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtol") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
extern unsigned long int strtoul (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoul") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));

__extension__
extern long long int strtoq (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoll") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoull") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));

__extension__
extern long long int strtoll (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoll") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoull") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));
# 278 "/usr/include/stdlib.h" 3 4
extern int strfromd (char *__dest, size_t __size, const char *__format,
       double __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));

extern int strfromf (char *__dest, size_t __size, const char *__format,
       float __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));

extern int strfroml (char *__dest, size_t __size, const char *__format,
       long double __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 298 "/usr/include/stdlib.h" 3 4
extern int strfromf32 (char *__dest, size_t __size, const char * __format,
         _Float32 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf64 (char *__dest, size_t __size, const char * __format,
         _Float64 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf128 (char *__dest, size_t __size, const char * __format,
   _Float128 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf32x (char *__dest, size_t __size, const char * __format,
   _Float32x __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf64x (char *__dest, size_t __size, const char * __format,
   _Float64x __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 340 "/usr/include/stdlib.h" 3 4
extern long int strtol_l (const char *__restrict __nptr,
     char **__restrict __endptr, int __base,
     locale_t __loc) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

extern unsigned long int strtoul_l (const char *__restrict __nptr,
        char **__restrict __endptr,
        int __base, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

__extension__
extern long long int strtoll_l (const char *__restrict __nptr,
    char **__restrict __endptr, int __base,
    locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

__extension__
extern unsigned long long int strtoull_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       int __base, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));





extern long int strtol_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtol_l") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1, 4)));
extern unsigned long int strtoul_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoul_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
__extension__
extern long long int strtoll_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoll_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
__extension__
extern unsigned long long int strtoull_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoull_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
# 415 "/usr/include/stdlib.h" 3 4
extern double strtod_l (const char *__restrict __nptr,
   char **__restrict __endptr, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));

extern float strtof_l (const char *__restrict __nptr,
         char **__restrict __endptr, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));

extern long double strtold_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));
# 436 "/usr/include/stdlib.h" 3 4
extern _Float32 strtof32_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float64 strtof64_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float128 strtof128_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float32x strtof32x_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float64x strtof64x_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));
# 505 "/usr/include/stdlib.h" 3 4
extern char *l64a (long int __n) __attribute__ ((__nothrow__ , __leaf__)) ;


extern long int a64l (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;
# 521 "/usr/include/stdlib.h" 3 4
extern long int random (void) __attribute__ ((__nothrow__ , __leaf__));


extern void srandom (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));





extern char *initstate (unsigned int __seed, char *__statebuf,
   size_t __statelen) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern char *setstate (char *__statebuf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







struct random_data
  {
    int32_t *fptr;
    int32_t *rptr;
    int32_t *state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t *end_ptr;
  };

extern int random_r (struct random_data *__restrict __buf,
       int32_t *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern int srandom_r (unsigned int __seed, struct random_data *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
   size_t __statelen,
   struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));

extern int setstate_r (char *__restrict __statebuf,
         struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern int rand (void) __attribute__ ((__nothrow__ , __leaf__));

extern void srand (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));



extern int rand_r (unsigned int *__seed) __attribute__ ((__nothrow__ , __leaf__));







extern double drand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern double erand48 (unsigned short int __xsubi[3]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int lrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int nrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int mrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int jrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void srand48 (long int __seedval) __attribute__ ((__nothrow__ , __leaf__));
extern unsigned short int *seed48 (unsigned short int __seed16v[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern void lcong48 (unsigned short int __param[7]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





struct drand48_data
  {
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    __extension__ unsigned long long int __a;

  };


extern int drand48_r (struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int erand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int lrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int nrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int mrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int jrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

extern int seed48_r (unsigned short int __seed16v[3],
       struct drand48_data *__buffer) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern int lcong48_r (unsigned short int __param[7],
        struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern __uint32_t arc4random (void)
     __attribute__ ((__nothrow__ , __leaf__)) ;


extern void arc4random_buf (void *__buf, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern __uint32_t arc4random_uniform (__uint32_t __upper_bound)
     __attribute__ ((__nothrow__ , __leaf__)) ;




extern void *malloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__))
     __attribute__ ((__alloc_size__ (1))) ;

extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_size__ (1, 2))) ;






extern void *realloc (void *__ptr, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__)) __attribute__ ((__alloc_size__ (2)));


extern void free (void *__ptr) __attribute__ ((__nothrow__ , __leaf__));







extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__))
     __attribute__ ((__alloc_size__ (2, 3)))
    __attribute__ ((__malloc__ (__builtin_free, 1)));


extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__ (reallocarray, 1)));



# 1 "/usr/include/alloca.h" 1 3 4
# 24 "/usr/include/alloca.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 25 "/usr/include/alloca.h" 2 3 4







extern void *alloca (size_t __size) __attribute__ ((__nothrow__ , __leaf__));






# 707 "/usr/include/stdlib.h" 2 3 4





extern void *valloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__))
     __attribute__ ((__alloc_size__ (1))) ;




extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;




extern void *aligned_alloc (size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_align__ (1)))
     __attribute__ ((__alloc_size__ (2))) ;



extern void abort (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));



extern int atexit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int at_quick_exit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern void exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));





extern void quick_exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));





extern void _Exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));




extern char *getenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;




extern char *secure_getenv (const char *__name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;






extern int putenv (char *__string) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int setenv (const char *__name, const char *__value, int __replace)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));


extern int unsetenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern int clearenv (void) __attribute__ ((__nothrow__ , __leaf__));
# 814 "/usr/include/stdlib.h" 3 4
extern char *mktemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 830 "/usr/include/stdlib.h" 3 4
extern int mkstemp (char *__template) __asm__ ("" "mkstemp64")
     __attribute__ ((__nonnull__ (1))) ;





extern int mkstemp64 (char *__template) __attribute__ ((__nonnull__ (1))) ;
# 852 "/usr/include/stdlib.h" 3 4
extern int mkstemps (char *__template, int __suffixlen) __asm__ ("" "mkstemps64")
                     __attribute__ ((__nonnull__ (1))) ;





extern int mkstemps64 (char *__template, int __suffixlen)
     __attribute__ ((__nonnull__ (1))) ;
# 870 "/usr/include/stdlib.h" 3 4
extern char *mkdtemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 884 "/usr/include/stdlib.h" 3 4
extern int mkostemp (char *__template, int __flags) __asm__ ("" "mkostemp64")
     __attribute__ ((__nonnull__ (1))) ;





extern int mkostemp64 (char *__template, int __flags) __attribute__ ((__nonnull__ (1))) ;
# 905 "/usr/include/stdlib.h" 3 4
extern int mkostemps (char *__template, int __suffixlen, int __flags) __asm__ ("" "mkostemps64")

     __attribute__ ((__nonnull__ (1))) ;





extern int mkostemps64 (char *__template, int __suffixlen, int __flags)
     __attribute__ ((__nonnull__ (1))) ;
# 923 "/usr/include/stdlib.h" 3 4
extern int system (const char *__command) ;





extern char *canonicalize_file_name (const char *__name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__malloc__))
     __attribute__ ((__malloc__ (__builtin_free, 1))) ;
# 940 "/usr/include/stdlib.h" 3 4
extern char *realpath (const char *__restrict __name,
         char *__restrict __resolved) __attribute__ ((__nothrow__ , __leaf__)) ;






typedef int (*__compar_fn_t) (const void *, const void *);


typedef __compar_fn_t comparison_fn_t;



typedef int (*__compar_d_fn_t) (const void *, const void *, void *);




extern void *bsearch (const void *__key, const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;







extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));

extern void qsort_r (void *__base, size_t __nmemb, size_t __size,
       __compar_d_fn_t __compar, void *__arg)
  __attribute__ ((__nonnull__ (1, 4)));




extern int abs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern long int labs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;


__extension__ extern long long int llabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;



extern unsigned int uabs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern unsigned long int ulabs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
__extension__ extern unsigned long long int ullabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;





extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;


__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
# 1018 "/usr/include/stdlib.h" 3 4
extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *gcvt (double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;




extern char *qecvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qfcvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;




extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));

extern int qecvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qfcvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));





extern int mblen (const char *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));


extern int mbtowc (wchar_t *__restrict __pwc,
     const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));


extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__ , __leaf__));



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__read_only__, 2)));

extern size_t wcstombs (char *__restrict __s,
   const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1, 3)))
  __attribute__ ((__access__ (__read_only__, 2)));






extern int rpmatch (const char *__response) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 1105 "/usr/include/stdlib.h" 3 4
extern int getsubopt (char **__restrict __optionp,
        char *const *__restrict __tokens,
        char **__restrict __valuep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2, 3))) ;







extern int posix_openpt (int __oflag) ;







extern int grantpt (int __fd) __attribute__ ((__nothrow__ , __leaf__));



extern int unlockpt (int __fd) __attribute__ ((__nothrow__ , __leaf__));




extern char *ptsname (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int ptsname_r (int __fd, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 2, 3)));


extern int getpt (void);






extern int getloadavg (double __loadavg[], int __nelem)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 1161 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/bits/stdlib-float.h" 1 3 4
# 1162 "/usr/include/stdlib.h" 2 3 4
# 1173 "/usr/include/stdlib.h" 3 4

# 10 "./include/holy/util/misc.h" 2


# 1 "/usr/include/setjmp.h" 1 3 4
# 27 "/usr/include/setjmp.h" 3 4


# 1 "/usr/include/bits/setjmp.h" 1 3 4
# 26 "/usr/include/bits/setjmp.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 27 "/usr/include/bits/setjmp.h" 2 3 4




typedef long int __jmp_buf[8];
# 30 "/usr/include/setjmp.h" 2 3 4
# 1 "/usr/include/bits/types/struct___jmp_buf_tag.h" 1 3 4
# 26 "/usr/include/bits/types/struct___jmp_buf_tag.h" 3 4
struct __jmp_buf_tag
  {




    __jmp_buf __jmpbuf;
    int __mask_was_saved;
    __sigset_t __saved_mask;
  };
# 31 "/usr/include/setjmp.h" 2 3 4

typedef struct __jmp_buf_tag jmp_buf[1];



extern int setjmp (jmp_buf __env) __attribute__ ((__nothrow__));




extern int __sigsetjmp (struct __jmp_buf_tag __env[1], int __savemask) __attribute__ ((__nothrow__));



extern int _setjmp (struct __jmp_buf_tag __env[1]) __attribute__ ((__nothrow__));
# 54 "/usr/include/setjmp.h" 3 4
extern void longjmp (struct __jmp_buf_tag __env[1], int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));





extern void _longjmp (struct __jmp_buf_tag __env[1], int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));







typedef struct __jmp_buf_tag sigjmp_buf[1];
# 80 "/usr/include/setjmp.h" 3 4
extern void siglongjmp (sigjmp_buf __env, int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));
# 90 "/usr/include/setjmp.h" 3 4

# 13 "./include/holy/util/misc.h" 2


# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 16 "./include/holy/util/misc.h" 2


# 1 "./include/holy/emu/misc.h" 1
# 10 "./include/holy/emu/misc.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 11 "./include/holy/emu/misc.h" 2



# 1 "./include/holy/gcrypt/gcrypt.h" 1





# 1 "./include/holy/gcrypt/gpg-error.h" 1
# 9 "./include/holy/gcrypt/gpg-error.h"
# 1 "./include/holy/crypto.h" 1
# 14 "./include/holy/crypto.h"

# 14 "./include/holy/crypto.h"
typedef enum
  {
    GPG_ERR_NO_ERROR,
    GPG_ERR_BAD_MPI,
    GPG_ERR_BAD_SECKEY,
    GPG_ERR_BAD_SIGNATURE,
    GPG_ERR_CIPHER_ALGO,
    GPG_ERR_CONFLICT,
    GPG_ERR_DECRYPT_FAILED,
    GPG_ERR_DIGEST_ALGO,
    GPG_ERR_GENERAL,
    GPG_ERR_INTERNAL,
    GPG_ERR_INV_ARG,
    GPG_ERR_INV_CIPHER_MODE,
    GPG_ERR_INV_FLAG,
    GPG_ERR_INV_KEYLEN,
    GPG_ERR_INV_OBJ,
    GPG_ERR_INV_OP,
    GPG_ERR_INV_SEXP,
    GPG_ERR_INV_VALUE,
    GPG_ERR_MISSING_VALUE,
    GPG_ERR_NO_ENCRYPTION_SCHEME,
    GPG_ERR_NO_OBJ,
    GPG_ERR_NO_PRIME,
    GPG_ERR_NO_SIGNATURE_SCHEME,
    GPG_ERR_NOT_FOUND,
    GPG_ERR_NOT_IMPLEMENTED,
    GPG_ERR_NOT_SUPPORTED,
    GPG_ERROR_CFLAGS,
    GPG_ERR_PUBKEY_ALGO,
    GPG_ERR_SELFTEST_FAILED,
    GPG_ERR_TOO_SHORT,
    GPG_ERR_UNSUPPORTED,
    GPG_ERR_WEAK_KEY,
    GPG_ERR_WRONG_KEY_USAGE,
    GPG_ERR_WRONG_PUBKEY_ALGO,
    GPG_ERR_OUT_OF_MEMORY,
    GPG_ERR_TOO_LARGE,
    GPG_ERR_ENOMEM
  } gpg_err_code_t;
typedef gpg_err_code_t gpg_error_t;
typedef gpg_error_t gcry_error_t;
typedef gpg_err_code_t gcry_err_code_t;
# 77 "./include/holy/crypto.h"
typedef gcry_err_code_t (*gcry_cipher_setkey_t) (void *c,
       const unsigned char *key,
       unsigned keylen);


typedef void (*gcry_cipher_encrypt_t) (void *c,
           unsigned char *outbuf,
           const unsigned char *inbuf);


typedef void (*gcry_cipher_decrypt_t) (void *c,
           unsigned char *outbuf,
           const unsigned char *inbuf);


typedef void (*gcry_cipher_stencrypt_t) (void *c,
      unsigned char *outbuf,
      const unsigned char *inbuf,
      unsigned int n);


typedef void (*gcry_cipher_stdecrypt_t) (void *c,
      unsigned char *outbuf,
      const unsigned char *inbuf,
      unsigned int n);

typedef struct gcry_cipher_oid_spec
{
  const char *oid;
  int mode;
} gcry_cipher_oid_spec_t;


typedef struct gcry_cipher_spec
{
  const char *name;
  const char **aliases;
  gcry_cipher_oid_spec_t *oids;
  holy_size_t blocksize;
  holy_size_t keylen;
  holy_size_t contextsize;
  gcry_cipher_setkey_t setkey;
  gcry_cipher_encrypt_t encrypt;
  gcry_cipher_decrypt_t decrypt;
  gcry_cipher_stencrypt_t stencrypt;
  gcry_cipher_stdecrypt_t stdecrypt;

  const char *modname;

  struct gcry_cipher_spec *next;
} gcry_cipher_spec_t;


typedef void (*gcry_md_init_t) (void *c);


typedef void (*gcry_md_write_t) (void *c, const void *buf, holy_size_t nbytes);


typedef void (*gcry_md_final_t) (void *c);


typedef unsigned char *(*gcry_md_read_t) (void *c);

typedef struct gcry_md_oid_spec
{
  const char *oidstring;
} gcry_md_oid_spec_t;


typedef struct gcry_md_spec
{
  const char *name;
  unsigned char *asnoid;
  int asnlen;
  gcry_md_oid_spec_t *oids;
  holy_size_t mdlen;
  gcry_md_init_t init;
  gcry_md_write_t write;
  gcry_md_final_t final;
  gcry_md_read_t read;
  holy_size_t contextsize;

  holy_size_t blocksize;

  const char *modname;

  struct gcry_md_spec *next;
} gcry_md_spec_t;

struct gcry_mpi;
typedef struct gcry_mpi *gcry_mpi_t;


typedef gcry_err_code_t (*gcry_pk_generate_t) (int algo,
            unsigned int nbits,
            unsigned long use_e,
            gcry_mpi_t *skey,
            gcry_mpi_t **retfactors);


typedef gcry_err_code_t (*gcry_pk_check_secret_key_t) (int algo,
             gcry_mpi_t *skey);


typedef gcry_err_code_t (*gcry_pk_encrypt_t) (int algo,
           gcry_mpi_t *resarr,
           gcry_mpi_t data,
           gcry_mpi_t *pkey,
           int flags);


typedef gcry_err_code_t (*gcry_pk_decrypt_t) (int algo,
           gcry_mpi_t *result,
           gcry_mpi_t *data,
           gcry_mpi_t *skey,
           int flags);


typedef gcry_err_code_t (*gcry_pk_sign_t) (int algo,
        gcry_mpi_t *resarr,
        gcry_mpi_t data,
        gcry_mpi_t *skey);


typedef gcry_err_code_t (*gcry_pk_verify_t) (int algo,
          gcry_mpi_t hash,
          gcry_mpi_t *data,
          gcry_mpi_t *pkey,
          int (*cmp) (void *, gcry_mpi_t),
          void *opaquev);


typedef unsigned (*gcry_pk_get_nbits_t) (int algo, gcry_mpi_t *pkey);


typedef struct gcry_pk_spec
{
  const char *name;
  const char **aliases;
  const char *elements_pkey;
  const char *elements_skey;
  const char *elements_enc;
  const char *elements_sig;
  const char *elements_grip;
  int use;
  gcry_pk_generate_t generate;
  gcry_pk_check_secret_key_t check_secret_key;
  gcry_pk_encrypt_t encrypt;
  gcry_pk_decrypt_t decrypt;
  gcry_pk_sign_t sign;
  gcry_pk_verify_t verify;
  gcry_pk_get_nbits_t get_nbits;

  const char *modname;

} gcry_pk_spec_t;

struct holy_crypto_cipher_handle
{
  const struct gcry_cipher_spec *cipher;
  char ctx[0];
};

typedef struct holy_crypto_cipher_handle *holy_crypto_cipher_handle_t;

struct holy_crypto_hmac_handle;

const gcry_cipher_spec_t *
holy_crypto_lookup_cipher_by_name (const char *name);

holy_crypto_cipher_handle_t
holy_crypto_cipher_open (const struct gcry_cipher_spec *cipher);

gcry_err_code_t
holy_crypto_cipher_set_key (holy_crypto_cipher_handle_t cipher,
       const unsigned char *key,
       unsigned keylen);

static inline void
holy_crypto_cipher_close (holy_crypto_cipher_handle_t cipher)
{
  holy_free (cipher);
}

static inline void
holy_crypto_xor (void *out, const void *in1, const void *in2, holy_size_t size)
{
  const holy_uint8_t *in1ptr = in1, *in2ptr = in2;
  holy_uint8_t *outptr = out;
  while (size && (((holy_addr_t) in1ptr & (sizeof (holy_uint64_t) - 1))
    || ((holy_addr_t) in2ptr & (sizeof (holy_uint64_t) - 1))
    || ((holy_addr_t) outptr & (sizeof (holy_uint64_t) - 1))))
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
      size--;
    }
  while (size >= sizeof (holy_uint64_t))
    {

      *(holy_uint64_t *) (void *) outptr
 = (*(const holy_uint64_t *) (const void *) in1ptr
    ^ *(const holy_uint64_t *) (const void *) in2ptr);
      in1ptr += sizeof (holy_uint64_t);
      in2ptr += sizeof (holy_uint64_t);
      outptr += sizeof (holy_uint64_t);
      size -= sizeof (holy_uint64_t);
    }
  while (size)
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
      size--;
    }
}

gcry_err_code_t
holy_crypto_ecb_decrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size);

gcry_err_code_t
holy_crypto_ecb_encrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size);
gcry_err_code_t
holy_crypto_cbc_encrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size,
    void *iv_in);
gcry_err_code_t
holy_crypto_cbc_decrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size,
    void *iv);
void
holy_cipher_register (gcry_cipher_spec_t *cipher);
void
holy_cipher_unregister (gcry_cipher_spec_t *cipher);
void
holy_md_register (gcry_md_spec_t *digest);
void
holy_md_unregister (gcry_md_spec_t *cipher);

extern struct gcry_pk_spec *holy_crypto_pk_dsa;
extern struct gcry_pk_spec *holy_crypto_pk_ecdsa;
extern struct gcry_pk_spec *holy_crypto_pk_ecdh;
extern struct gcry_pk_spec *holy_crypto_pk_rsa;

void
holy_crypto_hash (const gcry_md_spec_t *hash, void *out, const void *in,
    holy_size_t inlen);
const gcry_md_spec_t *
holy_crypto_lookup_md_by_name (const char *name);

holy_err_t
holy_crypto_gcry_error (gcry_err_code_t in);

void holy_burn_stack (holy_size_t size);

struct holy_crypto_hmac_handle *
holy_crypto_hmac_init (const struct gcry_md_spec *md,
         const void *key, holy_size_t keylen);
void
holy_crypto_hmac_write (struct holy_crypto_hmac_handle *hnd,
   const void *data,
   holy_size_t datalen);
gcry_err_code_t
holy_crypto_hmac_fini (struct holy_crypto_hmac_handle *hnd, void *out);

gcry_err_code_t
holy_crypto_hmac_buffer (const struct gcry_md_spec *md,
    const void *key, holy_size_t keylen,
    const void *data, holy_size_t datalen, void *out);

extern gcry_md_spec_t _gcry_digest_spec_md5;
extern gcry_md_spec_t _gcry_digest_spec_sha1;
extern gcry_md_spec_t _gcry_digest_spec_sha256;
extern gcry_md_spec_t _gcry_digest_spec_sha512;
extern gcry_md_spec_t _gcry_digest_spec_crc32;
extern gcry_cipher_spec_t _gcry_cipher_spec_aes;
# 372 "./include/holy/crypto.h"
gcry_err_code_t
holy_crypto_pbkdf2 (const struct gcry_md_spec *md,
      const holy_uint8_t *P, holy_size_t Plen,
      const holy_uint8_t *S, holy_size_t Slen,
      unsigned int c,
      holy_uint8_t *DK, holy_size_t dkLen);

int
holy_crypto_memcmp (const void *a, const void *b, holy_size_t n);

int
holy_password_get (char buf[], unsigned buf_size);




extern void (*holy_crypto_autoload_hook) (const char *name);

void _gcry_assert_failed (const char *expr, const char *file, int line,
                          const char *func) __attribute__ ((noreturn));

void _gcry_burn_stack (int bytes);
void _gcry_log_error( const char *fmt, ... ) __attribute__ ((format (__printf__, 1, 2)));



void holy_gcry_init_all (void);
void holy_gcry_fini_all (void);

int
holy_get_random (void *out, holy_size_t len);
# 10 "./include/holy/gcrypt/gpg-error.h" 2
typedef enum
  {
    GPG_ERR_SOURCE_USER_1
  }
  gpg_err_source_t;

static inline int
gpg_err_make (gpg_err_source_t source __attribute__ ((unused)), gpg_err_code_t code)
{
  return code;
}

static inline gpg_err_code_t
gpg_err_code (gpg_error_t err)
{
  return err;
}

static inline gpg_err_source_t
gpg_err_source (gpg_error_t err __attribute__ ((unused)))
{
  return GPG_ERR_SOURCE_USER_1;
}

gcry_err_code_t
gpg_error_from_syserror (void);
# 7 "./include/holy/gcrypt/gcrypt.h" 2
# 90 "./include/holy/gcrypt/gcrypt.h"
typedef gpg_err_source_t gcry_err_source_t;

static inline gcry_err_code_t
gcry_err_make (gcry_err_source_t source, gcry_err_code_t code)
{
  return gpg_err_make (source, code);
}







static inline gcry_err_code_t
gcry_error (gcry_err_code_t code)
{
  return gcry_err_make (GPG_ERR_SOURCE_USER_1, code);
}

static inline gcry_err_code_t
gcry_err_code (gcry_err_code_t err)
{
  return gpg_err_code (err);
}


static inline gcry_err_source_t
gcry_err_source (gcry_err_code_t err)
{
  return gpg_err_source (err);
}



const char *gcry_strerror (gcry_err_code_t err);



const char *gcry_strsource (gcry_err_code_t err);




gcry_err_code_t gcry_err_code_from_errno (int err);



int gcry_err_code_to_errno (gcry_err_code_t code);



gcry_err_code_t gcry_err_make_from_errno (gcry_err_source_t source, int err);


gcry_err_code_t gcry_error_from_errno (int err);



struct gcry_mpi;
# 159 "./include/holy/gcrypt/gcrypt.h"
const char *gcry_check_version (const char *req_version);




enum gcry_ctl_cmds
  {
    GCRYCTL_SET_KEY = 1,
    GCRYCTL_SET_IV = 2,
    GCRYCTL_CFB_SYNC = 3,
    GCRYCTL_RESET = 4,
    GCRYCTL_FINALIZE = 5,
    GCRYCTL_GET_KEYLEN = 6,
    GCRYCTL_GET_BLKLEN = 7,
    GCRYCTL_TEST_ALGO = 8,
    GCRYCTL_IS_SECURE = 9,
    GCRYCTL_GET_ASNOID = 10,
    GCRYCTL_ENABLE_ALGO = 11,
    GCRYCTL_DISABLE_ALGO = 12,
    GCRYCTL_DUMP_RANDOM_STATS = 13,
    GCRYCTL_DUMP_SECMEM_STATS = 14,
    GCRYCTL_GET_ALGO_NPKEY = 15,
    GCRYCTL_GET_ALGO_NSKEY = 16,
    GCRYCTL_GET_ALGO_NSIGN = 17,
    GCRYCTL_GET_ALGO_NENCR = 18,
    GCRYCTL_SET_VERBOSITY = 19,
    GCRYCTL_SET_DEBUG_FLAGS = 20,
    GCRYCTL_CLEAR_DEBUG_FLAGS = 21,
    GCRYCTL_USE_SECURE_RNDPOOL= 22,
    GCRYCTL_DUMP_MEMORY_STATS = 23,
    GCRYCTL_INIT_SECMEM = 24,
    GCRYCTL_TERM_SECMEM = 25,
    GCRYCTL_DISABLE_SECMEM_WARN = 27,
    GCRYCTL_SUSPEND_SECMEM_WARN = 28,
    GCRYCTL_RESUME_SECMEM_WARN = 29,
    GCRYCTL_DROP_PRIVS = 30,
    GCRYCTL_ENABLE_M_GUARD = 31,
    GCRYCTL_START_DUMP = 32,
    GCRYCTL_STOP_DUMP = 33,
    GCRYCTL_GET_ALGO_USAGE = 34,
    GCRYCTL_IS_ALGO_ENABLED = 35,
    GCRYCTL_DISABLE_INTERNAL_LOCKING = 36,
    GCRYCTL_DISABLE_SECMEM = 37,
    GCRYCTL_INITIALIZATION_FINISHED = 38,
    GCRYCTL_INITIALIZATION_FINISHED_P = 39,
    GCRYCTL_ANY_INITIALIZATION_P = 40,
    GCRYCTL_SET_CBC_CTS = 41,
    GCRYCTL_SET_CBC_MAC = 42,
    GCRYCTL_SET_CTR = 43,
    GCRYCTL_ENABLE_QUICK_RANDOM = 44,
    GCRYCTL_SET_RANDOM_SEED_FILE = 45,
    GCRYCTL_UPDATE_RANDOM_SEED_FILE = 46,
    GCRYCTL_SET_THREAD_CBS = 47,
    GCRYCTL_FAST_POLL = 48,
    GCRYCTL_SET_RANDOM_DAEMON_SOCKET = 49,
    GCRYCTL_USE_RANDOM_DAEMON = 50,
    GCRYCTL_FAKED_RANDOM_P = 51,
    GCRYCTL_SET_RNDEGD_SOCKET = 52,
    GCRYCTL_PRINT_CONFIG = 53,
    GCRYCTL_OPERATIONAL_P = 54,
    GCRYCTL_FIPS_MODE_P = 55,
    GCRYCTL_FORCE_FIPS_MODE = 56,
    GCRYCTL_SELFTEST = 57,

    GCRYCTL_DISABLE_HWF = 63,
    GCRYCTL_SET_ENFORCED_FIPS_FLAG = 64
  };


gcry_err_code_t gcry_control (enum gcry_ctl_cmds CMD, ...);






struct gcry_sexp;
typedef struct gcry_sexp *gcry_sexp_t;







enum gcry_sexp_format
  {
    GCRYSEXP_FMT_DEFAULT = 0,
    GCRYSEXP_FMT_CANON = 1,
    GCRYSEXP_FMT_BASE64 = 2,
    GCRYSEXP_FMT_ADVANCED = 3
  };




gcry_err_code_t gcry_sexp_new (gcry_sexp_t *retsexp,
                            const void *buffer, size_t length,
                            int autodetect);



gcry_err_code_t gcry_sexp_create (gcry_sexp_t *retsexp,
                               void *buffer, size_t length,
                               int autodetect, void (*freefnc) (void *));



gcry_err_code_t gcry_sexp_sscan (gcry_sexp_t *retsexp, size_t *erroff,
                              const char *buffer, size_t length);



gcry_err_code_t gcry_sexp_build (gcry_sexp_t *retsexp, size_t *erroff,
                              const char *format, ...);



gcry_err_code_t gcry_sexp_build_array (gcry_sexp_t *retsexp, size_t *erroff,
        const char *format, void **arg_list);


void gcry_sexp_release (gcry_sexp_t sexp);



size_t gcry_sexp_canon_len (const unsigned char *buffer, size_t length,
                            size_t *erroff, gcry_err_code_t *errcode);



size_t gcry_sexp_sprint (gcry_sexp_t sexp, int mode, void *buffer,
                         size_t maxlength);



void gcry_sexp_dump (const gcry_sexp_t a);

gcry_sexp_t gcry_sexp_cons (const gcry_sexp_t a, const gcry_sexp_t b);
gcry_sexp_t gcry_sexp_alist (const gcry_sexp_t *array);
gcry_sexp_t gcry_sexp_vlist (const gcry_sexp_t a, ...);
gcry_sexp_t gcry_sexp_append (const gcry_sexp_t a, const gcry_sexp_t n);
gcry_sexp_t gcry_sexp_prepend (const gcry_sexp_t a, const gcry_sexp_t n);






gcry_sexp_t gcry_sexp_find_token (gcry_sexp_t list,
                                const char *tok, size_t toklen);


int gcry_sexp_length (const gcry_sexp_t list);




gcry_sexp_t gcry_sexp_nth (const gcry_sexp_t list, int number);




gcry_sexp_t gcry_sexp_car (const gcry_sexp_t list);






gcry_sexp_t gcry_sexp_cdr (const gcry_sexp_t list);

gcry_sexp_t gcry_sexp_cadr (const gcry_sexp_t list);
# 340 "./include/holy/gcrypt/gcrypt.h"
const char *gcry_sexp_nth_data (const gcry_sexp_t list, int number,
                                size_t *datalen);






char *gcry_sexp_nth_string (gcry_sexp_t list, int number);







gcry_mpi_t gcry_sexp_nth_mpi (gcry_sexp_t list, int number, int mpifmt);
# 367 "./include/holy/gcrypt/gcrypt.h"
enum gcry_mpi_format
  {
    GCRYMPI_FMT_NONE= 0,
    GCRYMPI_FMT_STD = 1,
    GCRYMPI_FMT_PGP = 2,
    GCRYMPI_FMT_SSH = 3,
    GCRYMPI_FMT_HEX = 4,
    GCRYMPI_FMT_USG = 5
  };


enum gcry_mpi_flag
  {
    GCRYMPI_FLAG_SECURE = 1,
    GCRYMPI_FLAG_OPAQUE = 2


  };




gcry_mpi_t gcry_mpi_new (unsigned int nbits);


gcry_mpi_t gcry_mpi_snew (unsigned int nbits);


void gcry_mpi_release (gcry_mpi_t a);


gcry_mpi_t gcry_mpi_copy (const gcry_mpi_t a);


gcry_mpi_t gcry_mpi_set (gcry_mpi_t w, const gcry_mpi_t u);


gcry_mpi_t gcry_mpi_set_ui (gcry_mpi_t w, unsigned long u);


void gcry_mpi_swap (gcry_mpi_t a, gcry_mpi_t b);



int gcry_mpi_cmp (const gcry_mpi_t u, const gcry_mpi_t v);




int gcry_mpi_cmp_ui (const gcry_mpi_t u, unsigned long v);





gcry_err_code_t gcry_mpi_scan (gcry_mpi_t *ret_mpi, enum gcry_mpi_format format,
                            const void *buffer, size_t buflen,
                            size_t *nscanned);






gcry_err_code_t gcry_mpi_print (enum gcry_mpi_format format,
                             unsigned char *buffer, size_t buflen,
                             size_t *nwritten,
                             const gcry_mpi_t a);





gcry_err_code_t gcry_mpi_aprint (enum gcry_mpi_format format,
                              unsigned char **buffer, size_t *nwritten,
                              const gcry_mpi_t a);





void gcry_mpi_dump (const gcry_mpi_t a);



void gcry_mpi_add (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_add_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v);


void gcry_mpi_addm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_sub (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_sub_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v );


void gcry_mpi_subm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_mul (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_mul_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v );


void gcry_mpi_mulm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_mul_2exp (gcry_mpi_t w, gcry_mpi_t u, unsigned long cnt);



void gcry_mpi_div (gcry_mpi_t q, gcry_mpi_t r,
                   gcry_mpi_t dividend, gcry_mpi_t divisor, int round);


void gcry_mpi_mod (gcry_mpi_t r, gcry_mpi_t dividend, gcry_mpi_t divisor);


void gcry_mpi_powm (gcry_mpi_t w,
                    const gcry_mpi_t b, const gcry_mpi_t e,
                    const gcry_mpi_t m);



int gcry_mpi_gcd (gcry_mpi_t g, gcry_mpi_t a, gcry_mpi_t b);



int gcry_mpi_invm (gcry_mpi_t x, gcry_mpi_t a, gcry_mpi_t m);



unsigned int gcry_mpi_get_nbits (gcry_mpi_t a);


int gcry_mpi_test_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_set_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_clear_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_set_highbit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_clear_highbit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_rshift (gcry_mpi_t x, gcry_mpi_t a, unsigned int n);


void gcry_mpi_lshift (gcry_mpi_t x, gcry_mpi_t a, unsigned int n);




gcry_mpi_t gcry_mpi_set_opaque (gcry_mpi_t a, void *p, unsigned int nbits);




void *gcry_mpi_get_opaque (gcry_mpi_t a, unsigned int *nbits);




void gcry_mpi_set_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);



void gcry_mpi_clear_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);


int gcry_mpi_get_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);
# 607 "./include/holy/gcrypt/gcrypt.h"
struct gcry_cipher_handle;
typedef struct gcry_cipher_handle *gcry_cipher_hd_t;
# 617 "./include/holy/gcrypt/gcrypt.h"
enum gcry_cipher_algos
  {
    GCRY_CIPHER_NONE = 0,
    GCRY_CIPHER_IDEA = 1,
    GCRY_CIPHER_3DES = 2,
    GCRY_CIPHER_CAST5 = 3,
    GCRY_CIPHER_BLOWFISH = 4,
    GCRY_CIPHER_SAFER_SK128 = 5,
    GCRY_CIPHER_DES_SK = 6,
    GCRY_CIPHER_AES = 7,
    GCRY_CIPHER_AES192 = 8,
    GCRY_CIPHER_AES256 = 9,
    GCRY_CIPHER_TWOFISH = 10,


    GCRY_CIPHER_ARCFOUR = 301,
    GCRY_CIPHER_DES = 302,
    GCRY_CIPHER_TWOFISH128 = 303,
    GCRY_CIPHER_SERPENT128 = 304,
    GCRY_CIPHER_SERPENT192 = 305,
    GCRY_CIPHER_SERPENT256 = 306,
    GCRY_CIPHER_RFC2268_40 = 307,
    GCRY_CIPHER_RFC2268_128 = 308,
    GCRY_CIPHER_SEED = 309,
    GCRY_CIPHER_CAMELLIA128 = 310,
    GCRY_CIPHER_CAMELLIA192 = 311,
    GCRY_CIPHER_CAMELLIA256 = 312
  };
# 655 "./include/holy/gcrypt/gcrypt.h"
enum gcry_cipher_modes
  {
    GCRY_CIPHER_MODE_NONE = 0,
    GCRY_CIPHER_MODE_ECB = 1,
    GCRY_CIPHER_MODE_CFB = 2,
    GCRY_CIPHER_MODE_CBC = 3,
    GCRY_CIPHER_MODE_STREAM = 4,
    GCRY_CIPHER_MODE_OFB = 5,
    GCRY_CIPHER_MODE_CTR = 6,
    GCRY_CIPHER_MODE_AESWRAP= 7
  };


enum gcry_cipher_flags
  {
    GCRY_CIPHER_SECURE = 1,
    GCRY_CIPHER_ENABLE_SYNC = 2,
    GCRY_CIPHER_CBC_CTS = 4,
    GCRY_CIPHER_CBC_MAC = 8
  };




gcry_err_code_t gcry_cipher_open (gcry_cipher_hd_t *handle,
                              int algo, int mode, unsigned int flags);


void gcry_cipher_close (gcry_cipher_hd_t h);


gcry_err_code_t gcry_cipher_ctl (gcry_cipher_hd_t h, int cmd, void *buffer,
                             size_t buflen);


gcry_err_code_t gcry_cipher_info (gcry_cipher_hd_t h, int what, void *buffer,
                              size_t *nbytes);


gcry_err_code_t gcry_cipher_algo_info (int algo, int what, void *buffer,
                                   size_t *nbytes);




const char *gcry_cipher_algo_name (int algorithm) __attribute__ ((__pure__));



int gcry_cipher_map_name (const char *name) __attribute__ ((__pure__));




int gcry_cipher_mode_from_oid (const char *string) __attribute__ ((__pure__));





gcry_err_code_t gcry_cipher_encrypt (gcry_cipher_hd_t h,
                                  void *out, size_t outsize,
                                  const void *in, size_t inlen);


gcry_err_code_t gcry_cipher_decrypt (gcry_cipher_hd_t h,
                                  void *out, size_t outsize,
                                  const void *in, size_t inlen);


gcry_err_code_t gcry_cipher_setkey (gcry_cipher_hd_t hd,
                                 const void *key, size_t keylen);



gcry_err_code_t gcry_cipher_setiv (gcry_cipher_hd_t hd,
                                const void *iv, size_t ivlen);
# 747 "./include/holy/gcrypt/gcrypt.h"
gpg_error_t gcry_cipher_setctr (gcry_cipher_hd_t hd,
                                const void *ctr, size_t ctrlen);


size_t gcry_cipher_get_algo_keylen (int algo);


size_t gcry_cipher_get_algo_blklen (int algo);
# 766 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_cipher_list (int *list, int *list_length);
# 776 "./include/holy/gcrypt/gcrypt.h"
enum gcry_pk_algos
  {
    GCRY_PK_RSA = 1,
    GCRY_PK_RSA_E = 2,
    GCRY_PK_RSA_S = 3,
    GCRY_PK_ELG_E = 16,
    GCRY_PK_DSA = 17,
    GCRY_PK_ELG = 20,
    GCRY_PK_ECDSA = 301,
    GCRY_PK_ECDH = 302
  };
# 797 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_pk_encrypt (gcry_sexp_t *result,
                              gcry_sexp_t data, gcry_sexp_t pkey);



gcry_err_code_t gcry_pk_decrypt (gcry_sexp_t *result,
                              gcry_sexp_t data, gcry_sexp_t skey);



gcry_err_code_t gcry_pk_sign (gcry_sexp_t *result,
                           gcry_sexp_t data, gcry_sexp_t skey);


gcry_err_code_t gcry_pk_verify (gcry_sexp_t sigval,
                             gcry_sexp_t data, gcry_sexp_t pkey);


gcry_err_code_t gcry_pk_testkey (gcry_sexp_t key);




gcry_err_code_t gcry_pk_genkey (gcry_sexp_t *r_key, gcry_sexp_t s_parms);


gcry_err_code_t gcry_pk_ctl (int cmd, void *buffer, size_t buflen);


gcry_err_code_t gcry_pk_algo_info (int algo, int what,
                                void *buffer, size_t *nbytes);




const char *gcry_pk_algo_name (int algorithm) __attribute__ ((__pure__));



int gcry_pk_map_name (const char* name) __attribute__ ((__pure__));



unsigned int gcry_pk_get_nbits (gcry_sexp_t key) __attribute__ ((__pure__));



unsigned char *gcry_pk_get_keygrip (gcry_sexp_t key, unsigned char *array);


const char *gcry_pk_get_curve (gcry_sexp_t key, int iterator,
                               unsigned int *r_nbits);



gcry_sexp_t gcry_pk_get_param (int algo, const char *name);
# 864 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_pk_list (int *list, int *list_length);
# 876 "./include/holy/gcrypt/gcrypt.h"
enum gcry_md_algos
  {
    GCRY_MD_NONE = 0,
    GCRY_MD_MD5 = 1,
    GCRY_MD_SHA1 = 2,
    GCRY_MD_RMD160 = 3,
    GCRY_MD_MD2 = 5,
    GCRY_MD_TIGER = 6,
    GCRY_MD_HAVAL = 7,
    GCRY_MD_SHA256 = 8,
    GCRY_MD_SHA384 = 9,
    GCRY_MD_SHA512 = 10,
    GCRY_MD_SHA224 = 11,
    GCRY_MD_MD4 = 301,
    GCRY_MD_CRC32 = 302,
    GCRY_MD_CRC32_RFC1510 = 303,
    GCRY_MD_CRC24_RFC2440 = 304,
    GCRY_MD_WHIRLPOOL = 305,
    GCRY_MD_TIGER1 = 306,
    GCRY_MD_TIGER2 = 307
  };


enum gcry_md_flags
  {
    GCRY_MD_FLAG_SECURE = 1,
    GCRY_MD_FLAG_HMAC = 2
  };


struct gcry_md_context;




typedef struct gcry_md_handle
{

  struct gcry_md_context *ctx;


  int bufpos;
  int bufsize;
  unsigned char buf[1];
} *gcry_md_hd_t;
# 932 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_md_open (gcry_md_hd_t *h, int algo, unsigned int flags);


void gcry_md_close (gcry_md_hd_t hd);


gcry_err_code_t gcry_md_enable (gcry_md_hd_t hd, int algo);


gcry_err_code_t gcry_md_copy (gcry_md_hd_t *bhd, gcry_md_hd_t ahd);


void gcry_md_reset (gcry_md_hd_t hd);


gcry_err_code_t gcry_md_ctl (gcry_md_hd_t hd, int cmd,
                          void *buffer, size_t buflen);




void gcry_md_write (gcry_md_hd_t hd, const void *buffer, size_t length);



unsigned char *gcry_md_read (gcry_md_hd_t hd, int algo);






void gcry_md_hash_buffer (int algo, void *digest,
                          const void *buffer, size_t length);



int gcry_md_get_algo (gcry_md_hd_t hd);



unsigned int gcry_md_get_algo_dlen (int algo);



int gcry_md_is_enabled (gcry_md_hd_t a, int algo);


int gcry_md_is_secure (gcry_md_hd_t a);


gcry_err_code_t gcry_md_info (gcry_md_hd_t h, int what, void *buffer,
                          size_t *nbytes);


gcry_err_code_t gcry_md_algo_info (int algo, int what, void *buffer,
                               size_t *nbytes);




const char *gcry_md_algo_name (int algo) __attribute__ ((__pure__));



int gcry_md_map_name (const char* name) __attribute__ ((__pure__));



gcry_err_code_t gcry_md_setkey (gcry_md_hd_t hd, const void *key, size_t keylen);




void gcry_md_debug (gcry_md_hd_t hd, const char *suffix);
# 1055 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_md_list (int *list, int *list_length);
# 1476 "./include/holy/gcrypt/gcrypt.h"
enum gcry_kdf_algos
  {
    GCRY_KDF_NONE = 0,
    GCRY_KDF_SIMPLE_S2K = 16,
    GCRY_KDF_SALTED_S2K = 17,
    GCRY_KDF_ITERSALTED_S2K = 19,
    GCRY_KDF_PBKDF1 = 33,
    GCRY_KDF_PBKDF2 = 34
  };


gpg_error_t gcry_kdf_derive (const void *passphrase, size_t passphraselen,
                             int algo, int subalgo,
                             const void *salt, size_t saltlen,
                             unsigned long iterations,
                             size_t keysize, void *keybuffer);
# 1506 "./include/holy/gcrypt/gcrypt.h"
typedef enum gcry_random_level
  {
    GCRY_WEAK_RANDOM = 0,
    GCRY_STRONG_RANDOM = 1,
    GCRY_VERY_STRONG_RANDOM = 2
  }
gcry_random_level_t;



void gcry_randomize (void *buffer, size_t length,
                     enum gcry_random_level level);




gcry_err_code_t gcry_random_add_bytes (const void *buffer, size_t length,
                                    int quality);
# 1533 "./include/holy/gcrypt/gcrypt.h"
void *gcry_random_bytes (size_t nbytes, enum gcry_random_level level)
                         __attribute__ ((__malloc__));




void *gcry_random_bytes_secure (size_t nbytes, enum gcry_random_level level)
                                __attribute__ ((__malloc__));





void gcry_mpi_randomize (gcry_mpi_t w,
                         unsigned int nbits, enum gcry_random_level level);



void gcry_create_nonce (void *buffer, size_t length);
# 1570 "./include/holy/gcrypt/gcrypt.h"
typedef int (*gcry_prime_check_func_t) (void *arg, int mode,
                                        gcry_mpi_t candidate);
# 1588 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_prime_generate (gcry_mpi_t *prime,
                                  unsigned int prime_bits,
                                  unsigned int factor_bits,
                                  gcry_mpi_t **factors,
                                  gcry_prime_check_func_t cb_func,
                                  void *cb_arg,
                                  gcry_random_level_t random_level,
                                  unsigned int flags);





gcry_err_code_t gcry_prime_group_generator (gcry_mpi_t *r_g,
                                         gcry_mpi_t prime,
                                         gcry_mpi_t *factors,
                                         gcry_mpi_t start_g);



void gcry_prime_release_factors (gcry_mpi_t *factors);



gcry_err_code_t gcry_prime_check (gcry_mpi_t x, unsigned int flags);
# 1623 "./include/holy/gcrypt/gcrypt.h"
enum gcry_log_levels
  {
    GCRY_LOG_CONT = 0,
    GCRY_LOG_INFO = 10,
    GCRY_LOG_WARN = 20,
    GCRY_LOG_ERROR = 30,
    GCRY_LOG_FATAL = 40,
    GCRY_LOG_BUG = 50,
    GCRY_LOG_DEBUG = 100
  };


typedef void (*gcry_handler_progress_t) (void *, const char *, int, int, int);


typedef void *(*gcry_handler_alloc_t) (size_t n);


typedef int (*gcry_handler_secure_check_t) (const void *);


typedef void *(*gcry_handler_realloc_t) (void *p, size_t n);


typedef void (*gcry_handler_free_t) (void *);


typedef int (*gcry_handler_no_mem_t) (void *, size_t, unsigned int);


typedef void (*gcry_handler_error_t) (void *, int, const char *);


typedef void (*gcry_handler_log_t) (void *, int, const char *, va_list);



void gcry_set_progress_handler (gcry_handler_progress_t cb, void *cb_data);



void gcry_set_allocation_handler (
                             gcry_handler_alloc_t func_alloc,
                             gcry_handler_alloc_t func_alloc_secure,
                             gcry_handler_secure_check_t func_secure_check,
                             gcry_handler_realloc_t func_realloc,
                             gcry_handler_free_t func_free);



void gcry_set_outofcore_handler (gcry_handler_no_mem_t h, void *opaque);



void gcry_set_fatalerror_handler (gcry_handler_error_t fnc, void *opaque);



void gcry_set_log_handler (gcry_handler_log_t f, void *opaque);


void gcry_set_gettext_handler (const char *(*f)(const char*));



void *gcry_malloc (size_t n) __attribute__ ((__malloc__));
void *gcry_calloc (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_malloc_secure (size_t n) __attribute__ ((__malloc__));
void *gcry_calloc_secure (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_realloc (void *a, size_t n);
char *gcry_strdup (const char *string) __attribute__ ((__malloc__));
void *gcry_xmalloc (size_t n) __attribute__ ((__malloc__));
void *gcry_xcalloc (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_xmalloc_secure (size_t n) __attribute__ ((__malloc__));
void *gcry_xcalloc_secure (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_xrealloc (void *a, size_t n);
char *gcry_xstrdup (const char * a) __attribute__ ((__malloc__));
void gcry_free (void *a);


int gcry_is_secure (const void *a) __attribute__ ((__pure__));






# 1 "./holy-core/lib/libgcrypt-holy/src/gcrypt-module.h" 1
# 1711 "./include/holy/gcrypt/gcrypt.h" 2
# 15 "./include/holy/emu/misc.h" 2



# 1 "./include/holy/util/misc.h" 1
# 19 "./include/holy/emu/misc.h" 2

extern int verbosity;
extern const char *program_name;


extern const unsigned long long int holy_size_t;
void holy_init_all (void);
void holy_fini_all (void);

void holy_find_zpool_from_dir (const char *dir,
          char **poolname, char **poolfs);

char *holy_make_system_path_relative_to_its_root (const char *path)
 __attribute__ ((warn_unused_result));
int
holy_util_device_is_mapped (const char *dev);
# 44 "./include/holy/emu/misc.h"
void * xmalloc (holy_size_t size) __attribute__ ((warn_unused_result));
void * xrealloc (void *ptr, holy_size_t size) __attribute__ ((warn_unused_result));
char * xstrdup (const char *str) __attribute__ ((warn_unused_result));
char * xasprintf (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2))) __attribute__ ((warn_unused_result));

void holy_util_warn (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void holy_util_info (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void holy_util_error (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2), noreturn));

holy_uint64_t holy_util_get_cpu_time_ms (void);


int holy_device_mapper_supported (void);





FILE *
holy_util_fopen (const char *path, const char *mode);


void holy_util_file_sync (FILE *f);
# 19 "./include/holy/util/misc.h" 2

char *holy_util_get_path (const char *dir, const char *file);
size_t holy_util_get_image_size (const char *path);
char *holy_util_read_image (const char *path);
void holy_util_load_image (const char *path, char *buf);
void holy_util_write_image (const char *img, size_t size, FILE *out,
       const char *name);
void holy_util_write_image_at (const void *img, size_t size, off_t offset,
          FILE *out, const char *name);

char *make_system_path_relative_to_its_root (const char *path);

char *holy_canonicalize_file_name (const char *path);

void holy_util_init_nls (void);

void holy_util_host_init (int *argc, char ***argv);

int holy_qsort_strcmp (const void *, const void *);
# 15 "holy-core/kern/emu/hostfs.c" 2
# 1 "./include/holy/emu/hostdisk.h" 1
# 14 "./include/holy/emu/hostdisk.h"
holy_util_fd_t
holy_util_fd_open_device (const holy_disk_t disk, holy_disk_addr_t sector, int flags,
     holy_disk_addr_t *max);

void holy_util_biosdisk_init (const char *dev_map);
void holy_util_biosdisk_fini (void);
char *holy_util_biosdisk_get_holy_dev (const char *os_dev);
const char *holy_util_biosdisk_get_osdev (holy_disk_t disk);
int holy_util_biosdisk_is_present (const char *name);
int holy_util_biosdisk_is_floppy (holy_disk_t disk);
const char *
holy_util_biosdisk_get_compatibility_hint (holy_disk_t disk);
holy_err_t holy_util_biosdisk_flush (struct holy_disk *disk);
holy_err_t
holy_cryptodisk_cheat_mount (const char *sourcedev, const char *cheat);
const char *
holy_util_cryptodisk_get_uuid (holy_disk_t disk);
char *
holy_util_get_ldm (holy_disk_t disk, holy_disk_addr_t start);
int
holy_util_is_ldm (holy_disk_t disk);

holy_err_t
holy_util_ldm_embed (struct holy_disk *disk, unsigned int *nsectors,
       unsigned int max_nsectors,
       holy_embed_type_t embed_type,
       holy_disk_addr_t **sectors);

const char *
holy_hostdisk_os_dev_to_holy_drive (const char *os_dev, int add);


char *
holy_util_get_os_disk (const char *os_dev);

int
holy_util_get_dm_node_linear_info (dev_t dev,
       int *maj, int *min,
       holy_disk_addr_t *st);



holy_int64_t
holy_util_get_fd_size_os (holy_util_fd_t fd, const char *name, unsigned *log_secsize);

holy_disk_addr_t
holy_hostdisk_find_partition_start_os (const char *dev);
void
holy_hostdisk_flush_initial_buffer (const char *os_dev);







struct holy_util_hostdisk_data
{
  char *dev;
  int access_mode;
  holy_util_fd_t fd;
  int is_disk;
  int device_map;
};

void holy_host_init (void);
void holy_host_fini (void);
void holy_hostfs_init (void);
void holy_hostfs_fini (void);
# 16 "holy-core/kern/emu/hostfs.c" 2



# 1 "/usr/include/errno.h" 1 3 4
# 28 "/usr/include/errno.h" 3 4
# 1 "/usr/include/bits/errno.h" 1 3 4
# 26 "/usr/include/bits/errno.h" 3 4
# 1 "/usr/include/linux/errno.h" 1 3 4
# 1 "/usr/include/asm/errno.h" 1 3 4
# 1 "/usr/include/asm-generic/errno.h" 1 3 4




# 1 "/usr/include/asm-generic/errno-base.h" 1 3 4
# 6 "/usr/include/asm-generic/errno.h" 2 3 4
# 2 "/usr/include/asm/errno.h" 2 3 4
# 2 "/usr/include/linux/errno.h" 2 3 4
# 27 "/usr/include/bits/errno.h" 2 3 4
# 29 "/usr/include/errno.h" 2 3 4









# 37 "/usr/include/errno.h" 3 4
extern int *__errno_location (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));







extern char *program_invocation_name;
extern char *program_invocation_short_name;

# 1 "/usr/include/bits/types/error_t.h" 1 3 4
# 22 "/usr/include/bits/types/error_t.h" 3 4
typedef int error_t;
# 49 "/usr/include/errno.h" 2 3 4




# 20 "holy-core/kern/emu/hostfs.c" 2
# 1 "/usr/include/string.h" 1 3 4
# 26 "/usr/include/string.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 27 "/usr/include/string.h" 2 3 4






# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 34 "/usr/include/string.h" 2 3 4
# 43 "/usr/include/string.h" 3 4
extern void *memcpy (void *__restrict __dest, const void *__restrict __src,
       size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern void *memmove (void *__dest, const void *__src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern void *memccpy (void *__restrict __dest, const void *__restrict __src,
        int __c, size_t __n)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__write_only__, 1, 4)));




extern void *memset (void *__s, int __c, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int memcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 80 "/usr/include/string.h" 3 4
extern int __memcmpeq (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 107 "/usr/include/string.h" 3 4
extern void *memchr (const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 120 "/usr/include/string.h" 3 4
extern void *rawmemchr (const void *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 133 "/usr/include/string.h" 3 4
extern void *memrchr (const void *__s, int __c, size_t __n)
      __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)))
      __attribute__ ((__access__ (__read_only__, 1, 3)));





extern char *strcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern char *strcat (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern char *strncat (char *__restrict __dest, const char *__restrict __src,
        size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern int strncmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strcoll (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));

extern size_t strxfrm (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 1, 3)));






extern int strcoll_l (const char *__s1, const char *__s2, locale_t __l)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));


extern size_t strxfrm_l (char *__dest, const char *__src, size_t __n,
    locale_t __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)))
     __attribute__ ((__access__ (__write_only__, 1, 3)));





extern char *strdup (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));






extern char *strndup (const char *__string, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__nonnull__ (1)));
# 246 "/usr/include/string.h" 3 4
extern char *strchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 273 "/usr/include/string.h" 3 4
extern char *strrchr (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 286 "/usr/include/string.h" 3 4
extern char *strchrnul (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));





extern size_t strcspn (const char *__s, const char *__reject)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern size_t strspn (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 323 "/usr/include/string.h" 3 4
extern char *strpbrk (const char *__s, const char *__accept)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 350 "/usr/include/string.h" 3 4
extern char *strstr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));




extern char *strtok (char *__restrict __s, const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern char *__strtok_r (char *__restrict __s,
    const char *__restrict __delim,
    char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));

extern char *strtok_r (char *__restrict __s, const char *__restrict __delim,
         char **__restrict __save_ptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));
# 380 "/usr/include/string.h" 3 4
extern char *strcasestr (const char *__haystack, const char *__needle)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));







extern void *memmem (const void *__haystack, size_t __haystacklen,
       const void *__needle, size_t __needlelen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 3)))
    __attribute__ ((__access__ (__read_only__, 1, 2)))
    __attribute__ ((__access__ (__read_only__, 3, 4)));



extern void *__mempcpy (void *__restrict __dest,
   const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern void *mempcpy (void *__restrict __dest,
        const void *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern size_t strlen (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));




extern size_t strnlen (const char *__string, size_t __maxlen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));




extern char *strerror (int __errnum) __attribute__ ((__nothrow__ , __leaf__));
# 444 "/usr/include/string.h" 3 4
extern char *strerror_r (int __errnum, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 2, 3)));




extern const char *strerrordesc_np (int __err) __attribute__ ((__nothrow__ , __leaf__));

extern const char *strerrorname_np (int __err) __attribute__ ((__nothrow__ , __leaf__));





extern char *strerror_l (int __errnum, locale_t __l) __attribute__ ((__nothrow__ , __leaf__));



# 1 "/usr/include/strings.h" 1 3 4
# 23 "/usr/include/strings.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 24 "/usr/include/strings.h" 2 3 4










extern int bcmp (const void *__s1, const void *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bcopy (const void *__src, void *__dest, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern void bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 68 "/usr/include/strings.h" 3 4
extern char *index (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));
# 96 "/usr/include/strings.h" 3 4
extern char *rindex (const char *__s, int __c)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));






extern int ffs (int __i) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));





extern int ffsl (long int __l) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));
__extension__ extern int ffsll (long long int __ll)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));



extern int strcasecmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern int strncasecmp (const char *__s1, const char *__s2, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int strcasecmp_l (const char *__s1, const char *__s2, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 3)));



extern int strncasecmp_l (const char *__s1, const char *__s2,
     size_t __n, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2, 4)));



# 463 "/usr/include/string.h" 2 3 4



extern void explicit_bzero (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));



extern char *strsep (char **__restrict __stringp,
       const char *__restrict __delim)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern char *strsignal (int __sig) __attribute__ ((__nothrow__ , __leaf__));



extern const char *sigabbrev_np (int __sig) __attribute__ ((__nothrow__ , __leaf__));


extern const char *sigdescr_np (int __sig) __attribute__ ((__nothrow__ , __leaf__));



extern char *__stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpcpy (char *__restrict __dest, const char *__restrict __src)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern char *__stpncpy (char *__restrict __dest,
   const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern char *stpncpy (char *__restrict __dest,
        const char *__restrict __src, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern size_t strlcpy (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__write_only__, 1, 3)));



extern size_t strlcat (char *__restrict __dest,
         const char *__restrict __src, size_t __n)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) __attribute__ ((__access__ (__read_write__, 1, 3)));




extern int strverscmp (const char *__s1, const char *__s2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));


extern char *strfry (char *__string) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void *memfrob (void *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__read_write__, 1, 2)));
# 540 "/usr/include/string.h" 3 4
extern char *basename (const char *__filename) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 552 "/usr/include/string.h" 3 4

# 21 "holy-core/kern/emu/hostfs.c" 2


# 22 "holy-core/kern/emu/hostfs.c"
static int
is_dir (const char *path, const char *name)
{
  int len1 = strlen(path);
  int len2 = strlen(name);
  int ret;

  char *pathname = xmalloc (len1 + 1 + len2 + 1 + 13);
  strcpy (pathname, path);


  if (len1 > 0 && pathname[len1 - 1] != '/')
    strcat (pathname, "/");

  strcat (pathname, name);

  ret = holy_util_is_directory (pathname);
  free (pathname);
  return ret;
}

struct holy_hostfs_data
{
  char *filename;
  holy_util_fd_t f;
};

static holy_err_t
holy_hostfs_dir (holy_device_t device, const char *path,
   holy_fs_dir_hook_t hook, void *hook_data)
{
  holy_util_fd_dir_t dir;


  if (holy_strcmp (device->disk->name, "host"))
    return holy_error (holy_ERR_BAD_FS, "not a hostfs");

  dir = holy_util_fd_opendir (path);
  if (! dir)
    return holy_error (holy_ERR_BAD_FILENAME,
         "can't open `%s': %s", path,
         holy_util_fd_strerror ());

  while (1)
    {
      holy_util_fd_dirent_t de;
      struct holy_dirhook_info info;
      holy_memset (&info, 0, sizeof (info));

      de = holy_util_fd_readdir (dir);
      if (! de)
 break;

      info.dir = !! is_dir (path, de->d_name);
      hook (de->d_name, &info, hook_data);

    }

  holy_util_fd_closedir (dir);

  return holy_ERR_NONE;
}


static holy_err_t
holy_hostfs_open (struct holy_file *file, const char *name)
{
  holy_util_fd_t f;
  struct holy_hostfs_data *data;

  f = holy_util_fd_open (name, holy_UTIL_FD_O_RDONLY);
  if (! ((f) >= 0))
    return holy_error (holy_ERR_BAD_FILENAME,
         "can't open `%s': %s", name,
         strerror (
# 96 "holy-core/kern/emu/hostfs.c" 3 4
                  (*__errno_location ())
# 96 "holy-core/kern/emu/hostfs.c"
                       ));
  data = holy_malloc (sizeof (*data));
  if (!data)
    {
      holy_util_fd_close (f);
      return holy_errno;
    }
  data->filename = holy_strdup (name);
  if (!data->filename)
    {
      holy_free (data);
      holy_util_fd_close (f);
      return holy_errno;
    }

  data->f = f;

  file->data = data;

  file->size = holy_util_get_fd_size (f, name, 
# 115 "holy-core/kern/emu/hostfs.c" 3 4
                                              ((void *)0)
# 115 "holy-core/kern/emu/hostfs.c"
                                                  );

  return holy_ERR_NONE;
}

static holy_ssize_t
holy_hostfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_hostfs_data *data;

  data = file->data;
  if (holy_util_fd_seek (data->f, file->offset) != 0)
    {
      holy_error (holy_ERR_OUT_OF_RANGE, "cannot seek `%s': %s",
    data->filename, holy_util_fd_strerror ());
      return -1;
    }

  unsigned int s = holy_util_fd_read (data->f, buf, len);
  if (s != len)
    holy_error (holy_ERR_FILE_READ_ERROR, "cannot read `%s': %s",
  data->filename, holy_util_fd_strerror ());

  return (signed) s;
}

static holy_err_t
holy_hostfs_close (holy_file_t file)
{
  struct holy_hostfs_data *data;

  data = file->data;
  holy_util_fd_close (data->f);
  holy_free (data->filename);
  holy_free (data);

  return holy_ERR_NONE;
}

static holy_err_t
holy_hostfs_label (holy_device_t device __attribute ((unused)),
     char **label __attribute ((unused)))
{
  *label = 0;
  return holy_ERR_NONE;
}

static struct holy_fs holy_hostfs_fs =
  {
    .name = "hostfs",
    .dir = holy_hostfs_dir,
    .open = holy_hostfs_open,
    .read = holy_hostfs_read,
    .close = holy_hostfs_close,
    .label = holy_hostfs_label,
    .next = 0
  };



@MARKER@hostfs@
{
  holy_fs_register (&holy_hostfs_fs);
}

holy_MOD_FINI(hostfs)
{
  holy_fs_unregister (&holy_hostfs_fs);
}
# 0 "holy-core/disk/host.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "holy-core/disk/host.c"





# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 7 "holy-core/disk/host.c" 2
# 1 "./config-util.h" 1
# 8 "holy-core/disk/host.c" 2

# 1 "./include/holy/dl.h" 1
# 9 "./include/holy/dl.h"
# 1 "./include/holy/symbol.h" 1
# 9 "./include/holy/symbol.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/symbol.h" 2
# 10 "./include/holy/dl.h" 2

# 1 "./include/holy/err.h" 1
# 13 "./include/holy/err.h"
typedef enum
  {
    holy_ERR_NONE = 0,
    holy_ERR_TEST_FAILURE,
    holy_ERR_BAD_MODULE,
    holy_ERR_OUT_OF_MEMORY,
    holy_ERR_BAD_FILE_TYPE,
    holy_ERR_FILE_NOT_FOUND,
    holy_ERR_FILE_READ_ERROR,
    holy_ERR_BAD_FILENAME,
    holy_ERR_UNKNOWN_FS,
    holy_ERR_BAD_FS,
    holy_ERR_BAD_NUMBER,
    holy_ERR_OUT_OF_RANGE,
    holy_ERR_UNKNOWN_DEVICE,
    holy_ERR_BAD_DEVICE,
    holy_ERR_READ_ERROR,
    holy_ERR_WRITE_ERROR,
    holy_ERR_UNKNOWN_COMMAND,
    holy_ERR_INVALID_COMMAND,
    holy_ERR_BAD_ARGUMENT,
    holy_ERR_BAD_PART_TABLE,
    holy_ERR_UNKNOWN_OS,
    holy_ERR_BAD_OS,
    holy_ERR_NO_KERNEL,
    holy_ERR_BAD_FONT,
    holy_ERR_NOT_IMPLEMENTED_YET,
    holy_ERR_SYMLINK_LOOP,
    holy_ERR_BAD_COMPRESSED_DATA,
    holy_ERR_MENU,
    holy_ERR_TIMEOUT,
    holy_ERR_IO,
    holy_ERR_ACCESS_DENIED,
    holy_ERR_EXTRACTOR,
    holy_ERR_NET_BAD_ADDRESS,
    holy_ERR_NET_ROUTE_LOOP,
    holy_ERR_NET_NO_ROUTE,
    holy_ERR_NET_NO_ANSWER,
    holy_ERR_NET_NO_CARD,
    holy_ERR_WAIT,
    holy_ERR_BUG,
    holy_ERR_NET_PORT_CLOSED,
    holy_ERR_NET_INVALID_RESPONSE,
    holy_ERR_NET_UNKNOWN_ERROR,
    holy_ERR_NET_PACKET_TOO_BIG,
    holy_ERR_NET_NO_DOMAIN,
    holy_ERR_EOF,
    holy_ERR_BAD_SIGNATURE
  }
holy_err_t;

struct holy_error_saved
{
  holy_err_t holy_errno;
  char errmsg[256];
};

extern holy_err_t holy_errno;
extern char holy_errmsg[256];

holy_err_t holy_error (holy_err_t n, const char *fmt, ...);
void holy_fatal (const char *fmt, ...) __attribute__ ((noreturn));
void holy_error_push (void);
int holy_error_pop (void);
void holy_print_error (void);
extern int holy_err_printed_errors;
int holy_err_printf (const char *fmt, ...)
     __attribute__ ((format (__printf__, 1, 2)));
# 12 "./include/holy/dl.h" 2
# 1 "./include/holy/types.h" 1
# 9 "./include/holy/types.h"
# 1 "./include/holy/emu/config.h" 1
# 9 "./include/holy/emu/config.h"
# 1 "./include/holy/disk.h" 1
# 9 "./include/holy/disk.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/disk.h" 2



# 1 "./include/holy/types.h" 1
# 14 "./include/holy/disk.h" 2
# 1 "./include/holy/device.h" 1
# 12 "./include/holy/device.h"
struct holy_disk;
struct holy_net;

struct holy_device
{
  struct holy_disk *disk;
  struct holy_net *net;
};
typedef struct holy_device *holy_device_t;

typedef int (*holy_device_iterate_hook_t) (const char *name, void *data);

holy_device_t holy_device_open (const char *name);
holy_err_t holy_device_close (holy_device_t device);
int holy_device_iterate (holy_device_iterate_hook_t hook,
          void *hook_data);
# 15 "./include/holy/disk.h" 2

# 1 "./include/holy/mm.h" 1
# 11 "./include/holy/mm.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 12 "./include/holy/mm.h" 2





typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long long int holy_int64_t;
typedef holy_int64_t holy_ssize;

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


void holy_mm_init_region (void *addr, holy_size_t size);
void *holy_malloc (holy_size_t size);
void *holy_zalloc (holy_size_t size);
void holy_free (void *ptr);
void *holy_realloc (void *ptr, holy_size_t size);

void *holy_memalign (holy_size_t align, holy_size_t size);


void holy_mm_check_real (const char *file, int line);
# 17 "./include/holy/disk.h" 2



enum holy_disk_dev_id
  {
    holy_DISK_DEVICE_BIOSDISK_ID,
    holy_DISK_DEVICE_OFDISK_ID,
    holy_DISK_DEVICE_LOOPBACK_ID,
    holy_DISK_DEVICE_EFIDISK_ID,
    holy_DISK_DEVICE_DISKFILTER_ID,
    holy_DISK_DEVICE_HOST_ID,
    holy_DISK_DEVICE_ATA_ID,
    holy_DISK_DEVICE_MEMDISK_ID,
    holy_DISK_DEVICE_NAND_ID,
    holy_DISK_DEVICE_SCSI_ID,
    holy_DISK_DEVICE_CRYPTODISK_ID,
    holy_DISK_DEVICE_ARCDISK_ID,
    holy_DISK_DEVICE_HOSTDISK_ID,
    holy_DISK_DEVICE_PROCFS_ID,
    holy_DISK_DEVICE_CBFSDISK_ID,
    holy_DISK_DEVICE_UBOOTDISK_ID,
    holy_DISK_DEVICE_XEN,
  };
# 95 "./include/holy/disk.h"
struct holy_disk;

struct holy_disk_memberlist;


typedef enum
  {
    holy_DISK_PULL_NONE,
    holy_DISK_PULL_REMOVABLE,
    holy_DISK_PULL_RESCAN,
    holy_DISK_PULL_MAX
  } holy_disk_pull_t;

typedef int (*holy_disk_dev_iterate_hook_t) (const char *name, void *data);


struct holy_disk_dev
{

  const char *name;


  enum holy_disk_dev_id id;


  int (*iterate) (holy_disk_dev_iterate_hook_t hook, void *hook_data,
    holy_disk_pull_t pull);


  holy_err_t (*open) (const char *name, struct holy_disk *disk);


  void (*close) (struct holy_disk *disk);


  holy_err_t (*read) (struct holy_disk *disk, holy_disk_addr_t sector,
        holy_size_t size, char *buf);


  holy_err_t (*write) (struct holy_disk *disk, holy_disk_addr_t sector,
         holy_size_t size, const char *buf);


  struct holy_disk_memberlist *(*memberlist) (struct holy_disk *disk);
  const char * (*raidname) (struct holy_disk *disk);



  struct holy_disk_dev *next;
};
typedef struct holy_disk_dev *holy_disk_dev_t;

extern holy_disk_dev_t holy_disk_dev_list;

struct holy_partition;

typedef long int holy_disk_addr_t;

typedef void (*holy_disk_read_hook_t) (holy_disk_addr_t sector,
           unsigned offset, unsigned length,
           void *data);


struct holy_disk
{

  const char *name;


  holy_disk_dev_t dev;


  holy_uint64_t total_sectors;


  unsigned int log_sector_size;


  unsigned int max_agglomerate;


  unsigned long id;


  struct holy_partition *partition;



  holy_disk_read_hook_t read_hook;


  void *read_hook_data;


  void *data;
};
typedef struct holy_disk *holy_disk_t;


struct holy_disk_memberlist
{
  holy_disk_t disk;
  struct holy_disk_memberlist *next;
};
typedef struct holy_disk_memberlist *holy_disk_memberlist_t;
# 220 "./include/holy/disk.h"
void holy_disk_cache_invalidate_all (void);

void holy_disk_dev_register (holy_disk_dev_t dev);
void holy_disk_dev_unregister (holy_disk_dev_t dev);
static inline int
holy_disk_dev_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data)
{
  holy_disk_dev_t p;
  holy_disk_pull_t pull;

  for (pull = 0; pull < holy_DISK_PULL_MAX; pull++)
    for (p = holy_disk_dev_list; p; p = p->next)
      if (p->iterate && (p->iterate) (hook, hook_data, pull))
 return 1;

  return 0;
}

holy_disk_t holy_disk_open (const char *name);
void holy_disk_close (holy_disk_t disk);
holy_err_t holy_disk_read (holy_disk_t disk,
     holy_disk_addr_t sector,
     holy_off_t offset,
     holy_size_t size,
     void *buf);
holy_err_t holy_disk_write (holy_disk_t disk,
       holy_disk_addr_t sector,
       holy_off_t offset,
       holy_size_t size,
       const void *buf);
extern holy_err_t (*holy_disk_write_weak) (holy_disk_t disk,
             holy_disk_addr_t sector,
             holy_off_t offset,
             holy_size_t size,
             const void *buf);


holy_uint64_t holy_disk_get_size (holy_disk_t disk);






extern void (* holy_disk_firmware_fini) (void);
extern int holy_disk_firmware_is_tainted;

static inline void
holy_stop_disk_firmware (void)
{

  holy_disk_firmware_is_tainted = 1;
  if (holy_disk_firmware_fini)
    {
      holy_disk_firmware_fini ();
      holy_disk_firmware_fini = ((void *) 0);
    }
}


struct holy_disk_cache
{
  enum holy_disk_dev_id dev_id;
  unsigned long disk_id;
  holy_disk_addr_t sector;
  char *data;
  int lock;
};

extern struct holy_disk_cache holy_disk_cache_table[1021];


void holy_lvm_init (void);
void holy_ldm_init (void);
void holy_mdraid09_init (void);
void holy_mdraid1x_init (void);
void holy_diskfilter_init (void);
void holy_lvm_fini (void);
void holy_ldm_fini (void);
void holy_mdraid09_fini (void);
void holy_mdraid1x_fini (void);
void holy_diskfilter_fini (void);
# 10 "./include/holy/emu/config.h" 2
# 1 "./include/holy/partition.h" 1
# 9 "./include/holy/partition.h"
# 1 "./include/holy/dl.h" 1
# 10 "./include/holy/partition.h" 2
# 1 "./include/holy/list.h" 1
# 11 "./include/holy/list.h"
# 1 "./include/holy/compiler.h" 1
# 12 "./include/holy/list.h" 2

struct holy_list
{
  struct holy_list *next;
  struct holy_list **prev;
};
typedef struct holy_list *holy_list_t;

void holy_list_push (holy_list_t *head, holy_list_t item);
void holy_list_remove (holy_list_t item);




static inline void *
holy_bad_type_cast_real (int line, const char *file)
     __attribute__ ((__error__ ("bad type cast between incompatible holy types")));

static inline void *
holy_bad_type_cast_real (int line, const char *file)
{
  holy_fatal ("error:%s:%u: bad type cast between incompatible holy types",
       file, line);
}
# 50 "./include/holy/list.h"
struct holy_named_list
{
  struct holy_named_list *next;
  struct holy_named_list **prev;
  char *name;
};
typedef struct holy_named_list *holy_named_list_t;

void * holy_named_list_find (holy_named_list_t head,
       const char *name);
# 11 "./include/holy/partition.h" 2


typedef unsigned long long int holy_uint64_t;
# 68 "./include/holy/partition.h"
struct holy_disk;

typedef struct holy_partition *holy_partition_t;


typedef enum
{
  holy_EMBED_PCBIOS
} holy_embed_type_t;


typedef int (*holy_partition_iterate_hook_t) (struct holy_disk *disk,
           const holy_partition_t partition,
           void *data);


struct holy_partition_map
{

  struct holy_partition_map *next;
  struct holy_partition_map **prev;


  const char *name;


  holy_err_t (*iterate) (struct holy_disk *disk,
    holy_partition_iterate_hook_t hook, void *hook_data);


  holy_err_t (*embed) (struct holy_disk *disk, unsigned int *nsectors,
         unsigned int max_nsectors,
         holy_embed_type_t embed_type,
         holy_disk_addr_t **sectors);

};
typedef struct holy_partition_map *holy_partition_map_t;


struct holy_partition
{

  int number;


  unsigned long long int start;


  unsigned long long int len;


  unsigned long long int offset;


  int index;


  struct holy_partition *parent;


  holy_partition_map_t partmap;



  unsigned char msdostype;
};

holy_partition_t holy_partition_probe (struct holy_disk *disk,
          const char *str);
int holy_partition_iterate (struct holy_disk *disk,
      holy_partition_iterate_hook_t hook,
      void *hook_data);
char *holy_partition_get_name (const holy_partition_t partition);


extern holy_partition_map_t holy_partition_map_list;


static inline void
holy_partition_map_register (holy_partition_map_t partmap)
{
  holy_list_push ((((char *) &(*&holy_partition_map_list)->next == (char *) &((holy_list_t) (*&holy_partition_map_list))->next) && ((char *) &(*&holy_partition_map_list)->prev == (char *) &((holy_list_t) (*&holy_partition_map_list))->prev) ? (holy_list_t *) (void *) &holy_partition_map_list : (holy_list_t *) holy_bad_type_cast_real(149, "util/holy-fstest.c")),
    (((char *) &(partmap)->next == (char *) &((holy_list_t) (partmap))->next) && ((char *) &(partmap)->prev == (char *) &((holy_list_t) (partmap))->prev) ? (holy_list_t) partmap : (holy_list_t) holy_bad_type_cast_real(150, "util/holy-fstest.c")));
}


static inline void
holy_partition_map_unregister (holy_partition_map_t partmap)
{
  holy_list_remove ((((char *) &(partmap)->next == (char *) &((holy_list_t) (partmap))->next) && ((char *) &(partmap)->prev == (char *) &((holy_list_t) (partmap))->prev) ? (holy_list_t) partmap : (holy_list_t) holy_bad_type_cast_real(157, "util/holy-fstest.c")));
}




static inline holy_disk_addr_t
holy_partition_get_start (const holy_partition_t p)
{
  holy_partition_t part;
  holy_uint64_t part_start = 0;

  for (part = p; part; part = part->parent)
    part_start += part->start;

  return part_start;
}

static inline holy_uint64_t
holy_partition_get_len (const holy_partition_t p)
{
  return p->len;
}
# 11 "./include/holy/emu/config.h" 2
# 1 "./include/holy/emu/hostfile.h" 1
# 11 "./include/holy/emu/hostfile.h"
# 1 "/usr/include/sys/types.h" 1 3 4
# 25 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 415 "/usr/include/features.h" 3 4
# 1 "/usr/include/features-time64.h" 1 3 4
# 20 "/usr/include/features-time64.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 21 "/usr/include/features-time64.h" 2 3 4
# 1 "/usr/include/bits/timesize.h" 1 3 4
# 19 "/usr/include/bits/timesize.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 20 "/usr/include/bits/timesize.h" 2 3 4
# 22 "/usr/include/features-time64.h" 2 3 4
# 416 "/usr/include/features.h" 2 3 4
# 524 "/usr/include/features.h" 3 4
# 1 "/usr/include/sys/cdefs.h" 1 3 4
# 730 "/usr/include/sys/cdefs.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 731 "/usr/include/sys/cdefs.h" 2 3 4
# 1 "/usr/include/bits/long-double.h" 1 3 4
# 732 "/usr/include/sys/cdefs.h" 2 3 4
# 525 "/usr/include/features.h" 2 3 4
# 548 "/usr/include/features.h" 3 4
# 1 "/usr/include/gnu/stubs.h" 1 3 4
# 10 "/usr/include/gnu/stubs.h" 3 4
# 1 "/usr/include/gnu/stubs-64.h" 1 3 4
# 11 "/usr/include/gnu/stubs.h" 2 3 4
# 549 "/usr/include/features.h" 2 3 4
# 26 "/usr/include/sys/types.h" 2 3 4



# 1 "/usr/include/bits/types.h" 1 3 4
# 27 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 28 "/usr/include/bits/types.h" 2 3 4
# 1 "/usr/include/bits/timesize.h" 1 3 4
# 19 "/usr/include/bits/timesize.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 20 "/usr/include/bits/timesize.h" 2 3 4
# 29 "/usr/include/bits/types.h" 2 3 4



# 31 "/usr/include/bits/types.h" 3 4
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;






typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;



typedef long int __quad_t;
typedef unsigned long int __u_quad_t;







typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
# 141 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/typesizes.h" 1 3 4
# 142 "/usr/include/bits/types.h" 2 3 4
# 1 "/usr/include/bits/time64.h" 1 3 4
# 143 "/usr/include/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;




typedef int __sig_atomic_t;
# 30 "/usr/include/sys/types.h" 2 3 4



typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;


typedef __loff_t loff_t;






typedef __ino64_t ino_t;




typedef __ino64_t ino64_t;




typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;







typedef __off64_t off_t;




typedef __off64_t off64_t;




typedef __pid_t pid_t;





typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;




# 1 "/usr/include/bits/types/clock_t.h" 1 3 4






typedef __clock_t clock_t;
# 127 "/usr/include/sys/types.h" 2 3 4

# 1 "/usr/include/bits/types/clockid_t.h" 1 3 4






typedef __clockid_t clockid_t;
# 129 "/usr/include/sys/types.h" 2 3 4
# 1 "/usr/include/bits/types/time_t.h" 1 3 4
# 10 "/usr/include/bits/types/time_t.h" 3 4
typedef __time_t time_t;
# 130 "/usr/include/sys/types.h" 2 3 4
# 1 "/usr/include/bits/types/timer_t.h" 1 3 4






typedef __timer_t timer_t;
# 131 "/usr/include/sys/types.h" 2 3 4



typedef __useconds_t useconds_t;



typedef __suseconds_t suseconds_t;





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 229 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 145 "/usr/include/sys/types.h" 2 3 4



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;




# 1 "/usr/include/bits/stdint-intn.h" 1 3 4
# 24 "/usr/include/bits/stdint-intn.h" 3 4
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
# 156 "/usr/include/sys/types.h" 2 3 4


typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;


typedef int register_t __attribute__ ((__mode__ (__word__)));
# 176 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 24 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/endian.h" 1 3 4
# 35 "/usr/include/bits/endian.h" 3 4
# 1 "/usr/include/bits/endianness.h" 1 3 4
# 36 "/usr/include/bits/endian.h" 2 3 4
# 25 "/usr/include/endian.h" 2 3 4
# 35 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/byteswap.h" 1 3 4
# 33 "/usr/include/bits/byteswap.h" 3 4
static __inline __uint16_t
__bswap_16 (__uint16_t __bsx)
{

  return __builtin_bswap16 (__bsx);



}






static __inline __uint32_t
__bswap_32 (__uint32_t __bsx)
{

  return __builtin_bswap32 (__bsx);



}
# 69 "/usr/include/bits/byteswap.h" 3 4
__extension__ static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{

  return __builtin_bswap64 (__bsx);



}
# 36 "/usr/include/endian.h" 2 3 4
# 1 "/usr/include/bits/uintn-identity.h" 1 3 4
# 32 "/usr/include/bits/uintn-identity.h" 3 4
static __inline __uint16_t
__uint16_identity (__uint16_t __x)
{
  return __x;
}

static __inline __uint32_t
__uint32_identity (__uint32_t __x)
{
  return __x;
}

static __inline __uint64_t
__uint64_identity (__uint64_t __x)
{
  return __x;
}
# 37 "/usr/include/endian.h" 2 3 4
# 177 "/usr/include/sys/types.h" 2 3 4


# 1 "/usr/include/sys/select.h" 1 3 4
# 30 "/usr/include/sys/select.h" 3 4
# 1 "/usr/include/bits/select.h" 1 3 4
# 31 "/usr/include/sys/select.h" 2 3 4


# 1 "/usr/include/bits/types/sigset_t.h" 1 3 4



# 1 "/usr/include/bits/types/__sigset_t.h" 1 3 4




typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
# 5 "/usr/include/bits/types/sigset_t.h" 2 3 4


typedef __sigset_t sigset_t;
# 34 "/usr/include/sys/select.h" 2 3 4



# 1 "/usr/include/bits/types/struct_timeval.h" 1 3 4







struct timeval
{




  __time_t tv_sec;
  __suseconds_t tv_usec;

};
# 38 "/usr/include/sys/select.h" 2 3 4

# 1 "/usr/include/bits/types/struct_timespec.h" 1 3 4
# 11 "/usr/include/bits/types/struct_timespec.h" 3 4
struct timespec
{



  __time_t tv_sec;




  __syscall_slong_t tv_nsec;
# 31 "/usr/include/bits/types/struct_timespec.h" 3 4
};
# 40 "/usr/include/sys/select.h" 2 3 4
# 49 "/usr/include/sys/select.h" 3 4
typedef long int __fd_mask;
# 59 "/usr/include/sys/select.h" 3 4
typedef struct
  {



    __fd_mask fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];





  } fd_set;






typedef __fd_mask fd_mask;
# 91 "/usr/include/sys/select.h" 3 4

# 102 "/usr/include/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 127 "/usr/include/sys/select.h" 3 4
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);
# 153 "/usr/include/sys/select.h" 3 4

# 180 "/usr/include/sys/types.h" 2 3 4





typedef __blksize_t blksize_t;
# 205 "/usr/include/sys/types.h" 3 4
typedef __blkcnt64_t blkcnt_t;



typedef __fsblkcnt64_t fsblkcnt_t;



typedef __fsfilcnt64_t fsfilcnt_t;





typedef __blkcnt64_t blkcnt64_t;
typedef __fsblkcnt64_t fsblkcnt64_t;
typedef __fsfilcnt64_t fsfilcnt64_t;





# 1 "/usr/include/bits/pthreadtypes.h" 1 3 4
# 23 "/usr/include/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/bits/thread-shared-types.h" 1 3 4
# 44 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/pthreadtypes-arch.h" 1 3 4
# 21 "/usr/include/bits/pthreadtypes-arch.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 22 "/usr/include/bits/pthreadtypes-arch.h" 2 3 4
# 45 "/usr/include/bits/thread-shared-types.h" 2 3 4

# 1 "/usr/include/bits/atomic_wide_counter.h" 1 3 4
# 25 "/usr/include/bits/atomic_wide_counter.h" 3 4
typedef union
{
  __extension__ unsigned long long int __value64;
  struct
  {
    unsigned int __low;
    unsigned int __high;
  } __value32;
} __atomic_wide_counter;
# 47 "/usr/include/bits/thread-shared-types.h" 2 3 4




typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;

typedef struct __pthread_internal_slist
{
  struct __pthread_internal_slist *__next;
} __pthread_slist_t;
# 76 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/struct_mutex.h" 1 3 4
# 22 "/usr/include/bits/struct_mutex.h" 3 4
struct __pthread_mutex_s
{
  int __lock;
  unsigned int __count;
  int __owner;

  unsigned int __nusers;



  int __kind;

  short __spins;
  short __elision;
  __pthread_list_t __list;
# 53 "/usr/include/bits/struct_mutex.h" 3 4
};
# 77 "/usr/include/bits/thread-shared-types.h" 2 3 4
# 89 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/struct_rwlock.h" 1 3 4
# 23 "/usr/include/bits/struct_rwlock.h" 3 4
struct __pthread_rwlock_arch_t
{
  unsigned int __readers;
  unsigned int __writers;
  unsigned int __wrphase_futex;
  unsigned int __writers_futex;
  unsigned int __pad3;
  unsigned int __pad4;

  int __cur_writer;
  int __shared;
  signed char __rwelision;




  unsigned char __pad1[7];


  unsigned long int __pad2;


  unsigned int __flags;
# 55 "/usr/include/bits/struct_rwlock.h" 3 4
};
# 90 "/usr/include/bits/thread-shared-types.h" 2 3 4




struct __pthread_cond_s
{
  __atomic_wide_counter __wseq;
  __atomic_wide_counter __g1_start;
  unsigned int __g_size[2] ;
  unsigned int __g1_orig_size;
  unsigned int __wrefs;
  unsigned int __g_signals[2];
  unsigned int __unused_initialized_1;
  unsigned int __unused_initialized_2;
};

typedef unsigned int __tss_t;
typedef unsigned long int __thrd_t;

typedef struct
{
  int __data ;
} __once_flag;
# 24 "/usr/include/bits/pthreadtypes.h" 2 3 4



typedef unsigned long int pthread_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;



typedef unsigned int pthread_key_t;



typedef int pthread_once_t;


union pthread_attr_t
{
  char __size[56];
  long int __align;
};

typedef union pthread_attr_t pthread_attr_t;




typedef union
{
  struct __pthread_mutex_s __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;


typedef union
{
  struct __pthread_cond_s __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;





typedef union
{
  struct __pthread_rwlock_arch_t __data;
  char __size[56];
  long int __align;
} pthread_rwlock_t;

typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;





typedef volatile int pthread_spinlock_t;




typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;
# 228 "/usr/include/sys/types.h" 2 3 4



# 12 "./include/holy/emu/hostfile.h" 2
# 1 "./include/holy/osdep/hostfile.h" 1
# 11 "./include/holy/osdep/hostfile.h"
# 1 "./include/holy/osdep/hostfile_unix.h" 1
# 9 "./include/holy/osdep/hostfile_unix.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 1 3 4
# 40 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 103 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;
# 11 "./include/holy/osdep/hostfile_unix.h" 2





# 1 "/usr/include/sys/stat.h" 1 3 4
# 99 "/usr/include/sys/stat.h" 3 4


# 1 "/usr/include/bits/stat.h" 1 3 4
# 25 "/usr/include/bits/stat.h" 3 4
# 1 "/usr/include/bits/struct_stat.h" 1 3 4
# 26 "/usr/include/bits/struct_stat.h" 3 4
struct stat
  {



    __dev_t st_dev;




    __ino_t st_ino;







    __nlink_t st_nlink;
    __mode_t st_mode;

    __uid_t st_uid;
    __gid_t st_gid;

    int __pad0;

    __dev_t st_rdev;




    __off_t st_size;



    __blksize_t st_blksize;

    __blkcnt_t st_blocks;
# 74 "/usr/include/bits/struct_stat.h" 3 4
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 89 "/usr/include/bits/struct_stat.h" 3 4
    __syscall_slong_t __glibc_reserved[3];
# 99 "/usr/include/bits/struct_stat.h" 3 4
  };



struct stat64
  {



    __dev_t st_dev;

    __ino64_t st_ino;
    __nlink_t st_nlink;
    __mode_t st_mode;






    __uid_t st_uid;
    __gid_t st_gid;

    int __pad0;
    __dev_t st_rdev;
    __off_t st_size;





    __blksize_t st_blksize;
    __blkcnt64_t st_blocks;







    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 151 "/usr/include/bits/struct_stat.h" 3 4
    __syscall_slong_t __glibc_reserved[3];




  };
# 26 "/usr/include/bits/stat.h" 2 3 4
# 102 "/usr/include/sys/stat.h" 2 3 4
# 227 "/usr/include/sys/stat.h" 3 4
extern int stat (const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "stat64") __attribute__ ((__nothrow__ , __leaf__))

     __attribute__ ((__nonnull__ (1, 2)));
extern int fstat (int __fd, struct stat *__buf) __asm__ ("" "fstat64") __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__nonnull__ (2)));
# 240 "/usr/include/sys/stat.h" 3 4
extern int stat64 (const char *__restrict __file,
     struct stat64 *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int fstat64 (int __fd, struct stat64 *__buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
# 279 "/usr/include/sys/stat.h" 3 4
extern int fstatat (int __fd, const char *__restrict __file, struct stat *__restrict __buf, int __flag) __asm__ ("" "fstatat64") __attribute__ ((__nothrow__ , __leaf__))


                 __attribute__ ((__nonnull__ (3)));
# 291 "/usr/include/sys/stat.h" 3 4
extern int fstatat64 (int __fd, const char *__restrict __file,
        struct stat64 *__restrict __buf, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 327 "/usr/include/sys/stat.h" 3 4
extern int lstat (const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "lstat64") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1, 2)));







extern int lstat64 (const char *__restrict __file,
      struct stat64 *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
# 352 "/usr/include/sys/stat.h" 3 4
extern int chmod (const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int lchmod (const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fchmod (int __fd, __mode_t __mode) __attribute__ ((__nothrow__ , __leaf__));





extern int fchmodat (int __fd, const char *__file, __mode_t __mode,
       int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;






extern __mode_t umask (__mode_t __mask) __attribute__ ((__nothrow__ , __leaf__));




extern __mode_t getumask (void) __attribute__ ((__nothrow__ , __leaf__));



extern int mkdir (const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mkdirat (int __fd, const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));






extern int mknod (const char *__path, __mode_t __mode, __dev_t __dev)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mknodat (int __fd, const char *__path, __mode_t __mode,
      __dev_t __dev) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));





extern int mkfifo (const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mkfifoat (int __fd, const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));






extern int utimensat (int __fd, const char *__path,
        const struct timespec __times[2],
        int __flags)
     __attribute__ ((__nothrow__ , __leaf__));
# 452 "/usr/include/sys/stat.h" 3 4
extern int futimens (int __fd, const struct timespec __times[2]) __attribute__ ((__nothrow__ , __leaf__));
# 465 "/usr/include/sys/stat.h" 3 4
# 1 "/usr/include/bits/statx.h" 1 3 4
# 31 "/usr/include/bits/statx.h" 3 4
# 1 "/usr/include/linux/stat.h" 1 3 4




# 1 "/usr/include/linux/types.h" 1 3 4




# 1 "/usr/include/asm/types.h" 1 3 4
# 1 "/usr/include/asm-generic/types.h" 1 3 4






# 1 "/usr/include/asm-generic/int-ll64.h" 1 3 4
# 12 "/usr/include/asm-generic/int-ll64.h" 3 4
# 1 "/usr/include/asm/bitsperlong.h" 1 3 4
# 11 "/usr/include/asm/bitsperlong.h" 3 4
# 1 "/usr/include/asm-generic/bitsperlong.h" 1 3 4
# 12 "/usr/include/asm/bitsperlong.h" 2 3 4
# 13 "/usr/include/asm-generic/int-ll64.h" 2 3 4







typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;


__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
# 8 "/usr/include/asm-generic/types.h" 2 3 4
# 2 "/usr/include/asm/types.h" 2 3 4
# 6 "/usr/include/linux/types.h" 2 3 4



# 1 "/usr/include/linux/posix_types.h" 1 3 4




# 1 "/usr/include/linux/stddef.h" 1 3 4
# 6 "/usr/include/linux/posix_types.h" 2 3 4
# 25 "/usr/include/linux/posix_types.h" 3 4
typedef struct {
 unsigned long fds_bits[1024 / (8 * sizeof(long))];
} __kernel_fd_set;


typedef void (*__kernel_sighandler_t)(int);


typedef int __kernel_key_t;
typedef int __kernel_mqd_t;

# 1 "/usr/include/asm/posix_types.h" 1 3 4






# 1 "/usr/include/asm/posix_types_64.h" 1 3 4
# 11 "/usr/include/asm/posix_types_64.h" 3 4
typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;


typedef unsigned long __kernel_old_dev_t;


# 1 "/usr/include/asm-generic/posix_types.h" 1 3 4
# 15 "/usr/include/asm-generic/posix_types.h" 3 4
typedef long __kernel_long_t;
typedef unsigned long __kernel_ulong_t;



typedef __kernel_ulong_t __kernel_ino_t;



typedef unsigned int __kernel_mode_t;



typedef int __kernel_pid_t;



typedef int __kernel_ipc_pid_t;



typedef unsigned int __kernel_uid_t;
typedef unsigned int __kernel_gid_t;



typedef __kernel_long_t __kernel_suseconds_t;



typedef int __kernel_daddr_t;



typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;
# 72 "/usr/include/asm-generic/posix_types.h" 3 4
typedef __kernel_ulong_t __kernel_size_t;
typedef __kernel_long_t __kernel_ssize_t;
typedef __kernel_long_t __kernel_ptrdiff_t;




typedef struct {
 int val[2];
} __kernel_fsid_t;





typedef __kernel_long_t __kernel_off_t;
typedef long long __kernel_loff_t;
typedef __kernel_long_t __kernel_old_time_t;
typedef __kernel_long_t __kernel_time_t;
typedef long long __kernel_time64_t;
typedef __kernel_long_t __kernel_clock_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
# 19 "/usr/include/asm/posix_types_64.h" 2 3 4
# 8 "/usr/include/asm/posix_types.h" 2 3 4
# 37 "/usr/include/linux/posix_types.h" 2 3 4
# 10 "/usr/include/linux/types.h" 2 3 4


typedef __signed__ __int128 __s128 __attribute__((aligned(16)));
typedef unsigned __int128 __u128 __attribute__((aligned(16)));
# 31 "/usr/include/linux/types.h" 3 4
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef __u16 __sum16;
typedef __u32 __wsum;
# 55 "/usr/include/linux/types.h" 3 4
typedef unsigned __poll_t;
# 6 "/usr/include/linux/stat.h" 2 3 4
# 56 "/usr/include/linux/stat.h" 3 4
struct statx_timestamp {
 __s64 tv_sec;
 __u32 tv_nsec;
 __s32 __reserved;
};
# 99 "/usr/include/linux/stat.h" 3 4
struct statx {


 __u32 stx_mask;


 __u32 stx_blksize;


 __u64 stx_attributes;



 __u32 stx_nlink;


 __u32 stx_uid;


 __u32 stx_gid;


 __u16 stx_mode;
 __u16 __spare0[1];



 __u64 stx_ino;


 __u64 stx_size;


 __u64 stx_blocks;


 __u64 stx_attributes_mask;



 struct statx_timestamp stx_atime;


 struct statx_timestamp stx_btime;


 struct statx_timestamp stx_ctime;


 struct statx_timestamp stx_mtime;



 __u32 stx_rdev_major;
 __u32 stx_rdev_minor;


 __u32 stx_dev_major;
 __u32 stx_dev_minor;


 __u64 stx_mnt_id;


 __u32 stx_dio_mem_align;


 __u32 stx_dio_offset_align;



 __u64 stx_subvol;


 __u32 stx_atomic_write_unit_min;


 __u32 stx_atomic_write_unit_max;



 __u32 stx_atomic_write_segments_max;


 __u32 stx_dio_read_offset_align;


 __u32 stx_atomic_write_unit_max_opt;
 __u32 __spare2[1];


 __u64 __spare3[8];


};
# 32 "/usr/include/bits/statx.h" 2 3 4







# 1 "/usr/include/bits/statx-generic.h" 1 3 4
# 25 "/usr/include/bits/statx-generic.h" 3 4
# 1 "/usr/include/bits/types/struct_statx_timestamp.h" 1 3 4
# 26 "/usr/include/bits/statx-generic.h" 2 3 4
# 1 "/usr/include/bits/types/struct_statx.h" 1 3 4
# 27 "/usr/include/bits/statx-generic.h" 2 3 4
# 62 "/usr/include/bits/statx-generic.h" 3 4



int statx (int __dirfd, const char *__restrict __path, int __flags,
           unsigned int __mask, struct statx *__restrict __buf)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (5)));


# 40 "/usr/include/bits/statx.h" 2 3 4
# 466 "/usr/include/sys/stat.h" 2 3 4



# 17 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/fcntl.h" 1 3 4
# 28 "/usr/include/fcntl.h" 3 4







# 1 "/usr/include/bits/fcntl.h" 1 3 4
# 35 "/usr/include/bits/fcntl.h" 3 4
struct flock
  {
    short int l_type;
    short int l_whence;




    __off64_t l_start;
    __off64_t l_len;

    __pid_t l_pid;
  };


struct flock64
  {
    short int l_type;
    short int l_whence;
    __off64_t l_start;
    __off64_t l_len;
    __pid_t l_pid;
  };



# 1 "/usr/include/bits/fcntl-linux.h" 1 3 4
# 38 "/usr/include/bits/fcntl-linux.h" 3 4
# 1 "/usr/include/bits/types/struct_iovec.h" 1 3 4
# 23 "/usr/include/bits/types/struct_iovec.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 24 "/usr/include/bits/types/struct_iovec.h" 2 3 4


struct iovec
  {
    void *iov_base;
    size_t iov_len;
  };
# 39 "/usr/include/bits/fcntl-linux.h" 2 3 4
# 267 "/usr/include/bits/fcntl-linux.h" 3 4
enum __pid_type
  {
    F_OWNER_TID = 0,
    F_OWNER_PID,
    F_OWNER_PGRP,
    F_OWNER_GID = F_OWNER_PGRP
  };


struct f_owner_ex
  {
    enum __pid_type type;
    __pid_t pid;
  };
# 357 "/usr/include/bits/fcntl-linux.h" 3 4
# 1 "/usr/include/linux/falloc.h" 1 3 4
# 358 "/usr/include/bits/fcntl-linux.h" 2 3 4



struct file_handle
{
  unsigned int handle_bytes;
  int handle_type;

  unsigned char f_handle[0];
};
# 386 "/usr/include/bits/fcntl-linux.h" 3 4





extern __ssize_t readahead (int __fd, __off64_t __offset, size_t __count)
    __attribute__ ((__nothrow__ , __leaf__));






extern int sync_file_range (int __fd, __off64_t __offset, __off64_t __count,
       unsigned int __flags);






extern __ssize_t vmsplice (int __fdout, const struct iovec *__iov,
      size_t __count, unsigned int __flags);





extern __ssize_t splice (int __fdin, __off64_t *__offin, int __fdout,
    __off64_t *__offout, size_t __len,
    unsigned int __flags);





extern __ssize_t tee (int __fdin, int __fdout, size_t __len,
        unsigned int __flags);
# 433 "/usr/include/bits/fcntl-linux.h" 3 4
extern int fallocate (int __fd, int __mode, __off64_t __offset, __off64_t __len) __asm__ ("" "fallocate64")

                     ;





extern int fallocate64 (int __fd, int __mode, __off64_t __offset,
   __off64_t __len);




extern int name_to_handle_at (int __dfd, const char *__name,
         struct file_handle *__handle, int *__mnt_id,
         int __flags) __attribute__ ((__nothrow__ , __leaf__));





extern int open_by_handle_at (int __mountdirfd, struct file_handle *__handle,
         int __flags);




# 62 "/usr/include/bits/fcntl.h" 2 3 4
# 36 "/usr/include/fcntl.h" 2 3 4
# 78 "/usr/include/fcntl.h" 3 4
# 1 "/usr/include/bits/stat.h" 1 3 4
# 79 "/usr/include/fcntl.h" 2 3 4
# 180 "/usr/include/fcntl.h" 3 4
extern int fcntl (int __fd, int __cmd, ...) __asm__ ("" "fcntl64");





extern int fcntl64 (int __fd, int __cmd, ...);
# 212 "/usr/include/fcntl.h" 3 4
extern int open (const char *__file, int __oflag, ...) __asm__ ("" "open64")
     __attribute__ ((__nonnull__ (1)));





extern int open64 (const char *__file, int __oflag, ...) __attribute__ ((__nonnull__ (1)));
# 237 "/usr/include/fcntl.h" 3 4
extern int openat (int __fd, const char *__file, int __oflag, ...) __asm__ ("" "openat64")
                    __attribute__ ((__nonnull__ (2)));





extern int openat64 (int __fd, const char *__file, int __oflag, ...)
     __attribute__ ((__nonnull__ (2)));
# 258 "/usr/include/fcntl.h" 3 4
extern int creat (const char *__file, mode_t __mode) __asm__ ("" "creat64")
                  __attribute__ ((__nonnull__ (1)));





extern int creat64 (const char *__file, mode_t __mode) __attribute__ ((__nonnull__ (1)));
# 287 "/usr/include/fcntl.h" 3 4
extern int lockf (int __fd, int __cmd, __off64_t __len) __asm__ ("" "lockf64")
                                     ;





extern int lockf64 (int __fd, int __cmd, off64_t __len) ;
# 306 "/usr/include/fcntl.h" 3 4
extern int posix_fadvise (int __fd, __off64_t __offset, __off64_t __len, int __advise) __asm__ ("" "posix_fadvise64") __attribute__ ((__nothrow__ , __leaf__))

                      ;





extern int posix_fadvise64 (int __fd, off64_t __offset, off64_t __len,
       int __advise) __attribute__ ((__nothrow__ , __leaf__));
# 327 "/usr/include/fcntl.h" 3 4
extern int posix_fallocate (int __fd, __off64_t __offset, __off64_t __len) __asm__ ("" "posix_fallocate64")

                           ;





extern int posix_fallocate64 (int __fd, off64_t __offset, off64_t __len);
# 345 "/usr/include/fcntl.h" 3 4

# 18 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/dirent.h" 1 3 4
# 27 "/usr/include/dirent.h" 3 4

# 61 "/usr/include/dirent.h" 3 4
# 1 "/usr/include/bits/dirent.h" 1 3 4
# 22 "/usr/include/bits/dirent.h" 3 4
struct dirent
  {




    __ino64_t d_ino;
    __off64_t d_off;

    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
  };


struct dirent64
  {
    __ino64_t d_ino;
    __off64_t d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
  };
# 62 "/usr/include/dirent.h" 2 3 4
# 97 "/usr/include/dirent.h" 3 4
enum
  {
    DT_UNKNOWN = 0,

    DT_FIFO = 1,

    DT_CHR = 2,

    DT_DIR = 4,

    DT_BLK = 6,

    DT_REG = 8,

    DT_LNK = 10,

    DT_SOCK = 12,

    DT_WHT = 14

  };
# 127 "/usr/include/dirent.h" 3 4
typedef struct __dirstream DIR;






extern int closedir (DIR *__dirp) __attribute__ ((__nonnull__ (1)));






extern DIR *opendir (const char *__name) __attribute__ ((__nonnull__ (1)))
 __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (closedir, 1)));






extern DIR *fdopendir (int __fd)
 __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (closedir, 1)));
# 167 "/usr/include/dirent.h" 3 4
extern struct dirent *readdir (DIR *__dirp) __asm__ ("" "readdir64")
     __attribute__ ((__nonnull__ (1)));






extern struct dirent64 *readdir64 (DIR *__dirp) __attribute__ ((__nonnull__ (1)));
# 191 "/usr/include/dirent.h" 3 4
extern int readdir_r (DIR *__restrict __dirp, struct dirent *__restrict __entry, struct dirent **__restrict __result) __asm__ ("" "readdir64_r")




  __attribute__ ((__nonnull__ (1, 2, 3))) __attribute__ ((__deprecated__));






extern int readdir64_r (DIR *__restrict __dirp,
   struct dirent64 *__restrict __entry,
   struct dirent64 **__restrict __result)
  __attribute__ ((__nonnull__ (1, 2, 3))) __attribute__ ((__deprecated__));




extern void rewinddir (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern void seekdir (DIR *__dirp, long int __pos) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int telldir (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int dirfd (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 235 "/usr/include/dirent.h" 3 4
# 1 "/usr/include/bits/posix1_lim.h" 1 3 4
# 27 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 28 "/usr/include/bits/posix1_lim.h" 2 3 4
# 161 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/local_lim.h" 1 3 4
# 38 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/linux/limits.h" 1 3 4
# 39 "/usr/include/bits/local_lim.h" 2 3 4
# 81 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/bits/pthread_stack_min-dynamic.h" 1 3 4
# 23 "/usr/include/bits/pthread_stack_min-dynamic.h" 3 4

extern long int __sysconf (int __name) __attribute__ ((__nothrow__ , __leaf__));

# 82 "/usr/include/bits/local_lim.h" 2 3 4
# 162 "/usr/include/bits/posix1_lim.h" 2 3 4
# 236 "/usr/include/dirent.h" 2 3 4
# 247 "/usr/include/dirent.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 248 "/usr/include/dirent.h" 2 3 4
# 265 "/usr/include/dirent.h" 3 4
extern int scandir (const char *__restrict __dir, struct dirent ***__restrict __namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **)) __asm__ ("" "scandir64")





                    __attribute__ ((__nonnull__ (1, 2)));
# 280 "/usr/include/dirent.h" 3 4
extern int scandir64 (const char *__restrict __dir,
        struct dirent64 ***__restrict __namelist,
        int (*__selector) (const struct dirent64 *),
        int (*__cmp) (const struct dirent64 **,
        const struct dirent64 **))
     __attribute__ ((__nonnull__ (1, 2)));
# 303 "/usr/include/dirent.h" 3 4
extern int scandirat (int __dfd, const char *__restrict __dir, struct dirent ***__restrict __namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **)) __asm__ ("" "scandirat64")





                      __attribute__ ((__nonnull__ (2, 3)));







extern int scandirat64 (int __dfd, const char *__restrict __dir,
   struct dirent64 ***__restrict __namelist,
   int (*__selector) (const struct dirent64 *),
   int (*__cmp) (const struct dirent64 **,
          const struct dirent64 **))
     __attribute__ ((__nonnull__ (2, 3)));
# 332 "/usr/include/dirent.h" 3 4
extern int alphasort (const struct dirent **__e1, const struct dirent **__e2) __asm__ ("" "alphasort64") __attribute__ ((__nothrow__ , __leaf__))


                   __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int alphasort64 (const struct dirent64 **__e1,
   const struct dirent64 **__e2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 361 "/usr/include/dirent.h" 3 4
extern __ssize_t getdirentries (int __fd, char *__restrict __buf, size_t __nbytes, __off64_t *__restrict __basep) __asm__ ("" "getdirentries64") __attribute__ ((__nothrow__ , __leaf__))



                      __attribute__ ((__nonnull__ (2, 4)));






extern __ssize_t getdirentries64 (int __fd, char *__restrict __buf,
      size_t __nbytes,
      __off64_t *__restrict __basep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));
# 387 "/usr/include/dirent.h" 3 4
extern int versionsort (const struct dirent **__e1, const struct dirent **__e2) __asm__ ("" "versionsort64") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int versionsort64 (const struct dirent64 **__e1,
     const struct dirent64 **__e2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));





# 1 "/usr/include/bits/dirent_ext.h" 1 3 4
# 23 "/usr/include/bits/dirent_ext.h" 3 4






extern __ssize_t getdents64 (int __fd, void *__buffer, size_t __length)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



# 407 "/usr/include/dirent.h" 2 3 4
# 19 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/unistd.h" 1 3 4
# 27 "/usr/include/unistd.h" 3 4

# 202 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/posix_opt.h" 1 3 4
# 203 "/usr/include/unistd.h" 2 3 4



# 1 "/usr/include/bits/environments.h" 1 3 4
# 22 "/usr/include/bits/environments.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 23 "/usr/include/bits/environments.h" 2 3 4
# 207 "/usr/include/unistd.h" 2 3 4
# 226 "/usr/include/unistd.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 227 "/usr/include/unistd.h" 2 3 4
# 267 "/usr/include/unistd.h" 3 4
typedef __intptr_t intptr_t;






typedef __socklen_t socklen_t;
# 287 "/usr/include/unistd.h" 3 4
extern int access (const char *__name, int __type) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int euidaccess (const char *__name, int __type)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int eaccess (const char *__name, int __type)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int execveat (int __fd, const char *__path, char *const __argv[],
                     char *const __envp[], int __flags)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));






extern int faccessat (int __fd, const char *__file, int __type, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;
# 342 "/usr/include/unistd.h" 3 4
extern __off64_t lseek (int __fd, __off64_t __offset, int __whence) __asm__ ("" "lseek64") __attribute__ ((__nothrow__ , __leaf__))

             ;





extern __off64_t lseek64 (int __fd, __off64_t __offset, int __whence)
     __attribute__ ((__nothrow__ , __leaf__));






extern int close (int __fd);




extern void closefrom (int __lowfd) __attribute__ ((__nothrow__ , __leaf__));







extern ssize_t read (int __fd, void *__buf, size_t __nbytes)
    __attribute__ ((__access__ (__write_only__, 2, 3)));





extern ssize_t write (int __fd, const void *__buf, size_t __n)
    __attribute__ ((__access__ (__read_only__, 2, 3)));
# 404 "/usr/include/unistd.h" 3 4
extern ssize_t pread (int __fd, void *__buf, size_t __nbytes, __off64_t __offset) __asm__ ("" "pread64")


    __attribute__ ((__access__ (__write_only__, 2, 3)));
extern ssize_t pwrite (int __fd, const void *__buf, size_t __nbytes, __off64_t __offset) __asm__ ("" "pwrite64")


    __attribute__ ((__access__ (__read_only__, 2, 3)));
# 422 "/usr/include/unistd.h" 3 4
extern ssize_t pread64 (int __fd, void *__buf, size_t __nbytes,
   __off64_t __offset)
    __attribute__ ((__access__ (__write_only__, 2, 3)));


extern ssize_t pwrite64 (int __fd, const void *__buf, size_t __n,
    __off64_t __offset)
    __attribute__ ((__access__ (__read_only__, 2, 3)));







extern int pipe (int __pipedes[2]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int pipe2 (int __pipedes[2], int __flags) __attribute__ ((__nothrow__ , __leaf__)) ;
# 452 "/usr/include/unistd.h" 3 4
extern unsigned int alarm (unsigned int __seconds) __attribute__ ((__nothrow__ , __leaf__));
# 464 "/usr/include/unistd.h" 3 4
extern unsigned int sleep (unsigned int __seconds);







extern __useconds_t ualarm (__useconds_t __value, __useconds_t __interval)
     __attribute__ ((__nothrow__ , __leaf__));






extern int usleep (__useconds_t __useconds);
# 489 "/usr/include/unistd.h" 3 4
extern int pause (void);



extern int chown (const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchown (int __fd, __uid_t __owner, __gid_t __group) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int lchown (const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;






extern int fchownat (int __fd, const char *__file, __uid_t __owner,
       __gid_t __group, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;



extern int chdir (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchdir (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;
# 531 "/usr/include/unistd.h" 3 4
extern char *getcwd (char *__buf, size_t __size) __attribute__ ((__nothrow__ , __leaf__)) ;





extern char *get_current_dir_name (void) __attribute__ ((__nothrow__ , __leaf__));







extern char *getwd (char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__deprecated__))
    __attribute__ ((__access__ (__write_only__, 1)));




extern int dup (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;


extern int dup2 (int __fd, int __fd2) __attribute__ ((__nothrow__ , __leaf__));




extern int dup3 (int __fd, int __fd2, int __flags) __attribute__ ((__nothrow__ , __leaf__));



extern char **__environ;

extern char **environ;





extern int execve (const char *__path, char *const __argv[],
     char *const __envp[]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int fexecve (int __fd, char *const __argv[], char *const __envp[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));




extern int execv (const char *__path, char *const __argv[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execle (const char *__path, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execl (const char *__path, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execvp (const char *__file, char *const __argv[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int execlp (const char *__file, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int execvpe (const char *__file, char *const __argv[],
      char *const __envp[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern int nice (int __inc) __attribute__ ((__nothrow__ , __leaf__)) ;




extern void _exit (int __status) __attribute__ ((__noreturn__));





# 1 "/usr/include/bits/confname.h" 1 3 4
# 24 "/usr/include/bits/confname.h" 3 4
enum
  {
    _PC_LINK_MAX,

    _PC_MAX_CANON,

    _PC_MAX_INPUT,

    _PC_NAME_MAX,

    _PC_PATH_MAX,

    _PC_PIPE_BUF,

    _PC_CHOWN_RESTRICTED,

    _PC_NO_TRUNC,

    _PC_VDISABLE,

    _PC_SYNC_IO,

    _PC_ASYNC_IO,

    _PC_PRIO_IO,

    _PC_SOCK_MAXBUF,

    _PC_FILESIZEBITS,

    _PC_REC_INCR_XFER_SIZE,

    _PC_REC_MAX_XFER_SIZE,

    _PC_REC_MIN_XFER_SIZE,

    _PC_REC_XFER_ALIGN,

    _PC_ALLOC_SIZE_MIN,

    _PC_SYMLINK_MAX,

    _PC_2_SYMLINKS

  };


enum
  {
    _SC_ARG_MAX,

    _SC_CHILD_MAX,

    _SC_CLK_TCK,

    _SC_NGROUPS_MAX,

    _SC_OPEN_MAX,

    _SC_STREAM_MAX,

    _SC_TZNAME_MAX,

    _SC_JOB_CONTROL,

    _SC_SAVED_IDS,

    _SC_REALTIME_SIGNALS,

    _SC_PRIORITY_SCHEDULING,

    _SC_TIMERS,

    _SC_ASYNCHRONOUS_IO,

    _SC_PRIORITIZED_IO,

    _SC_SYNCHRONIZED_IO,

    _SC_FSYNC,

    _SC_MAPPED_FILES,

    _SC_MEMLOCK,

    _SC_MEMLOCK_RANGE,

    _SC_MEMORY_PROTECTION,

    _SC_MESSAGE_PASSING,

    _SC_SEMAPHORES,

    _SC_SHARED_MEMORY_OBJECTS,

    _SC_AIO_LISTIO_MAX,

    _SC_AIO_MAX,

    _SC_AIO_PRIO_DELTA_MAX,

    _SC_DELAYTIMER_MAX,

    _SC_MQ_OPEN_MAX,

    _SC_MQ_PRIO_MAX,

    _SC_VERSION,

    _SC_PAGESIZE,


    _SC_RTSIG_MAX,

    _SC_SEM_NSEMS_MAX,

    _SC_SEM_VALUE_MAX,

    _SC_SIGQUEUE_MAX,

    _SC_TIMER_MAX,




    _SC_BC_BASE_MAX,

    _SC_BC_DIM_MAX,

    _SC_BC_SCALE_MAX,

    _SC_BC_STRING_MAX,

    _SC_COLL_WEIGHTS_MAX,

    _SC_EQUIV_CLASS_MAX,

    _SC_EXPR_NEST_MAX,

    _SC_LINE_MAX,

    _SC_RE_DUP_MAX,

    _SC_CHARCLASS_NAME_MAX,


    _SC_2_VERSION,

    _SC_2_C_BIND,

    _SC_2_C_DEV,

    _SC_2_FORT_DEV,

    _SC_2_FORT_RUN,

    _SC_2_SW_DEV,

    _SC_2_LOCALEDEF,


    _SC_PII,

    _SC_PII_XTI,

    _SC_PII_SOCKET,

    _SC_PII_INTERNET,

    _SC_PII_OSI,

    _SC_POLL,

    _SC_SELECT,

    _SC_UIO_MAXIOV,

    _SC_IOV_MAX = _SC_UIO_MAXIOV,

    _SC_PII_INTERNET_STREAM,

    _SC_PII_INTERNET_DGRAM,

    _SC_PII_OSI_COTS,

    _SC_PII_OSI_CLTS,

    _SC_PII_OSI_M,

    _SC_T_IOV_MAX,



    _SC_THREADS,

    _SC_THREAD_SAFE_FUNCTIONS,

    _SC_GETGR_R_SIZE_MAX,

    _SC_GETPW_R_SIZE_MAX,

    _SC_LOGIN_NAME_MAX,

    _SC_TTY_NAME_MAX,

    _SC_THREAD_DESTRUCTOR_ITERATIONS,

    _SC_THREAD_KEYS_MAX,

    _SC_THREAD_STACK_MIN,

    _SC_THREAD_THREADS_MAX,

    _SC_THREAD_ATTR_STACKADDR,

    _SC_THREAD_ATTR_STACKSIZE,

    _SC_THREAD_PRIORITY_SCHEDULING,

    _SC_THREAD_PRIO_INHERIT,

    _SC_THREAD_PRIO_PROTECT,

    _SC_THREAD_PROCESS_SHARED,


    _SC_NPROCESSORS_CONF,

    _SC_NPROCESSORS_ONLN,

    _SC_PHYS_PAGES,

    _SC_AVPHYS_PAGES,

    _SC_ATEXIT_MAX,

    _SC_PASS_MAX,


    _SC_XOPEN_VERSION,

    _SC_XOPEN_XCU_VERSION,

    _SC_XOPEN_UNIX,

    _SC_XOPEN_CRYPT,

    _SC_XOPEN_ENH_I18N,

    _SC_XOPEN_SHM,


    _SC_2_CHAR_TERM,

    _SC_2_C_VERSION,

    _SC_2_UPE,


    _SC_XOPEN_XPG2,

    _SC_XOPEN_XPG3,

    _SC_XOPEN_XPG4,


    _SC_CHAR_BIT,

    _SC_CHAR_MAX,

    _SC_CHAR_MIN,

    _SC_INT_MAX,

    _SC_INT_MIN,

    _SC_LONG_BIT,

    _SC_WORD_BIT,

    _SC_MB_LEN_MAX,

    _SC_NZERO,

    _SC_SSIZE_MAX,

    _SC_SCHAR_MAX,

    _SC_SCHAR_MIN,

    _SC_SHRT_MAX,

    _SC_SHRT_MIN,

    _SC_UCHAR_MAX,

    _SC_UINT_MAX,

    _SC_ULONG_MAX,

    _SC_USHRT_MAX,


    _SC_NL_ARGMAX,

    _SC_NL_LANGMAX,

    _SC_NL_MSGMAX,

    _SC_NL_NMAX,

    _SC_NL_SETMAX,

    _SC_NL_TEXTMAX,


    _SC_XBS5_ILP32_OFF32,

    _SC_XBS5_ILP32_OFFBIG,

    _SC_XBS5_LP64_OFF64,

    _SC_XBS5_LPBIG_OFFBIG,


    _SC_XOPEN_LEGACY,

    _SC_XOPEN_REALTIME,

    _SC_XOPEN_REALTIME_THREADS,


    _SC_ADVISORY_INFO,

    _SC_BARRIERS,

    _SC_BASE,

    _SC_C_LANG_SUPPORT,

    _SC_C_LANG_SUPPORT_R,

    _SC_CLOCK_SELECTION,

    _SC_CPUTIME,

    _SC_THREAD_CPUTIME,

    _SC_DEVICE_IO,

    _SC_DEVICE_SPECIFIC,

    _SC_DEVICE_SPECIFIC_R,

    _SC_FD_MGMT,

    _SC_FIFO,

    _SC_PIPE,

    _SC_FILE_ATTRIBUTES,

    _SC_FILE_LOCKING,

    _SC_FILE_SYSTEM,

    _SC_MONOTONIC_CLOCK,

    _SC_MULTI_PROCESS,

    _SC_SINGLE_PROCESS,

    _SC_NETWORKING,

    _SC_READER_WRITER_LOCKS,

    _SC_SPIN_LOCKS,

    _SC_REGEXP,

    _SC_REGEX_VERSION,

    _SC_SHELL,

    _SC_SIGNALS,

    _SC_SPAWN,

    _SC_SPORADIC_SERVER,

    _SC_THREAD_SPORADIC_SERVER,

    _SC_SYSTEM_DATABASE,

    _SC_SYSTEM_DATABASE_R,

    _SC_TIMEOUTS,

    _SC_TYPED_MEMORY_OBJECTS,

    _SC_USER_GROUPS,

    _SC_USER_GROUPS_R,

    _SC_2_PBS,

    _SC_2_PBS_ACCOUNTING,

    _SC_2_PBS_LOCATE,

    _SC_2_PBS_MESSAGE,

    _SC_2_PBS_TRACK,

    _SC_SYMLOOP_MAX,

    _SC_STREAMS,

    _SC_2_PBS_CHECKPOINT,


    _SC_V6_ILP32_OFF32,

    _SC_V6_ILP32_OFFBIG,

    _SC_V6_LP64_OFF64,

    _SC_V6_LPBIG_OFFBIG,


    _SC_HOST_NAME_MAX,

    _SC_TRACE,

    _SC_TRACE_EVENT_FILTER,

    _SC_TRACE_INHERIT,

    _SC_TRACE_LOG,


    _SC_LEVEL1_ICACHE_SIZE,

    _SC_LEVEL1_ICACHE_ASSOC,

    _SC_LEVEL1_ICACHE_LINESIZE,

    _SC_LEVEL1_DCACHE_SIZE,

    _SC_LEVEL1_DCACHE_ASSOC,

    _SC_LEVEL1_DCACHE_LINESIZE,

    _SC_LEVEL2_CACHE_SIZE,

    _SC_LEVEL2_CACHE_ASSOC,

    _SC_LEVEL2_CACHE_LINESIZE,

    _SC_LEVEL3_CACHE_SIZE,

    _SC_LEVEL3_CACHE_ASSOC,

    _SC_LEVEL3_CACHE_LINESIZE,

    _SC_LEVEL4_CACHE_SIZE,

    _SC_LEVEL4_CACHE_ASSOC,

    _SC_LEVEL4_CACHE_LINESIZE,



    _SC_IPV6 = _SC_LEVEL1_ICACHE_SIZE + 50,

    _SC_RAW_SOCKETS,


    _SC_V7_ILP32_OFF32,

    _SC_V7_ILP32_OFFBIG,

    _SC_V7_LP64_OFF64,

    _SC_V7_LPBIG_OFFBIG,


    _SC_SS_REPL_MAX,


    _SC_TRACE_EVENT_NAME_MAX,

    _SC_TRACE_NAME_MAX,

    _SC_TRACE_SYS_MAX,

    _SC_TRACE_USER_EVENT_MAX,


    _SC_XOPEN_STREAMS,


    _SC_THREAD_ROBUST_PRIO_INHERIT,

    _SC_THREAD_ROBUST_PRIO_PROTECT,


    _SC_MINSIGSTKSZ,


    _SC_SIGSTKSZ

  };


enum
  {
    _CS_PATH,


    _CS_V6_WIDTH_RESTRICTED_ENVS,



    _CS_GNU_LIBC_VERSION,

    _CS_GNU_LIBPTHREAD_VERSION,


    _CS_V5_WIDTH_RESTRICTED_ENVS,



    _CS_V7_WIDTH_RESTRICTED_ENVS,



    _CS_LFS_CFLAGS = 1000,

    _CS_LFS_LDFLAGS,

    _CS_LFS_LIBS,

    _CS_LFS_LINTFLAGS,

    _CS_LFS64_CFLAGS,

    _CS_LFS64_LDFLAGS,

    _CS_LFS64_LIBS,

    _CS_LFS64_LINTFLAGS,


    _CS_XBS5_ILP32_OFF32_CFLAGS = 1100,

    _CS_XBS5_ILP32_OFF32_LDFLAGS,

    _CS_XBS5_ILP32_OFF32_LIBS,

    _CS_XBS5_ILP32_OFF32_LINTFLAGS,

    _CS_XBS5_ILP32_OFFBIG_CFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LDFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LIBS,

    _CS_XBS5_ILP32_OFFBIG_LINTFLAGS,

    _CS_XBS5_LP64_OFF64_CFLAGS,

    _CS_XBS5_LP64_OFF64_LDFLAGS,

    _CS_XBS5_LP64_OFF64_LIBS,

    _CS_XBS5_LP64_OFF64_LINTFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_CFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LDFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LIBS,

    _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V6_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LIBS,

    _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V6_LP64_OFF64_CFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LIBS,

    _CS_POSIX_V6_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V7_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LIBS,

    _CS_POSIX_V7_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V7_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V7_LP64_OFF64_CFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LIBS,

    _CS_POSIX_V7_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LINTFLAGS,


    _CS_V6_ENV,

    _CS_V7_ENV

  };
# 631 "/usr/include/unistd.h" 2 3 4


extern long int pathconf (const char *__path, int __name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int fpathconf (int __fd, int __name) __attribute__ ((__nothrow__ , __leaf__));


extern long int sysconf (int __name) __attribute__ ((__nothrow__ , __leaf__));



extern size_t confstr (int __name, char *__buf, size_t __len) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__write_only__, 2, 3)));




extern __pid_t getpid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t getppid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t getpgrp (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t __getpgid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));

extern __pid_t getpgid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));






extern int setpgid (__pid_t __pid, __pid_t __pgid) __attribute__ ((__nothrow__ , __leaf__));
# 682 "/usr/include/unistd.h" 3 4
extern int setpgrp (void) __attribute__ ((__nothrow__ , __leaf__));






extern __pid_t setsid (void) __attribute__ ((__nothrow__ , __leaf__));



extern __pid_t getsid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));



extern __uid_t getuid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __uid_t geteuid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __gid_t getgid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __gid_t getegid (void) __attribute__ ((__nothrow__ , __leaf__));




extern int getgroups (int __size, __gid_t __list[]) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__write_only__, 2, 1)));


extern int group_member (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__));






extern int setuid (__uid_t __uid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setreuid (__uid_t __ruid, __uid_t __euid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int seteuid (__uid_t __uid) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int setgid (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setregid (__gid_t __rgid, __gid_t __egid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setegid (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__)) ;





extern int getresuid (__uid_t *__ruid, __uid_t *__euid, __uid_t *__suid)
     __attribute__ ((__nothrow__ , __leaf__));



extern int getresgid (__gid_t *__rgid, __gid_t *__egid, __gid_t *__sgid)
     __attribute__ ((__nothrow__ , __leaf__));



extern int setresuid (__uid_t __ruid, __uid_t __euid, __uid_t __suid)
     __attribute__ ((__nothrow__ , __leaf__)) ;



extern int setresgid (__gid_t __rgid, __gid_t __egid, __gid_t __sgid)
     __attribute__ ((__nothrow__ , __leaf__)) ;






extern __pid_t fork (void) __attribute__ ((__nothrow__));







extern __pid_t vfork (void) __attribute__ ((__nothrow__ , __leaf__));






extern __pid_t _Fork (void) __attribute__ ((__nothrow__ , __leaf__));





extern char *ttyname (int __fd) __attribute__ ((__nothrow__ , __leaf__));



extern int ttyname_r (int __fd, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)))
     __attribute__ ((__access__ (__write_only__, 2, 3)));



extern int isatty (int __fd) __attribute__ ((__nothrow__ , __leaf__));




extern int ttyslot (void) __attribute__ ((__nothrow__ , __leaf__));




extern int link (const char *__from, const char *__to)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern int linkat (int __fromfd, const char *__from, int __tofd,
     const char *__to, int __flags)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4))) ;




extern int symlink (const char *__from, const char *__to)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern ssize_t readlink (const char *__restrict __path,
    char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)))
     __attribute__ ((__access__ (__write_only__, 2, 3)));





extern int symlinkat (const char *__from, int __tofd,
        const char *__to) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3))) ;


extern ssize_t readlinkat (int __fd, const char *__restrict __path,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)))
     __attribute__ ((__access__ (__write_only__, 3, 4)));



extern int unlink (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int unlinkat (int __fd, const char *__name, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern int rmdir (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern __pid_t tcgetpgrp (int __fd) __attribute__ ((__nothrow__ , __leaf__));


extern int tcsetpgrp (int __fd, __pid_t __pgrp_id) __attribute__ ((__nothrow__ , __leaf__));






extern char *getlogin (void);







extern int getlogin_r (char *__name, size_t __name_len) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));




extern int setlogin (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







# 1 "/usr/include/bits/getopt_posix.h" 1 3 4
# 27 "/usr/include/bits/getopt_posix.h" 3 4
# 1 "/usr/include/bits/getopt_core.h" 1 3 4
# 28 "/usr/include/bits/getopt_core.h" 3 4








extern char *optarg;
# 50 "/usr/include/bits/getopt_core.h" 3 4
extern int optind;




extern int opterr;



extern int optopt;
# 91 "/usr/include/bits/getopt_core.h" 3 4
extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));


# 28 "/usr/include/bits/getopt_posix.h" 2 3 4


# 49 "/usr/include/bits/getopt_posix.h" 3 4

# 904 "/usr/include/unistd.h" 2 3 4







extern int gethostname (char *__name, size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));






extern int sethostname (const char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__read_only__, 1, 2)));



extern int sethostid (long int __id) __attribute__ ((__nothrow__ , __leaf__)) ;





extern int getdomainname (char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
     __attribute__ ((__access__ (__write_only__, 1, 2)));
extern int setdomainname (const char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__read_only__, 1, 2)));




extern int vhangup (void) __attribute__ ((__nothrow__ , __leaf__));


extern int revoke (const char *__file) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;







extern int profil (unsigned short int *__sample_buffer, size_t __size,
     size_t __offset, unsigned int __scale)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int acct (const char *__name) __attribute__ ((__nothrow__ , __leaf__));



extern char *getusershell (void) __attribute__ ((__nothrow__ , __leaf__));
extern void endusershell (void) __attribute__ ((__nothrow__ , __leaf__));
extern void setusershell (void) __attribute__ ((__nothrow__ , __leaf__));





extern int daemon (int __nochdir, int __noclose) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int chroot (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern char *getpass (const char *__prompt) __attribute__ ((__nonnull__ (1)));







extern int fsync (int __fd);





extern int syncfs (int __fd) __attribute__ ((__nothrow__ , __leaf__));






extern long int gethostid (void);


extern void sync (void) __attribute__ ((__nothrow__ , __leaf__));





extern int getpagesize (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern int getdtablesize (void) __attribute__ ((__nothrow__ , __leaf__));
# 1030 "/usr/include/unistd.h" 3 4
extern int truncate (const char *__file, __off64_t __length) __asm__ ("" "truncate64") __attribute__ ((__nothrow__ , __leaf__))

                  __attribute__ ((__nonnull__ (1))) ;





extern int truncate64 (const char *__file, __off64_t __length)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 1052 "/usr/include/unistd.h" 3 4
extern int ftruncate (int __fd, __off64_t __length) __asm__ ("" "ftruncate64") __attribute__ ((__nothrow__ , __leaf__))
                        ;





extern int ftruncate64 (int __fd, __off64_t __length) __attribute__ ((__nothrow__ , __leaf__)) ;
# 1070 "/usr/include/unistd.h" 3 4
extern int brk (void *__addr) __attribute__ ((__nothrow__ , __leaf__)) ;





extern void *sbrk (intptr_t __delta) __attribute__ ((__nothrow__ , __leaf__));
# 1091 "/usr/include/unistd.h" 3 4
extern long int syscall (long int __sysno, ...) __attribute__ ((__nothrow__ , __leaf__));
# 1142 "/usr/include/unistd.h" 3 4
ssize_t copy_file_range (int __infd, __off64_t *__pinoff,
    int __outfd, __off64_t *__poutoff,
    size_t __length, unsigned int __flags);





extern int fdatasync (int __fildes);
# 1162 "/usr/include/unistd.h" 3 4
extern char *crypt (const char *__key, const char *__salt)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));







extern void swab (const void *__restrict __from, void *__restrict __to,
    ssize_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)))
    __attribute__ ((__access__ (__read_only__, 1, 3)))
    __attribute__ ((__access__ (__write_only__, 2, 3)));
# 1201 "/usr/include/unistd.h" 3 4
int getentropy (void *__buffer, size_t __length)
    __attribute__ ((__access__ (__write_only__, 1, 2)));
# 1211 "/usr/include/unistd.h" 3 4
extern int close_range (unsigned int __fd, unsigned int __max_fd,
   int __flags) __attribute__ ((__nothrow__ , __leaf__));
# 1221 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/unistd_ext.h" 1 3 4
# 34 "/usr/include/bits/unistd_ext.h" 3 4
extern __pid_t gettid (void) __attribute__ ((__nothrow__ , __leaf__));



# 1 "/usr/include/linux/close_range.h" 1 3 4
# 39 "/usr/include/bits/unistd_ext.h" 2 3 4
# 1222 "/usr/include/unistd.h" 2 3 4


# 20 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/stdio.h" 1 3 4
# 28 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 29 "/usr/include/stdio.h" 2 3 4





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 35 "/usr/include/stdio.h" 2 3 4





# 1 "/usr/include/bits/types/__fpos_t.h" 1 3 4




# 1 "/usr/include/bits/types/__mbstate_t.h" 1 3 4
# 13 "/usr/include/bits/types/__mbstate_t.h" 3 4
typedef struct
{
  int __count;
  union
  {
    unsigned int __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 6 "/usr/include/bits/types/__fpos_t.h" 2 3 4




typedef struct _G_fpos_t
{
  __off_t __pos;
  __mbstate_t __state;
} __fpos_t;
# 41 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/__fpos64_t.h" 1 3 4
# 10 "/usr/include/bits/types/__fpos64_t.h" 3 4
typedef struct _G_fpos64_t
{
  __off64_t __pos;
  __mbstate_t __state;
} __fpos64_t;
# 42 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/__FILE.h" 1 3 4



struct _IO_FILE;
typedef struct _IO_FILE __FILE;
# 43 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/FILE.h" 1 3 4



struct _IO_FILE;


typedef struct _IO_FILE FILE;
# 44 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/struct_FILE.h" 1 3 4
# 35 "/usr/include/bits/types/struct_FILE.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 36 "/usr/include/bits/types/struct_FILE.h" 2 3 4

struct _IO_FILE;
struct _IO_marker;
struct _IO_codecvt;
struct _IO_wide_data;




typedef void _IO_lock_t;





struct _IO_FILE
{
  int _flags;


  char *_IO_read_ptr;
  char *_IO_read_end;
  char *_IO_read_base;
  char *_IO_write_base;
  char *_IO_write_ptr;
  char *_IO_write_end;
  char *_IO_buf_base;
  char *_IO_buf_end;


  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;
  int _flags2:24;

  char _short_backupbuf[1];
  __off_t _old_offset;


  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

  _IO_lock_t *_lock;







  __off64_t _offset;

  struct _IO_codecvt *_codecvt;
  struct _IO_wide_data *_wide_data;
  struct _IO_FILE *_freeres_list;
  void *_freeres_buf;
  struct _IO_FILE **_prevchain;
  int _mode;

  int _unused3;

  __uint64_t _total_written;




  char _unused2[12 * sizeof (int) - 5 * sizeof (void *)];
};
# 45 "/usr/include/stdio.h" 2 3 4


# 1 "/usr/include/bits/types/cookie_io_functions_t.h" 1 3 4
# 27 "/usr/include/bits/types/cookie_io_functions_t.h" 3 4
typedef __ssize_t cookie_read_function_t (void *__cookie, char *__buf,
                                          size_t __nbytes);







typedef __ssize_t cookie_write_function_t (void *__cookie, const char *__buf,
                                           size_t __nbytes);







typedef int cookie_seek_function_t (void *__cookie, __off64_t *__pos, int __w);


typedef int cookie_close_function_t (void *__cookie);






typedef struct _IO_cookie_io_functions_t
{
  cookie_read_function_t *read;
  cookie_write_function_t *write;
  cookie_seek_function_t *seek;
  cookie_close_function_t *close;
} cookie_io_functions_t;
# 48 "/usr/include/stdio.h" 2 3 4
# 87 "/usr/include/stdio.h" 3 4
typedef __fpos64_t fpos_t;


typedef __fpos64_t fpos64_t;
# 129 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/stdio_lim.h" 1 3 4
# 130 "/usr/include/stdio.h" 2 3 4
# 149 "/usr/include/stdio.h" 3 4
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;






extern int remove (const char *__filename) __attribute__ ((__nothrow__ , __leaf__));

extern int rename (const char *__old, const char *__new) __attribute__ ((__nothrow__ , __leaf__));



extern int renameat (int __oldfd, const char *__old, int __newfd,
       const char *__new) __attribute__ ((__nothrow__ , __leaf__));
# 179 "/usr/include/stdio.h" 3 4
extern int renameat2 (int __oldfd, const char *__old, int __newfd,
        const char *__new, unsigned int __flags) __attribute__ ((__nothrow__ , __leaf__));






extern int fclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 201 "/usr/include/stdio.h" 3 4
extern FILE *tmpfile (void) __asm__ ("" "tmpfile64")
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;






extern FILE *tmpfile64 (void)
   __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;



extern char *tmpnam (char[20]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern char *tmpnam_r (char __s[20]) __attribute__ ((__nothrow__ , __leaf__)) ;
# 231 "/usr/include/stdio.h" 3 4
extern char *tempnam (const char *__dir, const char *__pfx)
   __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (__builtin_free, 1)));






extern int fflush (FILE *__stream);
# 248 "/usr/include/stdio.h" 3 4
extern int fflush_unlocked (FILE *__stream);
# 258 "/usr/include/stdio.h" 3 4
extern int fcloseall (void);
# 279 "/usr/include/stdio.h" 3 4
extern FILE *fopen (const char *__restrict __filename, const char *__restrict __modes) __asm__ ("" "fopen64")

  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
extern FILE *freopen (const char *__restrict __filename, const char *__restrict __modes, FILE *__restrict __stream) __asm__ ("" "freopen64")


  __attribute__ ((__nonnull__ (3)));






extern FILE *fopen64 (const char *__restrict __filename,
        const char *__restrict __modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
extern FILE *freopen64 (const char *__restrict __filename,
   const char *__restrict __modes,
   FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));




extern FILE *fdopen (int __fd, const char *__modes) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;





extern FILE *fopencookie (void *__restrict __magic_cookie,
     const char *__restrict __modes,
     cookie_io_functions_t __io_funcs) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *fmemopen (void *__s, size_t __len, const char *__modes)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *open_memstream (char **__bufloc, size_t *__sizeloc) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
# 337 "/usr/include/stdio.h" 3 4
extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__nonnull__ (1)));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int fprintf (FILE *__restrict __stream,
      const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int printf (const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nonnull__ (1)));




extern int vprintf (const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));



extern int snprintf (char *__restrict __s, size_t __maxlen,
       const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));





extern int vasprintf (char **__restrict __ptr, const char *__restrict __f,
        __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0))) ;
extern int __asprintf (char **__restrict __ptr,
         const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;
extern int asprintf (char **__restrict __ptr,
       const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;




extern int vdprintf (int __fd, const char *__restrict __fmt,
       __gnuc_va_list __arg)
     __attribute__ ((__format__ (__printf__, 2, 0)));
extern int dprintf (int __fd, const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));







extern int fscanf (FILE *__restrict __stream,
     const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int scanf (const char *__restrict __format, ...) ;

extern int sscanf (const char *__restrict __s,
     const char *__restrict __format, ...) __attribute__ ((__nothrow__ , __leaf__));





# 1 "/usr/include/bits/floatn.h" 1 3 4
# 131 "/usr/include/bits/floatn.h" 3 4
# 1 "/usr/include/bits/floatn-common.h" 1 3 4
# 24 "/usr/include/bits/floatn-common.h" 3 4
# 1 "/usr/include/bits/long-double.h" 1 3 4
# 25 "/usr/include/bits/floatn-common.h" 2 3 4
# 132 "/usr/include/bits/floatn.h" 2 3 4
# 441 "/usr/include/stdio.h" 2 3 4




extern int fscanf (FILE *__restrict __stream, const char *__restrict __format, ...) __asm__ ("" "__isoc23_fscanf")

                                __attribute__ ((__nonnull__ (1)));
extern int scanf (const char *__restrict __format, ...) __asm__ ("" "__isoc23_scanf")
                              ;
extern int sscanf (const char *__restrict __s, const char *__restrict __format, ...) __asm__ ("" "__isoc23_sscanf") __attribute__ ((__nothrow__ , __leaf__))

                      ;
# 493 "/usr/include/stdio.h" 3 4
extern int vfscanf (FILE *__restrict __s, const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));





extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (const char *__restrict __s,
      const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format__ (__scanf__, 2, 0)));






extern int vfscanf (FILE *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vfscanf")



     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));
extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vscanf")

     __attribute__ ((__format__ (__scanf__, 1, 0))) ;
extern int vsscanf (const char *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vsscanf") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__format__ (__scanf__, 2, 0)));
# 578 "/usr/include/stdio.h" 3 4
extern int fgetc (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getc (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern int getchar (void);






extern int getc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getchar_unlocked (void);
# 603 "/usr/include/stdio.h" 3 4
extern int fgetc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));







extern int fputc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));





extern int putchar (int __c);
# 627 "/usr/include/stdio.h" 3 4
extern int fputc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern int putc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream) __attribute__ ((__nonnull__ (1)));


extern int putw (int __w, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 677 "/usr/include/stdio.h" 3 4
extern char *fgets_unlocked (char *__restrict __s, int __n,
        FILE *__restrict __stream)
    __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 689 "/usr/include/stdio.h" 3 4
extern __ssize_t __getdelim (char **__restrict __lineptr,
                             size_t *__restrict __n, int __delimiter,
                             FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));
extern __ssize_t getdelim (char **__restrict __lineptr,
                           size_t *__restrict __n, int __delimiter,
                           FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));


extern __ssize_t getline (char **__restrict __lineptr,
                          size_t *__restrict __n,
                          FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));







extern int fputs (const char *__restrict __s, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (2)));





extern int puts (const char *__s);






extern int ungetc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));




extern size_t fwrite (const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s) __attribute__ ((__nonnull__ (4)));
# 745 "/usr/include/stdio.h" 3 4
extern int fputs_unlocked (const char *__restrict __s,
      FILE *__restrict __stream) __attribute__ ((__nonnull__ (2)));
# 756 "/usr/include/stdio.h" 3 4
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));
extern size_t fwrite_unlocked (const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));







extern int fseek (FILE *__stream, long int __off, int __whence)
  __attribute__ ((__nonnull__ (1)));




extern long int ftell (FILE *__stream) __attribute__ ((__nonnull__ (1)));




extern void rewind (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 802 "/usr/include/stdio.h" 3 4
extern int fseeko (FILE *__stream, __off64_t __off, int __whence) __asm__ ("" "fseeko64")

                   __attribute__ ((__nonnull__ (1)));
extern __off64_t ftello (FILE *__stream) __asm__ ("" "ftello64")
  __attribute__ ((__nonnull__ (1)));
# 828 "/usr/include/stdio.h" 3 4
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos) __asm__ ("" "fgetpos64")

  __attribute__ ((__nonnull__ (1)));
extern int fsetpos (FILE *__stream, const fpos_t *__pos) __asm__ ("" "fsetpos64")

  __attribute__ ((__nonnull__ (1)));







extern int fseeko64 (FILE *__stream, __off64_t __off, int __whence)
  __attribute__ ((__nonnull__ (1)));
extern __off64_t ftello64 (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int fgetpos64 (FILE *__restrict __stream, fpos64_t *__restrict __pos)
  __attribute__ ((__nonnull__ (1)));
extern int fsetpos64 (FILE *__stream, const fpos64_t *__pos) __attribute__ ((__nonnull__ (1)));



extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern void perror (const char *__s) __attribute__ ((__cold__));




extern int fileno (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 887 "/usr/include/stdio.h" 3 4
extern int pclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern FILE *popen (const char *__command, const char *__modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (pclose, 1))) ;






extern char *ctermid (char *__s) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1)));





extern char *cuserid (char *__s)
  __attribute__ ((__access__ (__write_only__, 1)));




struct obstack;


extern int obstack_printf (struct obstack *__restrict __obstack,
      const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3)));
extern int obstack_vprintf (struct obstack *__restrict __obstack,
       const char *__restrict __format,
       __gnuc_va_list __args)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0)));







extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 949 "/usr/include/stdio.h" 3 4
extern int __uflow (FILE *);
extern int __overflow (FILE *, int);
# 973 "/usr/include/stdio.h" 3 4

# 21 "./include/holy/osdep/hostfile_unix.h" 2


# 22 "./include/holy/osdep/hostfile_unix.h"
typedef struct dirent *holy_util_fd_dirent_t;
typedef DIR *holy_util_fd_dir_t;

static inline holy_util_fd_dir_t
holy_util_fd_opendir (const char *name)
{
  return opendir (name);
}

static inline void
holy_util_fd_closedir (holy_util_fd_dir_t dirp)
{
  closedir (dirp);
}

static inline holy_util_fd_dirent_t
holy_util_fd_readdir (holy_util_fd_dir_t dirp)
{
  return readdir (dirp);
}

static inline int
holy_util_unlink (const char *pathname)
{
  return unlink (pathname);
}

static inline int
holy_util_rmdir (const char *pathname)
{
  return rmdir (pathname);
}

static inline int
holy_util_rename (const char *from, const char *to)
{
  return rename (from, to);
}
# 70 "./include/holy/osdep/hostfile_unix.h"
enum holy_util_fd_open_flags_t
  {
    holy_UTIL_FD_O_RDONLY = 
# 72 "./include/holy/osdep/hostfile_unix.h" 3 4
                           00
# 72 "./include/holy/osdep/hostfile_unix.h"
                                   ,
    holy_UTIL_FD_O_WRONLY = 
# 73 "./include/holy/osdep/hostfile_unix.h" 3 4
                           01
# 73 "./include/holy/osdep/hostfile_unix.h"
                                   ,
    holy_UTIL_FD_O_RDWR = 
# 74 "./include/holy/osdep/hostfile_unix.h" 3 4
                         02
# 74 "./include/holy/osdep/hostfile_unix.h"
                               ,
    holy_UTIL_FD_O_CREATTRUNC = 
# 75 "./include/holy/osdep/hostfile_unix.h" 3 4
                               0100 
# 75 "./include/holy/osdep/hostfile_unix.h"
                                       | 
# 75 "./include/holy/osdep/hostfile_unix.h" 3 4
                                         01000
# 75 "./include/holy/osdep/hostfile_unix.h"
                                                ,
    holy_UTIL_FD_O_SYNC = (0

      | 
# 78 "./include/holy/osdep/hostfile_unix.h" 3 4
       04010000


      
# 81 "./include/holy/osdep/hostfile_unix.h"
     | 
# 81 "./include/holy/osdep/hostfile_unix.h" 3 4
       04010000

      
# 83 "./include/holy/osdep/hostfile_unix.h"
     )
  };



typedef int holy_util_fd_t;
# 12 "./include/holy/osdep/hostfile.h" 2
# 13 "./include/holy/emu/hostfile.h" 2

int
holy_util_is_directory (const char *path);
int
holy_util_is_special_file (const char *path);
int
holy_util_is_regular (const char *path);

char *
holy_util_path_concat (size_t n, ...);
char *
holy_util_path_concat_ext (size_t n, ...);

int
holy_util_fd_seek (holy_util_fd_t fd, holy_uint64_t off);
ssize_t
holy_util_fd_read (holy_util_fd_t fd, char *buf, size_t len);
ssize_t
holy_util_fd_write (holy_util_fd_t fd, const char *buf, size_t len);

holy_util_fd_t
holy_util_fd_open (const char *os_dev, int flags);
const char *
holy_util_fd_strerror (void);
void
holy_util_fd_sync (holy_util_fd_t fd);
void
holy_util_disable_fd_syncs (void);
void
holy_util_fd_close (holy_util_fd_t fd);

holy_uint64_t
holy_util_get_fd_size (holy_util_fd_t fd, const char *name, unsigned *log_secsize);
char *
holy_util_make_temporary_file (void);
char *
holy_util_make_temporary_dir (void);
void
holy_util_unlink_recursive (const char *name);
holy_uint32_t
holy_util_get_mtime (const char *name);
# 12 "./include/holy/emu/config.h" 2


const char *
holy_util_get_config_filename (void);
const char *
holy_util_get_pkgdatadir (void);
const char *
holy_util_get_pkglibdir (void);
const char *
holy_util_get_localedir (void);

struct holy_util_config
{
  int is_cryptodisk_enabled;
  char *holy_distributor;
};

void
holy_util_load_config (struct holy_util_config *cfg);

void
holy_util_parse_config (FILE *f, struct holy_util_config *cfg, int simple);
# 10 "./include/holy/types.h" 2
# 63 "./include/holy/types.h"
typedef signed char holy_int8_t;
typedef short holy_int16_t;
typedef int holy_int32_t;

typedef long holy_int64_t;




typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned holy_uint32_t;



typedef unsigned long holy_uint64_t;
# 90 "./include/holy/types.h"
typedef holy_uint64_t holy_addr_t;
typedef holy_uint64_t holy_size_t;
typedef holy_int64_t holy_ssize_t;
# 138 "./include/holy/types.h"
typedef holy_uint64_t holy_properly_aligned_t;




typedef holy_uint64_t holy_off_t;


typedef holy_uint64_t holy_disk_addr_t;


static inline holy_uint16_t holy_swap_bytes16(holy_uint16_t _x)
{
   return (holy_uint16_t) ((_x << 8) | (_x >> 8));
}
# 170 "./include/holy/types.h"
static inline holy_uint32_t holy_swap_bytes32(holy_uint32_t x)
{
 return __builtin_bswap32(x);
}

static inline holy_uint64_t holy_swap_bytes64(holy_uint64_t x)
{
 return __builtin_bswap64(x);
}
# 244 "./include/holy/types.h"
struct holy_unaligned_uint16
{
  holy_uint16_t val;
} __attribute__ ((packed));
struct holy_unaligned_uint32
{
  holy_uint32_t val;
} __attribute__ ((packed));
struct holy_unaligned_uint64
{
  holy_uint64_t val;
} __attribute__ ((packed));

typedef struct holy_unaligned_uint16 holy_unaligned_uint16_t;
typedef struct holy_unaligned_uint32 holy_unaligned_uint32_t;
typedef struct holy_unaligned_uint64 holy_unaligned_uint64_t;

static inline holy_uint16_t holy_get_unaligned16 (const void *ptr)
{
  const struct holy_unaligned_uint16 *dd
    = (const struct holy_unaligned_uint16 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned16 (void *ptr, holy_uint16_t val)
{
  struct holy_unaligned_uint16 *dd = (struct holy_unaligned_uint16 *) ptr;
  dd->val = val;
}

static inline holy_uint32_t holy_get_unaligned32 (const void *ptr)
{
  const struct holy_unaligned_uint32 *dd
    = (const struct holy_unaligned_uint32 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned32 (void *ptr, holy_uint32_t val)
{
  struct holy_unaligned_uint32 *dd = (struct holy_unaligned_uint32 *) ptr;
  dd->val = val;
}

static inline holy_uint64_t holy_get_unaligned64 (const void *ptr)
{
  const struct holy_unaligned_uint64 *dd
    = (const struct holy_unaligned_uint64 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned64 (void *ptr, holy_uint64_t val)
{
  struct holy_unaligned_uint64_t
  {
    holy_uint64_t d;
  } __attribute__ ((packed));
  struct holy_unaligned_uint64_t *dd = (struct holy_unaligned_uint64_t *) ptr;
  dd->d = val;
}
# 13 "./include/holy/dl.h" 2
# 1 "./include/holy/elf.h" 1
# 13 "./include/holy/elf.h"
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;

typedef holy_int64_t holy_ssize_t;





typedef holy_uint16_t Elf32_Half;
typedef holy_uint16_t Elf64_Half;


typedef holy_uint32_t Elf32_Word;
typedef holy_int32_t Elf32_Sword;
typedef holy_uint32_t Elf64_Word;
typedef holy_int32_t Elf64_Sword;


typedef holy_uint64_t Elf32_Xword;
typedef holy_int64_t Elf32_Sxword;
typedef holy_uint64_t Elf64_Xword;
typedef holy_int64_t Elf64_Sxword;


typedef holy_uint32_t Elf32_Addr;
typedef holy_uint64_t Elf64_Addr;


typedef holy_uint32_t Elf32_Off;
typedef holy_uint64_t Elf64_Off;


typedef holy_uint16_t Elf32_Section;
typedef holy_uint16_t Elf64_Section;


typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;






typedef struct
{
  unsigned char e_ident[(16)];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
  unsigned char e_ident[(16)];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;
# 268 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct
{
  Elf64_Word sh_name;
  Elf64_Word sh_type;
  Elf64_Xword sh_flags;
  Elf64_Addr sh_addr;
  Elf64_Off sh_offset;
  Elf64_Xword sh_size;
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Xword sh_addralign;
  Elf64_Xword sh_entsize;
} Elf64_Shdr;
# 367 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Section st_shndx;
} Elf32_Sym;

typedef struct
{
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Section st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
} Elf64_Sym;




typedef struct
{
  Elf32_Half si_boundto;
  Elf32_Half si_flags;
} Elf32_Syminfo;

typedef struct
{
  Elf64_Half si_boundto;
  Elf64_Half si_flags;
} Elf64_Syminfo;
# 481 "./include/holy/elf.h"
typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
} Elf32_Rel;






typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
} Elf64_Rel;



typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
  Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
  Elf64_Sxword r_addend;
} Elf64_Rela;
# 526 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;

typedef struct
{
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;
# 605 "./include/holy/elf.h"
typedef struct
{
  Elf32_Sword d_tag;
  union
    {
      Elf32_Word d_val;
      Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct
{
  Elf64_Sxword d_tag;
  union
    {
      Elf64_Xword d_val;
      Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;
# 769 "./include/holy/elf.h"
typedef struct
{
  Elf32_Half vd_version;
  Elf32_Half vd_flags;
  Elf32_Half vd_ndx;
  Elf32_Half vd_cnt;
  Elf32_Word vd_hash;
  Elf32_Word vd_aux;
  Elf32_Word vd_next;

} Elf32_Verdef;

typedef struct
{
  Elf64_Half vd_version;
  Elf64_Half vd_flags;
  Elf64_Half vd_ndx;
  Elf64_Half vd_cnt;
  Elf64_Word vd_hash;
  Elf64_Word vd_aux;
  Elf64_Word vd_next;

} Elf64_Verdef;
# 811 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word vda_name;
  Elf32_Word vda_next;

} Elf32_Verdaux;

typedef struct
{
  Elf64_Word vda_name;
  Elf64_Word vda_next;

} Elf64_Verdaux;




typedef struct
{
  Elf32_Half vn_version;
  Elf32_Half vn_cnt;
  Elf32_Word vn_file;

  Elf32_Word vn_aux;
  Elf32_Word vn_next;

} Elf32_Verneed;

typedef struct
{
  Elf64_Half vn_version;
  Elf64_Half vn_cnt;
  Elf64_Word vn_file;

  Elf64_Word vn_aux;
  Elf64_Word vn_next;

} Elf64_Verneed;
# 858 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word vna_hash;
  Elf32_Half vna_flags;
  Elf32_Half vna_other;
  Elf32_Word vna_name;
  Elf32_Word vna_next;

} Elf32_Vernaux;

typedef struct
{
  Elf64_Word vna_hash;
  Elf64_Half vna_flags;
  Elf64_Half vna_other;
  Elf64_Word vna_name;
  Elf64_Word vna_next;

} Elf64_Vernaux;
# 892 "./include/holy/elf.h"
typedef struct
{
  int a_type;
  union
    {
      long int a_val;
      void *a_ptr;
      void (*a_fcn) (void);
    } a_un;
} Elf32_auxv_t;

typedef struct
{
  long int a_type;
  union
    {
      long int a_val;
      void *a_ptr;
      void (*a_fcn) (void);
    } a_un;
} Elf64_auxv_t;
# 955 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
} Elf32_Nhdr;

typedef struct
{
  Elf64_Word n_namesz;
  Elf64_Word n_descsz;
  Elf64_Word n_type;
} Elf64_Nhdr;
# 1002 "./include/holy/elf.h"
typedef struct
{
  Elf32_Xword m_value;
  Elf32_Word m_info;
  Elf32_Word m_poffset;
  Elf32_Half m_repeat;
  Elf32_Half m_stride;
} Elf32_Move;

typedef struct
{
  Elf64_Xword m_value;
  Elf64_Xword m_info;
  Elf64_Xword m_poffset;
  Elf64_Half m_repeat;
  Elf64_Half m_stride;
} Elf64_Move;
# 1367 "./include/holy/elf.h"
typedef union
{
  struct
    {
      Elf32_Word gt_current_g_value;
      Elf32_Word gt_unused;
    } gt_header;
  struct
    {
      Elf32_Word gt_g_value;
      Elf32_Word gt_bytes;
    } gt_entry;
} Elf32_gptab;



typedef struct
{
  Elf32_Word ri_gprmask;
  Elf32_Word ri_cprmask[4];
  Elf32_Sword ri_gp_value;
} Elf32_RegInfo;



typedef struct
{
  unsigned char kind;

  unsigned char size;
  Elf32_Section section;

  Elf32_Word info;
} Elf_Options;
# 1443 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word hwp_flags1;
  Elf32_Word hwp_flags2;
} Elf_Options_Hw;
# 1582 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word l_name;
  Elf32_Word l_time_stamp;
  Elf32_Word l_checksum;
  Elf32_Word l_version;
  Elf32_Word l_flags;
} Elf32_Lib;

typedef struct
{
  Elf64_Word l_name;
  Elf64_Word l_time_stamp;
  Elf64_Word l_checksum;
  Elf64_Word l_version;
  Elf64_Word l_flags;
} Elf64_Lib;
# 1613 "./include/holy/elf.h"
typedef Elf32_Addr Elf32_Conflict;
# 14 "./include/holy/dl.h" 2

# 1 "./include/holy/misc.h" 1
# 13 "./include/holy/misc.h"
# 1 "./include/holy/i18n.h" 1
# 9 "./include/holy/i18n.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/i18n.h" 2





extern const char *(*holy_gettext) (const char *s) __attribute__ ((format_arg (1)));



# 1 "/usr/include/locale.h" 1 3 4
# 28 "/usr/include/locale.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 29 "/usr/include/locale.h" 2 3 4
# 1 "/usr/include/bits/locale.h" 1 3 4
# 30 "/usr/include/locale.h" 2 3 4


# 51 "/usr/include/locale.h" 3 4

# 51 "/usr/include/locale.h" 3 4
struct lconv
{


  char *decimal_point;
  char *thousands_sep;





  char *grouping;





  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char int_frac_digits;
  char frac_digits;

  char p_cs_precedes;

  char p_sep_by_space;

  char n_cs_precedes;

  char n_sep_by_space;






  char p_sign_posn;
  char n_sign_posn;


  char int_p_cs_precedes;

  char int_p_sep_by_space;

  char int_n_cs_precedes;

  char int_n_sep_by_space;






  char int_p_sign_posn;
  char int_n_sign_posn;
# 118 "/usr/include/locale.h" 3 4
};



extern char *setlocale (int __category, const char *__locale) __attribute__ ((__nothrow__ , __leaf__));


extern struct lconv *localeconv (void) __attribute__ ((__nothrow__ , __leaf__));
# 135 "/usr/include/locale.h" 3 4
# 1 "/usr/include/bits/types/locale_t.h" 1 3 4
# 22 "/usr/include/bits/types/locale_t.h" 3 4
# 1 "/usr/include/bits/types/__locale_t.h" 1 3 4
# 27 "/usr/include/bits/types/__locale_t.h" 3 4
struct __locale_struct
{

  struct __locale_data *__locales[13];


  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;


  const char *__names[13];
};

typedef struct __locale_struct *__locale_t;
# 23 "/usr/include/bits/types/locale_t.h" 2 3 4

typedef __locale_t locale_t;
# 136 "/usr/include/locale.h" 2 3 4





extern locale_t newlocale (int __category_mask, const char *__locale,
      locale_t __base) __attribute__ ((__nothrow__ , __leaf__));
# 176 "/usr/include/locale.h" 3 4
extern locale_t duplocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));



extern void freelocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));






extern locale_t uselocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));








# 20 "./include/holy/i18n.h" 2
# 1 "/usr/include/libintl.h" 1 3 4
# 34 "/usr/include/libintl.h" 3 4





extern char *gettext (const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (1)));



extern char *dgettext (const char *__domainname, const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));
extern char *__dgettext (const char *__domainname, const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));



extern char *dcgettext (const char *__domainname,
   const char *__msgid, int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));
extern char *__dcgettext (const char *__domainname,
     const char *__msgid, int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));




extern char *ngettext (const char *__msgid1, const char *__msgid2,
         unsigned long int __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (1))) __attribute__ ((__format_arg__ (2)));



extern char *dngettext (const char *__domainname, const char *__msgid1,
   const char *__msgid2, unsigned long int __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2))) __attribute__ ((__format_arg__ (3)));



extern char *dcngettext (const char *__domainname, const char *__msgid1,
    const char *__msgid2, unsigned long int __n,
    int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2))) __attribute__ ((__format_arg__ (3)));





extern char *textdomain (const char *__domainname) __attribute__ ((__nothrow__ , __leaf__));



extern char *bindtextdomain (const char *__domainname,
        const char *__dirname) __attribute__ ((__nothrow__ , __leaf__));



extern char *bind_textdomain_codeset (const char *__domainname,
          const char *__codeset) __attribute__ ((__nothrow__ , __leaf__));
# 121 "/usr/include/libintl.h" 3 4

# 21 "./include/holy/i18n.h" 2
# 40 "./include/holy/i18n.h"

# 40 "./include/holy/i18n.h"
static inline const char * __attribute__ ((always_inline,format_arg (1)))
_ (const char *str)
{
  return gettext(str);
}
# 14 "./include/holy/misc.h" 2
# 32 "./include/holy/misc.h"
typedef unsigned long long int holy_size_t;
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;
typedef holy_int64_t holy_ssize_t;


void *holy_memmove (void *dest, const void *src, holy_size_t n);
char *holy_strcpy (char *dest, const char *src);

static inline char *
holy_strncpy (char *dest, const char *src, int c)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0' && --c)
    ;

  return dest;
}

static inline char *
holy_stpcpy (char *dest, const char *src)
{
  char *d = dest;
  const char *s = src;

  do
    *d++ = *s;
  while (*s++ != '\0');

  return d - 1;
}


static inline void *
holy_memcpy (void *dest, const void *src, holy_size_t n)
{
  return holy_memmove (dest, src, n);
}
# 87 "./include/holy/misc.h"
int holy_memcmp (const void *s1, const void *s2, holy_size_t n);
int holy_strcmp (const char *s1, const char *s2);
int holy_strncmp (const char *s1, const char *s2, holy_size_t n);

char *holy_strchr (const char *s, int c);
char *holy_strrchr (const char *s, int c);
int holy_strword (const char *s, const char *w);



static inline char *
holy_strstr (const char *haystack, const char *needle)
{





  if (*needle != '\0')
    {


      char b = *needle++;

      for (;; haystack++)
 {
   if (*haystack == '\0')

     return 0;
   if (*haystack == b)

     {
       const char *rhaystack = haystack + 1;
       const char *rneedle = needle;

       for (;; rhaystack++, rneedle++)
  {
    if (*rneedle == '\0')

      return (char *) haystack;
    if (*rhaystack == '\0')

      return 0;
    if (*rhaystack != *rneedle)

      break;
  }
     }
 }
    }
  else
    return (char *) haystack;
}

int holy_isspace (int c);

static inline int
holy_isprint (int c)
{
  return (c >= ' ' && c <= '~');
}

static inline int
holy_iscntrl (int c)
{
  return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

static inline int
holy_isalpha (int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int
holy_islower (int c)
{
  return (c >= 'a' && c <= 'z');
}

static inline int
holy_isupper (int c)
{
  return (c >= 'A' && c <= 'Z');
}

static inline int
holy_isgraph (int c)
{
  return (c >= '!' && c <= '~');
}

static inline int
holy_isdigit (int c)
{
  return (c >= '0' && c <= '9');
}

static inline int
holy_isxdigit (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int
holy_isalnum (int c)
{
  return holy_isalpha (c) || holy_isdigit (c);
}

static inline int
holy_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';

  return c;
}

static inline int
holy_toupper (int c)
{
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 'A';

  return c;
}

static inline int
holy_strcasecmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (holy_tolower ((holy_uint8_t) *s1)
   != holy_tolower ((holy_uint8_t) *s2))
 break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

static inline int
holy_strncasecmp (const char *s1, const char *s2, holy_size_t n)
{
  if (n == 0)
    return 0;

  while (*s1 && *s2 && --n)
    {
      if (holy_tolower (*s1) != holy_tolower (*s2))
 break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

unsigned long holy_strtoul (const char *str, char **end, int base);
unsigned long long holy_strtoull (const char *str, char **end, int base);

static inline long
holy_strtol (const char *str, char **end, int base)
{
  int negative = 0;
  unsigned long long magnitude;

  while (*str && holy_isspace (*str))
    str++;

  if (*str == '-')
    {
      negative = 1;
      str++;
    }

  magnitude = holy_strtoull (str, end, base);
  if (negative)
    {
      if (magnitude > (unsigned long) (9223372036854775807L) + 1)
        {
          holy_error (holy_ERR_OUT_OF_RANGE, "overflow is detected");
          return (-9223372036854775807L - 1);
        }
      return -((long) magnitude);
    }
  else
    {
      if (magnitude > (9223372036854775807L))
        {
          holy_error (holy_ERR_OUT_OF_RANGE, "overflow is detected");
          return (9223372036854775807L);
        }
      return (long) magnitude;
    }
}

char *holy_strdup (const char *s) __attribute__ ((warn_unused_result));
char *holy_strndup (const char *s, holy_size_t n) __attribute__ ((warn_unused_result));
void *holy_memset (void *s, int c, holy_size_t n);
holy_size_t holy_strlen (const char *s) __attribute__ ((warn_unused_result));
int holy_printf (const char *fmt, ...) __attribute__ ((format (gnu_printf, 1, 2)));
int holy_printf_ (const char *fmt, ...) __attribute__ ((format (gnu_printf, 1, 2)));



static inline char *
holy_strchrsub (char *output, const char *input, char ch, const char *with)
{
  while (*input)
    {
      if (*input == ch)
 {
   holy_strcpy (output, with);
   output += holy_strlen (with);
   input++;
   continue;
 }
      *output++ = *input++;
    }
  *output = '\0';
  return output;
}

extern void (*holy_xputs) (const char *str);

static inline int
holy_puts (const char *s)
{
  const char nl[2] = "\n";
  holy_xputs (s);
  holy_xputs (nl);

  return 1;
}

int holy_puts_ (const char *s);
void holy_real_dprintf (const char *file,
                                     const int line,
                                     const char *condition,
                                     const char *fmt, ...) __attribute__ ((format (gnu_printf, 4, 5)));
int holy_vprintf (const char *fmt, va_list args);
int holy_snprintf (char *str, holy_size_t n, const char *fmt, ...)
     __attribute__ ((format (gnu_printf, 3, 4)));
int holy_vsnprintf (char *str, holy_size_t n, const char *fmt,
     va_list args);
char *holy_xasprintf (const char *fmt, ...)
     __attribute__ ((format (gnu_printf, 1, 2))) __attribute__ ((warn_unused_result));
char *holy_xvasprintf (const char *fmt, va_list args) __attribute__ ((warn_unused_result));
void holy_exit (void) __attribute__ ((noreturn));
holy_uint64_t holy_divmod64 (holy_uint64_t n,
       holy_uint64_t d,
       holy_uint64_t *r);
# 363 "./include/holy/misc.h"
holy_int64_t
holy_divmod64s (holy_int64_t n,
     holy_int64_t d,
     holy_int64_t *r);

holy_uint32_t
holy_divmod32 (holy_uint32_t n,
     holy_uint32_t d,
     holy_uint32_t *r);

holy_int32_t
holy_divmod32s (holy_int32_t n,
      holy_int32_t d,
      holy_int32_t *r);



static inline char *
holy_memchr (const void *p, int c, holy_size_t len)
{
  const char *s = (const char *) p;
  const char *e = s + len;

  for (; s < e; s++)
    if (*s == c)
      return (char *) s;

  return 0;
}


static inline unsigned int
holy_abs (int x)
{
  if (x < 0)
    return (unsigned int) (-x);
  else
    return (unsigned int) x;
}





void holy_reboot (void) __attribute__ ((noreturn));
# 421 "./include/holy/misc.h"
void holy_halt (void) __attribute__ ((noreturn));
# 431 "./include/holy/misc.h"
static inline void
holy_error_save (struct holy_error_saved *save)
{
  holy_memcpy (save->errmsg, holy_errmsg, sizeof (save->errmsg));
  save->holy_errno = holy_errno;
  holy_errno = holy_ERR_NONE;
}

static inline void
holy_error_load (const struct holy_error_saved *save)
{
  holy_memcpy (holy_errmsg, save->errmsg, sizeof (holy_errmsg));
  holy_errno = save->holy_errno;
}
# 16 "./include/holy/dl.h" 2
# 141 "./include/holy/dl.h"
struct holy_dl_segment
{
  struct holy_dl_segment *next;
  void *addr;
  holy_size_t size;
  unsigned section;
};
typedef struct holy_dl_segment *holy_dl_segment_t;

struct holy_dl;

struct holy_dl_dep
{
  struct holy_dl_dep *next;
  struct holy_dl *mod;
};
typedef struct holy_dl_dep *holy_dl_dep_t;
# 184 "./include/holy/dl.h"
typedef struct holy_dl *holy_dl_t;

holy_dl_t holy_dl_load_file (const char *filename);
holy_dl_t holy_dl_load (const char *name);
holy_dl_t holy_dl_load_core (void *addr, holy_size_t size);
holy_dl_t holy_dl_load_core_noinit (void *addr, holy_size_t size);
int holy_dl_unload (holy_dl_t mod);
void holy_dl_unload_unneeded (void);
int holy_dl_ref (holy_dl_t mod);
int holy_dl_unref (holy_dl_t mod);
extern holy_dl_t holy_dl_head;
# 231 "./include/holy/dl.h"
holy_err_t holy_dl_register_symbol (const char *name, void *addr,
        int isfunc, holy_dl_t mod);

holy_err_t holy_arch_dl_check_header (void *ehdr);
# 249 "./include/holy/dl.h"
holy_err_t
holy_ia64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
     holy_size_t *got);
holy_err_t
holy_arm64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
      holy_size_t *got);
# 263 "./include/holy/dl.h"
holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
     holy_size_t *got);
# 10 "holy-core/disk/host.c" 2


# 1 "./include/holy/emu/hostdisk.h" 1
# 14 "./include/holy/emu/hostdisk.h"
holy_util_fd_t
holy_util_fd_open_device (const holy_disk_t disk, holy_disk_addr_t sector, int flags,
     holy_disk_addr_t *max);

void holy_util_biosdisk_init (const char *dev_map);
void holy_util_biosdisk_fini (void);
char *holy_util_biosdisk_get_holy_dev (const char *os_dev);
const char *holy_util_biosdisk_get_osdev (holy_disk_t disk);
int holy_util_biosdisk_is_present (const char *name);
int holy_util_biosdisk_is_floppy (holy_disk_t disk);
const char *
holy_util_biosdisk_get_compatibility_hint (holy_disk_t disk);
holy_err_t holy_util_biosdisk_flush (struct holy_disk *disk);
holy_err_t
holy_cryptodisk_cheat_mount (const char *sourcedev, const char *cheat);
const char *
holy_util_cryptodisk_get_uuid (holy_disk_t disk);
char *
holy_util_get_ldm (holy_disk_t disk, holy_disk_addr_t start);
int
holy_util_is_ldm (holy_disk_t disk);

holy_err_t
holy_util_ldm_embed (struct holy_disk *disk, unsigned int *nsectors,
       unsigned int max_nsectors,
       holy_embed_type_t embed_type,
       holy_disk_addr_t **sectors);

const char *
holy_hostdisk_os_dev_to_holy_drive (const char *os_dev, int add);


char *
holy_util_get_os_disk (const char *os_dev);

int
holy_util_get_dm_node_linear_info (dev_t dev,
       int *maj, int *min,
       holy_disk_addr_t *st);



holy_int64_t
holy_util_get_fd_size_os (holy_util_fd_t fd, const char *name, unsigned *log_secsize);

holy_disk_addr_t
holy_hostdisk_find_partition_start_os (const char *dev);
void
holy_hostdisk_flush_initial_buffer (const char *os_dev);







struct holy_util_hostdisk_data
{
  char *dev;
  int access_mode;
  holy_util_fd_t fd;
  int is_disk;
  int device_map;
};

void holy_host_init (void);
void holy_host_fini (void);
void holy_hostfs_init (void);
void holy_hostfs_fini (void);
# 13 "holy-core/disk/host.c" 2

int holy_disk_host_i_want_a_reference;

static int
holy_host_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
     holy_disk_pull_t pull)
{
  if (pull != holy_DISK_PULL_NONE)
    return 0;

  if (hook ("host", hook_data))
    return 1;
  return 0;
}

static holy_err_t
holy_host_open (const char *name, holy_disk_t disk)
{
  if (holy_strcmp (name, "host"))
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a host disk");

  disk->total_sectors = 0;
  disk->id = 0;

  disk->data = 0;

  return holy_ERR_NONE;
}

static void
holy_host_close (holy_disk_t disk __attribute((unused)))
{
}

static holy_err_t
holy_host_read (holy_disk_t disk __attribute((unused)),
  holy_disk_addr_t sector __attribute((unused)),
  holy_size_t size __attribute((unused)),
  char *buf __attribute((unused)))
{
  return holy_ERR_OUT_OF_RANGE;
}

static holy_err_t
holy_host_write (holy_disk_t disk __attribute ((unused)),
       holy_disk_addr_t sector __attribute ((unused)),
       holy_size_t size __attribute ((unused)),
       const char *buf __attribute ((unused)))
{
  return holy_ERR_OUT_OF_RANGE;
}

static struct holy_disk_dev holy_host_dev =
  {

    .name = "host",
    .id = holy_DISK_DEVICE_HOST_ID,
    .iterate = holy_host_iterate,
    .open = holy_host_open,
    .close = holy_host_close,
    .read = holy_host_read,
    .write = holy_host_write,
    .next = 0
  };

@MARKER@host@
{
  holy_disk_dev_register (&holy_host_dev);
}

holy_MOD_FINI(host)
{
  holy_disk_dev_unregister (&holy_host_dev);
}
# 0 "holy-core/osdep/init.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "holy-core/osdep/init.c"
# 9 "holy-core/osdep/init.c"
# 1 "holy-core/osdep/basic/init.c" 1





# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 7 "holy-core/osdep/basic/init.c" 2
# 1 "./config-util.h" 1
# 8 "holy-core/osdep/basic/init.c" 2

# 1 "./include/holy/util/misc.h" 1
# 9 "./include/holy/util/misc.h"
# 1 "/usr/include/stdlib.h" 1 3 4
# 26 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 33 "/usr/include/bits/libc-header-start.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 415 "/usr/include/features.h" 3 4
# 1 "/usr/include/features-time64.h" 1 3 4
# 20 "/usr/include/features-time64.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 21 "/usr/include/features-time64.h" 2 3 4
# 1 "/usr/include/bits/timesize.h" 1 3 4
# 19 "/usr/include/bits/timesize.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 20 "/usr/include/bits/timesize.h" 2 3 4
# 22 "/usr/include/features-time64.h" 2 3 4
# 416 "/usr/include/features.h" 2 3 4
# 524 "/usr/include/features.h" 3 4
# 1 "/usr/include/sys/cdefs.h" 1 3 4
# 730 "/usr/include/sys/cdefs.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 731 "/usr/include/sys/cdefs.h" 2 3 4
# 1 "/usr/include/bits/long-double.h" 1 3 4
# 732 "/usr/include/sys/cdefs.h" 2 3 4
# 525 "/usr/include/features.h" 2 3 4
# 548 "/usr/include/features.h" 3 4
# 1 "/usr/include/gnu/stubs.h" 1 3 4
# 10 "/usr/include/gnu/stubs.h" 3 4
# 1 "/usr/include/gnu/stubs-64.h" 1 3 4
# 11 "/usr/include/gnu/stubs.h" 2 3 4
# 549 "/usr/include/features.h" 2 3 4
# 34 "/usr/include/bits/libc-header-start.h" 2 3 4
# 27 "/usr/include/stdlib.h" 2 3 4





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 229 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4

# 229 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4
typedef long unsigned int size_t;
# 344 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 3 4
typedef int wchar_t;
# 33 "/usr/include/stdlib.h" 2 3 4







# 1 "/usr/include/bits/waitflags.h" 1 3 4
# 41 "/usr/include/stdlib.h" 2 3 4
# 1 "/usr/include/bits/waitstatus.h" 1 3 4
# 42 "/usr/include/stdlib.h" 2 3 4
# 56 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/bits/floatn.h" 1 3 4
# 131 "/usr/include/bits/floatn.h" 3 4
# 1 "/usr/include/bits/floatn-common.h" 1 3 4
# 24 "/usr/include/bits/floatn-common.h" 3 4
# 1 "/usr/include/bits/long-double.h" 1 3 4
# 25 "/usr/include/bits/floatn-common.h" 2 3 4
# 132 "/usr/include/bits/floatn.h" 2 3 4
# 57 "/usr/include/stdlib.h" 2 3 4


typedef struct
  {
    int quot;
    int rem;
  } div_t;



typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;





__extension__ typedef struct
  {
    long long int quot;
    long long int rem;
  } lldiv_t;
# 98 "/usr/include/stdlib.h" 3 4
extern size_t __ctype_get_mb_cur_max (void) __attribute__ ((__nothrow__ , __leaf__)) ;



extern double atof (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern int atoi (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern long int atol (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;



__extension__ extern long long int atoll (const char *__nptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;



extern double strtod (const char *__restrict __nptr,
        char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern float strtof (const char *__restrict __nptr,
       char **__restrict __endptr) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern long double strtold (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 141 "/usr/include/stdlib.h" 3 4
extern _Float32 strtof32 (const char *__restrict __nptr,
     char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float64 strtof64 (const char *__restrict __nptr,
     char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float128 strtof128 (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float32x strtof32x (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern _Float64x strtof64x (const char *__restrict __nptr,
       char **__restrict __endptr)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 177 "/usr/include/stdlib.h" 3 4
extern long int strtol (const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern unsigned long int strtoul (const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



__extension__
extern long long int strtoq (const char *__restrict __nptr,
        char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr,
           char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




__extension__
extern long long int strtoll (const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern long int strtol (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtol") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
extern unsigned long int strtoul (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoul") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));

__extension__
extern long long int strtoq (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoll") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
__extension__
extern unsigned long long int strtouq (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoull") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));

__extension__
extern long long int strtoll (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoll") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1)));
__extension__
extern unsigned long long int strtoull (const char *__restrict __nptr, char **__restrict __endptr, int __base) __asm__ ("" "__isoc23_strtoull") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1)));
# 278 "/usr/include/stdlib.h" 3 4
extern int strfromd (char *__dest, size_t __size, const char *__format,
       double __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));

extern int strfromf (char *__dest, size_t __size, const char *__format,
       float __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));

extern int strfroml (char *__dest, size_t __size, const char *__format,
       long double __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 298 "/usr/include/stdlib.h" 3 4
extern int strfromf32 (char *__dest, size_t __size, const char * __format,
         _Float32 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf64 (char *__dest, size_t __size, const char * __format,
         _Float64 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf128 (char *__dest, size_t __size, const char * __format,
   _Float128 __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf32x (char *__dest, size_t __size, const char * __format,
   _Float32x __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));



extern int strfromf64x (char *__dest, size_t __size, const char * __format,
   _Float64x __f)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 338 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/bits/types/locale_t.h" 1 3 4
# 22 "/usr/include/bits/types/locale_t.h" 3 4
# 1 "/usr/include/bits/types/__locale_t.h" 1 3 4
# 27 "/usr/include/bits/types/__locale_t.h" 3 4
struct __locale_struct
{

  struct __locale_data *__locales[13];


  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;


  const char *__names[13];
};

typedef struct __locale_struct *__locale_t;
# 23 "/usr/include/bits/types/locale_t.h" 2 3 4

typedef __locale_t locale_t;
# 339 "/usr/include/stdlib.h" 2 3 4

extern long int strtol_l (const char *__restrict __nptr,
     char **__restrict __endptr, int __base,
     locale_t __loc) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

extern unsigned long int strtoul_l (const char *__restrict __nptr,
        char **__restrict __endptr,
        int __base, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

__extension__
extern long long int strtoll_l (const char *__restrict __nptr,
    char **__restrict __endptr, int __base,
    locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));

__extension__
extern unsigned long long int strtoull_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       int __base, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 4)));





extern long int strtol_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtol_l") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__nonnull__ (1, 4)));
extern unsigned long int strtoul_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoul_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
__extension__
extern long long int strtoll_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoll_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
__extension__
extern unsigned long long int strtoull_l (const char *__restrict __nptr, char **__restrict __endptr, int __base, locale_t __loc) __asm__ ("" "__isoc23_strtoull_l") __attribute__ ((__nothrow__ , __leaf__))




     __attribute__ ((__nonnull__ (1, 4)));
# 415 "/usr/include/stdlib.h" 3 4
extern double strtod_l (const char *__restrict __nptr,
   char **__restrict __endptr, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));

extern float strtof_l (const char *__restrict __nptr,
         char **__restrict __endptr, locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));

extern long double strtold_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));
# 436 "/usr/include/stdlib.h" 3 4
extern _Float32 strtof32_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float64 strtof64_l (const char *__restrict __nptr,
       char **__restrict __endptr,
       locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float128 strtof128_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float32x strtof32x_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));



extern _Float64x strtof64x_l (const char *__restrict __nptr,
         char **__restrict __endptr,
         locale_t __loc)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3)));
# 505 "/usr/include/stdlib.h" 3 4
extern char *l64a (long int __n) __attribute__ ((__nothrow__ , __leaf__)) ;


extern long int a64l (const char *__s)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;




# 1 "/usr/include/sys/types.h" 1 3 4
# 27 "/usr/include/sys/types.h" 3 4


# 1 "/usr/include/bits/types.h" 1 3 4
# 27 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 28 "/usr/include/bits/types.h" 2 3 4
# 1 "/usr/include/bits/timesize.h" 1 3 4
# 19 "/usr/include/bits/timesize.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 20 "/usr/include/bits/timesize.h" 2 3 4
# 29 "/usr/include/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;






typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;



typedef long int __quad_t;
typedef unsigned long int __u_quad_t;







typedef long int __intmax_t;
typedef unsigned long int __uintmax_t;
# 141 "/usr/include/bits/types.h" 3 4
# 1 "/usr/include/bits/typesizes.h" 1 3 4
# 142 "/usr/include/bits/types.h" 2 3 4
# 1 "/usr/include/bits/time64.h" 1 3 4
# 143 "/usr/include/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __suseconds64_t;

typedef int __daddr_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;


typedef long int __fsword_t;

typedef long int __ssize_t;


typedef long int __syscall_slong_t;

typedef unsigned long int __syscall_ulong_t;



typedef __off64_t __loff_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;




typedef int __sig_atomic_t;
# 30 "/usr/include/sys/types.h" 2 3 4



typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;


typedef __loff_t loff_t;






typedef __ino64_t ino_t;




typedef __ino64_t ino64_t;




typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;







typedef __off64_t off_t;




typedef __off64_t off64_t;




typedef __pid_t pid_t;





typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;




# 1 "/usr/include/bits/types/clock_t.h" 1 3 4






typedef __clock_t clock_t;
# 127 "/usr/include/sys/types.h" 2 3 4

# 1 "/usr/include/bits/types/clockid_t.h" 1 3 4






typedef __clockid_t clockid_t;
# 129 "/usr/include/sys/types.h" 2 3 4
# 1 "/usr/include/bits/types/time_t.h" 1 3 4
# 10 "/usr/include/bits/types/time_t.h" 3 4
typedef __time_t time_t;
# 130 "/usr/include/sys/types.h" 2 3 4
# 1 "/usr/include/bits/types/timer_t.h" 1 3 4






typedef __timer_t timer_t;
# 131 "/usr/include/sys/types.h" 2 3 4



typedef __useconds_t useconds_t;



typedef __suseconds_t suseconds_t;





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 145 "/usr/include/sys/types.h" 2 3 4



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;




# 1 "/usr/include/bits/stdint-intn.h" 1 3 4
# 24 "/usr/include/bits/stdint-intn.h" 3 4
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
# 156 "/usr/include/sys/types.h" 2 3 4


typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;


typedef int register_t __attribute__ ((__mode__ (__word__)));
# 176 "/usr/include/sys/types.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 24 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/endian.h" 1 3 4
# 35 "/usr/include/bits/endian.h" 3 4
# 1 "/usr/include/bits/endianness.h" 1 3 4
# 36 "/usr/include/bits/endian.h" 2 3 4
# 25 "/usr/include/endian.h" 2 3 4
# 35 "/usr/include/endian.h" 3 4
# 1 "/usr/include/bits/byteswap.h" 1 3 4
# 33 "/usr/include/bits/byteswap.h" 3 4
static __inline __uint16_t
__bswap_16 (__uint16_t __bsx)
{

  return __builtin_bswap16 (__bsx);



}






static __inline __uint32_t
__bswap_32 (__uint32_t __bsx)
{

  return __builtin_bswap32 (__bsx);



}
# 69 "/usr/include/bits/byteswap.h" 3 4
__extension__ static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{

  return __builtin_bswap64 (__bsx);



}
# 36 "/usr/include/endian.h" 2 3 4
# 1 "/usr/include/bits/uintn-identity.h" 1 3 4
# 32 "/usr/include/bits/uintn-identity.h" 3 4
static __inline __uint16_t
__uint16_identity (__uint16_t __x)
{
  return __x;
}

static __inline __uint32_t
__uint32_identity (__uint32_t __x)
{
  return __x;
}

static __inline __uint64_t
__uint64_identity (__uint64_t __x)
{
  return __x;
}
# 37 "/usr/include/endian.h" 2 3 4
# 177 "/usr/include/sys/types.h" 2 3 4


# 1 "/usr/include/sys/select.h" 1 3 4
# 30 "/usr/include/sys/select.h" 3 4
# 1 "/usr/include/bits/select.h" 1 3 4
# 31 "/usr/include/sys/select.h" 2 3 4


# 1 "/usr/include/bits/types/sigset_t.h" 1 3 4



# 1 "/usr/include/bits/types/__sigset_t.h" 1 3 4




typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
# 5 "/usr/include/bits/types/sigset_t.h" 2 3 4


typedef __sigset_t sigset_t;
# 34 "/usr/include/sys/select.h" 2 3 4



# 1 "/usr/include/bits/types/struct_timeval.h" 1 3 4







struct timeval
{




  __time_t tv_sec;
  __suseconds_t tv_usec;

};
# 38 "/usr/include/sys/select.h" 2 3 4

# 1 "/usr/include/bits/types/struct_timespec.h" 1 3 4
# 11 "/usr/include/bits/types/struct_timespec.h" 3 4
struct timespec
{



  __time_t tv_sec;




  __syscall_slong_t tv_nsec;
# 31 "/usr/include/bits/types/struct_timespec.h" 3 4
};
# 40 "/usr/include/sys/select.h" 2 3 4
# 49 "/usr/include/sys/select.h" 3 4
typedef long int __fd_mask;
# 59 "/usr/include/sys/select.h" 3 4
typedef struct
  {



    __fd_mask fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];





  } fd_set;






typedef __fd_mask fd_mask;
# 91 "/usr/include/sys/select.h" 3 4

# 102 "/usr/include/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 127 "/usr/include/sys/select.h" 3 4
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);
# 153 "/usr/include/sys/select.h" 3 4

# 180 "/usr/include/sys/types.h" 2 3 4





typedef __blksize_t blksize_t;
# 205 "/usr/include/sys/types.h" 3 4
typedef __blkcnt64_t blkcnt_t;



typedef __fsblkcnt64_t fsblkcnt_t;



typedef __fsfilcnt64_t fsfilcnt_t;





typedef __blkcnt64_t blkcnt64_t;
typedef __fsblkcnt64_t fsblkcnt64_t;
typedef __fsfilcnt64_t fsfilcnt64_t;





# 1 "/usr/include/bits/pthreadtypes.h" 1 3 4
# 23 "/usr/include/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/bits/thread-shared-types.h" 1 3 4
# 44 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/pthreadtypes-arch.h" 1 3 4
# 21 "/usr/include/bits/pthreadtypes-arch.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 22 "/usr/include/bits/pthreadtypes-arch.h" 2 3 4
# 45 "/usr/include/bits/thread-shared-types.h" 2 3 4

# 1 "/usr/include/bits/atomic_wide_counter.h" 1 3 4
# 25 "/usr/include/bits/atomic_wide_counter.h" 3 4
typedef union
{
  __extension__ unsigned long long int __value64;
  struct
  {
    unsigned int __low;
    unsigned int __high;
  } __value32;
} __atomic_wide_counter;
# 47 "/usr/include/bits/thread-shared-types.h" 2 3 4




typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;

typedef struct __pthread_internal_slist
{
  struct __pthread_internal_slist *__next;
} __pthread_slist_t;
# 76 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/struct_mutex.h" 1 3 4
# 22 "/usr/include/bits/struct_mutex.h" 3 4
struct __pthread_mutex_s
{
  int __lock;
  unsigned int __count;
  int __owner;

  unsigned int __nusers;



  int __kind;

  short __spins;
  short __elision;
  __pthread_list_t __list;
# 53 "/usr/include/bits/struct_mutex.h" 3 4
};
# 77 "/usr/include/bits/thread-shared-types.h" 2 3 4
# 89 "/usr/include/bits/thread-shared-types.h" 3 4
# 1 "/usr/include/bits/struct_rwlock.h" 1 3 4
# 23 "/usr/include/bits/struct_rwlock.h" 3 4
struct __pthread_rwlock_arch_t
{
  unsigned int __readers;
  unsigned int __writers;
  unsigned int __wrphase_futex;
  unsigned int __writers_futex;
  unsigned int __pad3;
  unsigned int __pad4;

  int __cur_writer;
  int __shared;
  signed char __rwelision;




  unsigned char __pad1[7];


  unsigned long int __pad2;


  unsigned int __flags;
# 55 "/usr/include/bits/struct_rwlock.h" 3 4
};
# 90 "/usr/include/bits/thread-shared-types.h" 2 3 4




struct __pthread_cond_s
{
  __atomic_wide_counter __wseq;
  __atomic_wide_counter __g1_start;
  unsigned int __g_size[2] ;
  unsigned int __g1_orig_size;
  unsigned int __wrefs;
  unsigned int __g_signals[2];
  unsigned int __unused_initialized_1;
  unsigned int __unused_initialized_2;
};

typedef unsigned int __tss_t;
typedef unsigned long int __thrd_t;

typedef struct
{
  int __data ;
} __once_flag;
# 24 "/usr/include/bits/pthreadtypes.h" 2 3 4



typedef unsigned long int pthread_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;




typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;



typedef unsigned int pthread_key_t;



typedef int pthread_once_t;


union pthread_attr_t
{
  char __size[56];
  long int __align;
};

typedef union pthread_attr_t pthread_attr_t;




typedef union
{
  struct __pthread_mutex_s __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;


typedef union
{
  struct __pthread_cond_s __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;





typedef union
{
  struct __pthread_rwlock_arch_t __data;
  char __size[56];
  long int __align;
} pthread_rwlock_t;

typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;





typedef volatile int pthread_spinlock_t;




typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;
# 228 "/usr/include/sys/types.h" 2 3 4



# 515 "/usr/include/stdlib.h" 2 3 4






extern long int random (void) __attribute__ ((__nothrow__ , __leaf__));


extern void srandom (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));





extern char *initstate (unsigned int __seed, char *__statebuf,
   size_t __statelen) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern char *setstate (char *__statebuf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







struct random_data
  {
    int32_t *fptr;
    int32_t *rptr;
    int32_t *state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t *end_ptr;
  };

extern int random_r (struct random_data *__restrict __buf,
       int32_t *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern int srandom_r (unsigned int __seed, struct random_data *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
   size_t __statelen,
   struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));

extern int setstate_r (char *__restrict __statebuf,
         struct random_data *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern int rand (void) __attribute__ ((__nothrow__ , __leaf__));

extern void srand (unsigned int __seed) __attribute__ ((__nothrow__ , __leaf__));



extern int rand_r (unsigned int *__seed) __attribute__ ((__nothrow__ , __leaf__));







extern double drand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern double erand48 (unsigned short int __xsubi[3]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int lrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int nrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int mrand48 (void) __attribute__ ((__nothrow__ , __leaf__));
extern long int jrand48 (unsigned short int __xsubi[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void srand48 (long int __seedval) __attribute__ ((__nothrow__ , __leaf__));
extern unsigned short int *seed48 (unsigned short int __seed16v[3])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern void lcong48 (unsigned short int __param[7]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





struct drand48_data
  {
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    __extension__ unsigned long long int __a;

  };


extern int drand48_r (struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int erand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        double *__restrict __result) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int lrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int nrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int mrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int jrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));

extern int seed48_r (unsigned short int __seed16v[3],
       struct drand48_data *__buffer) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));

extern int lcong48_r (unsigned short int __param[7],
        struct drand48_data *__buffer)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));


extern __uint32_t arc4random (void)
     __attribute__ ((__nothrow__ , __leaf__)) ;


extern void arc4random_buf (void *__buf, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern __uint32_t arc4random_uniform (__uint32_t __upper_bound)
     __attribute__ ((__nothrow__ , __leaf__)) ;




extern void *malloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__))
     __attribute__ ((__alloc_size__ (1))) ;

extern void *calloc (size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_size__ (1, 2))) ;






extern void *realloc (void *__ptr, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__)) __attribute__ ((__alloc_size__ (2)));


extern void free (void *__ptr) __attribute__ ((__nothrow__ , __leaf__));







extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__warn_unused_result__))
     __attribute__ ((__alloc_size__ (2, 3)))
    __attribute__ ((__malloc__ (__builtin_free, 1)));


extern void *reallocarray (void *__ptr, size_t __nmemb, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__ (reallocarray, 1)));



# 1 "/usr/include/alloca.h" 1 3 4
# 24 "/usr/include/alloca.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 25 "/usr/include/alloca.h" 2 3 4







extern void *alloca (size_t __size) __attribute__ ((__nothrow__ , __leaf__));






# 707 "/usr/include/stdlib.h" 2 3 4





extern void *valloc (size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__))
     __attribute__ ((__alloc_size__ (1))) ;




extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;




extern void *aligned_alloc (size_t __alignment, size_t __size)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__alloc_align__ (1)))
     __attribute__ ((__alloc_size__ (2))) ;



extern void abort (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__)) __attribute__ ((__cold__));



extern int atexit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int at_quick_exit (void (*__func) (void)) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern void exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));





extern void quick_exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));





extern void _Exit (int __status) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__noreturn__));




extern char *getenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;




extern char *secure_getenv (const char *__name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;






extern int putenv (char *__string) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int setenv (const char *__name, const char *__value, int __replace)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));


extern int unsetenv (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));






extern int clearenv (void) __attribute__ ((__nothrow__ , __leaf__));
# 814 "/usr/include/stdlib.h" 3 4
extern char *mktemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 830 "/usr/include/stdlib.h" 3 4
extern int mkstemp (char *__template) __asm__ ("" "mkstemp64")
     __attribute__ ((__nonnull__ (1))) ;





extern int mkstemp64 (char *__template) __attribute__ ((__nonnull__ (1))) ;
# 852 "/usr/include/stdlib.h" 3 4
extern int mkstemps (char *__template, int __suffixlen) __asm__ ("" "mkstemps64")
                     __attribute__ ((__nonnull__ (1))) ;





extern int mkstemps64 (char *__template, int __suffixlen)
     __attribute__ ((__nonnull__ (1))) ;
# 870 "/usr/include/stdlib.h" 3 4
extern char *mkdtemp (char *__template) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 884 "/usr/include/stdlib.h" 3 4
extern int mkostemp (char *__template, int __flags) __asm__ ("" "mkostemp64")
     __attribute__ ((__nonnull__ (1))) ;





extern int mkostemp64 (char *__template, int __flags) __attribute__ ((__nonnull__ (1))) ;
# 905 "/usr/include/stdlib.h" 3 4
extern int mkostemps (char *__template, int __suffixlen, int __flags) __asm__ ("" "mkostemps64")

     __attribute__ ((__nonnull__ (1))) ;





extern int mkostemps64 (char *__template, int __suffixlen, int __flags)
     __attribute__ ((__nonnull__ (1))) ;
# 923 "/usr/include/stdlib.h" 3 4
extern int system (const char *__command) ;





extern char *canonicalize_file_name (const char *__name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__malloc__))
     __attribute__ ((__malloc__ (__builtin_free, 1))) ;
# 940 "/usr/include/stdlib.h" 3 4
extern char *realpath (const char *__restrict __name,
         char *__restrict __resolved) __attribute__ ((__nothrow__ , __leaf__)) ;






typedef int (*__compar_fn_t) (const void *, const void *);


typedef __compar_fn_t comparison_fn_t;



typedef int (*__compar_d_fn_t) (const void *, const void *, void *);




extern void *bsearch (const void *__key, const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;







extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));

extern void qsort_r (void *__base, size_t __nmemb, size_t __size,
       __compar_d_fn_t __compar, void *__arg)
  __attribute__ ((__nonnull__ (1, 4)));




extern int abs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern long int labs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;


__extension__ extern long long int llabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;



extern unsigned int uabs (int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern unsigned long int ulabs (long int __x) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
__extension__ extern unsigned long long int ullabs (long long int __x)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;





extern div_t div (int __numer, int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;


__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__)) ;
# 1018 "/usr/include/stdlib.h" 3 4
extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;




extern char *gcvt (double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;




extern char *qecvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qfcvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3))) ;




extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));

extern int qecvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qfcvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3, 4, 5)));





extern int mblen (const char *__s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));


extern int mbtowc (wchar_t *__restrict __pwc,
     const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__));


extern int wctomb (char *__s, wchar_t __wchar) __attribute__ ((__nothrow__ , __leaf__));



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   const char *__restrict __s, size_t __n) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__read_only__, 2)));

extern size_t wcstombs (char *__restrict __s,
   const wchar_t *__restrict __pwcs, size_t __n)
     __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1, 3)))
  __attribute__ ((__access__ (__read_only__, 2)));






extern int rpmatch (const char *__response) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 1105 "/usr/include/stdlib.h" 3 4
extern int getsubopt (char **__restrict __optionp,
        char *const *__restrict __tokens,
        char **__restrict __valuep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2, 3))) ;







extern int posix_openpt (int __oflag) ;







extern int grantpt (int __fd) __attribute__ ((__nothrow__ , __leaf__));



extern int unlockpt (int __fd) __attribute__ ((__nothrow__ , __leaf__));




extern char *ptsname (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int ptsname_r (int __fd, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) __attribute__ ((__access__ (__write_only__, 2, 3)));


extern int getpt (void);






extern int getloadavg (double __loadavg[], int __nelem)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 1161 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/bits/stdlib-float.h" 1 3 4
# 1162 "/usr/include/stdlib.h" 2 3 4
# 1173 "/usr/include/stdlib.h" 3 4

# 10 "./include/holy/util/misc.h" 2
# 1 "/usr/include/stdio.h" 1 3 4
# 28 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/libc-header-start.h" 1 3 4
# 29 "/usr/include/stdio.h" 2 3 4





# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 35 "/usr/include/stdio.h" 2 3 4


# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 1 3 4
# 40 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 38 "/usr/include/stdio.h" 2 3 4


# 1 "/usr/include/bits/types/__fpos_t.h" 1 3 4




# 1 "/usr/include/bits/types/__mbstate_t.h" 1 3 4
# 13 "/usr/include/bits/types/__mbstate_t.h" 3 4
typedef struct
{
  int __count;
  union
  {
    unsigned int __wch;
    char __wchb[4];
  } __value;
} __mbstate_t;
# 6 "/usr/include/bits/types/__fpos_t.h" 2 3 4




typedef struct _G_fpos_t
{
  __off_t __pos;
  __mbstate_t __state;
} __fpos_t;
# 41 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/__fpos64_t.h" 1 3 4
# 10 "/usr/include/bits/types/__fpos64_t.h" 3 4
typedef struct _G_fpos64_t
{
  __off64_t __pos;
  __mbstate_t __state;
} __fpos64_t;
# 42 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/__FILE.h" 1 3 4



struct _IO_FILE;
typedef struct _IO_FILE __FILE;
# 43 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/FILE.h" 1 3 4



struct _IO_FILE;


typedef struct _IO_FILE FILE;
# 44 "/usr/include/stdio.h" 2 3 4
# 1 "/usr/include/bits/types/struct_FILE.h" 1 3 4
# 35 "/usr/include/bits/types/struct_FILE.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 36 "/usr/include/bits/types/struct_FILE.h" 2 3 4

struct _IO_FILE;
struct _IO_marker;
struct _IO_codecvt;
struct _IO_wide_data;




typedef void _IO_lock_t;





struct _IO_FILE
{
  int _flags;


  char *_IO_read_ptr;
  char *_IO_read_end;
  char *_IO_read_base;
  char *_IO_write_base;
  char *_IO_write_ptr;
  char *_IO_write_end;
  char *_IO_buf_base;
  char *_IO_buf_end;


  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;
  int _flags2:24;

  char _short_backupbuf[1];
  __off_t _old_offset;


  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

  _IO_lock_t *_lock;







  __off64_t _offset;

  struct _IO_codecvt *_codecvt;
  struct _IO_wide_data *_wide_data;
  struct _IO_FILE *_freeres_list;
  void *_freeres_buf;
  struct _IO_FILE **_prevchain;
  int _mode;

  int _unused3;

  __uint64_t _total_written;




  char _unused2[12 * sizeof (int) - 5 * sizeof (void *)];
};
# 45 "/usr/include/stdio.h" 2 3 4


# 1 "/usr/include/bits/types/cookie_io_functions_t.h" 1 3 4
# 27 "/usr/include/bits/types/cookie_io_functions_t.h" 3 4
typedef __ssize_t cookie_read_function_t (void *__cookie, char *__buf,
                                          size_t __nbytes);







typedef __ssize_t cookie_write_function_t (void *__cookie, const char *__buf,
                                           size_t __nbytes);







typedef int cookie_seek_function_t (void *__cookie, __off64_t *__pos, int __w);


typedef int cookie_close_function_t (void *__cookie);






typedef struct _IO_cookie_io_functions_t
{
  cookie_read_function_t *read;
  cookie_write_function_t *write;
  cookie_seek_function_t *seek;
  cookie_close_function_t *close;
} cookie_io_functions_t;
# 48 "/usr/include/stdio.h" 2 3 4





typedef __gnuc_va_list va_list;
# 87 "/usr/include/stdio.h" 3 4
typedef __fpos64_t fpos_t;


typedef __fpos64_t fpos64_t;
# 129 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/bits/stdio_lim.h" 1 3 4
# 130 "/usr/include/stdio.h" 2 3 4
# 149 "/usr/include/stdio.h" 3 4
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;






extern int remove (const char *__filename) __attribute__ ((__nothrow__ , __leaf__));

extern int rename (const char *__old, const char *__new) __attribute__ ((__nothrow__ , __leaf__));



extern int renameat (int __oldfd, const char *__old, int __newfd,
       const char *__new) __attribute__ ((__nothrow__ , __leaf__));
# 179 "/usr/include/stdio.h" 3 4
extern int renameat2 (int __oldfd, const char *__old, int __newfd,
        const char *__new, unsigned int __flags) __attribute__ ((__nothrow__ , __leaf__));






extern int fclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 201 "/usr/include/stdio.h" 3 4
extern FILE *tmpfile (void) __asm__ ("" "tmpfile64")
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;






extern FILE *tmpfile64 (void)
   __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;



extern char *tmpnam (char[20]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern char *tmpnam_r (char __s[20]) __attribute__ ((__nothrow__ , __leaf__)) ;
# 231 "/usr/include/stdio.h" 3 4
extern char *tempnam (const char *__dir, const char *__pfx)
   __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (__builtin_free, 1)));






extern int fflush (FILE *__stream);
# 248 "/usr/include/stdio.h" 3 4
extern int fflush_unlocked (FILE *__stream);
# 258 "/usr/include/stdio.h" 3 4
extern int fcloseall (void);
# 279 "/usr/include/stdio.h" 3 4
extern FILE *fopen (const char *__restrict __filename, const char *__restrict __modes) __asm__ ("" "fopen64")

  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
extern FILE *freopen (const char *__restrict __filename, const char *__restrict __modes, FILE *__restrict __stream) __asm__ ("" "freopen64")


  __attribute__ ((__nonnull__ (3)));






extern FILE *fopen64 (const char *__restrict __filename,
        const char *__restrict __modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
extern FILE *freopen64 (const char *__restrict __filename,
   const char *__restrict __modes,
   FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));




extern FILE *fdopen (int __fd, const char *__modes) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;





extern FILE *fopencookie (void *__restrict __magic_cookie,
     const char *__restrict __modes,
     cookie_io_functions_t __io_funcs) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *fmemopen (void *__s, size_t __len, const char *__modes)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;




extern FILE *open_memstream (char **__bufloc, size_t *__sizeloc) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (fclose, 1))) ;
# 337 "/usr/include/stdio.h" 3 4
extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__nonnull__ (1)));



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void setlinebuf (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern int fprintf (FILE *__restrict __stream,
      const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int printf (const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      const char *__restrict __format, ...) __attribute__ ((__nothrow__));





extern int vfprintf (FILE *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nonnull__ (1)));




extern int vprintf (const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, const char *__restrict __format,
       __gnuc_va_list __arg) __attribute__ ((__nothrow__));



extern int snprintf (char *__restrict __s, size_t __maxlen,
       const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 3, 0)));





extern int vasprintf (char **__restrict __ptr, const char *__restrict __f,
        __gnuc_va_list __arg)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0))) ;
extern int __asprintf (char **__restrict __ptr,
         const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;
extern int asprintf (char **__restrict __ptr,
       const char *__restrict __fmt, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3))) ;




extern int vdprintf (int __fd, const char *__restrict __fmt,
       __gnuc_va_list __arg)
     __attribute__ ((__format__ (__printf__, 2, 0)));
extern int dprintf (int __fd, const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));







extern int fscanf (FILE *__restrict __stream,
     const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));




extern int scanf (const char *__restrict __format, ...) ;

extern int sscanf (const char *__restrict __s,
     const char *__restrict __format, ...) __attribute__ ((__nothrow__ , __leaf__));
# 445 "/usr/include/stdio.h" 3 4
extern int fscanf (FILE *__restrict __stream, const char *__restrict __format, ...) __asm__ ("" "__isoc23_fscanf")

                                __attribute__ ((__nonnull__ (1)));
extern int scanf (const char *__restrict __format, ...) __asm__ ("" "__isoc23_scanf")
                              ;
extern int sscanf (const char *__restrict __s, const char *__restrict __format, ...) __asm__ ("" "__isoc23_sscanf") __attribute__ ((__nothrow__ , __leaf__))

                      ;
# 493 "/usr/include/stdio.h" 3 4
extern int vfscanf (FILE *__restrict __s, const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));





extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (const char *__restrict __s,
      const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format__ (__scanf__, 2, 0)));






extern int vfscanf (FILE *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vfscanf")



     __attribute__ ((__format__ (__scanf__, 2, 0))) __attribute__ ((__nonnull__ (1)));
extern int vscanf (const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vscanf")

     __attribute__ ((__format__ (__scanf__, 1, 0))) ;
extern int vsscanf (const char *__restrict __s, const char *__restrict __format, __gnuc_va_list __arg) __asm__ ("" "__isoc23_vsscanf") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__format__ (__scanf__, 2, 0)));
# 578 "/usr/include/stdio.h" 3 4
extern int fgetc (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getc (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern int getchar (void);






extern int getc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int getchar_unlocked (void);
# 603 "/usr/include/stdio.h" 3 4
extern int fgetc_unlocked (FILE *__stream) __attribute__ ((__nonnull__ (1)));







extern int fputc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));





extern int putchar (int __c);
# 627 "/usr/include/stdio.h" 3 4
extern int fputc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern int putc_unlocked (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream) __attribute__ ((__nonnull__ (1)));


extern int putw (int __w, FILE *__stream) __attribute__ ((__nonnull__ (2)));







extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 677 "/usr/include/stdio.h" 3 4
extern char *fgets_unlocked (char *__restrict __s, int __n,
        FILE *__restrict __stream)
    __attribute__ ((__access__ (__write_only__, 1, 2))) __attribute__ ((__nonnull__ (3)));
# 689 "/usr/include/stdio.h" 3 4
extern __ssize_t __getdelim (char **__restrict __lineptr,
                             size_t *__restrict __n, int __delimiter,
                             FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));
extern __ssize_t getdelim (char **__restrict __lineptr,
                           size_t *__restrict __n, int __delimiter,
                           FILE *__restrict __stream) __attribute__ ((__nonnull__ (4)));


extern __ssize_t getline (char **__restrict __lineptr,
                          size_t *__restrict __n,
                          FILE *__restrict __stream) __attribute__ ((__nonnull__ (3)));







extern int fputs (const char *__restrict __s, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (2)));





extern int puts (const char *__s);






extern int ungetc (int __c, FILE *__stream) __attribute__ ((__nonnull__ (2)));






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));




extern size_t fwrite (const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s) __attribute__ ((__nonnull__ (4)));
# 745 "/usr/include/stdio.h" 3 4
extern int fputs_unlocked (const char *__restrict __s,
      FILE *__restrict __stream) __attribute__ ((__nonnull__ (2)));
# 756 "/usr/include/stdio.h" 3 4
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));
extern size_t fwrite_unlocked (const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream)
  __attribute__ ((__nonnull__ (4)));







extern int fseek (FILE *__stream, long int __off, int __whence)
  __attribute__ ((__nonnull__ (1)));




extern long int ftell (FILE *__stream) __attribute__ ((__nonnull__ (1)));




extern void rewind (FILE *__stream) __attribute__ ((__nonnull__ (1)));
# 802 "/usr/include/stdio.h" 3 4
extern int fseeko (FILE *__stream, __off64_t __off, int __whence) __asm__ ("" "fseeko64")

                   __attribute__ ((__nonnull__ (1)));
extern __off64_t ftello (FILE *__stream) __asm__ ("" "ftello64")
  __attribute__ ((__nonnull__ (1)));
# 828 "/usr/include/stdio.h" 3 4
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos) __asm__ ("" "fgetpos64")

  __attribute__ ((__nonnull__ (1)));
extern int fsetpos (FILE *__stream, const fpos_t *__pos) __asm__ ("" "fsetpos64")

  __attribute__ ((__nonnull__ (1)));







extern int fseeko64 (FILE *__stream, __off64_t __off, int __whence)
  __attribute__ ((__nonnull__ (1)));
extern __off64_t ftello64 (FILE *__stream) __attribute__ ((__nonnull__ (1)));
extern int fgetpos64 (FILE *__restrict __stream, fpos64_t *__restrict __pos)
  __attribute__ ((__nonnull__ (1)));
extern int fsetpos64 (FILE *__stream, const fpos64_t *__pos) __attribute__ ((__nonnull__ (1)));



extern void clearerr (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int feof (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));

extern int ferror (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern void clearerr_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int feof_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
extern int ferror_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







extern void perror (const char *__s) __attribute__ ((__cold__));




extern int fileno (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fileno_unlocked (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 887 "/usr/include/stdio.h" 3 4
extern int pclose (FILE *__stream) __attribute__ ((__nonnull__ (1)));





extern FILE *popen (const char *__command, const char *__modes)
  __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (pclose, 1))) ;






extern char *ctermid (char *__s) __attribute__ ((__nothrow__ , __leaf__))
  __attribute__ ((__access__ (__write_only__, 1)));





extern char *cuserid (char *__s)
  __attribute__ ((__access__ (__write_only__, 1)));




struct obstack;


extern int obstack_printf (struct obstack *__restrict __obstack,
      const char *__restrict __format, ...)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 3)));
extern int obstack_vprintf (struct obstack *__restrict __obstack,
       const char *__restrict __format,
       __gnuc_va_list __args)
     __attribute__ ((__nothrow__)) __attribute__ ((__format__ (__printf__, 2, 0)));







extern void flockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int ftrylockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern void funlockfile (FILE *__stream) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 949 "/usr/include/stdio.h" 3 4
extern int __uflow (FILE *);
extern int __overflow (FILE *, int);
# 973 "/usr/include/stdio.h" 3 4

# 11 "./include/holy/util/misc.h" 2
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stdarg.h" 1 3 4
# 12 "./include/holy/util/misc.h" 2
# 1 "/usr/include/setjmp.h" 1 3 4
# 27 "/usr/include/setjmp.h" 3 4


# 1 "/usr/include/bits/setjmp.h" 1 3 4
# 26 "/usr/include/bits/setjmp.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 27 "/usr/include/bits/setjmp.h" 2 3 4




typedef long int __jmp_buf[8];
# 30 "/usr/include/setjmp.h" 2 3 4
# 1 "/usr/include/bits/types/struct___jmp_buf_tag.h" 1 3 4
# 26 "/usr/include/bits/types/struct___jmp_buf_tag.h" 3 4
struct __jmp_buf_tag
  {




    __jmp_buf __jmpbuf;
    int __mask_was_saved;
    __sigset_t __saved_mask;
  };
# 31 "/usr/include/setjmp.h" 2 3 4

typedef struct __jmp_buf_tag jmp_buf[1];



extern int setjmp (jmp_buf __env) __attribute__ ((__nothrow__));




extern int __sigsetjmp (struct __jmp_buf_tag __env[1], int __savemask) __attribute__ ((__nothrow__));



extern int _setjmp (struct __jmp_buf_tag __env[1]) __attribute__ ((__nothrow__));
# 54 "/usr/include/setjmp.h" 3 4
extern void longjmp (struct __jmp_buf_tag __env[1], int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));





extern void _longjmp (struct __jmp_buf_tag __env[1], int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));







typedef struct __jmp_buf_tag sigjmp_buf[1];
# 80 "/usr/include/setjmp.h" 3 4
extern void siglongjmp (sigjmp_buf __env, int __val)
     __attribute__ ((__nothrow__)) __attribute__ ((__noreturn__));
# 90 "/usr/include/setjmp.h" 3 4

# 13 "./include/holy/util/misc.h" 2
# 1 "/usr/include/unistd.h" 1 3 4
# 27 "/usr/include/unistd.h" 3 4

# 202 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/posix_opt.h" 1 3 4
# 203 "/usr/include/unistd.h" 2 3 4



# 1 "/usr/include/bits/environments.h" 1 3 4
# 22 "/usr/include/bits/environments.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 23 "/usr/include/bits/environments.h" 2 3 4
# 207 "/usr/include/unistd.h" 2 3 4
# 226 "/usr/include/unistd.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 227 "/usr/include/unistd.h" 2 3 4
# 267 "/usr/include/unistd.h" 3 4
typedef __intptr_t intptr_t;






typedef __socklen_t socklen_t;
# 287 "/usr/include/unistd.h" 3 4
extern int access (const char *__name, int __type) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int euidaccess (const char *__name, int __type)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int eaccess (const char *__name, int __type)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern int execveat (int __fd, const char *__path, char *const __argv[],
                     char *const __envp[], int __flags)
    __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));






extern int faccessat (int __fd, const char *__file, int __type, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;
# 342 "/usr/include/unistd.h" 3 4
extern __off64_t lseek (int __fd, __off64_t __offset, int __whence) __asm__ ("" "lseek64") __attribute__ ((__nothrow__ , __leaf__))

             ;





extern __off64_t lseek64 (int __fd, __off64_t __offset, int __whence)
     __attribute__ ((__nothrow__ , __leaf__));






extern int close (int __fd);




extern void closefrom (int __lowfd) __attribute__ ((__nothrow__ , __leaf__));







extern ssize_t read (int __fd, void *__buf, size_t __nbytes)
    __attribute__ ((__access__ (__write_only__, 2, 3)));





extern ssize_t write (int __fd, const void *__buf, size_t __n)
    __attribute__ ((__access__ (__read_only__, 2, 3)));
# 404 "/usr/include/unistd.h" 3 4
extern ssize_t pread (int __fd, void *__buf, size_t __nbytes, __off64_t __offset) __asm__ ("" "pread64")


    __attribute__ ((__access__ (__write_only__, 2, 3)));
extern ssize_t pwrite (int __fd, const void *__buf, size_t __nbytes, __off64_t __offset) __asm__ ("" "pwrite64")


    __attribute__ ((__access__ (__read_only__, 2, 3)));
# 422 "/usr/include/unistd.h" 3 4
extern ssize_t pread64 (int __fd, void *__buf, size_t __nbytes,
   __off64_t __offset)
    __attribute__ ((__access__ (__write_only__, 2, 3)));


extern ssize_t pwrite64 (int __fd, const void *__buf, size_t __n,
    __off64_t __offset)
    __attribute__ ((__access__ (__read_only__, 2, 3)));







extern int pipe (int __pipedes[2]) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int pipe2 (int __pipedes[2], int __flags) __attribute__ ((__nothrow__ , __leaf__)) ;
# 452 "/usr/include/unistd.h" 3 4
extern unsigned int alarm (unsigned int __seconds) __attribute__ ((__nothrow__ , __leaf__));
# 464 "/usr/include/unistd.h" 3 4
extern unsigned int sleep (unsigned int __seconds);







extern __useconds_t ualarm (__useconds_t __value, __useconds_t __interval)
     __attribute__ ((__nothrow__ , __leaf__));






extern int usleep (__useconds_t __useconds);
# 489 "/usr/include/unistd.h" 3 4
extern int pause (void);



extern int chown (const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchown (int __fd, __uid_t __owner, __gid_t __group) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int lchown (const char *__file, __uid_t __owner, __gid_t __group)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;






extern int fchownat (int __fd, const char *__file, __uid_t __owner,
       __gid_t __group, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;



extern int chdir (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern int fchdir (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;
# 531 "/usr/include/unistd.h" 3 4
extern char *getcwd (char *__buf, size_t __size) __attribute__ ((__nothrow__ , __leaf__)) ;





extern char *get_current_dir_name (void) __attribute__ ((__nothrow__ , __leaf__));







extern char *getwd (char *__buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__deprecated__))
    __attribute__ ((__access__ (__write_only__, 1)));




extern int dup (int __fd) __attribute__ ((__nothrow__ , __leaf__)) ;


extern int dup2 (int __fd, int __fd2) __attribute__ ((__nothrow__ , __leaf__));




extern int dup3 (int __fd, int __fd2, int __flags) __attribute__ ((__nothrow__ , __leaf__));



extern char **__environ;

extern char **environ;





extern int execve (const char *__path, char *const __argv[],
     char *const __envp[]) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int fexecve (int __fd, char *const __argv[], char *const __envp[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));




extern int execv (const char *__path, char *const __argv[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execle (const char *__path, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execl (const char *__path, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));



extern int execvp (const char *__file, char *const __argv[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int execlp (const char *__file, const char *__arg, ...)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));




extern int execvpe (const char *__file, char *const __argv[],
      char *const __envp[])
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));





extern int nice (int __inc) __attribute__ ((__nothrow__ , __leaf__)) ;




extern void _exit (int __status) __attribute__ ((__noreturn__));





# 1 "/usr/include/bits/confname.h" 1 3 4
# 24 "/usr/include/bits/confname.h" 3 4
enum
  {
    _PC_LINK_MAX,

    _PC_MAX_CANON,

    _PC_MAX_INPUT,

    _PC_NAME_MAX,

    _PC_PATH_MAX,

    _PC_PIPE_BUF,

    _PC_CHOWN_RESTRICTED,

    _PC_NO_TRUNC,

    _PC_VDISABLE,

    _PC_SYNC_IO,

    _PC_ASYNC_IO,

    _PC_PRIO_IO,

    _PC_SOCK_MAXBUF,

    _PC_FILESIZEBITS,

    _PC_REC_INCR_XFER_SIZE,

    _PC_REC_MAX_XFER_SIZE,

    _PC_REC_MIN_XFER_SIZE,

    _PC_REC_XFER_ALIGN,

    _PC_ALLOC_SIZE_MIN,

    _PC_SYMLINK_MAX,

    _PC_2_SYMLINKS

  };


enum
  {
    _SC_ARG_MAX,

    _SC_CHILD_MAX,

    _SC_CLK_TCK,

    _SC_NGROUPS_MAX,

    _SC_OPEN_MAX,

    _SC_STREAM_MAX,

    _SC_TZNAME_MAX,

    _SC_JOB_CONTROL,

    _SC_SAVED_IDS,

    _SC_REALTIME_SIGNALS,

    _SC_PRIORITY_SCHEDULING,

    _SC_TIMERS,

    _SC_ASYNCHRONOUS_IO,

    _SC_PRIORITIZED_IO,

    _SC_SYNCHRONIZED_IO,

    _SC_FSYNC,

    _SC_MAPPED_FILES,

    _SC_MEMLOCK,

    _SC_MEMLOCK_RANGE,

    _SC_MEMORY_PROTECTION,

    _SC_MESSAGE_PASSING,

    _SC_SEMAPHORES,

    _SC_SHARED_MEMORY_OBJECTS,

    _SC_AIO_LISTIO_MAX,

    _SC_AIO_MAX,

    _SC_AIO_PRIO_DELTA_MAX,

    _SC_DELAYTIMER_MAX,

    _SC_MQ_OPEN_MAX,

    _SC_MQ_PRIO_MAX,

    _SC_VERSION,

    _SC_PAGESIZE,


    _SC_RTSIG_MAX,

    _SC_SEM_NSEMS_MAX,

    _SC_SEM_VALUE_MAX,

    _SC_SIGQUEUE_MAX,

    _SC_TIMER_MAX,




    _SC_BC_BASE_MAX,

    _SC_BC_DIM_MAX,

    _SC_BC_SCALE_MAX,

    _SC_BC_STRING_MAX,

    _SC_COLL_WEIGHTS_MAX,

    _SC_EQUIV_CLASS_MAX,

    _SC_EXPR_NEST_MAX,

    _SC_LINE_MAX,

    _SC_RE_DUP_MAX,

    _SC_CHARCLASS_NAME_MAX,


    _SC_2_VERSION,

    _SC_2_C_BIND,

    _SC_2_C_DEV,

    _SC_2_FORT_DEV,

    _SC_2_FORT_RUN,

    _SC_2_SW_DEV,

    _SC_2_LOCALEDEF,


    _SC_PII,

    _SC_PII_XTI,

    _SC_PII_SOCKET,

    _SC_PII_INTERNET,

    _SC_PII_OSI,

    _SC_POLL,

    _SC_SELECT,

    _SC_UIO_MAXIOV,

    _SC_IOV_MAX = _SC_UIO_MAXIOV,

    _SC_PII_INTERNET_STREAM,

    _SC_PII_INTERNET_DGRAM,

    _SC_PII_OSI_COTS,

    _SC_PII_OSI_CLTS,

    _SC_PII_OSI_M,

    _SC_T_IOV_MAX,



    _SC_THREADS,

    _SC_THREAD_SAFE_FUNCTIONS,

    _SC_GETGR_R_SIZE_MAX,

    _SC_GETPW_R_SIZE_MAX,

    _SC_LOGIN_NAME_MAX,

    _SC_TTY_NAME_MAX,

    _SC_THREAD_DESTRUCTOR_ITERATIONS,

    _SC_THREAD_KEYS_MAX,

    _SC_THREAD_STACK_MIN,

    _SC_THREAD_THREADS_MAX,

    _SC_THREAD_ATTR_STACKADDR,

    _SC_THREAD_ATTR_STACKSIZE,

    _SC_THREAD_PRIORITY_SCHEDULING,

    _SC_THREAD_PRIO_INHERIT,

    _SC_THREAD_PRIO_PROTECT,

    _SC_THREAD_PROCESS_SHARED,


    _SC_NPROCESSORS_CONF,

    _SC_NPROCESSORS_ONLN,

    _SC_PHYS_PAGES,

    _SC_AVPHYS_PAGES,

    _SC_ATEXIT_MAX,

    _SC_PASS_MAX,


    _SC_XOPEN_VERSION,

    _SC_XOPEN_XCU_VERSION,

    _SC_XOPEN_UNIX,

    _SC_XOPEN_CRYPT,

    _SC_XOPEN_ENH_I18N,

    _SC_XOPEN_SHM,


    _SC_2_CHAR_TERM,

    _SC_2_C_VERSION,

    _SC_2_UPE,


    _SC_XOPEN_XPG2,

    _SC_XOPEN_XPG3,

    _SC_XOPEN_XPG4,


    _SC_CHAR_BIT,

    _SC_CHAR_MAX,

    _SC_CHAR_MIN,

    _SC_INT_MAX,

    _SC_INT_MIN,

    _SC_LONG_BIT,

    _SC_WORD_BIT,

    _SC_MB_LEN_MAX,

    _SC_NZERO,

    _SC_SSIZE_MAX,

    _SC_SCHAR_MAX,

    _SC_SCHAR_MIN,

    _SC_SHRT_MAX,

    _SC_SHRT_MIN,

    _SC_UCHAR_MAX,

    _SC_UINT_MAX,

    _SC_ULONG_MAX,

    _SC_USHRT_MAX,


    _SC_NL_ARGMAX,

    _SC_NL_LANGMAX,

    _SC_NL_MSGMAX,

    _SC_NL_NMAX,

    _SC_NL_SETMAX,

    _SC_NL_TEXTMAX,


    _SC_XBS5_ILP32_OFF32,

    _SC_XBS5_ILP32_OFFBIG,

    _SC_XBS5_LP64_OFF64,

    _SC_XBS5_LPBIG_OFFBIG,


    _SC_XOPEN_LEGACY,

    _SC_XOPEN_REALTIME,

    _SC_XOPEN_REALTIME_THREADS,


    _SC_ADVISORY_INFO,

    _SC_BARRIERS,

    _SC_BASE,

    _SC_C_LANG_SUPPORT,

    _SC_C_LANG_SUPPORT_R,

    _SC_CLOCK_SELECTION,

    _SC_CPUTIME,

    _SC_THREAD_CPUTIME,

    _SC_DEVICE_IO,

    _SC_DEVICE_SPECIFIC,

    _SC_DEVICE_SPECIFIC_R,

    _SC_FD_MGMT,

    _SC_FIFO,

    _SC_PIPE,

    _SC_FILE_ATTRIBUTES,

    _SC_FILE_LOCKING,

    _SC_FILE_SYSTEM,

    _SC_MONOTONIC_CLOCK,

    _SC_MULTI_PROCESS,

    _SC_SINGLE_PROCESS,

    _SC_NETWORKING,

    _SC_READER_WRITER_LOCKS,

    _SC_SPIN_LOCKS,

    _SC_REGEXP,

    _SC_REGEX_VERSION,

    _SC_SHELL,

    _SC_SIGNALS,

    _SC_SPAWN,

    _SC_SPORADIC_SERVER,

    _SC_THREAD_SPORADIC_SERVER,

    _SC_SYSTEM_DATABASE,

    _SC_SYSTEM_DATABASE_R,

    _SC_TIMEOUTS,

    _SC_TYPED_MEMORY_OBJECTS,

    _SC_USER_GROUPS,

    _SC_USER_GROUPS_R,

    _SC_2_PBS,

    _SC_2_PBS_ACCOUNTING,

    _SC_2_PBS_LOCATE,

    _SC_2_PBS_MESSAGE,

    _SC_2_PBS_TRACK,

    _SC_SYMLOOP_MAX,

    _SC_STREAMS,

    _SC_2_PBS_CHECKPOINT,


    _SC_V6_ILP32_OFF32,

    _SC_V6_ILP32_OFFBIG,

    _SC_V6_LP64_OFF64,

    _SC_V6_LPBIG_OFFBIG,


    _SC_HOST_NAME_MAX,

    _SC_TRACE,

    _SC_TRACE_EVENT_FILTER,

    _SC_TRACE_INHERIT,

    _SC_TRACE_LOG,


    _SC_LEVEL1_ICACHE_SIZE,

    _SC_LEVEL1_ICACHE_ASSOC,

    _SC_LEVEL1_ICACHE_LINESIZE,

    _SC_LEVEL1_DCACHE_SIZE,

    _SC_LEVEL1_DCACHE_ASSOC,

    _SC_LEVEL1_DCACHE_LINESIZE,

    _SC_LEVEL2_CACHE_SIZE,

    _SC_LEVEL2_CACHE_ASSOC,

    _SC_LEVEL2_CACHE_LINESIZE,

    _SC_LEVEL3_CACHE_SIZE,

    _SC_LEVEL3_CACHE_ASSOC,

    _SC_LEVEL3_CACHE_LINESIZE,

    _SC_LEVEL4_CACHE_SIZE,

    _SC_LEVEL4_CACHE_ASSOC,

    _SC_LEVEL4_CACHE_LINESIZE,



    _SC_IPV6 = _SC_LEVEL1_ICACHE_SIZE + 50,

    _SC_RAW_SOCKETS,


    _SC_V7_ILP32_OFF32,

    _SC_V7_ILP32_OFFBIG,

    _SC_V7_LP64_OFF64,

    _SC_V7_LPBIG_OFFBIG,


    _SC_SS_REPL_MAX,


    _SC_TRACE_EVENT_NAME_MAX,

    _SC_TRACE_NAME_MAX,

    _SC_TRACE_SYS_MAX,

    _SC_TRACE_USER_EVENT_MAX,


    _SC_XOPEN_STREAMS,


    _SC_THREAD_ROBUST_PRIO_INHERIT,

    _SC_THREAD_ROBUST_PRIO_PROTECT,


    _SC_MINSIGSTKSZ,


    _SC_SIGSTKSZ

  };


enum
  {
    _CS_PATH,


    _CS_V6_WIDTH_RESTRICTED_ENVS,



    _CS_GNU_LIBC_VERSION,

    _CS_GNU_LIBPTHREAD_VERSION,


    _CS_V5_WIDTH_RESTRICTED_ENVS,



    _CS_V7_WIDTH_RESTRICTED_ENVS,



    _CS_LFS_CFLAGS = 1000,

    _CS_LFS_LDFLAGS,

    _CS_LFS_LIBS,

    _CS_LFS_LINTFLAGS,

    _CS_LFS64_CFLAGS,

    _CS_LFS64_LDFLAGS,

    _CS_LFS64_LIBS,

    _CS_LFS64_LINTFLAGS,


    _CS_XBS5_ILP32_OFF32_CFLAGS = 1100,

    _CS_XBS5_ILP32_OFF32_LDFLAGS,

    _CS_XBS5_ILP32_OFF32_LIBS,

    _CS_XBS5_ILP32_OFF32_LINTFLAGS,

    _CS_XBS5_ILP32_OFFBIG_CFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LDFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LIBS,

    _CS_XBS5_ILP32_OFFBIG_LINTFLAGS,

    _CS_XBS5_LP64_OFF64_CFLAGS,

    _CS_XBS5_LP64_OFF64_LDFLAGS,

    _CS_XBS5_LP64_OFF64_LIBS,

    _CS_XBS5_LP64_OFF64_LINTFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_CFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LDFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LIBS,

    _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V6_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LIBS,

    _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V6_LP64_OFF64_CFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LIBS,

    _CS_POSIX_V6_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V7_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LIBS,

    _CS_POSIX_V7_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V7_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V7_LP64_OFF64_CFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LIBS,

    _CS_POSIX_V7_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LINTFLAGS,


    _CS_V6_ENV,

    _CS_V7_ENV

  };
# 631 "/usr/include/unistd.h" 2 3 4


extern long int pathconf (const char *__path, int __name)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int fpathconf (int __fd, int __name) __attribute__ ((__nothrow__ , __leaf__));


extern long int sysconf (int __name) __attribute__ ((__nothrow__ , __leaf__));



extern size_t confstr (int __name, char *__buf, size_t __len) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__write_only__, 2, 3)));




extern __pid_t getpid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t getppid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t getpgrp (void) __attribute__ ((__nothrow__ , __leaf__));


extern __pid_t __getpgid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));

extern __pid_t getpgid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));






extern int setpgid (__pid_t __pid, __pid_t __pgid) __attribute__ ((__nothrow__ , __leaf__));
# 682 "/usr/include/unistd.h" 3 4
extern int setpgrp (void) __attribute__ ((__nothrow__ , __leaf__));






extern __pid_t setsid (void) __attribute__ ((__nothrow__ , __leaf__));



extern __pid_t getsid (__pid_t __pid) __attribute__ ((__nothrow__ , __leaf__));



extern __uid_t getuid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __uid_t geteuid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __gid_t getgid (void) __attribute__ ((__nothrow__ , __leaf__));


extern __gid_t getegid (void) __attribute__ ((__nothrow__ , __leaf__));




extern int getgroups (int __size, __gid_t __list[]) __attribute__ ((__nothrow__ , __leaf__))
    __attribute__ ((__access__ (__write_only__, 2, 1)));


extern int group_member (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__));






extern int setuid (__uid_t __uid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setreuid (__uid_t __ruid, __uid_t __euid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int seteuid (__uid_t __uid) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int setgid (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setregid (__gid_t __rgid, __gid_t __egid) __attribute__ ((__nothrow__ , __leaf__)) ;




extern int setegid (__gid_t __gid) __attribute__ ((__nothrow__ , __leaf__)) ;





extern int getresuid (__uid_t *__ruid, __uid_t *__euid, __uid_t *__suid)
     __attribute__ ((__nothrow__ , __leaf__));



extern int getresgid (__gid_t *__rgid, __gid_t *__egid, __gid_t *__sgid)
     __attribute__ ((__nothrow__ , __leaf__));



extern int setresuid (__uid_t __ruid, __uid_t __euid, __uid_t __suid)
     __attribute__ ((__nothrow__ , __leaf__)) ;



extern int setresgid (__gid_t __rgid, __gid_t __egid, __gid_t __sgid)
     __attribute__ ((__nothrow__ , __leaf__)) ;






extern __pid_t fork (void) __attribute__ ((__nothrow__));







extern __pid_t vfork (void) __attribute__ ((__nothrow__ , __leaf__));






extern __pid_t _Fork (void) __attribute__ ((__nothrow__ , __leaf__));





extern char *ttyname (int __fd) __attribute__ ((__nothrow__ , __leaf__));



extern int ttyname_r (int __fd, char *__buf, size_t __buflen)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)))
     __attribute__ ((__access__ (__write_only__, 2, 3)));



extern int isatty (int __fd) __attribute__ ((__nothrow__ , __leaf__));




extern int ttyslot (void) __attribute__ ((__nothrow__ , __leaf__));




extern int link (const char *__from, const char *__to)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern int linkat (int __fromfd, const char *__from, int __tofd,
     const char *__to, int __flags)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4))) ;




extern int symlink (const char *__from, const char *__to)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2))) ;




extern ssize_t readlink (const char *__restrict __path,
    char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)))
     __attribute__ ((__access__ (__write_only__, 2, 3)));





extern int symlinkat (const char *__from, int __tofd,
        const char *__to) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 3))) ;


extern ssize_t readlinkat (int __fd, const char *__restrict __path,
      char *__restrict __buf, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)))
     __attribute__ ((__access__ (__write_only__, 3, 4)));



extern int unlink (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern int unlinkat (int __fd, const char *__name, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



extern int rmdir (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));



extern __pid_t tcgetpgrp (int __fd) __attribute__ ((__nothrow__ , __leaf__));


extern int tcsetpgrp (int __fd, __pid_t __pgrp_id) __attribute__ ((__nothrow__ , __leaf__));






extern char *getlogin (void);







extern int getlogin_r (char *__name, size_t __name_len) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));




extern int setlogin (const char *__name) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));







# 1 "/usr/include/bits/getopt_posix.h" 1 3 4
# 27 "/usr/include/bits/getopt_posix.h" 3 4
# 1 "/usr/include/bits/getopt_core.h" 1 3 4
# 28 "/usr/include/bits/getopt_core.h" 3 4








extern char *optarg;
# 50 "/usr/include/bits/getopt_core.h" 3 4
extern int optind;




extern int opterr;



extern int optopt;
# 91 "/usr/include/bits/getopt_core.h" 3 4
extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 3)));


# 28 "/usr/include/bits/getopt_posix.h" 2 3 4


# 49 "/usr/include/bits/getopt_posix.h" 3 4

# 904 "/usr/include/unistd.h" 2 3 4







extern int gethostname (char *__name, size_t __len) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
    __attribute__ ((__access__ (__write_only__, 1, 2)));






extern int sethostname (const char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__read_only__, 1, 2)));



extern int sethostid (long int __id) __attribute__ ((__nothrow__ , __leaf__)) ;





extern int getdomainname (char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)))
     __attribute__ ((__access__ (__write_only__, 1, 2)));
extern int setdomainname (const char *__name, size_t __len)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) __attribute__ ((__access__ (__read_only__, 1, 2)));




extern int vhangup (void) __attribute__ ((__nothrow__ , __leaf__));


extern int revoke (const char *__file) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;







extern int profil (unsigned short int *__sample_buffer, size_t __size,
     size_t __offset, unsigned int __scale)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int acct (const char *__name) __attribute__ ((__nothrow__ , __leaf__));



extern char *getusershell (void) __attribute__ ((__nothrow__ , __leaf__));
extern void endusershell (void) __attribute__ ((__nothrow__ , __leaf__));
extern void setusershell (void) __attribute__ ((__nothrow__ , __leaf__));





extern int daemon (int __nochdir, int __noclose) __attribute__ ((__nothrow__ , __leaf__)) ;






extern int chroot (const char *__path) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;



extern char *getpass (const char *__prompt) __attribute__ ((__nonnull__ (1)));







extern int fsync (int __fd);





extern int syncfs (int __fd) __attribute__ ((__nothrow__ , __leaf__));






extern long int gethostid (void);


extern void sync (void) __attribute__ ((__nothrow__ , __leaf__));





extern int getpagesize (void) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__const__));




extern int getdtablesize (void) __attribute__ ((__nothrow__ , __leaf__));
# 1030 "/usr/include/unistd.h" 3 4
extern int truncate (const char *__file, __off64_t __length) __asm__ ("" "truncate64") __attribute__ ((__nothrow__ , __leaf__))

                  __attribute__ ((__nonnull__ (1))) ;





extern int truncate64 (const char *__file, __off64_t __length)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1))) ;
# 1052 "/usr/include/unistd.h" 3 4
extern int ftruncate (int __fd, __off64_t __length) __asm__ ("" "ftruncate64") __attribute__ ((__nothrow__ , __leaf__))
                        ;





extern int ftruncate64 (int __fd, __off64_t __length) __attribute__ ((__nothrow__ , __leaf__)) ;
# 1070 "/usr/include/unistd.h" 3 4
extern int brk (void *__addr) __attribute__ ((__nothrow__ , __leaf__)) ;





extern void *sbrk (intptr_t __delta) __attribute__ ((__nothrow__ , __leaf__));
# 1091 "/usr/include/unistd.h" 3 4
extern long int syscall (long int __sysno, ...) __attribute__ ((__nothrow__ , __leaf__));
# 1117 "/usr/include/unistd.h" 3 4
extern int lockf (int __fd, int __cmd, __off64_t __len) __asm__ ("" "lockf64")
                       ;





extern int lockf64 (int __fd, int __cmd, __off64_t __len) ;
# 1142 "/usr/include/unistd.h" 3 4
ssize_t copy_file_range (int __infd, __off64_t *__pinoff,
    int __outfd, __off64_t *__poutoff,
    size_t __length, unsigned int __flags);





extern int fdatasync (int __fildes);
# 1162 "/usr/include/unistd.h" 3 4
extern char *crypt (const char *__key, const char *__salt)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));







extern void swab (const void *__restrict __from, void *__restrict __to,
    ssize_t __n) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)))
    __attribute__ ((__access__ (__read_only__, 1, 3)))
    __attribute__ ((__access__ (__write_only__, 2, 3)));
# 1201 "/usr/include/unistd.h" 3 4
int getentropy (void *__buffer, size_t __length)
    __attribute__ ((__access__ (__write_only__, 1, 2)));
# 1211 "/usr/include/unistd.h" 3 4
extern int close_range (unsigned int __fd, unsigned int __max_fd,
   int __flags) __attribute__ ((__nothrow__ , __leaf__));
# 1221 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/bits/unistd_ext.h" 1 3 4
# 34 "/usr/include/bits/unistd_ext.h" 3 4
extern __pid_t gettid (void) __attribute__ ((__nothrow__ , __leaf__));



# 1 "/usr/include/linux/close_range.h" 1 3 4
# 39 "/usr/include/bits/unistd_ext.h" 2 3 4
# 1222 "/usr/include/unistd.h" 2 3 4


# 14 "./include/holy/util/misc.h" 2

# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 16 "./include/holy/util/misc.h" 2
# 1 "./include/holy/types.h" 1
# 9 "./include/holy/types.h"
# 1 "./include/holy/emu/config.h" 1
# 9 "./include/holy/emu/config.h"
# 1 "./include/holy/disk.h" 1
# 9 "./include/holy/disk.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/disk.h" 2

# 1 "./include/holy/symbol.h" 1
# 9 "./include/holy/symbol.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/symbol.h" 2
# 12 "./include/holy/disk.h" 2
# 1 "./include/holy/err.h" 1
# 13 "./include/holy/err.h"

# 13 "./include/holy/err.h"
typedef enum
  {
    holy_ERR_NONE = 0,
    holy_ERR_TEST_FAILURE,
    holy_ERR_BAD_MODULE,
    holy_ERR_OUT_OF_MEMORY,
    holy_ERR_BAD_FILE_TYPE,
    holy_ERR_FILE_NOT_FOUND,
    holy_ERR_FILE_READ_ERROR,
    holy_ERR_BAD_FILENAME,
    holy_ERR_UNKNOWN_FS,
    holy_ERR_BAD_FS,
    holy_ERR_BAD_NUMBER,
    holy_ERR_OUT_OF_RANGE,
    holy_ERR_UNKNOWN_DEVICE,
    holy_ERR_BAD_DEVICE,
    holy_ERR_READ_ERROR,
    holy_ERR_WRITE_ERROR,
    holy_ERR_UNKNOWN_COMMAND,
    holy_ERR_INVALID_COMMAND,
    holy_ERR_BAD_ARGUMENT,
    holy_ERR_BAD_PART_TABLE,
    holy_ERR_UNKNOWN_OS,
    holy_ERR_BAD_OS,
    holy_ERR_NO_KERNEL,
    holy_ERR_BAD_FONT,
    holy_ERR_NOT_IMPLEMENTED_YET,
    holy_ERR_SYMLINK_LOOP,
    holy_ERR_BAD_COMPRESSED_DATA,
    holy_ERR_MENU,
    holy_ERR_TIMEOUT,
    holy_ERR_IO,
    holy_ERR_ACCESS_DENIED,
    holy_ERR_EXTRACTOR,
    holy_ERR_NET_BAD_ADDRESS,
    holy_ERR_NET_ROUTE_LOOP,
    holy_ERR_NET_NO_ROUTE,
    holy_ERR_NET_NO_ANSWER,
    holy_ERR_NET_NO_CARD,
    holy_ERR_WAIT,
    holy_ERR_BUG,
    holy_ERR_NET_PORT_CLOSED,
    holy_ERR_NET_INVALID_RESPONSE,
    holy_ERR_NET_UNKNOWN_ERROR,
    holy_ERR_NET_PACKET_TOO_BIG,
    holy_ERR_NET_NO_DOMAIN,
    holy_ERR_EOF,
    holy_ERR_BAD_SIGNATURE
  }
holy_err_t;

struct holy_error_saved
{
  holy_err_t holy_errno;
  char errmsg[256];
};

extern holy_err_t holy_errno;
extern char holy_errmsg[256];

holy_err_t holy_error (holy_err_t n, const char *fmt, ...);
void holy_fatal (const char *fmt, ...) __attribute__ ((noreturn));
void holy_error_push (void);
int holy_error_pop (void);
void holy_print_error (void);
extern int holy_err_printed_errors;
int holy_err_printf (const char *fmt, ...)
     __attribute__ ((format (__printf__, 1, 2)));
# 13 "./include/holy/disk.h" 2
# 1 "./include/holy/types.h" 1
# 14 "./include/holy/disk.h" 2
# 1 "./include/holy/device.h" 1
# 12 "./include/holy/device.h"
struct holy_disk;
struct holy_net;

struct holy_device
{
  struct holy_disk *disk;
  struct holy_net *net;
};
typedef struct holy_device *holy_device_t;

typedef int (*holy_device_iterate_hook_t) (const char *name, void *data);

holy_device_t holy_device_open (const char *name);
holy_err_t holy_device_close (holy_device_t device);
int holy_device_iterate (holy_device_iterate_hook_t hook,
          void *hook_data);
# 15 "./include/holy/disk.h" 2

# 1 "./include/holy/mm.h" 1
# 11 "./include/holy/mm.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 12 "./include/holy/mm.h" 2





typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long long int holy_int64_t;
typedef holy_int64_t holy_ssize;

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


void holy_mm_init_region (void *addr, holy_size_t size);
void *holy_malloc (holy_size_t size);
void *holy_zalloc (holy_size_t size);
void holy_free (void *ptr);
void *holy_realloc (void *ptr, holy_size_t size);

void *holy_memalign (holy_size_t align, holy_size_t size);


void holy_mm_check_real (const char *file, int line);
# 17 "./include/holy/disk.h" 2



enum holy_disk_dev_id
  {
    holy_DISK_DEVICE_BIOSDISK_ID,
    holy_DISK_DEVICE_OFDISK_ID,
    holy_DISK_DEVICE_LOOPBACK_ID,
    holy_DISK_DEVICE_EFIDISK_ID,
    holy_DISK_DEVICE_DISKFILTER_ID,
    holy_DISK_DEVICE_HOST_ID,
    holy_DISK_DEVICE_ATA_ID,
    holy_DISK_DEVICE_MEMDISK_ID,
    holy_DISK_DEVICE_NAND_ID,
    holy_DISK_DEVICE_SCSI_ID,
    holy_DISK_DEVICE_CRYPTODISK_ID,
    holy_DISK_DEVICE_ARCDISK_ID,
    holy_DISK_DEVICE_HOSTDISK_ID,
    holy_DISK_DEVICE_PROCFS_ID,
    holy_DISK_DEVICE_CBFSDISK_ID,
    holy_DISK_DEVICE_UBOOTDISK_ID,
    holy_DISK_DEVICE_XEN,
  };
# 95 "./include/holy/disk.h"
struct holy_disk;

struct holy_disk_memberlist;


typedef enum
  {
    holy_DISK_PULL_NONE,
    holy_DISK_PULL_REMOVABLE,
    holy_DISK_PULL_RESCAN,
    holy_DISK_PULL_MAX
  } holy_disk_pull_t;

typedef int (*holy_disk_dev_iterate_hook_t) (const char *name, void *data);


struct holy_disk_dev
{

  const char *name;


  enum holy_disk_dev_id id;


  int (*iterate) (holy_disk_dev_iterate_hook_t hook, void *hook_data,
    holy_disk_pull_t pull);


  holy_err_t (*open) (const char *name, struct holy_disk *disk);


  void (*close) (struct holy_disk *disk);


  holy_err_t (*read) (struct holy_disk *disk, holy_disk_addr_t sector,
        holy_size_t size, char *buf);


  holy_err_t (*write) (struct holy_disk *disk, holy_disk_addr_t sector,
         holy_size_t size, const char *buf);


  struct holy_disk_memberlist *(*memberlist) (struct holy_disk *disk);
  const char * (*raidname) (struct holy_disk *disk);



  struct holy_disk_dev *next;
};
typedef struct holy_disk_dev *holy_disk_dev_t;

extern holy_disk_dev_t holy_disk_dev_list;

struct holy_partition;

typedef long int holy_disk_addr_t;

typedef void (*holy_disk_read_hook_t) (holy_disk_addr_t sector,
           unsigned offset, unsigned length,
           void *data);


struct holy_disk
{

  const char *name;


  holy_disk_dev_t dev;


  holy_uint64_t total_sectors;


  unsigned int log_sector_size;


  unsigned int max_agglomerate;


  unsigned long id;


  struct holy_partition *partition;



  holy_disk_read_hook_t read_hook;


  void *read_hook_data;


  void *data;
};
typedef struct holy_disk *holy_disk_t;


struct holy_disk_memberlist
{
  holy_disk_t disk;
  struct holy_disk_memberlist *next;
};
typedef struct holy_disk_memberlist *holy_disk_memberlist_t;
# 220 "./include/holy/disk.h"
void holy_disk_cache_invalidate_all (void);

void holy_disk_dev_register (holy_disk_dev_t dev);
void holy_disk_dev_unregister (holy_disk_dev_t dev);
static inline int
holy_disk_dev_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data)
{
  holy_disk_dev_t p;
  holy_disk_pull_t pull;

  for (pull = 0; pull < holy_DISK_PULL_MAX; pull++)
    for (p = holy_disk_dev_list; p; p = p->next)
      if (p->iterate && (p->iterate) (hook, hook_data, pull))
 return 1;

  return 0;
}

holy_disk_t holy_disk_open (const char *name);
void holy_disk_close (holy_disk_t disk);
holy_err_t holy_disk_read (holy_disk_t disk,
     holy_disk_addr_t sector,
     holy_off_t offset,
     holy_size_t size,
     void *buf);
holy_err_t holy_disk_write (holy_disk_t disk,
       holy_disk_addr_t sector,
       holy_off_t offset,
       holy_size_t size,
       const void *buf);
extern holy_err_t (*holy_disk_write_weak) (holy_disk_t disk,
             holy_disk_addr_t sector,
             holy_off_t offset,
             holy_size_t size,
             const void *buf);


holy_uint64_t holy_disk_get_size (holy_disk_t disk);






extern void (* holy_disk_firmware_fini) (void);
extern int holy_disk_firmware_is_tainted;

static inline void
holy_stop_disk_firmware (void)
{

  holy_disk_firmware_is_tainted = 1;
  if (holy_disk_firmware_fini)
    {
      holy_disk_firmware_fini ();
      holy_disk_firmware_fini = 
# 275 "./include/holy/disk.h" 3 4
                               ((void *)0)
# 275 "./include/holy/disk.h"
                                   ;
    }
}


struct holy_disk_cache
{
  enum holy_disk_dev_id dev_id;
  unsigned long disk_id;
  holy_disk_addr_t sector;
  char *data;
  int lock;
};

extern struct holy_disk_cache holy_disk_cache_table[1021];


void holy_lvm_init (void);
void holy_ldm_init (void);
void holy_mdraid09_init (void);
void holy_mdraid1x_init (void);
void holy_diskfilter_init (void);
void holy_lvm_fini (void);
void holy_ldm_fini (void);
void holy_mdraid09_fini (void);
void holy_mdraid1x_fini (void);
void holy_diskfilter_fini (void);
# 10 "./include/holy/emu/config.h" 2
# 1 "./include/holy/partition.h" 1
# 9 "./include/holy/partition.h"
# 1 "./include/holy/dl.h" 1
# 13 "./include/holy/dl.h"
# 1 "./include/holy/elf.h" 1
# 13 "./include/holy/elf.h"
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;

typedef holy_int64_t holy_ssize_t;





typedef holy_uint16_t Elf32_Half;
typedef holy_uint16_t Elf64_Half;


typedef holy_uint32_t Elf32_Word;
typedef holy_int32_t Elf32_Sword;
typedef holy_uint32_t Elf64_Word;
typedef holy_int32_t Elf64_Sword;


typedef holy_uint64_t Elf32_Xword;
typedef holy_int64_t Elf32_Sxword;
typedef holy_uint64_t Elf64_Xword;
typedef holy_int64_t Elf64_Sxword;


typedef holy_uint32_t Elf32_Addr;
typedef holy_uint64_t Elf64_Addr;


typedef holy_uint32_t Elf32_Off;
typedef holy_uint64_t Elf64_Off;


typedef holy_uint16_t Elf32_Section;
typedef holy_uint16_t Elf64_Section;


typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;






typedef struct
{
  unsigned char e_ident[(16)];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct
{
  unsigned char e_ident[(16)];
  Elf64_Half e_type;
  Elf64_Half e_machine;
  Elf64_Word e_version;
  Elf64_Addr e_entry;
  Elf64_Off e_phoff;
  Elf64_Off e_shoff;
  Elf64_Word e_flags;
  Elf64_Half e_ehsize;
  Elf64_Half e_phentsize;
  Elf64_Half e_phnum;
  Elf64_Half e_shentsize;
  Elf64_Half e_shnum;
  Elf64_Half e_shstrndx;
} Elf64_Ehdr;
# 268 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct
{
  Elf64_Word sh_name;
  Elf64_Word sh_type;
  Elf64_Xword sh_flags;
  Elf64_Addr sh_addr;
  Elf64_Off sh_offset;
  Elf64_Xword sh_size;
  Elf64_Word sh_link;
  Elf64_Word sh_info;
  Elf64_Xword sh_addralign;
  Elf64_Xword sh_entsize;
} Elf64_Shdr;
# 367 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Section st_shndx;
} Elf32_Sym;

typedef struct
{
  Elf64_Word st_name;
  unsigned char st_info;
  unsigned char st_other;
  Elf64_Section st_shndx;
  Elf64_Addr st_value;
  Elf64_Xword st_size;
} Elf64_Sym;




typedef struct
{
  Elf32_Half si_boundto;
  Elf32_Half si_flags;
} Elf32_Syminfo;

typedef struct
{
  Elf64_Half si_boundto;
  Elf64_Half si_flags;
} Elf64_Syminfo;
# 481 "./include/holy/elf.h"
typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
} Elf32_Rel;






typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
} Elf64_Rel;



typedef struct
{
  Elf32_Addr r_offset;
  Elf32_Word r_info;
  Elf32_Sword r_addend;
} Elf32_Rela;

typedef struct
{
  Elf64_Addr r_offset;
  Elf64_Xword r_info;
  Elf64_Sxword r_addend;
} Elf64_Rela;
# 526 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} Elf32_Phdr;

typedef struct
{
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;
# 605 "./include/holy/elf.h"
typedef struct
{
  Elf32_Sword d_tag;
  union
    {
      Elf32_Word d_val;
      Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct
{
  Elf64_Sxword d_tag;
  union
    {
      Elf64_Xword d_val;
      Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;
# 769 "./include/holy/elf.h"
typedef struct
{
  Elf32_Half vd_version;
  Elf32_Half vd_flags;
  Elf32_Half vd_ndx;
  Elf32_Half vd_cnt;
  Elf32_Word vd_hash;
  Elf32_Word vd_aux;
  Elf32_Word vd_next;

} Elf32_Verdef;

typedef struct
{
  Elf64_Half vd_version;
  Elf64_Half vd_flags;
  Elf64_Half vd_ndx;
  Elf64_Half vd_cnt;
  Elf64_Word vd_hash;
  Elf64_Word vd_aux;
  Elf64_Word vd_next;

} Elf64_Verdef;
# 811 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word vda_name;
  Elf32_Word vda_next;

} Elf32_Verdaux;

typedef struct
{
  Elf64_Word vda_name;
  Elf64_Word vda_next;

} Elf64_Verdaux;




typedef struct
{
  Elf32_Half vn_version;
  Elf32_Half vn_cnt;
  Elf32_Word vn_file;

  Elf32_Word vn_aux;
  Elf32_Word vn_next;

} Elf32_Verneed;

typedef struct
{
  Elf64_Half vn_version;
  Elf64_Half vn_cnt;
  Elf64_Word vn_file;

  Elf64_Word vn_aux;
  Elf64_Word vn_next;

} Elf64_Verneed;
# 858 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word vna_hash;
  Elf32_Half vna_flags;
  Elf32_Half vna_other;
  Elf32_Word vna_name;
  Elf32_Word vna_next;

} Elf32_Vernaux;

typedef struct
{
  Elf64_Word vna_hash;
  Elf64_Half vna_flags;
  Elf64_Half vna_other;
  Elf64_Word vna_name;
  Elf64_Word vna_next;

} Elf64_Vernaux;
# 892 "./include/holy/elf.h"
typedef struct
{
  int a_type;
  union
    {
      long int a_val;
      void *a_ptr;
      void (*a_fcn) (void);
    } a_un;
} Elf32_auxv_t;

typedef struct
{
  long int a_type;
  union
    {
      long int a_val;
      void *a_ptr;
      void (*a_fcn) (void);
    } a_un;
} Elf64_auxv_t;
# 955 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
} Elf32_Nhdr;

typedef struct
{
  Elf64_Word n_namesz;
  Elf64_Word n_descsz;
  Elf64_Word n_type;
} Elf64_Nhdr;
# 1002 "./include/holy/elf.h"
typedef struct
{
  Elf32_Xword m_value;
  Elf32_Word m_info;
  Elf32_Word m_poffset;
  Elf32_Half m_repeat;
  Elf32_Half m_stride;
} Elf32_Move;

typedef struct
{
  Elf64_Xword m_value;
  Elf64_Xword m_info;
  Elf64_Xword m_poffset;
  Elf64_Half m_repeat;
  Elf64_Half m_stride;
} Elf64_Move;
# 1367 "./include/holy/elf.h"
typedef union
{
  struct
    {
      Elf32_Word gt_current_g_value;
      Elf32_Word gt_unused;
    } gt_header;
  struct
    {
      Elf32_Word gt_g_value;
      Elf32_Word gt_bytes;
    } gt_entry;
} Elf32_gptab;



typedef struct
{
  Elf32_Word ri_gprmask;
  Elf32_Word ri_cprmask[4];
  Elf32_Sword ri_gp_value;
} Elf32_RegInfo;



typedef struct
{
  unsigned char kind;

  unsigned char size;
  Elf32_Section section;

  Elf32_Word info;
} Elf_Options;
# 1443 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word hwp_flags1;
  Elf32_Word hwp_flags2;
} Elf_Options_Hw;
# 1582 "./include/holy/elf.h"
typedef struct
{
  Elf32_Word l_name;
  Elf32_Word l_time_stamp;
  Elf32_Word l_checksum;
  Elf32_Word l_version;
  Elf32_Word l_flags;
} Elf32_Lib;

typedef struct
{
  Elf64_Word l_name;
  Elf64_Word l_time_stamp;
  Elf64_Word l_checksum;
  Elf64_Word l_version;
  Elf64_Word l_flags;
} Elf64_Lib;
# 1613 "./include/holy/elf.h"
typedef Elf32_Addr Elf32_Conflict;
# 14 "./include/holy/dl.h" 2
# 1 "./include/holy/list.h" 1
# 11 "./include/holy/list.h"
# 1 "./include/holy/compiler.h" 1
# 12 "./include/holy/list.h" 2

struct holy_list
{
  struct holy_list *next;
  struct holy_list **prev;
};
typedef struct holy_list *holy_list_t;

void holy_list_push (holy_list_t *head, holy_list_t item);
void holy_list_remove (holy_list_t item);




static inline void *
holy_bad_type_cast_real (int line, const char *file)
     __attribute__ ((__error__ ("bad type cast between incompatible holy types")));

static inline void *
holy_bad_type_cast_real (int line, const char *file)
{
  holy_fatal ("error:%s:%u: bad type cast between incompatible holy types",
       file, line);
}
# 50 "./include/holy/list.h"
struct holy_named_list
{
  struct holy_named_list *next;
  struct holy_named_list **prev;
  char *name;
};
typedef struct holy_named_list *holy_named_list_t;

void * holy_named_list_find (holy_named_list_t head,
       const char *name);
# 15 "./include/holy/dl.h" 2
# 1 "./include/holy/misc.h" 1
# 13 "./include/holy/misc.h"
# 1 "./include/holy/i18n.h" 1
# 9 "./include/holy/i18n.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/i18n.h" 2





extern const char *(*holy_gettext) (const char *s) __attribute__ ((format_arg (1)));



# 1 "/usr/include/locale.h" 1 3 4
# 28 "/usr/include/locale.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 29 "/usr/include/locale.h" 2 3 4
# 1 "/usr/include/bits/locale.h" 1 3 4
# 30 "/usr/include/locale.h" 2 3 4


# 51 "/usr/include/locale.h" 3 4

# 51 "/usr/include/locale.h" 3 4
struct lconv
{


  char *decimal_point;
  char *thousands_sep;





  char *grouping;





  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char int_frac_digits;
  char frac_digits;

  char p_cs_precedes;

  char p_sep_by_space;

  char n_cs_precedes;

  char n_sep_by_space;






  char p_sign_posn;
  char n_sign_posn;


  char int_p_cs_precedes;

  char int_p_sep_by_space;

  char int_n_cs_precedes;

  char int_n_sep_by_space;






  char int_p_sign_posn;
  char int_n_sign_posn;
# 118 "/usr/include/locale.h" 3 4
};



extern char *setlocale (int __category, const char *__locale) __attribute__ ((__nothrow__ , __leaf__));


extern struct lconv *localeconv (void) __attribute__ ((__nothrow__ , __leaf__));
# 141 "/usr/include/locale.h" 3 4
extern locale_t newlocale (int __category_mask, const char *__locale,
      locale_t __base) __attribute__ ((__nothrow__ , __leaf__));
# 176 "/usr/include/locale.h" 3 4
extern locale_t duplocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));



extern void freelocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));






extern locale_t uselocale (locale_t __dataset) __attribute__ ((__nothrow__ , __leaf__));








# 20 "./include/holy/i18n.h" 2
# 1 "/usr/include/libintl.h" 1 3 4
# 34 "/usr/include/libintl.h" 3 4





extern char *gettext (const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (1)));



extern char *dgettext (const char *__domainname, const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));
extern char *__dgettext (const char *__domainname, const char *__msgid)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));



extern char *dcgettext (const char *__domainname,
   const char *__msgid, int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));
extern char *__dcgettext (const char *__domainname,
     const char *__msgid, int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2)));




extern char *ngettext (const char *__msgid1, const char *__msgid2,
         unsigned long int __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (1))) __attribute__ ((__format_arg__ (2)));



extern char *dngettext (const char *__domainname, const char *__msgid1,
   const char *__msgid2, unsigned long int __n)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2))) __attribute__ ((__format_arg__ (3)));



extern char *dcngettext (const char *__domainname, const char *__msgid1,
    const char *__msgid2, unsigned long int __n,
    int __category)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__format_arg__ (2))) __attribute__ ((__format_arg__ (3)));





extern char *textdomain (const char *__domainname) __attribute__ ((__nothrow__ , __leaf__));



extern char *bindtextdomain (const char *__domainname,
        const char *__dirname) __attribute__ ((__nothrow__ , __leaf__));



extern char *bind_textdomain_codeset (const char *__domainname,
          const char *__codeset) __attribute__ ((__nothrow__ , __leaf__));
# 121 "/usr/include/libintl.h" 3 4

# 21 "./include/holy/i18n.h" 2
# 40 "./include/holy/i18n.h"

# 40 "./include/holy/i18n.h"
static inline const char * __attribute__ ((always_inline,format_arg (1)))
_ (const char *str)
{
  return gettext(str);
}
# 14 "./include/holy/misc.h" 2
# 32 "./include/holy/misc.h"
typedef unsigned long long int holy_size_t;
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;
typedef holy_int64_t holy_ssize_t;


void *holy_memmove (void *dest, const void *src, holy_size_t n);
char *holy_strcpy (char *dest, const char *src);

static inline char *
holy_strncpy (char *dest, const char *src, int c)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0' && --c)
    ;

  return dest;
}

static inline char *
holy_stpcpy (char *dest, const char *src)
{
  char *d = dest;
  const char *s = src;

  do
    *d++ = *s;
  while (*s++ != '\0');

  return d - 1;
}


static inline void *
holy_memcpy (void *dest, const void *src, holy_size_t n)
{
  return holy_memmove (dest, src, n);
}
# 87 "./include/holy/misc.h"
int holy_memcmp (const void *s1, const void *s2, holy_size_t n);
int holy_strcmp (const char *s1, const char *s2);
int holy_strncmp (const char *s1, const char *s2, holy_size_t n);

char *holy_strchr (const char *s, int c);
char *holy_strrchr (const char *s, int c);
int holy_strword (const char *s, const char *w);



static inline char *
holy_strstr (const char *haystack, const char *needle)
{





  if (*needle != '\0')
    {


      char b = *needle++;

      for (;; haystack++)
 {
   if (*haystack == '\0')

     return 0;
   if (*haystack == b)

     {
       const char *rhaystack = haystack + 1;
       const char *rneedle = needle;

       for (;; rhaystack++, rneedle++)
  {
    if (*rneedle == '\0')

      return (char *) haystack;
    if (*rhaystack == '\0')

      return 0;
    if (*rhaystack != *rneedle)

      break;
  }
     }
 }
    }
  else
    return (char *) haystack;
}

int holy_isspace (int c);

static inline int
holy_isprint (int c)
{
  return (c >= ' ' && c <= '~');
}

static inline int
holy_iscntrl (int c)
{
  return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

static inline int
holy_isalpha (int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int
holy_islower (int c)
{
  return (c >= 'a' && c <= 'z');
}

static inline int
holy_isupper (int c)
{
  return (c >= 'A' && c <= 'Z');
}

static inline int
holy_isgraph (int c)
{
  return (c >= '!' && c <= '~');
}

static inline int
holy_isdigit (int c)
{
  return (c >= '0' && c <= '9');
}

static inline int
holy_isxdigit (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int
holy_isalnum (int c)
{
  return holy_isalpha (c) || holy_isdigit (c);
}

static inline int
holy_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';

  return c;
}

static inline int
holy_toupper (int c)
{
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 'A';

  return c;
}

static inline int
holy_strcasecmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (holy_tolower ((holy_uint8_t) *s1)
   != holy_tolower ((holy_uint8_t) *s2))
 break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

static inline int
holy_strncasecmp (const char *s1, const char *s2, holy_size_t n)
{
  if (n == 0)
    return 0;

  while (*s1 && *s2 && --n)
    {
      if (holy_tolower (*s1) != holy_tolower (*s2))
 break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

unsigned long holy_strtoul (const char *str, char **end, int base);
unsigned long long holy_strtoull (const char *str, char **end, int base);

static inline long
holy_strtol (const char *str, char **end, int base)
{
  int negative = 0;
  unsigned long long magnitude;

  while (*str && holy_isspace (*str))
    str++;

  if (*str == '-')
    {
      negative = 1;
      str++;
    }

  magnitude = holy_strtoull (str, end, base);
  if (negative)
    {
      if (magnitude > (unsigned long) (9223372036854775807L) + 1)
        {
          holy_error (holy_ERR_OUT_OF_RANGE, "overflow is detected");
          return (-9223372036854775807L - 1);
        }
      return -((long) magnitude);
    }
  else
    {
      if (magnitude > (9223372036854775807L))
        {
          holy_error (holy_ERR_OUT_OF_RANGE, "overflow is detected");
          return (9223372036854775807L);
        }
      return (long) magnitude;
    }
}

char *holy_strdup (const char *s) __attribute__ ((warn_unused_result));
char *holy_strndup (const char *s, holy_size_t n) __attribute__ ((warn_unused_result));
void *holy_memset (void *s, int c, holy_size_t n);
holy_size_t holy_strlen (const char *s) __attribute__ ((warn_unused_result));
int holy_printf (const char *fmt, ...) __attribute__ ((format (gnu_printf, 1, 2)));
int holy_printf_ (const char *fmt, ...) __attribute__ ((format (gnu_printf, 1, 2)));



static inline char *
holy_strchrsub (char *output, const char *input, char ch, const char *with)
{
  while (*input)
    {
      if (*input == ch)
 {
   holy_strcpy (output, with);
   output += holy_strlen (with);
   input++;
   continue;
 }
      *output++ = *input++;
    }
  *output = '\0';
  return output;
}

extern void (*holy_xputs) (const char *str);

static inline int
holy_puts (const char *s)
{
  const char nl[2] = "\n";
  holy_xputs (s);
  holy_xputs (nl);

  return 1;
}

int holy_puts_ (const char *s);
void holy_real_dprintf (const char *file,
                                     const int line,
                                     const char *condition,
                                     const char *fmt, ...) __attribute__ ((format (gnu_printf, 4, 5)));
int holy_vprintf (const char *fmt, va_list args);
int holy_snprintf (char *str, holy_size_t n, const char *fmt, ...)
     __attribute__ ((format (gnu_printf, 3, 4)));
int holy_vsnprintf (char *str, holy_size_t n, const char *fmt,
     va_list args);
char *holy_xasprintf (const char *fmt, ...)
     __attribute__ ((format (gnu_printf, 1, 2))) __attribute__ ((warn_unused_result));
char *holy_xvasprintf (const char *fmt, va_list args) __attribute__ ((warn_unused_result));
void holy_exit (void) __attribute__ ((noreturn));
holy_uint64_t holy_divmod64 (holy_uint64_t n,
       holy_uint64_t d,
       holy_uint64_t *r);
# 363 "./include/holy/misc.h"
holy_int64_t
holy_divmod64s (holy_int64_t n,
     holy_int64_t d,
     holy_int64_t *r);

holy_uint32_t
holy_divmod32 (holy_uint32_t n,
     holy_uint32_t d,
     holy_uint32_t *r);

holy_int32_t
holy_divmod32s (holy_int32_t n,
      holy_int32_t d,
      holy_int32_t *r);



static inline char *
holy_memchr (const void *p, int c, holy_size_t len)
{
  const char *s = (const char *) p;
  const char *e = s + len;

  for (; s < e; s++)
    if (*s == c)
      return (char *) s;

  return 0;
}


static inline unsigned int
holy_abs (int x)
{
  if (x < 0)
    return (unsigned int) (-x);
  else
    return (unsigned int) x;
}





void holy_reboot (void) __attribute__ ((noreturn));
# 421 "./include/holy/misc.h"
void holy_halt (void) __attribute__ ((noreturn));
# 431 "./include/holy/misc.h"
static inline void
holy_error_save (struct holy_error_saved *save)
{
  holy_memcpy (save->errmsg, holy_errmsg, sizeof (save->errmsg));
  save->holy_errno = holy_errno;
  holy_errno = holy_ERR_NONE;
}

static inline void
holy_error_load (const struct holy_error_saved *save)
{
  holy_memcpy (holy_errmsg, save->errmsg, sizeof (holy_errmsg));
  holy_errno = save->holy_errno;
}
# 16 "./include/holy/dl.h" 2
# 141 "./include/holy/dl.h"
struct holy_dl_segment
{
  struct holy_dl_segment *next;
  void *addr;
  holy_size_t size;
  unsigned section;
};
typedef struct holy_dl_segment *holy_dl_segment_t;

struct holy_dl;

struct holy_dl_dep
{
  struct holy_dl_dep *next;
  struct holy_dl *mod;
};
typedef struct holy_dl_dep *holy_dl_dep_t;
# 184 "./include/holy/dl.h"
typedef struct holy_dl *holy_dl_t;

holy_dl_t holy_dl_load_file (const char *filename);
holy_dl_t holy_dl_load (const char *name);
holy_dl_t holy_dl_load_core (void *addr, holy_size_t size);
holy_dl_t holy_dl_load_core_noinit (void *addr, holy_size_t size);
int holy_dl_unload (holy_dl_t mod);
void holy_dl_unload_unneeded (void);
int holy_dl_ref (holy_dl_t mod);
int holy_dl_unref (holy_dl_t mod);
extern holy_dl_t holy_dl_head;
# 231 "./include/holy/dl.h"
holy_err_t holy_dl_register_symbol (const char *name, void *addr,
        int isfunc, holy_dl_t mod);

holy_err_t holy_arch_dl_check_header (void *ehdr);
# 249 "./include/holy/dl.h"
holy_err_t
holy_ia64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
     holy_size_t *got);
holy_err_t
holy_arm64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
      holy_size_t *got);
# 263 "./include/holy/dl.h"
holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
     holy_size_t *got);
# 10 "./include/holy/partition.h" 2



typedef unsigned long long int holy_uint64_t;
# 68 "./include/holy/partition.h"
struct holy_disk;

typedef struct holy_partition *holy_partition_t;


typedef enum
{
  holy_EMBED_PCBIOS
} holy_embed_type_t;


typedef int (*holy_partition_iterate_hook_t) (struct holy_disk *disk,
           const holy_partition_t partition,
           void *data);


struct holy_partition_map
{

  struct holy_partition_map *next;
  struct holy_partition_map **prev;


  const char *name;


  holy_err_t (*iterate) (struct holy_disk *disk,
    holy_partition_iterate_hook_t hook, void *hook_data);


  holy_err_t (*embed) (struct holy_disk *disk, unsigned int *nsectors,
         unsigned int max_nsectors,
         holy_embed_type_t embed_type,
         holy_disk_addr_t **sectors);

};
typedef struct holy_partition_map *holy_partition_map_t;


struct holy_partition
{

  int number;


  unsigned long long int start;


  unsigned long long int len;


  unsigned long long int offset;


  int index;


  struct holy_partition *parent;


  holy_partition_map_t partmap;



  unsigned char msdostype;
};

holy_partition_t holy_partition_probe (struct holy_disk *disk,
          const char *str);
int holy_partition_iterate (struct holy_disk *disk,
      holy_partition_iterate_hook_t hook,
      void *hook_data);
char *holy_partition_get_name (const holy_partition_t partition);


extern holy_partition_map_t holy_partition_map_list;


static inline void
holy_partition_map_register (holy_partition_map_t partmap)
{
  holy_list_push ((((char *) &(*&holy_partition_map_list)->next == (char *) &((holy_list_t) (*&holy_partition_map_list))->next) && ((char *) &(*&holy_partition_map_list)->prev == (char *) &((holy_list_t) (*&holy_partition_map_list))->prev) ? (holy_list_t *) (void *) &holy_partition_map_list : (holy_list_t *) holy_bad_type_cast_real(149, "util/holy-fstest.c")),
    (((char *) &(partmap)->next == (char *) &((holy_list_t) (partmap))->next) && ((char *) &(partmap)->prev == (char *) &((holy_list_t) (partmap))->prev) ? (holy_list_t) partmap : (holy_list_t) holy_bad_type_cast_real(150, "util/holy-fstest.c")));
}


static inline void
holy_partition_map_unregister (holy_partition_map_t partmap)
{
  holy_list_remove ((((char *) &(partmap)->next == (char *) &((holy_list_t) (partmap))->next) && ((char *) &(partmap)->prev == (char *) &((holy_list_t) (partmap))->prev) ? (holy_list_t) partmap : (holy_list_t) holy_bad_type_cast_real(157, "util/holy-fstest.c")));
}




static inline holy_disk_addr_t
holy_partition_get_start (const holy_partition_t p)
{
  holy_partition_t part;
  holy_uint64_t part_start = 0;

  for (part = p; part; part = part->parent)
    part_start += part->start;

  return part_start;
}

static inline holy_uint64_t
holy_partition_get_len (const holy_partition_t p)
{
  return p->len;
}
# 11 "./include/holy/emu/config.h" 2
# 1 "./include/holy/emu/hostfile.h" 1
# 12 "./include/holy/emu/hostfile.h"
# 1 "./include/holy/osdep/hostfile.h" 1
# 11 "./include/holy/osdep/hostfile.h"
# 1 "./include/holy/osdep/hostfile_unix.h" 1
# 9 "./include/holy/osdep/hostfile_unix.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 10 "./include/holy/osdep/hostfile_unix.h" 2






# 1 "/usr/include/sys/stat.h" 1 3 4
# 99 "/usr/include/sys/stat.h" 3 4


# 1 "/usr/include/bits/stat.h" 1 3 4
# 25 "/usr/include/bits/stat.h" 3 4
# 1 "/usr/include/bits/struct_stat.h" 1 3 4
# 26 "/usr/include/bits/struct_stat.h" 3 4

# 26 "/usr/include/bits/struct_stat.h" 3 4
struct stat
  {



    __dev_t st_dev;




    __ino_t st_ino;







    __nlink_t st_nlink;
    __mode_t st_mode;

    __uid_t st_uid;
    __gid_t st_gid;

    int __pad0;

    __dev_t st_rdev;




    __off_t st_size;



    __blksize_t st_blksize;

    __blkcnt_t st_blocks;
# 74 "/usr/include/bits/struct_stat.h" 3 4
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 89 "/usr/include/bits/struct_stat.h" 3 4
    __syscall_slong_t __glibc_reserved[3];
# 99 "/usr/include/bits/struct_stat.h" 3 4
  };



struct stat64
  {



    __dev_t st_dev;

    __ino64_t st_ino;
    __nlink_t st_nlink;
    __mode_t st_mode;






    __uid_t st_uid;
    __gid_t st_gid;

    int __pad0;
    __dev_t st_rdev;
    __off_t st_size;





    __blksize_t st_blksize;
    __blkcnt64_t st_blocks;







    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
# 151 "/usr/include/bits/struct_stat.h" 3 4
    __syscall_slong_t __glibc_reserved[3];




  };
# 26 "/usr/include/bits/stat.h" 2 3 4
# 102 "/usr/include/sys/stat.h" 2 3 4
# 227 "/usr/include/sys/stat.h" 3 4
extern int stat (const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "stat64") __attribute__ ((__nothrow__ , __leaf__))

     __attribute__ ((__nonnull__ (1, 2)));
extern int fstat (int __fd, struct stat *__buf) __asm__ ("" "fstat64") __attribute__ ((__nothrow__ , __leaf__))
     __attribute__ ((__nonnull__ (2)));
# 240 "/usr/include/sys/stat.h" 3 4
extern int stat64 (const char *__restrict __file,
     struct stat64 *__restrict __buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
extern int fstat64 (int __fd, struct stat64 *__buf) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));
# 279 "/usr/include/sys/stat.h" 3 4
extern int fstatat (int __fd, const char *__restrict __file, struct stat *__restrict __buf, int __flag) __asm__ ("" "fstatat64") __attribute__ ((__nothrow__ , __leaf__))


                 __attribute__ ((__nonnull__ (3)));
# 291 "/usr/include/sys/stat.h" 3 4
extern int fstatat64 (int __fd, const char *__restrict __file,
        struct stat64 *__restrict __buf, int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (3)));
# 327 "/usr/include/sys/stat.h" 3 4
extern int lstat (const char *__restrict __file, struct stat *__restrict __buf) __asm__ ("" "lstat64") __attribute__ ((__nothrow__ , __leaf__))


     __attribute__ ((__nonnull__ (1, 2)));







extern int lstat64 (const char *__restrict __file,
      struct stat64 *__restrict __buf)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));
# 352 "/usr/include/sys/stat.h" 3 4
extern int chmod (const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int lchmod (const char *__file, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));




extern int fchmod (int __fd, __mode_t __mode) __attribute__ ((__nothrow__ , __leaf__));





extern int fchmodat (int __fd, const char *__file, __mode_t __mode,
       int __flag)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2))) ;






extern __mode_t umask (__mode_t __mask) __attribute__ ((__nothrow__ , __leaf__));




extern __mode_t getumask (void) __attribute__ ((__nothrow__ , __leaf__));



extern int mkdir (const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mkdirat (int __fd, const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));






extern int mknod (const char *__path, __mode_t __mode, __dev_t __dev)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mknodat (int __fd, const char *__path, __mode_t __mode,
      __dev_t __dev) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));





extern int mkfifo (const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int mkfifoat (int __fd, const char *__path, __mode_t __mode)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));






extern int utimensat (int __fd, const char *__path,
        const struct timespec __times[2],
        int __flags)
     __attribute__ ((__nothrow__ , __leaf__));
# 452 "/usr/include/sys/stat.h" 3 4
extern int futimens (int __fd, const struct timespec __times[2]) __attribute__ ((__nothrow__ , __leaf__));
# 465 "/usr/include/sys/stat.h" 3 4
# 1 "/usr/include/bits/statx.h" 1 3 4
# 31 "/usr/include/bits/statx.h" 3 4
# 1 "/usr/include/linux/stat.h" 1 3 4




# 1 "/usr/include/linux/types.h" 1 3 4




# 1 "/usr/include/asm/types.h" 1 3 4
# 1 "/usr/include/asm-generic/types.h" 1 3 4






# 1 "/usr/include/asm-generic/int-ll64.h" 1 3 4
# 12 "/usr/include/asm-generic/int-ll64.h" 3 4
# 1 "/usr/include/asm/bitsperlong.h" 1 3 4
# 11 "/usr/include/asm/bitsperlong.h" 3 4
# 1 "/usr/include/asm-generic/bitsperlong.h" 1 3 4
# 12 "/usr/include/asm/bitsperlong.h" 2 3 4
# 13 "/usr/include/asm-generic/int-ll64.h" 2 3 4







typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;


__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
# 8 "/usr/include/asm-generic/types.h" 2 3 4
# 2 "/usr/include/asm/types.h" 2 3 4
# 6 "/usr/include/linux/types.h" 2 3 4



# 1 "/usr/include/linux/posix_types.h" 1 3 4




# 1 "/usr/include/linux/stddef.h" 1 3 4
# 6 "/usr/include/linux/posix_types.h" 2 3 4
# 25 "/usr/include/linux/posix_types.h" 3 4
typedef struct {
 unsigned long fds_bits[1024 / (8 * sizeof(long))];
} __kernel_fd_set;


typedef void (*__kernel_sighandler_t)(int);


typedef int __kernel_key_t;
typedef int __kernel_mqd_t;

# 1 "/usr/include/asm/posix_types.h" 1 3 4






# 1 "/usr/include/asm/posix_types_64.h" 1 3 4
# 11 "/usr/include/asm/posix_types_64.h" 3 4
typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;


typedef unsigned long __kernel_old_dev_t;


# 1 "/usr/include/asm-generic/posix_types.h" 1 3 4
# 15 "/usr/include/asm-generic/posix_types.h" 3 4
typedef long __kernel_long_t;
typedef unsigned long __kernel_ulong_t;



typedef __kernel_ulong_t __kernel_ino_t;



typedef unsigned int __kernel_mode_t;



typedef int __kernel_pid_t;



typedef int __kernel_ipc_pid_t;



typedef unsigned int __kernel_uid_t;
typedef unsigned int __kernel_gid_t;



typedef __kernel_long_t __kernel_suseconds_t;



typedef int __kernel_daddr_t;



typedef unsigned int __kernel_uid32_t;
typedef unsigned int __kernel_gid32_t;
# 72 "/usr/include/asm-generic/posix_types.h" 3 4
typedef __kernel_ulong_t __kernel_size_t;
typedef __kernel_long_t __kernel_ssize_t;
typedef __kernel_long_t __kernel_ptrdiff_t;




typedef struct {
 int val[2];
} __kernel_fsid_t;





typedef __kernel_long_t __kernel_off_t;
typedef long long __kernel_loff_t;
typedef __kernel_long_t __kernel_old_time_t;
typedef __kernel_long_t __kernel_time_t;
typedef long long __kernel_time64_t;
typedef __kernel_long_t __kernel_clock_t;
typedef int __kernel_timer_t;
typedef int __kernel_clockid_t;
typedef char * __kernel_caddr_t;
typedef unsigned short __kernel_uid16_t;
typedef unsigned short __kernel_gid16_t;
# 19 "/usr/include/asm/posix_types_64.h" 2 3 4
# 8 "/usr/include/asm/posix_types.h" 2 3 4
# 37 "/usr/include/linux/posix_types.h" 2 3 4
# 10 "/usr/include/linux/types.h" 2 3 4


typedef __signed__ __int128 __s128 __attribute__((aligned(16)));
typedef unsigned __int128 __u128 __attribute__((aligned(16)));
# 31 "/usr/include/linux/types.h" 3 4
typedef __u16 __le16;
typedef __u16 __be16;
typedef __u32 __le32;
typedef __u32 __be32;
typedef __u64 __le64;
typedef __u64 __be64;

typedef __u16 __sum16;
typedef __u32 __wsum;
# 55 "/usr/include/linux/types.h" 3 4
typedef unsigned __poll_t;
# 6 "/usr/include/linux/stat.h" 2 3 4
# 56 "/usr/include/linux/stat.h" 3 4
struct statx_timestamp {
 __s64 tv_sec;
 __u32 tv_nsec;
 __s32 __reserved;
};
# 99 "/usr/include/linux/stat.h" 3 4
struct statx {


 __u32 stx_mask;


 __u32 stx_blksize;


 __u64 stx_attributes;



 __u32 stx_nlink;


 __u32 stx_uid;


 __u32 stx_gid;


 __u16 stx_mode;
 __u16 __spare0[1];



 __u64 stx_ino;


 __u64 stx_size;


 __u64 stx_blocks;


 __u64 stx_attributes_mask;



 struct statx_timestamp stx_atime;


 struct statx_timestamp stx_btime;


 struct statx_timestamp stx_ctime;


 struct statx_timestamp stx_mtime;



 __u32 stx_rdev_major;
 __u32 stx_rdev_minor;


 __u32 stx_dev_major;
 __u32 stx_dev_minor;


 __u64 stx_mnt_id;


 __u32 stx_dio_mem_align;


 __u32 stx_dio_offset_align;



 __u64 stx_subvol;


 __u32 stx_atomic_write_unit_min;


 __u32 stx_atomic_write_unit_max;



 __u32 stx_atomic_write_segments_max;


 __u32 stx_dio_read_offset_align;


 __u32 stx_atomic_write_unit_max_opt;
 __u32 __spare2[1];


 __u64 __spare3[8];


};
# 32 "/usr/include/bits/statx.h" 2 3 4







# 1 "/usr/include/bits/statx-generic.h" 1 3 4
# 25 "/usr/include/bits/statx-generic.h" 3 4
# 1 "/usr/include/bits/types/struct_statx_timestamp.h" 1 3 4
# 26 "/usr/include/bits/statx-generic.h" 2 3 4
# 1 "/usr/include/bits/types/struct_statx.h" 1 3 4
# 27 "/usr/include/bits/statx-generic.h" 2 3 4
# 62 "/usr/include/bits/statx-generic.h" 3 4



int statx (int __dirfd, const char *__restrict __path, int __flags,
           unsigned int __mask, struct statx *__restrict __buf)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (5)));


# 40 "/usr/include/bits/statx.h" 2 3 4
# 466 "/usr/include/sys/stat.h" 2 3 4



# 17 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/fcntl.h" 1 3 4
# 28 "/usr/include/fcntl.h" 3 4







# 1 "/usr/include/bits/fcntl.h" 1 3 4
# 35 "/usr/include/bits/fcntl.h" 3 4
struct flock
  {
    short int l_type;
    short int l_whence;




    __off64_t l_start;
    __off64_t l_len;

    __pid_t l_pid;
  };


struct flock64
  {
    short int l_type;
    short int l_whence;
    __off64_t l_start;
    __off64_t l_len;
    __pid_t l_pid;
  };



# 1 "/usr/include/bits/fcntl-linux.h" 1 3 4
# 38 "/usr/include/bits/fcntl-linux.h" 3 4
# 1 "/usr/include/bits/types/struct_iovec.h" 1 3 4
# 23 "/usr/include/bits/types/struct_iovec.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 24 "/usr/include/bits/types/struct_iovec.h" 2 3 4


struct iovec
  {
    void *iov_base;
    size_t iov_len;
  };
# 39 "/usr/include/bits/fcntl-linux.h" 2 3 4
# 267 "/usr/include/bits/fcntl-linux.h" 3 4
enum __pid_type
  {
    F_OWNER_TID = 0,
    F_OWNER_PID,
    F_OWNER_PGRP,
    F_OWNER_GID = F_OWNER_PGRP
  };


struct f_owner_ex
  {
    enum __pid_type type;
    __pid_t pid;
  };
# 357 "/usr/include/bits/fcntl-linux.h" 3 4
# 1 "/usr/include/linux/falloc.h" 1 3 4
# 358 "/usr/include/bits/fcntl-linux.h" 2 3 4



struct file_handle
{
  unsigned int handle_bytes;
  int handle_type;

  unsigned char f_handle[0];
};
# 386 "/usr/include/bits/fcntl-linux.h" 3 4





extern __ssize_t readahead (int __fd, __off64_t __offset, size_t __count)
    __attribute__ ((__nothrow__ , __leaf__));






extern int sync_file_range (int __fd, __off64_t __offset, __off64_t __count,
       unsigned int __flags);






extern __ssize_t vmsplice (int __fdout, const struct iovec *__iov,
      size_t __count, unsigned int __flags);





extern __ssize_t splice (int __fdin, __off64_t *__offin, int __fdout,
    __off64_t *__offout, size_t __len,
    unsigned int __flags);





extern __ssize_t tee (int __fdin, int __fdout, size_t __len,
        unsigned int __flags);
# 433 "/usr/include/bits/fcntl-linux.h" 3 4
extern int fallocate (int __fd, int __mode, __off64_t __offset, __off64_t __len) __asm__ ("" "fallocate64")

                     ;





extern int fallocate64 (int __fd, int __mode, __off64_t __offset,
   __off64_t __len);




extern int name_to_handle_at (int __dfd, const char *__name,
         struct file_handle *__handle, int *__mnt_id,
         int __flags) __attribute__ ((__nothrow__ , __leaf__));





extern int open_by_handle_at (int __mountdirfd, struct file_handle *__handle,
         int __flags);




# 62 "/usr/include/bits/fcntl.h" 2 3 4
# 36 "/usr/include/fcntl.h" 2 3 4
# 78 "/usr/include/fcntl.h" 3 4
# 1 "/usr/include/bits/stat.h" 1 3 4
# 79 "/usr/include/fcntl.h" 2 3 4
# 180 "/usr/include/fcntl.h" 3 4
extern int fcntl (int __fd, int __cmd, ...) __asm__ ("" "fcntl64");





extern int fcntl64 (int __fd, int __cmd, ...);
# 212 "/usr/include/fcntl.h" 3 4
extern int open (const char *__file, int __oflag, ...) __asm__ ("" "open64")
     __attribute__ ((__nonnull__ (1)));





extern int open64 (const char *__file, int __oflag, ...) __attribute__ ((__nonnull__ (1)));
# 237 "/usr/include/fcntl.h" 3 4
extern int openat (int __fd, const char *__file, int __oflag, ...) __asm__ ("" "openat64")
                    __attribute__ ((__nonnull__ (2)));





extern int openat64 (int __fd, const char *__file, int __oflag, ...)
     __attribute__ ((__nonnull__ (2)));
# 258 "/usr/include/fcntl.h" 3 4
extern int creat (const char *__file, mode_t __mode) __asm__ ("" "creat64")
                  __attribute__ ((__nonnull__ (1)));





extern int creat64 (const char *__file, mode_t __mode) __attribute__ ((__nonnull__ (1)));
# 306 "/usr/include/fcntl.h" 3 4
extern int posix_fadvise (int __fd, __off64_t __offset, __off64_t __len, int __advise) __asm__ ("" "posix_fadvise64") __attribute__ ((__nothrow__ , __leaf__))

                      ;





extern int posix_fadvise64 (int __fd, off64_t __offset, off64_t __len,
       int __advise) __attribute__ ((__nothrow__ , __leaf__));
# 327 "/usr/include/fcntl.h" 3 4
extern int posix_fallocate (int __fd, __off64_t __offset, __off64_t __len) __asm__ ("" "posix_fallocate64")

                           ;





extern int posix_fallocate64 (int __fd, off64_t __offset, off64_t __len);
# 345 "/usr/include/fcntl.h" 3 4

# 18 "./include/holy/osdep/hostfile_unix.h" 2
# 1 "/usr/include/dirent.h" 1 3 4
# 27 "/usr/include/dirent.h" 3 4

# 61 "/usr/include/dirent.h" 3 4
# 1 "/usr/include/bits/dirent.h" 1 3 4
# 22 "/usr/include/bits/dirent.h" 3 4
struct dirent
  {




    __ino64_t d_ino;
    __off64_t d_off;

    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
  };


struct dirent64
  {
    __ino64_t d_ino;
    __off64_t d_off;
    unsigned short int d_reclen;
    unsigned char d_type;
    char d_name[256];
  };
# 62 "/usr/include/dirent.h" 2 3 4
# 97 "/usr/include/dirent.h" 3 4
enum
  {
    DT_UNKNOWN = 0,

    DT_FIFO = 1,

    DT_CHR = 2,

    DT_DIR = 4,

    DT_BLK = 6,

    DT_REG = 8,

    DT_LNK = 10,

    DT_SOCK = 12,

    DT_WHT = 14

  };
# 127 "/usr/include/dirent.h" 3 4
typedef struct __dirstream DIR;






extern int closedir (DIR *__dirp) __attribute__ ((__nonnull__ (1)));






extern DIR *opendir (const char *__name) __attribute__ ((__nonnull__ (1)))
 __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (closedir, 1)));






extern DIR *fdopendir (int __fd)
 __attribute__ ((__malloc__)) __attribute__ ((__malloc__ (closedir, 1)));
# 167 "/usr/include/dirent.h" 3 4
extern struct dirent *readdir (DIR *__dirp) __asm__ ("" "readdir64")
     __attribute__ ((__nonnull__ (1)));






extern struct dirent64 *readdir64 (DIR *__dirp) __attribute__ ((__nonnull__ (1)));
# 191 "/usr/include/dirent.h" 3 4
extern int readdir_r (DIR *__restrict __dirp, struct dirent *__restrict __entry, struct dirent **__restrict __result) __asm__ ("" "readdir64_r")




  __attribute__ ((__nonnull__ (1, 2, 3))) __attribute__ ((__deprecated__));






extern int readdir64_r (DIR *__restrict __dirp,
   struct dirent64 *__restrict __entry,
   struct dirent64 **__restrict __result)
  __attribute__ ((__nonnull__ (1, 2, 3))) __attribute__ ((__deprecated__));




extern void rewinddir (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern void seekdir (DIR *__dirp, long int __pos) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));


extern long int telldir (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));





extern int dirfd (DIR *__dirp) __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1)));
# 235 "/usr/include/dirent.h" 3 4
# 1 "/usr/include/bits/posix1_lim.h" 1 3 4
# 27 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/wordsize.h" 1 3 4
# 28 "/usr/include/bits/posix1_lim.h" 2 3 4
# 161 "/usr/include/bits/posix1_lim.h" 3 4
# 1 "/usr/include/bits/local_lim.h" 1 3 4
# 38 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/linux/limits.h" 1 3 4
# 39 "/usr/include/bits/local_lim.h" 2 3 4
# 81 "/usr/include/bits/local_lim.h" 3 4
# 1 "/usr/include/bits/pthread_stack_min-dynamic.h" 1 3 4
# 23 "/usr/include/bits/pthread_stack_min-dynamic.h" 3 4

extern long int __sysconf (int __name) __attribute__ ((__nothrow__ , __leaf__));

# 82 "/usr/include/bits/local_lim.h" 2 3 4
# 162 "/usr/include/bits/posix1_lim.h" 2 3 4
# 236 "/usr/include/dirent.h" 2 3 4
# 247 "/usr/include/dirent.h" 3 4
# 1 "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include/stddef.h" 1 3 4
# 248 "/usr/include/dirent.h" 2 3 4
# 265 "/usr/include/dirent.h" 3 4
extern int scandir (const char *__restrict __dir, struct dirent ***__restrict __namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **)) __asm__ ("" "scandir64")





                    __attribute__ ((__nonnull__ (1, 2)));
# 280 "/usr/include/dirent.h" 3 4
extern int scandir64 (const char *__restrict __dir,
        struct dirent64 ***__restrict __namelist,
        int (*__selector) (const struct dirent64 *),
        int (*__cmp) (const struct dirent64 **,
        const struct dirent64 **))
     __attribute__ ((__nonnull__ (1, 2)));
# 303 "/usr/include/dirent.h" 3 4
extern int scandirat (int __dfd, const char *__restrict __dir, struct dirent ***__restrict __namelist, int (*__selector) (const struct dirent *), int (*__cmp) (const struct dirent **, const struct dirent **)) __asm__ ("" "scandirat64")





                      __attribute__ ((__nonnull__ (2, 3)));







extern int scandirat64 (int __dfd, const char *__restrict __dir,
   struct dirent64 ***__restrict __namelist,
   int (*__selector) (const struct dirent64 *),
   int (*__cmp) (const struct dirent64 **,
          const struct dirent64 **))
     __attribute__ ((__nonnull__ (2, 3)));
# 332 "/usr/include/dirent.h" 3 4
extern int alphasort (const struct dirent **__e1, const struct dirent **__e2) __asm__ ("" "alphasort64") __attribute__ ((__nothrow__ , __leaf__))


                   __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int alphasort64 (const struct dirent64 **__e1,
   const struct dirent64 **__e2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));
# 361 "/usr/include/dirent.h" 3 4
extern __ssize_t getdirentries (int __fd, char *__restrict __buf, size_t __nbytes, __off64_t *__restrict __basep) __asm__ ("" "getdirentries64") __attribute__ ((__nothrow__ , __leaf__))



                      __attribute__ ((__nonnull__ (2, 4)));






extern __ssize_t getdirentries64 (int __fd, char *__restrict __buf,
      size_t __nbytes,
      __off64_t *__restrict __basep)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2, 4)));
# 387 "/usr/include/dirent.h" 3 4
extern int versionsort (const struct dirent **__e1, const struct dirent **__e2) __asm__ ("" "versionsort64") __attribute__ ((__nothrow__ , __leaf__))



     __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));






extern int versionsort64 (const struct dirent64 **__e1,
     const struct dirent64 **__e2)
     __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1, 2)));





# 1 "/usr/include/bits/dirent_ext.h" 1 3 4
# 23 "/usr/include/bits/dirent_ext.h" 3 4






extern __ssize_t getdents64 (int __fd, void *__buffer, size_t __length)
  __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (2)));



# 407 "/usr/include/dirent.h" 2 3 4
# 19 "./include/holy/osdep/hostfile_unix.h" 2




# 22 "./include/holy/osdep/hostfile_unix.h"
typedef struct dirent *holy_util_fd_dirent_t;
typedef DIR *holy_util_fd_dir_t;

static inline holy_util_fd_dir_t
holy_util_fd_opendir (const char *name)
{
  return opendir (name);
}

static inline void
holy_util_fd_closedir (holy_util_fd_dir_t dirp)
{
  closedir (dirp);
}

static inline holy_util_fd_dirent_t
holy_util_fd_readdir (holy_util_fd_dir_t dirp)
{
  return readdir (dirp);
}

static inline int
holy_util_unlink (const char *pathname)
{
  return unlink (pathname);
}

static inline int
holy_util_rmdir (const char *pathname)
{
  return rmdir (pathname);
}

static inline int
holy_util_rename (const char *from, const char *to)
{
  return rename (from, to);
}
# 70 "./include/holy/osdep/hostfile_unix.h"
enum holy_util_fd_open_flags_t
  {
    holy_UTIL_FD_O_RDONLY = 
# 72 "./include/holy/osdep/hostfile_unix.h" 3 4
                           00
# 72 "./include/holy/osdep/hostfile_unix.h"
                                   ,
    holy_UTIL_FD_O_WRONLY = 
# 73 "./include/holy/osdep/hostfile_unix.h" 3 4
                           01
# 73 "./include/holy/osdep/hostfile_unix.h"
                                   ,
    holy_UTIL_FD_O_RDWR = 
# 74 "./include/holy/osdep/hostfile_unix.h" 3 4
                         02
# 74 "./include/holy/osdep/hostfile_unix.h"
                               ,
    holy_UTIL_FD_O_CREATTRUNC = 
# 75 "./include/holy/osdep/hostfile_unix.h" 3 4
                               0100 
# 75 "./include/holy/osdep/hostfile_unix.h"
                                       | 
# 75 "./include/holy/osdep/hostfile_unix.h" 3 4
                                         01000
# 75 "./include/holy/osdep/hostfile_unix.h"
                                                ,
    holy_UTIL_FD_O_SYNC = (0

      | 
# 78 "./include/holy/osdep/hostfile_unix.h" 3 4
       04010000


      
# 81 "./include/holy/osdep/hostfile_unix.h"
     | 
# 81 "./include/holy/osdep/hostfile_unix.h" 3 4
       04010000

      
# 83 "./include/holy/osdep/hostfile_unix.h"
     )
  };



typedef int holy_util_fd_t;
# 12 "./include/holy/osdep/hostfile.h" 2
# 13 "./include/holy/emu/hostfile.h" 2

int
holy_util_is_directory (const char *path);
int
holy_util_is_special_file (const char *path);
int
holy_util_is_regular (const char *path);

char *
holy_util_path_concat (size_t n, ...);
char *
holy_util_path_concat_ext (size_t n, ...);

int
holy_util_fd_seek (holy_util_fd_t fd, holy_uint64_t off);
ssize_t
holy_util_fd_read (holy_util_fd_t fd, char *buf, size_t len);
ssize_t
holy_util_fd_write (holy_util_fd_t fd, const char *buf, size_t len);

holy_util_fd_t
holy_util_fd_open (const char *os_dev, int flags);
const char *
holy_util_fd_strerror (void);
void
holy_util_fd_sync (holy_util_fd_t fd);
void
holy_util_disable_fd_syncs (void);
void
holy_util_fd_close (holy_util_fd_t fd);

holy_uint64_t
holy_util_get_fd_size (holy_util_fd_t fd, const char *name, unsigned *log_secsize);
char *
holy_util_make_temporary_file (void);
char *
holy_util_make_temporary_dir (void);
void
holy_util_unlink_recursive (const char *name);
holy_uint32_t
holy_util_get_mtime (const char *name);
# 12 "./include/holy/emu/config.h" 2


const char *
holy_util_get_config_filename (void);
const char *
holy_util_get_pkgdatadir (void);
const char *
holy_util_get_pkglibdir (void);
const char *
holy_util_get_localedir (void);

struct holy_util_config
{
  int is_cryptodisk_enabled;
  char *holy_distributor;
};

void
holy_util_load_config (struct holy_util_config *cfg);

void
holy_util_parse_config (FILE *f, struct holy_util_config *cfg, int simple);
# 10 "./include/holy/types.h" 2
# 63 "./include/holy/types.h"
typedef signed char holy_int8_t;
typedef short holy_int16_t;
typedef int holy_int32_t;

typedef long holy_int64_t;




typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned holy_uint32_t;



typedef unsigned long holy_uint64_t;
# 90 "./include/holy/types.h"
typedef holy_uint64_t holy_addr_t;
typedef holy_uint64_t holy_size_t;
typedef holy_int64_t holy_ssize_t;
# 138 "./include/holy/types.h"
typedef holy_uint64_t holy_properly_aligned_t;




typedef holy_uint64_t holy_off_t;


typedef holy_uint64_t holy_disk_addr_t;


static inline holy_uint16_t holy_swap_bytes16(holy_uint16_t _x)
{
   return (holy_uint16_t) ((_x << 8) | (_x >> 8));
}
# 170 "./include/holy/types.h"
static inline holy_uint32_t holy_swap_bytes32(holy_uint32_t x)
{
 return __builtin_bswap32(x);
}

static inline holy_uint64_t holy_swap_bytes64(holy_uint64_t x)
{
 return __builtin_bswap64(x);
}
# 244 "./include/holy/types.h"
struct holy_unaligned_uint16
{
  holy_uint16_t val;
} __attribute__ ((packed));
struct holy_unaligned_uint32
{
  holy_uint32_t val;
} __attribute__ ((packed));
struct holy_unaligned_uint64
{
  holy_uint64_t val;
} __attribute__ ((packed));

typedef struct holy_unaligned_uint16 holy_unaligned_uint16_t;
typedef struct holy_unaligned_uint32 holy_unaligned_uint32_t;
typedef struct holy_unaligned_uint64 holy_unaligned_uint64_t;

static inline holy_uint16_t holy_get_unaligned16 (const void *ptr)
{
  const struct holy_unaligned_uint16 *dd
    = (const struct holy_unaligned_uint16 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned16 (void *ptr, holy_uint16_t val)
{
  struct holy_unaligned_uint16 *dd = (struct holy_unaligned_uint16 *) ptr;
  dd->val = val;
}

static inline holy_uint32_t holy_get_unaligned32 (const void *ptr)
{
  const struct holy_unaligned_uint32 *dd
    = (const struct holy_unaligned_uint32 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned32 (void *ptr, holy_uint32_t val)
{
  struct holy_unaligned_uint32 *dd = (struct holy_unaligned_uint32 *) ptr;
  dd->val = val;
}

static inline holy_uint64_t holy_get_unaligned64 (const void *ptr)
{
  const struct holy_unaligned_uint64 *dd
    = (const struct holy_unaligned_uint64 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned64 (void *ptr, holy_uint64_t val)
{
  struct holy_unaligned_uint64_t
  {
    holy_uint64_t d;
  } __attribute__ ((packed));
  struct holy_unaligned_uint64_t *dd = (struct holy_unaligned_uint64_t *) ptr;
  dd->d = val;
}
# 17 "./include/holy/util/misc.h" 2

# 1 "./include/holy/emu/misc.h" 1
# 10 "./include/holy/emu/misc.h"
# 1 "./config.h" 1
# 38 "./config.h"
# 1 "./config-util.h" 1
# 39 "./config.h" 2
# 11 "./include/holy/emu/misc.h" 2



# 1 "./include/holy/gcrypt/gcrypt.h" 1





# 1 "./include/holy/gcrypt/gpg-error.h" 1
# 9 "./include/holy/gcrypt/gpg-error.h"
# 1 "./include/holy/crypto.h" 1
# 14 "./include/holy/crypto.h"
typedef enum
  {
    GPG_ERR_NO_ERROR,
    GPG_ERR_BAD_MPI,
    GPG_ERR_BAD_SECKEY,
    GPG_ERR_BAD_SIGNATURE,
    GPG_ERR_CIPHER_ALGO,
    GPG_ERR_CONFLICT,
    GPG_ERR_DECRYPT_FAILED,
    GPG_ERR_DIGEST_ALGO,
    GPG_ERR_GENERAL,
    GPG_ERR_INTERNAL,
    GPG_ERR_INV_ARG,
    GPG_ERR_INV_CIPHER_MODE,
    GPG_ERR_INV_FLAG,
    GPG_ERR_INV_KEYLEN,
    GPG_ERR_INV_OBJ,
    GPG_ERR_INV_OP,
    GPG_ERR_INV_SEXP,
    GPG_ERR_INV_VALUE,
    GPG_ERR_MISSING_VALUE,
    GPG_ERR_NO_ENCRYPTION_SCHEME,
    GPG_ERR_NO_OBJ,
    GPG_ERR_NO_PRIME,
    GPG_ERR_NO_SIGNATURE_SCHEME,
    GPG_ERR_NOT_FOUND,
    GPG_ERR_NOT_IMPLEMENTED,
    GPG_ERR_NOT_SUPPORTED,
    GPG_ERROR_CFLAGS,
    GPG_ERR_PUBKEY_ALGO,
    GPG_ERR_SELFTEST_FAILED,
    GPG_ERR_TOO_SHORT,
    GPG_ERR_UNSUPPORTED,
    GPG_ERR_WEAK_KEY,
    GPG_ERR_WRONG_KEY_USAGE,
    GPG_ERR_WRONG_PUBKEY_ALGO,
    GPG_ERR_OUT_OF_MEMORY,
    GPG_ERR_TOO_LARGE,
    GPG_ERR_ENOMEM
  } gpg_err_code_t;
typedef gpg_err_code_t gpg_error_t;
typedef gpg_error_t gcry_error_t;
typedef gpg_err_code_t gcry_err_code_t;
# 77 "./include/holy/crypto.h"
typedef gcry_err_code_t (*gcry_cipher_setkey_t) (void *c,
       const unsigned char *key,
       unsigned keylen);


typedef void (*gcry_cipher_encrypt_t) (void *c,
           unsigned char *outbuf,
           const unsigned char *inbuf);


typedef void (*gcry_cipher_decrypt_t) (void *c,
           unsigned char *outbuf,
           const unsigned char *inbuf);


typedef void (*gcry_cipher_stencrypt_t) (void *c,
      unsigned char *outbuf,
      const unsigned char *inbuf,
      unsigned int n);


typedef void (*gcry_cipher_stdecrypt_t) (void *c,
      unsigned char *outbuf,
      const unsigned char *inbuf,
      unsigned int n);

typedef struct gcry_cipher_oid_spec
{
  const char *oid;
  int mode;
} gcry_cipher_oid_spec_t;


typedef struct gcry_cipher_spec
{
  const char *name;
  const char **aliases;
  gcry_cipher_oid_spec_t *oids;
  holy_size_t blocksize;
  holy_size_t keylen;
  holy_size_t contextsize;
  gcry_cipher_setkey_t setkey;
  gcry_cipher_encrypt_t encrypt;
  gcry_cipher_decrypt_t decrypt;
  gcry_cipher_stencrypt_t stencrypt;
  gcry_cipher_stdecrypt_t stdecrypt;

  const char *modname;

  struct gcry_cipher_spec *next;
} gcry_cipher_spec_t;


typedef void (*gcry_md_init_t) (void *c);


typedef void (*gcry_md_write_t) (void *c, const void *buf, holy_size_t nbytes);


typedef void (*gcry_md_final_t) (void *c);


typedef unsigned char *(*gcry_md_read_t) (void *c);

typedef struct gcry_md_oid_spec
{
  const char *oidstring;
} gcry_md_oid_spec_t;


typedef struct gcry_md_spec
{
  const char *name;
  unsigned char *asnoid;
  int asnlen;
  gcry_md_oid_spec_t *oids;
  holy_size_t mdlen;
  gcry_md_init_t init;
  gcry_md_write_t write;
  gcry_md_final_t final;
  gcry_md_read_t read;
  holy_size_t contextsize;

  holy_size_t blocksize;

  const char *modname;

  struct gcry_md_spec *next;
} gcry_md_spec_t;

struct gcry_mpi;
typedef struct gcry_mpi *gcry_mpi_t;


typedef gcry_err_code_t (*gcry_pk_generate_t) (int algo,
            unsigned int nbits,
            unsigned long use_e,
            gcry_mpi_t *skey,
            gcry_mpi_t **retfactors);


typedef gcry_err_code_t (*gcry_pk_check_secret_key_t) (int algo,
             gcry_mpi_t *skey);


typedef gcry_err_code_t (*gcry_pk_encrypt_t) (int algo,
           gcry_mpi_t *resarr,
           gcry_mpi_t data,
           gcry_mpi_t *pkey,
           int flags);


typedef gcry_err_code_t (*gcry_pk_decrypt_t) (int algo,
           gcry_mpi_t *result,
           gcry_mpi_t *data,
           gcry_mpi_t *skey,
           int flags);


typedef gcry_err_code_t (*gcry_pk_sign_t) (int algo,
        gcry_mpi_t *resarr,
        gcry_mpi_t data,
        gcry_mpi_t *skey);


typedef gcry_err_code_t (*gcry_pk_verify_t) (int algo,
          gcry_mpi_t hash,
          gcry_mpi_t *data,
          gcry_mpi_t *pkey,
          int (*cmp) (void *, gcry_mpi_t),
          void *opaquev);


typedef unsigned (*gcry_pk_get_nbits_t) (int algo, gcry_mpi_t *pkey);


typedef struct gcry_pk_spec
{
  const char *name;
  const char **aliases;
  const char *elements_pkey;
  const char *elements_skey;
  const char *elements_enc;
  const char *elements_sig;
  const char *elements_grip;
  int use;
  gcry_pk_generate_t generate;
  gcry_pk_check_secret_key_t check_secret_key;
  gcry_pk_encrypt_t encrypt;
  gcry_pk_decrypt_t decrypt;
  gcry_pk_sign_t sign;
  gcry_pk_verify_t verify;
  gcry_pk_get_nbits_t get_nbits;

  const char *modname;

} gcry_pk_spec_t;

struct holy_crypto_cipher_handle
{
  const struct gcry_cipher_spec *cipher;
  char ctx[0];
};

typedef struct holy_crypto_cipher_handle *holy_crypto_cipher_handle_t;

struct holy_crypto_hmac_handle;

const gcry_cipher_spec_t *
holy_crypto_lookup_cipher_by_name (const char *name);

holy_crypto_cipher_handle_t
holy_crypto_cipher_open (const struct gcry_cipher_spec *cipher);

gcry_err_code_t
holy_crypto_cipher_set_key (holy_crypto_cipher_handle_t cipher,
       const unsigned char *key,
       unsigned keylen);

static inline void
holy_crypto_cipher_close (holy_crypto_cipher_handle_t cipher)
{
  holy_free (cipher);
}

static inline void
holy_crypto_xor (void *out, const void *in1, const void *in2, holy_size_t size)
{
  const holy_uint8_t *in1ptr = in1, *in2ptr = in2;
  holy_uint8_t *outptr = out;
  while (size && (((holy_addr_t) in1ptr & (sizeof (holy_uint64_t) - 1))
    || ((holy_addr_t) in2ptr & (sizeof (holy_uint64_t) - 1))
    || ((holy_addr_t) outptr & (sizeof (holy_uint64_t) - 1))))
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
      size--;
    }
  while (size >= sizeof (holy_uint64_t))
    {

      *(holy_uint64_t *) (void *) outptr
 = (*(const holy_uint64_t *) (const void *) in1ptr
    ^ *(const holy_uint64_t *) (const void *) in2ptr);
      in1ptr += sizeof (holy_uint64_t);
      in2ptr += sizeof (holy_uint64_t);
      outptr += sizeof (holy_uint64_t);
      size -= sizeof (holy_uint64_t);
    }
  while (size)
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
      size--;
    }
}

gcry_err_code_t
holy_crypto_ecb_decrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size);

gcry_err_code_t
holy_crypto_ecb_encrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size);
gcry_err_code_t
holy_crypto_cbc_encrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size,
    void *iv_in);
gcry_err_code_t
holy_crypto_cbc_decrypt (holy_crypto_cipher_handle_t cipher,
    void *out, const void *in, holy_size_t size,
    void *iv);
void
holy_cipher_register (gcry_cipher_spec_t *cipher);
void
holy_cipher_unregister (gcry_cipher_spec_t *cipher);
void
holy_md_register (gcry_md_spec_t *digest);
void
holy_md_unregister (gcry_md_spec_t *cipher);

extern struct gcry_pk_spec *holy_crypto_pk_dsa;
extern struct gcry_pk_spec *holy_crypto_pk_ecdsa;
extern struct gcry_pk_spec *holy_crypto_pk_ecdh;
extern struct gcry_pk_spec *holy_crypto_pk_rsa;

void
holy_crypto_hash (const gcry_md_spec_t *hash, void *out, const void *in,
    holy_size_t inlen);
const gcry_md_spec_t *
holy_crypto_lookup_md_by_name (const char *name);

holy_err_t
holy_crypto_gcry_error (gcry_err_code_t in);

void holy_burn_stack (holy_size_t size);

struct holy_crypto_hmac_handle *
holy_crypto_hmac_init (const struct gcry_md_spec *md,
         const void *key, holy_size_t keylen);
void
holy_crypto_hmac_write (struct holy_crypto_hmac_handle *hnd,
   const void *data,
   holy_size_t datalen);
gcry_err_code_t
holy_crypto_hmac_fini (struct holy_crypto_hmac_handle *hnd, void *out);

gcry_err_code_t
holy_crypto_hmac_buffer (const struct gcry_md_spec *md,
    const void *key, holy_size_t keylen,
    const void *data, holy_size_t datalen, void *out);

extern gcry_md_spec_t _gcry_digest_spec_md5;
extern gcry_md_spec_t _gcry_digest_spec_sha1;
extern gcry_md_spec_t _gcry_digest_spec_sha256;
extern gcry_md_spec_t _gcry_digest_spec_sha512;
extern gcry_md_spec_t _gcry_digest_spec_crc32;
extern gcry_cipher_spec_t _gcry_cipher_spec_aes;
# 372 "./include/holy/crypto.h"
gcry_err_code_t
holy_crypto_pbkdf2 (const struct gcry_md_spec *md,
      const holy_uint8_t *P, holy_size_t Plen,
      const holy_uint8_t *S, holy_size_t Slen,
      unsigned int c,
      holy_uint8_t *DK, holy_size_t dkLen);

int
holy_crypto_memcmp (const void *a, const void *b, holy_size_t n);

int
holy_password_get (char buf[], unsigned buf_size);




extern void (*holy_crypto_autoload_hook) (const char *name);

void _gcry_assert_failed (const char *expr, const char *file, int line,
                          const char *func) __attribute__ ((noreturn));

void _gcry_burn_stack (int bytes);
void _gcry_log_error( const char *fmt, ... ) __attribute__ ((format (__printf__, 1, 2)));



void holy_gcry_init_all (void);
void holy_gcry_fini_all (void);

int
holy_get_random (void *out, holy_size_t len);
# 10 "./include/holy/gcrypt/gpg-error.h" 2
typedef enum
  {
    GPG_ERR_SOURCE_USER_1
  }
  gpg_err_source_t;

static inline int
gpg_err_make (gpg_err_source_t source __attribute__ ((unused)), gpg_err_code_t code)
{
  return code;
}

static inline gpg_err_code_t
gpg_err_code (gpg_error_t err)
{
  return err;
}

static inline gpg_err_source_t
gpg_err_source (gpg_error_t err __attribute__ ((unused)))
{
  return GPG_ERR_SOURCE_USER_1;
}

gcry_err_code_t
gpg_error_from_syserror (void);
# 7 "./include/holy/gcrypt/gcrypt.h" 2
# 90 "./include/holy/gcrypt/gcrypt.h"
typedef gpg_err_source_t gcry_err_source_t;

static inline gcry_err_code_t
gcry_err_make (gcry_err_source_t source, gcry_err_code_t code)
{
  return gpg_err_make (source, code);
}







static inline gcry_err_code_t
gcry_error (gcry_err_code_t code)
{
  return gcry_err_make (GPG_ERR_SOURCE_USER_1, code);
}

static inline gcry_err_code_t
gcry_err_code (gcry_err_code_t err)
{
  return gpg_err_code (err);
}


static inline gcry_err_source_t
gcry_err_source (gcry_err_code_t err)
{
  return gpg_err_source (err);
}



const char *gcry_strerror (gcry_err_code_t err);



const char *gcry_strsource (gcry_err_code_t err);




gcry_err_code_t gcry_err_code_from_errno (int err);



int gcry_err_code_to_errno (gcry_err_code_t code);



gcry_err_code_t gcry_err_make_from_errno (gcry_err_source_t source, int err);


gcry_err_code_t gcry_error_from_errno (int err);



struct gcry_mpi;
# 159 "./include/holy/gcrypt/gcrypt.h"
const char *gcry_check_version (const char *req_version);




enum gcry_ctl_cmds
  {
    GCRYCTL_SET_KEY = 1,
    GCRYCTL_SET_IV = 2,
    GCRYCTL_CFB_SYNC = 3,
    GCRYCTL_RESET = 4,
    GCRYCTL_FINALIZE = 5,
    GCRYCTL_GET_KEYLEN = 6,
    GCRYCTL_GET_BLKLEN = 7,
    GCRYCTL_TEST_ALGO = 8,
    GCRYCTL_IS_SECURE = 9,
    GCRYCTL_GET_ASNOID = 10,
    GCRYCTL_ENABLE_ALGO = 11,
    GCRYCTL_DISABLE_ALGO = 12,
    GCRYCTL_DUMP_RANDOM_STATS = 13,
    GCRYCTL_DUMP_SECMEM_STATS = 14,
    GCRYCTL_GET_ALGO_NPKEY = 15,
    GCRYCTL_GET_ALGO_NSKEY = 16,
    GCRYCTL_GET_ALGO_NSIGN = 17,
    GCRYCTL_GET_ALGO_NENCR = 18,
    GCRYCTL_SET_VERBOSITY = 19,
    GCRYCTL_SET_DEBUG_FLAGS = 20,
    GCRYCTL_CLEAR_DEBUG_FLAGS = 21,
    GCRYCTL_USE_SECURE_RNDPOOL= 22,
    GCRYCTL_DUMP_MEMORY_STATS = 23,
    GCRYCTL_INIT_SECMEM = 24,
    GCRYCTL_TERM_SECMEM = 25,
    GCRYCTL_DISABLE_SECMEM_WARN = 27,
    GCRYCTL_SUSPEND_SECMEM_WARN = 28,
    GCRYCTL_RESUME_SECMEM_WARN = 29,
    GCRYCTL_DROP_PRIVS = 30,
    GCRYCTL_ENABLE_M_GUARD = 31,
    GCRYCTL_START_DUMP = 32,
    GCRYCTL_STOP_DUMP = 33,
    GCRYCTL_GET_ALGO_USAGE = 34,
    GCRYCTL_IS_ALGO_ENABLED = 35,
    GCRYCTL_DISABLE_INTERNAL_LOCKING = 36,
    GCRYCTL_DISABLE_SECMEM = 37,
    GCRYCTL_INITIALIZATION_FINISHED = 38,
    GCRYCTL_INITIALIZATION_FINISHED_P = 39,
    GCRYCTL_ANY_INITIALIZATION_P = 40,
    GCRYCTL_SET_CBC_CTS = 41,
    GCRYCTL_SET_CBC_MAC = 42,
    GCRYCTL_SET_CTR = 43,
    GCRYCTL_ENABLE_QUICK_RANDOM = 44,
    GCRYCTL_SET_RANDOM_SEED_FILE = 45,
    GCRYCTL_UPDATE_RANDOM_SEED_FILE = 46,
    GCRYCTL_SET_THREAD_CBS = 47,
    GCRYCTL_FAST_POLL = 48,
    GCRYCTL_SET_RANDOM_DAEMON_SOCKET = 49,
    GCRYCTL_USE_RANDOM_DAEMON = 50,
    GCRYCTL_FAKED_RANDOM_P = 51,
    GCRYCTL_SET_RNDEGD_SOCKET = 52,
    GCRYCTL_PRINT_CONFIG = 53,
    GCRYCTL_OPERATIONAL_P = 54,
    GCRYCTL_FIPS_MODE_P = 55,
    GCRYCTL_FORCE_FIPS_MODE = 56,
    GCRYCTL_SELFTEST = 57,

    GCRYCTL_DISABLE_HWF = 63,
    GCRYCTL_SET_ENFORCED_FIPS_FLAG = 64
  };


gcry_err_code_t gcry_control (enum gcry_ctl_cmds CMD, ...);






struct gcry_sexp;
typedef struct gcry_sexp *gcry_sexp_t;







enum gcry_sexp_format
  {
    GCRYSEXP_FMT_DEFAULT = 0,
    GCRYSEXP_FMT_CANON = 1,
    GCRYSEXP_FMT_BASE64 = 2,
    GCRYSEXP_FMT_ADVANCED = 3
  };




gcry_err_code_t gcry_sexp_new (gcry_sexp_t *retsexp,
                            const void *buffer, size_t length,
                            int autodetect);



gcry_err_code_t gcry_sexp_create (gcry_sexp_t *retsexp,
                               void *buffer, size_t length,
                               int autodetect, void (*freefnc) (void *));



gcry_err_code_t gcry_sexp_sscan (gcry_sexp_t *retsexp, size_t *erroff,
                              const char *buffer, size_t length);



gcry_err_code_t gcry_sexp_build (gcry_sexp_t *retsexp, size_t *erroff,
                              const char *format, ...);



gcry_err_code_t gcry_sexp_build_array (gcry_sexp_t *retsexp, size_t *erroff,
        const char *format, void **arg_list);


void gcry_sexp_release (gcry_sexp_t sexp);



size_t gcry_sexp_canon_len (const unsigned char *buffer, size_t length,
                            size_t *erroff, gcry_err_code_t *errcode);



size_t gcry_sexp_sprint (gcry_sexp_t sexp, int mode, void *buffer,
                         size_t maxlength);



void gcry_sexp_dump (const gcry_sexp_t a);

gcry_sexp_t gcry_sexp_cons (const gcry_sexp_t a, const gcry_sexp_t b);
gcry_sexp_t gcry_sexp_alist (const gcry_sexp_t *array);
gcry_sexp_t gcry_sexp_vlist (const gcry_sexp_t a, ...);
gcry_sexp_t gcry_sexp_append (const gcry_sexp_t a, const gcry_sexp_t n);
gcry_sexp_t gcry_sexp_prepend (const gcry_sexp_t a, const gcry_sexp_t n);






gcry_sexp_t gcry_sexp_find_token (gcry_sexp_t list,
                                const char *tok, size_t toklen);


int gcry_sexp_length (const gcry_sexp_t list);




gcry_sexp_t gcry_sexp_nth (const gcry_sexp_t list, int number);




gcry_sexp_t gcry_sexp_car (const gcry_sexp_t list);






gcry_sexp_t gcry_sexp_cdr (const gcry_sexp_t list);

gcry_sexp_t gcry_sexp_cadr (const gcry_sexp_t list);
# 340 "./include/holy/gcrypt/gcrypt.h"
const char *gcry_sexp_nth_data (const gcry_sexp_t list, int number,
                                size_t *datalen);






char *gcry_sexp_nth_string (gcry_sexp_t list, int number);







gcry_mpi_t gcry_sexp_nth_mpi (gcry_sexp_t list, int number, int mpifmt);
# 367 "./include/holy/gcrypt/gcrypt.h"
enum gcry_mpi_format
  {
    GCRYMPI_FMT_NONE= 0,
    GCRYMPI_FMT_STD = 1,
    GCRYMPI_FMT_PGP = 2,
    GCRYMPI_FMT_SSH = 3,
    GCRYMPI_FMT_HEX = 4,
    GCRYMPI_FMT_USG = 5
  };


enum gcry_mpi_flag
  {
    GCRYMPI_FLAG_SECURE = 1,
    GCRYMPI_FLAG_OPAQUE = 2


  };




gcry_mpi_t gcry_mpi_new (unsigned int nbits);


gcry_mpi_t gcry_mpi_snew (unsigned int nbits);


void gcry_mpi_release (gcry_mpi_t a);


gcry_mpi_t gcry_mpi_copy (const gcry_mpi_t a);


gcry_mpi_t gcry_mpi_set (gcry_mpi_t w, const gcry_mpi_t u);


gcry_mpi_t gcry_mpi_set_ui (gcry_mpi_t w, unsigned long u);


void gcry_mpi_swap (gcry_mpi_t a, gcry_mpi_t b);



int gcry_mpi_cmp (const gcry_mpi_t u, const gcry_mpi_t v);




int gcry_mpi_cmp_ui (const gcry_mpi_t u, unsigned long v);





gcry_err_code_t gcry_mpi_scan (gcry_mpi_t *ret_mpi, enum gcry_mpi_format format,
                            const void *buffer, size_t buflen,
                            size_t *nscanned);






gcry_err_code_t gcry_mpi_print (enum gcry_mpi_format format,
                             unsigned char *buffer, size_t buflen,
                             size_t *nwritten,
                             const gcry_mpi_t a);





gcry_err_code_t gcry_mpi_aprint (enum gcry_mpi_format format,
                              unsigned char **buffer, size_t *nwritten,
                              const gcry_mpi_t a);





void gcry_mpi_dump (const gcry_mpi_t a);



void gcry_mpi_add (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_add_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v);


void gcry_mpi_addm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_sub (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_sub_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v );


void gcry_mpi_subm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_mul (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v);


void gcry_mpi_mul_ui (gcry_mpi_t w, gcry_mpi_t u, unsigned long v );


void gcry_mpi_mulm (gcry_mpi_t w, gcry_mpi_t u, gcry_mpi_t v, gcry_mpi_t m);


void gcry_mpi_mul_2exp (gcry_mpi_t w, gcry_mpi_t u, unsigned long cnt);



void gcry_mpi_div (gcry_mpi_t q, gcry_mpi_t r,
                   gcry_mpi_t dividend, gcry_mpi_t divisor, int round);


void gcry_mpi_mod (gcry_mpi_t r, gcry_mpi_t dividend, gcry_mpi_t divisor);


void gcry_mpi_powm (gcry_mpi_t w,
                    const gcry_mpi_t b, const gcry_mpi_t e,
                    const gcry_mpi_t m);



int gcry_mpi_gcd (gcry_mpi_t g, gcry_mpi_t a, gcry_mpi_t b);



int gcry_mpi_invm (gcry_mpi_t x, gcry_mpi_t a, gcry_mpi_t m);



unsigned int gcry_mpi_get_nbits (gcry_mpi_t a);


int gcry_mpi_test_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_set_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_clear_bit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_set_highbit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_clear_highbit (gcry_mpi_t a, unsigned int n);


void gcry_mpi_rshift (gcry_mpi_t x, gcry_mpi_t a, unsigned int n);


void gcry_mpi_lshift (gcry_mpi_t x, gcry_mpi_t a, unsigned int n);




gcry_mpi_t gcry_mpi_set_opaque (gcry_mpi_t a, void *p, unsigned int nbits);




void *gcry_mpi_get_opaque (gcry_mpi_t a, unsigned int *nbits);




void gcry_mpi_set_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);



void gcry_mpi_clear_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);


int gcry_mpi_get_flag (gcry_mpi_t a, enum gcry_mpi_flag flag);
# 607 "./include/holy/gcrypt/gcrypt.h"
struct gcry_cipher_handle;
typedef struct gcry_cipher_handle *gcry_cipher_hd_t;
# 617 "./include/holy/gcrypt/gcrypt.h"
enum gcry_cipher_algos
  {
    GCRY_CIPHER_NONE = 0,
    GCRY_CIPHER_IDEA = 1,
    GCRY_CIPHER_3DES = 2,
    GCRY_CIPHER_CAST5 = 3,
    GCRY_CIPHER_BLOWFISH = 4,
    GCRY_CIPHER_SAFER_SK128 = 5,
    GCRY_CIPHER_DES_SK = 6,
    GCRY_CIPHER_AES = 7,
    GCRY_CIPHER_AES192 = 8,
    GCRY_CIPHER_AES256 = 9,
    GCRY_CIPHER_TWOFISH = 10,


    GCRY_CIPHER_ARCFOUR = 301,
    GCRY_CIPHER_DES = 302,
    GCRY_CIPHER_TWOFISH128 = 303,
    GCRY_CIPHER_SERPENT128 = 304,
    GCRY_CIPHER_SERPENT192 = 305,
    GCRY_CIPHER_SERPENT256 = 306,
    GCRY_CIPHER_RFC2268_40 = 307,
    GCRY_CIPHER_RFC2268_128 = 308,
    GCRY_CIPHER_SEED = 309,
    GCRY_CIPHER_CAMELLIA128 = 310,
    GCRY_CIPHER_CAMELLIA192 = 311,
    GCRY_CIPHER_CAMELLIA256 = 312
  };
# 655 "./include/holy/gcrypt/gcrypt.h"
enum gcry_cipher_modes
  {
    GCRY_CIPHER_MODE_NONE = 0,
    GCRY_CIPHER_MODE_ECB = 1,
    GCRY_CIPHER_MODE_CFB = 2,
    GCRY_CIPHER_MODE_CBC = 3,
    GCRY_CIPHER_MODE_STREAM = 4,
    GCRY_CIPHER_MODE_OFB = 5,
    GCRY_CIPHER_MODE_CTR = 6,
    GCRY_CIPHER_MODE_AESWRAP= 7
  };


enum gcry_cipher_flags
  {
    GCRY_CIPHER_SECURE = 1,
    GCRY_CIPHER_ENABLE_SYNC = 2,
    GCRY_CIPHER_CBC_CTS = 4,
    GCRY_CIPHER_CBC_MAC = 8
  };




gcry_err_code_t gcry_cipher_open (gcry_cipher_hd_t *handle,
                              int algo, int mode, unsigned int flags);


void gcry_cipher_close (gcry_cipher_hd_t h);


gcry_err_code_t gcry_cipher_ctl (gcry_cipher_hd_t h, int cmd, void *buffer,
                             size_t buflen);


gcry_err_code_t gcry_cipher_info (gcry_cipher_hd_t h, int what, void *buffer,
                              size_t *nbytes);


gcry_err_code_t gcry_cipher_algo_info (int algo, int what, void *buffer,
                                   size_t *nbytes);




const char *gcry_cipher_algo_name (int algorithm) __attribute__ ((__pure__));



int gcry_cipher_map_name (const char *name) __attribute__ ((__pure__));




int gcry_cipher_mode_from_oid (const char *string) __attribute__ ((__pure__));





gcry_err_code_t gcry_cipher_encrypt (gcry_cipher_hd_t h,
                                  void *out, size_t outsize,
                                  const void *in, size_t inlen);


gcry_err_code_t gcry_cipher_decrypt (gcry_cipher_hd_t h,
                                  void *out, size_t outsize,
                                  const void *in, size_t inlen);


gcry_err_code_t gcry_cipher_setkey (gcry_cipher_hd_t hd,
                                 const void *key, size_t keylen);



gcry_err_code_t gcry_cipher_setiv (gcry_cipher_hd_t hd,
                                const void *iv, size_t ivlen);
# 747 "./include/holy/gcrypt/gcrypt.h"
gpg_error_t gcry_cipher_setctr (gcry_cipher_hd_t hd,
                                const void *ctr, size_t ctrlen);


size_t gcry_cipher_get_algo_keylen (int algo);


size_t gcry_cipher_get_algo_blklen (int algo);
# 766 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_cipher_list (int *list, int *list_length);
# 776 "./include/holy/gcrypt/gcrypt.h"
enum gcry_pk_algos
  {
    GCRY_PK_RSA = 1,
    GCRY_PK_RSA_E = 2,
    GCRY_PK_RSA_S = 3,
    GCRY_PK_ELG_E = 16,
    GCRY_PK_DSA = 17,
    GCRY_PK_ELG = 20,
    GCRY_PK_ECDSA = 301,
    GCRY_PK_ECDH = 302
  };
# 797 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_pk_encrypt (gcry_sexp_t *result,
                              gcry_sexp_t data, gcry_sexp_t pkey);



gcry_err_code_t gcry_pk_decrypt (gcry_sexp_t *result,
                              gcry_sexp_t data, gcry_sexp_t skey);



gcry_err_code_t gcry_pk_sign (gcry_sexp_t *result,
                           gcry_sexp_t data, gcry_sexp_t skey);


gcry_err_code_t gcry_pk_verify (gcry_sexp_t sigval,
                             gcry_sexp_t data, gcry_sexp_t pkey);


gcry_err_code_t gcry_pk_testkey (gcry_sexp_t key);




gcry_err_code_t gcry_pk_genkey (gcry_sexp_t *r_key, gcry_sexp_t s_parms);


gcry_err_code_t gcry_pk_ctl (int cmd, void *buffer, size_t buflen);


gcry_err_code_t gcry_pk_algo_info (int algo, int what,
                                void *buffer, size_t *nbytes);




const char *gcry_pk_algo_name (int algorithm) __attribute__ ((__pure__));



int gcry_pk_map_name (const char* name) __attribute__ ((__pure__));



unsigned int gcry_pk_get_nbits (gcry_sexp_t key) __attribute__ ((__pure__));



unsigned char *gcry_pk_get_keygrip (gcry_sexp_t key, unsigned char *array);


const char *gcry_pk_get_curve (gcry_sexp_t key, int iterator,
                               unsigned int *r_nbits);



gcry_sexp_t gcry_pk_get_param (int algo, const char *name);
# 864 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_pk_list (int *list, int *list_length);
# 876 "./include/holy/gcrypt/gcrypt.h"
enum gcry_md_algos
  {
    GCRY_MD_NONE = 0,
    GCRY_MD_MD5 = 1,
    GCRY_MD_SHA1 = 2,
    GCRY_MD_RMD160 = 3,
    GCRY_MD_MD2 = 5,
    GCRY_MD_TIGER = 6,
    GCRY_MD_HAVAL = 7,
    GCRY_MD_SHA256 = 8,
    GCRY_MD_SHA384 = 9,
    GCRY_MD_SHA512 = 10,
    GCRY_MD_SHA224 = 11,
    GCRY_MD_MD4 = 301,
    GCRY_MD_CRC32 = 302,
    GCRY_MD_CRC32_RFC1510 = 303,
    GCRY_MD_CRC24_RFC2440 = 304,
    GCRY_MD_WHIRLPOOL = 305,
    GCRY_MD_TIGER1 = 306,
    GCRY_MD_TIGER2 = 307
  };


enum gcry_md_flags
  {
    GCRY_MD_FLAG_SECURE = 1,
    GCRY_MD_FLAG_HMAC = 2
  };


struct gcry_md_context;




typedef struct gcry_md_handle
{

  struct gcry_md_context *ctx;


  int bufpos;
  int bufsize;
  unsigned char buf[1];
} *gcry_md_hd_t;
# 932 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_md_open (gcry_md_hd_t *h, int algo, unsigned int flags);


void gcry_md_close (gcry_md_hd_t hd);


gcry_err_code_t gcry_md_enable (gcry_md_hd_t hd, int algo);


gcry_err_code_t gcry_md_copy (gcry_md_hd_t *bhd, gcry_md_hd_t ahd);


void gcry_md_reset (gcry_md_hd_t hd);


gcry_err_code_t gcry_md_ctl (gcry_md_hd_t hd, int cmd,
                          void *buffer, size_t buflen);




void gcry_md_write (gcry_md_hd_t hd, const void *buffer, size_t length);



unsigned char *gcry_md_read (gcry_md_hd_t hd, int algo);






void gcry_md_hash_buffer (int algo, void *digest,
                          const void *buffer, size_t length);



int gcry_md_get_algo (gcry_md_hd_t hd);



unsigned int gcry_md_get_algo_dlen (int algo);



int gcry_md_is_enabled (gcry_md_hd_t a, int algo);


int gcry_md_is_secure (gcry_md_hd_t a);


gcry_err_code_t gcry_md_info (gcry_md_hd_t h, int what, void *buffer,
                          size_t *nbytes);


gcry_err_code_t gcry_md_algo_info (int algo, int what, void *buffer,
                               size_t *nbytes);




const char *gcry_md_algo_name (int algo) __attribute__ ((__pure__));



int gcry_md_map_name (const char* name) __attribute__ ((__pure__));



gcry_err_code_t gcry_md_setkey (gcry_md_hd_t hd, const void *key, size_t keylen);




void gcry_md_debug (gcry_md_hd_t hd, const char *suffix);
# 1055 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_md_list (int *list, int *list_length);
# 1476 "./include/holy/gcrypt/gcrypt.h"
enum gcry_kdf_algos
  {
    GCRY_KDF_NONE = 0,
    GCRY_KDF_SIMPLE_S2K = 16,
    GCRY_KDF_SALTED_S2K = 17,
    GCRY_KDF_ITERSALTED_S2K = 19,
    GCRY_KDF_PBKDF1 = 33,
    GCRY_KDF_PBKDF2 = 34
  };


gpg_error_t gcry_kdf_derive (const void *passphrase, size_t passphraselen,
                             int algo, int subalgo,
                             const void *salt, size_t saltlen,
                             unsigned long iterations,
                             size_t keysize, void *keybuffer);
# 1506 "./include/holy/gcrypt/gcrypt.h"
typedef enum gcry_random_level
  {
    GCRY_WEAK_RANDOM = 0,
    GCRY_STRONG_RANDOM = 1,
    GCRY_VERY_STRONG_RANDOM = 2
  }
gcry_random_level_t;



void gcry_randomize (void *buffer, size_t length,
                     enum gcry_random_level level);




gcry_err_code_t gcry_random_add_bytes (const void *buffer, size_t length,
                                    int quality);
# 1533 "./include/holy/gcrypt/gcrypt.h"
void *gcry_random_bytes (size_t nbytes, enum gcry_random_level level)
                         __attribute__ ((__malloc__));




void *gcry_random_bytes_secure (size_t nbytes, enum gcry_random_level level)
                                __attribute__ ((__malloc__));





void gcry_mpi_randomize (gcry_mpi_t w,
                         unsigned int nbits, enum gcry_random_level level);



void gcry_create_nonce (void *buffer, size_t length);
# 1570 "./include/holy/gcrypt/gcrypt.h"
typedef int (*gcry_prime_check_func_t) (void *arg, int mode,
                                        gcry_mpi_t candidate);
# 1588 "./include/holy/gcrypt/gcrypt.h"
gcry_err_code_t gcry_prime_generate (gcry_mpi_t *prime,
                                  unsigned int prime_bits,
                                  unsigned int factor_bits,
                                  gcry_mpi_t **factors,
                                  gcry_prime_check_func_t cb_func,
                                  void *cb_arg,
                                  gcry_random_level_t random_level,
                                  unsigned int flags);





gcry_err_code_t gcry_prime_group_generator (gcry_mpi_t *r_g,
                                         gcry_mpi_t prime,
                                         gcry_mpi_t *factors,
                                         gcry_mpi_t start_g);



void gcry_prime_release_factors (gcry_mpi_t *factors);



gcry_err_code_t gcry_prime_check (gcry_mpi_t x, unsigned int flags);
# 1623 "./include/holy/gcrypt/gcrypt.h"
enum gcry_log_levels
  {
    GCRY_LOG_CONT = 0,
    GCRY_LOG_INFO = 10,
    GCRY_LOG_WARN = 20,
    GCRY_LOG_ERROR = 30,
    GCRY_LOG_FATAL = 40,
    GCRY_LOG_BUG = 50,
    GCRY_LOG_DEBUG = 100
  };


typedef void (*gcry_handler_progress_t) (void *, const char *, int, int, int);


typedef void *(*gcry_handler_alloc_t) (size_t n);


typedef int (*gcry_handler_secure_check_t) (const void *);


typedef void *(*gcry_handler_realloc_t) (void *p, size_t n);


typedef void (*gcry_handler_free_t) (void *);


typedef int (*gcry_handler_no_mem_t) (void *, size_t, unsigned int);


typedef void (*gcry_handler_error_t) (void *, int, const char *);


typedef void (*gcry_handler_log_t) (void *, int, const char *, va_list);



void gcry_set_progress_handler (gcry_handler_progress_t cb, void *cb_data);



void gcry_set_allocation_handler (
                             gcry_handler_alloc_t func_alloc,
                             gcry_handler_alloc_t func_alloc_secure,
                             gcry_handler_secure_check_t func_secure_check,
                             gcry_handler_realloc_t func_realloc,
                             gcry_handler_free_t func_free);



void gcry_set_outofcore_handler (gcry_handler_no_mem_t h, void *opaque);



void gcry_set_fatalerror_handler (gcry_handler_error_t fnc, void *opaque);



void gcry_set_log_handler (gcry_handler_log_t f, void *opaque);


void gcry_set_gettext_handler (const char *(*f)(const char*));



void *gcry_malloc (size_t n) __attribute__ ((__malloc__));
void *gcry_calloc (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_malloc_secure (size_t n) __attribute__ ((__malloc__));
void *gcry_calloc_secure (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_realloc (void *a, size_t n);
char *gcry_strdup (const char *string) __attribute__ ((__malloc__));
void *gcry_xmalloc (size_t n) __attribute__ ((__malloc__));
void *gcry_xcalloc (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_xmalloc_secure (size_t n) __attribute__ ((__malloc__));
void *gcry_xcalloc_secure (size_t n, size_t m) __attribute__ ((__malloc__));
void *gcry_xrealloc (void *a, size_t n);
char *gcry_xstrdup (const char * a) __attribute__ ((__malloc__));
void gcry_free (void *a);


int gcry_is_secure (const void *a) __attribute__ ((__pure__));






# 1 "./holy-core/lib/libgcrypt-holy/src/gcrypt-module.h" 1
# 1711 "./include/holy/gcrypt/gcrypt.h" 2
# 15 "./include/holy/emu/misc.h" 2



# 1 "./include/holy/util/misc.h" 1
# 19 "./include/holy/emu/misc.h" 2

extern int verbosity;
extern const char *program_name;


extern const unsigned long long int holy_size_t;
void holy_init_all (void);
void holy_fini_all (void);

void holy_find_zpool_from_dir (const char *dir,
          char **poolname, char **poolfs);

char *holy_make_system_path_relative_to_its_root (const char *path)
 __attribute__ ((warn_unused_result));
int
holy_util_device_is_mapped (const char *dev);
# 44 "./include/holy/emu/misc.h"
void * xmalloc (holy_size_t size) __attribute__ ((warn_unused_result));
void * xrealloc (void *ptr, holy_size_t size) __attribute__ ((warn_unused_result));
char * xstrdup (const char *str) __attribute__ ((warn_unused_result));
char * xasprintf (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2))) __attribute__ ((warn_unused_result));

void holy_util_warn (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void holy_util_info (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void holy_util_error (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2), noreturn));

holy_uint64_t holy_util_get_cpu_time_ms (void);


int holy_device_mapper_supported (void);





FILE *
holy_util_fopen (const char *path, const char *mode);


void holy_util_file_sync (FILE *f);
# 19 "./include/holy/util/misc.h" 2

char *holy_util_get_path (const char *dir, const char *file);
size_t holy_util_get_image_size (const char *path);
char *holy_util_read_image (const char *path);
void holy_util_load_image (const char *path, char *buf);
void holy_util_write_image (const char *img, size_t size, FILE *out,
       const char *name);
void holy_util_write_image_at (const void *img, size_t size, off_t offset,
          FILE *out, const char *name);

char *make_system_path_relative_to_its_root (const char *path);

char *holy_canonicalize_file_name (const char *path);

void holy_util_init_nls (void);

void holy_util_host_init (int *argc, char ***argv);

int holy_qsort_strcmp (const void *, const void *);
# 10 "holy-core/osdep/basic/init.c" 2


# 1 "./holy-core/gnulib/progname.h" 1
# 20 "./holy-core/gnulib/progname.h"
extern const char *program_name;




extern void set_program_name (const char *argv0);
# 13 "holy-core/osdep/basic/init.c" 2

void
holy_util_host_init (int *argc __attribute__ ((unused)),
       char ***argv)
{
  set_program_name ((*argv)[0]);


  setlocale (
# 21 "holy-core/osdep/basic/init.c" 3 4
            6
# 21 "holy-core/osdep/basic/init.c"
                  , "");
  bindtextdomain ("holy", "/usr/local/share/locale");
  textdomain ("holy");

}
# 10 "holy-core/osdep/init.c" 2
