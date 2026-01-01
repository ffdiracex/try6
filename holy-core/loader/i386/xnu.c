/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/env.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/xnu.h>
#include <holy/cpu/xnu.h>
#include <holy/mm.h>
#include <holy/loader.h>
#include <holy/autoefi.h>
#include <holy/i386/tsc.h>
#include <holy/i386/cpuid.h>
#include <holy/efi/api.h>
#include <holy/i386/pit.h>
#include <holy/misc.h>
#include <holy/charset.h>
#include <holy/term.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/bitmap_scale.h>
#include <holy/cpu/io.h>
#include <holy/random.h>

#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

#define DEFAULT_VIDEO_MODE "auto"

char holy_xnu_cmdline[1024];
holy_uint32_t holy_xnu_entry_point, holy_xnu_arg1, holy_xnu_stack;

/* Aliases set for some tables. */
struct tbl_alias
{
  holy_efi_guid_t guid;
  const char *name;
};

static struct tbl_alias table_aliases[] =
  {
    {holy_EFI_ACPI_20_TABLE_GUID, "ACPI_20"},
    {holy_EFI_ACPI_TABLE_GUID, "ACPI"},
  };

struct holy_xnu_devprop_device_descriptor
{
  struct holy_xnu_devprop_device_descriptor *next;
  struct holy_xnu_devprop_device_descriptor **prev;
  struct property_descriptor *properties;
  struct holy_efi_device_path *path;
  int pathlen;
};

static int
utf16_strlen (holy_uint16_t *in)
{
  int i;
  for (i = 0; in[i]; i++);
  return i;
}

/* Read frequency from a string in MHz and return it in Hz. */
static holy_uint64_t
readfrequency (const char *str)
{
  holy_uint64_t num = 0;
  int mul = 1000000;
  int found = 0;

  while (*str)
    {
      unsigned long digit;

      digit = holy_tolower (*str) - '0';
      if (digit > 9)
	break;

      found = 1;

      num = num * 10 + digit;
      str++;
    }
  num *= 1000000;
  if (*str == '.')
    {
      str++;
      while (*str)
	{
	  unsigned long digit;

	  digit = holy_tolower (*str) - '0';
	  if (digit > 9)
	    break;

	  found = 1;

	  mul /= 10;
	  num = num + mul * digit;
	  str++;
	}
    }
  if (! found)
    return 0;

  return num;
}

/* Thanks to Kabyl for precious information about Intel architecture. */
static holy_uint64_t
guessfsb (void)
{
  const holy_uint64_t sane_value = 100000000;
  holy_uint32_t manufacturer[3], max_cpuid, capabilities, msrlow;
  holy_uint32_t a, b, d, divisor;

  if (! holy_cpu_is_cpuid_supported ())
    return sane_value;

  holy_cpuid (0, max_cpuid, manufacturer[0], manufacturer[2], manufacturer[1]);

  /* Only Intel for now is done. */
  if (holy_memcmp (manufacturer, "GenuineIntel", 12) != 0)
    return sane_value;

  /* Check Speedstep. */
  if (max_cpuid < 1)
    return sane_value;

  holy_cpuid (1, a, b, capabilities, d);

  if (! (capabilities & (1 << 7)))
    return sane_value;

  /* Read the multiplier. */
  asm volatile ("movl $0x198, %%ecx\n"
		"rdmsr"
		: "=d" (msrlow)
		:
		: "%ecx", "%eax");

  holy_uint64_t v;
  holy_uint32_t r;

  /* (2000ULL << 32) / holy_tsc_rate  */
  /* Assumption: TSC frequency is over 2 MHz.  */
  v = 0xffffffff / holy_tsc_rate;
  v *= 2000;
  /* v is at most 2000 off from (2000ULL << 32) / holy_tsc_rate.
     Since holy_tsc_rate < 2^32/2^11=2^21, so no overflow.
   */
  r = (2000ULL << 32) - v * holy_tsc_rate;
  v += r / holy_tsc_rate;

  divisor = ((msrlow >> 7) & 0x3e) | ((msrlow >> 14) & 1);
  if (divisor == 0)
    return sane_value;
  return holy_divmod64 (v, divisor, 0);
}

struct property_descriptor
{
  struct property_descriptor *next;
  struct property_descriptor **prev;
  holy_uint8_t *name;
  holy_uint16_t *name16;
  int name16len;
  int length;
  void *data;
};

static struct holy_xnu_devprop_device_descriptor *devices = 0;

holy_err_t
holy_xnu_devprop_remove_property (struct holy_xnu_devprop_device_descriptor *dev,
				  char *name)
{
  struct property_descriptor *prop;
  prop = holy_named_list_find (holy_AS_NAMED_LIST (dev->properties), name);
  if (!prop)
    return holy_ERR_NONE;

  holy_free (prop->name);
  holy_free (prop->name16);
  holy_free (prop->data);

  holy_list_remove (holy_AS_LIST (prop));

  return holy_ERR_NONE;
}

holy_err_t
holy_xnu_devprop_remove_device (struct holy_xnu_devprop_device_descriptor *dev)
{
  void *t;
  struct property_descriptor *prop;

  holy_list_remove (holy_AS_LIST (dev));

  for (prop = dev->properties; prop; )
    {
      holy_free (prop->name);
      holy_free (prop->name16);
      holy_free (prop->data);
      t = prop;
      prop = prop->next;
      holy_free (t);
    }

  holy_free (dev->path);
  holy_free (dev);

  return holy_ERR_NONE;
}

struct holy_xnu_devprop_device_descriptor *
holy_xnu_devprop_add_device (struct holy_efi_device_path *path, int length)
{
  struct holy_xnu_devprop_device_descriptor *ret;

  ret = holy_zalloc (sizeof (*ret));
  if (!ret)
    return 0;

  ret->path = holy_malloc (length);
  if (!ret->path)
    {
      holy_free (ret);
      return 0;
    }
  ret->pathlen = length;
  holy_memcpy (ret->path, path, length);

  holy_list_push (holy_AS_LIST_P (&devices), holy_AS_LIST (ret));

  return ret;
}

static holy_err_t
holy_xnu_devprop_add_property (struct holy_xnu_devprop_device_descriptor *dev,
			       holy_uint8_t *utf8, holy_uint16_t *utf16,
			       int utf16len, void *data, int datalen)
{
  struct property_descriptor *prop;

  prop = holy_malloc (sizeof (*prop));
  if (!prop)
    return holy_errno;

  prop->name = utf8;
  prop->name16 = utf16;
  prop->name16len = utf16len;

  prop->length = datalen;
  prop->data = holy_malloc (prop->length);
  if (!prop->data)
    {
      holy_free (prop->name);
      holy_free (prop->name16);
      holy_free (prop);
      return holy_errno;
    }
  holy_memcpy (prop->data, data, prop->length);
  holy_list_push (holy_AS_LIST_P (&dev->properties),
		  holy_AS_LIST (prop));
  return holy_ERR_NONE;
}

holy_err_t
holy_xnu_devprop_add_property_utf8 (struct holy_xnu_devprop_device_descriptor *dev,
				    char *name, void *data, int datalen)
{
  holy_uint8_t *utf8;
  holy_uint16_t *utf16;
  int len, utf16len;
  holy_err_t err;

  utf8 = (holy_uint8_t *) holy_strdup (name);
  if (!utf8)
    return holy_errno;

  len = holy_strlen (name);
  utf16 = holy_malloc (sizeof (holy_uint16_t) * len);
  if (!utf16)
    {
      holy_free (utf8);
      return holy_errno;
    }

  utf16len = holy_utf8_to_utf16 (utf16, len, utf8, len, NULL);
  if (utf16len < 0)
    {
      holy_free (utf8);
      holy_free (utf16);
      return holy_errno;
    }

  err = holy_xnu_devprop_add_property (dev, utf8, utf16,
				       utf16len, data, datalen);
  if (err)
    {
      holy_free (utf8);
      holy_free (utf16);
      return err;
    }

  return holy_ERR_NONE;
}

holy_err_t
holy_xnu_devprop_add_property_utf16 (struct holy_xnu_devprop_device_descriptor *dev,
				     holy_uint16_t *name, int namelen,
				     void *data, int datalen)
{
  holy_uint8_t *utf8;
  holy_uint16_t *utf16;
  holy_err_t err;

  utf16 = holy_malloc (sizeof (holy_uint16_t) * namelen);
  if (!utf16)
    return holy_errno;
  holy_memcpy (utf16, name, sizeof (holy_uint16_t) * namelen);

  utf8 = holy_malloc (namelen * 4 + 1);
  if (!utf8)
    {
      holy_free (utf16);
      return holy_errno;
    }

  *holy_utf16_to_utf8 ((holy_uint8_t *) utf8, name, namelen) = '\0';

  err = holy_xnu_devprop_add_property (dev, utf8, utf16,
				       namelen, data, datalen);
  if (err)
    {
      holy_free (utf8);
      holy_free (utf16);
      return err;
    }

  return holy_ERR_NONE;
}

void
holy_cpu_xnu_unload (void)
{
  struct holy_xnu_devprop_device_descriptor *dev1, *dev2;

  for (dev1 = devices; dev1; )
    {
      dev2 = dev1->next;
      holy_xnu_devprop_remove_device (dev1);
      dev1 = dev2;
    }
}

static holy_err_t
holy_cpu_xnu_fill_devprop (void)
{
  struct holy_xnu_devtree_key *efikey;
  int total_length = sizeof (struct holy_xnu_devprop_header);
  struct holy_xnu_devtree_key *devprop;
  struct holy_xnu_devprop_device_descriptor *device;
  void *ptr;
  struct holy_xnu_devprop_header *head;
  void *t;
  int numdevs = 0;

  /* The key "efi". */
  efikey = holy_xnu_create_key (&holy_xnu_devtree_root, "efi");
  if (! efikey)
    return holy_errno;

  for (device = devices; device; device = device->next)
    {
      struct property_descriptor *propdesc;
      total_length += sizeof (struct holy_xnu_devprop_device_header);
      total_length += device->pathlen;

      for (propdesc = device->properties; propdesc; propdesc = propdesc->next)
	{
	  total_length += sizeof (holy_uint32_t);
	  total_length += sizeof (holy_uint16_t)
	    * (propdesc->name16len + 1);
	  total_length += sizeof (holy_uint32_t);
	  total_length += propdesc->length;
	}
      numdevs++;
    }

  devprop = holy_xnu_create_value (&(efikey->first_child), "device-properties");
  if (!devprop)
    return holy_errno;

  devprop->data = holy_malloc (total_length);
  devprop->datasize = total_length;

  ptr = devprop->data;
  head = ptr;
  ptr = head + 1;
  head->length = total_length;
  head->alwaysone = 1;
  head->num_devices = numdevs;
  for (device = devices; device; )
    {
      struct holy_xnu_devprop_device_header *devhead;
      struct property_descriptor *propdesc;
      devhead = ptr;
      devhead->num_values = 0;
      ptr = devhead + 1;

      holy_memcpy (ptr, device->path, device->pathlen);
      ptr = (char *) ptr + device->pathlen;

      for (propdesc = device->properties; propdesc; )
	{
	  holy_uint32_t *len;
	  holy_uint16_t *name;
	  void *data;

	  len = ptr;
	  *len = 2 * propdesc->name16len + sizeof (holy_uint16_t)
	    + sizeof (holy_uint32_t);
	  ptr = len + 1;

	  name = ptr;
	  holy_memcpy (name, propdesc->name16, 2 * propdesc->name16len);
	  name += propdesc->name16len;

	  /* NUL terminator.  */
	  *name = 0;
	  ptr = name + 1;

	  len = ptr;
	  *len = propdesc->length + sizeof (holy_uint32_t);
	  data = len + 1;
	  ptr = data;
	  holy_memcpy (ptr, propdesc->data, propdesc->length);
	  ptr = (char *) ptr + propdesc->length;

	  holy_free (propdesc->name);
	  holy_free (propdesc->name16);
	  holy_free (propdesc->data);
	  t = propdesc;
	  propdesc = propdesc->next;
	  holy_free (t);
	  devhead->num_values++;
	}

      devhead->length = (char *) ptr - (char *) devhead;
      t = device;
      device = device->next;
      holy_free (t);
    }

  devices = 0;

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_devprop_load (holy_command_t cmd __attribute__ ((unused)),
		       int argc, char *args[])
{
  holy_file_t file;
  void *buf, *bufstart, *bufend;
  struct holy_xnu_devprop_header *head;
  holy_size_t size;
  unsigned i, j;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = holy_file_open (args[0]);
  if (! file)
    return holy_errno;
  size = holy_file_size (file);
  buf = holy_malloc (size);
  if (!buf)
    {
      holy_file_close (file);
      return holy_errno;
    }
  if (holy_file_read (file, buf, size) != (holy_ssize_t) size)
    {
      holy_file_close (file);
      return holy_errno;
    }
  holy_file_close (file);

  bufstart = buf;
  bufend = (char *) buf + size;
  head = buf;
  buf = head + 1;
  for (i = 0; i < holy_le_to_cpu32 (head->num_devices) && buf < bufend; i++)
    {
      struct holy_efi_device_path *dp, *dpstart;
      struct holy_xnu_devprop_device_descriptor *dev;
      struct holy_xnu_devprop_device_header *devhead;

      devhead = buf;
      buf = devhead + 1;
      dpstart = buf;

      do
	{
	  dp = buf;
	  buf = (char *) buf + holy_EFI_DEVICE_PATH_LENGTH (dp);
	}
      while (!holy_EFI_END_ENTIRE_DEVICE_PATH (dp) && buf < bufend);

      dev = holy_xnu_devprop_add_device (dpstart, (char *) buf
					 - (char *) dpstart);

      for (j = 0; j < holy_le_to_cpu32 (devhead->num_values) && buf < bufend;
	   j++)
	{
	  holy_uint32_t *namelen;
	  holy_uint32_t *datalen;
	  holy_uint16_t *utf16;
	  void *data;
	  holy_err_t err;

	  namelen = buf;
	  buf = namelen + 1;
	  if (buf >= bufend)
	    break;

	  utf16 = buf;
	  buf = (char *) buf + *namelen - sizeof (holy_uint32_t);
	  if (buf >= bufend)
	    break;

	  datalen = buf;
	  buf = datalen + 1;
	  if (buf >= bufend)
	    break;

	  data = buf;
	  buf = (char *) buf + *datalen - sizeof (holy_uint32_t);
	  if (buf >= bufend)
	    break;
	  err = holy_xnu_devprop_add_property_utf16
	    (dev, utf16, (*namelen - sizeof (holy_uint32_t)
			  - sizeof (holy_uint16_t)) / sizeof (holy_uint16_t),
	     data, *datalen - sizeof (holy_uint32_t));
	  if (err)
	    {
	      holy_free (bufstart);
	      return err;
	    }
	}
    }

  holy_free (bufstart);
  return holy_ERR_NONE;
}

/* Fill device tree. */
/* FIXME: some entries may be platform-agnostic. Move them to loader/xnu.c. */
static holy_err_t
holy_cpu_xnu_fill_devicetree (holy_uint64_t *fsbfreq_out)
{
  struct holy_xnu_devtree_key *efikey;
  struct holy_xnu_devtree_key *chosenkey;
  struct holy_xnu_devtree_key *cfgtablekey;
  struct holy_xnu_devtree_key *curval;
  struct holy_xnu_devtree_key *runtimesrvkey;
  struct holy_xnu_devtree_key *platformkey;
  unsigned i, j;
  holy_err_t err;

  chosenkey = holy_xnu_create_key (&holy_xnu_devtree_root, "chosen");
  if (! chosenkey)
    return holy_errno;

  /* Random seed. */
  curval = holy_xnu_create_value (&(chosenkey->first_child), "random-seed");
  if (! curval)
    return holy_errno;
  curval->datasize = 64;
  curval->data = holy_malloc (curval->datasize);
  if (! curval->data)
    return holy_errno;
  /* Our random is not peer-reviewed but xnu uses this seed only for
     ASLR in kernel.  */
  err = holy_crypto_get_random (curval->data, curval->datasize);
  if (err)
    return err;

  /* The value "model". */
  /* FIXME: may this value be sometimes different? */
  curval = holy_xnu_create_value (&holy_xnu_devtree_root, "model");
  if (! curval)
    return holy_errno;
  curval->datasize = sizeof ("ACPI");
  curval->data = holy_strdup ("ACPI");
  curval = holy_xnu_create_value (&holy_xnu_devtree_root, "compatible");
  if (! curval)
    return holy_errno;
  curval->datasize = sizeof ("ACPI");
  curval->data = holy_strdup ("ACPI");

  /* The key "efi". */
  efikey = holy_xnu_create_key (&holy_xnu_devtree_root, "efi");
  if (! efikey)
    return holy_errno;

  /* Information about firmware. */
  curval = holy_xnu_create_value (&(efikey->first_child), "firmware-revision");
  if (! curval)
    return holy_errno;
  curval->datasize = (SYSTEM_TABLE_SIZEOF (firmware_revision));
  curval->data = holy_malloc (curval->datasize);
  if (! curval->data)
    return holy_errno;
  holy_memcpy (curval->data, (SYSTEM_TABLE_VAR(firmware_revision)),
	       curval->datasize);

  curval = holy_xnu_create_value (&(efikey->first_child), "firmware-vendor");
  if (! curval)
    return holy_errno;
  curval->datasize =
    2 * (utf16_strlen (SYSTEM_TABLE_PTR (firmware_vendor)) + 1);
  curval->data = holy_malloc (curval->datasize);
  if (! curval->data)
    return holy_errno;
  holy_memcpy (curval->data, SYSTEM_TABLE_PTR (firmware_vendor),
	       curval->datasize);

  curval = holy_xnu_create_value (&(efikey->first_child), "firmware-abi");
  if (! curval)
    return holy_errno;
  curval->datasize = sizeof ("EFI32");
  curval->data = holy_malloc (curval->datasize);
  if (! curval->data)
    return holy_errno;
  if (SIZEOF_OF_UINTN == 4)
    holy_memcpy (curval->data, "EFI32", curval->datasize);
  else
    holy_memcpy (curval->data, "EFI64", curval->datasize);

  /* The key "platform". */
  platformkey = holy_xnu_create_key (&(efikey->first_child),
				     "platform");
  if (! platformkey)
    return holy_errno;

  /* Pass FSB frequency to the kernel. */
  curval = holy_xnu_create_value (&(platformkey->first_child), "FSBFrequency");
  if (! curval)
    return holy_errno;
  curval->datasize = sizeof (holy_uint64_t);
  curval->data = holy_malloc (curval->datasize);
  if (!curval->data)
    return holy_errno;

  /* First see if user supplies the value. */
  const char *fsbvar = holy_env_get ("fsb");
  holy_uint64_t fsbfreq = 0;
  if (fsbvar)
    fsbfreq = readfrequency (fsbvar);
  /* Try autodetect. */
  if (! fsbfreq)
    fsbfreq = guessfsb ();
  *((holy_uint64_t *) curval->data) = fsbfreq;
  *fsbfreq_out = fsbfreq;
  holy_dprintf ("xnu", "fsb autodetected as %llu\n",
		(unsigned long long) *((holy_uint64_t *) curval->data));

  cfgtablekey = holy_xnu_create_key (&(efikey->first_child),
				     "configuration-table");
  if (!cfgtablekey)
    return holy_errno;

  /* Fill "configuration-table" key. */
  for (i = 0; i < SYSTEM_TABLE (num_table_entries); i++)
    {
      void *ptr;
      struct holy_xnu_devtree_key *curkey;
      holy_efi_packed_guid_t guid;
      char guidbuf[64];

      /* Retrieve current key. */
#ifdef holy_MACHINE_EFI
      {
	ptr = (void *)
	  holy_efi_system_table->configuration_table[i].vendor_table;
	guid = holy_efi_system_table->configuration_table[i].vendor_guid;
      }
#else
      if (SIZEOF_OF_UINTN == 4)
	{
	  ptr = (void *) (holy_addr_t) ((holy_efiemu_configuration_table32_t *)
					SYSTEM_TABLE_PTR (configuration_table))[i]
	    .vendor_table;
	  guid =
	    ((holy_efiemu_configuration_table32_t *)
	     SYSTEM_TABLE_PTR (configuration_table))[i].vendor_guid;
	}
      else
	{
	  ptr = (void *) (holy_addr_t) ((holy_efiemu_configuration_table64_t *)
					SYSTEM_TABLE_PTR (configuration_table))[i]
	    .vendor_table;
	  guid =
	    ((holy_efiemu_configuration_table64_t *)
	     SYSTEM_TABLE_PTR (configuration_table))[i].vendor_guid;
	}
#endif

      /* The name of key for new table. */
      holy_snprintf (guidbuf, sizeof (guidbuf), "%08x-%04x-%04x-%02x%02x-",
		     guid.data1, guid.data2, guid.data3, guid.data4[0],
		     guid.data4[1]);
      for (j = 2; j < 8; j++)
	holy_snprintf (guidbuf + holy_strlen (guidbuf),
		       sizeof (guidbuf) - holy_strlen (guidbuf),
		       "%02x", guid.data4[j]);
      /* For some reason GUID has to be in uppercase. */
      for (j = 0; guidbuf[j] ; j++)
	if (guidbuf[j] >= 'a' && guidbuf[j] <= 'f')
	  guidbuf[j] += 'A' - 'a';
      curkey = holy_xnu_create_key (&(cfgtablekey->first_child), guidbuf);
      if (! curkey)
	return holy_errno;

      curval = holy_xnu_create_value (&(curkey->first_child), "guid");
      if (! curval)
	return holy_errno;
      curval->datasize = sizeof (guid);
      curval->data = holy_malloc (curval->datasize);
      if (! curval->data)
	return holy_errno;
      holy_memcpy (curval->data, &guid, curval->datasize);

      /* The value "table". */
      curval = holy_xnu_create_value (&(curkey->first_child), "table");
      if (! curval)
	return holy_errno;
      curval->datasize = SIZEOF_OF_UINTN;
      curval->data = holy_malloc (curval->datasize);
      if (! curval->data)
	return holy_errno;
      if (SIZEOF_OF_UINTN == 4)
	*((holy_uint32_t *) curval->data) = (holy_addr_t) ptr;
      else
	*((holy_uint64_t *) curval->data) = (holy_addr_t) ptr;

      /* Create alias. */
      for (j = 0; j < ARRAY_SIZE(table_aliases); j++)
	if (holy_memcmp (&table_aliases[j].guid, &guid, sizeof (guid)) == 0)
	  break;
      if (j != ARRAY_SIZE(table_aliases))
	{
	  curval = holy_xnu_create_value (&(curkey->first_child), "alias");
	  if (!curval)
	    return holy_errno;
	  curval->datasize = holy_strlen (table_aliases[j].name) + 1;
	  curval->data = holy_malloc (curval->datasize);
	  if (!curval->data)
	    return holy_errno;
	  holy_memcpy (curval->data, table_aliases[j].name, curval->datasize);
	}
    }

  /* Create and fill "runtime-services" key. */
  runtimesrvkey = holy_xnu_create_key (&(efikey->first_child),
				       "runtime-services");
  if (! runtimesrvkey)
    return holy_errno;
  curval = holy_xnu_create_value (&(runtimesrvkey->first_child), "table");
  if (! curval)
    return holy_errno;
  curval->datasize = SIZEOF_OF_UINTN;
  curval->data = holy_malloc (curval->datasize);
  if (! curval->data)
    return holy_errno;
  if (SIZEOF_OF_UINTN == 4)
    *((holy_uint32_t *) curval->data)
      = (holy_addr_t) SYSTEM_TABLE_PTR (runtime_services);
  else
    *((holy_uint64_t *) curval->data)
      = (holy_addr_t) SYSTEM_TABLE_PTR (runtime_services);

  return holy_ERR_NONE;
}

holy_err_t
holy_xnu_boot_resume (void)
{
  struct holy_relocator32_state state;

  state.esp = holy_xnu_stack;
  state.ebp = holy_xnu_stack;
  state.eip = holy_xnu_entry_point;
  state.eax = holy_xnu_arg1;

  return holy_relocator32_boot (holy_xnu_relocator, state, 0);
}

/* Setup video for xnu. */
static holy_err_t
holy_xnu_set_video (struct holy_xnu_boot_params_common *params)
{
  struct holy_video_mode_info mode_info;
  char *tmp;
  const char *modevar;
  void *framebuffer;
  holy_err_t err;
  struct holy_video_bitmap *bitmap = NULL;

  modevar = holy_env_get ("gfxpayload");
  /* Consider only graphical 32-bit deep modes.  */
  if (! modevar || *modevar == 0)
    err = holy_video_set_mode (DEFAULT_VIDEO_MODE,
			       holy_VIDEO_MODE_TYPE_PURE_TEXT
			       | holy_VIDEO_MODE_TYPE_DEPTH_MASK,
			       32 << holy_VIDEO_MODE_TYPE_DEPTH_POS);
  else
    {
      tmp = holy_xasprintf ("%s;" DEFAULT_VIDEO_MODE, modevar);
      if (! tmp)
	return holy_errno;
      err = holy_video_set_mode (tmp,
				 holy_VIDEO_MODE_TYPE_PURE_TEXT
				 | holy_VIDEO_MODE_TYPE_DEPTH_MASK,
				 32 << holy_VIDEO_MODE_TYPE_DEPTH_POS);
      holy_free (tmp);
    }

  if (err)
    return err;

  err = holy_video_get_info (&mode_info);
  if (err)
    return err;

  if (holy_xnu_bitmap)
     {
       if (holy_xnu_bitmap_mode == holy_XNU_BITMAP_STRETCH)
	 err = holy_video_bitmap_create_scaled (&bitmap,
						mode_info.width,
						mode_info.height,
						holy_xnu_bitmap,
						holy_VIDEO_BITMAP_SCALE_METHOD_BEST);
       else
	 bitmap = holy_xnu_bitmap;
     }

  if (bitmap)
    {
      if (holy_xnu_bitmap_mode == holy_XNU_BITMAP_STRETCH)
	err = holy_video_bitmap_create_scaled (&bitmap,
					       mode_info.width,
					       mode_info.height,
					       holy_xnu_bitmap,
					       holy_VIDEO_BITMAP_SCALE_METHOD_BEST);
      else
	bitmap = holy_xnu_bitmap;
    }

  if (bitmap)
    {
      int x, y;

      x = mode_info.width - bitmap->mode_info.width;
      x /= 2;
      y = mode_info.height - bitmap->mode_info.height;
      y /= 2;
      err = holy_video_blit_bitmap (bitmap,
				    holy_VIDEO_BLIT_REPLACE,
				    x > 0 ? x : 0,
				    y > 0 ? y : 0,
				    x < 0 ? -x : 0,
				    y < 0 ? -y : 0,
				    min (bitmap->mode_info.width,
					 mode_info.width),
				    min (bitmap->mode_info.height,
					 mode_info.height));
    }
  if (err)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
      bitmap = 0;
    }

  err = holy_video_get_info_and_fini (&mode_info, &framebuffer);
  if (err)
    return err;

  params->lfb_width = mode_info.width;
  params->lfb_height = mode_info.height;
  params->lfb_depth = mode_info.bpp;
  params->lfb_line_len = mode_info.pitch;

  params->lfb_base = (holy_addr_t) framebuffer;
  params->lfb_mode = bitmap ? holy_XNU_VIDEO_SPLASH
    : holy_XNU_VIDEO_TEXT_IN_VIDEO;

  return holy_ERR_NONE;
}

static int
total_ram_hook (holy_uint64_t addr __attribute__ ((unused)), holy_uint64_t size,
		holy_memory_type_t type,
		void *data)
{
  holy_uint64_t *result = data;

  if (type != holy_MEMORY_AVAILABLE)
    return 0;
  *result += size;
  return 0;
}

static holy_uint64_t
get_total_ram (void)
{
  holy_uint64_t result = 0;

  holy_mmap_iterate (total_ram_hook, &result);
  return result;
}

/* Boot xnu. */
holy_err_t
holy_xnu_boot (void)
{
  union holy_xnu_boot_params_any *bootparams;
  struct holy_xnu_boot_params_common *bootparams_common;
  void *bp_in;
  holy_addr_t bootparams_target;
  holy_err_t err;
  holy_efi_uintn_t memory_map_size = 0;
  void *memory_map;
  holy_addr_t memory_map_target;
  holy_efi_uintn_t map_key = 0;
  holy_efi_uintn_t descriptor_size = 0;
  holy_efi_uint32_t descriptor_version = 0;
  holy_uint64_t firstruntimepage, lastruntimepage;
  holy_uint64_t curruntimepage;
  holy_addr_t devtree_target;
  holy_size_t devtreelen;
  int i;
  struct holy_relocator32_state state;
  holy_uint64_t fsbfreq = 100000000;
  int v2 = (holy_xnu_darwin_version >= 11);
  holy_uint32_t efi_system_table = 0;

  err = holy_autoefi_prepare ();
  if (err)
    return err;

  err = holy_cpu_xnu_fill_devprop ();
  if (err)
    return err;

  err = holy_cpu_xnu_fill_devicetree (&fsbfreq);
  if (err)
    return err;

  err = holy_xnu_fill_devicetree ();
  if (err)
    return err;

  /* Page-align to avoid following parts to be inadvertently freed. */
  err = holy_xnu_align_heap (holy_XNU_PAGESIZE);
  if (err)
    return err;

  /* Pass memory map to kernel. */
  memory_map_size = 0;
  memory_map = 0;
  map_key = 0;
  descriptor_size = 0;
  descriptor_version = 0;

  holy_dprintf ("xnu", "eip=%x, efi=%p\n", holy_xnu_entry_point,
		holy_autoefi_system_table);

  const char *debug = holy_env_get ("debug");

  if (debug && (holy_strword (debug, "all") || holy_strword (debug, "xnu")))
    {
      holy_puts_ (N_("Press any key to launch xnu"));
      holy_getkey ();
    }

  /* Relocate the boot parameters to heap. */
  err = holy_xnu_heap_malloc (sizeof (*bootparams),
			      &bp_in, &bootparams_target);
  if (err)
    return err;
  bootparams = bp_in;

  holy_memset (bootparams, 0, sizeof (*bootparams));
  if (v2)
    {
      bootparams_common = &bootparams->v2.common;
      bootparams->v2.fsbfreq = fsbfreq;
      bootparams->v2.ram_size = get_total_ram();
    }
  else
    bootparams_common = &bootparams->v1.common;

  /* Set video. */
  err = holy_xnu_set_video (bootparams_common);
  if (err != holy_ERR_NONE)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
      holy_puts_ (N_("Booting in blind mode"));

      bootparams_common->lfb_mode = 0;
      bootparams_common->lfb_width = 0;
      bootparams_common->lfb_height = 0;
      bootparams_common->lfb_depth = 0;
      bootparams_common->lfb_line_len = 0;
      bootparams_common->lfb_base = 0;
    }

  if (holy_autoefi_get_memory_map (&memory_map_size, memory_map,
				   &map_key, &descriptor_size,
				   &descriptor_version) < 0)
    return holy_errno;

  /* We will do few allocations later. Reserve some space for possible
     memory map growth.  */
  memory_map_size += 20 * descriptor_size;
  err = holy_xnu_heap_malloc (memory_map_size,
			      &memory_map, &memory_map_target);
  if (err)
    return err;

  err = holy_xnu_writetree_toheap (&devtree_target, &devtreelen);
  if (err)
    return err;

  holy_memcpy (bootparams_common->cmdline, holy_xnu_cmdline,
	       sizeof (bootparams_common->cmdline));

  bootparams_common->devtree = devtree_target;
  bootparams_common->devtreelen = devtreelen;

  err = holy_autoefi_finish_boot_services (&memory_map_size, memory_map,
					   &map_key, &descriptor_size,
					   &descriptor_version);
  if (err)
    return err;

  if (v2)
    bootparams->v2.efi_system_table = (holy_addr_t) holy_autoefi_system_table;
  else
    bootparams->v1.efi_system_table = (holy_addr_t) holy_autoefi_system_table;

  firstruntimepage = (((holy_addr_t) holy_xnu_heap_target_start
		       + holy_xnu_heap_size + holy_XNU_PAGESIZE - 1)
		      / holy_XNU_PAGESIZE) + 20;
  curruntimepage = firstruntimepage;

  for (i = 0; (unsigned) i < memory_map_size / descriptor_size; i++)
    {
      holy_efi_memory_descriptor_t *curdesc = (holy_efi_memory_descriptor_t *)
	((char *) memory_map + descriptor_size * i);

      curdesc->virtual_start = curdesc->physical_start;

      if (curdesc->type == holy_EFI_RUNTIME_SERVICES_DATA
	  || curdesc->type == holy_EFI_RUNTIME_SERVICES_CODE)
	{
	  curdesc->virtual_start = curruntimepage << 12;
	  curruntimepage += curdesc->num_pages;
	  if (curdesc->physical_start
	      <= (holy_addr_t) holy_autoefi_system_table
	      && curdesc->physical_start + (curdesc->num_pages << 12)
	      > (holy_addr_t) holy_autoefi_system_table)
	    efi_system_table
	      = (holy_addr_t) holy_autoefi_system_table
	      - curdesc->physical_start + curdesc->virtual_start;
	  if (SIZEOF_OF_UINTN == 8 && holy_xnu_is_64bit)
	    curdesc->virtual_start |= 0xffffff8000000000ULL;
	}
    }

  lastruntimepage = curruntimepage;

  if (v2)
    {
      bootparams->v2.efi_uintnbits = SIZEOF_OF_UINTN * 8;
      bootparams->v2.verminor = holy_XNU_BOOTARGSV2_VERMINOR;
      bootparams->v2.vermajor = holy_XNU_BOOTARGSV2_VERMAJOR;
      bootparams->v2.efi_system_table = efi_system_table;
    }
  else
    {
      bootparams->v1.efi_uintnbits = SIZEOF_OF_UINTN * 8;
      bootparams->v1.verminor = holy_XNU_BOOTARGSV1_VERMINOR;
      bootparams->v1.vermajor = holy_XNU_BOOTARGSV1_VERMAJOR;
      bootparams->v1.efi_system_table = efi_system_table;
    }

  bootparams_common->efi_runtime_first_page = firstruntimepage;
  bootparams_common->efi_runtime_npages = lastruntimepage - firstruntimepage;
  bootparams_common->efi_mem_desc_size = descriptor_size;
  bootparams_common->efi_mem_desc_version = descriptor_version;
  bootparams_common->efi_mmap = memory_map_target;
  bootparams_common->efi_mmap_size = memory_map_size;
  bootparams_common->heap_start = holy_xnu_heap_target_start;
  bootparams_common->heap_size = curruntimepage * holy_XNU_PAGESIZE - holy_xnu_heap_target_start;

  /* Parameters for asm helper. */
  holy_xnu_stack = bootparams_common->heap_start
    + bootparams_common->heap_size + holy_XNU_PAGESIZE;
  holy_xnu_arg1 = bootparams_target;

  holy_autoefi_set_virtual_address_map (memory_map_size, descriptor_size,
					descriptor_version, memory_map);

  state.eip = holy_xnu_entry_point;
  state.eax = holy_xnu_arg1;
  state.esp = holy_xnu_stack;
  state.ebp = holy_xnu_stack;

  /* XNU uses only APIC. Disable PIC.  */
  holy_outb (0xff, 0x21);
  holy_outb (0xff, 0xa1);

  return holy_relocator32_boot (holy_xnu_relocator, state, 0);
}

static holy_command_t cmd_devprop_load;

void
holy_cpu_xnu_init (void)
{
  cmd_devprop_load = holy_register_command ("xnu_devprop_load",
					    holy_cmd_devprop_load,
					    /* TRANSLATORS: `device-properties'
					       is a variable name,
					       not a program.  */
					    0, N_("Load `device-properties' dump."));
}

void
holy_cpu_xnu_fini (void)
{
  holy_unregister_command (cmd_devprop_load);
}
