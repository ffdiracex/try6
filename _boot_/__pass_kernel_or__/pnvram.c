/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/err.h>
#include <holy/normal.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/charset.h>
#include <holy/efiemu/efiemu.h>
#include <holy/efiemu/runtime.h>
#include <holy/extcmd.h>

/* Place for final location of variables */
static int nvram_handle = 0;
static int nvramsize_handle = 0;
static int high_monotonic_count_handle = 0;
static int timezone_handle = 0;
static int accuracy_handle = 0;
static int daylight_handle = 0;

static holy_size_t nvramsize;

/* Parse signed value */
static int
holy_strtosl (const char *arg, char **end, int base)
{
  if (arg[0] == '-')
    return -holy_strtoul (arg + 1, end, base);
  return holy_strtoul (arg, end, base);
}

static inline int
hextoval (char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10;
  return 0;
}

static inline holy_err_t
unescape (char *in, char *out, char *outmax, int *len)
{
  char *ptr, *dptr;
  dptr = out;
  for (ptr = in; *ptr && dptr < outmax; )
    if (*ptr == '%' && ptr[1] && ptr[2])
      {
	*dptr = (hextoval (ptr[1]) << 4) | (hextoval (ptr[2]));
	ptr += 3;
	dptr++;
      }
    else
      {
	*dptr = *ptr;
	ptr++;
	dptr++;
      }
  if (dptr == outmax)
    return holy_error (holy_ERR_OUT_OF_MEMORY,
		       "too many NVRAM variables for reserved variable space."
		       " Try increasing EfiEmu.pnvram.size");
  *len = dptr - out;
  return 0;
}

/* Export stuff for efiemu */
static holy_err_t
nvram_set (void * data __attribute__ ((unused)))
{
  const char *env;
  /* Take definitive pointers */
  char *nvram = holy_efiemu_mm_obtain_request (nvram_handle);
  holy_uint32_t *nvramsize_def
    = holy_efiemu_mm_obtain_request (nvramsize_handle);
  holy_uint32_t *high_monotonic_count
    = holy_efiemu_mm_obtain_request (high_monotonic_count_handle);
  holy_int16_t *timezone
    = holy_efiemu_mm_obtain_request (timezone_handle);
  holy_uint8_t *daylight
    = holy_efiemu_mm_obtain_request (daylight_handle);
  holy_uint32_t *accuracy
    = holy_efiemu_mm_obtain_request (accuracy_handle);
  char *nvramptr;
  struct holy_env_var *var;

  /* Copy to definitive loaction */
  holy_dprintf ("efiemu", "preparing pnvram\n");

  env = holy_env_get ("EfiEmu.pnvram.high_monotonic_count");
  *high_monotonic_count = env ? holy_strtoul (env, 0, 0) : 1;
  env = holy_env_get ("EfiEmu.pnvram.timezone");
  *timezone = env ? holy_strtosl (env, 0, 0) : holy_EFI_UNSPECIFIED_TIMEZONE;
  env = holy_env_get ("EfiEmu.pnvram.accuracy");
  *accuracy = env ? holy_strtoul (env, 0, 0) : 50000000;
  env = holy_env_get ("EfiEmu.pnvram.daylight");
  *daylight = env ? holy_strtoul (env, 0, 0) : 0;

  nvramptr = nvram;
  holy_memset (nvram, 0, nvramsize);
  FOR_SORTED_ENV (var)
  {
    char *guid, *attr, *name, *varname;
    struct efi_variable *efivar;
    int len = 0;
    int i;
    holy_uint64_t guidcomp;

    if (holy_memcmp (var->name, "EfiEmu.pnvram.",
		     sizeof ("EfiEmu.pnvram.") - 1) != 0)
      continue;

    guid = var->name + sizeof ("EfiEmu.pnvram.") - 1;

    attr = holy_strchr (guid, '.');
    if (!attr)
      continue;
    attr++;

    name = holy_strchr (attr, '.');
    if (!name)
      continue;
    name++;

    efivar = (struct efi_variable *) nvramptr;
    if (nvramptr - nvram + sizeof (struct efi_variable) > nvramsize)
      return holy_error (holy_ERR_OUT_OF_MEMORY,
			 "too many NVRAM variables for reserved variable space."
			 " Try increasing EfiEmu.pnvram.size");

    nvramptr += sizeof (struct efi_variable);

    efivar->guid.data1 = holy_cpu_to_le32 (holy_strtoul (guid, &guid, 16));
    if (*guid != '-')
      continue;
    guid++;

    efivar->guid.data2 = holy_cpu_to_le16 (holy_strtoul (guid, &guid, 16));
    if (*guid != '-')
      continue;
    guid++;

    efivar->guid.data3 = holy_cpu_to_le16 (holy_strtoul (guid, &guid, 16));
    if (*guid != '-')
      continue;
    guid++;

    guidcomp = holy_strtoull (guid, 0, 16);
    for (i = 0; i < 8; i++)
      efivar->guid.data4[i] = (guidcomp >> (56 - 8 * i)) & 0xff;

    efivar->attributes = holy_strtoull (attr, 0, 16);

    varname = holy_malloc (holy_strlen (name) + 1);
    if (! varname)
      return holy_errno;

    if (unescape (name, varname, varname + holy_strlen (name) + 1, &len))
      break;

    len = holy_utf8_to_utf16 ((holy_uint16_t *) nvramptr,
			      (nvramsize - (nvramptr - nvram)) / 2,
			      (holy_uint8_t *) varname, len, NULL);

    nvramptr += 2 * len;
    *((holy_uint16_t *) nvramptr) = 0;
    nvramptr += 2;
    efivar->namelen = 2 * len + 2;

    if (unescape (var->value, nvramptr, nvram + nvramsize, &len))
      {
	efivar->namelen = 0;
	break;
      }

    nvramptr += len;

    efivar->size = len;
  }
  if (holy_errno)
    return holy_errno;

  *nvramsize_def = nvramsize;

  /* Register symbols */
  holy_efiemu_register_symbol ("efiemu_variables", nvram_handle, 0);
  holy_efiemu_register_symbol ("efiemu_varsize", nvramsize_handle, 0);
  holy_efiemu_register_symbol ("efiemu_high_monotonic_count",
			       high_monotonic_count_handle, 0);
  holy_efiemu_register_symbol ("efiemu_time_zone", timezone_handle, 0);
  holy_efiemu_register_symbol ("efiemu_time_daylight", daylight_handle, 0);
  holy_efiemu_register_symbol ("efiemu_time_accuracy",
			       accuracy_handle, 0);

  return holy_ERR_NONE;
}

static void
nvram_unload (void * data __attribute__ ((unused)))
{
  holy_efiemu_mm_return_request (nvram_handle);
  holy_efiemu_mm_return_request (nvramsize_handle);
  holy_efiemu_mm_return_request (high_monotonic_count_handle);
  holy_efiemu_mm_return_request (timezone_handle);
  holy_efiemu_mm_return_request (accuracy_handle);
  holy_efiemu_mm_return_request (daylight_handle);
}

holy_err_t
holy_efiemu_pnvram (void)
{
  const char *size;
  holy_err_t err;

  nvramsize = 0;

  size = holy_env_get ("EfiEmu.pnvram.size");
  if (size)
    nvramsize = holy_strtoul (size, 0, 0);

  if (!nvramsize)
    nvramsize = 2048;

  err = holy_efiemu_register_prepare_hook (nvram_set, nvram_unload, 0);
  if (err)
    return err;

  nvram_handle
    = holy_efiemu_request_memalign (1, nvramsize,
				    holy_EFI_RUNTIME_SERVICES_DATA);
  nvramsize_handle
    = holy_efiemu_request_memalign (1, sizeof (holy_uint32_t),
				    holy_EFI_RUNTIME_SERVICES_DATA);
  high_monotonic_count_handle
    = holy_efiemu_request_memalign (1, sizeof (holy_uint32_t),
				    holy_EFI_RUNTIME_SERVICES_DATA);
  timezone_handle
    = holy_efiemu_request_memalign (1, sizeof (holy_uint16_t),
				    holy_EFI_RUNTIME_SERVICES_DATA);
  daylight_handle
    = holy_efiemu_request_memalign (1, sizeof (holy_uint8_t),
				    holy_EFI_RUNTIME_SERVICES_DATA);
  accuracy_handle
    = holy_efiemu_request_memalign (1, sizeof (holy_uint32_t),
				    holy_EFI_RUNTIME_SERVICES_DATA);

  return holy_ERR_NONE;
}
