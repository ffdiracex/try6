/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/install.h>
#include <holy/util/misc.h>
#include <holy/emu/config.h>

#include <string.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

static char *output_image;
static char **files;
static int nfiles;
const struct holy_install_image_target_desc *format;
static FILE *memdisk;

enum
  {
    OPTION_OUTPUT = 'o',
    OPTION_FORMAT = 'O'
  };

static struct argp_option options[] = {
  holy_INSTALL_OPTIONS,
  {"output", 'o', N_("FILE"),
   0, N_("save output in FILE [required]"), 2},
  {"format", 'O', N_("FILE"), 0, 0, 2},
  {"compression", 'C', "xz|none|auto", OPTION_HIDDEN, 0, 2},
  {0, 0, 0, 0, 0, 0}
};

static char *
help_filter (int key, const char *text, void *input __attribute__ ((unused)))
{
  switch (key)
    {
    case 'O':
      {
	char *formats = holy_install_get_image_targets_string (), *ret;
	ret = xasprintf ("%s\n%s %s", _("generate an image in FORMAT"),
			 _("available formats:"), formats);
	free (formats);
	return ret;
      }
    default:
      return holy_install_help_filter (key, text, input);
    }
}

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  if (key == 'C')
    key = holy_INSTALL_OPTIONS_INSTALL_CORE_COMPRESS;

  if (holy_install_parse (key, arg))
    return 0;

  switch (key)
    {

    case 'o':
      if (output_image)
	free (output_image);

      output_image = xstrdup (arg);
      break;

    case 'O':
      {
	format = holy_install_get_image_target (arg);
	if (!format)
	  {
	    printf (_("unknown target format %s\n"), arg);
	    argp_usage (state);
	    exit (1);
	  }
	break;
      }

    case ARGP_KEY_ARG:
      files[nfiles++] = xstrdup (arg);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp argp = {
  options, argp_parser, N_("[OPTION] SOURCE..."),
  N_("Generate a standalone image (containing all modules) in the selected format")"\v"N_("Graft point syntax (E.g. /boot/holy/holy.cfg=./holy.cfg) is accepted"),
  NULL, help_filter, NULL
};

/* tar support */
#define MAGIC	"ustar"
struct head
{
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char pad[12];
} holy_PACKED;

static void
write_zeros (unsigned rsz)
{
  char buf[512];

  memset (buf, 0, 512);
  fwrite (buf, 1, rsz, memdisk);
}

static void
write_pad (unsigned sz)
{
  write_zeros ((~sz + 1) & 511);
}

static void
set_tar_value (char *field, holy_uint32_t val,
	       unsigned len)
{
  unsigned i;
  for (i = 0; i < len - 1; i++)
    field[len - 2 - i] = '0' + ((val >> (3 * i)) & 7);
}

static void
compute_checksum (struct head *hd)
{
  unsigned int chk = 0;
  unsigned char *ptr;
  memset (hd->chksum, ' ', 8);
  for (ptr = (unsigned char *) hd; ptr < (unsigned char *) (hd + 1); ptr++)
    chk += *ptr;
  set_tar_value (hd->chksum, chk, 8);
}

static void
add_tar_file (const char *from,
	      const char *to)
{
  char *tcn;
  const char *iptr;
  char *optr;
  struct head hd;
  holy_util_fd_t in;
  ssize_t r;
  holy_uint32_t mtime = 0;
  holy_uint32_t size;

  COMPILE_TIME_ASSERT (sizeof (hd) == 512);

  if (holy_util_is_special_file (from))
    return;

  mtime = holy_util_get_mtime (from);

  optr = tcn = xmalloc (strlen (to) + 1);
  for (iptr = to; *iptr == '/'; iptr++);
  for (; *iptr; iptr++)
    if (!(iptr[0] == '/' && iptr[1] == '/'))
      *optr++ = *iptr;
  *optr = '\0';

  if (holy_util_is_directory (from))
    {
      holy_util_fd_dir_t d;
      holy_util_fd_dirent_t de;

      d = holy_util_fd_opendir (from);

      while ((de = holy_util_fd_readdir (d)))
	{
	  char *fp, *tfp;
	  if (strcmp (de->d_name, ".") == 0)
	    continue;
	  if (strcmp (de->d_name, "..") == 0)
	    continue;
	  fp = holy_util_path_concat (2, from, de->d_name);
	  tfp = xasprintf ("%s/%s", to, de->d_name);
	  add_tar_file (fp, tfp);
	  free (fp);
	}
      holy_util_fd_closedir (d);
      free (tcn);
      return;
    }

  if (optr - tcn > 99)
    {
      memset (&hd, 0, sizeof (hd));
      memcpy (hd.name, tcn, 99);
      memcpy (hd.mode, "0000600", 7);
      memcpy (hd.uid, "0001750", 7);
      memcpy (hd.gid, "0001750", 7);

      set_tar_value (hd.size, optr - tcn, 12);
      set_tar_value (hd.mtime, mtime, 12);
      hd.typeflag = 'L';
      memcpy (hd.magic, MAGIC, sizeof (hd.magic));
      memcpy (hd.uname, "holy", 4);
      memcpy (hd.gname, "holy", 4);

      compute_checksum (&hd);

      fwrite (&hd, 1, sizeof (hd), memdisk);
      fwrite (tcn, 1, optr - tcn, memdisk);

      write_pad (optr - tcn);
    }

  in = holy_util_fd_open (from, holy_UTIL_FD_O_RDONLY);
  if (!holy_UTIL_FD_IS_VALID (in))
    holy_util_error (_("cannot open `%s': %s"), from, holy_util_fd_strerror ());

  if (!holy_install_copy_buffer)
    holy_install_copy_buffer = xmalloc (holy_INSTALL_COPY_BUFFER_SIZE);

  size = holy_util_get_fd_size (in, from, NULL);

  memset (&hd, 0, sizeof (hd));
  memcpy (hd.name, tcn, optr - tcn < 99 ? optr - tcn : 99);
  memcpy (hd.mode, "0000600", 7);
  memcpy (hd.uid, "0001750", 7);
  memcpy (hd.gid, "0001750", 7);

  set_tar_value (hd.size, size, 12);
  set_tar_value (hd.mtime, mtime, 12);
  hd.typeflag = '0';
  memcpy (hd.magic, MAGIC, sizeof (hd.magic));
  memcpy (hd.uname, "holy", 4);
  memcpy (hd.gname, "holy", 4);

  compute_checksum (&hd);

  fwrite (&hd, 1, sizeof (hd), memdisk);
 
  while (1)
    {
      r = holy_util_fd_read (in, holy_install_copy_buffer, holy_INSTALL_COPY_BUFFER_SIZE);
      if (r <= 0)
	break;
      fwrite (holy_install_copy_buffer, 1, r, memdisk);
    }
  holy_util_fd_close (in);

  write_pad (size);
  free (tcn);
}

int
main (int argc, char *argv[])
{
  const char *pkglibdir;
  int i;

  holy_util_host_init (&argc, &argv);
  holy_util_disable_fd_syncs ();

  files = xmalloc ((argc + 1) * sizeof (files[0]));

  argp_parse (&argp, argc, argv, 0, 0, 0);

  pkglibdir = holy_util_get_pkglibdir ();

  if (!output_image)
    holy_util_error ("%s", _("output file must be specified"));

  if (!format)
    holy_util_error ("%s", _("Target format not specified (use the -O option)."));

  if (!holy_install_source_directory)
    holy_install_source_directory = holy_util_path_concat (2, pkglibdir, holy_util_get_target_dirname (format));

  enum holy_install_plat plat = holy_install_get_target (holy_install_source_directory);

  char *memdisk_dir = holy_util_make_temporary_dir ();
  char *boot_holy = holy_util_path_concat (3, memdisk_dir, "boot", "holy");
  holy_install_copy_files (holy_install_source_directory,
			   boot_holy, plat);

  char *memdisk_img = holy_util_make_temporary_file ();

  memdisk = holy_util_fopen (memdisk_img, "wb");

  add_tar_file (memdisk_dir, "");
  for (i = 0; i < nfiles; i++)
    {
      char *eq = holy_strchr (files[i], '=');
      char *from, *to;
      if (!eq)
	{
	  from = files[i];
	  to = files[i];
	}
      else
	{
	  *eq = '\0';
	  to = files[i];
	  from = eq + 1;
	}
      while (*to == '/')
	to++;
      add_tar_file (from, to);
    }
  write_zeros (512);

  fclose (memdisk);

  holy_util_unlink_recursive (memdisk_dir);

  holy_install_push_module ("memdisk");
  holy_install_push_module ("tar");

  holy_install_make_image_wrap (holy_install_source_directory,
				"(memdisk)/boot/holy", output_image,
				memdisk_img, NULL,
				holy_util_get_target_name (format), 0);

  holy_util_unlink (memdisk_img);
  return 0;
}
