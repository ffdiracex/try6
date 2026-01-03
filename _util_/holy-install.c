/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/types.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/misc.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/file.h>
#include <holy/fs.h>
#include <holy/env.h>
#include <holy/term.h>
#include <holy/mm.h>
#include <holy/lib/hexdump.h>
#include <holy/crypto.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/zfs/zfs.h>
#include <holy/util/install.h>
#include <holy/emu/getroot.h>
#include <holy/diskfilter.h>
#include <holy/cryptodisk.h>
#include <holy/legacy_parse.h>
#include <holy/gpt_partition.h>
#include <holy/emu/config.h>
#include <holy/util/ofpath.h>
#include <holy/hfsplus.h>

#include <string.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

#include "progname.h"

static char *target;
static int removable = 0;
static int recheck = 0;
static int update_nvram = 1;
static char *install_device = NULL;
static char *debug_image = NULL;
static char *rootdir = NULL;
static char *bootdir = NULL;
static int allow_floppy = 0;
static int force_file_id = 0;
static char *disk_module = NULL;
static char *efidir = NULL;
static char *macppcdir = NULL;
static int force = 0;
static int have_abstractions = 0;
static int have_cryptodisk = 0;
static char * bootloader_id;
static int have_load_cfg = 0;
static FILE * load_cfg_f = NULL;
static char *load_cfg;
static int install_bootsector = 1;
static char *label_font;
static char *label_color;
static char *label_bgcolor;
static char *product_version;
static int add_rs_codes = 1;

enum
  {
    OPTION_BOOT_DIRECTORY = 0x301,
    OPTION_ROOT_DIRECTORY,
    OPTION_TARGET,
    OPTION_SETUP,
    OPTION_MKRELPATH, 
    OPTION_MKDEVICEMAP, 
    OPTION_PROBE, 
    OPTION_EDITENV, 
    OPTION_ALLOW_FLOPPY, 
    OPTION_RECHECK, 
    OPTION_FORCE,
    OPTION_FORCE_FILE_ID,
    OPTION_NO_NVRAM, 
    OPTION_REMOVABLE, 
    OPTION_BOOTLOADER_ID, 
    OPTION_EFI_DIRECTORY,
    OPTION_FONT,
    OPTION_DEBUG,
    OPTION_DEBUG_IMAGE,
    OPTION_NO_FLOPPY,
    OPTION_DISK_MODULE,
    OPTION_NO_BOOTSECTOR,
    OPTION_NO_RS_CODES,
    OPTION_MACPPC_DIRECTORY,
    OPTION_LABEL_FONT,
    OPTION_LABEL_COLOR,
    OPTION_LABEL_BGCOLOR,
    OPTION_PRODUCT_VERSION
  };

static int fs_probe = 1;

static error_t 
argp_parser (int key, char *arg, struct argp_state *state)
{
  if (holy_install_parse (key, arg))
    return 0;
  switch (key)
    {
    case OPTION_FORCE_FILE_ID:
      force_file_id = 1;
      return 0;
    case 's':
      fs_probe = 0;
      return 0;

    case OPTION_SETUP:
      if (!holy_strstr (arg, "setup"))
	install_bootsector = 0;
      return 0;

    case OPTION_PRODUCT_VERSION:
      free (product_version);
      product_version = xstrdup (arg);
      return 0;
    case OPTION_LABEL_FONT:
      free (label_font);
      label_font = xstrdup (arg);
      return 0;

    case OPTION_LABEL_COLOR:
      free (label_color);
      label_color = xstrdup (arg);
      return 0;

    case OPTION_LABEL_BGCOLOR:
      free (label_bgcolor);
      label_bgcolor = xstrdup (arg);
      return 0;

      /* Accept and ignore for compatibility.  */
    case OPTION_FONT:
    case OPTION_MKRELPATH:
    case OPTION_PROBE:
    case OPTION_EDITENV:
    case OPTION_MKDEVICEMAP:
    case OPTION_NO_FLOPPY:
      return 0;
    case OPTION_ROOT_DIRECTORY:
      /* Accept for compatibility.  */
      free (rootdir);
      rootdir = xstrdup (arg);
      return 0;

    case OPTION_BOOT_DIRECTORY:
      free (bootdir);
      bootdir = xstrdup (arg);
      return 0;

    case OPTION_MACPPC_DIRECTORY:
      free (macppcdir);
      macppcdir = xstrdup (arg);
      return 0;

    case OPTION_EFI_DIRECTORY:
      free (efidir);
      efidir = xstrdup (arg);
      return 0;

    case OPTION_DISK_MODULE:
      free (disk_module);
      disk_module = xstrdup (arg);
      return 0;

    case OPTION_TARGET:
      free (target);
      target = xstrdup (arg);
      return 0;

    case OPTION_DEBUG_IMAGE:
      free (debug_image);
      debug_image = xstrdup (arg);
      return 0;

    case OPTION_NO_NVRAM:
      update_nvram = 0;
      return 0;

    case OPTION_FORCE:
      force = 1;
      return 0;

    case OPTION_RECHECK:
      recheck = 1;
      return 0;

    case OPTION_REMOVABLE:
      removable = 1;
      return 0;

    case OPTION_ALLOW_FLOPPY:
      allow_floppy = 1;
      return 0;

    case OPTION_NO_BOOTSECTOR:
      install_bootsector = 0;
      return 0;

    case OPTION_NO_RS_CODES:
      add_rs_codes = 0;
      return 0;

    case OPTION_DEBUG:
      verbosity++;
      return 0;

    case OPTION_BOOTLOADER_ID:
      free (bootloader_id);
      bootloader_id = xstrdup (arg);
      return 0;

    case ARGP_KEY_ARG:
      if (install_device)
	holy_util_error ("%s", _("More than one install device?"));
      install_device = xstrdup (arg);
      return 0;

    default:
      return ARGP_ERR_UNKNOWN;
    }
}


static struct argp_option options[] = {
  holy_INSTALL_OPTIONS,
  {"boot-directory", OPTION_BOOT_DIRECTORY, N_("DIR"),
   0, N_("install holy images under the directory DIR/%s instead of the %s directory"), 2},
  {"root-directory", OPTION_ROOT_DIRECTORY, N_("DIR"),
   OPTION_HIDDEN, 0, 2},
  {"font", OPTION_FONT, N_("FILE"),
   OPTION_HIDDEN, 0, 2},
  {"target", OPTION_TARGET, N_("TARGET"),
   /* TRANSLATORS: "TARGET" as in "target platform".  */
   0, N_("install holy for TARGET platform [default=%s]; available targets: %s"), 2},
  {"holy-setup", OPTION_SETUP, "FILE", OPTION_HIDDEN, 0, 2},
  {"holy-mkrelpath", OPTION_MKRELPATH, "FILE", OPTION_HIDDEN, 0, 2},
  {"holy-mkdevicemap", OPTION_MKDEVICEMAP, "FILE", OPTION_HIDDEN, 0, 2},
  {"holy-probe", OPTION_PROBE, "FILE", OPTION_HIDDEN, 0, 2},
  {"holy-editenv", OPTION_EDITENV, "FILE", OPTION_HIDDEN, 0, 2},
  {"allow-floppy", OPTION_ALLOW_FLOPPY, 0, 0,
   /* TRANSLATORS: "may break" doesn't just mean that option wouldn't have any
      effect but that it will make the resulting install unbootable from HDD. */
   N_("make the drive also bootable as floppy (default for fdX devices)."
      " May break on some BIOSes."), 2},
  {"recheck", OPTION_RECHECK, 0, 0,
   N_("delete device map if it already exists"), 2},
  {"force", OPTION_FORCE, 0, 0,
   N_("install even if problems are detected"), 2},
  {"force-file-id", OPTION_FORCE_FILE_ID, 0, 0,
   N_("use identifier file even if UUID is available"), 2},
  {"disk-module", OPTION_DISK_MODULE, N_("MODULE"), 0,
   N_("disk module to use (biosdisk or native). "
      "This option is only available on BIOS target."), 2},
  {"no-nvram", OPTION_NO_NVRAM, 0, 0,
   N_("don't update the `boot-device'/`Boot*' NVRAM variables. "
      "This option is only available on EFI and IEEE1275 targets."), 2},
  {"skip-fs-probe",'s',0,      0,
   N_("do not probe for filesystems in DEVICE"), 0},
  {"no-bootsector", OPTION_NO_BOOTSECTOR, 0, 0,
   N_("do not install bootsector"), 0},
  {"no-rs-codes", OPTION_NO_RS_CODES, 0, 0,
   N_("Do not apply any reed-solomon codes when embedding core.img. "
      "This option is only available on x86 BIOS targets."), 0},

  {"debug", OPTION_DEBUG, 0, OPTION_HIDDEN, 0, 2},
  {"no-floppy", OPTION_NO_FLOPPY, 0, OPTION_HIDDEN, 0, 2},
  {"debug-image", OPTION_DEBUG_IMAGE, N_("STRING"), OPTION_HIDDEN, 0, 2},
  {"removable", OPTION_REMOVABLE, 0, 0,
   N_("the installation device is removable. "
      "This option is only available on EFI."), 2},
  {"bootloader-id", OPTION_BOOTLOADER_ID, N_("ID"), 0,
   N_("the ID of bootloader. This option is only available on EFI and Macs."), 2},
  {"efi-directory", OPTION_EFI_DIRECTORY, N_("DIR"), 0,
   N_("use DIR as the EFI System Partition root."), 2},
  {"macppc-directory", OPTION_MACPPC_DIRECTORY, N_("DIR"), 0,
   N_("use DIR for PPC MAC install."), 2},
  {"label-font", OPTION_LABEL_FONT, N_("FILE"), 0, N_("use FILE as font for label"), 2},
  {"label-color", OPTION_LABEL_COLOR, N_("COLOR"), 0, N_("use COLOR for label"), 2},
  {"label-bgcolor", OPTION_LABEL_BGCOLOR, N_("COLOR"), 0, N_("use COLOR for label background"), 2},
  {"product-version", OPTION_PRODUCT_VERSION, N_("STRING"), 0, N_("use STRING as product version"), 2},
  {0, 0, 0, 0, 0, 0}
};

static const char *
get_default_platform (void)
{
#ifdef __powerpc__
   return "powerpc-ieee1275";
#elif defined (__sparc__) || defined (__sparc64__)
   return "sparc64-ieee1275";
#elif defined (__MIPSEL__)
   return "mipsel-loongson";
#elif defined (__MIPSEB__)
   return "mips-arc";
#elif defined (__ia64__)
   return "ia64-efi";
#elif defined (__arm__)
   return "arm-uboot";
#elif defined (__aarch64__)
   return "arm64-efi";
#elif defined (__amd64__) || defined (__x86_64__) || defined (__i386__)
   return holy_install_get_default_x86_platform ();
#else
   return NULL;
#endif
}

#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static char *
help_filter (int key, const char *text, void *input __attribute__ ((unused)))
{
  switch (key)
    {
    case OPTION_BOOT_DIRECTORY:
      return xasprintf (text, holy_DIR_NAME, holy_BOOT_DIR_NAME "/" holy_DIR_NAME);
    case OPTION_TARGET:
      {
	char *plats = holy_install_get_platforms_string ();
	char *ret;
	ret = xasprintf (text, get_default_platform (), plats);
	free (plats);
	return ret;
      }
    case ARGP_KEY_HELP_POST_DOC:
      return xasprintf (text, program_name, holy_BOOT_DIR_NAME "/" holy_DIR_NAME);
    default:
      return holy_install_help_filter (key, text, input);
    }
}

#pragma GCC diagnostic error "-Wformat-nonliteral"

/* TRANSLATORS: INSTALL_DEVICE isn't an identifier and is the DEVICE you
   install to.  */
struct argp argp = {
  options, argp_parser, N_("[OPTION] [INSTALL_DEVICE]"),
  N_("Install holy on your drive.")"\v"
  N_("INSTALL_DEVICE must be system device filename.\n"
     "%s copies holy images into %s.  On some platforms, it"
     " may also install holy into the boot sector."),
  NULL, help_filter, NULL
};

static int
probe_raid_level (holy_disk_t disk)
{
  /* disk might be NULL in the case of a LVM physical volume with no LVM
     signature.  Ignore such cases here.  */
  if (!disk)
    return -1;

  if (disk->dev->id != holy_DISK_DEVICE_DISKFILTER_ID)
    return -1;

  if (disk->name[0] != 'm' || disk->name[1] != 'd')
    return -1;

  if (!((struct holy_diskfilter_lv *) disk->data)->segments)
    return -1;
  return ((struct holy_diskfilter_lv *) disk->data)->segments->type;
}

static void
push_partmap_module (const char *map, void *data __attribute__ ((unused)))
{
  char buf[50];

  if (strcmp (map, "openbsd") == 0 || strcmp (map, "netbsd") == 0)
    {
      holy_install_push_module ("part_bsd");
      return;
    }

  snprintf (buf, sizeof (buf), "part_%s", map);
  holy_install_push_module (buf);
}

static void
push_cryptodisk_module (const char *mod, void *data __attribute__ ((unused)))
{
  holy_install_push_module (mod);
}

static void
probe_mods (holy_disk_t disk)
{
  holy_partition_t part;
  holy_disk_memberlist_t list = NULL, tmp;
  int raid_level;

  if (disk->partition == NULL)
    holy_util_info ("no partition map found for %s", disk->name);

  for (part = disk->partition; part; part = part->parent)
    push_partmap_module (part->partmap->name, NULL);

  if (disk->dev->id == holy_DISK_DEVICE_DISKFILTER_ID)
    {
      holy_diskfilter_get_partmap (disk, push_partmap_module, NULL);
      have_abstractions = 1;
    }

  if (disk->dev->id == holy_DISK_DEVICE_DISKFILTER_ID
      && (holy_memcmp (disk->name, "lvm/", sizeof ("lvm/") - 1) == 0 ||
	  holy_memcmp (disk->name, "lvmid/", sizeof ("lvmid/") - 1) == 0))
    holy_install_push_module ("lvm");

  if (disk->dev->id == holy_DISK_DEVICE_DISKFILTER_ID
      && holy_memcmp (disk->name, "ldm/", sizeof ("ldm/") - 1) == 0)
    holy_install_push_module ("ldm");

  if (disk->dev->id == holy_DISK_DEVICE_CRYPTODISK_ID)
    {
      holy_util_cryptodisk_get_abstraction (disk,
					    push_cryptodisk_module, NULL);
      have_abstractions = 1;
      have_cryptodisk = 1;
    }

  raid_level = probe_raid_level (disk);
  if (raid_level >= 0)
    {
      holy_install_push_module ("diskfilter");
      if (disk->dev->raidname)
	holy_install_push_module (disk->dev->raidname (disk));
    }
  if (raid_level == 5)
    holy_install_push_module ("raid5rec");
  if (raid_level == 6)
    holy_install_push_module ("raid6rec");

  /* In case of LVM/RAID, check the member devices as well.  */
  if (disk->dev->memberlist)
    list = disk->dev->memberlist (disk);
  while (list)
    {
      probe_mods (list->disk);
      tmp = list->next;
      free (list);
      list = tmp;
    }
}

static int
have_bootdev (enum holy_install_plat pl)
{
  switch (pl)
    {
    case holy_INSTALL_PLATFORM_I386_PC:
    case holy_INSTALL_PLATFORM_I386_EFI:
    case holy_INSTALL_PLATFORM_X86_64_EFI:
    case holy_INSTALL_PLATFORM_IA64_EFI:
    case holy_INSTALL_PLATFORM_ARM_EFI:
    case holy_INSTALL_PLATFORM_ARM64_EFI:
    case holy_INSTALL_PLATFORM_I386_IEEE1275:
    case holy_INSTALL_PLATFORM_SPARC64_IEEE1275:
    case holy_INSTALL_PLATFORM_POWERPC_IEEE1275:
    case holy_INSTALL_PLATFORM_MIPSEL_ARC:
    case holy_INSTALL_PLATFORM_MIPS_ARC:
      return 1;

    case holy_INSTALL_PLATFORM_I386_QEMU:
    case holy_INSTALL_PLATFORM_I386_COREBOOT:
    case holy_INSTALL_PLATFORM_I386_MULTIBOOT:
    case holy_INSTALL_PLATFORM_MIPSEL_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_MIPS_QEMU_MIPS:

    case holy_INSTALL_PLATFORM_MIPSEL_LOONGSON:
    case holy_INSTALL_PLATFORM_ARM_UBOOT:

    case holy_INSTALL_PLATFORM_I386_XEN:
    case holy_INSTALL_PLATFORM_X86_64_XEN:
      return 0;

      /* pacify warning.  */
    case holy_INSTALL_PLATFORM_MAX:
      return 0;
    }
  return 0;
}

static void
probe_cryptodisk_uuid (holy_disk_t disk)
{
  holy_disk_memberlist_t list = NULL, tmp;

  /* In case of LVM/RAID, check the member devices as well.  */
  if (disk->dev->memberlist)
    {
      list = disk->dev->memberlist (disk);
    }
  while (list)
    {
      probe_cryptodisk_uuid (list->disk);
      tmp = list->next;
      free (list);
      list = tmp;
    }
  if (disk->dev->id == holy_DISK_DEVICE_CRYPTODISK_ID)
    {
      const char *uuid = holy_util_cryptodisk_get_uuid (disk);
      if (!load_cfg_f)
	load_cfg_f = holy_util_fopen (load_cfg, "wb");
      have_load_cfg = 1;

      fprintf (load_cfg_f, "cryptomount -u %s\n",
	      uuid);
    }
}

static int
is_same_disk (const char *a, const char *b)
{
  while (1)
    {
      if ((*a == ',' || *a == '\0') && (*b == ',' || *b == '\0'))
	return 1;
      if (*a != *b)
	return 0;
      if (*a == '\\')
	{
	  if (a[1] != b[1])
	    return 0;
	  a += 2;
	  b += 2;
	  continue;
	}
      a++;
      b++;
    }
}

static char *
get_rndstr (void)
{
  holy_uint8_t rnd[15];
  const size_t sz = sizeof (rnd) * holy_CHAR_BIT / 5;
  char * ret = xmalloc (sz + 1);
  size_t i;
  if (holy_get_random (rnd, sizeof (rnd)))
    holy_util_error ("%s", _("couldn't retrieve random data"));
  for (i = 0; i < sz; i++)
    {
      holy_size_t b = i * 5;
      holy_uint8_t r;
      holy_size_t f1 = holy_CHAR_BIT - b % holy_CHAR_BIT;
      holy_size_t f2;
      if (f1 > 5)
	f1 = 5;
      f2 = 5 - f1;
      r = (rnd[b / holy_CHAR_BIT] >> (b % holy_CHAR_BIT)) & ((1 << f1) - 1);
      if (f2)
	r |= (rnd[b / holy_CHAR_BIT + 1] & ((1 << f2) - 1)) << f1;
      if (r < 10)
	ret[i] = '0' + r;
      else
	ret[i] = 'a' + (r - 10);
    }
  ret[sz] = '\0';
  return ret;
}

static char *
escape (const char *in)
{
  char *ptr;
  char *ret;
  int overhead = 0;

  for (ptr = (char*)in; *ptr; ptr++)
    if (*ptr == '\'')
      overhead += 3;
  ret = holy_malloc (ptr - in + overhead + 1);
  if (!ret)
    return NULL;

  holy_strchrsub (ret, in, '\'', "'\\''");
  return ret;
}

static struct holy_util_config config;

static void
device_map_check_duplicates (const char *dev_map)
{
  FILE *fp;
  char buf[1024];	/* XXX */
  size_t alloced = 8;
  size_t filled = 0;
  char **d;
  size_t i;

  if (dev_map[0] == '\0')
    return;

  fp = holy_util_fopen (dev_map, "r");
  if (! fp)
    return;

  d = xmalloc (alloced * sizeof (d[0]));

  while (fgets (buf, sizeof (buf), fp))
    {
      char *p = buf;
      char *e;

      /* Skip leading spaces.  */
      while (*p && holy_isspace (*p))
	p++;

      /* If the first character is `#' or NUL, skip this line.  */
      if (*p == '\0' || *p == '#')
	continue;

      if (*p != '(')
	continue;

      p++;

      e = p;
      p = strchr (p, ')');
      if (! p)
	continue;

      if (filled >= alloced)
	{
	  alloced *= 2;
	  d = xrealloc (d, alloced * sizeof (d[0]));
	}

      *p = '\0';

      d[filled++] = xstrdup (e);
    }

  fclose (fp);

  qsort (d, filled, sizeof (d[0]), holy_qsort_strcmp);

  for (i = 0; i + 1 < filled; i++)
    if (strcmp (d[i], d[i+1]) == 0)
      {
	holy_util_error (_("the drive %s is defined multiple times in the device map %s"),
			 d[i], dev_map);
      }

  for (i = 0; i < filled; i++)
    free (d[i]);

  free (d);
}

static holy_err_t
write_to_disk (holy_device_t dev, const char *fn)
{
  char *core_img;
  size_t core_size;
  holy_err_t err;

  core_size = holy_util_get_image_size (fn);

  core_img = holy_util_read_image (fn);

  holy_util_info ("writing `%s' to `%s'", fn, dev->disk->name);
  err = holy_disk_write (dev->disk, 0, 0,
			 core_size, core_img);
  free (core_img);
  return err;
}

static int
is_prep_partition (holy_device_t dev)
{
  if (!dev->disk)
    return 0;
  if (!dev->disk->partition)
    return 0;
  if (strcmp(dev->disk->partition->partmap->name, "msdos") == 0)
    return (dev->disk->partition->msdostype == 0x41);

  if (strcmp (dev->disk->partition->partmap->name, "gpt") == 0)
    {
      struct holy_gpt_partentry gptdata;
      holy_partition_t p = dev->disk->partition;
      int ret = 0;
      dev->disk->partition = dev->disk->partition->parent;

      if (holy_disk_read (dev->disk, p->offset, p->index,
			  sizeof (gptdata), &gptdata) == 0)
	{
	  const holy_gpt_part_type_t template = {
	    holy_cpu_to_le32_compile_time (0x9e1a2d38),
	    holy_cpu_to_le16_compile_time (0xc612),
	    holy_cpu_to_le16_compile_time (0x4316),
	    { 0xaa, 0x26, 0x8b, 0x49, 0x52, 0x1e, 0x5a, 0x8b }
	  };

	  ret = holy_memcmp (&template, &gptdata.type,
			     sizeof (template)) == 0;
	}
      dev->disk->partition = p;
      return ret;
    }

  return 0;
}

static int
is_prep_empty (holy_device_t dev)
{
  holy_disk_addr_t dsize, addr;
  holy_uint32_t buffer[32768];

  dsize = holy_disk_get_size (dev->disk);
  for (addr = 0; addr < dsize;
       addr += sizeof (buffer) / holy_DISK_SECTOR_SIZE)
    {
      holy_size_t sz = sizeof (buffer);
      holy_uint32_t *ptr;

      if (sizeof (buffer) / holy_DISK_SECTOR_SIZE > dsize - addr)
	sz = (dsize - addr) * holy_DISK_SECTOR_SIZE;
      holy_disk_read (dev->disk, addr, 0, sz, buffer);

      if (addr == 0 && holy_memcmp (buffer, ELFMAG, SELFMAG) == 0)
	return 1;

      for (ptr = buffer; ptr < buffer + sz / sizeof (*buffer); ptr++)
	if (*ptr)
	  return 0;
    }

  return 1;
}

static void
bless (holy_device_t dev, const char *path, int x86)
{
  struct stat st;
  holy_err_t err;

  holy_util_info ("blessing %s", path);

  if (stat (path, &st) < 0)
    holy_util_error (N_("cannot stat `%s': %s"),
		     path, strerror (errno));

  err = holy_mac_bless_inode (dev, st.st_ino, S_ISDIR (st.st_mode), x86);
  if (err)
    holy_util_error ("%s", holy_errmsg);
  holy_util_info ("blessed");
}

static void
fill_core_services (const char *core_services)
{
  char *label;
  FILE *f;
  char *label_text;
  char *label_string = xasprintf ("%s %s", bootloader_id, product_version);
  char *sysv_plist;

  label = holy_util_path_concat (2, core_services, ".disk_label");
  holy_util_info ("rendering label %s", label_string);
  holy_util_render_label (label_font, label_bgcolor ? : "white",
			  label_color ? : "black", label_string, label);
  holy_util_info ("label rendered");
  free (label);
  label_text = holy_util_path_concat (2, core_services, ".disk_label.contentDetails");
  f = holy_util_fopen (label_text, "wb");
  fprintf (f, "%s\n", label_string);
  fclose (f);
  free (label_string);
  free (label_text);

  sysv_plist = holy_util_path_concat (2, core_services, "SystemVersion.plist");
  f = holy_util_fopen (sysv_plist, "wb");
  fprintf (f,
	   "<plist version=\"1.0\">\n"
	   "<dict>\n"
	   "        <key>ProductBuildVersion</key>\n"
	   "        <string></string>\n"
	   "        <key>ProductName</key>\n"
	   "        <string>%s</string>\n"
	   "        <key>ProductVersion</key>\n"
	   "        <string>%s</string>\n"
	   "</dict>\n"
	   "</plist>\n", bootloader_id, product_version);
  fclose (f);
  free (sysv_plist);
}

int
main (int argc, char *argv[])
{
  int is_efi = 0;
  const char *efi_distributor = NULL;
  const char *efi_file = NULL;
  char **holy_devices;
  holy_fs_t holy_fs;
  holy_device_t holy_dev = NULL;
  enum holy_install_plat platform;
  char *holydir, *device_map;
  char **curdev, **curdrive;
  char **holy_drives;
  char *relative_holydir;
  char **efidir_device_names = NULL;
  holy_device_t efidir_holy_dev = NULL;
  char *efidir_holy_devname;
  int efidir_is_mac = 0;
  int is_prep = 0;
  const char *pkgdatadir;

  holy_util_host_init (&argc, &argv);
  product_version = xstrdup (PACKAGE_VERSION);
  pkgdatadir = holy_util_get_pkgdatadir ();
  label_font = holy_util_path_concat (2, pkgdatadir, "unicode.pf2");

  argp_parse (&argp, argc, argv, 0, 0, 0);

  if (verbosity > 1)
    holy_env_set ("debug", "all");

  holy_util_load_config (&config);

  if (!bootloader_id && config.holy_distributor)
    {
      char *ptr;
      bootloader_id = xstrdup (config.holy_distributor);
      for (ptr = bootloader_id; *ptr && *ptr != ' '; ptr++)
	if (*ptr >= 'A' && *ptr <= 'Z')
	  *ptr = *ptr - 'A' + 'a';
      *ptr = '\0';
    }
  if (!bootloader_id || bootloader_id[0] == '\0')
    {
      free (bootloader_id);
      bootloader_id = xstrdup ("holy");
    }

  if (!holy_install_source_directory)
    {
      if (!target)
	{
	  const char * t;
	  t = get_default_platform ();
	  if (!t)
	    holy_util_error ("%s",
			     _("Unable to determine your platform."
			       " Use --target.")
			     );
	  target = xstrdup (t); 
	}
      holy_install_source_directory
	= holy_util_path_concat (2, holy_util_get_pkglibdir (), target);
    }

  platform = holy_install_get_target (holy_install_source_directory);

  {
    char *platname = holy_install_get_platform_name (platform);
    fprintf (stderr, _("Installing for %s platform.\n"), platname);
    free (platname);
  }

  switch (platform)
    {
    case holy_INSTALL_PLATFORM_I386_PC:
      if (!disk_module)
	disk_module = xstrdup ("biosdisk");
      break;
    case holy_INSTALL_PLATFORM_I386_EFI:
    case holy_INSTALL_PLATFORM_X86_64_EFI:
    case holy_INSTALL_PLATFORM_ARM_EFI:
    case holy_INSTALL_PLATFORM_ARM64_EFI:
    case holy_INSTALL_PLATFORM_IA64_EFI:
    case holy_INSTALL_PLATFORM_I386_IEEE1275:
    case holy_INSTALL_PLATFORM_SPARC64_IEEE1275:
    case holy_INSTALL_PLATFORM_POWERPC_IEEE1275:
    case holy_INSTALL_PLATFORM_MIPSEL_ARC:
    case holy_INSTALL_PLATFORM_MIPS_ARC:
    case holy_INSTALL_PLATFORM_ARM_UBOOT:
    case holy_INSTALL_PLATFORM_I386_XEN:
    case holy_INSTALL_PLATFORM_X86_64_XEN:
      break;

    case holy_INSTALL_PLATFORM_I386_QEMU:
    case holy_INSTALL_PLATFORM_I386_COREBOOT:
    case holy_INSTALL_PLATFORM_I386_MULTIBOOT:
    case holy_INSTALL_PLATFORM_MIPSEL_LOONGSON:
    case holy_INSTALL_PLATFORM_MIPSEL_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_MIPS_QEMU_MIPS:
      disk_module = xstrdup ("native");
      break;

      /* pacify warning.  */
    case holy_INSTALL_PLATFORM_MAX:
      break;
    }

  switch (platform)
    {
    case holy_INSTALL_PLATFORM_I386_PC:
    case holy_INSTALL_PLATFORM_SPARC64_IEEE1275:
      if (!install_device)
	holy_util_error ("%s", _("install device isn't specified"));
      break;
    case holy_INSTALL_PLATFORM_POWERPC_IEEE1275:
      if (install_device)
	is_prep = 1;
      break;
    case holy_INSTALL_PLATFORM_MIPS_ARC:
    case holy_INSTALL_PLATFORM_MIPSEL_ARC:
      break;
    case holy_INSTALL_PLATFORM_I386_EFI:
    case holy_INSTALL_PLATFORM_X86_64_EFI:
    case holy_INSTALL_PLATFORM_ARM_EFI:
    case holy_INSTALL_PLATFORM_ARM64_EFI:
    case holy_INSTALL_PLATFORM_IA64_EFI:
    case holy_INSTALL_PLATFORM_I386_IEEE1275:
    case holy_INSTALL_PLATFORM_ARM_UBOOT:
    case holy_INSTALL_PLATFORM_I386_QEMU:
    case holy_INSTALL_PLATFORM_I386_COREBOOT:
    case holy_INSTALL_PLATFORM_I386_MULTIBOOT:
    case holy_INSTALL_PLATFORM_MIPSEL_LOONGSON:
    case holy_INSTALL_PLATFORM_MIPSEL_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_MIPS_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_I386_XEN:
    case holy_INSTALL_PLATFORM_X86_64_XEN:
      free (install_device);
      install_device = NULL;
      break;

      /* pacify warning.  */
    case holy_INSTALL_PLATFORM_MAX:
      break;
    }

  if (!bootdir)
    bootdir = holy_util_path_concat (3, "/", rootdir, holy_BOOT_DIR_NAME);

  {
    char * t = holy_util_path_concat (2, bootdir, holy_DIR_NAME);
    holy_install_mkdir_p (t);
    holydir = holy_canonicalize_file_name (t);
    if (!holydir)
      holy_util_error (_("failed to get canonical path of `%s'"), t);
    free (t);
  }
  device_map = holy_util_path_concat (2, holydir, "device.map");

  if (recheck)
    holy_util_unlink (device_map);

  device_map_check_duplicates (device_map);
  holy_util_biosdisk_init (device_map);

  /* Initialize all modules. */
  holy_init_all ();
  holy_gcry_init_all ();
  holy_hostfs_init ();
  holy_host_init ();

  switch (platform)
    {
    case holy_INSTALL_PLATFORM_I386_EFI:
    case holy_INSTALL_PLATFORM_X86_64_EFI:
    case holy_INSTALL_PLATFORM_ARM_EFI:
    case holy_INSTALL_PLATFORM_ARM64_EFI:
    case holy_INSTALL_PLATFORM_IA64_EFI:
      is_efi = 1;
      break;
    default:
      is_efi = 0;
      break;

      /* pacify warning.  */
    case holy_INSTALL_PLATFORM_MAX:
      break;
    }

  /* Find the EFI System Partition.  */

  if (is_efi)
    {
      holy_fs_t fs;
      free (install_device);
      install_device = NULL;
      if (!efidir)
	{
	  char *d = holy_util_path_concat (2, bootdir, "efi");
	  char *dr = NULL;
	  if (!holy_util_is_directory (d))
	    {
	      free (d);
	      d = holy_util_path_concat (2, bootdir, "EFI");
	    }
	  /*
	    The EFI System Partition may have been given directly using
	    --root-directory.
	  */
	  if (!holy_util_is_directory (d)
	      && rootdir && holy_strcmp (rootdir, "/") != 0)
	    {
	      free (d);
	      d = xstrdup (rootdir);
	    }
	  if (holy_util_is_directory (d))
	    dr = holy_make_system_path_relative_to_its_root (d);
	  /* Is it a mount point? */
	  if (dr && dr[0] == '\0')
	    efidir = d;
	  else
	    free (d);
	  free (dr);
	}
      if (!efidir)
	holy_util_error ("%s", _("cannot find EFI directory"));
      efidir_device_names = holy_guess_root_devices (efidir);
      if (!efidir_device_names || !efidir_device_names[0])
	holy_util_error (_("cannot find a device for %s (is /dev mounted?)"),
			     efidir);
      install_device = efidir_device_names[0];

      for (curdev = efidir_device_names; *curdev; curdev++)
	  holy_util_pull_device (*curdev);
      
      efidir_holy_devname = holy_util_get_holy_dev (efidir_device_names[0]);
      if (!efidir_holy_devname)
	holy_util_error (_("cannot find a holy drive for %s.  Check your device.map"),
			 efidir_device_names[0]);

      efidir_holy_dev = holy_device_open (efidir_holy_devname);
      if (! efidir_holy_dev)
	holy_util_error ("%s", holy_errmsg);

      fs = holy_fs_probe (efidir_holy_dev);
      if (! fs)
	holy_util_error ("%s", holy_errmsg);

      efidir_is_mac = 0;

      if (holy_strcmp (fs->name, "hfs") == 0
	  || holy_strcmp (fs->name, "hfsplus") == 0)
	efidir_is_mac = 1;

      if (!efidir_is_mac && holy_strcmp (fs->name, "fat") != 0)
	holy_util_error (_("%s doesn't look like an EFI partition"), efidir);

      /* The EFI specification requires that an EFI System Partition must
	 contain an "EFI" subdirectory, and that OS loaders are stored in
	 subdirectories below EFI.  Vendors are expected to pick names that do
	 not collide with other vendors.  To minimise collisions, we use the
	 name of our distributor if possible.
      */
      char *t;
      efi_distributor = bootloader_id;
      if (removable)
	{
	  /* The specification makes stricter requirements of removable
	     devices, in order that only one image can be automatically loaded
	     from them.  The image must always reside under /EFI/BOOT, and it
	     must have a specific file name depending on the architecture.
	  */
	  efi_distributor = "BOOT";
	  switch (platform)
	    {
	    case holy_INSTALL_PLATFORM_I386_EFI:
	      efi_file = "BOOTIA32.EFI";
	      break;
	    case holy_INSTALL_PLATFORM_X86_64_EFI:
	      efi_file = "BOOTX64.EFI";
	      break;
	    case holy_INSTALL_PLATFORM_IA64_EFI:
	      efi_file = "BOOTIA64.EFI";
	      break;
	    case holy_INSTALL_PLATFORM_ARM_EFI:
	      efi_file = "BOOTARM.EFI";
	      break;
	    case holy_INSTALL_PLATFORM_ARM64_EFI:
	      efi_file = "BOOTAA64.EFI";
	      break;
	    default:
	      holy_util_error ("%s", _("You've found a bug"));
	      break;
	    }
	}
      else
	{
	  /* It is convenient for each architecture to have a different
	     efi_file, so that different versions can be installed in parallel.
	  */
	  switch (platform)
	    {
	    case holy_INSTALL_PLATFORM_I386_EFI:
	      efi_file = "holyia32.efi";
	      break;
	    case holy_INSTALL_PLATFORM_X86_64_EFI:
	      efi_file = "holyx64.efi";
	      break;
	    case holy_INSTALL_PLATFORM_IA64_EFI:
	      efi_file = "holyia64.efi";
	      break;
	    case holy_INSTALL_PLATFORM_ARM_EFI:
	      efi_file = "holyarm.efi";
	      break;
	    case holy_INSTALL_PLATFORM_ARM64_EFI:
	      efi_file = "holyaa64.efi";
	      break;
	    default:
	      efi_file = "holy.efi";
	      break;
	    }
	}
      t = holy_util_path_concat (3, efidir, "EFI", efi_distributor);
      free (efidir);
      efidir = t;
      holy_install_mkdir_p (efidir);
    }

  if (platform == holy_INSTALL_PLATFORM_POWERPC_IEEE1275)
    {
      int is_guess = 0;
      if (!macppcdir)
	{
	  char *d;

	  is_guess = 1;
	  d = holy_util_path_concat (2, bootdir, "macppc");
	  if (!holy_util_is_directory (d))
	    {
	      free (d);
	      d = holy_util_path_concat (2, bootdir, "efi");
	    }
	  /* Find the Mac HFS(+) System Partition.  */
	  if (!holy_util_is_directory (d))
	    {
	      free (d);
	      d = holy_util_path_concat (2, bootdir, "EFI");
	    }
	  if (!holy_util_is_directory (d))
	    {
	      free (d);
	      d = 0;
	    }
	  if (d)
	    macppcdir = d;
	}
      if (macppcdir)
	{
	  char **macppcdir_device_names = NULL;
	  holy_device_t macppcdir_holy_dev = NULL;
	  char *macppcdir_holy_devname;
	  holy_fs_t fs;

	  macppcdir_device_names = holy_guess_root_devices (macppcdir);
	  if (!macppcdir_device_names || !macppcdir_device_names[0])
	    holy_util_error (_("cannot find a device for %s (is /dev mounted?)"),
			     macppcdir);

	  for (curdev = macppcdir_device_names; *curdev; curdev++)
	    holy_util_pull_device (*curdev);
      
	  macppcdir_holy_devname = holy_util_get_holy_dev (macppcdir_device_names[0]);
	  if (!macppcdir_holy_devname)
	    holy_util_error (_("cannot find a holy drive for %s.  Check your device.map"),
			     macppcdir_device_names[0]);
	  
	  macppcdir_holy_dev = holy_device_open (macppcdir_holy_devname);
	  if (! macppcdir_holy_dev)
	    holy_util_error ("%s", holy_errmsg);

	  fs = holy_fs_probe (macppcdir_holy_dev);
	  if (! fs)
	    holy_util_error ("%s", holy_errmsg);

	  if (holy_strcmp (fs->name, "hfs") != 0
	      && holy_strcmp (fs->name, "hfsplus") != 0
	      && !is_guess)
	    holy_util_error (_("filesystem on %s is neither HFS nor HFS+"),
			     macppcdir);
	  if (holy_strcmp (fs->name, "hfs") == 0
	      || holy_strcmp (fs->name, "hfsplus") == 0)
	    {
	      install_device = macppcdir_device_names[0];
	      is_prep = 0;
	    }
	}
    }

  holy_install_copy_files (holy_install_source_directory,
			   holydir, platform);

  char *envfile = holy_util_path_concat (2, holydir, "holyenv");
  if (!holy_util_is_regular (envfile))
    holy_util_create_envblk_file (envfile);

  size_t ndev = 0;

  /* Write device to a variable so we don't have to traverse /dev every time.  */
  holy_devices = holy_guess_root_devices (holydir);
  if (!holy_devices || !holy_devices[0])
    holy_util_error (_("cannot find a device for %s (is /dev mounted?)"),
		     holydir);

  for (curdev = holy_devices; *curdev; curdev++)
    {
      holy_util_pull_device (*curdev);
      ndev++;
    }

  holy_drives = xmalloc (sizeof (holy_drives[0]) * (ndev + 1));

  for (curdev = holy_devices, curdrive = holy_drives; *curdev; curdev++,
       curdrive++)
    {
      *curdrive = holy_util_get_holy_dev (*curdev);
      if (! *curdrive)
	holy_util_error (_("cannot find a holy drive for %s.  Check your device.map"),
			 *curdev);
    }
  *curdrive = 0;

  holy_dev = holy_device_open (holy_drives[0]);
  if (! holy_dev)
    holy_util_error ("%s", holy_errmsg);

  holy_fs = holy_fs_probe (holy_dev);
  if (! holy_fs)
    holy_util_error ("%s", holy_errmsg);

  holy_install_push_module (holy_fs->name);

  if (holy_dev->disk)
    probe_mods (holy_dev->disk);

  for (curdrive = holy_drives + 1; *curdrive; curdrive++)
    {
      holy_device_t dev = holy_device_open (*curdrive);
      if (!dev)
	continue;
      if (dev->disk)
	probe_mods (dev->disk);
      holy_device_close (dev);
    }

  if (!config.is_cryptodisk_enabled && have_cryptodisk)
    holy_util_error (_("attempt to install to encrypted disk without cryptodisk enabled. "
		       "Set `%s' in file `%s'"), "holy_ENABLE_CRYPTODISK=y",
		     holy_util_get_config_filename ());

  if (disk_module && holy_strcmp (disk_module, "ata") == 0)
    holy_install_push_module ("pata");
  else if (disk_module && holy_strcmp (disk_module, "native") == 0)
    {
      holy_install_push_module ("pata");
      holy_install_push_module ("ahci");
      holy_install_push_module ("ohci");
      holy_install_push_module ("uhci");
      holy_install_push_module ("ehci");
      holy_install_push_module ("usbms");
    }
  else if (disk_module && disk_module[0])
    holy_install_push_module (disk_module);

  relative_holydir = holy_make_system_path_relative_to_its_root (holydir);
  if (relative_holydir[0] == '\0')
    {
      free (relative_holydir);
      relative_holydir = xstrdup ("/");
    }

  char *platname =  holy_install_get_platform_name (platform);
  char *platdir;
  {
    char *t = holy_util_path_concat (2, holydir,
				   platname);
    platdir = holy_canonicalize_file_name (t);
    if (!platdir)
      holy_util_error (_("failed to get canonical path of `%s'"),
		       t);
    free (t);
  }
  load_cfg = holy_util_path_concat (2, platdir,
				  "load.cfg");

  holy_util_unlink (load_cfg);

  if (debug_image && debug_image[0])
    {
      load_cfg_f = holy_util_fopen (load_cfg, "wb");
      have_load_cfg = 1;
      fprintf (load_cfg_f, "set debug='%s'\n",
	      debug_image);
    }
  char *prefix_drive = NULL;
  char *install_drive = NULL;

  if (install_device)
    {
      if (install_device[0] == '('
	  && install_device[holy_strlen (install_device) - 1] == ')')
        {
	  size_t len = holy_strlen (install_device) - 2;
	  install_drive = xmalloc (len + 1);
	  memcpy (install_drive, install_device + 1, len);
	  install_drive[len] = '\0';
        }
      else
	{
	  holy_util_pull_device (install_device);
	  install_drive = holy_util_get_holy_dev (install_device);
	  if (!install_drive)
	    holy_util_error (_("cannot find a holy drive for %s.  Check your device.map"),
			     install_device);
	}
    }

  if (!have_abstractions)
    {
      if ((disk_module && holy_strcmp (disk_module, "biosdisk") != 0)
	  || holy_drives[1]
	  || (!install_drive
	      && platform != holy_INSTALL_PLATFORM_POWERPC_IEEE1275)
	  || (install_drive && !is_same_disk (holy_drives[0], install_drive))
	  || !have_bootdev (platform))
	{
	  char *uuid = NULL;
	  /*  generic method (used on coreboot and ata mod).  */
	  if (!force_file_id && holy_fs->uuid && holy_fs->uuid (holy_dev,
								&uuid))
	    {
	      holy_print_error ();
	      holy_errno = 0;
	      uuid = NULL;
	    }

	  if (!load_cfg_f)
	    load_cfg_f = holy_util_fopen (load_cfg, "wb");
	  have_load_cfg = 1;
	  if (uuid)
	    {
	      fprintf (load_cfg_f, "search.fs_uuid %s root ",
		      uuid);
	      holy_install_push_module ("search_fs_uuid");
	    }
	  else
	    {
	      char *rndstr = get_rndstr ();
	      char *fl = holy_util_path_concat (3, holydir,
						     "uuid", rndstr);
	      char *fldir = holy_util_path_concat (2, holydir,
							"uuid");
	      char *relfl;
	      FILE *flf;
	      holy_install_mkdir_p (fldir);
	      flf = holy_util_fopen (fl, "w");
	      if (!flf)
		holy_util_error (_("Can't create file: %s"), strerror (errno));
	      fclose (flf);
	      relfl = holy_make_system_path_relative_to_its_root (fl);
	      fprintf (load_cfg_f, "search.file %s root ",
		       relfl);
	      holy_install_push_module ("search_fs_file");
	    }
	  for (curdev = holy_devices, curdrive = holy_drives; *curdev; curdev++,
		 curdrive++)
	    {
	      const char *map;
	      char *g = NULL;
	      holy_device_t dev;
	      if (curdrive == holy_drives)
		dev = holy_dev;
	      else
		dev = holy_device_open (*curdrive);
	      if (!dev)
		continue;

	      if (dev->disk->dev->id != holy_DISK_DEVICE_HOSTDISK_ID)
		{
		  holy_util_fprint_full_disk_name (load_cfg_f,
						   dev->disk->name,
						   dev);
		  fprintf (load_cfg_f, " ");
		  if (dev != holy_dev)
		    holy_device_close (dev);
		  continue;
		}

	      map = holy_util_biosdisk_get_compatibility_hint (dev->disk);

	      if (map)
		{
		  holy_util_fprint_full_disk_name (load_cfg_f, map, dev);
		  fprintf (load_cfg_f, " ");
		}


	      if (disk_module && disk_module[0]
		  && holy_strcmp (disk_module, "biosdisk") != 0)
		  g = holy_util_guess_baremetal_drive (*curdev);
	      else
		switch (platform)
		  {
		  case holy_INSTALL_PLATFORM_I386_PC:
		    g = holy_util_guess_bios_drive (*curdev);
		    break;
		  case holy_INSTALL_PLATFORM_I386_EFI:
		  case holy_INSTALL_PLATFORM_X86_64_EFI:
		  case holy_INSTALL_PLATFORM_ARM_EFI:
		  case holy_INSTALL_PLATFORM_ARM64_EFI:
		  case holy_INSTALL_PLATFORM_IA64_EFI:
		    g = holy_util_guess_efi_drive (*curdev);
		    break;
		  case holy_INSTALL_PLATFORM_SPARC64_IEEE1275:
		  case holy_INSTALL_PLATFORM_POWERPC_IEEE1275:
		  case holy_INSTALL_PLATFORM_I386_IEEE1275:
		    {
		      const char * ofpath = holy_util_devname_to_ofpath (*curdev);
		      g = xasprintf ("ieee1275/%s", ofpath);
		      break;
		    }
		  case holy_INSTALL_PLATFORM_MIPSEL_LOONGSON:
		  case holy_INSTALL_PLATFORM_I386_QEMU:
		  case holy_INSTALL_PLATFORM_I386_COREBOOT:
		  case holy_INSTALL_PLATFORM_I386_MULTIBOOT:
		  case holy_INSTALL_PLATFORM_MIPSEL_QEMU_MIPS:
		  case holy_INSTALL_PLATFORM_MIPS_QEMU_MIPS:
		    g = holy_util_guess_baremetal_drive (*curdev);
		    break;
		  case holy_INSTALL_PLATFORM_MIPS_ARC:
		  case holy_INSTALL_PLATFORM_MIPSEL_ARC:
		  case holy_INSTALL_PLATFORM_ARM_UBOOT:
		  case holy_INSTALL_PLATFORM_I386_XEN:
		  case holy_INSTALL_PLATFORM_X86_64_XEN:
		    holy_util_warn ("%s", _("no hints available for your platform. Expect reduced performance"));
		    break;
		    /* pacify warning.  */
		  case holy_INSTALL_PLATFORM_MAX:
		    break;
		  }
	      if (g)
		{
		  holy_util_fprint_full_disk_name (load_cfg_f, g, dev);
		  fprintf (load_cfg_f, " ");
		}
	      if (dev != holy_dev)
		holy_device_close (dev);
	    }
	  fprintf (load_cfg_f, "\n");
	  char *escaped_relpath = escape (relative_holydir);
	  fprintf (load_cfg_f, "set prefix=($root)'%s'\n",
		   escaped_relpath);
	}
      else
	{
	  /* We need to hardcode the partition number in the core image's prefix.  */
	  char *p;
	  for (p = holy_drives[0]; *p; )
	    {
	      if (*p == '\\' && p[1])
		{
		  p += 2;
		  continue;
		}
	      if (*p == ',' || *p == '\0')
		break;
	      p++;
	    }
	  prefix_drive = xasprintf ("(%s)", p);
	}
    }
  else
    {
      if (config.is_cryptodisk_enabled)
	{
	  if (holy_dev->disk)
	    probe_cryptodisk_uuid (holy_dev->disk);

	  for (curdrive = holy_drives + 1; *curdrive; curdrive++)
	    {
	      holy_device_t dev = holy_device_open (*curdrive);
	      if (!dev)
		continue;
	      if (dev->disk)
		probe_cryptodisk_uuid (dev->disk);
	      holy_device_close (dev);
	    }
	}
      prefix_drive = xasprintf ("(%s)", holy_drives[0]);
    }

  char mkimage_target[200];
  const char *core_name = NULL;

  switch (platform)
    {
    case holy_INSTALL_PLATFORM_I386_EFI:
    case holy_INSTALL_PLATFORM_X86_64_EFI:
    case holy_INSTALL_PLATFORM_ARM_EFI:
    case holy_INSTALL_PLATFORM_ARM64_EFI:
    case holy_INSTALL_PLATFORM_IA64_EFI:
      core_name = "core.efi";
      snprintf (mkimage_target, sizeof (mkimage_target),
		"%s-%s",
		holy_install_get_platform_cpu (platform),
		holy_install_get_platform_platform (platform));
      break;
    case holy_INSTALL_PLATFORM_MIPSEL_LOONGSON:
    case holy_INSTALL_PLATFORM_MIPSEL_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_MIPS_QEMU_MIPS:
      core_name = "core.elf";
      snprintf (mkimage_target, sizeof (mkimage_target),
		"%s-%s-elf",
		holy_install_get_platform_cpu (platform),
		holy_install_get_platform_platform (platform));
      break;

    case holy_INSTALL_PLATFORM_I386_COREBOOT:
    case holy_INSTALL_PLATFORM_I386_MULTIBOOT:
    case holy_INSTALL_PLATFORM_I386_IEEE1275:
    case holy_INSTALL_PLATFORM_POWERPC_IEEE1275:
    case holy_INSTALL_PLATFORM_I386_XEN:
    case holy_INSTALL_PLATFORM_X86_64_XEN:
      core_name = "core.elf";
      snprintf (mkimage_target, sizeof (mkimage_target),
		"%s-%s",
		holy_install_get_platform_cpu (platform),
		holy_install_get_platform_platform (platform));
      break;


    case holy_INSTALL_PLATFORM_I386_PC:
    case holy_INSTALL_PLATFORM_MIPSEL_ARC:
    case holy_INSTALL_PLATFORM_MIPS_ARC:
    case holy_INSTALL_PLATFORM_ARM_UBOOT:
    case holy_INSTALL_PLATFORM_I386_QEMU:
      snprintf (mkimage_target, sizeof (mkimage_target),
		"%s-%s",
		holy_install_get_platform_cpu (platform),
		holy_install_get_platform_platform (platform));
      core_name = "core.img";
      break;
    case holy_INSTALL_PLATFORM_SPARC64_IEEE1275:
      strcpy (mkimage_target, "sparc64-ieee1275-raw");
      core_name = "core.img";
      break;
      /* pacify warning.  */
    case holy_INSTALL_PLATFORM_MAX:
      break;
    }

  if (!core_name)
    holy_util_error ("%s", _("You've found a bug"));

  if (load_cfg_f)
    fclose (load_cfg_f);

  char *imgfile = holy_util_path_concat (2, platdir,
				       core_name);
  char *prefix = xasprintf ("%s%s", prefix_drive ? : "",
			    relative_holydir);
  holy_install_make_image_wrap (/* source dir  */ holy_install_source_directory,
				/*prefix */ prefix,
				/* output */ imgfile,
				/* memdisk */ NULL,
				have_load_cfg ? load_cfg : NULL,
				/* image target */ mkimage_target, 0);
  /* Backward-compatibility kludges.  */
  switch (platform)
    {
    case holy_INSTALL_PLATFORM_MIPSEL_LOONGSON:
      {
	char *dst = holy_util_path_concat (2, bootdir, "holy.elf");
	holy_install_copy_file (imgfile, dst, 1);
	free (dst);
      }
      break;

    case holy_INSTALL_PLATFORM_I386_IEEE1275:
    case holy_INSTALL_PLATFORM_POWERPC_IEEE1275:
      {
	char *dst = holy_util_path_concat (2, holydir, "holy");
	holy_install_copy_file (imgfile, dst, 1);
	free (dst);
      }
      break;

    case holy_INSTALL_PLATFORM_I386_EFI:
    case holy_INSTALL_PLATFORM_X86_64_EFI:
      {
	char *dst = holy_util_path_concat (2, platdir, "holy.efi");
	holy_install_make_image_wrap (/* source dir  */ holy_install_source_directory,
				      /* prefix */ "",
				       /* output */ dst,
				       /* memdisk */ NULL,
				      have_load_cfg ? load_cfg : NULL,
				       /* image target */ mkimage_target, 0);
      }
      break;
    case holy_INSTALL_PLATFORM_ARM_EFI:
    case holy_INSTALL_PLATFORM_ARM64_EFI:
    case holy_INSTALL_PLATFORM_IA64_EFI:
    case holy_INSTALL_PLATFORM_MIPSEL_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_MIPS_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_I386_COREBOOT:
    case holy_INSTALL_PLATFORM_I386_MULTIBOOT:
    case holy_INSTALL_PLATFORM_I386_PC:
    case holy_INSTALL_PLATFORM_MIPSEL_ARC:
    case holy_INSTALL_PLATFORM_MIPS_ARC:
    case holy_INSTALL_PLATFORM_ARM_UBOOT:
    case holy_INSTALL_PLATFORM_I386_QEMU:
    case holy_INSTALL_PLATFORM_SPARC64_IEEE1275:
    case holy_INSTALL_PLATFORM_I386_XEN:
    case holy_INSTALL_PLATFORM_X86_64_XEN:
      break;
      /* pacify warning.  */
    case holy_INSTALL_PLATFORM_MAX:
      break;
    }

  /* Perform the platform-dependent install */

  switch (platform)
    {
    case holy_INSTALL_PLATFORM_I386_PC:
      {
	char *boot_img_src = holy_util_path_concat (2,
						  holy_install_source_directory,
						  "boot.img");
	char *boot_img = holy_util_path_concat (2, platdir,
					      "boot.img");
	holy_install_copy_file (boot_img_src, boot_img, 1);

	holy_util_info ("%sholy-bios-setup %s %s %s %s %s --directory='%s' --device-map='%s' '%s'",
			/* TRANSLATORS: This is a prefix in the log to indicate that usually
			   a command would be executed but due to an option was skipped.  */
			install_bootsector ? "" : _("NOT RUNNING: "),
			allow_floppy ? "--allow-floppy " : "",
			verbosity ? "--verbose " : "",
			force ? "--force " : "",
			!fs_probe ? "--skip-fs-probe" : "",
			!add_rs_codes ? "--no-rs-codes" : "",
			platdir,
			device_map,
			install_device);
			
	/*  Now perform the installation.  */
	if (install_bootsector)
	  holy_util_bios_setup (platdir, "boot.img", "core.img",
				install_drive, force,
				fs_probe, allow_floppy, add_rs_codes);
	break;
      }
    case holy_INSTALL_PLATFORM_SPARC64_IEEE1275:
      {
	char *boot_img_src = holy_util_path_concat (2,
						  holy_install_source_directory,
						  "boot.img");
	char *boot_img = holy_util_path_concat (2, platdir,
					      "boot.img");
	holy_install_copy_file (boot_img_src, boot_img, 1);

	holy_util_info ("%sholy-sparc64-setup %s %s %s %s --directory='%s' --device-map='%s' '%s'",
			install_bootsector ? "" : "NOT RUNNING: ",
			allow_floppy ? "--allow-floppy " : "",
			verbosity ? "--verbose " : "",
			force ? "--force " : "",
			!fs_probe ? "--skip-fs-probe" : "",
			platdir,
			device_map,
			install_drive);
			
	/*  Now perform the installation.  */
	if (install_bootsector)
	  holy_util_sparc_setup (platdir, "boot.img", "core.img",
				 install_drive, force,
				 fs_probe, allow_floppy,
				 0 /* unused */ );
	break;
      }

    case holy_INSTALL_PLATFORM_POWERPC_IEEE1275:
      if (macppcdir)
	{
	  char *core_services = holy_util_path_concat (4, macppcdir,
						       "System", "Library",
						       "CoreServices");
	  char *mach_kernel = holy_util_path_concat (2, macppcdir,
						     "mach_kernel");
	  char *holy_elf, *bootx;
	  FILE *f;
	  holy_device_t ins_dev;
	  char *holy_chrp = holy_util_path_concat (2,
						   holy_install_source_directory,
						   "holy.chrp");

	  holy_install_mkdir_p (core_services);

	  bootx = holy_util_path_concat (2, core_services, "BootX");
	  holy_install_copy_file (holy_chrp, bootx, 1);

	  holy_elf = holy_util_path_concat (2, core_services, "holy.elf");
	  holy_install_copy_file (imgfile, holy_elf, 1);

	  f = holy_util_fopen (mach_kernel, "a+");
	  if (!f)
	    holy_util_error (_("Can't create file: %s"), strerror (errno));
	  fclose (f);

	  fill_core_services (core_services);

	  ins_dev = holy_device_open (install_drive);

	  bless (ins_dev, core_services, 0);

	  if (update_nvram)
	    {
	      const char *dev;
	      int partno;

	      partno = ins_dev->disk->partition
		? ins_dev->disk->partition->number + 1 : 0;
	      dev = holy_util_get_os_disk (install_device);
	      holy_install_register_ieee1275 (0, dev, partno,
					      "\\\\BootX");
	    }
	  holy_device_close (ins_dev);
  	  free (holy_elf);
	  free (bootx);
	  free (mach_kernel);
	  free (holy_chrp);
	  break;
	}
      /* If a install device is defined, copy the core.elf to PReP partition.  */
      if (is_prep && install_device && install_device[0])
	{
	  holy_device_t ins_dev;
	  ins_dev = holy_device_open (install_drive);
	  if (!ins_dev || !is_prep_partition (ins_dev))
	    {
	      holy_util_error ("%s", _("the chosen partition is not a PReP partition"));
	    }
	  if (is_prep_empty (ins_dev))
	    {
	      if (write_to_disk (ins_dev, imgfile))
		holy_util_error ("%s", _("failed to copy holy to the PReP partition"));
	    }
	  else
	    {
	      char *s = xasprintf ("dd if=/dev/zero of=%s", install_device);
	      holy_util_error (_("the PReP partition is not empty. If you are sure you want to use it, run dd to clear it: `%s'"),
			       s);
	    }
	  holy_device_close (ins_dev);
	  if (update_nvram)
	    holy_install_register_ieee1275 (1, holy_util_get_os_disk (install_device),
					    0, NULL);
	  break;
      }
      /* fallthrough.  */
    case holy_INSTALL_PLATFORM_I386_IEEE1275:
      if (update_nvram)
	{
	  const char *dev;
	  char *relpath;
	  int partno;
	  relpath = holy_make_system_path_relative_to_its_root (imgfile);
	  partno = holy_dev->disk->partition
	    ? holy_dev->disk->partition->number + 1 : 0;
	  dev = holy_util_get_os_disk (holy_devices[0]);
	  holy_install_register_ieee1275 (0, dev,
					  partno, relpath);
	}
      break;
    case holy_INSTALL_PLATFORM_MIPS_ARC:
      holy_install_sgi_setup (install_device, imgfile, "holy");
      break;

    case holy_INSTALL_PLATFORM_I386_EFI:
      if (!efidir_is_mac)
	{
	  char *dst = holy_util_path_concat (2, efidir, "holy.efi");
	  /* For old macs. Suggested by Peter Jones.  */
	  holy_install_copy_file (imgfile, dst, 1);
	  free (dst);
	}
      /* Fallthrough.  */
    case holy_INSTALL_PLATFORM_X86_64_EFI:
      if (efidir_is_mac)
	{
	  char *boot_efi;
	  char *core_services = holy_util_path_concat (4, efidir,
						       "System", "Library",
						       "CoreServices");
	  char *mach_kernel = holy_util_path_concat (2, efidir,
						     "mach_kernel");
	  FILE *f;
	  holy_device_t ins_dev;

	  holy_install_mkdir_p (core_services);

	  boot_efi = holy_util_path_concat (2, core_services, "boot.efi");
	  holy_install_copy_file (imgfile, boot_efi, 1);

	  f = holy_util_fopen (mach_kernel, "r+");
	  if (!f)
	    holy_util_error (_("Can't create file: %s"), strerror (errno));
	  fclose (f);

	  fill_core_services(core_services);

	  ins_dev = holy_device_open (install_drive);

	  bless (ins_dev, boot_efi, 1);
	  if (!removable && update_nvram)
	    {
	      /* Try to make this image bootable using the EFI Boot Manager, if available.  */
	      holy_install_register_efi (efidir_holy_dev,
					 "\\System\\Library\\CoreServices",
					 efi_distributor);
	    }

	  holy_device_close (ins_dev);
  	  free (boot_efi);
	  free (mach_kernel);
	  break;
	}
      /* FALLTHROUGH */
    case holy_INSTALL_PLATFORM_ARM_EFI:
    case holy_INSTALL_PLATFORM_ARM64_EFI:
    case holy_INSTALL_PLATFORM_IA64_EFI:
      {
	char *dst = holy_util_path_concat (2, efidir, efi_file);
	holy_install_copy_file (imgfile, dst, 1);
	free (dst);
      }
      if (!removable && update_nvram)
	{
	  char * efifile_path;
	  char * part;

	  /* Try to make this image bootable using the EFI Boot Manager, if available.  */
	  if (!efi_distributor || efi_distributor[0] == '\0')
	    holy_util_error ("%s", _("EFI bootloader id isn't specified."));
	  efifile_path = xasprintf ("\\EFI\\%s\\%s",
				    efi_distributor,
				    efi_file);
	  part = (efidir_holy_dev->disk->partition
		  ? holy_partition_get_name (efidir_holy_dev->disk->partition)
		  : 0);
	  holy_util_info ("Registering with EFI: distributor = `%s',"
			  " path = `%s', ESP at %s%s%s",
			  efi_distributor, efifile_path,
			  efidir_holy_dev->disk->name,
			  (part ? ",": ""), (part ? : ""));
	  holy_free (part);
	  holy_install_register_efi (efidir_holy_dev,
				     efifile_path, efi_distributor);
	}
      break;

    case holy_INSTALL_PLATFORM_MIPSEL_LOONGSON:
    case holy_INSTALL_PLATFORM_MIPSEL_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_MIPS_QEMU_MIPS:
    case holy_INSTALL_PLATFORM_I386_COREBOOT:
    case holy_INSTALL_PLATFORM_I386_MULTIBOOT:
    case holy_INSTALL_PLATFORM_MIPSEL_ARC:
    case holy_INSTALL_PLATFORM_ARM_UBOOT:
    case holy_INSTALL_PLATFORM_I386_QEMU:
    case holy_INSTALL_PLATFORM_I386_XEN:
    case holy_INSTALL_PLATFORM_X86_64_XEN:
      holy_util_warn ("%s",
		      _("WARNING: no platform-specific install was performed"));
      break;
      /* pacify warning.  */
    case holy_INSTALL_PLATFORM_MAX:
      break;
    }

  fprintf (stderr, "%s\n", _("Installation finished. No error reported."));

  /* Free resources.  */
  holy_gcry_fini_all ();
  holy_fini_all ();

  return 0;
}
