/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/fs.h>
#include <holy/ntfs.h>
#include <holy/fat.h>
#include <holy/exfat.h>
#include <holy/udf.h>
#include <holy/util/misc.h>
#include <holy/util/install.h>
#include <holy/emu/getroot.h>
#include <holy/emu/hostfile.h>

#include <windows.h>
#include <winioctl.h>

void
holy_install_get_blocklist (holy_device_t root_dev,
			    const char *core_path,
			    const char *core_img __attribute__ ((unused)),
			    size_t core_size,
			    void (*callback) (holy_disk_addr_t sector,
					      unsigned offset,
					      unsigned length,
					      void *data),
			    void *hook_data)
{
  holy_disk_addr_t first_lcn = 0;
  HANDLE filehd;
  DWORD rets;
  RETRIEVAL_POINTERS_BUFFER *extbuf;
  size_t extbuf_size;
  DWORD i;
  holy_uint64_t sec_per_lcn;
  holy_uint64_t curvcn = 0;
  STARTING_VCN_INPUT_BUFFER start_vcn;
  holy_fs_t fs;
  holy_err_t err;

  fs = holy_fs_probe (root_dev);
  if (!fs)
    holy_util_error ("%s", holy_errmsg);

  /* This is ugly but windows doesn't give all needed data. Or does anyone
     have a pointer how to retrieve it?
   */
  if (holy_strcmp (fs->name, "ntfs") == 0)
    {
      struct holy_ntfs_bpb bpb;
      err = holy_disk_read (root_dev->disk, 0, 0, sizeof (bpb), &bpb);
      if (err)
	holy_util_error ("%s", holy_errmsg);
      sec_per_lcn = ((holy_uint32_t) bpb.sectors_per_cluster
		     * (holy_uint32_t) holy_le_to_cpu16 (bpb.bytes_per_sector))
	>> 9;
      first_lcn = 0;
    }
  else if (holy_strcmp (fs->name, "exfat") == 0)
    first_lcn = holy_exfat_get_cluster_sector (root_dev->disk, &sec_per_lcn);
  else if (holy_strcmp (fs->name, "fat") == 0)
    first_lcn = holy_fat_get_cluster_sector (root_dev->disk, &sec_per_lcn);
  else if (holy_strcmp (fs->name, "udf") == 0)
    first_lcn = holy_udf_get_cluster_sector (root_dev->disk, &sec_per_lcn);
  else
    holy_util_error ("unsupported fs for blocklist on windows: %s",
		     fs->name);

  holy_util_info ("sec_per_lcn = %"  holy_HOST_PRIuLONG_LONG
		  ", first_lcn=%" holy_HOST_PRIuLONG_LONG,
		  (unsigned long long) sec_per_lcn,
		  (unsigned long long) first_lcn);

  first_lcn += holy_partition_get_start (root_dev->disk->partition);

  start_vcn.StartingVcn.QuadPart = 0;

  filehd = holy_util_fd_open (core_path, holy_UTIL_FD_O_RDONLY);
  if (!holy_UTIL_FD_IS_VALID (filehd))
    holy_util_error (_("cannot open `%s': %s"), core_path,
		     holy_util_fd_strerror ());

  extbuf_size = sizeof (*extbuf) + sizeof (extbuf->Extents[0])
    * ((core_size + 511) / 512);
  extbuf = xmalloc (extbuf_size);

  if (!DeviceIoControl(filehd, FSCTL_GET_RETRIEVAL_POINTERS,
		       &start_vcn, sizeof (start_vcn),
		       extbuf, extbuf_size, &rets, NULL))
    holy_util_error ("FSCTL_GET_RETRIEVAL_POINTERS fails: %s",
		     holy_util_fd_strerror ());

  CloseHandle (filehd);

  for (i = 0; i < extbuf->ExtentCount; i++)
    callback (extbuf->Extents[i].Lcn.QuadPart
	      * sec_per_lcn + first_lcn,
	      0, 512 * sec_per_lcn * (extbuf->Extents[i].NextVcn.QuadPart - curvcn), hook_data);
  free (extbuf);
}
