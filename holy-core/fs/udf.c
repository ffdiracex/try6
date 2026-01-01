/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/types.h>
#include <holy/fshelp.h>
#include <holy/charset.h>
#include <holy/datetime.h>
#include <holy/udf.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_UDF_MAX_PDS		2
#define holy_UDF_MAX_PMS		6

#define U16				holy_le_to_cpu16
#define U32				holy_le_to_cpu32
#define U64				holy_le_to_cpu64

#define holy_UDF_TAG_IDENT_PVD		0x0001
#define holy_UDF_TAG_IDENT_AVDP		0x0002
#define holy_UDF_TAG_IDENT_VDP		0x0003
#define holy_UDF_TAG_IDENT_IUVD		0x0004
#define holy_UDF_TAG_IDENT_PD		0x0005
#define holy_UDF_TAG_IDENT_LVD		0x0006
#define holy_UDF_TAG_IDENT_USD		0x0007
#define holy_UDF_TAG_IDENT_TD		0x0008
#define holy_UDF_TAG_IDENT_LVID		0x0009

#define holy_UDF_TAG_IDENT_FSD		0x0100
#define holy_UDF_TAG_IDENT_FID		0x0101
#define holy_UDF_TAG_IDENT_AED		0x0102
#define holy_UDF_TAG_IDENT_IE		0x0103
#define holy_UDF_TAG_IDENT_TE		0x0104
#define holy_UDF_TAG_IDENT_FE		0x0105
#define holy_UDF_TAG_IDENT_EAHD		0x0106
#define holy_UDF_TAG_IDENT_USE		0x0107
#define holy_UDF_TAG_IDENT_SBD		0x0108
#define holy_UDF_TAG_IDENT_PIE		0x0109
#define holy_UDF_TAG_IDENT_EFE		0x010A

#define holy_UDF_ICBTAG_TYPE_UNDEF	0x00
#define holy_UDF_ICBTAG_TYPE_USE	0x01
#define holy_UDF_ICBTAG_TYPE_PIE	0x02
#define holy_UDF_ICBTAG_TYPE_IE		0x03
#define holy_UDF_ICBTAG_TYPE_DIRECTORY	0x04
#define holy_UDF_ICBTAG_TYPE_REGULAR	0x05
#define holy_UDF_ICBTAG_TYPE_BLOCK	0x06
#define holy_UDF_ICBTAG_TYPE_CHAR	0x07
#define holy_UDF_ICBTAG_TYPE_EA		0x08
#define holy_UDF_ICBTAG_TYPE_FIFO	0x09
#define holy_UDF_ICBTAG_TYPE_SOCKET	0x0A
#define holy_UDF_ICBTAG_TYPE_TE		0x0B
#define holy_UDF_ICBTAG_TYPE_SYMLINK	0x0C
#define holy_UDF_ICBTAG_TYPE_STREAMDIR	0x0D

#define holy_UDF_ICBTAG_FLAG_AD_MASK	0x0007
#define holy_UDF_ICBTAG_FLAG_AD_SHORT	0x0000
#define holy_UDF_ICBTAG_FLAG_AD_LONG	0x0001
#define holy_UDF_ICBTAG_FLAG_AD_EXT	0x0002
#define holy_UDF_ICBTAG_FLAG_AD_IN_ICB	0x0003

#define holy_UDF_EXT_NORMAL		0x00000000
#define holy_UDF_EXT_NREC_ALLOC		0x40000000
#define holy_UDF_EXT_NREC_NALLOC	0x80000000
#define holy_UDF_EXT_MASK		0xC0000000

#define holy_UDF_FID_CHAR_HIDDEN	0x01
#define holy_UDF_FID_CHAR_DIRECTORY	0x02
#define holy_UDF_FID_CHAR_DELETED	0x04
#define holy_UDF_FID_CHAR_PARENT	0x08
#define holy_UDF_FID_CHAR_METADATA	0x10

#define holy_UDF_STD_IDENT_BEA01	"BEA01"
#define holy_UDF_STD_IDENT_BOOT2	"BOOT2"
#define holy_UDF_STD_IDENT_CD001	"CD001"
#define holy_UDF_STD_IDENT_CDW02	"CDW02"
#define holy_UDF_STD_IDENT_NSR02	"NSR02"
#define holy_UDF_STD_IDENT_NSR03	"NSR03"
#define holy_UDF_STD_IDENT_TEA01	"TEA01"

#define holy_UDF_CHARSPEC_TYPE_CS0	0x00
#define holy_UDF_CHARSPEC_TYPE_CS1	0x01
#define holy_UDF_CHARSPEC_TYPE_CS2	0x02
#define holy_UDF_CHARSPEC_TYPE_CS3	0x03
#define holy_UDF_CHARSPEC_TYPE_CS4	0x04
#define holy_UDF_CHARSPEC_TYPE_CS5	0x05
#define holy_UDF_CHARSPEC_TYPE_CS6	0x06
#define holy_UDF_CHARSPEC_TYPE_CS7	0x07
#define holy_UDF_CHARSPEC_TYPE_CS8	0x08

#define holy_UDF_PARTMAP_TYPE_1		1
#define holy_UDF_PARTMAP_TYPE_2		2

struct holy_udf_lb_addr
{
  holy_uint32_t block_num;
  holy_uint16_t part_ref;
} holy_PACKED;

struct holy_udf_short_ad
{
  holy_uint32_t length;
  holy_uint32_t position;
} holy_PACKED;

struct holy_udf_long_ad
{
  holy_uint32_t length;
  struct holy_udf_lb_addr block;
  holy_uint8_t imp_use[6];
} holy_PACKED;

struct holy_udf_extent_ad
{
  holy_uint32_t length;
  holy_uint32_t start;
} holy_PACKED;

struct holy_udf_charspec
{
  holy_uint8_t charset_type;
  holy_uint8_t charset_info[63];
} holy_PACKED;

struct holy_udf_timestamp
{
  holy_uint16_t type_and_timezone;
  holy_uint16_t year;
  holy_uint8_t month;
  holy_uint8_t day;
  holy_uint8_t hour;
  holy_uint8_t minute;
  holy_uint8_t second;
  holy_uint8_t centi_seconds;
  holy_uint8_t hundreds_of_micro_seconds;
  holy_uint8_t micro_seconds;
} holy_PACKED;

struct holy_udf_regid
{
  holy_uint8_t flags;
  holy_uint8_t ident[23];
  holy_uint8_t ident_suffix[8];
} holy_PACKED;

struct holy_udf_tag
{
  holy_uint16_t tag_ident;
  holy_uint16_t desc_version;
  holy_uint8_t tag_checksum;
  holy_uint8_t reserved;
  holy_uint16_t tag_serial_number;
  holy_uint16_t desc_crc;
  holy_uint16_t desc_crc_length;
  holy_uint32_t tag_location;
} holy_PACKED;

struct holy_udf_fileset
{
  struct holy_udf_tag tag;
  struct holy_udf_timestamp datetime;
  holy_uint16_t interchange_level;
  holy_uint16_t max_interchange_level;
  holy_uint32_t charset_list;
  holy_uint32_t max_charset_list;
  holy_uint32_t fileset_num;
  holy_uint32_t fileset_desc_num;
  struct holy_udf_charspec vol_charset;
  holy_uint8_t vol_ident[128];
  struct holy_udf_charspec fileset_charset;
  holy_uint8_t fileset_ident[32];
  holy_uint8_t copyright_file_ident[32];
  holy_uint8_t abstract_file_ident[32];
  struct holy_udf_long_ad root_icb;
  struct holy_udf_regid domain_ident;
  struct holy_udf_long_ad next_ext;
  struct holy_udf_long_ad streamdir_icb;
} holy_PACKED;

struct holy_udf_icbtag
{
  holy_uint32_t prior_recorded_num_direct_entries;
  holy_uint16_t strategy_type;
  holy_uint16_t strategy_parameter;
  holy_uint16_t num_entries;
  holy_uint8_t reserved;
  holy_uint8_t file_type;
  struct holy_udf_lb_addr parent_idb;
  holy_uint16_t flags;
} holy_PACKED;

struct holy_udf_file_ident
{
  struct holy_udf_tag tag;
  holy_uint16_t version_num;
  holy_uint8_t characteristics;
#define MAX_FILE_IDENT_LENGTH 256
  holy_uint8_t file_ident_length;
  struct holy_udf_long_ad icb;
  holy_uint16_t imp_use_length;
} holy_PACKED;

struct holy_udf_file_entry
{
  struct holy_udf_tag tag;
  struct holy_udf_icbtag icbtag;
  holy_uint32_t uid;
  holy_uint32_t gid;
  holy_uint32_t permissions;
  holy_uint16_t link_count;
  holy_uint8_t record_format;
  holy_uint8_t record_display_attr;
  holy_uint32_t record_length;
  holy_uint64_t file_size;
  holy_uint64_t blocks_recorded;
  struct holy_udf_timestamp access_time;
  struct holy_udf_timestamp modification_time;
  struct holy_udf_timestamp attr_time;
  holy_uint32_t checkpoint;
  struct holy_udf_long_ad extended_attr_idb;
  struct holy_udf_regid imp_ident;
  holy_uint64_t unique_id;
  holy_uint32_t ext_attr_length;
  holy_uint32_t alloc_descs_length;
  holy_uint8_t ext_attr[0];
} holy_PACKED;

struct holy_udf_extended_file_entry
{
  struct holy_udf_tag tag;
  struct holy_udf_icbtag icbtag;
  holy_uint32_t uid;
  holy_uint32_t gid;
  holy_uint32_t permissions;
  holy_uint16_t link_count;
  holy_uint8_t record_format;
  holy_uint8_t record_display_attr;
  holy_uint32_t record_length;
  holy_uint64_t file_size;
  holy_uint64_t object_size;
  holy_uint64_t blocks_recorded;
  struct holy_udf_timestamp access_time;
  struct holy_udf_timestamp modification_time;
  struct holy_udf_timestamp create_time;
  struct holy_udf_timestamp attr_time;
  holy_uint32_t checkpoint;
  holy_uint32_t reserved;
  struct holy_udf_long_ad extended_attr_icb;
  struct holy_udf_long_ad streamdir_icb;
  struct holy_udf_regid imp_ident;
  holy_uint64_t unique_id;
  holy_uint32_t ext_attr_length;
  holy_uint32_t alloc_descs_length;
  holy_uint8_t ext_attr[0];
} holy_PACKED;

struct holy_udf_vrs
{
  holy_uint8_t type;
  holy_uint8_t magic[5];
  holy_uint8_t version;
} holy_PACKED;

struct holy_udf_avdp
{
  struct holy_udf_tag tag;
  struct holy_udf_extent_ad vds;
} holy_PACKED;

struct holy_udf_pd
{
  struct holy_udf_tag tag;
  holy_uint32_t seq_num;
  holy_uint16_t flags;
  holy_uint16_t part_num;
  struct holy_udf_regid contents;
  holy_uint8_t contents_use[128];
  holy_uint32_t access_type;
  holy_uint32_t start;
  holy_uint32_t length;
} holy_PACKED;

struct holy_udf_partmap
{
  holy_uint8_t type;
  holy_uint8_t length;
  union
  {
    struct
    {
      holy_uint16_t seq_num;
      holy_uint16_t part_num;
    } type1;

    struct
    {
      holy_uint8_t ident[62];
    } type2;
  };
} holy_PACKED;

struct holy_udf_lvd
{
  struct holy_udf_tag tag;
  holy_uint32_t seq_num;
  struct holy_udf_charspec charset;
  holy_uint8_t ident[128];
  holy_uint32_t bsize;
  struct holy_udf_regid domain_ident;
  struct holy_udf_long_ad root_fileset;
  holy_uint32_t map_table_length;
  holy_uint32_t num_part_maps;
  struct holy_udf_regid imp_ident;
  holy_uint8_t imp_use[128];
  struct holy_udf_extent_ad integrity_seq_ext;
  holy_uint8_t part_maps[1608];
} holy_PACKED;

struct holy_udf_aed
{
  struct holy_udf_tag tag;
  holy_uint32_t prev_ae;
  holy_uint32_t ae_len;
} holy_PACKED;

struct holy_udf_data
{
  holy_disk_t disk;
  struct holy_udf_lvd lvd;
  struct holy_udf_pd pds[holy_UDF_MAX_PDS];
  struct holy_udf_partmap *pms[holy_UDF_MAX_PMS];
  struct holy_udf_long_ad root_icb;
  int npd, npm, lbshift;
};

struct holy_fshelp_node
{
  struct holy_udf_data *data;
  int part_ref;
  union
  {
    struct holy_udf_file_entry fe;
    struct holy_udf_extended_file_entry efe;
    char raw[0];
  } block;
};

static inline holy_size_t
get_fshelp_size (struct holy_udf_data *data)
{
  struct holy_fshelp_node *x = NULL;
  return sizeof (*x)
    + (1 << (holy_DISK_SECTOR_BITS
	     + data->lbshift)) - sizeof (x->block);
}

static holy_dl_t my_mod;

static holy_uint32_t
holy_udf_get_block (struct holy_udf_data *data,
		    holy_uint16_t part_ref, holy_uint32_t block)
{
  part_ref = U16 (part_ref);

  if (part_ref >= data->npm)
    {
      holy_error (holy_ERR_BAD_FS, "invalid part ref");
      return 0;
    }

  return (U32 (data->pds[data->pms[part_ref]->type1.part_num].start)
          + U32 (block));
}

static holy_err_t
holy_udf_read_icb (struct holy_udf_data *data,
		   struct holy_udf_long_ad *icb,
		   struct holy_fshelp_node *node)
{
  holy_uint32_t block;

  block = holy_udf_get_block (data,
			      icb->block.part_ref,
                              icb->block.block_num);

  if (holy_errno)
    return holy_errno;

  if (holy_disk_read (data->disk, block << data->lbshift, 0,
		      1 << (holy_DISK_SECTOR_BITS
			    + data->lbshift),
		      &node->block))
    return holy_errno;

  if ((U16 (node->block.fe.tag.tag_ident) != holy_UDF_TAG_IDENT_FE) &&
      (U16 (node->block.fe.tag.tag_ident) != holy_UDF_TAG_IDENT_EFE))
    return holy_error (holy_ERR_BAD_FS, "invalid fe/efe descriptor");

  node->part_ref = icb->block.part_ref;
  node->data = data;
  return 0;
}

static holy_disk_addr_t
holy_udf_read_block (holy_fshelp_node_t node, holy_disk_addr_t fileblock)
{
  char *buf = NULL;
  char *ptr;
  holy_ssize_t len;
  holy_disk_addr_t filebytes;

  switch (U16 (node->block.fe.tag.tag_ident))
    {
    case holy_UDF_TAG_IDENT_FE:
      ptr = (char *) &node->block.fe.ext_attr[0] + U32 (node->block.fe.ext_attr_length);
      len = U32 (node->block.fe.alloc_descs_length);
      break;

    case holy_UDF_TAG_IDENT_EFE:
      ptr = (char *) &node->block.efe.ext_attr[0] + U32 (node->block.efe.ext_attr_length);
      len = U32 (node->block.efe.alloc_descs_length);
      break;

    default:
      holy_error (holy_ERR_BAD_FS, "invalid file entry");
      return 0;
    }

  if ((U16 (node->block.fe.icbtag.flags) & holy_UDF_ICBTAG_FLAG_AD_MASK)
      == holy_UDF_ICBTAG_FLAG_AD_SHORT)
    {
      struct holy_udf_short_ad *ad = (struct holy_udf_short_ad *) ptr;

      filebytes = fileblock * U32 (node->data->lvd.bsize);
      while (len >= (holy_ssize_t) sizeof (struct holy_udf_short_ad))
	{
	  holy_uint32_t adlen = U32 (ad->length) & 0x3fffffff;
	  holy_uint32_t adtype = U32 (ad->length) >> 30;
	  if (adtype == 3)
	    {
	      struct holy_udf_aed *extension;
	      holy_disk_addr_t sec = holy_udf_get_block(node->data,
							node->part_ref,
							ad->position);
	      if (!buf)
		{
		  buf = holy_malloc (U32 (node->data->lvd.bsize));
		  if (!buf)
		    return 0;
		}
	      if (holy_disk_read (node->data->disk, sec << node->data->lbshift,
				  0, adlen, buf))
		goto fail;

	      extension = (struct holy_udf_aed *) buf;
	      if (U16 (extension->tag.tag_ident) != holy_UDF_TAG_IDENT_AED)
		{
		  holy_error (holy_ERR_BAD_FS, "invalid aed tag");
		  goto fail;
		}

	      len = U32 (extension->ae_len);
	      ad = (struct holy_udf_short_ad *)
		    (buf + sizeof (struct holy_udf_aed));
	      continue;
	    }

	  if (filebytes < adlen)
	    {
	      holy_uint32_t ad_pos = ad->position;
	      holy_free (buf);
	      return ((U32 (ad_pos) & holy_UDF_EXT_MASK) ? 0 :
		      (holy_udf_get_block (node->data, node->part_ref, ad_pos)
		       + (filebytes >> (holy_DISK_SECTOR_BITS
					+ node->data->lbshift))));
	    }

	  filebytes -= adlen;
	  ad++;
	  len -= sizeof (struct holy_udf_short_ad);
	}
    }
  else
    {
      struct holy_udf_long_ad *ad = (struct holy_udf_long_ad *) ptr;

      filebytes = fileblock * U32 (node->data->lvd.bsize);
      while (len >= (holy_ssize_t) sizeof (struct holy_udf_long_ad))
	{
	  holy_uint32_t adlen = U32 (ad->length) & 0x3fffffff;
	  holy_uint32_t adtype = U32 (ad->length) >> 30;
	  if (adtype == 3)
	    {
	      struct holy_udf_aed *extension;
	      holy_disk_addr_t sec = holy_udf_get_block(node->data,
							ad->block.part_ref,
							ad->block.block_num);
	      if (!buf)
		{
		  buf = holy_malloc (U32 (node->data->lvd.bsize));
		  if (!buf)
		    return 0;
		}
	      if (holy_disk_read (node->data->disk, sec << node->data->lbshift,
				  0, adlen, buf))
		goto fail;

	      extension = (struct holy_udf_aed *) buf;
	      if (U16 (extension->tag.tag_ident) != holy_UDF_TAG_IDENT_AED)
		{
		  holy_error (holy_ERR_BAD_FS, "invalid aed tag");
		  goto fail;
		}

	      len = U32 (extension->ae_len);
	      ad = (struct holy_udf_long_ad *)
		    (buf + sizeof (struct holy_udf_aed));
	      continue;
	    }
	      
	  if (filebytes < adlen)
	    {
	      holy_uint32_t ad_block_num = ad->block.block_num;
	      holy_uint32_t ad_part_ref = ad->block.part_ref;
	      holy_free (buf);
	      return ((U32 (ad_block_num) & holy_UDF_EXT_MASK) ?  0 :
		      (holy_udf_get_block (node->data, ad_part_ref,
					   ad_block_num)
		       + (filebytes >> (holy_DISK_SECTOR_BITS
				        + node->data->lbshift))));
	    }

	  filebytes -= adlen;
	  ad++;
	  len -= sizeof (struct holy_udf_long_ad);
	}
    }

fail:
  holy_free (buf);

  return 0;
}

static holy_ssize_t
holy_udf_read_file (holy_fshelp_node_t node,
		    holy_disk_read_hook_t read_hook, void *read_hook_data,
		    holy_off_t pos, holy_size_t len, char *buf)
{
  switch (U16 (node->block.fe.icbtag.flags) & holy_UDF_ICBTAG_FLAG_AD_MASK)
    {
    case holy_UDF_ICBTAG_FLAG_AD_IN_ICB:
      {
	char *ptr;

	ptr = ((U16 (node->block.fe.tag.tag_ident) == holy_UDF_TAG_IDENT_FE) ?
	       ((char *) &node->block.fe.ext_attr[0]
                + U32 (node->block.fe.ext_attr_length)) :
	       ((char *) &node->block.efe.ext_attr[0]
                + U32 (node->block.efe.ext_attr_length)));

	holy_memcpy (buf, ptr + pos, len);

	return len;
      }

    case holy_UDF_ICBTAG_FLAG_AD_EXT:
      holy_error (holy_ERR_BAD_FS, "invalid extent type");
      return 0;
    }

  return holy_fshelp_read_file (node->data->disk, node,
				read_hook, read_hook_data,
				pos, len, buf, holy_udf_read_block,
				U64 (node->block.fe.file_size),
				node->data->lbshift, 0);
}

static unsigned sblocklist[] = { 256, 512, 0 };

static struct holy_udf_data *
holy_udf_mount (holy_disk_t disk)
{
  struct holy_udf_data *data = 0;
  struct holy_udf_fileset root_fs;
  unsigned *sblklist;
  holy_uint32_t block, vblock;
  int i, lbshift;

  data = holy_malloc (sizeof (struct holy_udf_data));
  if (!data)
    return 0;

  data->disk = disk;

  /* Search for Anchor Volume Descriptor Pointer (AVDP)
   * and determine logical block size.  */
  block = 0;
  for (lbshift = 0; lbshift < 4; lbshift++)
    {
      for (sblklist = sblocklist; *sblklist; sblklist++)
        {
	  struct holy_udf_avdp avdp;

	  if (holy_disk_read (disk, *sblklist << lbshift, 0,
			      sizeof (struct holy_udf_avdp), &avdp))
	    {
	      holy_error (holy_ERR_BAD_FS, "not an UDF filesystem");
	      goto fail;
	    }

	  if (U16 (avdp.tag.tag_ident) == holy_UDF_TAG_IDENT_AVDP &&
	      U32 (avdp.tag.tag_location) == *sblklist)
	    {
	      block = U32 (avdp.vds.start);
	      break;
	    }
	}

      if (block)
	break;
    }

  if (!block)
    {
      holy_error (holy_ERR_BAD_FS, "not an UDF filesystem");
      goto fail;
    }
  data->lbshift = lbshift;

  /* Search for Volume Recognition Sequence (VRS).  */
  for (vblock = (32767 >> (lbshift + holy_DISK_SECTOR_BITS)) + 1;;
       vblock += (2047 >> (lbshift + holy_DISK_SECTOR_BITS)) + 1)
    {
      struct holy_udf_vrs vrs;

      if (holy_disk_read (disk, vblock << lbshift, 0,
			  sizeof (struct holy_udf_vrs), &vrs))
	{
	  holy_error (holy_ERR_BAD_FS, "not an UDF filesystem");
	  goto fail;
	}

      if ((!holy_memcmp (vrs.magic, holy_UDF_STD_IDENT_NSR03, 5)) ||
	  (!holy_memcmp (vrs.magic, holy_UDF_STD_IDENT_NSR02, 5)))
	break;

      if ((holy_memcmp (vrs.magic, holy_UDF_STD_IDENT_BEA01, 5)) &&
	  (holy_memcmp (vrs.magic, holy_UDF_STD_IDENT_BOOT2, 5)) &&
	  (holy_memcmp (vrs.magic, holy_UDF_STD_IDENT_CD001, 5)) &&
	  (holy_memcmp (vrs.magic, holy_UDF_STD_IDENT_CDW02, 5)) &&
	  (holy_memcmp (vrs.magic, holy_UDF_STD_IDENT_TEA01, 5)))
	{
	  holy_error (holy_ERR_BAD_FS, "not an UDF filesystem");
	  goto fail;
	}
    }

  data->npd = data->npm = 0;
  /* Locate Partition Descriptor (PD) and Logical Volume Descriptor (LVD).  */
  while (1)
    {
      struct holy_udf_tag tag;

      if (holy_disk_read (disk, block << lbshift, 0,
			  sizeof (struct holy_udf_tag), &tag))
	{
	  holy_error (holy_ERR_BAD_FS, "not an UDF filesystem");
	  goto fail;
	}

      tag.tag_ident = U16 (tag.tag_ident);
      if (tag.tag_ident == holy_UDF_TAG_IDENT_PD)
	{
	  if (data->npd >= holy_UDF_MAX_PDS)
	    {
	      holy_error (holy_ERR_BAD_FS, "too many PDs");
	      goto fail;
	    }

	  if (holy_disk_read (disk, block << lbshift, 0,
			      sizeof (struct holy_udf_pd),
			      &data->pds[data->npd]))
	    {
	      holy_error (holy_ERR_BAD_FS, "not an UDF filesystem");
	      goto fail;
	    }

	  data->npd++;
	}
      else if (tag.tag_ident == holy_UDF_TAG_IDENT_LVD)
	{
	  int k;

	  struct holy_udf_partmap *ppm;

	  if (holy_disk_read (disk, block << lbshift, 0,
			      sizeof (struct holy_udf_lvd),
			      &data->lvd))
	    {
	      holy_error (holy_ERR_BAD_FS, "not an UDF filesystem");
	      goto fail;
	    }

	  if (data->npm + U32 (data->lvd.num_part_maps) > holy_UDF_MAX_PMS)
	    {
	      holy_error (holy_ERR_BAD_FS, "too many partition maps");
	      goto fail;
	    }

	  ppm = (struct holy_udf_partmap *) &data->lvd.part_maps;
	  for (k = U32 (data->lvd.num_part_maps); k > 0; k--)
	    {
	      if (ppm->type != holy_UDF_PARTMAP_TYPE_1)
		{
		  holy_error (holy_ERR_BAD_FS, "partmap type not supported");
		  goto fail;
		}

	      data->pms[data->npm++] = ppm;
	      ppm = (struct holy_udf_partmap *) ((char *) ppm +
                                                 U32 (ppm->length));
	    }
	}
      else if (tag.tag_ident > holy_UDF_TAG_IDENT_TD)
	{
	  holy_error (holy_ERR_BAD_FS, "invalid tag ident");
	  goto fail;
	}
      else if (tag.tag_ident == holy_UDF_TAG_IDENT_TD)
	break;

      block++;
    }

  for (i = 0; i < data->npm; i++)
    {
      int j;

      for (j = 0; j < data->npd; j++)
	if (data->pms[i]->type1.part_num == data->pds[j].part_num)
	  {
	    data->pms[i]->type1.part_num = j;
	    break;
	  }

      if (j == data->npd)
	{
	  holy_error (holy_ERR_BAD_FS, "can\'t find PD");
	  goto fail;
	}
    }

  block = holy_udf_get_block (data,
			      data->lvd.root_fileset.block.part_ref,
			      data->lvd.root_fileset.block.block_num);

  if (holy_errno)
    goto fail;

  if (holy_disk_read (disk, block << lbshift, 0,
		      sizeof (struct holy_udf_fileset), &root_fs))
    {
      holy_error (holy_ERR_BAD_FS, "not an UDF filesystem");
      goto fail;
    }

  if (U16 (root_fs.tag.tag_ident) != holy_UDF_TAG_IDENT_FSD)
    {
      holy_error (holy_ERR_BAD_FS, "invalid fileset descriptor");
      goto fail;
    }

  data->root_icb = root_fs.root_icb;

  return data;

fail:
  holy_free (data);
  return 0;
}

#ifdef holy_UTIL
holy_disk_addr_t
holy_udf_get_cluster_sector (holy_disk_t disk, holy_uint64_t *sec_per_lcn)
{
  holy_disk_addr_t ret;
  static struct holy_udf_data *data;

  data = holy_udf_mount (disk);
  if (!data)
    return 0;

  ret = U32 (data->pds[data->pms[0]->type1.part_num].start);
  *sec_per_lcn = 1ULL << data->lbshift;
  holy_free (data);
  return ret;
}
#endif

static char *
read_string (const holy_uint8_t *raw, holy_size_t sz, char *outbuf)
{
  holy_uint16_t *utf16 = NULL;
  holy_size_t utf16len = 0;

  if (sz == 0)
    return NULL;

  if (raw[0] != 8 && raw[0] != 16)
    return NULL;

  if (raw[0] == 8)
    {
      unsigned i;
      utf16len = sz - 1;
      utf16 = holy_malloc (utf16len * sizeof (utf16[0]));
      if (!utf16)
	return NULL;
      for (i = 0; i < utf16len; i++)
	utf16[i] = raw[i + 1];
    }
  if (raw[0] == 16)
    {
      unsigned i;
      utf16len = (sz - 1) / 2;
      utf16 = holy_malloc (utf16len * sizeof (utf16[0]));
      if (!utf16)
	return NULL;
      for (i = 0; i < utf16len; i++)
	utf16[i] = (raw[2 * i + 1] << 8) | raw[2*i + 2];
    }
  if (!outbuf)
    outbuf = holy_malloc (utf16len * holy_MAX_UTF8_PER_UTF16 + 1);
  if (outbuf)
    *holy_utf16_to_utf8 ((holy_uint8_t *) outbuf, utf16, utf16len) = '\0';
  holy_free (utf16);
  return outbuf;
}

static int
holy_udf_iterate_dir (holy_fshelp_node_t dir,
		      holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  holy_fshelp_node_t child;
  struct holy_udf_file_ident dirent;
  holy_off_t offset = 0;

  child = holy_malloc (get_fshelp_size (dir->data));
  if (!child)
    return 0;

  /* The current directory is not stored.  */
  holy_memcpy (child, dir, get_fshelp_size (dir->data));

  if (hook (".", holy_FSHELP_DIR, child, hook_data))
    return 1;

  while (offset < U64 (dir->block.fe.file_size))
    {
      if (holy_udf_read_file (dir, 0, 0, offset, sizeof (dirent),
			      (char *) &dirent) != sizeof (dirent))
	return 0;

      if (U16 (dirent.tag.tag_ident) != holy_UDF_TAG_IDENT_FID)
	{
	  holy_error (holy_ERR_BAD_FS, "invalid fid tag");
	  return 0;
	}

      offset += sizeof (dirent) + U16 (dirent.imp_use_length);
      if (!(dirent.characteristics & holy_UDF_FID_CHAR_DELETED))
	{
	  child = holy_malloc (get_fshelp_size (dir->data));
	  if (!child)
	    return 0;

          if (holy_udf_read_icb (dir->data, &dirent.icb, child))
	    return 0;

          if (dirent.characteristics & holy_UDF_FID_CHAR_PARENT)
	    {
	      /* This is the parent directory.  */
	      if (hook ("..", holy_FSHELP_DIR, child, hook_data))
	        return 1;
	    }
          else
	    {
	      enum holy_fshelp_filetype type;
	      char *filename;
	      holy_uint8_t raw[MAX_FILE_IDENT_LENGTH];

	      type = ((dirent.characteristics & holy_UDF_FID_CHAR_DIRECTORY) ?
		      (holy_FSHELP_DIR) : (holy_FSHELP_REG));
	      if (child->block.fe.icbtag.file_type == holy_UDF_ICBTAG_TYPE_SYMLINK)
		type = holy_FSHELP_SYMLINK;

	      if ((holy_udf_read_file (dir, 0, 0, offset,
				       dirent.file_ident_length,
				       (char *) raw))
		  != dirent.file_ident_length)
		return 0;

	      filename = read_string (raw, dirent.file_ident_length, 0);
	      if (!filename)
		holy_print_error ();

	      if (filename && hook (filename, type, child, hook_data))
		{
		  holy_free (filename);
		  return 1;
		}
	      holy_free (filename);
	    }
	}

      /* Align to dword boundary.  */
      offset = (offset + dirent.file_ident_length + 3) & (~3);
    }

  return 0;
}

static char *
holy_udf_read_symlink (holy_fshelp_node_t node)
{
  holy_size_t sz = U64 (node->block.fe.file_size);
  holy_uint8_t *raw;
  const holy_uint8_t *ptr;
  char *out, *optr;

  if (sz < 4)
    return NULL;
  raw = holy_malloc (sz);
  if (!raw)
    return NULL;
  if (holy_udf_read_file (node, NULL, NULL, 0, sz, (char *) raw) < 0)
    {
      holy_free (raw);
      return NULL;
    }

  out = holy_malloc (sz * 2 + 1);
  if (!out)
    {
      holy_free (raw);
      return NULL;
    }

  optr = out;

  for (ptr = raw; ptr < raw + sz; )
    {
      holy_size_t s;
      if ((holy_size_t) (ptr - raw + 4) > sz)
	goto fail;
      if (!(ptr[2] == 0 && ptr[3] == 0))
	goto fail;
      s = 4 + ptr[1];
      if ((holy_size_t) (ptr - raw + s) > sz)
	goto fail;
      switch (*ptr)
	{
	case 1:
	  if (ptr[1])
	    goto fail;
	  /* Fallthrough.  */
	case 2:
	  /* in 4 bytes. out: 1 byte.  */
	  optr = out;
	  *optr++ = '/';
	  break;
	case 3:
	  /* in 4 bytes. out: 3 bytes.  */
	  if (optr != out)
	    *optr++ = '/';
	  *optr++ = '.';
	  *optr++ = '.';
	  break;
	case 4:
	  /* in 4 bytes. out: 2 bytes.  */
	  if (optr != out)
	    *optr++ = '/';
	  *optr++ = '.';
	  break;
	case 5:
	  /* in 4 + n bytes. out, at most: 1 + 2 * n bytes.  */
	  if (optr != out)
	    *optr++ = '/';
	  if (!read_string (ptr + 4, s - 4, optr))
	    goto fail;
	  optr += holy_strlen (optr);
	  break;
	default:
	  goto fail;
	}
      ptr += s;
    }
  *optr = 0;
  holy_free (raw);
  return out;

 fail:
  holy_free (raw);
  holy_free (out);
  holy_error (holy_ERR_BAD_FS, "invalid symlink");
  return NULL;
}

/* Context for holy_udf_dir.  */
struct holy_udf_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_udf_dir.  */
static int
holy_udf_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		   holy_fshelp_node_t node, void *data)
{
  struct holy_udf_dir_ctx *ctx = data;
  struct holy_dirhook_info info;
  const struct holy_udf_timestamp *tstamp = NULL;

  holy_memset (&info, 0, sizeof (info));
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  if (U16 (node->block.fe.tag.tag_ident) == holy_UDF_TAG_IDENT_FE)
    tstamp = &node->block.fe.modification_time;
  else if (U16 (node->block.fe.tag.tag_ident) == holy_UDF_TAG_IDENT_EFE)
    tstamp = &node->block.efe.modification_time;

  if (tstamp && (U16 (tstamp->type_and_timezone) & 0xf000) == 0x1000)
    {
      holy_int16_t tz;
      struct holy_datetime datetime;

      datetime.year = U16 (tstamp->year);
      datetime.month = tstamp->month;
      datetime.day = tstamp->day;
      datetime.hour = tstamp->hour;
      datetime.minute = tstamp->minute;
      datetime.second = tstamp->second;

      tz = U16 (tstamp->type_and_timezone) & 0xfff;
      if (tz & 0x800)
	tz |= 0xf000;
      if (tz == -2047)
	tz = 0;

      info.mtimeset = !!holy_datetime2unixtime (&datetime, &info.mtime);

      info.mtime -= 60 * tz;
    }
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_udf_dir (holy_device_t device, const char *path,
	      holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_udf_dir_ctx ctx = { hook, hook_data };
  struct holy_udf_data *data = 0;
  struct holy_fshelp_node *rootnode = 0;
  struct holy_fshelp_node *foundnode = 0;

  holy_dl_ref (my_mod);

  data = holy_udf_mount (device->disk);
  if (!data)
    goto fail;

  rootnode = holy_malloc (get_fshelp_size (data));
  if (!rootnode)
    goto fail;

  if (holy_udf_read_icb (data, &data->root_icb, rootnode))
    goto fail;

  if (holy_fshelp_find_file (path, rootnode,
			     &foundnode,
			     holy_udf_iterate_dir, holy_udf_read_symlink,
			     holy_FSHELP_DIR))
    goto fail;

  holy_udf_iterate_dir (foundnode, holy_udf_dir_iter, &ctx);

  if (foundnode != rootnode)
    holy_free (foundnode);

fail:
  holy_free (rootnode);

  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_udf_open (struct holy_file *file, const char *name)
{
  struct holy_udf_data *data;
  struct holy_fshelp_node *rootnode = 0;
  struct holy_fshelp_node *foundnode;

  holy_dl_ref (my_mod);

  data = holy_udf_mount (file->device->disk);
  if (!data)
    goto fail;

  rootnode = holy_malloc (get_fshelp_size (data));
  if (!rootnode)
    goto fail;

  if (holy_udf_read_icb (data, &data->root_icb, rootnode))
    goto fail;

  if (holy_fshelp_find_file (name, rootnode,
			     &foundnode,
			     holy_udf_iterate_dir, holy_udf_read_symlink,
			     holy_FSHELP_REG))
    goto fail;

  file->data = foundnode;
  file->offset = 0;
  file->size = U64 (foundnode->block.fe.file_size);

  holy_free (rootnode);

  return 0;

fail:
  holy_dl_unref (my_mod);

  holy_free (data);
  holy_free (rootnode);

  return holy_errno;
}

static holy_ssize_t
holy_udf_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_fshelp_node *node = (struct holy_fshelp_node *) file->data;

  return holy_udf_read_file (node, file->read_hook, file->read_hook_data,
			     file->offset, len, buf);
}

static holy_err_t
holy_udf_close (holy_file_t file)
{
  if (file->data)
    {
      struct holy_fshelp_node *node = (struct holy_fshelp_node *) file->data;

      holy_free (node->data);
      holy_free (node);
    }

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}

static holy_err_t
holy_udf_label (holy_device_t device, char **label)
{
  struct holy_udf_data *data;
  data = holy_udf_mount (device->disk);

  if (data)
    {
      *label = read_string (data->lvd.ident, sizeof (data->lvd.ident), 0);
      holy_free (data);
    }
  else
    *label = 0;

  return holy_errno;
}

static struct holy_fs holy_udf_fs = {
  .name = "udf",
  .dir = holy_udf_dir,
  .open = holy_udf_open,
  .read = holy_udf_read,
  .close = holy_udf_close,
  .label = holy_udf_label,
#ifdef holy_UTIL
  .reserved_first_sector = 1,
  .blocklist_install = 1,
#endif
  .next = 0
};

holy_MOD_INIT (udf)
{
  holy_fs_register (&holy_udf_fs);
  my_mod = mod;
}

holy_MOD_FINI (udf)
{
  holy_fs_unregister (&holy_udf_fs);
}
