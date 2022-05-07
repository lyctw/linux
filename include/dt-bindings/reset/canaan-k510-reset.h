/*
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright (c) 2016 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * The full GNU General Public License is included in this distribution
 * in the file called COPYING.
 *
 * BSD LICENSE
 *
 * Copyright (c) 2016 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _DT_BINDINGS_CANAAN_K510_RESET_H_
#define _DT_BINDINGS_CANAAN_K510_RESET_H_

/* reset register offset */
#define K510_RESET_REG_OFFSET_SHIFT     16
#define K510_RESET_REG_OFFSET_MASK      0xffff0000

/* reset type */
#define K510_RESET_TYPE_SHIFT           14
#define K510_RESET_TYPE_MASK            0x0000c000

#define K510_RESET_TYPE_DSP             0
#define K510_RESET_TYPE_HW_AUTO_DONE    1
#define K510_RESET_TYPE_SW_SET_DONE     2

/* reset done bit */
#define K510_RESET_DONE_BIT_SHIFT       7
#define K510_RESET_DONE_BIT_MASK        0x00003F80

/* reset assert&deassert bit */
#define K510_RESET_ASSERT_BIT_SHIFT     0
#define K510_RESET_ASSERT_BIT_MASK      0x0000007F


/* dsp reset */
#define K510_RESET_DSP_REG_OFFSET       0x14
#define K510_RESET_DSP_TYPE             K510_RESET_TYPE_DSP
#define K510_RESET_DSP_DONE_BIT         30
#define K510_RESET_DSP_ASSERT_BIT       0

/* dsp lm reset */
#define K510_RESET_DSP_LM_REG_OFFSET    0x14
#define K510_RESET_DSP_LM_TYPE          K510_RESET_TYPE_DSP
#define K510_RESET_DSP_LM_DONE_BIT      31
#define K510_RESET_DSP_LM_ASSERT_BIT    1

/* gnne reset */
#define K510_RESET_GNNE_REG_OFFSET      0x2c
#define K510_RESET_GNNE_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_GNNE_DONE_BIT        31
#define K510_RESET_GNNE_ASSERT_BIT      0

/* uart0 reset */
#define K510_RESET_UART0_REG_OFFSET     0x58
#define K510_RESET_UART0_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_UART0_DONE_BIT       31
#define K510_RESET_UART0_ASSERT_BIT     0

/* uart1 reset */
#define K510_RESET_UART1_REG_OFFSET     0x5c
#define K510_RESET_UART1_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_UART1_DONE_BIT       31
#define K510_RESET_UART1_ASSERT_BIT     0

/* uart2 reset */
#define K510_RESET_UART2_REG_OFFSET     0x60
#define K510_RESET_UART2_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_UART2_DONE_BIT       31
#define K510_RESET_UART2_ASSERT_BIT     0

/* uart3 reset */
#define K510_RESET_UART3_REG_OFFSET     0x64
#define K510_RESET_UART3_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_UART3_DONE_BIT       31
#define K510_RESET_UART3_ASSERT_BIT     0

/* i2ss reset */
#define K510_RESET_I2SS_REG_OFFSET      0x6c
#define K510_RESET_I2SS_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_I2SS_DONE_BIT        31
#define K510_RESET_I2SS_ASSERT_BIT      0

/* spi0 reset */
#define K510_RESET_SPI0_REG_OFFSET      0x74
#define K510_RESET_SPI0_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SPI0_DONE_BIT        31
#define K510_RESET_SPI0_ASSERT_BIT      0

/* spi1 reset */
#define K510_RESET_SPI1_REG_OFFSET      0x78
#define K510_RESET_SPI1_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SPI1_DONE_BIT        31
#define K510_RESET_SPI1_ASSERT_BIT      0

/* spi2 reset */
#define K510_RESET_SPI2_REG_OFFSET      0x7c
#define K510_RESET_SPI2_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SPI2_DONE_BIT        31
#define K510_RESET_SPI2_ASSERT_BIT      0

/* spi3 reset */
#define K510_RESET_SPI3_REG_OFFSET      0x80
#define K510_RESET_SPI3_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SPI3_DONE_BIT        31
#define K510_RESET_SPI3_ASSERT_BIT      0

/* audioif reset */
#define K510_RESET_AUDIO_REG_OFFSET     0x88
#define K510_RESET_AUDIO_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_AUDIO_DONE_BIT       31
#define K510_RESET_AUDIO_ASSERT_BIT     0

/* sha reset */
#define K510_RESET_SHA_REG_OFFSET       0x94
#define K510_RESET_SHA_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SHA_DONE_BIT         31
#define K510_RESET_SHA_ASSERT_BIT       0

/* AES reset */
#define K510_RESET_AES_REG_OFFSET       0x9c
#define K510_RESET_AES_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_AES_DONE_BIT         31
#define K510_RESET_AES_ASSERT_BIT       0

/* RSA reset */
#define K510_RESET_RSA_REG_OFFSET       0xa4
#define K510_RESET_RSA_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_RSA_DONE_BIT         31
#define K510_RESET_RSA_ASSERT_BIT       0

/* rom reset */
#define K510_RESET_ROM_REG_OFFSET       0xac
#define K510_RESET_ROM_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_ROM_DONE_BIT         31
#define K510_RESET_ROM_ASSERT_BIT       0

/* OTP reset */
#define K510_RESET_OTP_REG_OFFSET       0xb4
#define K510_RESET_OTP_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_OTP_DONE_BIT         31
#define K510_RESET_OTP_ASSERT_BIT       0

/* PUF reset */
#define K510_RESET_PUF_REG_OFFSET       0xbc
#define K510_RESET_PUF_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_PUF_DONE_BIT         31
#define K510_RESET_PUF_ASSERT_BIT       0

/* sdio0 reset */
#define K510_RESET_SDIO0_REG_OFFSET     0xc8
#define K510_RESET_SDIO0_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SDIO0_DONE_BIT       31
#define K510_RESET_SDIO0_ASSERT_BIT     0

/* sdio1 reset */
#define K510_RESET_SDIO1_REG_OFFSET     0xcc
#define K510_RESET_SDIO1_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SDIO1_DONE_BIT       31
#define K510_RESET_SDIO1_ASSERT_BIT     0

/* sdio2 reset */
#define K510_RESET_SDIO2_REG_OFFSET     0xd0
#define K510_RESET_SDIO2_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SDIO2_DONE_BIT       31
#define K510_RESET_SDIO2_ASSERT_BIT     0

/* peridma reset */
#define K510_RESET_PDMA_REG_OFFSET      0xd8
#define K510_RESET_PDMA_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_PDMA_DONE_BIT        31
#define K510_RESET_PDMA_ASSERT_BIT      0

/* sysdma reset */
#define K510_RESET_SDMA_REG_OFFSET      0xdc
#define K510_RESET_SDMA_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SDMA_DONE_BIT        31
#define K510_RESET_SDMA_ASSERT_BIT      0

/* EMAC reset */
#define K510_RESET_EMAC_REG_OFFSET      0xe4
#define K510_RESET_EMAC_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_EMAC_DONE_BIT        31
#define K510_RESET_EMAC_ASSERT_BIT      0

/* sdio slave reset */
#define K510_RESET_SDIOS_REG_OFFSET     0xec
#define K510_RESET_SDIOS_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SDIOS_DONE_BIT       31
#define K510_RESET_SDIOS_ASSERT_BIT     0

/* ddr controller reset */
#define K510_RESET_MCTRL_REG_OFFSET     0xf4
#define K510_RESET_MCTRL_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_MCTRL_DONE_BIT       31
#define K510_RESET_MCTRL_ASSERT_BIT     0

/* sram0 reset */
#define K510_RESET_SRAM0_REG_OFFSET     0x104
#define K510_RESET_SRAM0_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SRAM0_DONE_BIT       31
#define K510_RESET_SRAM0_ASSERT_BIT     0

/* sram1 reset */
#define K510_RESET_SRAM1_REG_OFFSET     0x114
#define K510_RESET_SRAM1_TYPE           K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SRAM1_DONE_BIT       31
#define K510_RESET_SRAM1_ASSERT_BIT     0

/* isp_f4k reset */
#define K510_RESET_ISP_F4K_REG_OFFSET   0x120
#define K510_RESET_ISP_F4K_TYPE         K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_ISP_F4K_DONE_BIT     31
#define K510_RESET_ISP_F4K_ASSERT_BIT   0

/* isp_f2k reset */
#define K510_RESET_ISP_F2K_REG_OFFSET   0x124
#define K510_RESET_ISP_F2K_TYPE         K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_ISP_F2K_DONE_BIT     31
#define K510_RESET_ISP_F2K_ASSERT_BIT   0

/* isp_r2k reset */
#define K510_RESET_ISP_R2K_REG_OFFSET   0x128
#define K510_RESET_ISP_R2K_TYPE         K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_ISP_R2K_DONE_BIT     31
#define K510_RESET_ISP_R2K_ASSERT_BIT   0

/* isp_tof reset */
#define K510_RESET_ISP_TOF_REG_OFFSET   0x12c
#define K510_RESET_ISP_TOF_TYPE         K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_ISP_TOF_DONE_BIT     31
#define K510_RESET_ISP_TOF_ASSERT_BIT   0

/* CSI0 reset */
#define K510_RESET_CSI0_REG_OFFSET      0x134
#define K510_RESET_CSI0_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_CSI0_DONE_BIT        31
#define K510_RESET_CSI0_ASSERT_BIT      0

/* CSI1 reset */
#define K510_RESET_CSI1_REG_OFFSET      0x138
#define K510_RESET_CSI1_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_CSI1_DONE_BIT        31
#define K510_RESET_CSI1_ASSERT_BIT      0

/* CSI2 reset */
#define K510_RESET_CSI2_REG_OFFSET      0x13c
#define K510_RESET_CSI2_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_CSI2_DONE_BIT        31
#define K510_RESET_CSI2_ASSERT_BIT      0

/* CSI3 reset */
#define K510_RESET_CSI3_REG_OFFSET      0x140
#define K510_RESET_CSI3_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_CSI3_DONE_BIT        31
#define K510_RESET_CSI3_ASSERT_BIT      0

/* SENSOR reset */
#define K510_RESET_SENSOR_REG_OFFSET    0x148
#define K510_RESET_SENSOR_TYPE          K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_SENSOR_DONE_BIT      31
#define K510_RESET_SENSOR_ASSERT_BIT    0

/* VI reset */
#define K510_RESET_VI_REG_OFFSET        0x150
#define K510_RESET_VI_TYPE              K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_VI_DONE_BIT          31
#define K510_RESET_VI_ASSERT_BIT        0

/* MFBC reset */
#define K510_RESET_MFBC_REG_OFFSET      0x158
#define K510_RESET_MFBC_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_MFBC_DONE_BIT        31
#define K510_RESET_MFBC_ASSERT_BIT      0

/* DSI reset */
#define K510_RESET_DSI_REG_OFFSET       0x164
#define K510_RESET_DSI_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_DSI_DONE_BIT         31
#define K510_RESET_DSI_ASSERT_BIT       0

/* bt1120 reset */
#define K510_RESET_BT1120_REG_OFFSET    0x16c
#define K510_RESET_BT1120_TYPE          K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_BT1120_DONE_BIT      31
#define K510_RESET_BT1120_ASSERT_BIT    0

/* TWDO reset */
#define K510_RESET_TWDO_REG_OFFSET      0x174
#define K510_RESET_TWDO_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_TWDO_DONE_BIT        31
#define K510_RESET_TWDO_ASSERT_BIT      0

/* VO reset */
#define K510_RESET_VO_REG_OFFSET        0x17c
#define K510_RESET_VO_TYPE              K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_VO_DONE_BIT          31
#define K510_RESET_VO_ASSERT_BIT        0

/* H264 reset */
#define K510_RESET_H264_REG_OFFSET      0x184
#define K510_RESET_H264_TYPE            K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_H264_DONE_BIT        31
#define K510_RESET_H264_ASSERT_BIT      0

/* USB reset */
#define K510_RESET_USB_REG_OFFSET       0x18c
#define K510_RESET_USB_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_USB_DONE_BIT         31
#define K510_RESET_USB_ASSERT_BIT       0

/*MIPI corner reset */
#define K510_RESET_MIPI_CORNER_REG_OFFSET       0x194
#define K510_RESET_MIPI_CORNER_TYPE             K510_RESET_TYPE_HW_AUTO_DONE
#define K510_RESET_MIPI_CORNER_DONE_BIT         31
#define K510_RESET_MIPI_CORNER_ASSERT_BIT       0

/*--------------------software assert and deassert-------------------*/
/* i2c0 reset */
#define K510_RESET_I2C0_REG_OFFSET      0x40
#define K510_RESET_I2C0_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_I2C0_DONE_BIT        0                           //NOT USED
#define K510_RESET_I2C0_ASSERT_BIT      0

/* i2c1 reset */
#define K510_RESET_I2C1_REG_OFFSET      0x40
#define K510_RESET_I2C1_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_I2C1_DONE_BIT        0                           //NOT USED
#define K510_RESET_I2C1_ASSERT_BIT      1

/* i2c2 reset */
#define K510_RESET_I2C2_REG_OFFSET      0x40
#define K510_RESET_I2C2_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_I2C2_DONE_BIT        0                           //NOT USED
#define K510_RESET_I2C2_ASSERT_BIT      2

/* i2c3 reset */
#define K510_RESET_I2C3_REG_OFFSET      0x44
#define K510_RESET_I2C3_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_I2C3_DONE_BIT        0                           //NOT USED
#define K510_RESET_I2C3_ASSERT_BIT      0

/* i2c4 reset */
#define K510_RESET_I2C4_REG_OFFSET      0x44
#define K510_RESET_I2C4_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_I2C4_DONE_BIT        0                           //NOT USED
#define K510_RESET_I2C4_ASSERT_BIT      1

/* i2c5 reset */
#define K510_RESET_I2C5_REG_OFFSET      0x44
#define K510_RESET_I2C5_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_I2C5_DONE_BIT        0                           //NOT USED
#define K510_RESET_I2C5_ASSERT_BIT      2

/* i2c6 reset */
#define K510_RESET_I2C6_REG_OFFSET      0x44
#define K510_RESET_I2C6_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_I2C6_DONE_BIT        0                           //NOT USED
#define K510_RESET_I2C6_ASSERT_BIT      3

/* WDT0 reset */
#define K510_RESET_WDT0_REG_OFFSET      0x40
#define K510_RESET_WDT0_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_WDT0_DONE_BIT        0                           //NOT USED
#define K510_RESET_WDT0_ASSERT_BIT      3

/* WDT1 reset */
#define K510_RESET_WDT1_REG_OFFSET      0x40
#define K510_RESET_WDT1_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_WDT1_DONE_BIT        0                           //NOT USED
#define K510_RESET_WDT1_ASSERT_BIT      4

/* WDT2 reset */
#define K510_RESET_WDT2_REG_OFFSET      0x40
#define K510_RESET_WDT2_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_WDT2_DONE_BIT        0                           //NOT USED
#define K510_RESET_WDT2_ASSERT_BIT      5

/* TIMER reset */
#define K510_RESET_TIMER_REG_OFFSET     0x40
#define K510_RESET_TIMER_TYPE           K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_TIMER_DONE_BIT       0                           //NOT USED
#define K510_RESET_TIMER_ASSERT_BIT     6

/* RTC reset */
#define K510_RESET_RTC_REG_OFFSET       0x40
#define K510_RESET_RTC_TYPE             K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_RTC_DONE_BIT         0                           //NOT USED
#define K510_RESET_RTC_ASSERT_BIT       7

/* GPIO reset */
#define K510_RESET_GPIO_REG_OFFSET      0x40
#define K510_RESET_GPIO_TYPE            K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_GPIO_DONE_BIT        0                           //NOT USED
#define K510_RESET_GPIO_ASSERT_BIT      8

/* IOMUX reset */
#define K510_RESET_IOMUX_REG_OFFSET     0x40
#define K510_RESET_IOMUX_TYPE           K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_IOMUX_DONE_BIT       0                           //NOT USED
#define K510_RESET_IOMUX_ASSERT_BIT     9

/* MAILBOX reset */
#define K510_RESET_MAILBOX_REG_OFFSET   0x40
#define K510_RESET_MAILBOX_TYPE         K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_MAILBOX_DONE_BIT     0                           //NOT USED
#define K510_RESET_MAILBOX_ASSERT_BIT   10

/* PWM reset */
#define K510_RESET_PWM_REG_OFFSET       0x40
#define K510_RESET_PWM_TYPE             K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_PWM_DONE_BIT         0                           //NOT USED
#define K510_RESET_PWM_ASSERT_BIT       11

/* VAD reset */
#define K510_RESET_VAD_REG_OFFSET       0x40
#define K510_RESET_VAD_TYPE             K510_RESET_TYPE_SW_SET_DONE
#define K510_RESET_VAD_DONE_BIT         0                           //NOT USED
#define K510_RESET_VAD_ASSERT_BIT       12

#endif
