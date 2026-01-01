/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BUFIO_H
#define holy_BUFIO_H	1

#include <holy/file.h>

holy_file_t EXPORT_FUNC (holy_bufio_open) (holy_file_t io, int size);
holy_file_t EXPORT_FUNC (holy_buffile_open) (const char *name, int size);

#endif /* ! holy_BUFIO_H */
