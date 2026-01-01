/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_REED_SOLOMON_HEADER
#define holy_REED_SOLOMON_HEADER	1

void
holy_reed_solomon_add_redundancy (void *buffer, holy_size_t data_size,
				  holy_size_t redundancy);

void
holy_reed_solomon_recover (void *buffer, holy_size_t data_size,
			   holy_size_t redundancy);

#endif
