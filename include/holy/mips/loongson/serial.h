/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MACHINE_SERIAL_HEADER
#define holy_MACHINE_SERIAL_HEADER	1

#define holy_MACHINE_SERIAL_PORT0_DIVISOR_115200 2
#define holy_MACHINE_SERIAL_PORT2_DIVISOR_115200 1
#define holy_MACHINE_SERIAL_PORT0  0xbff003f8
#define holy_MACHINE_SERIAL_PORT1  0xbfd003f8
#define holy_MACHINE_SERIAL_PORT2  0xbfd002f8

#ifndef ASM_FILE
#define holy_MACHINE_SERIAL_PORTS { holy_MACHINE_SERIAL_PORT0, holy_MACHINE_SERIAL_PORT1, holy_MACHINE_SERIAL_PORT2 }
#else
#endif

#endif 
