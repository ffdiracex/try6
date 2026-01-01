/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NTFS_H
#define holy_NTFS_H	1

enum
  {
    holy_NTFS_FILE_MFT     =  0,
    holy_NTFS_FILE_MFTMIRR =  1,
    holy_NTFS_FILE_LOGFILE =  2,
    holy_NTFS_FILE_VOLUME  =  3,
    holy_NTFS_FILE_ATTRDEF =  4,
    holy_NTFS_FILE_ROOT    =  5,
    holy_NTFS_FILE_BITMAP  =  6,
    holy_NTFS_FILE_BOOT    =  7,
    holy_NTFS_FILE_BADCLUS =  8,
    holy_NTFS_FILE_QUOTA   =  9,
    holy_NTFS_FILE_UPCASE  = 10,
  };

enum
  {
    holy_NTFS_AT_STANDARD_INFORMATION = 0x10,
    holy_NTFS_AT_ATTRIBUTE_LIST       = 0x20,
    holy_NTFS_AT_FILENAME             = 0x30,
    holy_NTFS_AT_OBJECT_ID            = 0x40,
    holy_NTFS_AT_SECURITY_DESCRIPTOR  = 0x50,
    holy_NTFS_AT_VOLUME_NAME          = 0x60,
    holy_NTFS_AT_VOLUME_INFORMATION   = 0x70,
    holy_NTFS_AT_DATA                 = 0x80,
    holy_NTFS_AT_INDEX_ROOT           = 0x90,
    holy_NTFS_AT_INDEX_ALLOCATION     = 0xA0,
    holy_NTFS_AT_BITMAP               = 0xB0,
    holy_NTFS_AT_SYMLINK              = 0xC0,
    holy_NTFS_AT_EA_INFORMATION	      = 0xD0,
    holy_NTFS_AT_EA                   = 0xE0,
  };

enum
  {
    holy_NTFS_ATTR_READ_ONLY   = 0x1,
    holy_NTFS_ATTR_HIDDEN      = 0x2,
    holy_NTFS_ATTR_SYSTEM      = 0x4,
    holy_NTFS_ATTR_ARCHIVE     = 0x20,
    holy_NTFS_ATTR_DEVICE      = 0x40,
    holy_NTFS_ATTR_NORMAL      = 0x80,
    holy_NTFS_ATTR_TEMPORARY   = 0x100,
    holy_NTFS_ATTR_SPARSE      = 0x200,
    holy_NTFS_ATTR_REPARSE     = 0x400,
    holy_NTFS_ATTR_COMPRESSED  = 0x800,
    holy_NTFS_ATTR_OFFLINE     = 0x1000,
    holy_NTFS_ATTR_NOT_INDEXED = 0x2000,
    holy_NTFS_ATTR_ENCRYPTED   = 0x4000,
    holy_NTFS_ATTR_DIRECTORY   = 0x10000000,
    holy_NTFS_ATTR_INDEX_VIEW  = 0x20000000
  };

enum
  {
    holy_NTFS_FLAG_COMPRESSED		= 1,
    holy_NTFS_FLAG_ENCRYPTED		= 0x4000,
    holy_NTFS_FLAG_SPARSE		= 0x8000
  };

#define holy_NTFS_BLK_SHR		holy_DISK_SECTOR_BITS

#define holy_NTFS_MAX_MFT		(4096 >> holy_NTFS_BLK_SHR)
#define holy_NTFS_MAX_IDX		(16384 >> holy_NTFS_BLK_SHR)

#define holy_NTFS_COM_LEN		4096
#define holy_NTFS_COM_LOG_LEN	12
#define holy_NTFS_COM_SEC		(holy_NTFS_COM_LEN >> holy_NTFS_BLK_SHR)
#define holy_NTFS_LOG_COM_SEC		(holy_NTFS_COM_LOG_LEN - holy_NTFS_BLK_SHR)

enum
  {
    holy_NTFS_AF_ALST		= 1,
    holy_NTFS_AF_MMFT		= 2,
    holy_NTFS_AF_GPOS		= 4,
  };

enum
  {
    holy_NTFS_RF_BLNK		= 1
  };

struct holy_ntfs_bpb
{
  holy_uint8_t jmp_boot[3];
  holy_uint8_t oem_name[8];
  holy_uint16_t bytes_per_sector;
  holy_uint8_t sectors_per_cluster;
  holy_uint8_t reserved_1[7];
  holy_uint8_t media;
  holy_uint16_t reserved_2;
  holy_uint16_t sectors_per_track;
  holy_uint16_t num_heads;
  holy_uint32_t num_hidden_sectors;
  holy_uint32_t reserved_3;
  holy_uint8_t bios_drive;
  holy_uint8_t reserved_4[3];
  holy_uint64_t num_total_sectors;
  holy_uint64_t mft_lcn;
  holy_uint64_t mft_mirr_lcn;
  holy_int8_t clusters_per_mft;
  holy_int8_t reserved_5[3];
  holy_int8_t clusters_per_index;
  holy_int8_t reserved_6[3];
  holy_uint64_t num_serial;
  holy_uint32_t checksum;
} holy_PACKED;

struct holy_ntfs_attr
{
  int flags;
  holy_uint8_t *emft_buf, *edat_buf;
  holy_uint8_t *attr_cur, *attr_nxt, *attr_end;
  holy_uint32_t save_pos;
  holy_uint8_t *sbuf;
  struct holy_ntfs_file *mft;
};

struct holy_ntfs_file
{
  struct holy_ntfs_data *data;
  holy_uint8_t *buf;
  holy_uint64_t size;
  holy_uint64_t mtime;
  holy_uint64_t ino;
  int inode_read;
  struct holy_ntfs_attr attr;
};

struct holy_ntfs_data
{
  struct holy_ntfs_file cmft;
  struct holy_ntfs_file mmft;
  holy_disk_t disk;
  holy_uint64_t mft_size;
  holy_uint64_t idx_size;
  int log_spc;
  holy_uint64_t mft_start;
  holy_uint64_t uuid;
};

struct holy_ntfs_comp_table_element
{
  holy_uint32_t next_vcn;
  holy_uint32_t next_lcn;
};

struct holy_ntfs_comp
{
  holy_disk_t disk;
  int comp_head, comp_tail;
  struct holy_ntfs_comp_table_element comp_table[16];
  holy_uint32_t cbuf_ofs, cbuf_vcn;
  int log_spc;
  holy_uint8_t *cbuf;
};

struct holy_ntfs_rlst
{
  int flags;
  holy_disk_addr_t target_vcn, curr_vcn, next_vcn, curr_lcn;
  holy_uint8_t *cur_run;
  struct holy_ntfs_attr *attr;
  struct holy_ntfs_comp comp;
  void *file;
};

typedef holy_err_t (*holy_ntfscomp_func_t) (holy_uint8_t *dest,
					    holy_disk_addr_t ofs,
					    holy_size_t len,
					    struct holy_ntfs_rlst * ctx);

extern holy_ntfscomp_func_t holy_ntfscomp_func;

holy_err_t holy_ntfs_read_run_list (struct holy_ntfs_rlst *ctx);

#endif /* ! holy_NTFS_H */
