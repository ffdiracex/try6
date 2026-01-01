/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_TPM_HEADER
#define holy_TPM_HEADER 1

#define SHA1_DIGEST_SIZE 20

#define TPM_BASE 0x0
#define TPM_SUCCESS TPM_BASE
#define TPM_AUTHFAIL (TPM_BASE + 0x1)
#define TPM_BADINDEX (TPM_BASE + 0x2)

#define holy_ASCII_PCR 8
#define holy_BINARY_PCR 9

#define TPM_TAG_RQU_COMMAND 0x00C1
#define TPM_ORD_Extend 0x14

#define EV_IPL 0x0d

/* TCG_PassThroughToTPM Input Parameter Block */
typedef struct {
        holy_uint16_t IPBLength;
        holy_uint16_t Reserved1;
        holy_uint16_t OPBLength;
        holy_uint16_t Reserved2;
        holy_uint8_t TPMOperandIn[1];
} holy_PACKED PassThroughToTPM_InputParamBlock;

/* TCG_PassThroughToTPM Output Parameter Block */
typedef struct {
        holy_uint16_t OPBLength;
        holy_uint16_t Reserved;
        holy_uint8_t TPMOperandOut[1];
} holy_PACKED PassThroughToTPM_OutputParamBlock;

typedef struct {
        holy_uint16_t tag;
        holy_uint32_t paramSize;
        holy_uint32_t ordinal;
        holy_uint32_t pcrNum;
        holy_uint8_t inDigest[SHA1_DIGEST_SIZE];                /* The 160 bit value representing the event to be recorded. */
} holy_PACKED ExtendIncoming;

/* TPM_Extend Outgoing Operand */
typedef struct {
        holy_uint16_t tag;
        holy_uint32_t paramSize;
        holy_uint32_t returnCode;
        holy_uint8_t outDigest[SHA1_DIGEST_SIZE];               /* The PCR value after execution of the command. */
} holy_PACKED ExtendOutgoing;

holy_err_t EXPORT_FUNC(holy_tpm_measure) (unsigned char *buf, holy_size_t size,
					  holy_uint8_t pcr, const char *kind,
					  const char *description);
#if defined (holy_MACHINE_EFI) || defined (holy_MACHINE_PCBIOS)
holy_err_t holy_tpm_execute(PassThroughToTPM_InputParamBlock *inbuf,
			    PassThroughToTPM_OutputParamBlock *outbuf);
holy_err_t holy_tpm_log_event(unsigned char *buf, holy_size_t size,
			      holy_uint8_t pcr, const char *description);
#else
static inline holy_err_t holy_tpm_execute(
	PassThroughToTPM_InputParamBlock *inbuf __attribute__ ((unused)),
	PassThroughToTPM_OutputParamBlock *outbuf __attribute__ ((unused)))
{
	return 0;
};
static inline holy_err_t holy_tpm_log_event(
	unsigned char *buf __attribute__ ((unused)),
	holy_size_t size __attribute__ ((unused)),
	holy_uint8_t pcr __attribute__ ((unused)),
	const char *description __attribute__ ((unused)))
{
	return 0;
};
#endif

#endif
