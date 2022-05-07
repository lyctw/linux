/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2014-2016 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#ifndef __KENDRYTE_VO_REG_H__
#define __KENDRYTE_VO_REG_H__

/*
 * VO Registers
 */
#define MASK(x)				(BIT(x) - 1)

#define VO_CTRL			0x0004
#define FRM_END_START_OFST		0
#define FRM_END_START_MASK		MASK(2)
#define AUTO_CLK_GATE_EN_OFST		0
#define AUTO_CLK_GATE_EN		BIT(0)
#define VO_DISP_SRC_CFG		0x0018
#define VO_CTRL1			0x008C
#define VO_EN				0x0100
#define VO_DISABLE			0
#define VO_ENABLE			1
/* reset and reload regs */
#define VO_SOFT_RST_SEL(x)		(0x0078 + (x) * 0x4)
#define VO_RELOAD_DIS(x)		(0x00AC + (x) * 0x4)
#define RDMA_OFST			0
#define CLIP_OFST			15
#define SCL_OFST			21
#define CTRAN_OFST			24
#define OVLY_OFST			37 /* 32+5 */
/* channel regs */
#define RD_CH_CTRL(x)			(0x1004 + (x) * 0x80)
#define RD_CH_ADDR(x)			(0x1008 + (x) * 0x80)
#define RD_CH_SIZE(x)			(0x100C + (x) * 0x80)
#define RD_CH_STRIDE(x)			(0x1010 + (x) * 0x80)
#define RD_CH_SPACE(x)			(0x1014 + (x) * 0x80)
#define RD_CH_EN(x)			(0x1020 + (x) * 0x80)
/* overlay regs */
#define VO_OVLY1_TRANS_CFG		0x002C
#define VO_OVLY_CTL			0x0098
#define VO_OVLY_CH_XY0(x)		(0x2004 + (x) * 4)
#define VO_OVLY_CH_XY1(x)		(0x2024 + (x) * 4)
#define VO_OVLY_CH_CTL(x)		(0x204C + (x) * 4)
#define VO_OVLY_OUTPUT_SIZE(x)		(0x2070 + (x) * 8)
#define OUTPUT_XSIZE_OFST		16
#define VO_OVLYX_CTL(x)		(0x209C + (x) * 4)
#define CH_OVLY_SEL_OFST(x)		((x) * 4)
#define CH_OVLY_SEL_MASK		MASK(2)
#define CH_OVLY_SEL_VAL(x)		((x) + 1)
#define CH_ALP_MODE_OFST		0
#define CH_ALP_SEL_OFST			2
#define CH_UNDER_ALP_SEL_OFST		4
#define CH_EN_OFST			6
#define CH_ALP_GBL_OFST			15
#define CH_SEL_OFST			28
/* ctran regs */
#define VO_CTRAN_DIS(x)		(0x5004 + (x) * 0x100)
#define CTRAN_BYPASS_ON			1
#define CTRAN_BYPASS_OFF		0
#define VO_CTRAN_IMAGE_SIZE(x)		(0x503C + (x) * 0x100)
/* clip regs */
#define VO_CLIP_DISABLE(x)		(0x6800 + (x) * 0x100)
#define VO_CLIP_SIZE0(x)		(0x6804 + (x) * 0x100)
#define VO_CLIP_SIZE1(x)		(0x6808 + (x) * 0x100)

/*
 * LDI Registers
 */
#define LDI_HRZ_CTRL0			0x7400
#define HBP_OFST			20
#define LDI_HRZ_CTRL1			0x7404
#define LDI_VRT_CTRL0			0x7408
#define VBP_OFST			20
#define LDI_VRT_CTRL1			0x740C
#define LDI_PLR_CTRL			0x7410
#define FLAG_NVSYNC			BIT(0)
#define FLAG_NHSYNC			BIT(1)
#define FLAG_NPIXCLK			BIT(2)
#define FLAG_NDE			BIT(3)
#define LDI_DSP_SIZE			0x7414
#define VSIZE_OFST			20
#define LDI_INT_EN			0x741C
#define FRAME_END_INT_EN_OFST		1
#define LDI_CTRL			0x7420
#define BPP_OFST			3
#define DATA_GATE_EN			BIT(2)
#define LDI_EN				BIT(0)
#define LDI_MSK_INT			0x7428
#define LDI_INT_CLR			0x742C
#define LDI_WORK_MODE			0x7430
#define LDI_HDMI_DSI_GT			0x7434

/*
 * VO media bus service regs
 */
#define VO0_QOSGENERATOR_MODE		0x010C
#define QOSGENERATOR_MODE_MASK		MASK(2)
#define VO0_QOSGENERATOR_EXTCONTROL	0x0118
#define SOCKET_QOS_EN			BIT(0)
#define VO1_QOSGENERATOR_MODE		0x020C
#define VO1_QOSGENERATOR_EXTCONTROL	0x0218

/*
 * VO regs relevant enums
 */
enum frame_end_start {
	/* regs take effect in every vsync */
	REG_EFFECTIVE_IN_VSYNC = 0,
	/* regs take effect in fist vo en and every frame end */
	REG_EFFECTIVE_IN_VOEN_FRMEND,
	/* regs take effect in vo en immediately */
	REG_EFFECTIVE_IN_VOEN,
	/* regs take effect in first vsync and every frame end */
	REG_EFFECTIVE_IN_VSYNC_FRMEND
};

enum vo_fb_format {
	VO_RGB_565 = 0,
	VO_BGR_565,
	VO_XRGB_8888,
	VO_XBGR_8888,
	VO_ARGB_8888,
	VO_ABGR_8888,
	VO_RGBA_8888,
	VO_BGRA_8888,
	VO_RGB_888,
	VO_BGR_888 = 9,
	VO_FORMAT_UNSUPPORT = 800
};

enum vo_channel {
	VO_CH1 = 0,	/* channel 1 for primary plane */
	VO_CH_NUM
};

enum vo_scale {
	VO_SCL1 = 0,
	VO_SCL2,
	VO_SCL3,
	VO_SCL_NUM
};

enum vo_ctran {
	VO_CTRAN1 = 0,
	VO_CTRAN2,
	VO_CTRAN3,
	VO_CTRAN4,
	VO_CTRAN5,
	VO_CTRAN6,
	VO_CTRAN_NUM
};

enum vo_overlay {
	VO_OVLY1 = 0,
	VO_OVLY2,
	VO_OVLY3,
	VO_OVLY_NUM
};

enum vo_alpha_mode {
	VO_ALP_GLOBAL = 0,
	VO_ALP_PIXEL,
	VO_ALP_PIXEL_AND_GLB
};

enum vo_alpha_blending_mode {
	VO_ALP_MUL_COEFF_0 = 0,	/* alpha */
	VO_ALP_MUL_COEFF_1,		/* 1-alpha */
	VO_ALP_MUL_COEFF_2,		/* 0 */
	VO_ALP_MUL_COEFF_3		/* 1 */
};

/*
 * LDI regs relevant enums
 */
enum dsi_pclk_en {
	DSI_PCLK_ON = 0,
	DSI_PCLK_OFF
};

enum ldi_output_format {
	LDI_OUT_RGB_565 = 0,
	LDI_OUT_RGB_666,
	LDI_OUT_RGB_888
};

enum ldi_work_mode {
	TEST_MODE = 0,
	NORMAL_MODE
};

enum ldi_input_source {
	DISP_SRC_NONE = 0,
	DISP_SRC_OVLY2,
	DISP_SRC_DISP,
	DISP_SRC_ROT,
	DISP_SRC_SCL2
};

/*
 * VO media bus service relevant enums
 */
enum qos_generator_mode {
	FIXED_MODE = 0,
	LIMITER_MODE,
	BYPASS_MODE,
	REGULATOR_MODE
};

/*
 * Register Write/Read Helper functions
 */
static inline void vo_update_bits(void __iomem *addr, u32 bit_start,
				   u32 mask, u32 val)
{
	u32 tmp, orig;

	orig = readl(addr);
	tmp = orig & ~(mask << bit_start);
	tmp |= (val & mask) << bit_start;
	writel(tmp, addr);
}

#endif

