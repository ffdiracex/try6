/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/err.h>
#include <holy/linux.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/tpm.h>

struct newc_head
{
  char magic[6];
  char ino[8];
  char mode[8];
  char uid[8];
  char gid[8];
  char nlink[8];
  char mtime[8];
  char filesize[8];
  char devmajor[8];
  char devminor[8];
  char rdevmajor[8];
  char rdevminor[8];
  char namesize[8];
  char check[8];
} holy_PACKED;

struct holy_linux_initrd_component
{
  holy_file_t file;
  char *newc_name;
  holy_off_t size;
};

struct dir
{
  char *name;
  struct dir *next;
  struct dir *child;
};

static char
hex (holy_uint8_t val)
{
  if (val < 10)
    return '0' + val;
  return 'a' + val - 10;
}

static void
set_field (char *var, holy_uint32_t val)
{
  int i;
  char *ptr = var;
  for (i = 28; i >= 0; i -= 4)
    *ptr++ = hex((val >> i) & 0xf);
}

static holy_uint8_t *
make_header (holy_uint8_t *ptr,
	     const char *name, holy_size_t len,
	     holy_uint32_t mode,
	     holy_off_t fsize)
{
  struct newc_head *head = (struct newc_head *) ptr;
  holy_uint8_t *optr;
  holy_size_t oh = 0;
  holy_memcpy (head->magic, "070701", 6);
  set_field (head->ino, 0);
  set_field (head->mode, mode);
  set_field (head->uid, 0);
  set_field (head->gid, 0);
  set_field (head->nlink, 1);
  set_field (head->mtime, 0);
  set_field (head->filesize, fsize);
  set_field (head->devmajor, 0);
  set_field (head->devminor, 0);
  set_field (head->rdevmajor, 0);
  set_field (head->rdevminor, 0);
  set_field (head->namesize, len);
  set_field (head->check, 0);
  optr = ptr;
  ptr += sizeof (struct newc_head);
  holy_memcpy (ptr, name, len);
  ptr += len;
  oh = ALIGN_UP_OVERHEAD (ptr - optr, 4);
  holy_memset (ptr, 0, oh);
  ptr += oh;
  return ptr;
}

static void
free_dir (struct dir *root)
{
  if (!root)
    return;
  free_dir (root->next);
  free_dir (root->child);
  holy_free (root->name);
  holy_free (root);
}

static holy_size_t
insert_dir (const char *name, struct dir **root,
	    holy_uint8_t *ptr)
{
  struct dir *cur, **head = root;
  const char *cb, *ce = name;
  holy_size_t size = 0;
  while (1)
    {
      for (cb = ce; *cb == '/'; cb++);
      for (ce = cb; *ce && *ce != '/'; ce++);
      if (!*ce)
	break;

      for (cur = *root; cur; cur = cur->next)
	if (holy_memcmp (cur->name, cb, ce - cb)
	    && cur->name[ce - cb] == 0)
	  break;
      if (!cur)
	{
	  struct dir *n;
	  n = holy_zalloc (sizeof (*n));
	  if (!n)
	    return 0;
	  n->next = *head;
	  n->name = holy_strndup (cb, ce - cb);
	  if (ptr)
	    {
	      holy_dprintf ("linux", "Creating directory %s, %s\n", name, ce);
	      ptr = make_header (ptr, name, ce - name,
				 040777, 0);
	    }
	  size += ALIGN_UP ((ce - (char *) name)
			    + sizeof (struct newc_head), 4);
	  *head = n;
	  cur = n;
	}
      root = &cur->next;
    }
  return size;
}

holy_err_t
holy_initrd_init (int argc, char *argv[],
		  struct holy_linux_initrd_context *initrd_ctx)
{
  int i;
  int newc = 0;
  struct dir *root = 0;

  initrd_ctx->nfiles = 0;
  initrd_ctx->components = 0;

  initrd_ctx->components = holy_zalloc (argc
					* sizeof (initrd_ctx->components[0]));
  if (!initrd_ctx->components)
    return holy_errno;

  initrd_ctx->size = 0;

  for (i = 0; i < argc; i++)
    {
      const char *fname = argv[i];

      initrd_ctx->size = ALIGN_UP (initrd_ctx->size, 4);

      if (holy_memcmp (argv[i], "newc:", 5) == 0)
	{
	  const char *ptr, *eptr;
	  ptr = argv[i] + 5;
	  while (*ptr == '/')
	    ptr++;
	  eptr = holy_strchr (ptr, ':');
	  if (eptr)
	    {
	      holy_file_filter_disable_compression ();
	      initrd_ctx->components[i].newc_name = holy_strndup (ptr, eptr - ptr);
	      if (!initrd_ctx->components[i].newc_name)
		{
		  holy_initrd_close (initrd_ctx);
		  return holy_errno;
		}
	      initrd_ctx->size
		+= ALIGN_UP (sizeof (struct newc_head)
			    + holy_strlen (initrd_ctx->components[i].newc_name),
			     4);
	      initrd_ctx->size += insert_dir (initrd_ctx->components[i].newc_name,
					      &root, 0);
	      newc = 1;
	      fname = eptr + 1;
	    }
	}
      else if (newc)
	{
	  initrd_ctx->size += ALIGN_UP (sizeof (struct newc_head)
					+ sizeof ("TRAILER!!!") - 1, 4);
	  free_dir (root);
	  root = 0;
	  newc = 0;
	}
      holy_file_filter_disable_compression ();
      initrd_ctx->components[i].file = holy_file_open (fname);
      if (!initrd_ctx->components[i].file)
	{
	  holy_initrd_close (initrd_ctx);
	  return holy_errno;
	}
      initrd_ctx->nfiles++;
      initrd_ctx->components[i].size
	= holy_file_size (initrd_ctx->components[i].file);
      initrd_ctx->size += initrd_ctx->components[i].size;
    }

  if (newc)
    {
      initrd_ctx->size = ALIGN_UP (initrd_ctx->size, 4);
      initrd_ctx->size += ALIGN_UP (sizeof (struct newc_head)
				    + sizeof ("TRAILER!!!") - 1, 4);
      free_dir (root);
      root = 0;
    }
  
  return holy_ERR_NONE;
}

holy_size_t
holy_get_initrd_size (struct holy_linux_initrd_context *initrd_ctx)
{
  return initrd_ctx->size;
}

void
holy_initrd_close (struct holy_linux_initrd_context *initrd_ctx)
{
  int i;
  if (!initrd_ctx->components)
    return;
  for (i = 0; i < initrd_ctx->nfiles; i++)
    {
      holy_free (initrd_ctx->components[i].newc_name);
      holy_file_close (initrd_ctx->components[i].file);
    }
  holy_free (initrd_ctx->components);
  initrd_ctx->components = 0;
}

holy_err_t
holy_initrd_load (struct holy_linux_initrd_context *initrd_ctx,
		  char *argv[], void *target)
{
  holy_uint8_t *ptr = target;
  int i;
  int newc = 0;
  struct dir *root = 0;
  holy_ssize_t cursize = 0;

  for (i = 0; i < initrd_ctx->nfiles; i++)
    {
      holy_memset (ptr, 0, ALIGN_UP_OVERHEAD (cursize, 4));
      ptr += ALIGN_UP_OVERHEAD (cursize, 4);

      if (initrd_ctx->components[i].newc_name)
	{
	  ptr += insert_dir (initrd_ctx->components[i].newc_name,
			     &root, ptr);
	  ptr = make_header (ptr, initrd_ctx->components[i].newc_name,
			     holy_strlen (initrd_ctx->components[i].newc_name),
			     0100777,
			     initrd_ctx->components[i].size);
	  newc = 1;
	}
      else if (newc)
	{
	  ptr = make_header (ptr, "TRAILER!!!", sizeof ("TRAILER!!!") - 1,
			     0, 0);
	  free_dir (root);
	  root = 0;
	  newc = 0;
	}

      cursize = initrd_ctx->components[i].size;
      if (holy_file_read (initrd_ctx->components[i].file, ptr, cursize)
	  != cursize)
	{
	  if (!holy_errno)
	    holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
			argv[i]);
	  holy_initrd_close (initrd_ctx);
	  return holy_errno;
	}
      holy_tpm_measure (ptr, cursize, holy_BINARY_PCR, "holy_initrd", "Initrd");
      holy_print_error();

      ptr += cursize;
    }
  if (newc)
    {
      holy_memset (ptr, 0, ALIGN_UP_OVERHEAD (cursize, 4));
      ptr += ALIGN_UP_OVERHEAD (cursize, 4);
      ptr = make_header (ptr, "TRAILER!!!", sizeof ("TRAILER!!!") - 1, 0, 0);
    }
  free_dir (root);
  root = 0;
  return holy_ERR_NONE;
}
