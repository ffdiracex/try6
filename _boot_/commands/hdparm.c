/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/ata.h>
#include <holy/scsi.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/lib/hexdump.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] = {
  {"apm",             'B', 0, N_("Set Advanced Power Management\n"
			      "(1=low, ..., 254=high, 255=off)."),
			      0, ARG_TYPE_INT},
  {"power",           'C', 0, N_("Display power mode."), 0, ARG_TYPE_NONE},
  {"security-freeze", 'F', 0, N_("Freeze ATA security settings until reset."),
			      0, ARG_TYPE_NONE},
  {"health",          'H', 0, N_("Display SMART health status."), 0, ARG_TYPE_NONE},
  {"aam",             'M', 0, N_("Set Automatic Acoustic Management\n"
			      "(0=off, 128=quiet, ..., 254=fast)."),
			      0, ARG_TYPE_INT},
  {"standby-timeout", 'S', 0, N_("Set standby timeout\n"
			      "(0=off, 1=5s, 2=10s, ..., 240=20m, 241=30m, ...)."),
			      0, ARG_TYPE_INT},
  {"standby",         'y', 0, N_("Set drive to standby mode."), 0, ARG_TYPE_NONE},
  {"sleep",           'Y', 0, N_("Set drive to sleep mode."), 0, ARG_TYPE_NONE},
  {"identify",        'i', 0, N_("Print drive identity and settings."),
			      0, ARG_TYPE_NONE},
  {"dumpid",          'I', 0, N_("Show raw contents of ATA IDENTIFY sector."),
			       0, ARG_TYPE_NONE},
  {"smart",            -1, 0, N_("Disable/enable SMART (0/1)."), 0, ARG_TYPE_INT},
  {"quiet",           'q', 0, N_("Do not print messages."), 0, ARG_TYPE_NONE},
  {0, 0, 0, 0, 0, 0}
};

enum holy_ata_smart_commands
  {
    holy_ATA_FEAT_SMART_ENABLE  = 0xd8,
    holy_ATA_FEAT_SMART_DISABLE = 0xd9,
    holy_ATA_FEAT_SMART_STATUS  = 0xda,
  };

static int quiet = 0;

static holy_err_t
holy_hdparm_do_ata_cmd (holy_ata_t ata, holy_uint8_t cmd,
			holy_uint8_t features, holy_uint8_t sectors,
			void * buffer, int size)
{
  struct holy_disk_ata_pass_through_parms apt;
  holy_memset (&apt, 0, sizeof (apt));

  apt.taskfile.cmd = cmd;
  apt.taskfile.features = features;
  apt.taskfile.sectors = sectors;
  apt.taskfile.disk = 0xE0;

  apt.buffer = buffer;
  apt.size = size;

  if (ata->dev->readwrite (ata, &apt, 0))
    return holy_errno;

  return holy_ERR_NONE;
}

static int
holy_hdparm_do_check_powermode_cmd (holy_ata_t ata)
{
  struct holy_disk_ata_pass_through_parms apt;
  holy_memset (&apt, 0, sizeof (apt));

  apt.taskfile.cmd = holy_ATA_CMD_CHECK_POWER_MODE;
  apt.taskfile.disk = 0xE0;

  if (ata->dev->readwrite (ata, &apt, 0))
    return -1;

  return apt.taskfile.sectors;
}

static int
holy_hdparm_do_smart_cmd (holy_ata_t ata, holy_uint8_t features)
{
  struct holy_disk_ata_pass_through_parms apt;
  holy_memset (&apt, 0, sizeof (apt));

  apt.taskfile.cmd = holy_ATA_CMD_SMART;
  apt.taskfile.features = features;
  apt.taskfile.lba_mid  = 0x4f;
  apt.taskfile.lba_high = 0xc2;
  apt.taskfile.disk = 0xE0;

  if (ata->dev->readwrite (ata, &apt, 0))
    return -1;

  if (features == holy_ATA_FEAT_SMART_STATUS)
    {
      if (   apt.taskfile.lba_mid  == 0x4f
          && apt.taskfile.lba_high == 0xc2)
	return 0; /* Good SMART status.  */
      else if (   apt.taskfile.lba_mid  == 0xf4
	       && apt.taskfile.lba_high == 0x2c)
	return 1; /* Bad SMART status.  */
      else
	return -1;
    }
  return 0;
}

static holy_err_t
holy_hdparm_simple_cmd (const char * msg,
			holy_ata_t ata, holy_uint8_t cmd)
{
  if (! quiet && msg)
    holy_printf ("%s", msg);

  holy_err_t err = holy_hdparm_do_ata_cmd (ata, cmd, 0, 0, NULL, 0);

  if (! quiet && msg)
    holy_printf ("%s\n", ! err ? "" : ": not supported");
  return err;
}

static holy_err_t
holy_hdparm_set_val_cmd (const char * msg, int val,
			 holy_ata_t ata, holy_uint8_t cmd,
			 holy_uint8_t features, holy_uint8_t sectors)
{
  if (! quiet && msg && *msg)
    {
      if (val >= 0)
	holy_printf ("Set %s to %d", msg, val);
      else
	holy_printf ("Disable %s", msg);
    }

  holy_err_t err = holy_hdparm_do_ata_cmd (ata, cmd, features, sectors,
					   NULL, 0);

  if (! quiet && msg)
    holy_printf ("%s\n", ! err ? "" : ": not supported");
  return err;
}

static const char *
le16_to_char (holy_uint16_t *dest, const holy_uint16_t * src16, unsigned bytes)
{
  unsigned i;
  for (i = 0; i < bytes / 2; i++)
    dest[i] = holy_swap_bytes16 (src16[i]);
  dest[i] = 0;
  return (char *) dest;
}

static void
holy_hdparm_print_identify (const holy_uint16_t * idw)
{
  /* Print identity strings.  */
  holy_uint16_t tmp[21];
  holy_printf ("Model:    \"%.40s\"\n", le16_to_char (tmp, &idw[27], 40));
  holy_printf ("Firmware: \"%.8s\"\n",  le16_to_char (tmp, &idw[23], 8));
  holy_printf ("Serial:   \"%.20s\"\n", le16_to_char (tmp, &idw[10], 20));

  /* Print AAM, APM and SMART settings.  */
  holy_uint16_t features1 = holy_le_to_cpu16 (idw[82]);
  holy_uint16_t features2 = holy_le_to_cpu16 (idw[83]);
  holy_uint16_t enabled1  = holy_le_to_cpu16 (idw[85]);
  holy_uint16_t enabled2  = holy_le_to_cpu16 (idw[86]);

  holy_printf ("Automatic Acoustic Management: ");
  if (features2 & 0x0200)
    {
      if (enabled2 & 0x0200)
	{
	  holy_uint16_t aam = holy_le_to_cpu16 (idw[94]);
	  holy_printf ("%u (128=quiet, ..., 254=fast, recommended=%u)\n",
		       aam & 0xff, (aam >> 8) & 0xff);
	}
      else
	holy_printf ("disabled\n");
    }
  else
    holy_printf ("not supported\n");

  holy_printf ("Advanced Power Management: ");
  if (features2 & 0x0008)
    {
      if (enabled2 & 0x0008)
	holy_printf ("%u (1=low, ..., 254=high)\n",
		     holy_le_to_cpu16 (idw[91]) & 0xff);
      else
	holy_printf ("disabled\n");
    }
  else
    holy_printf ("not supported\n");

  holy_printf ("SMART Feature Set: ");
  if (features1 & 0x0001)
    holy_printf ("%sabled\n", (enabled1 & 0x0001 ? "en" : "dis"));
  else
    holy_printf ("not supported\n");

  /* Print security settings.  */
  holy_uint16_t security = holy_le_to_cpu16 (idw[128]);

  holy_printf ("ATA Security: ");
  if (security & 0x0001)
    holy_printf ("%s, %s, %s, %s\n",
		 (security & 0x0002 ? "ENABLED" : "disabled"),
		 (security & 0x0004 ? "**LOCKED**"  : "not locked"),
		 (security & 0x0008 ? "frozen" : "NOT FROZEN"),
		 (security & 0x0010 ? "COUNT EXPIRED" : "count not expired"));
  else
    holy_printf ("not supported\n");
}

static void
holy_hdparm_print_standby_tout (int timeout)
{
  if (timeout == 0)
    holy_printf ("off");
  else if (timeout <= 252 || timeout == 255)
    {
      int h = 0, m = 0 , s = 0;
      if (timeout == 255)
	{
	  m = 21;
	  s = 15;
	}
      else if (timeout == 252)
	m = 21;
      else if (timeout <= 240)
	{
	  s = timeout * 5;
	  m = s / 60;
	  s %= 60;
	}
      else
	{
	  m = (timeout - 240) * 30;
	  h  = m / 60;
	  m %= 60;
	}
      holy_printf ("%02d:%02d:%02d", h, m, s);
    }
  else
    holy_printf ("invalid or vendor-specific");
}

static int get_int_arg (const struct holy_arg_list *state)
{
  return (state->set ? (int)holy_strtoul (state->arg, 0, 0) : -1);
}

static holy_err_t
holy_cmd_hdparm (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  struct holy_ata *ata;
  const char *diskname;

  /* Check command line.  */
  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  if (args[0][0] == '(')
    {
      holy_size_t len = holy_strlen (args[0]);
      if (args[0][len - 1] == ')')
	args[0][len - 1] = 0;
      diskname = &args[0][1];
    }
  else
    diskname = &args[0][0];

  int i = 0;
  int apm          = get_int_arg (&state[i++]);
  int power        = state[i++].set;
  int sec_freeze   = state[i++].set;
  int health       = state[i++].set;
  int aam          = get_int_arg (&state[i++]);
  int standby_tout = get_int_arg (&state[i++]);
  int standby_now  = state[i++].set;
  int sleep_now    = state[i++].set;
  int ident        = state[i++].set;
  int dumpid       = state[i++].set;
  int enable_smart = get_int_arg (&state[i++]);
  quiet            = state[i++].set;

  /* Open disk.  */
  holy_disk_t disk = holy_disk_open (diskname);
  if (! disk)
    return holy_errno;

  switch (disk->dev->id)
    {
    case holy_DISK_DEVICE_ATA_ID:
      ata = disk->data;
      break;
    case holy_DISK_DEVICE_SCSI_ID:
      if (((disk->id >> holy_SCSI_ID_SUBSYSTEM_SHIFT) & 0xFF)
	  == holy_SCSI_SUBSYSTEM_PATA
	  || (((disk->id >> holy_SCSI_ID_SUBSYSTEM_SHIFT) & 0xFF)
	      == holy_SCSI_SUBSYSTEM_AHCI))
	{
	  ata = ((struct holy_scsi *) disk->data)->data;
	  break;
	}
      /* FALLTHROUGH */
    default:
      holy_disk_close (disk);
      return holy_error (holy_ERR_IO, "not an ATA device");
    }
    

  /* Change settings.  */
  if (aam >= 0)
    holy_hdparm_set_val_cmd ("Automatic Acoustic Management", (aam ? aam : -1),
			     ata, holy_ATA_CMD_SET_FEATURES,
			     (aam ? 0x42 : 0xc2), aam);

  if (apm >= 0)
    holy_hdparm_set_val_cmd ("Advanced Power Management",
			     (apm != 255 ? apm : -1), ata,
			     holy_ATA_CMD_SET_FEATURES,
			     (apm != 255 ? 0x05 : 0x85),
			     (apm != 255 ? apm : 0));

  if (standby_tout >= 0)
    {
      if (! quiet)
	{
	  holy_printf ("Set standby timeout to %d (", standby_tout);
	  holy_hdparm_print_standby_tout (standby_tout);
	  holy_printf (")");
	}
      /* The IDLE cmd sets disk to idle mode and configures standby timer.  */
      holy_hdparm_set_val_cmd ("", -1, ata, holy_ATA_CMD_IDLE, 0, standby_tout);
    }

  if (enable_smart >= 0)
    {
      if (! quiet)
	holy_printf ("%sable SMART operations", (enable_smart ? "En" : "Dis"));
      int err = holy_hdparm_do_smart_cmd (ata, (enable_smart ?
	          holy_ATA_FEAT_SMART_ENABLE : holy_ATA_FEAT_SMART_DISABLE));
      if (! quiet)
	holy_printf ("%s\n", err ? ": not supported" : "");
    }

  if (sec_freeze)
    holy_hdparm_simple_cmd ("Freeze security settings", ata,
                            holy_ATA_CMD_SECURITY_FREEZE_LOCK);

  /* Print/dump IDENTIFY.  */
  if (ident || dumpid)
    {
      holy_uint16_t buf[holy_DISK_SECTOR_SIZE / 2];
      if (holy_hdparm_do_ata_cmd (ata, holy_ATA_CMD_IDENTIFY_DEVICE,
          0, 0, buf, sizeof (buf)))
	holy_printf ("Cannot read ATA IDENTIFY data\n");
      else
	{
	  if (ident)
	    holy_hdparm_print_identify (buf);
	  if (dumpid)
	    hexdump (0, (char *) buf, sizeof (buf));
	}
    }

  /* Check power mode.  */
  if (power)
    {
      holy_printf ("Disk power mode is: ");
      int mode = holy_hdparm_do_check_powermode_cmd (ata);
      if (mode < 0)
        holy_printf ("unknown\n");
      else
	holy_printf ("%s (0x%02x)\n",
		     (mode == 0xff ? "active/idle" :
		      mode == 0x80 ? "idle" :
		      mode == 0x00 ? "standby" : "unknown"), mode);
    }

  /* Check health.  */
  int status = 0;
  if (health)
    {
      if (! quiet)
	holy_printf ("SMART status is: ");
      int err = holy_hdparm_do_smart_cmd (ata, holy_ATA_FEAT_SMART_STATUS);
      if (! quiet)
	holy_printf ("%s\n", (err  < 0 ? "unknown" :
	                      err == 0 ? "OK" : "*BAD*"));
      status = (err > 0);
    }

  /* Change power mode.  */
  if (standby_now)
    holy_hdparm_simple_cmd ("Set disk to standby mode", ata,
			    holy_ATA_CMD_STANDBY_IMMEDIATE);

  if (sleep_now)
    holy_hdparm_simple_cmd ("Set disk to sleep mode", ata,
			    holy_ATA_CMD_SLEEP);

  holy_disk_close (disk);

  holy_errno = holy_ERR_NONE;
  return status;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(hdparm)
{
  cmd = holy_register_extcmd ("hdparm", holy_cmd_hdparm, 0,
			      N_("[OPTIONS] DISK"),
			      N_("Get/set ATA disk parameters."), options);
}

holy_MOD_FINI(hdparm)
{
  holy_unregister_extcmd (cmd);
}
