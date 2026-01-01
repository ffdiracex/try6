/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CS5536_HEADER
#define holy_CS5536_HEADER 1

#ifndef ASM_FILE
#include <holy/pci.h>
#include <holy/err.h>
#include <holy/smbus.h>
#endif

#define holy_CS5536_PCIID 0x208f1022
#define holy_CS5536_MSR_MAILBOX_CONFIG_ENABLED        0x1
#define holy_CS5536_MSR_MAILBOX_CONFIG 0xf0
#define holy_CS5536_MSR_MAILBOX_ADDR   0xf4
#define holy_CS5536_MSR_MAILBOX_DATA0  0xf8
#define holy_CS5536_MSR_MAILBOX_DATA1  0xfc
#define holy_CS5536_MSR_IRQ_MAP_BAR 0x80000008
#define holy_CS5536_MSR_SMB_BAR 0x8000000b

#define holy_CS5536_SMBUS_REGS_SIZE 8
#define holy_CS5536_GPIO_REGS_SIZE 256
#define holy_CS5536_MFGPT_REGS_SIZE 64
#define holy_CS5536_IRQ_MAP_REGS_SIZE 32
#define holy_CS5536_PM_REGS_SIZE 128
#define holy_CS5536_ACPI_REGS_SIZE 32

#define holy_CS5536_USB_OPTION_REGS_SIZE 0x1c
#define holy_CS5536_USB_OPTION_REG_UOCMUX  1
#define holy_CS5536_USB_OPTION_REG_UOCMUX_PMUX_MASK 0x03
#define holy_CS5536_USB_OPTION_REG_UOCMUX_PMUX_HC   0x02

#define holy_CS5536_DESTINATION_GLIU     0
#define holy_CS5536_DESTINATION_GLPCI_SB 1
#define holy_CS5536_DESTINATION_USB      2
#define holy_CS5536_DESTINATION_IDE      3
#define holy_CS5536_DESTINATION_DD       4
#define holy_CS5536_DESTINATION_ACC      5
#define holy_CS5536_DESTINATION_GLCP     7

#define holy_CS5536_P2D_DEST_SHIFT    61
#define holy_CS5536_P2D_LOG_ALIGN  12
#define holy_CS5536_P2D_ALIGN  (1 << holy_CS5536_P2D_LOG_ALIGN)
#define holy_CS5536_P2D_BASE_SHIFT 20
#define holy_CS5536_P2D_MASK_SHIFT 0

#define holy_CS5536_MSR_GL_IOD_START 0x000100e0
#define holy_CS5536_IOD_DEST_SHIFT    61
#define holy_CS5536_IOD_BASE_SHIFT    20
#define holy_CS5536_IOD_MASK_SHIFT 0
#define holy_CS5536_IOD_ADDR_MASK     0xfffff

#define holy_CS5536_MSR_GPIO_BAR 0x8000000c
#define holy_CS5536_MSR_MFGPT_BAR 0x8000000d
#define holy_CS5536_MSR_ACPI_BAR 0x8000000e
#define holy_CS5536_MSR_PM_BAR 0x8000000f
#define holy_CS5536_MSR_DIVIL_LEG_IO 0x80000014
#define holy_CS5536_MSR_DIVIL_LEG_IO_RTC_ENABLE0 0x00000001
#define holy_CS5536_MSR_DIVIL_LEG_IO_RTC_ENABLE1 0x00000002
#define holy_CS5536_MSR_DIVIL_LEG_IO_MODE_X86    0x10000000
#define holy_CS5536_MSR_DIVIL_LEG_IO_F_REMAP     0x04000000
#define holy_CS5536_MSR_DIVIL_LEG_IO_UART1_COM1  0x00070000
#define holy_CS5536_MSR_DIVIL_LEG_IO_UART2_COM3  0x00500000
#define holy_CS5536_MSR_DIVIL_RESET 0x80000017
#define holy_CS5536_MSR_DIVIL_IRQ_MAPPER_PRIMARY_MASK 0x80000024
#define holy_CS5536_MSR_DIVIL_IRQ_MAPPER_LPC_MASK 0x80000025
#define holy_CS5536_DIVIL_LPC_INTERRUPTS 0x1002
#define holy_CS5536_MSR_DIVIL_LPC_SERIAL_IRQ_CONTROL 0x8000004e
#define holy_CS5536_MSR_DIVIL_LPC_SERIAL_IRQ_CONTROL_ENABLE 0x80
#define holy_CS5536_MSR_DIVIL_UART1_CONF 0x8000003a
#define holy_CS5536_MSR_DIVIL_UART2_CONF 0x8000003e

#define holy_CS5536_MSR_USB_OHCI_BASE 0x40000008
#define holy_CS5536_MSR_USB_EHCI_BASE 0x40000009
#define holy_CS5536_MSR_USB_CONTROLLER_BASE 0x4000000a
#define holy_CS5536_MSR_USB_OPTION_CONTROLLER_BASE 0x4000000b
#define holy_CS5536_MSR_USB_BASE_ADDR_MASK     0x00ffffff00ULL
#define holy_CS5536_MSR_USB_BASE_SMI_ENABLE    0x3f000000000000ULL
#define holy_CS5536_MSR_USB_BASE_BUS_MASTER    0x0400000000ULL
#define holy_CS5536_MSR_USB_BASE_MEMORY_ENABLE 0x0200000000ULL
#define holy_CS5536_MSR_USB_BASE_PME_ENABLED       0x0800000000ULL
#define holy_CS5536_MSR_USB_BASE_PME_STATUS        0x1000000000ULL
#define holy_CS5536_MSR_USB_EHCI_BASE_FLDJ_SHIFT 40

#define holy_CS5536_MSR_IDE_IO_BAR 0x60000008
#define holy_CS5536_MSR_IDE_IO_BAR_UNITS 1
#define holy_CS5536_MSR_IDE_IO_BAR_ADDR_MASK 0xfffffff0
#define holy_CS5536_MSR_IDE_CFG 0x60000010
#define holy_CS5536_MSR_IDE_CFG_CHANNEL_ENABLE 2
#define holy_CS5536_MSR_IDE_TIMING 0x60000012
#define holy_CS5536_MSR_IDE_TIMING_PIO0 0x98
#define holy_CS5536_MSR_IDE_TIMING_DRIVE0_SHIFT 24
#define holy_CS5536_MSR_IDE_TIMING_DRIVE1_SHIFT 16
#define holy_CS5536_MSR_IDE_CAS_TIMING 0x60000013
#define holy_CS5536_MSR_IDE_CAS_TIMING_CMD_PIO0 0x99
#define holy_CS5536_MSR_IDE_CAS_TIMING_CMD_SHIFT 24
#define holy_CS5536_MSR_IDE_CAS_TIMING_DRIVE0_SHIFT 6
#define holy_CS5536_MSR_IDE_CAS_TIMING_DRIVE1_SHIFT 4
#define holy_CS5536_MSR_IDE_CAS_TIMING_PIO0 2

#define holy_CS5536_MSR_GL_PCI_CTRL 0x00000010
#define holy_CS5536_MSR_GL_PCI_CTRL_MEMORY_ENABLE 1
#define holy_CS5536_MSR_GL_PCI_CTRL_IO_ENABLE 2
#define holy_CS5536_MSR_GL_PCI_CTRL_LATENCY_SHIFT 35
#define holy_CS5536_MSR_GL_PCI_CTRL_OUT_THR_SHIFT 60
#define holy_CS5536_MSR_GL_PCI_CTRL_IN_THR_SHIFT 56

#define holy_CS5536_MSR_GL_REGIONS_START 0x00000020
#define holy_CS5536_MSR_GL_REGIONS_NUM   16
#define holy_CS5536_MSR_GL_REGION_ENABLE 1
#define holy_CS5536_MSR_GL_REGION_IO        0x100000000ULL
#define holy_CS5536_MSR_GL_REGION_BASE_MASK 0xfffff000ULL
#define holy_CS5536_MSR_GL_REGION_IO_BASE_SHIFT 12
#define holy_CS5536_MSR_GL_REGION_TOP_MASK 0xfffff00000000000ULL
#define holy_CS5536_MSR_GL_REGION_IO_TOP_SHIFT 44

#define holy_CS5536_MSR_GL_P2D_START 0x00010020

#define holy_CS5536_SMB_REG_DATA 0x0
#define holy_CS5536_SMB_REG_STATUS 0x1
#define holy_CS5536_SMB_REG_STATUS_SDAST (1 << 6)
#define holy_CS5536_SMB_REG_STATUS_BER (1 << 5)
#define holy_CS5536_SMB_REG_STATUS_NACK (1 << 4)
#define holy_CS5536_SMB_REG_CTRL1 0x3
#define holy_CS5536_SMB_REG_CTRL1_START 0x01
#define holy_CS5536_SMB_REG_CTRL1_STOP  0x02
#define holy_CS5536_SMB_REG_CTRL1_ACK   0x10
#define holy_CS5536_SMB_REG_ADDR 0x4
#define holy_CS5536_SMB_REG_ADDR_MASTER 0x0
#define holy_CS5536_SMB_REG_CTRL2 0x5
#define holy_CS5536_SMB_REG_CTRL2_ENABLE 0x1
#define holy_CS5536_SMB_REG_CTRL3 0x6

#ifdef ASM_FILE
#define holy_ULL(x) x
#else
#define holy_ULL(x) x ## ULL
#endif

#define holy_CS5536_LBAR_ADDR_MASK holy_ULL (0x000000000000fff8)
#define holy_CS5536_LBAR_ENABLE holy_ULL (0x0000000100000000)
#define holy_CS5536_LBAR_MASK_MASK holy_ULL (0x0000f00000000000)
#define holy_CS5536_LBAR_TURN_ON (holy_CS5536_LBAR_ENABLE | holy_CS5536_LBAR_MASK_MASK)

/* PMON-compatible LBARs.  */
#define holy_CS5536_LBAR_GPIO      0xb000
#define holy_CS5536_LBAR_ACC       0xb200
#define holy_CS5536_LBAR_PM        0xb280
#define holy_CS5536_LBAR_MFGPT     0xb300
#define holy_CS5536_LBAR_ACPI      0xb340
#define holy_CS5536_LBAR_IRQ_MAP   0xb360
#define holy_CS5536_LBAR_IDE       0xb380
#define holy_CS5536_LBAR_SMBUS     0xb390

#define holy_GPIO_SMBUS_PINS ((1 << 14) | (1 << 15))
#define holy_GPIO_REG_OUT_EN 0x4
#define holy_GPIO_REG_OUT_AUX1 0x10
#define holy_GPIO_REG_IN_EN 0x20
#define holy_GPIO_REG_IN_AUX1 0x34

#ifndef ASM_FILE
int EXPORT_FUNC (holy_cs5536_find) (holy_pci_device_t *devp);

holy_uint64_t EXPORT_FUNC (holy_cs5536_read_msr) (holy_pci_device_t dev,
						  holy_uint32_t addr);
void EXPORT_FUNC (holy_cs5536_write_msr) (holy_pci_device_t dev,
					  holy_uint32_t addr,
					  holy_uint64_t val);
holy_err_t holy_cs5536_read_spd_byte (holy_port_t smbbase, holy_uint8_t dev,
				      holy_uint8_t addr, holy_uint8_t *res);
holy_err_t EXPORT_FUNC (holy_cs5536_read_spd) (holy_port_t smbbase,
					       holy_uint8_t dev,
					       struct holy_smbus_spd *res);
holy_err_t holy_cs5536_smbus_wait (holy_port_t smbbase);
holy_err_t EXPORT_FUNC (holy_cs5536_init_smbus) (holy_pci_device_t dev,
						 holy_uint16_t divisor,
						 holy_port_t *smbbase);

void holy_cs5536_init_geode (holy_pci_device_t dev);
#endif

#endif
