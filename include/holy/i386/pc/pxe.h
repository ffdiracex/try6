/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CPU_PXE_H
#define holy_CPU_PXE_H

#include <holy/types.h>

#define holy_PXENV_TFTP_OPEN			0x0020
#define holy_PXENV_TFTP_CLOSE			0x0021
#define holy_PXENV_TFTP_READ			0x0022
#define holy_PXENV_TFTP_READ_FILE		0x0023
#define holy_PXENV_TFTP_READ_FILE_PMODE		0x0024
#define holy_PXENV_TFTP_GET_FSIZE		0x0025

#define holy_PXENV_UDP_OPEN			0x0030
#define holy_PXENV_UDP_CLOSE			0x0031
#define holy_PXENV_UDP_READ			0x0032
#define holy_PXENV_UDP_WRITE			0x0033

#define holy_PXENV_START_UNDI			0x0000
#define holy_PXENV_UNDI_STARTUP			0x0001
#define holy_PXENV_UNDI_CLEANUP			0x0002
#define holy_PXENV_UNDI_INITIALIZE		0x0003
#define holy_PXENV_UNDI_RESET_NIC		0x0004
#define holy_PXENV_UNDI_SHUTDOWN		0x0005
#define holy_PXENV_UNDI_OPEN			0x0006
#define holy_PXENV_UNDI_CLOSE			0x0007
#define holy_PXENV_UNDI_TRANSMIT		0x0008
#define holy_PXENV_UNDI_SET_MCAST_ADDR		0x0009
#define holy_PXENV_UNDI_SET_STATION_ADDR	0x000A
#define holy_PXENV_UNDI_SET_PACKET_FILTER	0x000B
#define holy_PXENV_UNDI_GET_INFORMATION		0x000C
#define holy_PXENV_UNDI_GET_STATISTICS		0x000D
#define holy_PXENV_UNDI_CLEAR_STATISTICS	0x000E
#define holy_PXENV_UNDI_INITIATE_DIAGS		0x000F
#define holy_PXENV_UNDI_FORCE_INTERRUPT		0x0010
#define holy_PXENV_UNDI_GET_MCAST_ADDR		0x0011
#define holy_PXENV_UNDI_GET_NIC_TYPE		0x0012
#define holy_PXENV_UNDI_GET_IFACE_INFO		0x0013
#define holy_PXENV_UNDI_ISR			0x0014
#define	holy_PXENV_STOP_UNDI			0x0015
#define holy_PXENV_UNDI_GET_STATE		0x0015

#define holy_PXENV_UNLOAD_STACK			0x0070
#define holy_PXENV_GET_CACHED_INFO		0x0071
#define holy_PXENV_RESTART_DHCP			0x0072
#define holy_PXENV_RESTART_TFTP			0x0073
#define holy_PXENV_MODE_SWITCH			0x0074
#define holy_PXENV_START_BASE			0x0075
#define holy_PXENV_STOP_BASE			0x0076

#define holy_PXENV_EXIT_SUCCESS			0x0000
#define holy_PXENV_EXIT_FAILURE			0x0001

#define holy_PXENV_STATUS_SUCCESS				0x00
#define holy_PXENV_STATUS_FAILURE				0x01
#define holy_PXENV_STATUS_BAD_FUNC				0x02
#define holy_PXENV_STATUS_UNSUPPORTED				0x03
#define holy_PXENV_STATUS_KEEP_UNDI				0x04
#define holy_PXENV_STATUS_KEEP_ALL				0x05
#define holy_PXENV_STATUS_OUT_OF_RESOURCES			0x06
#define holy_PXENV_STATUS_ARP_TIMEOUT				0x11
#define holy_PXENV_STATUS_UDP_CLOSED				0x18
#define holy_PXENV_STATUS_UDP_OPEN				0x19
#define holy_PXENV_STATUS_TFTP_CLOSED				0x1A
#define holy_PXENV_STATUS_TFTP_OPEN				0x1B
#define holy_PXENV_STATUS_MCOPY_PROBLEM				0x20
#define holy_PXENV_STATUS_BIS_INTEGRITY_FAILURE			0x21
#define holy_PXENV_STATUS_BIS_VALIDATE_FAILURE			0x22
#define holy_PXENV_STATUS_BIS_INIT_FAILURE			0x23
#define holy_PXENV_STATUS_BIS_SHUTDOWN_FAILURE			0x24
#define holy_PXENV_STATUS_BIS_GBOA_FAILURE			0x25
#define holy_PXENV_STATUS_BIS_FREE_FAILURE			0x26
#define holy_PXENV_STATUS_BIS_GSI_FAILURE			0x27
#define holy_PXENV_STATUS_BIS_BAD_CKSUM				0x28
#define holy_PXENV_STATUS_TFTP_CANNOT_ARP_ADDRESS		0x30
#define holy_PXENV_STATUS_TFTP_OPEN_TIMEOUT			0x32

#define holy_PXENV_STATUS_TFTP_UNKNOWN_OPCODE			0x33
#define holy_PXENV_STATUS_TFTP_READ_TIMEOUT			0x35
#define holy_PXENV_STATUS_TFTP_ERROR_OPCODE			0x36
#define holy_PXENV_STATUS_TFTP_CANNOT_OPEN_CONNECTION		0x38
#define holy_PXENV_STATUS_TFTP_CANNOT_READ_FROM_CONNECTION	0x39
#define holy_PXENV_STATUS_TFTP_TOO_MANY_PACKAGES		0x3A
#define holy_PXENV_STATUS_TFTP_FILE_NOT_FOUND			0x3B
#define holy_PXENV_STATUS_TFTP_ACCESS_VIOLATION			0x3C
#define holy_PXENV_STATUS_TFTP_NO_MCAST_ADDRESS			0x3D
#define holy_PXENV_STATUS_TFTP_NO_FILESIZE			0x3E
#define holy_PXENV_STATUS_TFTP_INVALID_PACKET_SIZE		0x3F
#define holy_PXENV_STATUS_DHCP_TIMEOUT				0x51
#define holy_PXENV_STATUS_DHCP_NO_IP_ADDRESS			0x52
#define holy_PXENV_STATUS_DHCP_NO_BOOTFILE_NAME			0x53
#define holy_PXENV_STATUS_DHCP_BAD_IP_ADDRESS			0x54
#define holy_PXENV_STATUS_UNDI_INVALID_FUNCTION			0x60
#define holy_PXENV_STATUS_UNDI_MEDIATEST_FAILED			0x61
#define holy_PXENV_STATUS_UNDI_CANNOT_INIT_NIC_FOR_MCAST	0x62
#define holy_PXENV_STATUS_UNDI_CANNOT_INITIALIZE_NIC		0x63
#define holy_PXENV_STATUS_UNDI_CANNOT_INITIALIZE_PHY		0x64
#define holy_PXENV_STATUS_UNDI_CANNOT_READ_CONFIG_DATA		0x65
#define holy_PXENV_STATUS_UNDI_CANNOT_READ_INIT_DATA		0x66
#define holy_PXENV_STATUS_UNDI_BAD_MAC_ADDRESS			0x67
#define holy_PXENV_STATUS_UNDI_BAD_EEPROM_CHECKSUM		0x68
#define holy_PXENV_STATUS_UNDI_ERROR_SETTING_ISR		0x69
#define holy_PXENV_STATUS_UNDI_INVALID_STATE			0x6A
#define holy_PXENV_STATUS_UNDI_TRANSMIT_ERROR			0x6B
#define holy_PXENV_STATUS_UNDI_INVALID_PARAMETER		0x6C
#define holy_PXENV_STATUS_BSTRAP_PROMPT_MENU			0x74
#define holy_PXENV_STATUS_BSTRAP_MCAST_ADDR			0x76
#define holy_PXENV_STATUS_BSTRAP_MISSING_LIST			0x77
#define holy_PXENV_STATUS_BSTRAP_NO_RESPONSE			0x78
#define holy_PXENV_STATUS_BSTRAP_FILE_TOO_BIG			0x79
#define holy_PXENV_STATUS_BINL_CANCELED_BY_KEYSTROKE		0xA0
#define holy_PXENV_STATUS_BINL_NO_PXE_SERVER			0xA1
#define holy_PXENV_STATUS_NOT_AVAILABLE_IN_PMODE		0xA2
#define holy_PXENV_STATUS_NOT_AVAILABLE_IN_RMODE		0xA3
#define holy_PXENV_STATUS_BUSD_DEVICE_NOT_SUPPORTED		0xB0
#define holy_PXENV_STATUS_LOADER_NO_FREE_BASE_MEMORY		0xC0
#define holy_PXENV_STATUS_LOADER_NO_BC_ROMID			0xC1
#define holy_PXENV_STATUS_LOADER_BAD_BC_ROMID			0xC2
#define holy_PXENV_STATUS_LOADER_BAD_BC_RUNTIME_IMAGE		0xC3
#define holy_PXENV_STATUS_LOADER_NO_UNDI_ROMID			0xC4
#define holy_PXENV_STATUS_LOADER_BAD_UNDI_ROMID			0xC5
#define holy_PXENV_STATUS_LOADER_BAD_UNDI_DRIVER_IMAGE		0xC6
#define holy_PXENV_STATUS_LOADER_NO_PXE_STRUCT			0xC8
#define holy_PXENV_STATUS_LOADER_NO_PXENV_STRUCT		0xC9
#define holy_PXENV_STATUS_LOADER_UNDI_START			0xCA
#define holy_PXENV_STATUS_LOADER_BC_START			0xCB

#define holy_PXENV_PACKET_TYPE_DHCP_DISCOVER	1
#define holy_PXENV_PACKET_TYPE_DHCP_ACK		2
#define holy_PXENV_PACKET_TYPE_CACHED_REPLY	3

#define holy_PXE_BOOTP_REQ	1
#define holy_PXE_BOOTP_REP	2

#define holy_PXE_BOOTP_BCAST	0x8000

#if 1
#define holy_PXE_BOOTP_SIZE	(1024 + 236)	/* DHCP extended vendor field size.  */
#else
#define holy_PXE_BOOTP_SIZE	(312 + 236)	/* DHCP standard vendor field size.  */
#endif

#define holy_PXE_TFTP_PORT	69

#define holy_PXE_ERR_LEN	0xFFFFFFFF

#ifndef ASM_FILE

#define holy_PXE_SIGNATURE "PXENV+"

struct holy_pxenv
{
  holy_uint8_t signature[6];	/* 'PXENV+'.  */
  holy_uint16_t version;	/* MSB = major, LSB = minor.  */
  holy_uint8_t length;		/* structure length.  */
  holy_uint8_t checksum;	/* checksum pad.  */
  holy_uint32_t rm_entry;	/* SEG:OFF to PXE entry point.  */
  holy_uint32_t	pm_offset;	/* Protected mode entry.  */
  holy_uint16_t pm_selector;	/* Protected mode selector.  */
  holy_uint16_t stack_seg;	/* Stack segment address.  */
  holy_uint16_t	stack_size;	/* Stack segment size (bytes).  */
  holy_uint16_t bc_code_seg;	/* BC Code segment address.  */
  holy_uint16_t	bc_code_size;	/* BC Code segment size (bytes).  */
  holy_uint16_t	bc_data_seg;	/* BC Data segment address.  */
  holy_uint16_t	bc_data_size;	/* BC Data segment size (bytes).  */
  holy_uint16_t	undi_data_seg;	/* UNDI Data segment address.  */
  holy_uint16_t	undi_data_size;	/* UNDI Data segment size (bytes).  */
  holy_uint16_t	undi_code_seg;	/* UNDI Code segment address.  */
  holy_uint16_t	undi_code_size;	/* UNDI Code segment size (bytes).  */
  holy_uint32_t pxe_ptr;	/* SEG:OFF to !PXE struct.  */
} holy_PACKED;

struct holy_pxe_bangpxe
{
  holy_uint8_t signature[4];
#define holy_PXE_BANGPXE_SIGNATURE "!PXE"
  holy_uint8_t length;
  holy_uint8_t chksum;
  holy_uint8_t rev;
  holy_uint8_t reserved;
  holy_uint32_t undiromid;
  holy_uint32_t baseromid;
  holy_uint32_t rm_entry;
} holy_PACKED;

struct holy_pxenv_get_cached_info
{
  holy_uint16_t status;
  holy_uint16_t packet_type;
  holy_uint16_t buffer_size;
  holy_uint32_t buffer;
  holy_uint16_t buffer_limit;
} holy_PACKED;

struct holy_pxenv_tftp_open
{
  holy_uint16_t status;
  holy_uint32_t server_ip;
  holy_uint32_t gateway_ip;
  holy_uint8_t filename[128];
  holy_uint16_t tftp_port;
  holy_uint16_t packet_size;
} holy_PACKED;

struct holy_pxenv_tftp_close
{
  holy_uint16_t status;
} holy_PACKED;

struct holy_pxenv_tftp_read
{
  holy_uint16_t status;
  holy_uint16_t packet_number;
  holy_uint16_t buffer_size;
  holy_uint32_t buffer;
} holy_PACKED;

struct holy_pxenv_tftp_get_fsize
{
  holy_uint16_t status;
  holy_uint32_t server_ip;
  holy_uint32_t gateway_ip;
  holy_uint8_t filename[128];
  holy_uint32_t file_size;
} holy_PACKED;

struct holy_pxenv_udp_open
{
  holy_uint16_t status;
  holy_uint32_t src_ip;
} holy_PACKED;

struct holy_pxenv_udp_close
{
  holy_uint16_t status;
} holy_PACKED;

struct holy_pxenv_udp_write
{
  holy_uint16_t status;
  holy_uint32_t ip;
  holy_uint32_t gateway;
  holy_uint16_t src_port;
  holy_uint16_t dst_port;
  holy_uint16_t buffer_size;
  holy_uint32_t buffer;
} holy_PACKED;

struct holy_pxenv_udp_read
{
  holy_uint16_t status;
  holy_uint32_t src_ip;
  holy_uint32_t dst_ip;
  holy_uint16_t src_port;
  holy_uint16_t dst_port;
  holy_uint16_t buffer_size;
  holy_uint32_t buffer;
} holy_PACKED;

struct holy_pxenv_unload_stack
{
  holy_uint16_t status;
  holy_uint8_t reserved[10];
} holy_PACKED;

int EXPORT_FUNC(holy_pxe_call) (int func, void * data, holy_uint32_t pxe_rm_entry) __attribute__ ((regparm(3)));

extern struct holy_pxe_bangpxe *holy_pxe_pxenv;

void *
holy_pxe_get_cached (holy_uint16_t type);

#endif

#endif /* holy_CPU_PXE_H */
