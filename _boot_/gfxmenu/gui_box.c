/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/gui.h>
#include <holy/gui_string_util.h>

struct component_node
{
  holy_gui_component_t component;
  struct component_node *next;
  struct component_node *prev;
};

typedef struct holy_gui_box *holy_gui_box_t;

typedef void (*layout_func_t) (holy_gui_box_t self, int modify_layout,
                               unsigned *minimal_width,
			       unsigned *minimal_height);

struct holy_gui_box
{
  struct holy_gui_container container;

  holy_gui_container_t parent;
  holy_video_rect_t bounds;
  char *id;

  /* Doubly linked list of components with dummy head & tail nodes.  */
  struct component_node chead;
  struct component_node ctail;

  /* The layout function: differs for vertical and horizontal boxes.  */
  layout_func_t layout_func;
};

static void
box_destroy (void *vself)
{
  holy_gui_box_t self = vself;
  struct component_node *cur;
  struct component_node *next;
  for (cur = self->chead.next; cur != &self->ctail; cur = next)
    {
      /* Copy the 'next' pointer, since we need it for the next iteration,
         and we're going to free the memory it is stored in.  */
      next = cur->next;
      /* Destroy the child component.  */
      cur->component->ops->destroy (cur->component);
      /* Free the linked list node.  */
      holy_free (cur);
    }
  holy_free (self);
}

static const char *
box_get_id (void *vself)
{
  holy_gui_box_t self = vself;
  return self->id;
}

static int
box_is_instance (void *vself __attribute__((unused)), const char *type)
{
  return (holy_strcmp (type, "component") == 0
          || holy_strcmp (type, "container") == 0);
}

static void
layout_horizontally (holy_gui_box_t self, int modify_layout,
                     unsigned *min_width, unsigned *min_height)
{
  /* Start at the left (chead) and set the x coordinates as we go right.  */
  /* All components have their width set to the box's width.  */

  struct component_node *cur;
  unsigned w = 0, mwfrac = 0, h = 0, x = 0;
  holy_fixed_signed_t wfrac = 0;
  int bogus_frac = 0;

  for (cur = self->chead.next; cur != &self->ctail; cur = cur->next)
    {
      holy_gui_component_t c = cur->component;
      unsigned mw = 0, mh = 0;

      if (c->ops->get_minimal_size)
	c->ops->get_minimal_size (c, &mw, &mh);

      if (c->h > (signed) h)
	h = c->h;
      if (mh > h)
	h = mh;
      wfrac += c->wfrac;
      w += c->w;
      if (mw - c->w > 0)
	mwfrac += mw - c->w;
    }
  if (wfrac > holy_FIXED_1 || (w > 0 && wfrac == holy_FIXED_1))
    bogus_frac = 1;

  if (min_width)
    {
      if (wfrac < holy_FIXED_1)
	*min_width = holy_fixed_sfs_divide (w, holy_FIXED_1 - wfrac);
      else
	*min_width = w;
      if (*min_width < w + mwfrac)
	*min_width = w + mwfrac;
    }
  if (min_height)
    *min_height = h;

  if (!modify_layout)
    return;

  for (cur = self->chead.next; cur != &self->ctail; cur = cur->next)
    {
      holy_video_rect_t r;
      holy_gui_component_t c = cur->component;
      unsigned mw = 0, mh = 0;

      r.x = x;
      r.y = 0;
      r.height = h;

      if (c->ops->get_minimal_size)
	c->ops->get_minimal_size (c, &mw, &mh);

      r.width = c->w;
      if (!bogus_frac)
	r.width += holy_fixed_sfs_multiply (self->bounds.width, c->wfrac);

      if (r.width < mw)
	r.width = mw;

      c->ops->set_bounds (c, &r);

      x += r.width;
    }
}

static void
layout_vertically (holy_gui_box_t self, int modify_layout,
                     unsigned *min_width, unsigned *min_height)
{
  /* Start at the top (chead) and set the y coordinates as we go rdown.  */
  /* All components have their height set to the box's height.  */

  struct component_node *cur;
  unsigned h = 0, mhfrac = 0, w = 0, y = 0;
  holy_fixed_signed_t hfrac = 0;
  int bogus_frac = 0;

  for (cur = self->chead.next; cur != &self->ctail; cur = cur->next)
    {
      holy_gui_component_t c = cur->component;
      unsigned mw = 0, mh = 0;

      if (c->ops->get_minimal_size)
	c->ops->get_minimal_size (c, &mw, &mh);

      if (c->w > (signed) w)
	w = c->w;
      if (mw > w)
	w = mw;
      hfrac += c->hfrac;
      h += c->h;
      if (mh - c->h > 0)
	mhfrac += mh - c->h;
    }
  if (hfrac > holy_FIXED_1 || (h > 0 && hfrac == holy_FIXED_1))
    bogus_frac = 1;

  if (min_height)
    {
      if (hfrac < holy_FIXED_1)
	*min_height = holy_fixed_sfs_divide (h, holy_FIXED_1 - hfrac);
      else
	*min_height = h;
      if (*min_height < h + mhfrac)
	*min_height = h + mhfrac;
    }
  if (min_width)
    *min_width = w;

  if (!modify_layout)
    return;

  for (cur = self->chead.next; cur != &self->ctail; cur = cur->next)
    {
      holy_video_rect_t r;
      holy_gui_component_t c = cur->component;
      unsigned mw = 0, mh = 0;

      r.x = 0;
      r.y = y;
      r.width = w;

      if (c->ops->get_minimal_size)
	c->ops->get_minimal_size (c, &mw, &mh);

      r.height = c->h;
      if (!bogus_frac)
	r.height += holy_fixed_sfs_multiply (self->bounds.height, c->hfrac);

      if (r.height < mh)
	r.height = mh;

      c->ops->set_bounds (c, &r);

      y += r.height;
    }
}

static void
box_paint (void *vself, const holy_video_rect_t *region)
{
  holy_gui_box_t self = vself;

  struct component_node *cur;
  holy_video_rect_t vpsave;

  holy_video_area_status_t box_area_status;
  holy_video_get_area_status (&box_area_status);

  holy_gui_set_viewport (&self->bounds, &vpsave);
  for (cur = self->chead.next; cur != &self->ctail; cur = cur->next)
    {
      holy_gui_component_t comp = cur->component;
      holy_video_rect_t r;
      comp->ops->get_bounds(comp, &r);

      if (!holy_video_have_common_points (region, &r))
        continue;

      /* Paint the child.  */
      if (box_area_status == holy_VIDEO_AREA_ENABLED
          && holy_video_bounds_inside_region (&r, region))
        holy_video_set_area_status (holy_VIDEO_AREA_DISABLED);
      comp->ops->paint (comp, region);
      if (box_area_status == holy_VIDEO_AREA_ENABLED)
        holy_video_set_area_status (holy_VIDEO_AREA_ENABLED);
    }
  holy_gui_restore_viewport (&vpsave);
}

static void
box_set_parent (void *vself, holy_gui_container_t parent)
{
  holy_gui_box_t self = vself;
  self->parent = parent;
}

static holy_gui_container_t
box_get_parent (void *vself)
{
  holy_gui_box_t self = vself;
  return self->parent;
}

static void
box_set_bounds (void *vself, const holy_video_rect_t *bounds)
{
  holy_gui_box_t self = vself;
  self->bounds = *bounds;
  self->layout_func (self, 1, 0, 0);   /* Relayout the children.  */
}

static void
box_get_bounds (void *vself, holy_video_rect_t *bounds)
{
  holy_gui_box_t self = vself;
  *bounds = self->bounds;
}

/* The box's preferred size is based on the preferred sizes
   of its children.  */
static void
box_get_minimal_size (void *vself, unsigned *width, unsigned *height)
{
  holy_gui_box_t self = vself;
  self->layout_func (self, 0, width, height);   /* Just calculate the size.  */
}

static holy_err_t
box_set_property (void *vself, const char *name, const char *value)
{
  holy_gui_box_t self = vself;
  if (holy_strcmp (name, "id") == 0)
    {
      holy_free (self->id);
      if (value)
        {
          self->id = holy_strdup (value);
          if (! self->id)
            return holy_errno;
        }
      else
        self->id = 0;
    }

  return holy_errno;
}

static void
box_add (void *vself, holy_gui_component_t comp)
{
  holy_gui_box_t self = vself;
  struct component_node *node;
  node = holy_malloc (sizeof (*node));
  if (! node)
    return;   /* Note: probably should handle the error.  */
  node->component = comp;
  /* Insert the node before the tail.  */
  node->prev = self->ctail.prev;
  node->prev->next = node;
  node->next = &self->ctail;
  node->next->prev = node;

  comp->ops->set_parent (comp, (holy_gui_container_t) self);
  self->layout_func (self, 1, 0, 0);   /* Relayout the children.  */
}

static void
box_remove (void *vself, holy_gui_component_t comp)
{
  holy_gui_box_t self = vself;
  struct component_node *cur;
  for (cur = self->chead.next; cur != &self->ctail; cur = cur->next)
    {
      if (cur->component == comp)
        {
          /* Unlink 'cur' from the list.  */
          cur->prev->next = cur->next;
          cur->next->prev = cur->prev;
          /* Free the node's memory (but don't destroy the component).  */
          holy_free (cur);
          /* Must not loop again, since 'cur' would be dereferenced!  */
          return;
        }
    }
}

static void
box_iterate_children (void *vself,
                      holy_gui_component_callback cb, void *userdata)
{
  holy_gui_box_t self = vself;
  struct component_node *cur;
  for (cur = self->chead.next; cur != &self->ctail; cur = cur->next)
    cb (cur->component, userdata);
}

static struct holy_gui_component_ops box_comp_ops =
  {
    .destroy = box_destroy,
    .get_id = box_get_id,
    .is_instance = box_is_instance,
    .paint = box_paint,
    .set_parent = box_set_parent,
    .get_parent = box_get_parent,
    .set_bounds = box_set_bounds,
    .get_bounds = box_get_bounds,
    .get_minimal_size = box_get_minimal_size,
    .set_property = box_set_property
  };

static struct holy_gui_container_ops box_ops =
{
  .add = box_add,
  .remove = box_remove,
  .iterate_children = box_iterate_children
};

/* Box constructor.  Specify the appropriate layout function to create
   a horizontal or vertical stacking box.  */
static holy_gui_box_t
box_new (layout_func_t layout_func)
{
  holy_gui_box_t box;
  box = holy_zalloc (sizeof (*box));
  if (! box)
    return 0;
  box->container.ops = &box_ops;
  box->container.component.ops = &box_comp_ops;
  box->chead.next = &box->ctail;
  box->ctail.prev = &box->chead;
  box->layout_func = layout_func;
  return box;
}

/* Create a new container that stacks its child components horizontally,
   from left to right.  Each child get a width corresponding to its
   preferred width.  The height of each child is set the maximum of the
   preferred heights of all children.  */
holy_gui_container_t
holy_gui_hbox_new (void)
{
  return (holy_gui_container_t) box_new (layout_horizontally);
}

/* Create a new container that stacks its child components verticallyj,
   from top to bottom.  Each child get a height corresponding to its
   preferred height.  The width of each child is set the maximum of the
   preferred widths of all children.  */
holy_gui_container_t
holy_gui_vbox_new (void)
{
  return (holy_gui_container_t) box_new (layout_vertically);
}
