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

/* Uncomment following define to enable TGA debug.  */
//#define TGA_DEBUG

#define dump_int_field(x) holy_dprintf ("tga", #x " = %d (0x%04x)\n", x, x);
#if defined(TGA_DEBUG)
static holy_command_t cmd;
#endif

enum
{
  holy_TGA_IMAGE_TYPE_NONE = 0,
  holy_TGA_IMAGE_TYPE_UNCOMPRESSED_INDEXCOLOR = 1,
  holy_TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOR = 2,
  holy_TGA_IMAGE_TYPE_UNCOMPRESSED_BLACK_AND_WHITE = 3,
  holy_TGA_IMAGE_TYPE_RLE_INDEXCOLOR = 9,
  holy_TGA_IMAGE_TYPE_RLE_TRUECOLOR = 10,
  holy_TGA_IMAGE_TYPE_RLE_BLACK_AND_WHITE = 11,
};

enum
{
  holy_TGA_COLOR_MAP_TYPE_NONE = 0,
  holy_TGA_COLOR_MAP_TYPE_INCLUDED = 1
};

enum
{
  holy_TGA_IMAGE_ORIGIN_RIGHT = 0x10,
  holy_TGA_IMAGE_ORIGIN_TOP   = 0x20
};

struct holy_tga_header
{
  holy_uint8_t id_length;
  holy_uint8_t color_map_type;
  holy_uint8_t image_type;

  /* Color Map Specification.  */
  holy_uint16_t color_map_first_index;
  holy_uint16_t color_map_length;
  holy_uint8_t color_map_bpp;

  /* Image Specification.  */
  holy_uint16_t image_x_origin;
  holy_uint16_t image_y_origin;
  holy_uint16_t image_width;
  holy_uint16_t image_height;
  holy_uint8_t image_bpp;
  holy_uint8_t image_descriptor;
} holy_PACKED;

struct tga_data
{
  struct holy_tga_header hdr;
  int uses_rle;
  int pktlen;
  int bpp;
  int is_rle;
  holy_uint8_t pixel[4];
  holy_uint8_t palette[256][3];
  struct holy_video_bitmap *bitmap;
  holy_file_t file;
  unsigned image_width;
  unsigned image_height;
};

static holy_err_t
fetch_pixel (struct tga_data *data)
{
  if (!data->uses_rle)
    {
      if (holy_file_read (data->file, &data->pixel[0], data->bpp)
	  != data->bpp)
	return holy_errno;
      return holy_ERR_NONE;
    }
  if (!data->pktlen)
    {
      holy_uint8_t type;
      if (holy_file_read (data->file, &type, sizeof (type)) != sizeof(type))
	return holy_errno;
      data->is_rle = !!(type & 0x80);
      data->pktlen = (type & 0x7f) + 1;
      if (data->is_rle && holy_file_read (data->file, &data->pixel[0], data->bpp)
	  != data->bpp)
	return holy_errno;
    }
  if (!data->is_rle && holy_file_read (data->file, &data->pixel[0], data->bpp)
      != data->bpp)
    return holy_errno;
  data->pktlen--;
  return holy_ERR_NONE;
}

static holy_err_t
tga_load_palette (struct tga_data *data)
{
  holy_size_t len = holy_le_to_cpu32 (data->hdr.color_map_length) * 3;

  if (len > sizeof (data->palette))
    len = sizeof (data->palette);
  
  if (holy_file_read (data->file, &data->palette, len)
      != (holy_ssize_t) len)
    return holy_errno;

#ifndef holy_CPU_WORDS_BIGENDIAN
  {
    holy_size_t i;
    for (i = 0; 3 * i < len; i++)
      {
	holy_uint8_t t;
	t = data->palette[i][0];
	data->palette[i][0] = data->palette[i][2];
	data->palette[i][2] = t;
      }
  }
#endif
  return holy_ERR_NONE;
}

static holy_err_t
tga_load_index_color (struct tga_data *data)
{
  unsigned int x;
  unsigned int y;
  holy_uint8_t *ptr;

  for (y = 0; y < data->image_height; y++)
    {
      ptr = data->bitmap->data;
      if ((data->hdr.image_descriptor & holy_TGA_IMAGE_ORIGIN_TOP) != 0)
        ptr += y * data->bitmap->mode_info.pitch;
      else
        ptr += (data->image_height - 1 - y) * data->bitmap->mode_info.pitch;

      for (x = 0; x < data->image_width; x++)
        {
	  holy_err_t err;
	  err = fetch_pixel (data);
          if (err)
            return err;

          ptr[0] = data->palette[data->pixel[0]][0];
          ptr[1] = data->palette[data->pixel[0]][1];
          ptr[2] = data->palette[data->pixel[0]][2];

          ptr += 3;
        }
    }
  return holy_ERR_NONE;
}

static holy_err_t
tga_load_grayscale (struct tga_data *data)
{
  unsigned int x;
  unsigned int y;
  holy_uint8_t *ptr;

  for (y = 0; y < data->image_height; y++)
    {
      ptr = data->bitmap->data;
      if ((data->hdr.image_descriptor & holy_TGA_IMAGE_ORIGIN_TOP) != 0)
        ptr += y * data->bitmap->mode_info.pitch;
      else
        ptr += (data->image_height - 1 - y) * data->bitmap->mode_info.pitch;

      for (x = 0; x < data->image_width; x++)
        {
	  holy_err_t err;
	  err = fetch_pixel (data);
          if (err)
            return err;

          ptr[0] = data->pixel[0];
          ptr[1] = data->pixel[0];
          ptr[2] = data->pixel[0];

          ptr += 3;
        }
    }
  return holy_ERR_NONE;
}

static holy_err_t
tga_load_truecolor_R8G8B8 (struct tga_data *data)
{
  unsigned int x;
  unsigned int y;
  holy_uint8_t *ptr;

  for (y = 0; y < data->image_height; y++)
    {
      ptr = data->bitmap->data;
      if ((data->hdr.image_descriptor & holy_TGA_IMAGE_ORIGIN_TOP) != 0)
        ptr += y * data->bitmap->mode_info.pitch;
      else
        ptr += (data->image_height - 1 - y) * data->bitmap->mode_info.pitch;

      for (x = 0; x < data->image_width; x++)
        {
	  holy_err_t err;
	  err = fetch_pixel (data);
          if (err)
            return err;

#ifdef holy_CPU_WORDS_BIGENDIAN
          ptr[0] = data->pixel[0];
          ptr[1] = data->pixel[1];
          ptr[2] = data->pixel[2];
#else
          ptr[0] = data->pixel[2];
          ptr[1] = data->pixel[1];
          ptr[2] = data->pixel[0];
#endif
          ptr += 3;
        }
    }
  return holy_ERR_NONE;
}

static holy_err_t
tga_load_truecolor_R8G8B8A8 (struct tga_data *data)
{
  unsigned int x;
  unsigned int y;
  holy_uint8_t *ptr;

  for (y = 0; y < data->image_height; y++)
    {
      ptr = data->bitmap->data;
      if ((data->hdr.image_descriptor & holy_TGA_IMAGE_ORIGIN_TOP) != 0)
        ptr += y * data->bitmap->mode_info.pitch;
      else
        ptr += (data->image_height - 1 - y) * data->bitmap->mode_info.pitch;

      for (x = 0; x < data->image_width; x++)
        {
	  holy_err_t err;
	  err = fetch_pixel (data);
          if (err)
            return err;

#ifdef holy_CPU_WORDS_BIGENDIAN
          ptr[0] = data->pixel[0];
          ptr[1] = data->pixel[1];
          ptr[2] = data->pixel[2];
          ptr[3] = data->pixel[3];
#else
          ptr[0] = data->pixel[2];
          ptr[1] = data->pixel[1];
          ptr[2] = data->pixel[0];
          ptr[3] = data->pixel[3];
#endif

          ptr += 4;
        }
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_reader_tga (struct holy_video_bitmap **bitmap,
                       const char *filename)
{
  holy_ssize_t pos;
  struct tga_data data;

  holy_memset (&data, 0, sizeof (data));

  data.file = holy_buffile_open (filename, 0);
  if (! data.file)
    return holy_errno;

  /* TGA Specification states that we SHOULD start by reading
     ID from end of file, but we really don't care about that as we are
     not going to support developer area & extensions at this point.  */

  /* Read TGA header from beginning of file.  */
  if (holy_file_read (data.file, &data.hdr, sizeof (data.hdr))
      != sizeof (data.hdr))
    {
      holy_file_close (data.file);
      return holy_errno;
    }

  /* Skip ID field.  */
  pos = holy_file_tell (data.file);
  pos += data.hdr.id_length;
  holy_file_seek (data.file, pos);
  if (holy_errno != holy_ERR_NONE)
    {
      holy_file_close (data.file);
      return holy_errno;
    }

  holy_dprintf("tga", "tga: header\n");
  dump_int_field(data.hdr.id_length);
  dump_int_field(data.hdr.color_map_type);
  dump_int_field(data.hdr.image_type);
  dump_int_field(data.hdr.color_map_first_index);
  dump_int_field(data.hdr.color_map_length);
  dump_int_field(data.hdr.color_map_bpp);
  dump_int_field(data.hdr.image_x_origin);
  dump_int_field(data.hdr.image_y_origin);
  dump_int_field(data.hdr.image_width);
  dump_int_field(data.hdr.image_height);
  dump_int_field(data.hdr.image_bpp);
  dump_int_field(data.hdr.image_descriptor);

  data.image_width = holy_le_to_cpu16 (data.hdr.image_width);
  data.image_height = holy_le_to_cpu16 (data.hdr.image_height);

  /* Check that bitmap encoding is supported.  */
  switch (data.hdr.image_type)
    {
    case holy_TGA_IMAGE_TYPE_RLE_TRUECOLOR:
    case holy_TGA_IMAGE_TYPE_RLE_BLACK_AND_WHITE:
    case holy_TGA_IMAGE_TYPE_RLE_INDEXCOLOR:
      data.uses_rle = 1;
      break;
    case holy_TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOR:
    case holy_TGA_IMAGE_TYPE_UNCOMPRESSED_BLACK_AND_WHITE:
    case holy_TGA_IMAGE_TYPE_UNCOMPRESSED_INDEXCOLOR:
      data.uses_rle = 0;
      break;

    default:
      holy_file_close (data.file);
      return holy_error (holy_ERR_BAD_FILE_TYPE,
			 "unsupported bitmap format (unknown encoding %d)", data.hdr.image_type);
    }

  data.bpp = data.hdr.image_bpp / 8;

  /* Check that bitmap depth is supported.  */
  switch (data.hdr.image_type)
    {
    case holy_TGA_IMAGE_TYPE_RLE_BLACK_AND_WHITE:
    case holy_TGA_IMAGE_TYPE_UNCOMPRESSED_BLACK_AND_WHITE:
      if (data.hdr.image_bpp != 8)
	{
	  holy_file_close (data.file);
	  return holy_error (holy_ERR_BAD_FILE_TYPE,
			     "unsupported bitmap format (bpp=%d)",
			     data.hdr.image_bpp);
	}
      holy_video_bitmap_create (bitmap, data.image_width,
				data.image_height,
				holy_VIDEO_BLIT_FORMAT_RGB_888);
      if (holy_errno != holy_ERR_NONE)
	{
	  holy_file_close (data.file);
	  return holy_errno;
	}

      data.bitmap = *bitmap;
      /* Load bitmap data.  */
      tga_load_grayscale (&data);
      break;

    case holy_TGA_IMAGE_TYPE_RLE_INDEXCOLOR:
    case holy_TGA_IMAGE_TYPE_UNCOMPRESSED_INDEXCOLOR:
      if (data.hdr.image_bpp != 8
	  || data.hdr.color_map_bpp != 24
	  || data.hdr.color_map_first_index != 0)
	{
	  holy_file_close (data.file);
	  return holy_error (holy_ERR_BAD_FILE_TYPE,
			     "unsupported bitmap format (bpp=%d)",
			     data.hdr.image_bpp);
	}
      holy_video_bitmap_create (bitmap, data.image_width,
				data.image_height,
				holy_VIDEO_BLIT_FORMAT_RGB_888);
      if (holy_errno != holy_ERR_NONE)
	{
	  holy_file_close (data.file);
	  return holy_errno;
	}

      data.bitmap = *bitmap;
      /* Load bitmap data.  */
      tga_load_palette (&data);
      tga_load_index_color (&data);
      break;

    case holy_TGA_IMAGE_TYPE_RLE_TRUECOLOR:
    case holy_TGA_IMAGE_TYPE_UNCOMPRESSED_TRUECOLOR:
      switch (data.hdr.image_bpp)
	{
	case 24:
	  holy_video_bitmap_create (bitmap, data.image_width,
				    data.image_height,
				    holy_VIDEO_BLIT_FORMAT_RGB_888);
	  if (holy_errno != holy_ERR_NONE)
	    {
	      holy_file_close (data.file);
	      return holy_errno;
	    }

	  data.bitmap = *bitmap;
	  /* Load bitmap data.  */
	  tga_load_truecolor_R8G8B8 (&data);
	  break;

	case 32:
	  holy_video_bitmap_create (bitmap, data.image_width,
				    data.image_height,
				    holy_VIDEO_BLIT_FORMAT_RGBA_8888);
	  if (holy_errno != holy_ERR_NONE)
	    {
	      holy_file_close (data.file);
	      return holy_errno;
	    }

	  data.bitmap = *bitmap;
	  /* Load bitmap data.  */
	  tga_load_truecolor_R8G8B8A8 (&data);
	  break;

	default:
	  holy_file_close (data.file);
	  return holy_error (holy_ERR_BAD_FILE_TYPE,
			     "unsupported bitmap format (bpp=%d)",
			     data.hdr.image_bpp);
	}
    }

  /* If there was a loading problem, destroy bitmap.  */
  if (holy_errno != holy_ERR_NONE)
    {
      holy_video_bitmap_destroy (*bitmap);
      *bitmap = 0;
    }

  holy_file_close (data.file);
  return holy_errno;
}

#if defined(TGA_DEBUG)
static holy_err_t
holy_cmd_tgatest (holy_command_t cmd_d __attribute__ ((unused)),
                  int argc, char **args)
{
  struct holy_video_bitmap *bitmap = 0;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "file name required");

  holy_video_reader_tga (&bitmap, args[0]);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  holy_video_bitmap_destroy (bitmap);

  return holy_ERR_NONE;
}
#endif

static struct holy_video_bitmap_reader tga_reader = {
  .extension = ".tga",
  .reader = holy_video_reader_tga,
  .next = 0
};

holy_MOD_INIT(tga)
{
  holy_video_bitmap_reader_register (&tga_reader);
#if defined(TGA_DEBUG)
  cmd = holy_register_command ("tgatest", holy_cmd_tgatest,
                               "FILE", "Tests loading of TGA bitmap.");
#endif
}

holy_MOD_FINI(tga)
{
#if defined(TGA_DEBUG)
  holy_unregister_command (cmd);
#endif
  holy_video_bitmap_reader_unregister (&tga_reader);
}
