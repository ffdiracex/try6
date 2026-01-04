/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/gui_string_util.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>

/* Create a new NUL-terminated string on the heap as a substring of BUF.
   The range of buf included is the half-open interval [START,END).
   The index START is inclusive, END is exclusive.  */
char *
holy_new_substring (const char *buf,
                    holy_size_t start, holy_size_t end)
{
  if (end < start)
    return 0;
  holy_size_t len = end - start;
  char *s = holy_malloc (len + 1);
  if (! s)
    return 0;
  holy_memcpy (s, buf + start, len);
  s[len] = '\0';
  return s;
}

/* Eliminate "." and ".." path elements from PATH.  A new heap-allocated
   string is returned.  */
static char *
canonicalize_path (const char *path)
{
  int i;
  const char *p;
  char *newpath = 0;

  /* Count the path components in path.  */
  int components = 1;
  for (p = path; *p; p++)
    if (*p == '/')
      components++;

  char **path_array = holy_malloc (components * sizeof (*path_array));
  if (! path_array)
    return 0;

  /* Initialize array elements to NULL pointers; in case once of the
     allocations fails, the cleanup code can just call holy_free() for all
     pointers in the array.  */
  for (i = 0; i < components; i++)
    path_array[i] = 0;

  /* Parse the path into path_array.  */
  p = path;
  for (i = 0; i < components && p; i++)
    {
      /* Find the end of the path element.  */
      const char *end = holy_strchr (p, '/');
      if (!end)
        end = p + holy_strlen (p);

      /* Copy the element.  */
      path_array[i] = holy_new_substring (p, 0, end - p);
      if (! path_array[i])
        goto cleanup;

      /* Advance p to point to the start of the next element, or NULL.  */
      if (*end)
        p = end + 1;
      else
        p = 0;
    }

  /* Eliminate '.' and '..' elements from the path array.  */
  int newpath_length = 0;
  for (i = components - 1; i >= 0; --i)
    {
      if (! holy_strcmp (path_array[i], "."))
        {
          holy_free (path_array[i]);
          path_array[i] = 0;
        }
      else if (! holy_strcmp (path_array[i], "..")
               && i > 0)
        {
          /* Delete the '..' and the prior path element.  */
          holy_free (path_array[i]);
          path_array[i] = 0;
          --i;
          holy_free (path_array[i]);
          path_array[i] = 0;
        }
      else
        {
          newpath_length += holy_strlen (path_array[i]) + 1;
        }
    }

  /* Construct a new path string.  */
  newpath = holy_malloc (newpath_length + 1);
  if (! newpath)
    goto cleanup;

  newpath[0] = '\0';
  char *newpath_end = newpath;
  int first = 1;
  for (i = 0; i < components; i++)
    {
      char *element = path_array[i];
      if (element)
        {
          /* For all components but the first, prefix with a slash.  */
          if (! first)
            newpath_end = holy_stpcpy (newpath_end, "/");
          newpath_end = holy_stpcpy (newpath_end, element);
          first = 0;
        }
    }

cleanup:
  for (i = 0; i < components; i++)
    holy_free (path_array[i]);
  holy_free (path_array);

  return newpath;
}

/* Return a new heap-allocated string representing to absolute path
   to the file referred to by PATH.  If PATH is an absolute path, then
   the returned path is a copy of PATH.  If PATH is a relative path, then
   BASE is with PATH used to construct the absolute path.  */
char *
holy_resolve_relative_path (const char *base, const char *path)
{
  char *abspath;
  char *canonpath;
  char *p;
  holy_size_t l;

  /* If PATH is an absolute path, then just use it as is.  */
  if (path[0] == '/' || path[0] == '(')
    return canonicalize_path (path);

  abspath = holy_malloc (holy_strlen (base) + holy_strlen (path) + 3);
  if (! abspath)
    return 0;

  /* Concatenate BASE and PATH.  */
  p = holy_stpcpy (abspath, base);
  l = holy_strlen (abspath);
  if (l == 0 || abspath[l-1] != '/')
    {
      *p = '/';
      p++;
      *p = 0;
    }
  holy_stpcpy (p, path);

  canonpath = canonicalize_path (abspath);
  if (! canonpath)
    return abspath;

  holy_free (abspath);
  return canonpath;
}

/* Get the path of the directory where the file at FILE_PATH is located.
   FILE_PATH should refer to a file, not a directory.  The returned path
   includes a trailing slash.
   This does not handle holy "(hd0,0)" paths properly yet since it only
   looks at slashes.  */
char *
holy_get_dirname (const char *file_path)
{
  int i;
  int last_slash;

  last_slash = -1;
  for (i = holy_strlen (file_path) - 1; i >= 0; --i)
    {
      if (file_path[i] == '/')
        {
          last_slash = i;
          break;
        }
    }
  if (last_slash == -1)
    return holy_strdup ("/");

  return holy_new_substring (file_path, 0, last_slash + 1);
}
