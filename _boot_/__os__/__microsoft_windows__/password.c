/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/crypto.h>
#include <holy/mm.h>
#include <holy/term.h>

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int
holy_password_get (char buf[], unsigned buf_size)
{
  HANDLE hStdin = GetStdHandle (STD_INPUT_HANDLE); 
  DWORD mode = 0;
  char *ptr;

  holy_refresh ();
  
  GetConsoleMode (hStdin, &mode);
  SetConsoleMode (hStdin, mode & (~ENABLE_ECHO_INPUT));

  fgets (buf, buf_size, stdin);
  ptr = buf + strlen (buf) - 1;
  while (buf <= ptr && (*ptr == '\n' || *ptr == '\r'))
    *ptr-- = 0;

  SetConsoleMode (hStdin, mode);

  holy_refresh ();

  return 1;
}
