/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define MODE_MDA 1
#include "vga_text.c"

holy_MOD_INIT(mda_text)
{
  holy_term_register_output ("mda_text", &holy_vga_text_term);
}

holy_MOD_FINI(mda_text)
{
  holy_term_unregister_output (&holy_vga_text_term);
}

