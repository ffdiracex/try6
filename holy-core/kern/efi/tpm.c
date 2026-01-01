/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/i18n.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/efi/tpm.h>
#include <holy/mm.h>
#include <holy/tpm.h>
#include <holy/term.h>

static holy_efi_guid_t tpm_guid = EFI_TPM_GUID;
static holy_efi_guid_t tpm2_guid = EFI_TPM2_GUID;

static holy_efi_boolean_t holy_tpm_present(holy_efi_tpm_protocol_t *tpm)
{
  holy_efi_status_t status;
  TCG_EFI_BOOT_SERVICE_CAPABILITY caps;
  holy_uint32_t flags;
  holy_efi_physical_address_t eventlog, lastevent;

  caps.Size = (holy_uint8_t)sizeof(caps);

  status = efi_call_5(tpm->status_check, tpm, &caps, &flags, &eventlog,
		      &lastevent);

  if (status != holy_EFI_SUCCESS || caps.TPMDeactivatedFlag
      || !caps.TPMPresentFlag)
    return 0;

  return 1;
}

static holy_efi_boolean_t holy_tpm2_present(holy_efi_tpm2_protocol_t *tpm)
{
  holy_efi_status_t status;
  EFI_TCG2_BOOT_SERVICE_CAPABILITY caps;

  caps.Size = (holy_uint8_t)sizeof(caps);

  status = efi_call_2(tpm->get_capability, tpm, &caps);

  if (status != holy_EFI_SUCCESS || !caps.TPMPresentFlag)
    return 0;

  return 1;
}

static holy_efi_boolean_t holy_tpm_handle_find(holy_efi_handle_t *tpm_handle,
					       holy_efi_uint8_t *protocol_version)
{
  holy_efi_handle_t *handles;
  holy_efi_uintn_t num_handles;

  handles = holy_efi_locate_handle (holy_EFI_BY_PROTOCOL, &tpm_guid, NULL,
				    &num_handles);
  if (handles && num_handles > 0) {
    *tpm_handle = handles[0];
    *protocol_version = 1;
    return 1;
  }

  handles = holy_efi_locate_handle (holy_EFI_BY_PROTOCOL, &tpm2_guid, NULL,
				    &num_handles);
  if (handles && num_handles > 0) {
    *tpm_handle = handles[0];
    *protocol_version = 2;
    return 1;
  }

  return 0;
}

static holy_err_t
holy_tpm1_execute(holy_efi_handle_t tpm_handle,
		  PassThroughToTPM_InputParamBlock *inbuf,
		  PassThroughToTPM_OutputParamBlock *outbuf)
{
  holy_efi_status_t status;
  holy_efi_tpm_protocol_t *tpm;
  holy_uint32_t inhdrsize = sizeof(*inbuf) - sizeof(inbuf->TPMOperandIn);
  holy_uint32_t outhdrsize = sizeof(*outbuf) - sizeof(outbuf->TPMOperandOut);

  tpm = holy_efi_open_protocol (tpm_handle, &tpm_guid,
				holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (!holy_tpm_present(tpm))
    return 0;

  /* UEFI TPM protocol takes the raw operand block, no param block header */
  status = efi_call_5 (tpm->pass_through_to_tpm, tpm,
		       inbuf->IPBLength - inhdrsize, inbuf->TPMOperandIn,
		       outbuf->OPBLength - outhdrsize, outbuf->TPMOperandOut);

  switch (status) {
  case holy_EFI_SUCCESS:
    return 0;
  case holy_EFI_DEVICE_ERROR:
    return holy_error (holy_ERR_IO, N_("Command failed"));
  case holy_EFI_INVALID_PARAMETER:
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("Invalid parameter"));
  case holy_EFI_BUFFER_TOO_SMALL:
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("Output buffer too small"));
  case holy_EFI_NOT_FOUND:
    return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("TPM unavailable"));
  default:
    return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("Unknown TPM error"));
  }
}

static holy_err_t
holy_tpm2_execute(holy_efi_handle_t tpm_handle,
		  PassThroughToTPM_InputParamBlock *inbuf,
		  PassThroughToTPM_OutputParamBlock *outbuf)
{
  holy_efi_status_t status;
  holy_efi_tpm2_protocol_t *tpm;
  holy_uint32_t inhdrsize = sizeof(*inbuf) - sizeof(inbuf->TPMOperandIn);
  holy_uint32_t outhdrsize = sizeof(*outbuf) - sizeof(outbuf->TPMOperandOut);

  tpm = holy_efi_open_protocol (tpm_handle, &tpm2_guid,
				holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (!holy_tpm2_present(tpm))
    return 0;

  /* UEFI TPM protocol takes the raw operand block, no param block header */
  status = efi_call_5 (tpm->submit_command, tpm,
		       inbuf->IPBLength - inhdrsize, inbuf->TPMOperandIn,
		       outbuf->OPBLength - outhdrsize, outbuf->TPMOperandOut);

  switch (status) {
  case holy_EFI_SUCCESS:
    return 0;
  case holy_EFI_DEVICE_ERROR:
    return holy_error (holy_ERR_IO, N_("Command failed"));
  case holy_EFI_INVALID_PARAMETER:
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("Invalid parameter"));
  case holy_EFI_BUFFER_TOO_SMALL:
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("Output buffer too small"));
  case holy_EFI_NOT_FOUND:
    return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("TPM unavailable"));
  default:
    return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("Unknown TPM error"));
  }
}

holy_err_t
holy_tpm_execute(PassThroughToTPM_InputParamBlock *inbuf,
		 PassThroughToTPM_OutputParamBlock *outbuf)
{
  holy_efi_handle_t tpm_handle;
   holy_uint8_t protocol_version;

  /* It's not a hard failure for there to be no TPM */
  if (!holy_tpm_handle_find(&tpm_handle, &protocol_version))
    return 0;

  if (protocol_version == 1) {
    return holy_tpm1_execute(tpm_handle, inbuf, outbuf);
  } else {
    return holy_tpm2_execute(tpm_handle, inbuf, outbuf);
  }
}

static holy_err_t
holy_tpm1_log_event(holy_efi_handle_t tpm_handle, unsigned char *buf,
		    holy_size_t size, holy_uint8_t pcr,
		    const char *description)
{
  TCG_PCR_EVENT *event;
  holy_efi_status_t status;
  holy_efi_tpm_protocol_t *tpm;
  holy_efi_physical_address_t lastevent;
  holy_uint32_t algorithm;
  holy_uint32_t eventnum = 0;

  tpm = holy_efi_open_protocol (tpm_handle, &tpm_guid,
				holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (!holy_tpm_present(tpm))
    return 0;

  event = holy_zalloc(sizeof (TCG_PCR_EVENT) + holy_strlen(description) + 1);
  if (!event)
    return holy_error (holy_ERR_OUT_OF_MEMORY,
		       N_("cannot allocate TPM event buffer"));

  event->PCRIndex  = pcr;
  event->EventType = EV_IPL;
  event->EventSize = holy_strlen(description) + 1;
  holy_memcpy(event->Event, description, event->EventSize);

  algorithm = TCG_ALG_SHA;
  status = efi_call_7 (tpm->log_extend_event, tpm, (holy_efi_physical_address_t)buf, (holy_uint64_t) size,
		       algorithm, event, &eventnum, &lastevent);

  switch (status) {
  case holy_EFI_SUCCESS:
    return 0;
  case holy_EFI_DEVICE_ERROR:
    return holy_error (holy_ERR_IO, N_("Command failed"));
  case holy_EFI_INVALID_PARAMETER:
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("Invalid parameter"));
  case holy_EFI_BUFFER_TOO_SMALL:
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("Output buffer too small"));
  case holy_EFI_NOT_FOUND:
    return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("TPM unavailable"));
  default:
    return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("Unknown TPM error"));
  }
}

static holy_err_t
holy_tpm2_log_event(holy_efi_handle_t tpm_handle, unsigned char *buf,
		   holy_size_t size, holy_uint8_t pcr,
		   const char *description)
{
  EFI_TCG2_EVENT *event;
  holy_efi_status_t status;
  holy_efi_tpm2_protocol_t *tpm;

  tpm = holy_efi_open_protocol (tpm_handle, &tpm2_guid,
				holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (!holy_tpm2_present(tpm))
    return 0;

  event = holy_zalloc(sizeof (EFI_TCG2_EVENT) + holy_strlen(description) + 1);
  if (!event)
    return holy_error (holy_ERR_OUT_OF_MEMORY,
		       N_("cannot allocate TPM event buffer"));

  event->Header.HeaderSize = sizeof(EFI_TCG2_EVENT_HEADER);
  event->Header.HeaderVersion = 1;
  event->Header.PCRIndex = pcr;
  event->Header.EventType = EV_IPL;
  event->Size = sizeof(*event) - sizeof(event->Event) + holy_strlen(description) + 1;
  holy_memcpy(event->Event, description, holy_strlen(description) + 1);

  status = efi_call_5 (tpm->hash_log_extend_event, tpm, 0, (holy_efi_physical_address_t)buf,
		       (holy_uint64_t) size, event);

  switch (status) {
  case holy_EFI_SUCCESS:
    return 0;
  case holy_EFI_DEVICE_ERROR:
    return holy_error (holy_ERR_IO, N_("Command failed"));
  case holy_EFI_INVALID_PARAMETER:
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("Invalid parameter"));
  case holy_EFI_BUFFER_TOO_SMALL:
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("Output buffer too small"));
  case holy_EFI_NOT_FOUND:
    return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("TPM unavailable"));
  default:
    return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("Unknown TPM error"));
  }
}

holy_err_t
holy_tpm_log_event(unsigned char *buf, holy_size_t size, holy_uint8_t pcr,
		   const char *description)
{
  holy_efi_handle_t tpm_handle;
  holy_efi_uint8_t protocol_version;

  if (!holy_tpm_handle_find(&tpm_handle, &protocol_version))
    return 0;

  if (protocol_version == 1) {
    return holy_tpm1_log_event(tpm_handle, buf, size, pcr, description);
  } else {
    return holy_tpm2_log_event(tpm_handle, buf, size, pcr, description);
  }
}
