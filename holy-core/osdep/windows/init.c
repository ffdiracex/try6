/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>
#include <holy/util/misc.h>
#include <holy/osdep/hostfile.h>
#include <holy/util/windows.h>
#include <holy/emu/config.h>

#include <wincon.h>
#include <windows.h>

#include <holy/util/misc.h>

#include "progname.h"

struct holy_windows_console_font_infoex {
  ULONG cbSize;
  DWORD nFont;
  COORD dwFontSize;
  UINT  FontFamily;
  UINT  FontWeight;
  WCHAR FaceName[LF_FACESIZE];
};

static int
check_is_raster (HMODULE kernel32, HANDLE hnd)
{
  CONSOLE_FONT_INFO console_font_info;
  BOOL (WINAPI * func_GetCurrentConsoleFont) (HANDLE, BOOL,
					      PCONSOLE_FONT_INFO);

  func_GetCurrentConsoleFont = (void *)
    GetProcAddress (kernel32, "GetCurrentConsoleFont");

  if (!func_GetCurrentConsoleFont)
    return 1;

  if (!func_GetCurrentConsoleFont (hnd, FALSE, &console_font_info))
    return 1;
  return console_font_info.nFont < 12;
}

static void
set_console_unicode_font (void)
{
  BOOL (WINAPI * func_SetCurrentConsoleFontEx) (HANDLE, BOOL,
						struct holy_windows_console_font_infoex *);
  BOOL (WINAPI * func_SetConsoleFont)(HANDLE, DWORD);
  HMODULE kernel32;
  HANDLE out_handle = GetStdHandle (STD_OUTPUT_HANDLE);
  HANDLE err_handle = GetStdHandle (STD_ERROR_HANDLE);
  int out_raster, err_raster;

  kernel32 = GetModuleHandle(TEXT("kernel32.dll"));
  if (!kernel32)
    return;

  out_raster = check_is_raster (kernel32, out_handle);
  err_raster = check_is_raster (kernel32, err_handle);

  if (!out_raster && !err_raster)
    return;

  func_SetCurrentConsoleFontEx = (void *) GetProcAddress (kernel32, "SetCurrentConsoleFontEx");

  /* Newer windows versions.  */
  if (func_SetCurrentConsoleFontEx)
    {
      struct holy_windows_console_font_infoex new_console_font_info;
      new_console_font_info.cbSize = sizeof (new_console_font_info);
      new_console_font_info.nFont = 12;
      new_console_font_info.dwFontSize.X = 7;
      new_console_font_info.dwFontSize.Y = 12;
      new_console_font_info.FontFamily = FF_DONTCARE;
      new_console_font_info.FontWeight = 400;
      memcpy (new_console_font_info.FaceName, TEXT("Lucida Console"),
	      sizeof (TEXT("Lucida Console")));
      if (out_raster)
	func_SetCurrentConsoleFontEx (out_handle, FALSE,
				      &new_console_font_info);
      if (err_raster)
	func_SetCurrentConsoleFontEx (err_handle, FALSE,
				      &new_console_font_info);
      return;
    }

  /* Fallback for older versions.  */
  func_SetConsoleFont = (void *) GetProcAddress (kernel32, "SetConsoleFont");
  if (func_SetConsoleFont)
    {
      if (out_raster)
	func_SetConsoleFont (out_handle, 12);
      if (err_raster)
	func_SetConsoleFont (err_handle, 12);
    }
}

static char *holy_util_base_directory;
static char *locale_dir;

const char *
holy_util_get_config_filename (void)
{
  static char *value = NULL;
  if (!value)
    value = holy_util_path_concat (2, holy_util_base_directory, "holy.cfg");
  return value;
}

const char *
holy_util_get_pkgdatadir (void)
{
  return holy_util_base_directory;
}

const char *
holy_util_get_localedir (void)
{
  return locale_dir;
}

const char *
holy_util_get_pkglibdir (void)
{
  return holy_util_base_directory;
}

void
holy_util_host_init (int *argc __attribute__ ((unused)),
		     char ***argv)
{
  char *ptr;

  SetConsoleOutputCP (CP_UTF8);
  SetConsoleCP (CP_UTF8);

  set_console_unicode_font ();

#if SIZEOF_TCHAR == 1

#elif SIZEOF_TCHAR == 2
  LPWSTR tcmdline = GetCommandLineW ();
  int i;
  LPWSTR *targv;

  targv = CommandLineToArgvW (tcmdline, argc);
  *argv = xmalloc ((*argc + 1) * sizeof (argv[0]));

  for (i = 0; i < *argc; i++)
    (*argv)[i] = holy_util_tchar_to_utf8 (targv[i]);
  (*argv)[i] = NULL;
#else
#error "Unsupported TCHAR size"
#endif

  holy_util_base_directory = holy_canonicalize_file_name ((*argv)[0]);
  if (!holy_util_base_directory)
    holy_util_base_directory = xstrdup ((*argv)[0]);
  for (ptr = holy_util_base_directory + strlen (holy_util_base_directory) - 1;
       ptr >= holy_util_base_directory && *ptr != '/' && *ptr != '\\'; ptr--);
  if (ptr >= holy_util_base_directory)
    *ptr = '\0';

  locale_dir = holy_util_path_concat (2, holy_util_base_directory, "locale");

  set_program_name ((*argv)[0]);

#if (defined (holy_UTIL) && defined(ENABLE_NLS) && ENABLE_NLS)
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, locale_dir);
  textdomain (PACKAGE);
#endif /* (defined(ENABLE_NLS) && ENABLE_NLS) */
}
