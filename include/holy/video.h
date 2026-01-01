/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_VIDEO_HEADER
#define holy_VIDEO_HEADER	1

#include <holy/err.h>
#include <holy/types.h>
#include <holy/list.h>

/* Video color in hardware dependent format.  Users should not assume any
   specific coding format.  */
typedef holy_uint32_t holy_video_color_t;

/* Video color in hardware independent format.  */
typedef struct holy_video_rgba_color
{
  holy_uint8_t red;
  holy_uint8_t green;
  holy_uint8_t blue;
  holy_uint8_t alpha;
} holy_video_rgba_color_t;

/* This structure is driver specific and should not be accessed directly by
   outside code.  */
struct holy_video_render_target;

/* Forward declarations for used data structures.  */
struct holy_video_bitmap;

/* Defines used to describe video mode or rendering target.  */
/* If following is set render target contains currenly displayed image
   after swapping buffers (otherwise it contains previously displayed image).
 */
typedef enum holy_video_mode_type
  {
    holy_VIDEO_MODE_TYPE_RGB              = 0x00000001,
    holy_VIDEO_MODE_TYPE_INDEX_COLOR      = 0x00000002,
    holy_VIDEO_MODE_TYPE_1BIT_BITMAP      = 0x00000004,
    holy_VIDEO_MODE_TYPE_YUV              = 0x00000008,

    /* Defines used to mask flags.  */
    holy_VIDEO_MODE_TYPE_COLOR_MASK       = 0x0000000F,

    holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED  = 0x00000010,
    holy_VIDEO_MODE_TYPE_ALPHA            = 0x00000020,
    holy_VIDEO_MODE_TYPE_PURE_TEXT        = 0x00000040,
    holy_VIDEO_MODE_TYPE_UPDATING_SWAP    = 0x00000080,
    holy_VIDEO_MODE_TYPE_OPERATIONAL_MASK = 0x000000F0,

    /* Defines used to specify requested bit depth.  */
    holy_VIDEO_MODE_TYPE_DEPTH_MASK       = 0x0000FF00,
#define holy_VIDEO_MODE_TYPE_DEPTH_POS	  8

    holy_VIDEO_MODE_TYPE_UNKNOWN          = 0x00010000,
    holy_VIDEO_MODE_TYPE_HERCULES         = 0x00020000,
    holy_VIDEO_MODE_TYPE_PLANAR           = 0x00040000,
    holy_VIDEO_MODE_TYPE_NONCHAIN4        = 0x00080000,
    holy_VIDEO_MODE_TYPE_CGA              = 0x00100000,
    holy_VIDEO_MODE_TYPE_INFO_MASK        = 0x00FF0000,
  } holy_video_mode_type_t;

/* The basic render target representing the whole display.  This always
   renders to the back buffer when double-buffering is in use.  */
#define holy_VIDEO_RENDER_TARGET_DISPLAY \
  ((struct holy_video_render_target *) 0)

/* Defined blitting formats.  */
enum holy_video_blit_format
  {
    /* Generic RGBA, use fields & masks.  */
    holy_VIDEO_BLIT_FORMAT_RGBA,

    /* Optimized RGBA's.  */
    holy_VIDEO_BLIT_FORMAT_RGBA_8888,
    holy_VIDEO_BLIT_FORMAT_BGRA_8888,

    /* Generic RGB, use fields & masks.  */
    holy_VIDEO_BLIT_FORMAT_RGB,

    /* Optimized RGB's.  */
    holy_VIDEO_BLIT_FORMAT_RGB_888,
    holy_VIDEO_BLIT_FORMAT_BGR_888,
    holy_VIDEO_BLIT_FORMAT_RGB_565,
    holy_VIDEO_BLIT_FORMAT_BGR_565,

    /* When needed, decode color or just use value as is.  */
    holy_VIDEO_BLIT_FORMAT_INDEXCOLOR,
    /* Like index but only 16-colors and F0 is a special value for transparency.
       Could be extended to 4 bits of alpha and 4 bits of color if necessary.
       Used internally for text rendering.
     */
    holy_VIDEO_BLIT_FORMAT_INDEXCOLOR_ALPHA,

    /* Two color bitmap; bits packed: rows are not padded to byte boundary.  */
    holy_VIDEO_BLIT_FORMAT_1BIT_PACKED
  };

/* Define blitting operators.  */
enum holy_video_blit_operators
  {
    /* Replace target bitmap data with source.  */
    holy_VIDEO_BLIT_REPLACE,
    /* Blend target and source based on source's alpha value.  */
    holy_VIDEO_BLIT_BLEND
  };

struct holy_video_mode_info
{
  /* Width of the screen.  */
  unsigned int width;

  /* Height of the screen.  */
  unsigned int height;

  /* Mode type bitmask.  Contains information like is it Index color or
     RGB mode.  */
  holy_video_mode_type_t mode_type;

  /* Bits per pixel.  */
  unsigned int bpp;

  /* Bytes per pixel.  */
  unsigned int bytes_per_pixel;

  /* Pitch of one scanline.  How many bytes there are for scanline.  */
  unsigned int pitch;

  /* In index color mode, number of colors.  In RGB mode this is 256.  */
  unsigned int number_of_colors;

  unsigned int mode_number;
#define holy_VIDEO_MODE_NUMBER_INVALID 0xffffffff

  /* Optimization hint how binary data is coded.  */
  enum holy_video_blit_format blit_format;

  /* How many bits are reserved for red color.  */
  unsigned int red_mask_size;

  /* What is location of red color bits.  In Index Color mode, this is 0.  */
  unsigned int red_field_pos;

  /* How many bits are reserved for green color.  */
  unsigned int green_mask_size;

  /* What is location of green color bits.  In Index Color mode, this is 0.  */
  unsigned int green_field_pos;

  /* How many bits are reserved for blue color.  */
  unsigned int blue_mask_size;

  /* What is location of blue color bits.  In Index Color mode, this is 0.  */
  unsigned int blue_field_pos;

  /* How many bits are reserved in color.  */
  unsigned int reserved_mask_size;

  /* What is location of reserved color bits.  In Index Color mode,
     this is 0.  */
  unsigned int reserved_field_pos;

  /* For 1-bit bitmaps, the background color.  Used for bits = 0.  */
  holy_uint8_t bg_red;
  holy_uint8_t bg_green;
  holy_uint8_t bg_blue;
  holy_uint8_t bg_alpha;

  /* For 1-bit bitmaps, the foreground color.  Used for bits = 1.  */
  holy_uint8_t fg_red;
  holy_uint8_t fg_green;
  holy_uint8_t fg_blue;
  holy_uint8_t fg_alpha;
};

/* A 2D rectangle type.  */
struct holy_video_rect
{
  unsigned x;
  unsigned y;
  unsigned width;
  unsigned height;
};
typedef struct holy_video_rect holy_video_rect_t;

struct holy_video_signed_rect
{
  signed x;
  signed y;
  unsigned width;
  unsigned height;
};
typedef struct holy_video_signed_rect holy_video_signed_rect_t;

struct holy_video_palette_data
{
  holy_uint8_t r; /* Red color value (0-255).  */
  holy_uint8_t g; /* Green color value (0-255).  */
  holy_uint8_t b; /* Blue color value (0-255).  */
  holy_uint8_t a; /* Reserved bits value (0-255).  */
};

struct holy_video_edid_info
{
  holy_uint8_t header[8];
  holy_uint16_t manufacturer_id;
  holy_uint16_t product_id;
  holy_uint32_t serial_number;
  holy_uint8_t week_of_manufacture;
  holy_uint8_t year_of_manufacture;
  holy_uint8_t version;
  holy_uint8_t revision;

  holy_uint8_t video_input_definition;
  holy_uint8_t max_horizontal_image_size;
  holy_uint8_t max_vertical_image_size;
  holy_uint8_t display_gamma;
  holy_uint8_t feature_support;
#define holy_VIDEO_EDID_FEATURE_PREFERRED_TIMING_MODE	(1 << 1)

  holy_uint8_t red_green_lo;
  holy_uint8_t blue_white_lo;
  holy_uint8_t red_x_hi;
  holy_uint8_t red_y_hi;
  holy_uint8_t green_x_hi;
  holy_uint8_t green_y_hi;
  holy_uint8_t blue_x_hi;
  holy_uint8_t blue_y_hi;
  holy_uint8_t white_x_hi;
  holy_uint8_t white_y_hi;

  holy_uint8_t established_timings_1;
  holy_uint8_t established_timings_2;
  holy_uint8_t manufacturer_reserved_timings;

  holy_uint16_t standard_timings[8];

  struct {
    holy_uint16_t pixel_clock;
    /* Only valid if the pixel clock is non-null.  */
    holy_uint8_t horizontal_active_lo;
    holy_uint8_t horizontal_blanking_lo;
    holy_uint8_t horizontal_hi;
    holy_uint8_t vertical_active_lo;
    holy_uint8_t vertical_blanking_lo;
    holy_uint8_t vertical_hi;
    holy_uint8_t horizontal_sync_offset_lo;
    holy_uint8_t horizontal_sync_pulse_width_lo;
    holy_uint8_t vertical_sync_lo;
    holy_uint8_t sync_hi;
    holy_uint8_t horizontal_image_size_lo;
    holy_uint8_t vertical_image_size_lo;
    holy_uint8_t image_size_hi;
    holy_uint8_t horizontal_border;
    holy_uint8_t vertical_border;
    holy_uint8_t flags;
  } detailed_timings[4];

  holy_uint8_t extension_flag;
  holy_uint8_t checksum;
} holy_PACKED;

typedef enum holy_video_driver_id
  {
    holy_VIDEO_DRIVER_NONE,
    holy_VIDEO_DRIVER_VBE,
    holy_VIDEO_DRIVER_EFI_UGA,
    holy_VIDEO_DRIVER_EFI_GOP,
    holy_VIDEO_DRIVER_SM712,
    holy_VIDEO_DRIVER_VGA,
    holy_VIDEO_DRIVER_CIRRUS,
    holy_VIDEO_DRIVER_BOCHS,
    holy_VIDEO_DRIVER_SDL,
    holy_VIDEO_DRIVER_SIS315PRO,
    holy_VIDEO_DRIVER_RADEON_FULOONG2E,
    holy_VIDEO_DRIVER_COREBOOT,
    holy_VIDEO_DRIVER_IEEE1275,
    holy_VIDEO_ADAPTER_CAPTURE,
    holy_VIDEO_DRIVER_XEN,
    holy_VIDEO_DRIVER_RADEON_YEELOONG3A
  } holy_video_driver_id_t;

typedef enum holy_video_adapter_prio
  {
    holy_VIDEO_ADAPTER_PRIO_FALLBACK = 60,
    holy_VIDEO_ADAPTER_PRIO_FIRMWARE_DIRTY = 70,
    holy_VIDEO_ADAPTER_PRIO_FIRMWARE = 80,
    holy_VIDEO_ADAPTER_PRIO_NATIVE = 100
  } holy_video_adapter_prio_t;

typedef enum holy_video_area_status
  {
    holy_VIDEO_AREA_DISABLED,
    holy_VIDEO_AREA_ENABLED
  } holy_video_area_status_t;

struct holy_video_adapter
{
  /* The next video adapter.  */
  struct holy_video_adapter *next;
  struct holy_video_adapter **prev;

  /* The video adapter name.  */
  const char *name;
  holy_video_driver_id_t id;

  holy_video_adapter_prio_t prio;

  /* Initialize the video adapter.  */
  holy_err_t (*init) (void);

  /* Clean up the video adapter.  */
  holy_err_t (*fini) (void);

  holy_err_t (*setup) (unsigned int width,  unsigned int height,
                       holy_video_mode_type_t mode_type,
		       holy_video_mode_type_t mode_mask);

  holy_err_t (*get_info) (struct holy_video_mode_info *mode_info);

  holy_err_t (*get_info_and_fini) (struct holy_video_mode_info *mode_info,
				   void **framebuffer);

  holy_err_t (*set_palette) (unsigned int start, unsigned int count,
                             struct holy_video_palette_data *palette_data);

  holy_err_t (*get_palette) (unsigned int start, unsigned int count,
                             struct holy_video_palette_data *palette_data);

  holy_err_t (*set_viewport) (unsigned int x, unsigned int y,
                              unsigned int width, unsigned int height);

  holy_err_t (*get_viewport) (unsigned int *x, unsigned int *y,
                              unsigned int *width, unsigned int *height);

  holy_err_t (*set_region) (unsigned int x, unsigned int y,
                            unsigned int width, unsigned int height);

  holy_err_t (*get_region) (unsigned int *x, unsigned int *y,
                            unsigned int *width, unsigned int *height);

  holy_err_t (*set_area_status) (holy_video_area_status_t area_status);

  holy_err_t (*get_area_status) (holy_video_area_status_t *area_status);

  holy_video_color_t (*map_color) (holy_uint32_t color_name);

  holy_video_color_t (*map_rgb) (holy_uint8_t red, holy_uint8_t green,
                                 holy_uint8_t blue);

  holy_video_color_t (*map_rgba) (holy_uint8_t red, holy_uint8_t green,
                                  holy_uint8_t blue, holy_uint8_t alpha);

  holy_err_t (*unmap_color) (holy_video_color_t color,
                             holy_uint8_t *red, holy_uint8_t *green,
                             holy_uint8_t *blue, holy_uint8_t *alpha);

  holy_err_t (*fill_rect) (holy_video_color_t color, int x, int y,
                           unsigned int width, unsigned int height);

  holy_err_t (*blit_bitmap) (struct holy_video_bitmap *bitmap,
                             enum holy_video_blit_operators oper,
                             int x, int y, int offset_x, int offset_y,
                             unsigned int width, unsigned int height);

  holy_err_t (*blit_render_target) (struct holy_video_render_target *source,
                                    enum holy_video_blit_operators oper,
                                    int x, int y, int offset_x, int offset_y,
                                    unsigned int width, unsigned int height);

  holy_err_t (*scroll) (holy_video_color_t color, int dx, int dy);

  holy_err_t (*swap_buffers) (void);

  holy_err_t (*create_render_target) (struct holy_video_render_target **result,
                                      unsigned int width, unsigned int height,
                                      unsigned int mode_type);

  holy_err_t (*delete_render_target) (struct holy_video_render_target *target);

  holy_err_t (*set_active_render_target) (struct holy_video_render_target *target);

  holy_err_t (*get_active_render_target) (struct holy_video_render_target **target);

  int (*iterate) (int (*hook) (const struct holy_video_mode_info *info, void *hook_arg), void *hook_arg);

  holy_err_t (*get_edid) (struct holy_video_edid_info *edid_info);

  void (*print_adapter_specific_info) (void);
};
typedef struct holy_video_adapter *holy_video_adapter_t;

extern holy_video_adapter_t EXPORT_VAR(holy_video_adapter_list);

#ifndef holy_LST_GENERATOR
/* Register video driver.  */
static inline void
holy_video_register (holy_video_adapter_t adapter)
{
  holy_video_adapter_t *p;
  for (p = &holy_video_adapter_list; *p && (*p)->prio > adapter->prio;
       p = &((*p)->next));
  adapter->next = *p;
  *p = adapter;

  adapter->prev = p;
  if (adapter->next)
    adapter->next->prev = &adapter->next;
}
#endif

/* Unregister video driver.  */
static inline void
holy_video_unregister (holy_video_adapter_t adapter)
{
  holy_list_remove (holy_AS_LIST (adapter));
}

#define FOR_VIDEO_ADAPTERS(var) FOR_LIST_ELEMENTS((var), (holy_video_adapter_list))

holy_err_t EXPORT_FUNC (holy_video_restore) (void);

holy_err_t EXPORT_FUNC (holy_video_get_info) (struct holy_video_mode_info *mode_info);

/* Framebuffer address may change as a part of normal operation
   (e.g. double buffering). That's why you need to stop video subsystem to be
   sure that framebuffer address doesn't change. To ensure this abstraction
   holy_video_get_info_and_fini is the only function supplying framebuffer
   address. */
holy_err_t EXPORT_FUNC (holy_video_get_info_and_fini) (struct holy_video_mode_info *mode_info,
					 void **framebuffer);

enum holy_video_blit_format EXPORT_FUNC(holy_video_get_blit_format) (struct holy_video_mode_info *mode_info);

holy_err_t holy_video_set_palette (unsigned int start, unsigned int count,
                                   struct holy_video_palette_data *palette_data);

holy_err_t EXPORT_FUNC (holy_video_get_palette) (unsigned int start,
						 unsigned int count,
						 struct holy_video_palette_data *palette_data);

holy_err_t EXPORT_FUNC (holy_video_set_viewport) (unsigned int x,
						  unsigned int y,
						  unsigned int width,
						  unsigned int height);

holy_err_t EXPORT_FUNC (holy_video_get_viewport) (unsigned int *x,
						  unsigned int *y,
						  unsigned int *width,
						  unsigned int *height);

holy_err_t EXPORT_FUNC (holy_video_set_region) (unsigned int x,
                                                unsigned int y,
                                                unsigned int width,
                                                unsigned int height);

holy_err_t EXPORT_FUNC (holy_video_get_region) (unsigned int *x,
                                                unsigned int *y,
                                                unsigned int *width,
                                                unsigned int *height);

holy_err_t EXPORT_FUNC (holy_video_set_area_status)
    (holy_video_area_status_t area_status);

holy_err_t EXPORT_FUNC (holy_video_get_area_status)
    (holy_video_area_status_t *area_status);

holy_video_color_t EXPORT_FUNC (holy_video_map_color) (holy_uint32_t color_name);

holy_video_color_t EXPORT_FUNC (holy_video_map_rgb) (holy_uint8_t red,
						     holy_uint8_t green,
						     holy_uint8_t blue);

holy_video_color_t EXPORT_FUNC (holy_video_map_rgba) (holy_uint8_t red,
						      holy_uint8_t green,
						      holy_uint8_t blue,
						      holy_uint8_t alpha);

holy_err_t EXPORT_FUNC (holy_video_unmap_color) (holy_video_color_t color,
						 holy_uint8_t *red,
						 holy_uint8_t *green,
						 holy_uint8_t *blue,
						 holy_uint8_t *alpha);

holy_err_t EXPORT_FUNC (holy_video_fill_rect) (holy_video_color_t color,
					       int x, int y,
					       unsigned int width,
					       unsigned int height);

holy_err_t EXPORT_FUNC (holy_video_blit_bitmap) (struct holy_video_bitmap *bitmap,
						 enum holy_video_blit_operators oper,
						 int x, int y,
						 int offset_x, int offset_y,
						 unsigned int width,
						 unsigned int height);

holy_err_t EXPORT_FUNC (holy_video_blit_render_target) (struct holy_video_render_target *source,
							enum holy_video_blit_operators oper,
							int x, int y,
							int offset_x,
							int offset_y,
							unsigned int width,
							unsigned int height);

holy_err_t holy_video_scroll (holy_video_color_t color, int dx, int dy);

holy_err_t EXPORT_FUNC (holy_video_swap_buffers) (void);

holy_err_t EXPORT_FUNC (holy_video_create_render_target) (struct holy_video_render_target **result,
							  unsigned int width,
							  unsigned int height,
							  unsigned int mode_type);

holy_err_t EXPORT_FUNC (holy_video_delete_render_target) (struct holy_video_render_target *target);

holy_err_t EXPORT_FUNC (holy_video_set_active_render_target) (struct holy_video_render_target *target);

holy_err_t holy_video_get_active_render_target (struct holy_video_render_target **target);

holy_err_t EXPORT_FUNC (holy_video_edid_checksum) (struct holy_video_edid_info *edid_info);
holy_err_t EXPORT_FUNC (holy_video_edid_preferred_mode) (struct holy_video_edid_info *edid_info,
					   unsigned int *width,
					   unsigned int *height);

holy_err_t EXPORT_FUNC (holy_video_set_mode) (const char *modestring,
					      unsigned int modemask,
					      unsigned int modevalue);

static inline int
holy_video_check_mode_flag (holy_video_mode_type_t flags,
			    holy_video_mode_type_t mask,
			    holy_video_mode_type_t flag, int def)
{
  return (flag & mask) ? !! (flags & flag) : def;
}

holy_video_driver_id_t EXPORT_FUNC (holy_video_get_driver_id) (void);

static __inline holy_video_rgba_color_t
holy_video_rgba_color_rgb (holy_uint8_t r, holy_uint8_t g, holy_uint8_t b)
{
  holy_video_rgba_color_t c;
  c.red = r;
  c.green = g;
  c.blue = b;
  c.alpha = 255;
  return c;
}

static __inline holy_video_color_t
holy_video_map_rgba_color (holy_video_rgba_color_t c)
{
  return holy_video_map_rgba (c.red, c.green, c.blue, c.alpha);
}

#ifndef holy_MACHINE_EMU
extern void holy_font_init (void);
extern void holy_font_fini (void);
extern void holy_gfxterm_init (void);
extern void holy_gfxterm_fini (void);
extern void holy_video_sm712_init (void);
extern void holy_video_sm712_fini (void);
extern void holy_video_sis315pro_init (void);
extern void holy_video_radeon_fuloong2e_init (void);
extern void holy_video_sis315pro_fini (void);
extern void holy_video_radeon_fuloong2e_fini (void);
extern void holy_video_radeon_yeeloong3a_init (void);
extern void holy_video_radeon_yeeloong3a_fini (void);
#endif

void
holy_video_set_adapter (holy_video_adapter_t adapter);
holy_video_adapter_t
holy_video_get_adapter (void);
holy_err_t
holy_video_capture_start (const struct holy_video_mode_info *mode_info,
			  struct holy_video_palette_data *palette,
			  unsigned int palette_size);
void
holy_video_capture_end (void);

void *
holy_video_capture_get_framebuffer (void);

extern holy_video_adapter_t EXPORT_VAR (holy_video_adapter_active);
extern void (*holy_video_capture_refresh_cb) (void);

#define holy_VIDEO_MI_RGB555(x)						\
  x.mode_type = holy_VIDEO_MODE_TYPE_RGB,				\
    x.bpp = 15,								\
    x.bytes_per_pixel = 2,						\
    x.number_of_colors = 256,						\
    x.red_mask_size = 5,						\
    x.red_field_pos = 10,						\
    x.green_mask_size = 5,						\
    x.green_field_pos = 5,						\
    x.blue_mask_size = 5,						\
    x.blue_field_pos = 0

#define holy_VIDEO_MI_RGB565(x)						\
  x.mode_type = holy_VIDEO_MODE_TYPE_RGB,				\
    x.bpp = 16,								\
    x.bytes_per_pixel = 2,						\
    x.number_of_colors = 256,						\
    x.red_mask_size = 5,						\
    x.red_field_pos = 11,						\
    x.green_mask_size = 6,						\
    x.green_field_pos = 5,						\
    x.blue_mask_size = 5,						\
    x.blue_field_pos = 0

#define holy_VIDEO_MI_RGB888(x) \
  x.mode_type = holy_VIDEO_MODE_TYPE_RGB,			\
    x.bpp = 24,							\
    x.bytes_per_pixel = 3,					\
    x.number_of_colors = 256,					\
    x.red_mask_size = 8,					\
    x.red_field_pos = 16,					\
    x.green_mask_size = 8,					\
    x.green_field_pos = 8,					\
    x.blue_mask_size = 8,					\
    x.blue_field_pos = 0

#define holy_VIDEO_MI_RGBA8888(x) \
  x.mode_type = holy_VIDEO_MODE_TYPE_RGB,	\
    x.bpp = 32,					\
    x.bytes_per_pixel = 4,			\
    x.number_of_colors = 256,			\
    x.reserved_mask_size = 8,			\
    x.reserved_field_pos = 24,			\
    x.red_mask_size = 8,			\
    x.red_field_pos = 16,			\
    x.green_mask_size = 8,			\
    x.green_field_pos = 8,			\
    x.blue_mask_size = 8,			\
    x.blue_field_pos = 0


#define holy_VIDEO_MI_BGR555(x)						\
  x.mode_type = holy_VIDEO_MODE_TYPE_RGB,				\
    x.bpp = 15,								\
    x.bytes_per_pixel = 2,						\
    x.number_of_colors = 256,						\
    x.red_mask_size = 5,						\
    x.red_field_pos = 0,						\
    x.green_mask_size = 5,						\
    x.green_field_pos = 5,						\
    x.blue_mask_size = 5,						\
    x.blue_field_pos = 10

#define holy_VIDEO_MI_BGR565(x)						\
  x.mode_type = holy_VIDEO_MODE_TYPE_RGB,				\
    x.bpp = 16,								\
    x.bytes_per_pixel = 2,						\
    x.number_of_colors = 256,						\
    x.red_mask_size = 5,						\
    x.red_field_pos = 0,						\
    x.green_mask_size = 6,						\
    x.green_field_pos = 5,						\
    x.blue_mask_size = 5,						\
    x.blue_field_pos = 11

#define holy_VIDEO_MI_BGR888(x) \
  x.mode_type = holy_VIDEO_MODE_TYPE_RGB,			\
    x.bpp = 24,							\
    x.bytes_per_pixel = 3,					\
    x.number_of_colors = 256,					\
    x.red_mask_size = 8,					\
    x.red_field_pos = 0,					\
    x.green_mask_size = 8,					\
    x.green_field_pos = 8,					\
    x.blue_mask_size = 8,					\
    x.blue_field_pos = 16

#define holy_VIDEO_MI_BGRA8888(x) \
  x.mode_type = holy_VIDEO_MODE_TYPE_RGB,	\
    x.bpp = 32,					\
    x.bytes_per_pixel = 4,			\
    x.number_of_colors = 256,			\
    x.reserved_mask_size = 8,			\
    x.reserved_field_pos = 24,			\
    x.red_mask_size = 8,			\
    x.red_field_pos = 0,			\
    x.green_mask_size = 8,			\
    x.green_field_pos = 8,			\
    x.blue_mask_size = 8,			\
    x.blue_field_pos = 16

#endif /* ! holy_VIDEO_HEADER */
