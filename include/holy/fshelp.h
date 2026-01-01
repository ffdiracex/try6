/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FSHELP_HEADER
#define holy_FSHELP_HEADER	1

#include <holy/types.h>
#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/disk.h>

typedef struct holy_fshelp_node *holy_fshelp_node_t;

#define holy_FSHELP_CASE_INSENSITIVE	0x100
#define holy_FSHELP_TYPE_MASK	0xff
#define holy_FSHELP_FLAGS_MASK	0x100

enum holy_fshelp_filetype
  {
    holy_FSHELP_UNKNOWN,
    holy_FSHELP_REG,
    holy_FSHELP_DIR,
    holy_FSHELP_SYMLINK
  };

typedef int (*holy_fshelp_iterate_dir_hook_t) (const char *filename,
					       enum holy_fshelp_filetype filetype,
					       holy_fshelp_node_t node,
					       void *data);

/* Lookup the node PATH.  The node ROOTNODE describes the root of the
   directory tree.  The node found is returned in FOUNDNODE, which is
   either a ROOTNODE or a new malloc'ed node.  ITERATE_DIR is used to
   iterate over all directory entries in the current node.
   READ_SYMLINK is used to read the symlink if a node is a symlink.
   EXPECTTYPE is the type node that is expected by the called, an
   error is generated if the node is not of the expected type.  */
holy_err_t
EXPORT_FUNC(holy_fshelp_find_file) (const char *path,
				    holy_fshelp_node_t rootnode,
				    holy_fshelp_node_t *foundnode,
				    int (*iterate_dir) (holy_fshelp_node_t dir,
							holy_fshelp_iterate_dir_hook_t hook,
							void *hook_data),
				    char *(*read_symlink) (holy_fshelp_node_t node),
				    enum holy_fshelp_filetype expect);


holy_err_t
EXPORT_FUNC(holy_fshelp_find_file_lookup) (const char *path,
					   holy_fshelp_node_t rootnode,
					   holy_fshelp_node_t *foundnode,
					   holy_err_t (*lookup_file) (holy_fshelp_node_t dir,
								      const char *name,
								      holy_fshelp_node_t *foundnode,
								      enum holy_fshelp_filetype *foundtype),
					   char *(*read_symlink) (holy_fshelp_node_t node),
					   enum holy_fshelp_filetype expect);

/* Read LEN bytes from the file NODE on disk DISK into the buffer BUF,
   beginning with the block POS.  READ_HOOK should be set before
   reading a block from the file.  GET_BLOCK is used to translate file
   blocks to disk blocks.  The file is FILESIZE bytes big and the
   blocks have a size of LOG2BLOCKSIZE (in log2).  */
holy_ssize_t
EXPORT_FUNC(holy_fshelp_read_file) (holy_disk_t disk, holy_fshelp_node_t node,
				    holy_disk_read_hook_t read_hook,
				    void *read_hook_data,
				    holy_off_t pos, holy_size_t len, char *buf,
				    holy_disk_addr_t (*get_block) (holy_fshelp_node_t node,
                                                                   holy_disk_addr_t block),
				    holy_off_t filesize, int log2blocksize,
				    holy_disk_addr_t blocks_start);

#endif /* ! holy_FSHELP_HEADER */
