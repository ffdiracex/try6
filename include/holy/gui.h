/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/err.h>
#include <holy/video.h>
#include <holy/bitmap.h>
#include <holy/gfxmenu_view.h>
#include <holy/mm.h>

#ifndef holy_GUI_H
#define holy_GUI_H 1

/* The component ID identifying GUI components to be updated as the timeout
   status changes.  */
#define holy_GFXMENU_TIMEOUT_COMPONENT_ID "__timeout__"

typedef struct holy_gui_component *holy_gui_component_t;
typedef struct holy_gui_container *holy_gui_container_t;
typedef struct holy_gui_list *holy_gui_list_t;

typedef void (*holy_gui_component_callback) (holy_gui_component_t component,
                                             void *userdata);

/* Component interface.  */

struct holy_gui_component_ops
{
  void (*destroy) (void *self);
  const char * (*get_id) (void *self);
  int (*is_instance) (void *self, const char *type);
  void (*paint) (void *self, const holy_video_rect_t *bounds);
  void (*set_parent) (void *self, holy_gui_container_t parent);
  holy_gui_container_t (*get_parent) (void *self);
  void (*set_bounds) (void *self, const holy_video_rect_t *bounds);
  void (*get_bounds) (void *self, holy_video_rect_t *bounds);
  void (*get_minimal_size) (void *self, unsigned *width, unsigned *height);
  holy_err_t (*set_property) (void *self, const char *name, const char *value);
  void (*repaint) (void *self, int second_pass);
};

struct holy_gui_container_ops
{
  void (*add) (void *self, holy_gui_component_t comp);
  void (*remove) (void *self, holy_gui_component_t comp);
  void (*iterate_children) (void *self,
                            holy_gui_component_callback cb, void *userdata);
};

struct holy_gui_list_ops
{
  void (*set_view_info) (void *self,
                         holy_gfxmenu_view_t view);
  void (*refresh_list) (void *self,
                        holy_gfxmenu_view_t view);
};

struct holy_gui_progress_ops
{
  void (*set_state) (void *self, int visible, int start, int current, int end);
};

typedef void (*holy_gfxmenu_set_state_t) (void *self, int visible, int start,
					  int current, int end);

struct holy_gfxmenu_timeout_notify
{
  struct holy_gfxmenu_timeout_notify *next;
  holy_gfxmenu_set_state_t set_state;
  holy_gui_component_t self;
};

extern struct holy_gfxmenu_timeout_notify *holy_gfxmenu_timeout_notifications;

static inline holy_err_t
holy_gfxmenu_timeout_register (holy_gui_component_t self,
			       holy_gfxmenu_set_state_t set_state)
{
  struct holy_gfxmenu_timeout_notify *ne = holy_malloc (sizeof (*ne));
  if (!ne)
    return holy_errno;
  ne->set_state = set_state;
  ne->self = self;
  ne->next = holy_gfxmenu_timeout_notifications;
  holy_gfxmenu_timeout_notifications = ne;
  return holy_ERR_NONE;
}

static inline void
holy_gfxmenu_timeout_unregister (holy_gui_component_t self)
{
  struct holy_gfxmenu_timeout_notify **p, *q;

  for (p = &holy_gfxmenu_timeout_notifications, q = *p;
       q; p = &(q->next), q = q->next)
    if (q->self == self)
      {
	*p = q->next;
	holy_free (q);
	break;
      }
}

typedef signed holy_fixed_signed_t;
#define holy_FIXED_1 0x10000

/* Special care is taken to round to nearest integer and not just truncate.  */
static inline signed
holy_divide_round (signed a, signed b)
{
  int neg = 0;
  signed ret;
  if (b < 0)
    {
      b = -b;
      neg = !neg;
    }
  if (a < 0)
    {
      a = -a;
      neg = !neg;
    }
  ret = (unsigned) (a + b / 2) / (unsigned) b;
  return neg ? -ret : ret;
}

static inline signed
holy_fixed_sfs_divide (signed a, holy_fixed_signed_t b)
{
  return holy_divide_round (a * holy_FIXED_1, b);
}

static inline holy_fixed_signed_t
holy_fixed_fsf_divide (holy_fixed_signed_t a, signed b)
{
  return holy_divide_round (a, b);
}

static inline signed
holy_fixed_sfs_multiply (signed a, holy_fixed_signed_t b)
{
  return (a * b) / holy_FIXED_1;
}

static inline signed
holy_fixed_to_signed (holy_fixed_signed_t in)
{
  return in / holy_FIXED_1;
}

static inline holy_fixed_signed_t
holy_signed_to_fixed (signed in)
{
  return in * holy_FIXED_1;
}

struct holy_gui_component
{
  struct holy_gui_component_ops *ops;
  signed x;
  holy_fixed_signed_t xfrac;
  signed y;
  holy_fixed_signed_t yfrac;
  signed w;
  holy_fixed_signed_t wfrac;
  signed h;
  holy_fixed_signed_t hfrac;
};

struct holy_gui_progress
{
  struct holy_gui_component component;
  struct holy_gui_progress_ops *ops;
};

struct holy_gui_container
{
  struct holy_gui_component component;
  struct holy_gui_container_ops *ops;
};

struct holy_gui_list
{
  struct holy_gui_component component;
  struct holy_gui_list_ops *ops;
};


/* Interfaces to concrete component classes.  */

holy_gui_container_t holy_gui_canvas_new (void);
holy_gui_container_t holy_gui_vbox_new (void);
holy_gui_container_t holy_gui_hbox_new (void);
holy_gui_component_t holy_gui_label_new (void);
holy_gui_component_t holy_gui_image_new (void);
holy_gui_component_t holy_gui_progress_bar_new (void);
holy_gui_component_t holy_gui_list_new (void);
holy_gui_component_t holy_gui_circular_progress_new (void);

/* Manipulation functions.  */

/* Visit all components with the specified ID.  */
void holy_gui_find_by_id (holy_gui_component_t root,
                          const char *id,
                          holy_gui_component_callback cb,
                          void *userdata);

/* Visit all components.  */
void holy_gui_iterate_recursively (holy_gui_component_t root,
                                   holy_gui_component_callback cb,
                                   void *userdata);

/* Helper functions.  */

static __inline void
holy_gui_save_viewport (holy_video_rect_t *r)
{
  holy_video_get_viewport ((unsigned *) &r->x,
                           (unsigned *) &r->y,
                           (unsigned *) &r->width,
                           (unsigned *) &r->height);
}

static __inline void
holy_gui_restore_viewport (const holy_video_rect_t *r)
{
  holy_video_set_viewport (r->x, r->y, r->width, r->height);
}

/* Set a new viewport relative the the current one, saving the current
   viewport in OLD so it can be later restored.  */
static __inline void
holy_gui_set_viewport (const holy_video_rect_t *r, holy_video_rect_t *old)
{
  holy_gui_save_viewport (old);
  holy_video_set_viewport (old->x + r->x,
                           old->y + r->y,
                           r->width,
                           r->height);
}

static inline int
holy_video_have_common_points (const holy_video_rect_t *a,
			       const holy_video_rect_t *b)
{
  if (!((a->x <= b->x && b->x <= a->x + a->width)
	|| (b->x <= a->x && a->x <= b->x + b->width)))
    return 0;
  if (!((a->y <= b->y && b->y <= a->y + a->height)
	|| (b->y <= a->y && a->y <= b->y + b->height)))
    return 0;
  return 1;
}

static inline int
holy_video_bounds_inside_region (const holy_video_rect_t *b,
                                 const holy_video_rect_t *r)
{
  if (r->x > b->x || r->x + r->width < b->x + b->width)
    return 0;
  if (r->y > b->y || r->y + r->height < b->y + b->height)
    return 0;
  return 1;
}

#endif /* ! holy_GUI_H */
