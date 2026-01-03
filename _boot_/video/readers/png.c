/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/bitmap.h>
#include <holy/types.h>
#include <holy/normal.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/bufio.h>

holy_MOD_LICENSE ("GPLv2+");

/* Uncomment following define to enable PNG debug.  */
//#define PNG_DEBUG

enum
  {
    PNG_COLOR_TYPE_GRAY = 0,
    PNG_COLOR_MASK_PALETTE = 1,
    PNG_COLOR_MASK_COLOR = 2,
    PNG_COLOR_MASK_ALPHA = 4,
    PNG_COLOR_TYPE_PALETTE = (PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE),
  };

#define PNG_COMPRESSION_BASE	0

#define PNG_INTERLACE_NONE	0
#define PNG_INTERLACE_ADAM7	1

#define PNG_FILTER_TYPE_BASE	0

#define PNG_FILTER_VALUE_NONE	0
#define PNG_FILTER_VALUE_SUB	1
#define PNG_FILTER_VALUE_UP	2
#define PNG_FILTER_VALUE_AVG	3
#define PNG_FILTER_VALUE_PAETH	4
#define PNG_FILTER_VALUE_LAST	5

enum
  {
    PNG_CHUNK_IHDR = 0x49484452,
    PNG_CHUNK_IDAT = 0x49444154,
    PNG_CHUNK_IEND = 0x49454e44,
    PNG_CHUNK_PLTE = 0x504c5445
  };

#define Z_DEFLATED		8
#define Z_FLAG_DICT		32

#define INFLATE_STORED		0
#define INFLATE_FIXED		1
#define INFLATE_DYNAMIC		2

#define WSIZE			0x8000

#define DEFLATE_HCLEN_BASE	4
#define DEFLATE_HCLEN_MAX	19
#define DEFLATE_HLIT_BASE	257
#define DEFLATE_HLIT_MAX	288
#define DEFLATE_HDIST_BASE	1
#define DEFLATE_HDIST_MAX	30

#define DEFLATE_HUFF_LEN	16

#ifdef PNG_DEBUG
static holy_command_t cmd;
#endif

struct huff_table
{
  int *values, *maxval, *offset;
  int num_values, max_length;
};

struct holy_png_data
{
  holy_file_t file;
  struct holy_video_bitmap **bitmap;

  int bit_count, bit_save;

  holy_uint32_t next_offset;

  unsigned image_width, image_height;
  int bpp, is_16bit;
  int raw_bytes, is_gray, is_alpha, is_palette;
  int row_bytes, color_bits;
  holy_uint8_t *image_data;

  int inside_idat, idat_remain;

  int code_values[DEFLATE_HLIT_MAX];
  int code_maxval[DEFLATE_HUFF_LEN];
  int code_offset[DEFLATE_HUFF_LEN];

  int dist_values[DEFLATE_HDIST_MAX];
  int dist_maxval[DEFLATE_HUFF_LEN];
  int dist_offset[DEFLATE_HUFF_LEN];

  holy_uint8_t palette[256][3];

  struct huff_table code_table;
  struct huff_table dist_table;

  holy_uint8_t slide[WSIZE];
  int wp;

  holy_uint8_t *cur_rgb;

  int cur_column, cur_filter, first_line;
};

static holy_uint32_t
holy_png_get_dword (struct holy_png_data *data)
{
  holy_uint32_t r;

  r = 0;
  holy_file_read (data->file, &r, sizeof (holy_uint32_t));

  return holy_be_to_cpu32 (r);
}

static holy_uint8_t
holy_png_get_byte (struct holy_png_data *data)
{
  holy_uint8_t r;

  if ((data->inside_idat) && (data->idat_remain == 0))
    {
      holy_uint32_t len, type;

      do
	{
          /* Skip crc checksum.  */
	  holy_png_get_dword (data);

          if (data->file->offset != data->next_offset)
            {
              holy_error (holy_ERR_BAD_FILE_TYPE,
                          "png: chunk size error");
              return 0;
            }

	  len = holy_png_get_dword (data);
	  type = holy_png_get_dword (data);
	  if (type != PNG_CHUNK_IDAT)
	    {
	      holy_error (holy_ERR_BAD_FILE_TYPE,
			  "png: unexpected end of data");
	      return 0;
	    }

          data->next_offset = data->file->offset + len + 4;
	}
      while (len == 0);
      data->idat_remain = len;
    }

  r = 0;
  holy_file_read (data->file, &r, 1);

  if (data->inside_idat)
    data->idat_remain--;

  return r;
}

static int
holy_png_get_bits (struct holy_png_data *data, int num)
{
  int code, shift;

  if (data->bit_count == 0)
    {
      data->bit_save = holy_png_get_byte (data);
      data->bit_count = 8;
    }

  code = 0;
  shift = 0;
  while (holy_errno == 0)
    {
      int n;

      n = data->bit_count;
      if (n > num)
	n = num;

      code += (int) (data->bit_save & ((1 << n) - 1)) << shift;
      num -= n;
      if (!num)
	{
	  data->bit_count -= n;
	  data->bit_save >>= n;
	  break;
	}

      shift += n;

      data->bit_save = holy_png_get_byte (data);
      data->bit_count = 8;
    }

  return code;
}

static holy_err_t
holy_png_decode_image_palette (struct holy_png_data *data,
			       unsigned len)
{
  unsigned i = 0, j;

  if (len == 0)
    return holy_ERR_NONE;

  for (i = 0; 3 * i < len && i < 256; i++)
    for (j = 0; j < 3; j++)
      data->palette[i][j] = holy_png_get_byte (data);
  for (i *= 3; i < len; i++)
    holy_png_get_byte (data);

  holy_png_get_dword (data);

  return holy_ERR_NONE;
}

static holy_err_t
holy_png_decode_image_header (struct holy_png_data *data)
{
  int color_type;
  int color_bits;
  enum holy_video_blit_format blt;

  data->image_width = holy_png_get_dword (data);
  data->image_height = holy_png_get_dword (data);

  if ((!data->image_height) || (!data->image_width))
    return holy_error (holy_ERR_BAD_FILE_TYPE, "png: invalid image size");

  color_bits = holy_png_get_byte (data);
  data->is_16bit = (color_bits == 16);

  color_type = holy_png_get_byte (data);

  /* According to PNG spec, no other types are valid.  */
  if ((color_type & ~(PNG_COLOR_MASK_ALPHA | PNG_COLOR_MASK_COLOR))
      && (color_type != PNG_COLOR_TYPE_PALETTE))
    return holy_error (holy_ERR_BAD_FILE_TYPE,
		       "png: color type not supported");
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    data->is_palette = 1;
  if (data->is_16bit && data->is_palette)
    return holy_error (holy_ERR_BAD_FILE_TYPE,
		       "png: color type not supported");
  if (color_type & PNG_COLOR_MASK_ALPHA)
    blt = holy_VIDEO_BLIT_FORMAT_RGBA_8888;
  else
    blt = holy_VIDEO_BLIT_FORMAT_RGB_888;
  if (data->is_palette)
    data->bpp = 1;
  else if (color_type & PNG_COLOR_MASK_COLOR)
    data->bpp = 3;
  else
    {
      data->is_gray = 1;
      data->bpp = 1;
    }

  if ((color_bits != 8) && (color_bits != 16)
      && (color_bits != 4
	  || !(data->is_gray || data->is_palette)))
    return holy_error (holy_ERR_BAD_FILE_TYPE,
                       "png: bit depth must be 8 or 16");

  if (color_type & PNG_COLOR_MASK_ALPHA)
    data->bpp++;

  if (holy_video_bitmap_create (data->bitmap, data->image_width,
				data->image_height,
				blt))
    return holy_errno;

  if (data->is_16bit)
      data->bpp <<= 1;

  data->color_bits = color_bits;
  data->row_bytes = data->image_width * data->bpp;
  if (data->color_bits <= 4)
    data->row_bytes = (data->image_width * data->color_bits + 7) / 8;

#ifndef holy_CPU_WORDS_BIGENDIAN
  if (data->is_16bit || data->is_gray || data->is_palette)
#endif
    {
      data->image_data = holy_malloc (data->image_height * data->row_bytes);
      if (holy_errno)
        return holy_errno;

      data->cur_rgb = data->image_data;
    }
#ifndef holy_CPU_WORDS_BIGENDIAN
  else
    {
      data->image_data = 0;
      data->cur_rgb = (*data->bitmap)->data;
    }
#endif

  data->raw_bytes = data->image_height * (data->row_bytes + 1);

  data->cur_column = 0;
  data->first_line = 1;

  if (holy_png_get_byte (data) != PNG_COMPRESSION_BASE)
    return holy_error (holy_ERR_BAD_FILE_TYPE,
		       "png: compression method not supported");

  if (holy_png_get_byte (data) != PNG_FILTER_TYPE_BASE)
    return holy_error (holy_ERR_BAD_FILE_TYPE,
		       "png: filter method not supported");

  if (holy_png_get_byte (data) != PNG_INTERLACE_NONE)
    return holy_error (holy_ERR_BAD_FILE_TYPE,
		       "png: interlace method not supported");

  /* Skip crc checksum.  */
  holy_png_get_dword (data);

  return holy_errno;
}

/* Order of the bit length code lengths.  */
static const holy_uint8_t bitorder[] = {
  16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/* Copy lengths for literal codes 257..285.  */
static const int cplens[] = {
  3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
  35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
};

/* Extra bits for literal codes 257..285.  */
static const holy_uint8_t cplext[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
  3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99
};				/* 99==invalid  */

/* Copy offsets for distance codes 0..29.  */
static const int cpdist[] = {
  1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
  257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
  8193, 12289, 16385, 24577
};

/* Extra bits for distance codes.  */
static const holy_uint8_t cpdext[] = {
  0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
  7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
  12, 12, 13, 13
};

static void
holy_png_init_huff_table (struct huff_table *ht, int cur_maxlen,
			  int *cur_values, int *cur_maxval, int *cur_offset)
{
  ht->values = cur_values;
  ht->maxval = cur_maxval;
  ht->offset = cur_offset;
  ht->num_values = 0;
  ht->max_length = cur_maxlen;
  holy_memset (cur_maxval, 0, sizeof (int) * cur_maxlen);
}

static void
holy_png_insert_huff_item (struct huff_table *ht, int code, int len)
{
  int i, n;

  if (len == 0)
    return;

  if (len > ht->max_length)
    {
      holy_error (holy_ERR_BAD_FILE_TYPE, "png: invalid code length");
      return;
    }

  n = 0;
  for (i = len; i < ht->max_length; i++)
    n += ht->maxval[i];

  for (i = 0; i < n; i++)
    ht->values[ht->num_values - i] = ht->values[ht->num_values - i - 1];

  ht->values[ht->num_values - n] = code;
  ht->num_values++;
  ht->maxval[len - 1]++;
}

static void
holy_png_build_huff_table (struct huff_table *ht)
{
  int base, ofs, i;

  base = 0;
  ofs = 0;
  for (i = 0; i < ht->max_length; i++)
    {
      base += ht->maxval[i];
      ofs += ht->maxval[i];

      ht->maxval[i] = base;
      ht->offset[i] = ofs - base;

      base <<= 1;
    }
}

static int
holy_png_get_huff_code (struct holy_png_data *data, struct huff_table *ht)
{
  int code, i;

  code = 0;
  for (i = 0; i < ht->max_length; i++)
    {
      code = (code << 1) + holy_png_get_bits (data, 1);
      if (code < ht->maxval[i])
	return ht->values[code + ht->offset[i]];
    }
  return 0;
}

static holy_err_t
holy_png_init_fixed_block (struct holy_png_data *data)
{
  int i;

  holy_png_init_huff_table (&data->code_table, DEFLATE_HUFF_LEN,
			    data->code_values, data->code_maxval,
			    data->code_offset);

  for (i = 0; i < 144; i++)
    holy_png_insert_huff_item (&data->code_table, i, 8);

  for (; i < 256; i++)
    holy_png_insert_huff_item (&data->code_table, i, 9);

  for (; i < 280; i++)
    holy_png_insert_huff_item (&data->code_table, i, 7);

  for (; i < DEFLATE_HLIT_MAX; i++)
    holy_png_insert_huff_item (&data->code_table, i, 8);

  holy_png_build_huff_table (&data->code_table);

  holy_png_init_huff_table (&data->dist_table, DEFLATE_HUFF_LEN,
			    data->dist_values, data->dist_maxval,
			    data->dist_offset);

  for (i = 0; i < DEFLATE_HDIST_MAX; i++)
    holy_png_insert_huff_item (&data->dist_table, i, 5);

  holy_png_build_huff_table (&data->dist_table);

  return holy_errno;
}

static holy_err_t
holy_png_init_dynamic_block (struct holy_png_data *data)
{
  int nl, nd, nb, i, prev;
  struct huff_table cl;
  int cl_values[sizeof (bitorder)];
  int cl_maxval[8];
  int cl_offset[8];
  holy_uint8_t lens[DEFLATE_HCLEN_MAX];

  nl = DEFLATE_HLIT_BASE + holy_png_get_bits (data, 5);
  nd = DEFLATE_HDIST_BASE + holy_png_get_bits (data, 5);
  nb = DEFLATE_HCLEN_BASE + holy_png_get_bits (data, 4);

  if ((nl > DEFLATE_HLIT_MAX) || (nd > DEFLATE_HDIST_MAX) ||
      (nb > DEFLATE_HCLEN_MAX))
    return holy_error (holy_ERR_BAD_FILE_TYPE, "png: too much data");

  holy_png_init_huff_table (&cl, 8, cl_values, cl_maxval, cl_offset);

  for (i = 0; i < nb; i++)
    lens[bitorder[i]] = holy_png_get_bits (data, 3);

  for (; i < DEFLATE_HCLEN_MAX; i++)
    lens[bitorder[i]] = 0;

  for (i = 0; i < DEFLATE_HCLEN_MAX; i++)
    holy_png_insert_huff_item (&cl, i, lens[i]);

  holy_png_build_huff_table (&cl);

  holy_png_init_huff_table (&data->code_table, DEFLATE_HUFF_LEN,
			    data->code_values, data->code_maxval,
			    data->code_offset);

  holy_png_init_huff_table (&data->dist_table, DEFLATE_HUFF_LEN,
			    data->dist_values, data->dist_maxval,
			    data->dist_offset);

  prev = 0;
  for (i = 0; i < nl + nd; i++)
    {
      int n, code;
      struct huff_table *ht;

      if (holy_errno)
	return holy_errno;

      if (i < nl)
	{
	  ht = &data->code_table;
	  code = i;
	}
      else
	{
	  ht = &data->dist_table;
	  code = i - nl;
	}

      n = holy_png_get_huff_code (data, &cl);
      if (n < 16)
	{
	  holy_png_insert_huff_item (ht, code, n);
	  prev = n;
	}
      else if (n == 16)
	{
	  int c;

	  c = 3 + holy_png_get_bits (data, 2);
	  while (c > 0)
	    {
	      holy_png_insert_huff_item (ht, code++, prev);
	      i++;
	      c--;
	    }
	  i--;
	}
      else if (n == 17)
	i += 3 + holy_png_get_bits (data, 3) - 1;
      else
	i += 11 + holy_png_get_bits (data, 7) - 1;
    }

  holy_png_build_huff_table (&data->code_table);
  holy_png_build_huff_table (&data->dist_table);

  return holy_errno;
}

static holy_err_t
holy_png_output_byte (struct holy_png_data *data, holy_uint8_t n)
{
  if (--data->raw_bytes < 0)
    return holy_error (holy_ERR_BAD_FILE_TYPE, "image size overflown");

  if (data->cur_column == 0)
    {
      if (n >= PNG_FILTER_VALUE_LAST)
	return holy_error (holy_ERR_BAD_FILE_TYPE, "invalid filter value");

      data->cur_filter = n;
    }
  else
    *data->cur_rgb++ = n;

  data->cur_column++;
  if (data->cur_column == data->row_bytes + 1)
    {
      holy_uint8_t *blank_line = NULL;
      holy_uint8_t *cur = data->cur_rgb - data->row_bytes;
      holy_uint8_t *left = cur;
      holy_uint8_t *up;

      if (data->first_line)
	{
	  blank_line = holy_zalloc (data->row_bytes);
	  if (blank_line == NULL)
	    return holy_errno;

	  up = blank_line;
	}
      else
	up = cur - data->row_bytes;

      switch (data->cur_filter)
	{
	case PNG_FILTER_VALUE_SUB:
	  {
	    int i;

	    cur += data->bpp;
	    for (i = data->bpp; i < data->row_bytes; i++, cur++, left++)
	      *cur += *left;

	    break;
	  }
	case PNG_FILTER_VALUE_UP:
	  {
	    int i;

	    for (i = 0; i < data->row_bytes; i++, cur++, up++)
	      *cur += *up;

	    break;
	  }
	case PNG_FILTER_VALUE_AVG:
	  {
	    int i;

	    for (i = 0; i < data->bpp; i++, cur++, up++)
	      *cur += *up >> 1;

	    for (; i < data->row_bytes; i++, cur++, up++, left++)
	      *cur += ((int) *up + (int) *left) >> 1;

	    break;
	  }
	case PNG_FILTER_VALUE_PAETH:
	  {
	    int i;
	    holy_uint8_t *upper_left = up;

	    for (i = 0; i < data->bpp; i++, cur++, up++)
	      *cur += *up;

	    for (; i < data->row_bytes; i++, cur++, up++, left++, upper_left++)
	      {
		int a, b, c, pa, pb, pc;

                a = *left;
                b = *up;
                c = *upper_left;

                pa = b - c;
                pb = a - c;
                pc = pa + pb;

                if (pa < 0)
                  pa = -pa;

                if (pb < 0)
                  pb = -pb;

                if (pc < 0)
                  pc = -pc;

                *cur += ((pa <= pb) && (pa <= pc)) ? a : (pb <= pc) ? b : c;
	      }
	  }
	}

      holy_free (blank_line);

      data->cur_column = 0;
      data->first_line = 0;
    }

  return holy_errno;
}

static holy_err_t
holy_png_read_dynamic_block (struct holy_png_data *data)
{
  while (holy_errno == 0)
    {
      int n;

      n = holy_png_get_huff_code (data, &data->code_table);
      if (n < 256)
	{
	  data->slide[data->wp] = n;
	  holy_png_output_byte (data, n);

	  data->wp++;
	  if (data->wp >= WSIZE)
	    data->wp = 0;
	}
      else if (n == 256)
	break;
      else
	{
	  int len, dist, pos;

	  n -= 257;
	  len = cplens[n];
	  if (cplext[n])
	    len += holy_png_get_bits (data, cplext[n]);

	  n = holy_png_get_huff_code (data, &data->dist_table);
	  dist = cpdist[n];
	  if (cpdext[n])
	    dist += holy_png_get_bits (data, cpdext[n]);

	  pos = data->wp - dist;
	  if (pos < 0)
	    pos += WSIZE;

	  while (len > 0)
	    {
	      data->slide[data->wp] = data->slide[pos];
	      holy_png_output_byte (data, data->slide[data->wp]);

	      data->wp++;
	      if (data->wp >= WSIZE)
		data->wp = 0;

	      pos++;
	      if (pos >= WSIZE)
		pos = 0;

	      len--;
	    }
	}
    }

  return holy_errno;
}

static holy_err_t
holy_png_decode_image_data (struct holy_png_data *data)
{
  holy_uint8_t cmf, flg;
  int final;

  cmf = holy_png_get_byte (data);
  flg = holy_png_get_byte (data);

  if ((cmf & 0xF) != Z_DEFLATED)
    return holy_error (holy_ERR_BAD_FILE_TYPE,
		       "png: only support deflate compression method");

  if (flg & Z_FLAG_DICT)
    return holy_error (holy_ERR_BAD_FILE_TYPE,
		       "png: dictionary not supported");

  do
    {
      int block_type;

      final = holy_png_get_bits (data, 1);
      block_type = holy_png_get_bits (data, 2);

      switch (block_type)
	{
	case INFLATE_STORED:
	  {
	    holy_uint16_t i, len;

	    data->bit_count = 0;
	    len = holy_png_get_byte (data);
	    len += ((holy_uint16_t) holy_png_get_byte (data)) << 8;

            /* Skip NLEN field.  */
	    holy_png_get_byte (data);
	    holy_png_get_byte (data);

	    for (i = 0; i < len; i++)
	      holy_png_output_byte (data, holy_png_get_byte (data));

	    break;
	  }

	case INFLATE_FIXED:
          holy_png_init_fixed_block (data);
	  holy_png_read_dynamic_block (data);
	  break;

	case INFLATE_DYNAMIC:
	  holy_png_init_dynamic_block (data);
	  holy_png_read_dynamic_block (data);
	  break;

	default:
	  return holy_error (holy_ERR_BAD_FILE_TYPE,
			     "png: unknown block type");
	}
    }
  while ((!final) && (holy_errno == 0));

  /* Skip adler checksum.  */
  holy_png_get_dword (data);

  /* Skip crc checksum.  */
  holy_png_get_dword (data);

  return holy_errno;
}

static const holy_uint8_t png_magic[8] =
  { 0x89, 0x50, 0x4e, 0x47, 0xd, 0xa, 0x1a, 0x0a };

static void
holy_png_convert_image (struct holy_png_data *data)
{
  unsigned i;
  holy_uint8_t *d1, *d2;

  d1 = (*data->bitmap)->data;
  d2 = data->image_data + data->is_16bit;

#ifndef holy_CPU_WORDS_BIGENDIAN
#define R4 3
#define G4 2
#define B4 1
#define A4 0
#define R3 2
#define G3 1
#define B3 0
#else
#define R4 0
#define G4 1
#define B4 2
#define A4 3
#define R3 0
#define G3 1
#define B3 2
#endif

  if (data->color_bits <= 4)
    {
      holy_uint8_t palette[16][3];
      holy_uint8_t *d1c, *d2c;
      int shift;
      int mask = (1 << data->color_bits) - 1;
      unsigned j;
      if (data->is_gray)
	{
	  /* Generic formula is
	     (0xff * i) / ((1U << data->color_bits) - 1)
	     but for allowed bit depth of 1, 2 and for it's
	     equivalent to
	     (0xff / ((1U << data->color_bits) - 1)) * i
	     Precompute the multipliers to avoid division.
	  */

	  const holy_uint8_t multipliers[5] = { 0xff, 0xff, 0x55, 0x24, 0x11 };
	  for (i = 0; i < (1U << data->color_bits); i++)
	    {
	      holy_uint8_t col = multipliers[data->color_bits] * i;
	      palette[i][0] = col;
	      palette[i][1] = col;
	      palette[i][2] = col;
	    }
	}
      else
	holy_memcpy (palette, data->palette, 3 << data->color_bits);
      d1c = d1;
      d2c = d2;
      for (j = 0; j < data->image_height; j++, d1c += data->image_width * 3,
	   d2c += data->row_bytes)
	{
	  d1 = d1c;
	  d2 = d2c;
	  shift = 8 - data->color_bits;
	  for (i = 0; i < data->image_width; i++, d1 += 3)
	    {
	      holy_uint8_t col = (d2[0] >> shift) & mask;
	      d1[R3] = data->palette[col][2];
	      d1[G3] = data->palette[col][1];
	      d1[B3] = data->palette[col][0];
	      shift -= data->color_bits;
	      if (shift < 0)
		{
		  d2++;
		  shift += 8;
		}
	    }
	}
      return;
    }

  if (data->is_palette)
    {
      for (i = 0; i < (data->image_width * data->image_height);
	   i++, d1 += 3, d2++)
	{
	  d1[R3] = data->palette[d2[0]][2];
	  d1[G3] = data->palette[d2[0]][1];
	  d1[B3] = data->palette[d2[0]][0];
	}
      return;
    }
  
  if (data->is_gray)
    {
      switch (data->bpp)
	{
	case 4:
	  /* 16-bit gray with alpha.  */
	  for (i = 0; i < (data->image_width * data->image_height);
	       i++, d1 += 4, d2 += 4)
	    {
	      d1[R4] = d2[3];
	      d1[G4] = d2[3];
	      d1[B4] = d2[3];
	      d1[A4] = d2[1];
	    }
	  break;
	case 2:
	  if (data->is_16bit)
	    /* 16-bit gray without alpha.  */
	    {
	      for (i = 0; i < (data->image_width * data->image_height);
		   i++, d1 += 4, d2 += 2)
		{
		  d1[R3] = d2[1];
		  d1[G3] = d2[1];
		  d1[B3] = d2[1];
		}
	    }
	  else
	    /* 8-bit gray with alpha.  */
	    {
	      for (i = 0; i < (data->image_width * data->image_height);
		   i++, d1 += 4, d2 += 2)
		{
		  d1[R4] = d2[1];
		  d1[G4] = d2[1];
		  d1[B4] = d2[1];
		  d1[A4] = d2[0];
		}
	    }
	  break;
	  /* 8-bit gray without alpha.  */
	case 1:
	  for (i = 0; i < (data->image_width * data->image_height);
	       i++, d1 += 3, d2++)
	    {
	      d1[R3] = d2[0];
	      d1[G3] = d2[0];
	      d1[B3] = d2[0];
	    }
	  break;
	}
      return;
    }

    {
  /* Only copy the upper 8 bit.  */
#ifndef holy_CPU_WORDS_BIGENDIAN
      for (i = 0; i < (data->image_width * data->image_height * data->bpp >> 1);
	   i++, d1++, d2 += 2)
	*d1 = *d2;
#else
      switch (data->bpp)
	{
	  /* 16-bit with alpha.  */
	case 8:
	  for (i = 0; i < (data->image_width * data->image_height);
	       i++, d1 += 4, d2+=8)
	    {
	      d1[0] = d2[7];
	      d1[1] = d2[5];
	      d1[2] = d2[3];
	      d1[3] = d2[1];
	    }
	  break;
	  /* 16-bit without alpha.  */
	case 6:
	  for (i = 0; i < (data->image_width * data->image_height);
	       i++, d1 += 3, d2+=6)
	    {
	      d1[0] = d2[5];
	      d1[1] = d2[3];
	      d1[2] = d2[1];
	    }
	  break;
	case 4:
	  /* 8-bit with alpha.  */
	  for (i = 0; i < (data->image_width * data->image_height);
	       i++, d1 += 4, d2 += 4)
	    {
	      d1[0] = d2[3];
	      d1[1] = d2[2];
	      d1[2] = d2[1];
	      d1[3] = d2[0];
	    }
	  break;
	  /* 8-bit without alpha.  */
	case 3:
	  for (i = 0; i < (data->image_width * data->image_height);
	       i++, d1 += 3, d2 += 3)
	    {
	      d1[0] = d2[2];
	      d1[1] = d2[1];
	      d1[2] = d2[0];
	    }
	  break;
	}
#endif
    }

}

static holy_err_t
holy_png_decode_png (struct holy_png_data *data)
{
  holy_uint8_t magic[8];

  if (holy_file_read (data->file, &magic[0], 8) != 8)
    return holy_errno;

  if (holy_memcmp (magic, png_magic, sizeof (png_magic)))
    return holy_error (holy_ERR_BAD_FILE_TYPE, "png: not a png file");

  while (1)
    {
      holy_uint32_t len, type;

      len = holy_png_get_dword (data);
      type = holy_png_get_dword (data);
      data->next_offset = data->file->offset + len + 4;

      switch (type)
	{
	case PNG_CHUNK_IHDR:
	  holy_png_decode_image_header (data);
	  break;

	case PNG_CHUNK_PLTE:
	  holy_png_decode_image_palette (data, len);
	  break;

	case PNG_CHUNK_IDAT:
	  data->inside_idat = 1;
	  data->idat_remain = len;
	  data->bit_count = 0;

	  holy_png_decode_image_data (data);

	  data->inside_idat = 0;
	  break;

	case PNG_CHUNK_IEND:
          if (data->image_data)
            holy_png_convert_image (data);

	  return holy_errno;

	default:
	  holy_file_seek (data->file, data->file->offset + len + 4);
	}

      if (holy_errno)
        break;

      if (data->file->offset != data->next_offset)
        return holy_error (holy_ERR_BAD_FILE_TYPE,
                           "png: chunk size error");
    }

  return holy_errno;
}

static holy_err_t
holy_video_reader_png (struct holy_video_bitmap **bitmap,
		       const char *filename)
{
  holy_file_t file;
  struct holy_png_data *data;

  file = holy_buffile_open (filename, 0);
  if (!file)
    return holy_errno;

  data = holy_zalloc (sizeof (*data));
  if (data != NULL)
    {
      data->file = file;
      data->bitmap = bitmap;

      holy_png_decode_png (data);

      holy_free (data->image_data);
      holy_free (data);
    }

  if (holy_errno != holy_ERR_NONE)
    {
      holy_video_bitmap_destroy (*bitmap);
      *bitmap = 0;
    }

  holy_file_close (file);
  return holy_errno;
}

#if defined(PNG_DEBUG)
static holy_err_t
holy_cmd_pngtest (holy_command_t cmd_d __attribute__ ((unused)),
		  int argc, char **args)
{
  struct holy_video_bitmap *bitmap = 0;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_video_reader_png (&bitmap, args[0]);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  holy_video_bitmap_destroy (bitmap);

  return holy_ERR_NONE;
}
#endif

static struct holy_video_bitmap_reader png_reader = {
  .extension = ".png",
  .reader = holy_video_reader_png,
  .next = 0
};

holy_MOD_INIT (png)
{
  holy_video_bitmap_reader_register (&png_reader);
#if defined(PNG_DEBUG)
  cmd = holy_register_command ("pngtest", holy_cmd_pngtest,
			       "FILE",
			       "Tests loading of PNG bitmap.");
#endif
}

holy_MOD_FINI (png)
{
#if defined(PNG_DEBUG)
  holy_unregister_command (cmd);
#endif
  holy_video_bitmap_reader_unregister (&png_reader);
}
