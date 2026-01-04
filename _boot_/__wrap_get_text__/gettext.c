/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/normal.h>
#include <holy/file.h>
#include <holy/kernel.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");


static const char *(*holy_gettext_original) (const char *s);

struct holy_gettext_msg
{
  char *name;
  char *translated;
};

struct header
{
  holy_uint32_t magic;
  holy_uint32_t version;
  holy_uint32_t number_of_strings;
  holy_uint32_t offset_original;
  holy_uint32_t offset_translation;
};

struct string_descriptor
{
  holy_uint32_t length;
  holy_uint32_t offset;
};

struct holy_gettext_context
{
  holy_file_t fd_mo;
  holy_off_t holy_gettext_offset_original;
  holy_off_t holy_gettext_offset_translation;
  holy_size_t holy_gettext_max;
  int holy_gettext_max_log;
  struct holy_gettext_msg *holy_gettext_msg_list;
};

static struct holy_gettext_context main_context, secondary_context;

#define MO_MAGIC_NUMBER 		0x950412de

static holy_err_t
holy_gettext_pread (holy_file_t file, void *buf, holy_size_t len,
		    holy_off_t offset)
{
  if (len == 0)
    return holy_ERR_NONE;
  if (holy_file_seek (file, offset) == (holy_off_t) - 1)
    return holy_errno;
  if (holy_file_read (file, buf, len) != (holy_ssize_t) len)
    {
      if (!holy_errno)
	holy_error (holy_ERR_READ_ERROR, N_("premature end of file"));
      return holy_errno;
    }
  return holy_ERR_NONE;
}

static char *
holy_gettext_getstr_from_position (struct holy_gettext_context *ctx,
				   holy_off_t off,
				   holy_size_t position)
{
  holy_off_t internal_position;
  holy_size_t length;
  holy_off_t offset;
  char *translation;
  struct string_descriptor desc;
  holy_err_t err;

  internal_position = (off + position * sizeof (desc));

  err = holy_gettext_pread (ctx->fd_mo, (char *) &desc,
			    sizeof (desc), internal_position);
  if (err)
    return NULL;
  length = holy_cpu_to_le32 (desc.length);
  offset = holy_cpu_to_le32 (desc.offset);

  translation = holy_malloc (length + 1);
  if (!translation)
    return NULL;

  err = holy_gettext_pread (ctx->fd_mo, translation, length, offset);
  if (err)
    {
      holy_free (translation);
      return NULL;
    }
  translation[length] = '\0';

  return translation;
}

static const char *
holy_gettext_gettranslation_from_position (struct holy_gettext_context *ctx,
					   holy_size_t position)
{
  if (!ctx->holy_gettext_msg_list[position].translated)
    ctx->holy_gettext_msg_list[position].translated
      = holy_gettext_getstr_from_position (ctx,
					   ctx->holy_gettext_offset_translation,
					   position);
  return ctx->holy_gettext_msg_list[position].translated;
}

static const char *
holy_gettext_getstring_from_position (struct holy_gettext_context *ctx,
				      holy_size_t position)
{
  if (!ctx->holy_gettext_msg_list[position].name)
    ctx->holy_gettext_msg_list[position].name
      = holy_gettext_getstr_from_position (ctx,
					   ctx->holy_gettext_offset_original,
					   position);
  return ctx->holy_gettext_msg_list[position].name;
}

static const char *
holy_gettext_translate_real (struct holy_gettext_context *ctx,
			     const char *orig)
{
  holy_size_t current = 0;
  int i;
  const char *current_string;
  static int depth = 0;

  if (!ctx->holy_gettext_msg_list || !ctx->fd_mo)
    return NULL;

  /* Shouldn't happen. Just a precaution if our own code
     calls gettext somehow.  */
  if (depth > 2)
    return NULL;
  depth++;

  /* Make sure we can use holy_gettext_translate for error messages.  Push
     active error message to error stack and reset error message.  */
  holy_error_push ();

  for (i = ctx->holy_gettext_max_log; i >= 0; i--)
    {
      holy_size_t test;
      int cmp;

      test = current | (1 << i);
      if (test >= ctx->holy_gettext_max)
	continue;

      current_string = holy_gettext_getstring_from_position (ctx, test);

      if (!current_string)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_error_pop ();
	  depth--;
	  return NULL;
	}

      /* Search by bisection.  */
      cmp = holy_strcmp (current_string, orig);
      if (cmp <= 0)
	current = test;
      if (cmp == 0)
	{
	  const char *ret = 0;
	  ret = holy_gettext_gettranslation_from_position (ctx, current);
	  if (!ret)
	    {
	      holy_errno = holy_ERR_NONE;
	      holy_error_pop ();
	      depth--;
	      return NULL;
	    }
	  holy_error_pop ();
	  depth--;
	  return ret;
	}
    }

  if (current == 0 && ctx->holy_gettext_max != 0)
    {
      current_string = holy_gettext_getstring_from_position (ctx, 0);

      if (!current_string)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_error_pop ();
	  depth--;
	  return NULL;
	}

      if (holy_strcmp (current_string, orig) == 0)
	{
	  const char *ret = 0;
	  ret = holy_gettext_gettranslation_from_position (ctx, current);
	  if (!ret)
	    {
	      holy_errno = holy_ERR_NONE;
	      holy_error_pop ();
	      depth--;
	      return NULL;
	    }
	  holy_error_pop ();
	  depth--;
	  return ret;
	}
    }

  holy_error_pop ();
  depth--;
  return NULL;
}

static const char *
holy_gettext_translate (const char *orig)
{
  const char *ret;
  if (orig[0] == 0)
    return orig;

  ret = holy_gettext_translate_real (&main_context, orig);
  if (ret)
    return ret;
  ret = holy_gettext_translate_real (&secondary_context, orig);
  if (ret)
    return ret;
  return orig;
}

static void
holy_gettext_delete_list (struct holy_gettext_context *ctx)
{
  struct holy_gettext_msg *l = ctx->holy_gettext_msg_list;
  holy_size_t i;

  if (!l)
    return;
  ctx->holy_gettext_msg_list = 0;
  for (i = 0; i < ctx->holy_gettext_max; i++)
    holy_free (l[i].name);
  /* Don't delete the translated message because could be in use.  */
  holy_free (l);
  if (ctx->fd_mo)
    holy_file_close (ctx->fd_mo);
  ctx->fd_mo = 0;
  holy_memset (ctx, 0, sizeof (*ctx));
}

/* This is similar to holy_file_open. */
static holy_err_t
holy_mofile_open (struct holy_gettext_context *ctx,
		  const char *filename)
{
  struct header head;
  holy_err_t err;
  holy_file_t fd;

  /* Using fd_mo and not another variable because
     it's needed for holy_gettext_get_info.  */

  fd = holy_file_open (filename);

  if (!fd)
    return holy_errno;

  err = holy_gettext_pread (fd, &head, sizeof (head), 0);
  if (err)
    {
      holy_file_close (fd);
      return err;
    }

  if (head.magic != holy_cpu_to_le32_compile_time (MO_MAGIC_NUMBER))
    {
      holy_file_close (fd);
      return holy_error (holy_ERR_BAD_FILE_TYPE,
			 "mo: invalid mo magic in file: %s", filename);
    }

  if (head.version != 0)
    {
      holy_file_close (fd);
      return holy_error (holy_ERR_BAD_FILE_TYPE,
			 "mo: invalid mo version in file: %s", filename);
    }

  ctx->holy_gettext_offset_original = holy_le_to_cpu32 (head.offset_original);
  ctx->holy_gettext_offset_translation = holy_le_to_cpu32 (head.offset_translation);
  ctx->holy_gettext_max = holy_le_to_cpu32 (head.number_of_strings);
  for (ctx->holy_gettext_max_log = 0; ctx->holy_gettext_max >> ctx->holy_gettext_max_log;
       ctx->holy_gettext_max_log++);

  ctx->holy_gettext_msg_list = holy_zalloc (ctx->holy_gettext_max
					    * sizeof (ctx->holy_gettext_msg_list[0]));
  if (!ctx->holy_gettext_msg_list)
    {
      holy_file_close (fd);
      return holy_errno;
    }
  ctx->fd_mo = fd;
  if (holy_gettext != holy_gettext_translate)
    {
      holy_gettext_original = holy_gettext;
      holy_gettext = holy_gettext_translate;
    }
  return 0;
}

/* Returning holy_file_t would be more natural, but holy_mofile_open assigns
   to fd_mo anyway ...  */
static holy_err_t
holy_mofile_open_lang (struct holy_gettext_context *ctx,
		       const char *part1, const char *part2, const char *locale)
{
  char *mo_file;
  holy_err_t err;

  /* mo_file e.g.: /boot/holy/locale/ca.mo   */

  mo_file = holy_xasprintf ("%s%s/%s.mo", part1, part2, locale);
  if (!mo_file)
    return holy_errno;

  err = holy_mofile_open (ctx, mo_file);
  holy_free (mo_file);

  /* Will try adding .gz as well.  */
  if (err)
    {
      holy_errno = holy_ERR_NONE;
      mo_file = holy_xasprintf ("%s%s/%s.mo.gz", part1, part2, locale);
      if (!mo_file)
	return holy_errno;
      err = holy_mofile_open (ctx, mo_file);
      holy_free (mo_file);
    }

  /* Will try adding .gmo as well.  */
  if (err)
    {
      holy_errno = holy_ERR_NONE;
      mo_file = holy_xasprintf ("%s%s/%s.gmo", part1, part2, locale);
      if (!mo_file)
	return holy_errno;
      err = holy_mofile_open (ctx, mo_file);
      holy_free (mo_file);
    }

  return err;
}

static holy_err_t
holy_gettext_init_ext (struct holy_gettext_context *ctx,
		       const char *locale,
		       const char *locale_dir, const char *prefix)
{
  const char *part1, *part2;
  holy_err_t err;

  holy_gettext_delete_list (ctx);

  if (!locale || locale[0] == 0)
    return 0;

  part1 = locale_dir;
  part2 = "";
  if (!part1 || part1[0] == 0)
    {
      part1 = prefix;
      part2 = "/locale";
    }

  if (!part1 || part1[0] == 0)
    return 0;

  err = holy_mofile_open_lang (ctx, part1, part2, locale);

  /* ll_CC didn't work, so try ll.  */
  if (err)
    {
      char *lang = holy_strdup (locale);
      char *underscore = lang ? holy_strchr (lang, '_') : 0;

      if (underscore)
	{
	  *underscore = '\0';
	  holy_errno = holy_ERR_NONE;
	  err = holy_mofile_open_lang (ctx, part1, part2, lang);
	}

      holy_free (lang);
    }

  if (locale[0] == 'e' && locale[1] == 'n'
      && (locale[2] == '\0' || locale[2] == '_'))
    holy_errno = err = holy_ERR_NONE;
  return err;
}

static char *
holy_gettext_env_write_lang (struct holy_env_var *var
			     __attribute__ ((unused)), const char *val)
{
  holy_err_t err;
  err = holy_gettext_init_ext (&main_context, val, holy_env_get ("locale_dir"),
			       holy_env_get ("prefix"));
  if (err)
    holy_print_error ();

  err = holy_gettext_init_ext (&secondary_context, val,
			       holy_env_get ("secondary_locale_dir"), 0);
  if (err)
    holy_print_error ();

  return holy_strdup (val);
}

void
holy_gettext_reread_prefix (const char *val)
{
  holy_err_t err;
  err = holy_gettext_init_ext (&main_context, holy_env_get ("lang"),
			       holy_env_get ("locale_dir"),
			       val);
  if (err)
    holy_print_error ();
}

static char *
read_main (struct holy_env_var *var
	   __attribute__ ((unused)), const char *val)
{
  holy_err_t err;
  err = holy_gettext_init_ext (&main_context, holy_env_get ("lang"), val,
			       holy_env_get ("prefix"));
  if (err)
    holy_print_error ();
  return holy_strdup (val);
}

static char *
read_secondary (struct holy_env_var *var
		__attribute__ ((unused)), const char *val)
{
  holy_err_t err;
  err = holy_gettext_init_ext (&secondary_context, holy_env_get ("lang"), val,
			       0);
  if (err)
    holy_print_error ();

  return holy_strdup (val);
}

static holy_err_t
holy_cmd_translate (holy_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  const char *translation;
  translation = holy_gettext_translate (args[0]);
  holy_printf ("%s\n", translation);
  return 0;
}

holy_MOD_INIT (gettext)
{
  const char *lang;
  holy_err_t err;

  lang = holy_env_get ("lang");

  err = holy_gettext_init_ext (&main_context, lang, holy_env_get ("locale_dir"),
			       holy_env_get ("prefix"));
  if (err)
    holy_print_error ();
  err = holy_gettext_init_ext (&secondary_context, lang,
			       holy_env_get ("secondary_locale_dir"), 0);
  if (err)
    holy_print_error ();

  holy_register_variable_hook ("locale_dir", NULL, read_main);
  holy_register_variable_hook ("secondary_locale_dir", NULL, read_secondary);

  holy_register_command_p1 ("gettext", holy_cmd_translate,
			    N_("STRING"),
			    /* TRANSLATORS: It refers to passing the string through gettext.
			       So it's "translate" in the same meaning as in what you're
			       doing now.
			     */
			    N_("Translates the string with the current settings."));

  /* Reload .mo file information if lang changes.  */
  holy_register_variable_hook ("lang", NULL, holy_gettext_env_write_lang);

  /* Preserve hooks after context changes.  */
  holy_env_export ("lang");
  holy_env_export ("locale_dir");
  holy_env_export ("secondary_locale_dir");
}

holy_MOD_FINI (gettext)
{
  holy_gettext_delete_list (&main_context);
  holy_gettext_delete_list (&secondary_context);

  holy_gettext = holy_gettext_original;
}
