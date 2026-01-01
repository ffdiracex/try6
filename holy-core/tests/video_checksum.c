/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/test.h>
#include <holy/dl.h>
#include <holy/video.h>
#include <holy/lib/crc.h>
#include <holy/mm.h>
#include <holy/term.h>
#ifdef holy_MACHINE_EMU
#include <holy/emu/hostdisk.h>
#include <holy/emu/misc.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

static int ctr;
static int nchk;
static char *basename;
static const holy_uint32_t *checksums;
static struct holy_video_mode_info capt_mode_info;

struct holy_video_mode_info holy_test_video_modes[holy_TEST_VIDEO_ALL_N_MODES] = {
  {
    .width = 640,
    .height = 480,
    .pitch = 640,
    .mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR,
    .bpp = 8,
    .bytes_per_pixel = 1,
    .number_of_colors = holy_VIDEO_FBSTD_NUMCOLORS
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 800,
    .mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR,
    .bpp = 8,
    .bytes_per_pixel = 1,
    .number_of_colors = holy_VIDEO_FBSTD_NUMCOLORS
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 1024,
    .mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR,
    .bpp = 8,
    .bytes_per_pixel = 1,
    .number_of_colors = holy_VIDEO_FBSTD_NUMCOLORS
  },
  {
    .width = 640,
    .height = 480,
    .pitch = 640 * 4,
    holy_VIDEO_MI_RGBA8888()
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 800 * 4,
    holy_VIDEO_MI_RGBA8888()
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 1024 * 4,
    holy_VIDEO_MI_RGBA8888()
  },
  {
    .width = 2560,
    .height = 1440,
    .pitch = 2560 * 4,
    holy_VIDEO_MI_RGBA8888()
  },
  {
    .width = 640,
    .height = 480,
    .pitch = 640,
    .mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR,
    .bpp = 8,
    .bytes_per_pixel = 1,
    .number_of_colors = holy_VIDEO_FBSTD_EXT_NUMCOLORS
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 800,
    .mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR,
    .bpp = 8,
    .bytes_per_pixel = 1,
    .number_of_colors = holy_VIDEO_FBSTD_EXT_NUMCOLORS
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 1024,
    .mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR,
    .bpp = 8,
    .bytes_per_pixel = 1,
    .number_of_colors = holy_VIDEO_FBSTD_EXT_NUMCOLORS
  },




  {
    .width = 640,
    .height = 480,
    .pitch = 1280,
    holy_VIDEO_MI_RGB555 ()
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 1600,
    holy_VIDEO_MI_RGB555 ()
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 2048,
    holy_VIDEO_MI_RGB555 ()
  },
  {
    .width = 640,
    .height = 480,
    .pitch = 1280,
    holy_VIDEO_MI_RGB565 ()
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 1600,
    holy_VIDEO_MI_RGB565 ()
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 2048,
    holy_VIDEO_MI_RGB565 ()
  },
  {
    .width = 640,
    .height = 480,
    .pitch = 640 * 3,
    holy_VIDEO_MI_RGB888 ()
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 800 * 3,
    holy_VIDEO_MI_RGB888 ()
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 1024 * 3,
    holy_VIDEO_MI_RGB888 ()
  },
  {
    .width = 640,
    .height = 480,
    .pitch = 1280,
    holy_VIDEO_MI_BGR555 ()
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 1600,
    holy_VIDEO_MI_BGR555 ()
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 2048,
    holy_VIDEO_MI_BGR555 ()
  },
  {
    .width = 640,
    .height = 480,
    .pitch = 1280,
    holy_VIDEO_MI_BGR565 ()
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 1600,
    holy_VIDEO_MI_BGR565 ()
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 2048,
    holy_VIDEO_MI_BGR565 ()
  },
  {
    .width = 640,
    .height = 480,
    .pitch = 640 * 3,
    holy_VIDEO_MI_BGR888 ()
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 800 * 3,
    holy_VIDEO_MI_BGR888 ()
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 1024 * 3,
    holy_VIDEO_MI_BGR888 ()
  },
  {
    .width = 640,
    .height = 480,
    .pitch = 640 * 4,
    holy_VIDEO_MI_BGRA8888()
  },
  {
    .width = 800,
    .height = 600,
    .pitch = 800 * 4,
    holy_VIDEO_MI_BGRA8888()
  },
  {
    .width = 1024,
    .height = 768,
    .pitch = 1024 * 4,
    holy_VIDEO_MI_BGRA8888()
  },
};

#ifdef holy_MACHINE_EMU
#include <holy/emu/hostfile.h>

struct bmp_header
{
  holy_uint8_t magic[2];
  holy_uint32_t filesize;
  holy_uint32_t reserved;
  holy_uint32_t bmp_off;
  holy_uint32_t head_size;
  holy_uint16_t width;
  holy_uint16_t height;
  holy_uint16_t planes;
  holy_uint16_t bpp;
} holy_PACKED;

static void
holy_video_capture_write_bmp (const char *fname,
			      void *ptr,
			      const struct holy_video_mode_info *mode_info)
{
  holy_util_fd_t fd = holy_util_fd_open (fname, holy_UTIL_FD_O_WRONLY | holy_UTIL_FD_O_CREATTRUNC);
  struct bmp_header head;

  if (!holy_UTIL_FD_IS_VALID (fd))
    {
      holy_printf (_("cannot open `%s': %s"),
		   fname, holy_util_fd_strerror ());
    }

  holy_memset (&head, 0, sizeof (head));

  head.magic[0] = 'B';
  head.magic[1] = 'M';

  if (mode_info->mode_type & holy_VIDEO_MODE_TYPE_RGB)
    {
      head.filesize = holy_cpu_to_le32 (sizeof (head) + mode_info->width * mode_info->height * 3);
      head.bmp_off = holy_cpu_to_le32_compile_time (sizeof (head));
      head.bpp = holy_cpu_to_le16_compile_time (24);
    }
  else
    {
      head.filesize = holy_cpu_to_le32 (sizeof (head) + 3 * 256 + mode_info->width * mode_info->height);
      head.bmp_off = holy_cpu_to_le32_compile_time (sizeof (head) + 3 * 256);
      head.bpp = holy_cpu_to_le16_compile_time (8);
    }
  head.head_size = holy_cpu_to_le32_compile_time (sizeof (head) - 14);
  head.width = holy_cpu_to_le16 (mode_info->width);
  head.height = holy_cpu_to_le16 (mode_info->height);
  head.planes = holy_cpu_to_le16_compile_time (1);

  head.width = holy_cpu_to_le16 (mode_info->width);
  head.height = holy_cpu_to_le16 (mode_info->height);

  holy_util_fd_write (fd, (char *) &head, sizeof (head));

  if (!(mode_info->mode_type & holy_VIDEO_MODE_TYPE_RGB))
    {
      struct holy_video_palette_data palette_data[256];
      int i;
      int palette_len = mode_info->number_of_colors;
      holy_memset (palette_data, 0, sizeof (palette_data));
      if (palette_len > 256)
	palette_len = 256;
      holy_video_get_palette (0, palette_len, palette_data);
      for (i = 0; i < 256; i++)
	{
	  holy_uint8_t r, g, b;
	  r = palette_data[i].r;
	  g = palette_data[i].g;
	  b = palette_data[i].b;

	  holy_util_fd_write (fd, (char *) &b, 1);
	  holy_util_fd_write (fd, (char *) &g, 1);
	  holy_util_fd_write (fd, (char *) &r, 1);
	}
    }

  /* This does essentialy the same as some fbblit functions yet using
     them would mean testing them against themselves so keep this
     implemetation separate.  */
  switch (mode_info->bytes_per_pixel)
    {
    case 4:
      {
	holy_uint8_t *buffer = xmalloc (mode_info->width * 3);
	holy_uint32_t rmask = ((1 << mode_info->red_mask_size) - 1);
	holy_uint32_t gmask = ((1 << mode_info->green_mask_size) - 1);
	holy_uint32_t bmask = ((1 << mode_info->blue_mask_size) - 1);
	int rshift = mode_info->red_field_pos;
	int gshift = mode_info->green_field_pos;
	int bshift = mode_info->blue_field_pos;
	int mulrshift = (8 - mode_info->red_mask_size);
	int mulgshift = (8 - mode_info->green_mask_size);
	int mulbshift = (8 - mode_info->blue_mask_size);
	int y;

	for (y = mode_info->height - 1; y >= 0; y--)
	  {
	    holy_uint32_t *iptr = (holy_uint32_t *) ptr + (mode_info->pitch / 4) * y;
	    int x;
	    holy_uint8_t *optr = buffer;
	    for (x = 0; x < (int) mode_info->width; x++)
	      {
		holy_uint32_t val = *iptr++;
		*optr++ = ((val >> bshift) & bmask) << mulbshift;
		*optr++ = ((val >> gshift) & gmask) << mulgshift;
		*optr++ = ((val >> rshift) & rmask) << mulrshift;
	      }
	    holy_util_fd_write (fd, (char *) buffer, mode_info->width * 3);
	  }
	holy_free (buffer);
	break;
      }
    case 3:
      {
	holy_uint8_t *buffer = xmalloc (mode_info->width * 3);
	holy_uint32_t rmask = ((1 << mode_info->red_mask_size) - 1);
	holy_uint32_t gmask = ((1 << mode_info->green_mask_size) - 1);
	holy_uint32_t bmask = ((1 << mode_info->blue_mask_size) - 1);
	int rshift = mode_info->red_field_pos;
	int gshift = mode_info->green_field_pos;
	int bshift = mode_info->blue_field_pos;
	int mulrshift = (8 - mode_info->red_mask_size);
	int mulgshift = (8 - mode_info->green_mask_size);
	int mulbshift = (8 - mode_info->blue_mask_size);
	int y;

	for (y = mode_info->height - 1; y >= 0; y--)
	  {
	    holy_uint8_t *iptr = ((holy_uint8_t *) ptr + mode_info->pitch * y);
	    int x;
	    holy_uint8_t *optr = buffer;
	    for (x = 0; x < (int) mode_info->width; x++)
	      {
		holy_uint32_t val = 0;
#ifdef holy_CPU_WORDS_BIGENDIAN
		val |= *iptr++ << 16;
		val |= *iptr++ << 8;
		val |= *iptr++;
#else
		val |= *iptr++;
		val |= *iptr++ << 8;
		val |= *iptr++ << 16;
#endif
		*optr++ = ((val >> bshift) & bmask) << mulbshift;
		*optr++ = ((val >> gshift) & gmask) << mulgshift;
		*optr++ = ((val >> rshift) & rmask) << mulrshift;
	      }
	    holy_util_fd_write (fd, (char *) buffer, mode_info->width * 3);
	  }
	holy_free (buffer);
	break;
      }
    case 2:
      {
	holy_uint8_t *buffer = xmalloc (mode_info->width * 3);
	holy_uint16_t rmask = ((1 << mode_info->red_mask_size) - 1);
	holy_uint16_t gmask = ((1 << mode_info->green_mask_size) - 1);
	holy_uint16_t bmask = ((1 << mode_info->blue_mask_size) - 1);
	int rshift = mode_info->red_field_pos;
	int gshift = mode_info->green_field_pos;
	int bshift = mode_info->blue_field_pos;
	int mulrshift = (8 - mode_info->red_mask_size);
	int mulgshift = (8 - mode_info->green_mask_size);
	int mulbshift = (8 - mode_info->blue_mask_size);
	int y;

	for (y = mode_info->height - 1; y >= 0; y--)
	  {
	    holy_uint16_t *iptr = (holy_uint16_t *) ptr + (mode_info->pitch / 2) * y;
	    int x;
	    holy_uint8_t *optr = buffer;
	    for (x = 0; x < (int) mode_info->width; x++)
	      {
		holy_uint16_t val = *iptr++;
		*optr++ = ((val >> bshift) & bmask) << mulbshift;
		*optr++ = ((val >> gshift) & gmask) << mulgshift;
		*optr++ = ((val >> rshift) & rmask) << mulrshift;
	      }
	    holy_util_fd_write (fd, (char *) buffer, mode_info->width * 3);
	  }
	holy_free (buffer);
	break;
      }
    case 1:
      {
	int y;

	for (y = mode_info->height - 1; y >= 0; y--)
	  holy_util_fd_write (fd, ((char *) ptr + mode_info->pitch * y),
			      mode_info->width);
	break;
      }
    }
  holy_util_fd_close (fd);
}

#endif

const char *
holy_video_checksum_get_modename (void)
{
  static char buf[40];
  if (capt_mode_info.mode_type & holy_VIDEO_MODE_TYPE_INDEX_COLOR)
    {
      holy_snprintf (buf, sizeof (buf), "i%d", capt_mode_info.number_of_colors);
      return buf;
    }
  if (capt_mode_info.red_field_pos == 0)
    {
      holy_snprintf (buf, sizeof (buf), "bgra%d%d%d%d", capt_mode_info.blue_mask_size,
		     capt_mode_info.green_mask_size,
		     capt_mode_info.red_mask_size,
		     capt_mode_info.reserved_mask_size);
      return buf;
    }
  holy_snprintf (buf, sizeof (buf), "rgba%d%d%d%d", capt_mode_info.red_mask_size,
		 capt_mode_info.green_mask_size,
		 capt_mode_info.blue_mask_size,
		 capt_mode_info.reserved_mask_size);
  return buf;
}

#define GENERATE_MODE 1
//#define SAVE_ALL_IMAGES
//#define COLLECT_TIME_STATISTICS 1

#if defined (GENERATE_MODE) && defined (holy_MACHINE_EMU)
holy_util_fd_t genfd = holy_UTIL_FD_INVALID;
#endif

#include <holy/time.h>

static void
write_time (void)
{
#if defined (holy_MACHINE_EMU) && defined (COLLECT_TIME_STATISTICS)
  char buf[60];
  static holy_uint64_t prev;
  holy_uint64_t cur;
  static holy_util_fd_t tmrfd = holy_UTIL_FD_INVALID;
  if (!holy_UTIL_FD_IS_VALID (tmrfd))
    tmrfd = holy_util_fd_open ("time.txt", holy_UTIL_FD_O_WRONLY
			       | holy_UTIL_FD_O_CREATTRUNC);

  cur = holy_util_get_cpu_time_ms ();
  holy_snprintf (buf, sizeof (buf), "%s_%dx%dx%s:%d: %" PRIuholy_UINT64_T " ms\n",
		 basename, 			
		 capt_mode_info.width,
		 capt_mode_info.height, 
		 holy_video_checksum_get_modename (), ctr,
		 cur - prev);
  prev = cur;
  if (holy_UTIL_FD_IS_VALID (tmrfd))
    holy_util_fd_write (tmrfd, buf, holy_strlen (buf));
#endif
}


static void
checksum (void)
{
  void *ptr;
  holy_uint32_t crc = 0;

  ptr = holy_video_capture_get_framebuffer ();

  write_time ();

#ifdef holy_CPU_WORDS_BIGENDIAN
  switch (capt_mode_info.bytes_per_pixel)
    {
    case 1:
      crc = holy_getcrc32c (0, ptr, capt_mode_info.pitch
			    * capt_mode_info.height);
      break;
    case 2:
      {
	unsigned x, y, rowskip;
	holy_uint8_t *iptr = ptr;
	crc = 0;
	rowskip = capt_mode_info.pitch - capt_mode_info.width * 2;
	for (y = 0; y < capt_mode_info.height; y++)
	  {
	    for (x = 0; x < capt_mode_info.width; x++)
	      {
		crc = holy_getcrc32c (crc, iptr + 1, 1);
		crc = holy_getcrc32c (crc, iptr, 1);
		iptr += 2;
	      }
	    crc = holy_getcrc32c (crc, iptr, rowskip);
	    iptr += rowskip;
	  }
	break;
      }
    case 3:
      {
	unsigned x, y, rowskip;
	holy_uint8_t *iptr = ptr;
	crc = 0;
	rowskip = capt_mode_info.pitch - capt_mode_info.width * 3;
	for (y = 0; y < capt_mode_info.height; y++)
	  {
	    for (x = 0; x < capt_mode_info.width; x++)
	      {
		crc = holy_getcrc32c (crc, iptr + 2, 1);
		crc = holy_getcrc32c (crc, iptr + 1, 1);
		crc = holy_getcrc32c (crc, iptr, 1);
		iptr += 3;
	      }
	    crc = holy_getcrc32c (crc, iptr, rowskip);
	    iptr += rowskip;
	  }
	break;
      }
    case 4:
      {
	unsigned x, y, rowskip;
	holy_uint8_t *iptr = ptr;
	crc = 0;
	rowskip = capt_mode_info.pitch - capt_mode_info.width * 4;
	for (y = 0; y < capt_mode_info.height; y++)
	  {
	    for (x = 0; x < capt_mode_info.width; x++)
	      {
		crc = holy_getcrc32c (crc, iptr + 3, 1);
		crc = holy_getcrc32c (crc, iptr + 2, 1);
		crc = holy_getcrc32c (crc, iptr + 1, 1);
		crc = holy_getcrc32c (crc, iptr, 1);
		iptr += 4;
	      }
	    crc = holy_getcrc32c (crc, iptr, rowskip);
	    iptr += rowskip;
	  }
	break;
      }
    }
#else
  crc = holy_getcrc32c (0, ptr, capt_mode_info.pitch * capt_mode_info.height);
#endif

#if defined (GENERATE_MODE) && defined (holy_MACHINE_EMU)
  if (holy_UTIL_FD_IS_VALID (genfd))
    {
      char buf[20];
      holy_snprintf (buf, sizeof (buf), "0x%x, ", crc);
      holy_util_fd_write (genfd, buf, holy_strlen (buf));
    }
#endif

  if (!checksums || ctr >= nchk)
    {
      holy_test_assert (0, "Unexpected checksum %s_%dx%dx%s:%d: 0x%x",
			basename, 			
			capt_mode_info.width,
			capt_mode_info.height, 
			holy_video_checksum_get_modename (), ctr, crc);
    }
  else if (crc != checksums[ctr])
    {
      holy_test_assert (0, "Checksum %s_%dx%dx%s:%d failed: 0x%x vs 0x%x",
			basename,
			capt_mode_info.width,
			capt_mode_info.height,
			holy_video_checksum_get_modename (),
			ctr, crc, checksums[ctr]);
    }
#if !(defined (SAVE_ALL_IMAGES) && defined (holy_MACHINE_EMU))
  else
    {
      write_time ();
      ctr++;
      return;
    }
#endif
#ifdef holy_MACHINE_EMU
  char *name = holy_xasprintf ("%s_%dx%dx%s_%d.bmp", basename,
			       capt_mode_info.width,
			       capt_mode_info.height,
			       holy_video_checksum_get_modename (),
			       ctr);
  holy_video_capture_write_bmp (name, ptr, &capt_mode_info);
  holy_free (name);
#endif

  write_time ();

  ctr++;
}

struct checksum_desc
{
  const char *name;
  unsigned width;
  unsigned height;
  unsigned mode_type;
  unsigned number_of_colors;
  unsigned bpp;
  unsigned bytes_per_pixel;
  unsigned red_field_pos;
  unsigned red_mask_size;
  unsigned green_field_pos;
  unsigned green_mask_size;
  unsigned blue_field_pos;
  unsigned blue_mask_size;
  unsigned reserved_field_pos;
  unsigned reserved_mask_size;
  const holy_uint32_t *checksums;
  int nchk;
};

const struct checksum_desc checksum_table[] = {
#include "checksums.h"
};

void
holy_video_checksum (const char *basename_in)
{
  unsigned i;

  holy_video_get_info (&capt_mode_info);

#if defined (GENERATE_MODE) && defined (holy_MACHINE_EMU)
  if (!holy_UTIL_FD_IS_VALID (genfd))
    genfd = holy_util_fd_open ("checksums.h", holy_UTIL_FD_O_WRONLY
			       | holy_UTIL_FD_O_CREATTRUNC);
  if (holy_UTIL_FD_IS_VALID (genfd))
    {
      char buf[400];

      holy_snprintf (buf, sizeof (buf), "\", %d, %d, 0x%x, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d /* %dx%dx%s */, (holy_uint32_t []) { ",
		     capt_mode_info.width,
		     capt_mode_info.height,
		     capt_mode_info.mode_type,
		     capt_mode_info.number_of_colors,
		     capt_mode_info.bpp,
		     capt_mode_info.bytes_per_pixel,
		     capt_mode_info.red_field_pos,
		     capt_mode_info.red_mask_size,
		     capt_mode_info.green_field_pos,
		     capt_mode_info.green_mask_size,
		     capt_mode_info.blue_field_pos,
		     capt_mode_info.blue_mask_size,
		     capt_mode_info.reserved_field_pos,
		     capt_mode_info.reserved_mask_size,
		     capt_mode_info.width,
		     capt_mode_info.height,
		     holy_video_checksum_get_modename ());

      holy_util_fd_write (genfd, "  { \"", 5);
      holy_util_fd_write (genfd, basename_in, holy_strlen (basename_in));
      holy_util_fd_write (genfd, buf, holy_strlen (buf));
    }
#endif

  basename = holy_strdup (basename_in);
  nchk = 0;
  checksums = 0;
  /* FIXME: optimize this.  */
  for (i = 0; i < ARRAY_SIZE (checksum_table); i++)
    if (holy_strcmp (checksum_table[i].name, basename_in) == 0
	&& capt_mode_info.width == checksum_table[i].width
	&& capt_mode_info.height == checksum_table[i].height
	&& capt_mode_info.mode_type == checksum_table[i].mode_type
	&& capt_mode_info.number_of_colors == checksum_table[i].number_of_colors
	&& capt_mode_info.bpp == checksum_table[i].bpp
	&& capt_mode_info.bytes_per_pixel == checksum_table[i].bytes_per_pixel
	&& capt_mode_info.red_field_pos == checksum_table[i].red_field_pos
	&& capt_mode_info.red_mask_size == checksum_table[i].red_mask_size
	&& capt_mode_info.green_field_pos == checksum_table[i].green_field_pos
	&& capt_mode_info.green_mask_size == checksum_table[i].green_mask_size
	&& capt_mode_info.blue_field_pos == checksum_table[i].blue_field_pos
	&& capt_mode_info.blue_mask_size == checksum_table[i].blue_mask_size
	&& capt_mode_info.reserved_field_pos == checksum_table[i].reserved_field_pos
	&& capt_mode_info.reserved_mask_size == checksum_table[i].reserved_mask_size)
      {
	nchk = checksum_table[i].nchk;
	checksums = checksum_table[i].checksums;
	break;
      }

  ctr = 0;
  holy_video_capture_refresh_cb = checksum;
}

void
holy_video_checksum_end (void)
{
#if defined (GENERATE_MODE) && defined (holy_MACHINE_EMU)
  if (holy_UTIL_FD_IS_VALID (genfd))
    {
      char buf[40];
      holy_snprintf (buf, sizeof (buf), "}, %d },\n", ctr);
      holy_util_fd_write (genfd, buf, holy_strlen (buf));
    }
#endif
  holy_test_assert (ctr == nchk, "Not enough checksums %s_%dx%dx%s: %d vs %d",
		    basename,
		    capt_mode_info.width,
		    capt_mode_info.height,
		    holy_video_checksum_get_modename (),
		    ctr, nchk);
  holy_free (basename);
  basename = 0;
  nchk = 0;
  checksums = 0;
  ctr = 0;
  holy_video_capture_refresh_cb = 0;
}

static struct holy_term_output *saved_outputs;
static struct holy_term_output *saved_gfxnext;
static struct holy_term_output *gfxterm;
static int use_gfxterm = 0;

int
holy_test_use_gfxterm (void)
{
  FOR_ACTIVE_TERM_OUTPUTS (gfxterm)
    if (holy_strcmp (gfxterm->name, "gfxterm") == 0)
      break;
  if (!gfxterm)
    FOR_DISABLED_TERM_OUTPUTS (gfxterm)
      if (holy_strcmp (gfxterm->name, "gfxterm") == 0)
	break;

  if (!gfxterm)
    {
      holy_test_assert (0, "terminal `%s' isn't found", "gfxterm");
      return 1;
    }

  if (gfxterm->init (gfxterm))
    { 
      holy_test_assert (0, "terminal `%s' failed: %s", "gfxterm", holy_errmsg);
      return 1;
    }

  saved_outputs = holy_term_outputs;
  saved_gfxnext = gfxterm->next;
  holy_term_outputs = gfxterm;
  gfxterm->next = 0;
  use_gfxterm = 1;

  return 0;
}

void
holy_test_use_gfxterm_end (void)
{
  if (!use_gfxterm)
    return;
  use_gfxterm = 0;
  gfxterm->fini (gfxterm);
  gfxterm->next = saved_gfxnext;
  holy_term_outputs = saved_outputs;
  saved_outputs = 0;
  saved_gfxnext = 0;
}
