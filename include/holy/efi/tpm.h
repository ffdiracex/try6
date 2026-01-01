/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_TPM_HEADER
#define holy_EFI_TPM_HEADER 1

#define EFI_TPM_GUID {0xf541796d, 0xa62e, 0x4954, {0xa7, 0x75, 0x95, 0x84, 0xf6, 0x1b, 0x9c, 0xdd }};
#define EFI_TPM2_GUID {0x607f766c, 0x7455, 0x42be, {0x93, 0x0b, 0xe4, 0xd7, 0x6d, 0xb2, 0x72, 0x0f }};

typedef struct {
  holy_efi_uint8_t Major;
  holy_efi_uint8_t Minor;
  holy_efi_uint8_t RevMajor;
  holy_efi_uint8_t RevMinor;
} TCG_VERSION;

typedef struct _TCG_EFI_BOOT_SERVICE_CAPABILITY {
  holy_efi_uint8_t          Size;                /// Size of this structure.
  TCG_VERSION    StructureVersion;
  TCG_VERSION    ProtocolSpecVersion;
  holy_efi_uint8_t          HashAlgorithmBitmap; /// Hash algorithms .
  char        TPMPresentFlag;      /// 00h = TPM not present.
  char        TPMDeactivatedFlag;  /// 01h = TPM currently deactivated.
} TCG_EFI_BOOT_SERVICE_CAPABILITY;

typedef struct {
  holy_efi_uint32_t PCRIndex;
  holy_efi_uint32_t EventType;
  holy_efi_uint8_t digest[20];
  holy_efi_uint32_t EventSize;
  holy_efi_uint8_t  Event[1];
} TCG_PCR_EVENT;

struct holy_efi_tpm_protocol
{
  holy_efi_status_t (*status_check) (struct holy_efi_tpm_protocol *this,
				     TCG_EFI_BOOT_SERVICE_CAPABILITY *ProtocolCapability,
				     holy_efi_uint32_t *TCGFeatureFlags,
				     holy_efi_physical_address_t *EventLogLocation,
				     holy_efi_physical_address_t *EventLogLastEntry);
  holy_efi_status_t (*hash_all) (struct holy_efi_tpm_protocol *this,
				 holy_efi_uint8_t *HashData,
				 holy_efi_uint64_t HashLen,
				 holy_efi_uint32_t AlgorithmId,
				 holy_efi_uint64_t *HashedDataLen,
				 holy_efi_uint8_t **HashedDataResult);
  holy_efi_status_t (*log_event) (struct holy_efi_tpm_protocol *this,
				  TCG_PCR_EVENT *TCGLogData,
				  holy_efi_uint32_t *EventNumber,
				  holy_efi_uint32_t Flags);
  holy_efi_status_t (*pass_through_to_tpm) (struct holy_efi_tpm_protocol *this,
					    holy_efi_uint32_t TpmInputParameterBlockSize,
					    holy_efi_uint8_t *TpmInputParameterBlock,
					    holy_efi_uint32_t TpmOutputParameterBlockSize,
					    holy_efi_uint8_t *TpmOutputParameterBlock);
  holy_efi_status_t (*log_extend_event) (struct holy_efi_tpm_protocol *this,
					 holy_efi_physical_address_t HashData,
					 holy_efi_uint64_t HashDataLen,
					 holy_efi_uint32_t AlgorithmId,
					 TCG_PCR_EVENT *TCGLogData,
					 holy_efi_uint32_t *EventNumber,
					 holy_efi_physical_address_t *EventLogLastEntry);
};

typedef struct holy_efi_tpm_protocol holy_efi_tpm_protocol_t;

typedef holy_efi_uint32_t EFI_TCG2_EVENT_LOG_BITMAP;
typedef holy_efi_uint32_t EFI_TCG2_EVENT_LOG_FORMAT;
typedef holy_efi_uint32_t EFI_TCG2_EVENT_ALGORITHM_BITMAP;

typedef struct tdEFI_TCG2_VERSION {
  holy_efi_uint8_t Major;
  holy_efi_uint8_t Minor;
} holy_PACKED EFI_TCG2_VERSION;

typedef struct tdEFI_TCG2_BOOT_SERVICE_CAPABILITY {
  holy_efi_uint8_t Size;
  EFI_TCG2_VERSION StructureVersion;
  EFI_TCG2_VERSION ProtocolVersion;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP HashAlgorithmBitmap;
  EFI_TCG2_EVENT_LOG_BITMAP SupportedEventLogs;
  holy_efi_boolean_t TPMPresentFlag;
  holy_efi_uint16_t MaxCommandSize;
  holy_efi_uint16_t MaxResponseSize;
  holy_efi_uint32_t ManufacturerID;
  holy_efi_uint32_t NumberOfPcrBanks;
  EFI_TCG2_EVENT_ALGORITHM_BITMAP ActivePcrBanks;
} EFI_TCG2_BOOT_SERVICE_CAPABILITY;

typedef holy_efi_uint32_t TCG_PCRINDEX;
typedef holy_efi_uint32_t TCG_EVENTTYPE;

typedef struct tdEFI_TCG2_EVENT_HEADER {
  holy_efi_uint32_t HeaderSize;
  holy_efi_uint16_t HeaderVersion;
  TCG_PCRINDEX PCRIndex;
  TCG_EVENTTYPE EventType;
} holy_PACKED EFI_TCG2_EVENT_HEADER;

typedef struct tdEFI_TCG2_EVENT {
  holy_efi_uint32_t Size;
  EFI_TCG2_EVENT_HEADER Header;
  holy_efi_uint8_t Event[1];
} holy_PACKED EFI_TCG2_EVENT;

struct holy_efi_tpm2_protocol
{
  holy_efi_status_t (*get_capability) (struct holy_efi_tpm2_protocol *this,
				       EFI_TCG2_BOOT_SERVICE_CAPABILITY *ProtocolCapability);
  holy_efi_status_t (*get_event_log) (struct holy_efi_tpm2_protocol *this,
				      EFI_TCG2_EVENT_LOG_FORMAT EventLogFormat,
				      holy_efi_physical_address_t *EventLogLocation,
				      holy_efi_physical_address_t *EventLogLastEntry,
				      holy_efi_boolean_t *EventLogTruncated);
  holy_efi_status_t (*hash_log_extend_event) (struct holy_efi_tpm2_protocol *this,
					      holy_efi_uint64_t Flags,
					      holy_efi_physical_address_t DataToHash,
					      holy_efi_uint64_t DataToHashLen,
					      EFI_TCG2_EVENT *EfiTcgEvent);
  holy_efi_status_t (*submit_command) (struct holy_efi_tpm2_protocol *this,
				       holy_efi_uint32_t InputParameterBlockSize,
				       holy_efi_uint8_t *InputParameterBlock,
				       holy_efi_uint32_t OutputParameterBlockSize,
				       holy_efi_uint8_t *OutputParameterBlock);
  holy_efi_status_t (*get_active_pcr_blanks) (struct holy_efi_tpm2_protocol *this,
					      holy_efi_uint32_t *ActivePcrBanks);
  holy_efi_status_t (*set_active_pcr_banks) (struct holy_efi_tpm2_protocol *this,
					     holy_efi_uint32_t ActivePcrBanks);
  holy_efi_status_t (*get_result_of_set_active_pcr_banks) (struct holy_efi_tpm2_protocol *this,
							   holy_efi_uint32_t *OperationPresent,
							   holy_efi_uint32_t *Response);
};

typedef struct holy_efi_tpm2_protocol holy_efi_tpm2_protocol_t;

#define TCG_ALG_SHA 0x00000004

#endif
