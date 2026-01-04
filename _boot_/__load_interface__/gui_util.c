/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/gui.h>
#include <holy/gui_string_util.h>


struct find_by_id_state
{
  const char *match_id;
  holy_gui_component_callback match_callback;
  void *match_userdata;
};

static void
find_by_id_recursively (holy_gui_component_t component, void *userdata)
{
  struct find_by_id_state *state;
  const char *id;

  state = (struct find_by_id_state *) userdata;
  id = component->ops->get_id (component);
  if (id && holy_strcmp (id, state->match_id) == 0)
    state->match_callback (component, state->match_userdata);

  if (component->ops->is_instance (component, "container"))
    {
      holy_gui_container_t container;
      container = (holy_gui_container_t) component;
      container->ops->iterate_children (container,
                                        find_by_id_recursively,
                                        state);
    }
}

void
holy_gui_find_by_id (holy_gui_component_t root,
                     const char *id,
                     holy_gui_component_callback cb,
                     void *userdata)
{
  struct find_by_id_state state;
  state.match_id = id;
  state.match_callback = cb;
  state.match_userdata = userdata;
  find_by_id_recursively (root, &state);
}


struct iterate_recursively_state
{
  holy_gui_component_callback callback;
  void *userdata;
};

static
void iterate_recursively_cb (holy_gui_component_t component, void *userdata)
{
  struct iterate_recursively_state *state;

  state = (struct iterate_recursively_state *) userdata;
  state->callback (component, state->userdata);

  if (component->ops->is_instance (component, "container"))
    {
      holy_gui_container_t container;
      container = (holy_gui_container_t) component;
      container->ops->iterate_children (container,
                                        iterate_recursively_cb,
                                        state);
    }
}

void
holy_gui_iterate_recursively (holy_gui_component_t root,
                              holy_gui_component_callback cb,
                              void *userdata)
{
  struct iterate_recursively_state state;
  state.callback = cb;
  state.userdata = userdata;
  iterate_recursively_cb (root, &state);
}
