/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2014-2016 Hisilicon Limited.
 *
 * Author:
 * 	Canaan Bright Sight Co., Ltd
 *      Xinliang Liu <z.liuxinliang@hisilicon.com>
 *      Xinliang Liu <xinliang.liu@linaro.org>
 *      Xinwei Kong <kong.kongxinwei@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <video/display_timing.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/reset.h>

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>

#include "kendryte_drm_drv.h"
#include "kendryte_vo_reg.h"

#define PRIMARY_CH	VO_CH1 /* primary plane */
#define OUT_OVLY	VO_OVLY2 /* output overlay compositor */
#define VO_DEBUG	1

#define to_vo_crtc(crtc) \
	container_of(crtc, struct vo_crtc, base)

#define to_vo_plane(plane) \
	container_of(plane, struct vo_plane, base)

struct vo_hw_ctx {
	void __iomem  *base;
	struct regmap *noc_regmap;
	struct clk *vo_core_clk;
	struct clk *media_noc_clk;
	struct clk *vo_pix_clk;
	struct reset_control *reset;
	bool power_on;
	int irq;
};

struct vo_crtc {
	struct drm_crtc base;
	struct vo_hw_ctx *ctx;
	bool enable;
	u32 out_format;
};

struct vo_plane {
	struct drm_plane base;
	void *ctx;
	u8 ch; /* channel */
};

struct vo_data {
	struct vo_crtc acrtc;
	struct vo_plane aplane[VO_CH_NUM];
	struct vo_hw_ctx ctx;
};

/* vo-format info: */
struct vo_format {
	u32 pixel_format;
	enum vo_fb_format vo_format;
};

static const struct vo_format vo_formats[] = {
	/* 16bpp RGB: */
	{ DRM_FORMAT_RGB565, VO_RGB_565 },
	{ DRM_FORMAT_BGR565, VO_BGR_565 },
	/* 24bpp RGB: */
	{ DRM_FORMAT_RGB888, VO_RGB_888 },
	{ DRM_FORMAT_BGR888, VO_BGR_888 },
	/* 32bpp [A]RGB: */
	{ DRM_FORMAT_XRGB8888, VO_XRGB_8888 },
	{ DRM_FORMAT_XBGR8888, VO_XBGR_8888 },
	{ DRM_FORMAT_RGBA8888, VO_RGBA_8888 },
	{ DRM_FORMAT_BGRA8888, VO_BGRA_8888 },
	{ DRM_FORMAT_ARGB8888, VO_ARGB_8888 },
	{ DRM_FORMAT_ABGR8888, VO_ABGR_8888 },
};

static const u32 channel_formats1[] = {
	/* channel 1,2,3,4 */
	DRM_FORMAT_RGB565, DRM_FORMAT_BGR565, DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888, DRM_FORMAT_XRGB8888, DRM_FORMAT_XBGR8888,
	DRM_FORMAT_RGBA8888, DRM_FORMAT_BGRA8888, DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ABGR8888
};

/* vo hw function */
#include "VIDEO_SUBSYSTEM.h"
#include "vo_define.h"
int V_Coef[128]= {0x400000,
        0x0,
        0x3ffff8,
        0xfff008,
        0x3fdff0,
        0xfff011,
        0x3fafea,
        0xffe01c,
        0x3f6fe3,
        0xffe027,
        0x3f1fde,
        0xffd033,
        0x3eafd8,
        0xffb040,
        0x3e3fd3,
        0xffa04e,
        0x3dbfcf,
        0xff905d,
        0x3d1fca,
        0xff706c,
        0x3c7fc7,
        0xff507c,
        0x3bcfc3,
        0xff308c,
        0x3b0fc0,
        0xff109d,
        0x3a3fbd,
        0xfef0af,
        0x395fbb,
        0xfec0c1,
        0x387fb9,
        0xfea0d4,
        0x378fb8,
        0xfe80e8,
        0x368fb6,
        0xfe50fb,
        0x357fb5,
        0xfe210f,
        0x346fb4,
        0xfe0124,
        0x334fb4,
        0xfdd139,
        0x322fb4,
        0xfda14e,
        0x30ffb4,
        0xfd8163,
        0x2fcfb4,
        0xfd5179,
        0x2e9fb5,
        0xfd318f,
        0x2d4fb5,
        0xfd01a4,
        0x2c0fb6,
        0xfcd1bb,
        0x2abfb7,
        0xfcb1d1,
        0x296fb9,
        0xfc81e7,
        0x281fba,
        0xfc61fd,
        0x26bfbc,
        0xfc4213,
        0x255fbe,
        0xfc2229,
        0x240fc0,
        0xfc0240,
        0x229fc2,
        0xfbe255,
        0x213fc4,
        0xfbc26b,
        0x1fdfc6,
        0xfba281,
        0x1e7fc8,
        0xfb9296,
        0x1d1fcb,
        0xfb72ab,
        0x1bbfcd,
        0xfb62c0,
        0x1a4fd0,
        0xfb52d4,
        0x18ffd3,
        0xfb52e9,
        0x179fd5,
        0xfb42fc,
        0x163fd8,
        0xfb430f,
        0x14efda,
        0xfb4322,
        0x139fdd,
        0xfb4334,
        0x124fe0,
        0xfb4346,
        0x10ffe2,
        0xfb5357,
        0xfbfe5,
        0xfb6368,
        0xe8fe8,
        0xfb8378,
        0xd4fea,
        0xfb9387,
        0xc1fec,
        0xfbb395,
        0xaffef,
        0xfbd3a3,
        0x9dff1,
        0xfc03b0,
        0x8cff3,
        0xfc33bc,
        0x7cff5,
        0xfc73c7,
        0x6cff7,
        0xfca3d1,
        0x5dff9,
        0xfcf3db,
        0x4effa,
        0xfd33e3,
        0x40ffb,
        0xfd83ea,
        0x33ffd,
        0xfde3f1,
        0x27ffe,
        0xfe33f6,
        0x1cffe,
        0xfea3fa,
        0x11fff,
        0xff03fd,
        0x8fff,
        0xff83ff};




int H_Coef[192] ={0x0,
        0x4000,
        0x0,
        0xf8000000,
        0xf0083fff,
        0xff,
        0xf0000000,
        0xf0113fdf,
        0xff,
        0xea000000,
        0xe01c3faf,
        0xff,
        0xe3000000,
        0xe0273f6f,
        0xff,
        0xde000000,
        0xd0333f1f,
        0xff,
        0xd8000000,
        0xb0403eaf,
        0xff,
        0xd3000000,
        0xa04e3e3f,
        0xff,
        0xcf000000,
        0x905d3dbf,
        0xff,
        0xca000000,
        0x706c3d1f,
        0xff,
        0xc7000000,
        0x507c3c7f,
        0xff,
        0xc3000000,
        0x308c3bcf,
        0xff,
        0xc0000000,
        0x109d3b0f,
        0xff,
        0xbd000000,
        0xf0af3a3f,
        0xfe,
        0xbb000000,
        0xc0c1395f,
        0xfe,
        0xb9000000,
        0xa0d4387f,
        0xfe,
        0xb8000000,
        0x80e8378f,
        0xfe,
        0xb6000000,
        0x50fb368f,
        0xfe,
        0xb5000000,
        0x210f357f,
        0xfe,
        0xb4000000,
        0x124346f,
        0xfe,
        0xb4000000,
        0xd139334f,
        0xfd,
        0xb4000000,
        0xa14e322f,
        0xfd,
        0xb4000000,
        0x816330ff,
        0xfd,
        0xb4000000,
        0x51792fcf,
        0xfd,
        0xb5000000,
        0x318f2e9f,
        0xfd,
        0xb5000000,
        0x1a42d4f,
        0xfd,
        0xb6000000,
        0xd1bb2c0f,
        0xfc,
        0xb7000000,
        0xb1d12abf,
        0xfc,
        0xb9000000,
        0x81e7296f,
        0xfc,
        0xba000000,
        0x61fd281f,
        0xfc,
        0xbc000000,
        0x421326bf,
        0xfc,
        0xbe000000,
        0x2229255f,
        0xfc,
        0xc0000000,
        0x240240f,
        0xfc,
        0xc2000000,
        0xe255229f,
        0xfb,
        0xc4000000,
        0xc26b213f,
        0xfb,
        0xc6000000,
        0xa2811fdf,
        0xfb,
        0xc8000000,
        0x92961e7f,
        0xfb,
        0xcb000000,
        0x72ab1d1f,
        0xfb,
        0xcd000000,
        0x62c01bbf,
        0xfb,
        0xd0000000,
        0x52d41a4f,
        0xfb,
        0xd3000000,
        0x52e918ff,
        0xfb,
        0xd5000000,
        0x42fc179f,
        0xfb,
        0xd8000000,
        0x430f163f,
        0xfb,
        0xda000000,
        0x432214ef,
        0xfb,
        0xdd000000,
        0x4334139f,
        0xfb,
        0xe0000000,
        0x4346124f,
        0xfb,
        0xe2000000,
        0x535710ff,
        0xfb,
        0xe5000000,
        0x63680fbf,
        0xfb,
        0xe8000000,
        0x83780e8f,
        0xfb,
        0xea000000,
        0x93870d4f,
        0xfb,
        0xec000000,
        0xb3950c1f,
        0xfb,
        0xef000000,
        0xd3a30aff,
        0xfb,
        0xf1000000,
        0x3b009df,
        0xfc,
        0xf3000000,
        0x33bc08cf,
        0xfc,
        0xf5000000,
        0x73c707cf,
        0xfc,
        0xf7000000,
        0xa3d106cf,
        0xfc,
        0xf9000000,
        0xf3db05df,
        0xfc,
        0xfa000000,
        0x33e304ef,
        0xfd,
        0xfb000000,
        0x83ea040f,
        0xfd,
        0xfd000000,
        0xe3f1033f,
        0xfd,
        0xfe000000,
        0x33f6027f,
        0xfe,
        0xfe000000,
        0xa3fa01cf,
        0xfe,
        0xff000000,
        0x3fd011f,
        0xff,
        0xff000000,
        0x83ff008f,
        0xff};



int  GammaCoef[256] ={
        0b00000000,
        0b00000001,
        0b00000010,
        0b00000011,
        0b00000100,
        0b00000101,
        0b00000110,
        0b00000111,
        0b00001000,
        0b00001001,
        0b00001010,
        0b00001011,
        0b00001100,
        0b00001101,
        0b00001110,
        0b00001111,
        0b00010000,
        0b00010001,
        0b00010010,
        0b00010011,
        0b00010100,
        0b00010101,
        0b00010110,
        0b00010111,
        0b00011000,
        0b00011001,
        0b00011010,
        0b00011011,
        0b00011100,
        0b00011101,
        0b00011110,
        0b00011111,
        0b00100000,
        0b00100001,
        0b00100010,
        0b00100011,
        0b00100100,
        0b00100101,
        0b00100110,
        0b00100111,
        0b00101000,
        0b00101001,
        0b00101010,
        0b00101011,
        0b00101100,
        0b00101101,
        0b00101110,
        0b00101111,
        0b00110000,
        0b00110001,
        0b00110010,
        0b00110011,
        0b00110100,
        0b00110101,
        0b00110110,
        0b00110111,
        0b00111000,
        0b00111001,
        0b00111010,
        0b00111011,
        0b00111100,
        0b00111101,
        0b00111110,
        0b00111111,
        0b01000000,
        0b01000001,
        0b01000010,
        0b01000011,
        0b01000100,
        0b01000101,
        0b01000110,
        0b01000111,
        0b01001000,
        0b01001001,
        0b01001010,
        0b01001011,
        0b01001100,
        0b01001101,
        0b01001110,
        0b01001111,
        0b01010000,
        0b01010001,
        0b01010010,
        0b01010011,
        0b01010100,
        0b01010101,
        0b01010110,
        0b01010111,
        0b01011000,
        0b01011001,
        0b01011010,
        0b01011011,
        0b01011100,
        0b01011101,
        0b01011110,
        0b01011111,
        0b01100000,
        0b01100001,
        0b01100010,
        0b01100011,
        0b01100100,
        0b01100101,
        0b01100110,
        0b01100111,
        0b01101000,
        0b01101001,
        0b01101010,
        0b01101011,
        0b01101100,
        0b01101101,
        0b01101110,
        0b01101111,
        0b01110000,
        0b01110001,
        0b01110010,
        0b01110011,
        0b01110100,
        0b01110101,
        0b01110110,
        0b01110111,
        0b01111000,
        0b01111001,
        0b01111010,
        0b01111011,
        0b01111100,
        0b01111101,
        0b01111110,
        0b01111111,
        0b10000000,
        0b10000001,
        0b10000010,
        0b10000011,
        0b10000100,
        0b10000101,
        0b10000110,
        0b10000111,
        0b10001000,
        0b10001001,
        0b10001010,
        0b10001011,
        0b10001100,
        0b10001101,
        0b10001110,
        0b10001111,
        0b10010000,
        0b10010001,
        0b10010010,
        0b10010011,
        0b10010100,
        0b10010101,
        0b10010110,
        0b10010111,
        0b10011000,
        0b10011001,
        0b10011010,
        0b10011011,
        0b10011100,
        0b10011101,
        0b10011110,
        0b10011111,
        0b10100000,
        0b10100001,
        0b10100010,
        0b10100011,
        0b10100100,
        0b10100101,
        0b10100110,
        0b10100111,
        0b10101000,
        0b10101001,
        0b10101010,
        0b10101011,
        0b10101100,
        0b10101101,
        0b10101110,
        0b10101111,
        0b10110000,
        0b10110001,
        0b10110010,
        0b10110011,
        0b10110100,
        0b10110101,
        0b10110110,
        0b10110111,
        0b10111000,
        0b10111001,
        0b10111010,
        0b10111011,
        0b10111100,
        0b10111101,
        0b10111110,
        0b10111111,
        0b11000000,
        0b11000001,
        0b11000010,
        0b11000011,
        0b11000100,
        0b11000101,
        0b11000110,
        0b11000111,
        0b11001000,
        0b11001001,
        0b11001010,
        0b11001011,
        0b11001100,
        0b11001101,
        0b11001110,
        0b11001111,
        0b11010000,
        0b11010001,
        0b11010010,
        0b11010011,
        0b11010100,
        0b11010101,
        0b11010110,
        0b11010111,
        0b11011000,
        0b11011001,
        0b11011010,
        0b11011011,
        0b11011100,
        0b11011101,
        0b11011110,
        0b11011111,
        0b11100000,
        0b11100001,
        0b11100010,
        0b11100011,
        0b11100100,
        0b11100101,
        0b11100110,
        0b11100111,
        0b11101000,
        0b11101001,
        0b11101010,
        0b11101011,
        0b11101100,
        0b11101101,
        0b11101110,
        0b11101111,
        0b11110000,
        0b11110001,
        0b11110010,
        0b11110011,
        0b11110100,
        0b11110101,
        0b11110110,
        0b11110111,
        0b11111000,
        0b11111001,
        0b11111010,
        0b11111011,
        0b11111100,
        0b11111101,
        0b11111110,
        0b11111111
};


static void RegWr32(void __iomem *base, u32 addr, u32 data)
{
    u32 addr_offset = addr - ISP_VO_BASE_ADDR;
    printk("RegWr32, vo base 0x%lx, offset = 0x%x, data 0x%x\n", base, addr_offset, data);
    writel(data, base + addr_offset);
}

static u32 RegRd32(void __iomem *base, u32 addr)
{
    u32 addr_offset = addr - ISP_VO_BASE_ADDR;
    return readl(base + addr_offset);
}

void  VoConfigVCoef(void __iomem *base, int a[],int num)
{
    int base_addr= VO_VSCALE_START;
    int i=0;
    //printk("VoConfigVCoef enter, vo base 0x%lx, base_addr = 0x%x\n", base, base_addr);
    for (i =0;i<num; i++)
    {
        RegWr32(base, (base_addr + ((i*2)<<2)),a[i*2]);
        RegWr32(base, (base_addr + ((i*2+1)<<2)),a[i*2+1]);

    }
    printk("VO  VCoef Config done!\n");
}
void  VoConfigHCoef(void __iomem *base, int a[],int num)
{
    int base_addr= VO_HSCALE_START;
    int i=0;
    for (i =0;i<num; i++)
    {
        RegWr32(base, (base_addr + ((i*4)<<2)),a[i*3]);
        RegWr32(base, (base_addr + ((i*4+1)<<2)),a[i*3+1]);
        RegWr32(base, (base_addr + ((i*4+2)<<2)),a[i*3+2]);

    }
    printk("VO  HCoef Config done!\n");
}

void  VoConfigGammaCoef(void __iomem *base, int a[],int num)
{
    int base_addr= VO_GAMMA_START;
    int i=0;
    for (i =0;i<num; i++)
    {
        RegWr32(base, (base_addr + 0x00  +((i)<<2)),a[i]);
        RegWr32(base, (base_addr + 0x400 +((i)<<2)),a[i]);
        RegWr32(base, (base_addr + 0x800 +((i)<<2)),a[i]);

    }
    printk("VO  HCoef Config done!\n");
}

void VoRegWr(void __iomem *base, int addr, int data)
{
    int offset_addr =0;
    offset_addr = VO_CONFIG_WRAP_START  + (addr <<2);
    RegWr32(base, offset_addr,data) ;
}
int VoRegRd(void __iomem *base, int addr)
{
    int offset_addr =0;
    offset_addr = VO_CONFIG_WRAP_START +(addr <<2);
    return(RegRd32(base, offset_addr));
}


void InitDisplay_720p50fps(struct vo_hw_ctx *ctx)
{
//layer0 param  ---video
    int  layer0_hsize_in  = 1280;
    int  layer0_vsize_in  = 720;
    int  layer0_hsize_out = 1280;
    int  layer0_vsize_out = 720;
     int layer0_rd_stride= 2048;
    int  hscale_pstep = (layer0_hsize_in/layer0_hsize_out);
    int  hscale_dstep = ((layer0_hsize_in % layer0_hsize_out) * 65536 / layer0_hsize_out);
    int  vscale_pstep = (layer0_vsize_in/layer0_vsize_out);
    int  vscale_dstep = ((layer0_vsize_in % layer0_vsize_out) * 65536 / layer0_vsize_out);

    // background_hsize = layer0_hsize_out + 0;
    // background_vsize = layer0_vsize_out + 0;
    // h_back_porch = 30;
    // v_back_porch = 5;

    //
     int layer0_hposition = 266;
     int layer0_vposition = 26;


    //
     int layer0_y_baseaddr0  = 0x800000;//0x20000000; //  # y base: 0x800000
     int layer0_y_baseaddr1  = 0x800000;//0x20000000; //  # uv base: 0x900000
     int layer0_uv_baseaddr0 = 0x800000 + 2048*720;//0x20280000; //  # y base: 0x800000
     int layer0_uv_baseaddr1 = 0x800000 + 2048*720;//0x20280000; //  # uv base: 0x900000




//layer1 param  ---video
     int layer1_hsize_out = 320;
     int layer1_vsize_out = 240;
     int layer1_rd_stride= 2048;
     int layer1_y_baseaddr0  = 0x20400000; //  # y base: 0x800000
     int layer1_y_baseaddr1  = 0x20400000; // # uv base: 0x900000
     int layer1_uv_baseaddr0 = 0x20680000; //  # y base: 0x800000
     int layer1_uv_baseaddr1 = 0x20680000; // # uv base: 0x900000
     int layer1_hposition = 198+1400;
     int layer1_vposition = 100;




//layer2 param  ---video
     int layer2_hsize_out = 320;
     int layer2_vsize_out = 240;
     int layer2_rd_stride= 2048;
     int layer2_y_baseaddr0  = 0x20400000; //  # y base: 0x800000
     int layer2_y_baseaddr1  = 0x20400000; // # uv base: 0x900000
     int layer2_uv_baseaddr0 = 0x20680000; //  # y base: 0x800000
     int layer2_uv_baseaddr1 = 0x20680000; // # uv base: 0x900000
     int layer2_hposition = 198+1400;
     int layer2_vposition = 500;



//layer3 param  ---video
     int layer3_hsize_out = 320;
     int layer3_vsize_out = 240;
     int layer3_rd_stride= 2048;
     int layer3_y_baseaddr0  = 0x20400000; //  # y base: 0x800000
     int layer3_y_baseaddr1  = 0x20400000; // # uv base: 0x900000
     int layer3_uv_baseaddr0 = 0x20680000; //  # y base: 0x800000
     int layer3_uv_baseaddr1 = 0x20680000; // # uv base: 0x900000
     int layer3_hposition = 198+1400;
     int layer3_vposition = 800;
    //

//layer4 param --- osd




     int osd0_hsize_out = 240;
     int osd0_vsize_out = 240;
     int osd0_rd_stride= 2048;
     int osd0_vlu_addr0  = 0x20c00000; //  # y base: 0x800000  --argb mode --32bit
     int osd0_vlu_addr1  = 0x20c00000; // # uv base: 0x900000
     int osd0_alp_addr0  = 0x20c00000; //  # y base: 0x800000
     int osd0_alp_addr1  = 0x20c00000; // # uv base: 0x900000
     int osd0_hposition = 198+300;
     int osd0_vposition = 800;




//layer5 param --- osd
     int osd1_hsize_out = 80;
     int osd1_vsize_out = 160;
     int osd1_rd_stride= 2048;
     int osd1_vlu_addr0  = 0x20c00000; //  # y base: 0x800000
     int osd1_vlu_addr1  = 0x20c00000; // # uv base: 0x900000
     int osd1_alp_addr0 = 0x20c00000; //  # y base: 0x800000
     int osd1_alp_addr1 = 0x20c00000; // # uv base: 0x900000
     int osd1_hposition = 198+300;
     int osd1_vposition = 400;




//layer6 param --- osd
     int osd2_hsize_out = 80;
     int osd2_vsize_out = 160;
     int osd2_rd_stride= 2048;
     int osd2_vlu_addr0 = 0x20c00000; //  # y base: 0x800000
     int osd2_vlu_addr1 = 0x20c00000; // # uv base: 0x900000
     int osd2_alp_addr0 = 0x20c00000; //  # y base: 0x800000
     int osd2_alp_addr1 = 0x20c00000; // # uv base: 0x900000
     int osd2_hposition = 198+700;
     int osd2_vposition = 750;

    printk("InitDisplay enter, vo base 0x%lx\n", ctx->base);


     VoConfigVCoef(ctx->base, V_Coef,64);
     VoConfigHCoef(ctx->base, H_Coef,64);

    // TEST exchange ch ID

     //VoRegWr( ctx->base, ADDR_VO_DMA_ID_RD_0,      0x76543289);
     //VoRegWr( ctx->base, ADDR_VO_DMA_ID_RD_1,      0xfedcba10);

    // # bandwidth limit
    // #--------------------
    // # disp layer0 band-width
     VoRegWr( ctx->base, ADDR_VO_LAYER0_LINE0_BD_CTL,      0x701);    // # outstand: 3+1
     VoRegWr( ctx->base, ADDR_VO_LAYER0_LINE1_BD_CTL,      0x701);    // # outstand: 3+1
     VoRegWr( ctx->base, ADDR_VO_LAYER0_LINE2_BD_CTL,      0x701);    // # outstand: 3+1
     VoRegWr( ctx->base, ADDR_VO_LAYER0_LINE3_BD_CTL,      0x701);    // # outstand: 3+1
    // # disp layer1 band-width
     VoRegWr( ctx->base, ADDR_VO_LAYER1_BD_CTL,      0x701);         //  # outstand: 3+1
    // # disp layer2 band-width
     VoRegWr( ctx->base, ADDR_VO_LAYER2_BD_CTL,      0x701);        //   # outstand: 3+1
    // # disp layer3 band-width
     VoRegWr( ctx->base, ADDR_VO_LAYER3_BD_CTL,      0x701);        //   # outstand: 3+1

     VoRegWr( ctx->base, ADDR_VO_LAYER4_BD_CTL,      0x701);        //   # outstand: 3+1

     VoRegWr( ctx->base, ADDR_VO_LAYER5_BD_CTL,      0x701);        //   # outstand: 3+1

     VoRegWr( ctx->base, ADDR_VO_LAYER6_BD_CTL,      0x701);        //   # outstand: 3+1

    // # disp layer0
    // #-------------
     VoRegWr( ctx->base, ADDR_VO_LAYER0_SCALE_BLENTH,        0xf);        //        # burst length = 15+1
    // #VoRegWr( ctx->base, ADDR_VO_LAYER0_IMG_IN_DAT_MODE,     0x9);              # yuv422 mode
     VoRegWr( ctx->base, ADDR_VO_LAYER0_IMG_IN_DAT_MODE,     0x01003108);     //           # yuv420 mode , indian y and uv all 0
     VoRegWr( ctx->base, ADDR_VO_LAYER0_BADDR0_Y,             layer0_y_baseaddr0); //  # y base: 0x800000
     VoRegWr( ctx->base, ADDR_VO_LAYER0_BADDR0_UV,            layer0_uv_baseaddr0); // # uv base: 0x900000
     VoRegWr( ctx->base, ADDR_VO_LAYER0_BADDR1_Y,             layer0_y_baseaddr1); //  # y base: 0x800000
     VoRegWr( ctx->base, ADDR_VO_LAYER0_BADDR1_UV,            layer0_uv_baseaddr1); // # uv base: 0x900000
     VoRegWr( ctx->base, ADDR_VO_LAYER0_IMG_IN_PIX_SIZE,     (((layer0_vsize_in-1) << 16) | (layer0_hsize_in-1))); //  #input pix size
     VoRegWr( ctx->base, ADDR_VO_LAYER0_IMG_OUT_PIX_SIZE,    (((layer0_vsize_out-1) << 16)| (layer0_hsize_out-1))); /// #output pix size
     VoRegWr( ctx->base, ADDR_VO_LAYER0_IMG_IN_SIZE,         (((layer0_vsize_in-1) << 16) | ((layer0_rd_stride/8-1))));// # input word size

     VoRegWr( ctx->base, ADDR_VO_LAYER0_IMG_IN_OFFSET,       0x0);               // # line offset: 0; word offset: 0
     VoRegWr( ctx->base, ADDR_VO_LAYER0_VSCALE_STEP,         (vscale_dstep<<16) | vscale_pstep);
     VoRegWr( ctx->base, ADDR_VO_LAYER0_VSCALE_ST_PSTEP,     0x0);               // # horizontal start phase: 0
     VoRegWr( ctx->base, ADDR_VO_LAYER0_HSCALE_STEP,         (hscale_dstep<<16) | hscale_pstep);
     VoRegWr( ctx->base, ADDR_VO_LAYER0_HSCALE_ST_PSTEP,     0x0);               //  # vertical start phase: 0
     VoRegWr( ctx->base, ADDR_VO_LAYER0_CTL,                 0x1);               //  # enable layer 0

   //  # disp layer1
   //  #--------------
     VoRegWr( ctx->base, ADDR_VO_LAYER1_CTL,             0x21100);        //     # layer1 disable
   //  #VoRegWr( ctx->base, ADDR_VO_LAYER1_CTL,           0x11);                # layer1 enable, yuv422 format
     VoRegWr( ctx->base, ADDR_VO_LAYER1_IMG_IN_BADDR0_Y,  layer1_y_baseaddr0);
     VoRegWr( ctx->base, ADDR_VO_LAYER1_IMG_IN_BADDR0_UV, layer1_uv_baseaddr0);
     VoRegWr( ctx->base, ADDR_VO_LAYER1_IMG_IN_BADDR1_Y,  layer1_y_baseaddr1);
     VoRegWr( ctx->base, ADDR_VO_LAYER1_IMG_IN_BADDR1_UV, layer1_uv_baseaddr1);
     VoRegWr( ctx->base, ADDR_VO_LAYER1_IMG_IN_OFFSET,   0x0);
     VoRegWr( ctx->base, ADDR_VO_LAYER1_IMG_IN_BLENTH,   0xf);
     VoRegWr( ctx->base, ADDR_VO_LAYER1_IMG_IN_SIZE,    ((layer1_vsize_out-1)<<16) + (layer1_rd_stride/8-1)); //x01df00ff);       //   # 512(h) * 32(v)
     VoRegWr( ctx->base, ADDR_VO_LAYER1_IMG_IN_PIX_SIZE, ((layer1_vsize_out-1)<<16) + (layer1_hsize_out-1));//0x01df027f);



    // # disp layer2
   //  #--------------
     VoRegWr( ctx->base, ADDR_VO_LAYER2_CTL,             0x21100);          //   # layer1 disable
   //  #VoRegWr( ctx->base, ADDR_VO_LAYER2_CTL,           0x11);                # layer1 enable, yuv422 format
     VoRegWr( ctx->base, ADDR_VO_LAYER2_IMG_IN_BADDR0_Y,  layer2_y_baseaddr0);
     VoRegWr( ctx->base, ADDR_VO_LAYER2_IMG_IN_BADDR0_UV, layer2_uv_baseaddr0);
     VoRegWr( ctx->base, ADDR_VO_LAYER2_IMG_IN_BADDR1_Y,  layer2_y_baseaddr1);
     VoRegWr( ctx->base, ADDR_VO_LAYER2_IMG_IN_BADDR1_UV, layer2_uv_baseaddr1);
     VoRegWr( ctx->base, ADDR_VO_LAYER2_IMG_IN_OFFSET,   0x0);
     VoRegWr( ctx->base, ADDR_VO_LAYER2_IMG_IN_BLENTH,   0xf);
     VoRegWr( ctx->base, ADDR_VO_LAYER2_IMG_IN_SIZE,    ((layer2_vsize_out-1)<<16) + (layer2_rd_stride/8-1));     //    # 512(h) * 32(v)
     VoRegWr( ctx->base, ADDR_VO_LAYER2_IMG_IN_PIX_SIZE,((layer2_vsize_out-1)<<16) + (layer2_hsize_out-1));


   //  # disp layer3
  //   #--------------
     VoRegWr( ctx->base, ADDR_VO_LAYER3_CTL,             0x21100);          //   # layer1 disable
  //   #VoRegWr( ctx->base, ADDR_VO_LAYER3_CTL,           0x11);                # layer1 enable, yuv422 format
     VoRegWr( ctx->base, ADDR_VO_LAYER3_IMG_IN_BADDR0_Y,  layer3_y_baseaddr0);
     VoRegWr( ctx->base, ADDR_VO_LAYER3_IMG_IN_BADDR0_UV, layer3_uv_baseaddr0);
     VoRegWr( ctx->base, ADDR_VO_LAYER3_IMG_IN_BADDR1_Y,  layer3_y_baseaddr1);
     VoRegWr( ctx->base, ADDR_VO_LAYER3_IMG_IN_BADDR1_UV, layer3_uv_baseaddr1);
     VoRegWr( ctx->base, ADDR_VO_LAYER3_IMG_IN_OFFSET,   0x0);
     VoRegWr( ctx->base, ADDR_VO_LAYER3_IMG_IN_BLENTH,   0xf);
     VoRegWr( ctx->base, ADDR_VO_LAYER3_IMG_IN_SIZE,    ((layer3_vsize_out-1)<<16) + (layer3_rd_stride/8-1));       //   # 512(h) * 32(v)
     VoRegWr( ctx->base, ADDR_VO_LAYER3_IMG_IN_PIX_SIZE,((layer3_vsize_out-1)<<16) + (layer3_hsize_out-1));




  //osd0

     VoRegWr( ctx->base, ADDR_VO_DISP_OSD0_INFO    ,   0x03);// #alpha fix , RGB24
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD0_SIZE    ,  ((osd0_vsize_out)<<16) + (osd0_hsize_out));
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD0_VLU_ADDR0,  osd0_vlu_addr0);
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD0_ALP_ADDR0,  osd0_alp_addr0 );
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD0_VLU_ADDR1,  osd0_vlu_addr1);
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD0_ALP_ADDR1,  osd0_alp_addr1 );
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD0_DMA_CTRL,   0x4f); //#dma_len=3+1, mem_osd addr R
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD0_STRIDE  ,    ((osd0_rd_stride/8)<<16)+(osd0_rd_stride/8)  );


  //osd1
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD1_INFO    ,   0x03); //#alpha fix , RGB24
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD1_SIZE    , ((osd1_vsize_out)<<16) + (osd1_hsize_out));
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD1_VLU_ADDR0,  osd1_vlu_addr0);
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD1_ALP_ADDR0,  osd1_alp_addr0 );
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD1_VLU_ADDR1,  osd1_vlu_addr1);
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD1_ALP_ADDR1,  osd1_alp_addr1 );
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD1_DMA_CTRL,   0x4f); //#dma_len=3+1, mem_osd addr R
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD1_STRIDE  ,   ((osd1_rd_stride/8)<<16)+(osd1_rd_stride/8)  );


  //osd2
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD2_INFO    ,   0x03);// #alpha fix , RGB24
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD2_SIZE    ,   ((osd2_vsize_out)<<16) + (osd2_hsize_out));
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD2_VLU_ADDR0,    osd2_vlu_addr0);
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD2_ALP_ADDR0,    osd2_alp_addr0 );
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD2_VLU_ADDR1,    osd2_vlu_addr1);
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD2_ALP_ADDR1,    osd2_alp_addr1 );
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD2_DMA_CTRL,     0x4f); //#dma_len=3+1, mem_osd addr R
     VoRegWr( ctx->base, ADDR_VO_DISP_OSD2_STRIDE  ,      ((osd2_rd_stride/8)<<16)+(osd2_rd_stride/8)  );

    // # disp mix
   //  #----------
     VoRegWr( ctx->base, ADDR_VO_DISP_MIX_SEL,   0x76543210);               //   # layer0~7 order is 0~7 in blending sequence
     VoRegWr( ctx->base, ADDR_VO_DISP_MIX_LAYER_GLB_EN,  0xff);             //   # layer0~7 use global regiter alpha
     VoRegWr( ctx->base, ADDR_VO_DISP_MIX_LAYER_GLB_ALPHA0,  0xffffffff);    //  # alpha is 127
     VoRegWr( ctx->base, ADDR_VO_DISP_MIX_LAYER_GLB_ALPHA1,  0xffffffff);    //  # alpha is 127
     VoRegWr( ctx->base, ADDR_VO_DISP_BACKGROUND,    0x00ffffff);            //  # back-ground color: blue

 //    # disp csc enable
     VoRegWr( ctx->base, ADDR_VO_DISP_YUV2RGB_CTL, 0x1);

//     # disp gamma enable
     VoRegWr( ctx->base, ADDR_VO_DISP_CLUT_CTL,  0x0);

//     # disp dith enable
     VoRegWr( ctx->base, ADDR_VO_DISP_DITH_CTL,  0x1);

//     # disp ctl
//     #------------
//     # background zone control
     VoRegWr( ctx->base, ADDR_VO_DISP_XZONE_CTL, 0x0609010a);
     VoRegWr( ctx->base, ADDR_VO_DISP_YZONE_CTL, 0x02e9001a);
//      #VoRegWr( ctx->base, ADDR_VO_DISP_XZONE_CTL, ((background_hsize-1)<<16) + ((h_back_porch <<16)|h_back_porch) );  #real display area
//      #VoRegWr( ctx->base, ADDR_VO_DISP_YZONE_CTL, ((background_vsize-1)<<16) + ((v_back_porch <<16)|v_back_porch) );



//       #layer 0 zone contorl
     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER0_XCTL,(((198+1219)<<16)+ (198+900)));
     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER0_YCTL,(((641)<<16) + (400) ));


     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER0_XCTL,(((layer0_hposition+ layer0_hsize_out-1)<<16)+ (layer0_hposition)));
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER0_YCTL,(((layer0_vposition + layer0_vsize_out)<<16) + (layer0_vposition) ));
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER1_XCTL,(((layer1_hposition+layer1_hsize_out-1)<<16) + (layer1_hposition)));// #((198+839)<<16 + (198+520) )
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER1_YCTL, (((layer1_vposition+layer1_vsize_out-1)<<16) + (layer1_vposition) ));//#((1122-20)<<16 + (1122-260) )

     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER2_XCTL,(((layer2_hposition + layer2_hsize_out -1 )<<16)+ (layer2_hposition))); // #((198+519)<<16 + (198+200))
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER2_YCTL,(((layer2_vposition + layer2_vsize_out)<<16) + (layer2_vposition) ));//#((1122-20)<<16 + (1122-260) )
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER3_XCTL,(((layer3_hposition +layer3_hsize_out -1 )<<16) + (layer3_hposition) ));// #((198+1159)<<16 + (198+840) )
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER3_YCTL,(((layer3_vposition + layer3_vsize_out)<<16) + (layer3_vposition)));//#((1122-20)<<16 + (1122-260))

     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER4_XCTL,(((osd0_hposition+osd0_hsize_out-1)<<16) +osd0_hposition)); //#((2117-20) <<16 +(2117-260))
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER4_YCTL,(((osd0_vposition+osd0_vsize_out)<<16) + osd0_vposition));//#((60+240)<<16 + (60))
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER5_XCTL,(((osd1_hposition + osd1_hsize_out-1)<<16)+ osd1_hposition) );// #((2117-20) <<16 +(2117-260))
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER5_YCTL,(((osd1_vposition+osd1_vsize_out)<<16) + osd1_vposition) );//#((60+480)<<16 + (60+241))
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER6_XCTL,(((osd2_hposition + osd2_hsize_out-1)<<16) +osd2_hposition) );// #((2117-20) <<16 +(2117-260))
     VoRegWr( ctx->base, ADDR_VO_DISP_LAYER6_YCTL,(((osd2_vposition+osd2_vsize_out)<<16) + osd2_vposition ));//#((60+720)<<16 + (60+481)

     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER2_XCTL,(((198+1619)<<16)+ (198+1300))); // #((198+519)<<16 + (198+200))
     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER2_YCTL,(((569)<<16) + (100) ));//#((1122-20)<<16 + (1122-260) )

     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER1_XCTL,(((198+329)<<16)+ (198+10)));// #((198+839)<<16 + (198+520) )
     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER1_YCTL, (((341)<<16) + (100) ));//#((1122-20)<<16 + (1122-260) )


     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER3_XCTL,(((198+1159)<<16) + (198+840) ));// #((198+1159)<<16 + (198+840) )
     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER3_YCTL,(((1122-20)<<16) + (1122-260)));//#((1122-20)<<16 + (1122-260))

     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER4_XCTL,(((198+755)<<16) +(198+500))); //#((2117-20) <<16 +(2117-260))
     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER4_YCTL,(((940)<<16) + (700)));//#((60+240)<<16 + (60))

     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER5_XCTL,(((2117-20)<<16) +(2117-260)) );// #((2117-20) <<16 +(2117-260))
     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER5_YCTL,(((60+480)<<16) + (60+241)) );//#((60+480)<<16 + (60+241))

     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER6_XCTL,(((2117-20)<<16) +(2117-260)) );// #((2117-20) <<16 +(2117-260))
     // VoRegWr( ctx->base, ADDR_VO_DISP_LAYER6_YCTL,(((60+720)<<16) + (60+481) ));//#((60+720)<<16 + (60+481) )


//  # hsync, vsync, contorl
     VoRegWr( ctx->base, ADDR_VO_DISP_HSYNC_CTL,0x002d0006);
     VoRegWr( ctx->base, ADDR_VO_DISP_HSYNC1_CTL,0x00060006);
     VoRegWr( ctx->base, ADDR_VO_DISP_VSYNC1_CTL,0x00060001);
     VoRegWr( ctx->base, ADDR_VO_DISP_HSYNC2_CTL,0x00060006);
     VoRegWr( ctx->base, ADDR_VO_DISP_VSYNC2_CTL,0x00060001);
//     #VoRegWr( ctx->base, ADDR_VO_DISP_SIZE, ((background_vsize<<16)|background_hsize) + (FRAME_EDGE_VWIDTH*65536 + FRAME_EDGE_HWIDTH));

     VoRegWr( ctx->base, ADDR_VO_DISP_SIZE, 0x2ee07bc);
     //VoRegWr( ctx->base, ADDR_VO_DISP_SIZE, 0x4650898);


//       # hsync, vsync, contorl
//      #  VoRegWr( ctx->base, ADDR_VO_DISP_HSYNC_CTL,  0x080001);
//      #  VoRegWr( ctx->base, ADDR_VO_DISP_HSYNC1_CTL, 0x080001);
//       # VoRegWr( ctx->base, ADDR_VO_DISP_VSYNC1_CTL, 0x050001);
//      #  VoRegWr( ctx->base, ADDR_VO_DISP_HSYNC2_CTL, 0x080001);
//       # VoRegWr( ctx->base, ADDR_VO_DISP_VSYNC2_CTL, 0x050001);
//       # VoRegWr( ctx->base, ADDR_VO_DISP_SIZE,       ((background_vsize<<16)|background_hsize) + (FRAME_EDGE_VWIDTH*65536 + FRAME_EDGE_HWIDTH));  #big picture

     VoRegWr( ctx->base, ADDR_VO_DISP_CTL,        0x80101);
     VoRegWr( ctx->base, ADDR_VO_DISP_ENABLE,     0x0001);
     VoRegWr( ctx->base, ADDR_VO_REG_LOAD_CTL,0x11);
//        # disp interrupt
     VoRegWr( ctx->base, ADDR_VO_DISP_IRQ0_CTL,   0x0);
     VoRegWr( ctx->base, ADDR_VO_DISP_IRQ1_CTL,   0x0);
     VoRegWr( ctx->base, ADDR_VO_DISP_IRQ2_CTL,   0x0);

    printk("Display Config done!\n");
}


/* endof of vo hw function */

u32 vo_get_channel_formats(u8 ch, const u32 **formats)
{
	switch (ch) {
	case VO_CH1:
		*formats = channel_formats1;
		return ARRAY_SIZE(channel_formats1);
	default:
		DRM_ERROR("no this channel %d\n", ch);
		*formats = NULL;
		return 0;
	}
}

/* convert from fourcc format to vo format */
static u32 vo_get_format(u32 pixel_format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(vo_formats); i++)
		if (vo_formats[i].pixel_format == pixel_format)
			return vo_formats[i].vo_format;

	/* not found */
	DRM_ERROR("Not found pixel format!!fourcc_format= %d\n",
		  pixel_format);
	return VO_FORMAT_UNSUPPORT;
}

static void vo_update_reload_bit(void __iomem *base, u32 bit_num, u32 val)
{
	u32 bit_ofst, reg_num;

	bit_ofst = bit_num % 32;
	reg_num = bit_num / 32;

	vo_update_bits(base + VO_RELOAD_DIS(reg_num), bit_ofst,
			MASK(1), !!val);
}

static u32 vo_read_reload_bit(void __iomem *base, u32 bit_num)
{
	u32 tmp, bit_ofst, reg_num;

	bit_ofst = bit_num % 32;
	reg_num = bit_num / 32;

	tmp = readl(base + VO_RELOAD_DIS(reg_num));
	return !!(BIT(bit_ofst) & tmp);
}

static void vo_init(struct vo_hw_ctx *ctx)
{
	void __iomem *base = ctx->base;

	/* enable clk gate */
	vo_update_bits(base + VO_CTRL1, AUTO_CLK_GATE_EN_OFST,
			AUTO_CLK_GATE_EN, VO_ENABLE);
	/* clear overlay */
	writel(0, base + VO_OVLY1_TRANS_CFG);
	writel(0, base + VO_OVLY_CTL);
	writel(0, base + VO_OVLYX_CTL(OUT_OVLY));
	/* clear reset and reload regs */
	writel(MASK(32), base + VO_SOFT_RST_SEL(0));
	writel(MASK(32), base + VO_SOFT_RST_SEL(1));
	writel(MASK(32), base + VO_RELOAD_DIS(0));
	writel(MASK(32), base + VO_RELOAD_DIS(1));
	/*
	 * for video mode, all the vo registers should
	 * become effective at frame end.
	 */
	vo_update_bits(base + VO_CTRL, FRM_END_START_OFST,
			FRM_END_START_MASK, REG_EFFECTIVE_IN_VOEN_FRMEND);
}

static bool vo_crtc_mode_fixup(struct drm_crtc *crtc,
				const struct drm_display_mode *mode,
				struct drm_display_mode *adjusted_mode)
{
/* just return true for debug
	struct vo_crtc *acrtc = to_vo_crtc(crtc);
	struct vo_hw_ctx *ctx = acrtc->ctx;

	adjusted_mode->clock =
		clk_round_rate(ctx->vo_pix_clk, mode->clock * 1000) / 1000;

*/		
	return true;
}


static void vo_set_pix_clk(struct vo_hw_ctx *ctx,
			    struct drm_display_mode *mode,
			    struct drm_display_mode *adj_mode)
{
	u32 clk_Hz = mode->clock * 1000;
	int ret;

	/*
	 * Success should be guaranteed in mode_valid call back,
	 * so failure shouldn't happen here
	 */
	ret = clk_set_rate(ctx->vo_pix_clk, clk_Hz);
	if (ret)
		DRM_ERROR("failed to set pixel clk %dHz (%d)\n", clk_Hz, ret);
	adj_mode->clock = clk_get_rate(ctx->vo_pix_clk) / 1000;
}

static void vo_ldi_set_mode(struct vo_crtc *acrtc,
			     struct drm_display_mode *mode,
			     struct drm_display_mode *adj_mode)
{
	struct vo_hw_ctx *ctx = acrtc->ctx;
	void __iomem *base = ctx->base;
	u32 width = mode->hdisplay;
	u32 height = mode->vdisplay;
	u32 hfp, hbp, hsw, vfp, vbp, vsw;
	u32 plr_flags;

	plr_flags = (mode->flags & DRM_MODE_FLAG_NVSYNC) ? FLAG_NVSYNC : 0;
	plr_flags |= (mode->flags & DRM_MODE_FLAG_NHSYNC) ? FLAG_NHSYNC : 0;
	hfp = mode->hsync_start - mode->hdisplay;
	hbp = mode->htotal - mode->hsync_end;
	hsw = mode->hsync_end - mode->hsync_start;
	vfp = mode->vsync_start - mode->vdisplay;
	vbp = mode->vtotal - mode->vsync_end;
	vsw = mode->vsync_end - mode->vsync_start;
	if (vsw > 15) {
		DRM_DEBUG_DRIVER("vsw exceeded 15\n");
		vsw = 15;
	}

	writel((hbp << HBP_OFST) | hfp, base + LDI_HRZ_CTRL0);
	 /* the configured value is actual value - 1 */
	writel(hsw - 1, base + LDI_HRZ_CTRL1);
	writel((vbp << VBP_OFST) | vfp, base + LDI_VRT_CTRL0);
	 /* the configured value is actual value - 1 */
	writel(vsw - 1, base + LDI_VRT_CTRL1);
	 /* the configured value is actual value - 1 */
	writel(((height - 1) << VSIZE_OFST) | (width - 1),
	       base + LDI_DSP_SIZE);
	writel(plr_flags, base + LDI_PLR_CTRL);

	/* set overlay compositor output size */
	writel(((width - 1) << OUTPUT_XSIZE_OFST) | (height - 1),
	       base + VO_OVLY_OUTPUT_SIZE(OUT_OVLY));

	/* ctran6 setting */
	writel(CTRAN_BYPASS_ON, base + VO_CTRAN_DIS(VO_CTRAN6));
	 /* the configured value is actual value - 1 */
	writel(width * height - 1, base + VO_CTRAN_IMAGE_SIZE(VO_CTRAN6));
	vo_update_reload_bit(base, CTRAN_OFST + VO_CTRAN6, 0);

	vo_set_pix_clk(ctx, mode, adj_mode);

	DRM_DEBUG_DRIVER("set mode: %dx%d\n", width, height);
}

static int vo_power_up(struct vo_hw_ctx *ctx)
{
	int ret;

	ret = clk_prepare_enable(ctx->media_noc_clk);
	if (ret) {
		DRM_ERROR("failed to enable media_noc_clk (%d)\n", ret);
		return ret;
	}

	ret = reset_control_deassert(ctx->reset);
	if (ret) {
		DRM_ERROR("failed to deassert reset\n");
		return ret;
	}

	ret = clk_prepare_enable(ctx->vo_core_clk);
	if (ret) {
		DRM_ERROR("failed to enable vo_core_clk (%d)\n", ret);
		return ret;
	}

	vo_init(ctx);
	ctx->power_on = true;
	return 0;
}

static void vo_power_down(struct vo_hw_ctx *ctx)
{
	void __iomem *base = ctx->base;

	writel(VO_DISABLE, base + LDI_CTRL);
	/* dsi pixel off */
	writel(DSI_PCLK_OFF, base + LDI_HDMI_DSI_GT);

	clk_disable_unprepare(ctx->vo_core_clk);
	reset_control_assert(ctx->reset);
	clk_disable_unprepare(ctx->media_noc_clk);
	ctx->power_on = false;
}

static void vo_set_medianoc_qos(struct vo_crtc *acrtc)
{
	struct vo_hw_ctx *ctx = acrtc->ctx;
	struct regmap *map = ctx->noc_regmap;

	regmap_update_bits(map, VO0_QOSGENERATOR_MODE,
			   QOSGENERATOR_MODE_MASK, BYPASS_MODE);
	regmap_update_bits(map, VO0_QOSGENERATOR_EXTCONTROL,
			   SOCKET_QOS_EN, SOCKET_QOS_EN);

	regmap_update_bits(map, VO1_QOSGENERATOR_MODE,
			   QOSGENERATOR_MODE_MASK, BYPASS_MODE);
	regmap_update_bits(map, VO1_QOSGENERATOR_EXTCONTROL,
			   SOCKET_QOS_EN, SOCKET_QOS_EN);
}
static bool vblank_enable = 0;
static int vo_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct vo_crtc *acrtc = to_vo_crtc(crtc);
	struct vo_hw_ctx *ctx = acrtc->ctx;
	void __iomem *base = ctx->base;

printk("vo_crtc_enable_vblank enter, need add our own code\n");
vblank_enable = 1;
#if 0
	if (!ctx->power_on)
		(void)vo_power_up(ctx);

	vo_update_bits(base + LDI_INT_EN, FRAME_END_INT_EN_OFST,
			MASK(1), 1);
#endif
	return 0;
}

static void vo_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct vo_crtc *acrtc = to_vo_crtc(crtc);
	struct vo_hw_ctx *ctx = acrtc->ctx;
	void __iomem *base = ctx->base;

    printk("vo_crtc_disable_vblank enter, need add our own code\n");
    vblank_enable = 0;

    VoRegRd( ctx->base, ADDR_VO_LAYER0_BADDR0_Y); //  # y base: 0x800000

    
    printk("vo addr y0 0x%x\n", VoRegRd( ctx->base, ADDR_VO_LAYER0_BADDR0_Y));
    printk("vo addr y1 0x%x\n", VoRegRd( ctx->base, ADDR_VO_LAYER0_BADDR1_Y));
    printk("vo addr uv0 0x%x\n", VoRegRd( ctx->base, ADDR_VO_LAYER0_BADDR0_UV));
    printk("vo addr uv1 0x%x\n", VoRegRd( ctx->base, ADDR_VO_LAYER0_BADDR1_UV));
    printk("vo addr img in size 0x%x\n", VoRegRd( ctx->base, ADDR_VO_LAYER0_IMG_IN_SIZE));
    printk("vo addr img out pix size 0x%x\n", VoRegRd( ctx->base, ADDR_VO_LAYER0_IMG_OUT_PIX_SIZE));
    printk("vo addr img in pix size 0x%x\n", VoRegRd( ctx->base, ADDR_VO_LAYER0_IMG_IN_PIX_SIZE));

/*
    struct drm_plane *plane = crtc->primary;
    
	struct drm_gem_cma_object *obj = drm_fb_cma_get_gem_obj(plane->fb, 0);
    printk("dump mem addr 0x%lx\n", obj->vaddr);
    int i=0;
    u32 *addr = (u32 *)obj->vaddr;
    for (i=0;i++;i<16)
        printk("dump buf data, data[%d]=0x%x\n", i, *(addr + i));
*/
#if 0
	if (!ctx->power_on) {
		DRM_ERROR("power is down! vblank disable fail\n");
		return;
	}

	vo_update_bits(base + LDI_INT_EN, FRAME_END_INT_EN_OFST,
			MASK(1), 0);
#endif
}




static irqreturn_t vo_irq_handler(int irq, void *data)
{
	struct vo_crtc *acrtc = data;
	struct vo_hw_ctx *ctx = acrtc->ctx;
	struct drm_crtc *crtc = &acrtc->base;
	void __iomem *base = ctx->base;
	u32 status;

printk("vo_irq_handler enter\n");
#if 0
	status = readl(base + LDI_MSK_INT);
	DRM_DEBUG_DRIVER("LDI IRQ: status=0x%X\n", status);

	/* vblank irq */
	if (status & BIT(FRAME_END_INT_EN_OFST)) {
		vo_update_bits(base + LDI_INT_CLR, FRAME_END_INT_EN_OFST,
				MASK(1), 1);
		drm_crtc_handle_vblank(crtc);
	}
#endif

    drm_crtc_handle_vblank(crtc);

	return IRQ_HANDLED;
}

static void vo_display_enable(struct vo_crtc *acrtc)
{
	struct vo_hw_ctx *ctx = acrtc->ctx;
	void __iomem *base = ctx->base;
	u32 out_fmt = acrtc->out_format;

	/* enable output overlay compositor */
	writel(VO_ENABLE, base + VO_OVLYX_CTL(OUT_OVLY));
	vo_update_reload_bit(base, OVLY_OFST + OUT_OVLY, 0);

	/* display source setting */
	writel(DISP_SRC_OVLY2, base + VO_DISP_SRC_CFG);

	/* enable vo */
	writel(VO_ENABLE, base + VO_EN);
	/* enable ldi */
	writel(NORMAL_MODE, base + LDI_WORK_MODE);
	writel((out_fmt << BPP_OFST) | DATA_GATE_EN | LDI_EN,
	       base + LDI_CTRL);
	/* dsi pixel on */
	writel(DSI_PCLK_ON, base + LDI_HDMI_DSI_GT);
}

#if VO_DEBUG
static void vo_rdma_dump_regs(void __iomem *base, u32 ch)
{
	u32 reg_ctrl, reg_addr, reg_size, reg_stride, reg_space, reg_en;
	u32 val;

	reg_ctrl = RD_CH_CTRL(ch);
	reg_addr = RD_CH_ADDR(ch);
	reg_size = RD_CH_SIZE(ch);
	reg_stride = RD_CH_STRIDE(ch);
	reg_space = RD_CH_SPACE(ch);
	reg_en = RD_CH_EN(ch);

	val = vo_read_reload_bit(base, RDMA_OFST + ch);
	DRM_DEBUG_DRIVER("[rdma%d]: reload(%d)\n", ch + 1, val);
	val = readl(base + reg_ctrl);
	DRM_DEBUG_DRIVER("[rdma%d]: reg_ctrl(0x%08x)\n", ch + 1, val);
	val = readl(base + reg_addr);
	DRM_DEBUG_DRIVER("[rdma%d]: reg_addr(0x%08x)\n", ch + 1, val);
	val = readl(base + reg_size);
	DRM_DEBUG_DRIVER("[rdma%d]: reg_size(0x%08x)\n", ch + 1, val);
	val = readl(base + reg_stride);
	DRM_DEBUG_DRIVER("[rdma%d]: reg_stride(0x%08x)\n", ch + 1, val);
	val = readl(base + reg_space);
	DRM_DEBUG_DRIVER("[rdma%d]: reg_space(0x%08x)\n", ch + 1, val);
	val = readl(base + reg_en);
	DRM_DEBUG_DRIVER("[rdma%d]: reg_en(0x%08x)\n", ch + 1, val);
}

static void vo_clip_dump_regs(void __iomem *base, u32 ch)
{
	u32 val;

	val = vo_read_reload_bit(base, CLIP_OFST + ch);
	DRM_DEBUG_DRIVER("[clip%d]: reload(%d)\n", ch + 1, val);
	val = readl(base + VO_CLIP_DISABLE(ch));
	DRM_DEBUG_DRIVER("[clip%d]: reg_clip_disable(0x%08x)\n", ch + 1, val);
	val = readl(base + VO_CLIP_SIZE0(ch));
	DRM_DEBUG_DRIVER("[clip%d]: reg_clip_size0(0x%08x)\n", ch + 1, val);
	val = readl(base + VO_CLIP_SIZE1(ch));
	DRM_DEBUG_DRIVER("[clip%d]: reg_clip_size1(0x%08x)\n", ch + 1, val);
}

static void vo_compositor_routing_dump_regs(void __iomem *base, u32 ch)
{
	u8 ovly_ch = 0; /* TODO: Only primary plane now */
	u32 val;

	val = readl(base + VO_OVLY_CH_XY0(ovly_ch));
	DRM_DEBUG_DRIVER("[overlay ch%d]: reg_ch_xy0(0x%08x)\n", ovly_ch, val);
	val = readl(base + VO_OVLY_CH_XY1(ovly_ch));
	DRM_DEBUG_DRIVER("[overlay ch%d]: reg_ch_xy1(0x%08x)\n", ovly_ch, val);
	val = readl(base + VO_OVLY_CH_CTL(ovly_ch));
	DRM_DEBUG_DRIVER("[overlay ch%d]: reg_ch_ctl(0x%08x)\n", ovly_ch, val);
}

static void vo_dump_overlay_compositor_regs(void __iomem *base, u32 comp)
{
	u32 val;

	val = vo_read_reload_bit(base, OVLY_OFST + comp);
	DRM_DEBUG_DRIVER("[overlay%d]: reload(%d)\n", comp + 1, val);
	writel(VO_ENABLE, base + VO_OVLYX_CTL(comp));
	DRM_DEBUG_DRIVER("[overlay%d]: reg_ctl(0x%08x)\n", comp + 1, val);
	val = readl(base + VO_OVLY_CTL);
	DRM_DEBUG_DRIVER("ovly_ctl(0x%08x)\n", val);
}

static void vo_dump_regs(void __iomem *base)
{
	u32 i;

	/* dump channel regs */
	for (i = 0; i < VO_CH_NUM; i++) {
		/* dump rdma regs */
		vo_rdma_dump_regs(base, i);

		/* dump clip regs */
		vo_clip_dump_regs(base, i);

		/* dump compositor routing regs */
		vo_compositor_routing_dump_regs(base, i);
	}

	/* dump overlay compositor regs */
	vo_dump_overlay_compositor_regs(base, OUT_OVLY);
}
#else
static void vo_dump_regs(void __iomem *base) { }
#endif

static void vo_crtc_atomic_enable(struct drm_crtc *crtc,
				   struct drm_crtc_state *old_state)
{
	struct vo_crtc *acrtc = to_vo_crtc(crtc);
	struct vo_hw_ctx *ctx = acrtc->ctx;
	int ret;
printk("vo_crtc_atomic_enable enter\n");
	if (acrtc->enable)
		return;
#if 0
	if (!ctx->power_on) {
		ret = vo_power_up(ctx);
		if (ret)
			return;
	}
	vo_set_medianoc_qos(acrtc);
	vo_display_enable(acrtc);
	vo_dump_regs(ctx->base);
#endif
    
	drm_crtc_vblank_on(crtc);
	acrtc->enable = true;

    printk("vo_crtc_atomic_enable exit\n");
}

static void vo_crtc_atomic_disable(struct drm_crtc *crtc,
				    struct drm_crtc_state *old_state)
{
	struct vo_crtc *acrtc = to_vo_crtc(crtc);
	struct vo_hw_ctx *ctx = acrtc->ctx;

    printk("vo_crtc_atomic_disable enter\n");

	if (!acrtc->enable)
		return;

	drm_crtc_vblank_off(crtc);
	//vo_power_down(ctx);
	acrtc->enable = false;
    printk("vo_crtc_atomic_disable exit\n");

}

static void vo_crtc_mode_set_nofb(struct drm_crtc *crtc)
{
	struct vo_crtc *acrtc = to_vo_crtc(crtc);
	struct vo_hw_ctx *ctx = acrtc->ctx;
	struct drm_display_mode *mode = &crtc->state->mode;
	struct drm_display_mode *adj_mode = &crtc->state->adjusted_mode;

printk("vo_crtc_mode_set_nofb enter, need add our own code\n");
#if 0
	if (!ctx->power_on)
		(void)vo_power_up(ctx);
	vo_ldi_set_mode(acrtc, mode, adj_mode);
#endif
}

static void vo_crtc_atomic_begin(struct drm_crtc *crtc,
				  struct drm_crtc_state *old_state)
{
	struct vo_crtc *acrtc = to_vo_crtc(crtc);
	struct vo_hw_ctx *ctx = acrtc->ctx;
	struct drm_display_mode *mode = &crtc->state->mode;
	struct drm_display_mode *adj_mode = &crtc->state->adjusted_mode;
    printk("vo_crtc_atomic_begin enter, need add our own code\n");
#if 0

	if (!ctx->power_on)
		(void)vo_power_up(ctx);
	vo_ldi_set_mode(acrtc, mode, adj_mode);
#endif
}

static void vo_crtc_atomic_flush(struct drm_crtc *crtc,
				  struct drm_crtc_state *old_state)

{
	struct vo_crtc *acrtc = to_vo_crtc(crtc);
	struct vo_hw_ctx *ctx = acrtc->ctx;
	struct drm_pending_vblank_event *event = crtc->state->event;
	void __iomem *base = ctx->base;

    printk("vo_crtc_atomic_flush enter, need add our own code\n");
#if 0

	/* only crtc is enabled regs take effect */
	if (acrtc->enable) {
		vo_dump_regs(base);
		/* flush vo registers */
		writel(VO_ENABLE, base + VO_EN);
	}
#endif
	if (event) {
		crtc->state->event = NULL;

		spin_lock_irq(&crtc->dev->event_lock);
		if (drm_crtc_vblank_get(crtc) == 0)
			drm_crtc_arm_vblank_event(crtc, event);
		else
			drm_crtc_send_vblank_event(crtc, event);
		spin_unlock_irq(&crtc->dev->event_lock);
	}
    printk("vo_crtc_atomic_flush exit\n");    
}



static const struct drm_crtc_helper_funcs vo_crtc_helper_funcs = {
	.mode_fixup	= vo_crtc_mode_fixup,
	.mode_set_nofb	= vo_crtc_mode_set_nofb,
	.atomic_begin	= vo_crtc_atomic_begin,
	.atomic_flush	= vo_crtc_atomic_flush,
	.atomic_enable	= vo_crtc_atomic_enable,
	.atomic_disable	= vo_crtc_atomic_disable,
};

static const struct drm_crtc_funcs vo_crtc_funcs = {
	.destroy	= drm_crtc_cleanup,
	.set_config	= drm_atomic_helper_set_config,
	.page_flip	= drm_atomic_helper_page_flip,
	.reset		= drm_atomic_helper_crtc_reset,
	.atomic_duplicate_state	= drm_atomic_helper_crtc_duplicate_state,
	.atomic_destroy_state	= drm_atomic_helper_crtc_destroy_state,
	.enable_vblank	= vo_crtc_enable_vblank,
	.disable_vblank	= vo_crtc_disable_vblank,
};

static int vo_crtc_init(struct drm_device *dev, struct drm_crtc *crtc,
			 struct drm_plane *plane)
{
	struct device_node *port;
	int ret;

	/* set crtc port so that
	 * drm_of_find_possible_crtcs call works
	 */
	port = of_get_child_by_name(dev->dev->of_node, "port");
	if (!port) {
		DRM_ERROR("no port node found in %pOF\n", dev->dev->of_node);
		return -EINVAL;
	}
	of_node_put(port);
	crtc->port = port;

	ret = drm_crtc_init_with_planes(dev, crtc, plane, NULL,
					&vo_crtc_funcs, NULL);
	if (ret) {
		DRM_ERROR("failed to init crtc.\n");
		return ret;
	}

	drm_crtc_helper_add(crtc, &vo_crtc_helper_funcs);


	return 0;
}

static void vo_rdma_set(void __iomem *base, struct drm_framebuffer *fb,
			 u32 ch, u32 y, u32 in_h, u32 fmt)
{
	struct drm_gem_cma_object *obj = drm_fb_cma_get_gem_obj(fb, 0);
	struct drm_format_name_buf format_name;
	u32 reg_ctrl, reg_addr, reg_size, reg_stride, reg_space, reg_en;
	u32 stride = fb->pitches[0];
	u32 addr = (u32)obj->paddr + y * stride;

	DRM_DEBUG_DRIVER("rdma%d: (y=%d, height=%d), stride=%d, paddr=0x%x\n",
			 ch + 1, y, in_h, stride, (u32)obj->paddr);
	DRM_DEBUG_DRIVER("addr=0x%x, fb:%dx%d, pixel_format=%d(%s)\n",
			 addr, fb->width, fb->height, fmt,
			 drm_get_format_name(fb->format->format, &format_name));

	/* get reg offset */
	reg_ctrl = RD_CH_CTRL(ch);
	reg_addr = RD_CH_ADDR(ch);
	reg_size = RD_CH_SIZE(ch);
	reg_stride = RD_CH_STRIDE(ch);
	reg_space = RD_CH_SPACE(ch);
	reg_en = RD_CH_EN(ch);

	/*
	 * TODO: set rotation
	 */
	writel((fmt << 16) & 0x1f0000, base + reg_ctrl);
	writel(addr, base + reg_addr);
	writel((in_h << 16) | stride, base + reg_size);
	writel(stride, base + reg_stride);
	writel(in_h * stride, base + reg_space);
	writel(VO_ENABLE, base + reg_en);
	vo_update_reload_bit(base, RDMA_OFST + ch, 0);
}

static void vo_rdma_disable(void __iomem *base, u32 ch)
{
	u32 reg_en;

	/* get reg offset */
	reg_en = RD_CH_EN(ch);
	writel(0, base + reg_en);
	vo_update_reload_bit(base, RDMA_OFST + ch, 1);
}

static void vo_clip_set(void __iomem *base, u32 ch, u32 fb_w, u32 x,
			 u32 in_w, u32 in_h)
{
	u32 disable_val;
	u32 clip_left;
	u32 clip_right;

	/*
	 * clip width, no need to clip height
	 */
	if (fb_w == in_w) { /* bypass */
		disable_val = 1;
		clip_left = 0;
		clip_right = 0;
	} else {
		disable_val = 0;
		clip_left = x;
		clip_right = fb_w - (x + in_w) - 1;
	}

	DRM_DEBUG_DRIVER("clip%d: clip_left=%d, clip_right=%d\n",
			 ch + 1, clip_left, clip_right);

	writel(disable_val, base + VO_CLIP_DISABLE(ch));
	writel((fb_w - 1) << 16 | (in_h - 1), base + VO_CLIP_SIZE0(ch));
	writel(clip_left << 16 | clip_right, base + VO_CLIP_SIZE1(ch));
	vo_update_reload_bit(base, CLIP_OFST + ch, 0);
}

static void vo_clip_disable(void __iomem *base, u32 ch)
{
	writel(1, base + VO_CLIP_DISABLE(ch));
	vo_update_reload_bit(base, CLIP_OFST + ch, 1);
}

static bool has_Alpha_channel(int format)
{
	switch (format) {
	case VO_ARGB_8888:
	case VO_ABGR_8888:
	case VO_RGBA_8888:
	case VO_BGRA_8888:
		return true;
	default:
		return false;
	}
}

static void vo_get_blending_params(u32 fmt, u8 glb_alpha, u8 *alp_mode,
				    u8 *alp_sel, u8 *under_alp_sel)
{
	bool has_alpha = has_Alpha_channel(fmt);

	/*
	 * get alp_mode
	 */
	if (has_alpha && glb_alpha < 255)
		*alp_mode = VO_ALP_PIXEL_AND_GLB;
	else if (has_alpha)
		*alp_mode = VO_ALP_PIXEL;
	else
		*alp_mode = VO_ALP_GLOBAL;

	/*
	 * get alp sel
	 */
	*alp_sel = VO_ALP_MUL_COEFF_3; /* 1 */
	*under_alp_sel = VO_ALP_MUL_COEFF_2; /* 0 */
}

static void vo_compositor_routing_set(void __iomem *base, u8 ch,
				       u32 x0, u32 y0,
				       u32 in_w, u32 in_h, u32 fmt)
{
	u8 ovly_ch = 0; /* TODO: This is the zpos, only one plane now */
	u8 glb_alpha = 255;
	u32 x1 = x0 + in_w - 1;
	u32 y1 = y0 + in_h - 1;
	u32 val;
	u8 alp_sel;
	u8 under_alp_sel;
	u8 alp_mode;

	vo_get_blending_params(fmt, glb_alpha, &alp_mode, &alp_sel,
				&under_alp_sel);

	/* overlay routing setting
	 */
	writel(x0 << 16 | y0, base + VO_OVLY_CH_XY0(ovly_ch));
	writel(x1 << 16 | y1, base + VO_OVLY_CH_XY1(ovly_ch));
	val = (ch + 1) << CH_SEL_OFST | BIT(CH_EN_OFST) |
		alp_sel << CH_ALP_SEL_OFST |
		under_alp_sel << CH_UNDER_ALP_SEL_OFST |
		glb_alpha << CH_ALP_GBL_OFST |
		alp_mode << CH_ALP_MODE_OFST;
	writel(val, base + VO_OVLY_CH_CTL(ovly_ch));
	/* connect this plane/channel to overlay2 compositor */
	vo_update_bits(base + VO_OVLY_CTL, CH_OVLY_SEL_OFST(ovly_ch),
			CH_OVLY_SEL_MASK, CH_OVLY_SEL_VAL(OUT_OVLY));
}

static void vo_compositor_routing_disable(void __iomem *base, u32 ch)
{
	u8 ovly_ch = 0; /* TODO: Only primary plane now */

	/* disable this plane/channel */
	vo_update_bits(base + VO_OVLY_CH_CTL(ovly_ch), CH_EN_OFST,
			MASK(1), 0);
	/* dis-connect this plane/channel of overlay2 compositor */
	vo_update_bits(base + VO_OVLY_CTL, CH_OVLY_SEL_OFST(ovly_ch),
			CH_OVLY_SEL_MASK, 0);
}

/*
 * Typicaly, a channel looks like: DMA-->clip-->scale-->ctrans-->compositor
 */
static void vo_update_channel(struct vo_plane *aplane,
			       struct drm_framebuffer *fb, int crtc_x,
			       int crtc_y, unsigned int crtc_w,
			       unsigned int crtc_h, u32 src_x,
			       u32 src_y, u32 src_w, u32 src_h)
{
	struct vo_hw_ctx *ctx = aplane->ctx;
	void __iomem *base = ctx->base;
	u32 fmt = vo_get_format(fb->format->format);
	u32 ch = aplane->ch;
	u32 in_w;
	u32 in_h;

	DRM_DEBUG_DRIVER("channel%d: src:(%d, %d)-%dx%d, crtc:(%d, %d)-%dx%d",
			 ch + 1, src_x, src_y, src_w, src_h,
			 crtc_x, crtc_y, crtc_w, crtc_h);

	/* 1) DMA setting */
	in_w = src_w;
	in_h = src_h;
	vo_rdma_set(base, fb, ch, src_y, in_h, fmt);

	/* 2) clip setting */
	vo_clip_set(base, ch, fb->width, src_x, in_w, in_h);

	/* 3) TODO: scale setting for overlay planes */

	/* 4) TODO: ctran/csc setting for overlay planes */

	/* 5) compositor routing setting */
	vo_compositor_routing_set(base, ch, crtc_x, crtc_y, in_w, in_h, fmt);
}

static void vo_disable_channel(struct vo_plane *aplane)
{
	struct vo_hw_ctx *ctx = aplane->ctx;
	void __iomem *base = ctx->base;
	u32 ch = aplane->ch;

	DRM_DEBUG_DRIVER("disable channel%d\n", ch + 1);

	/* disable read DMA */
	vo_rdma_disable(base, ch);

	/* disable clip */
	vo_clip_disable(base, ch);

	/* disable compositor routing */
	vo_compositor_routing_disable(base, ch);
}

static int vo_plane_atomic_check(struct drm_plane *plane,
				  struct drm_plane_state *state)
{
	struct drm_framebuffer *fb = state->fb;
	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	u32 src_x = state->src_x >> 16;
	u32 src_y = state->src_y >> 16;
	u32 src_w = state->src_w >> 16;
	u32 src_h = state->src_h >> 16;
	int crtc_x = state->crtc_x;
	int crtc_y = state->crtc_y;
	u32 crtc_w = state->crtc_w;
	u32 crtc_h = state->crtc_h;
	u32 fmt;
printk("vo_plane_atomic_check enter\n");
	if (!crtc || !fb)
		return 0;

	fmt = vo_get_format(fb->format->format);
	if (fmt == VO_FORMAT_UNSUPPORT)
		return -EINVAL;

	crtc_state = drm_atomic_get_crtc_state(state->state, crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	if (src_w != crtc_w || src_h != crtc_h) {
		DRM_ERROR("Scale not support!!!\n");
		return -EINVAL;
	}

	if (src_x + src_w > fb->width ||
	    src_y + src_h > fb->height)
		return -EINVAL;

	if (crtc_x < 0 || crtc_y < 0)
		return -EINVAL;

	if (crtc_x + crtc_w > crtc_state->adjusted_mode.hdisplay ||
	    crtc_y + crtc_h > crtc_state->adjusted_mode.vdisplay)
		return -EINVAL;

printk("vo_plane_atomic_check normally exit\n");
	return 0;
}

static void vo_plane_atomic_update(struct drm_plane *plane,
				    struct drm_plane_state *old_state)
{
	struct drm_plane_state	*state	= plane->state;
	struct vo_plane *aplane = to_vo_plane(plane);

	struct vo_hw_ctx *ctx = aplane->ctx;
	
	struct drm_gem_cma_object *obj = drm_fb_cma_get_gem_obj(state->fb, 0);
    u64 paddr = obj->paddr;
    
    printk("vo_plane_atomic_update enter, fb paddr = 0x%lx, need add our own code\n", paddr);

    //update layer 0 addr for now for debug

    VoRegWr( ctx->base, ADDR_VO_LAYER0_BADDR0_Y, paddr); //0x50000000);//  # y base: 0x800000
    VoRegWr( ctx->base, ADDR_VO_LAYER0_BADDR0_UV, paddr + 2048*720); //0x50000000 + 2048*720); # uv base: 0x900000
    VoRegWr( ctx->base, ADDR_VO_LAYER0_BADDR1_Y,  paddr); //0x50000000);//  # y base: 0x800000
    VoRegWr( ctx->base, ADDR_VO_LAYER0_BADDR1_UV,  paddr + 2048*720); //0x50000000 + 2048*720); # uv base: 0x900000
/*
    int size = 1024*1024;
    void *vaddr = obj->vaddr;

	unsigned long offset = offset_in_page(vaddr);
	void *start = vaddr - offset;
    
	size = round_up(size+offset, PAGE_SIZE);
    
while (size) {
    flush_dcache_page(virt_to_page(start));
    start += PAGE_SIZE;
    size -= PAGE_SIZE;
}
printk("cache flush done\n");
*/    
#if 0
	vo_update_channel(aplane, state->fb, state->crtc_x, state->crtc_y,
			   state->crtc_w, state->crtc_h,
			   state->src_x >> 16, state->src_y >> 16,
			   state->src_w >> 16, state->src_h >> 16);
#endif
        printk("vo_plane_atomic_update exit\n");

}

static void vo_plane_atomic_disable(struct drm_plane *plane,
				     struct drm_plane_state *old_state)
{
    printk("vo_plane_atomic_disable enter, need add our own code\n");
#if 0
	struct vo_plane *aplane = to_vo_plane(plane);

	vo_disable_channel(aplane);
#endif
}

static const struct drm_plane_helper_funcs vo_plane_helper_funcs = {
	.atomic_check = vo_plane_atomic_check,
	.atomic_update = vo_plane_atomic_update,
	.atomic_disable = vo_plane_atomic_disable,
};

static struct drm_plane_funcs vo_plane_funcs = {
	.update_plane	= drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
};

static int vo_plane_init(struct drm_device *dev, struct vo_plane *aplane,
			  enum drm_plane_type type)
{
	const u32 *fmts;
	u32 fmts_cnt;
	int ret = 0;

    printk("vo_plane_init enter\n");
	/* get  properties */
	fmts_cnt = vo_get_channel_formats(aplane->ch, &fmts);
	if (ret)
		return ret;

	ret = drm_universal_plane_init(dev, &aplane->base, 1, &vo_plane_funcs,
				       fmts, fmts_cnt, NULL, type, NULL);
	if (ret) {
		DRM_ERROR("fail to init plane, ch=%d\n", aplane->ch);
		return ret;
	}

	drm_plane_helper_add(&aplane->base, &vo_plane_helper_funcs);

    printk("vo_plane_init normally exit\n");

	return 0;
}

static int vo_dts_parse(struct platform_device *pdev, struct vo_hw_ctx *ctx)
{
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *np = pdev->dev.of_node;

    printk("drm_vo debug, vo_dts_parse enter\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ctx->base = devm_ioremap_resource(dev, res);
    printk("drm_vo debug, ctx base 0x%lx, res 0x%lx\n", ctx->base, res);    
	if (IS_ERR(ctx->base)) {
		DRM_ERROR("failed to remap vo io base\n");
		return  PTR_ERR(ctx->base);
	}

	ctx->reset = devm_reset_control_get(dev, NULL);
	if (IS_ERR(ctx->reset))

    {        
		DRM_ERROR("failed to get reset control\n");
		//return PTR_ERR(ctx->reset);
    }
	ctx->noc_regmap =
		syscon_regmap_lookup_by_phandle(np, "kendryte,noc-syscon");
	if (IS_ERR(ctx->noc_regmap)) {
		DRM_ERROR("failed to get noc regmap\n");
		//return PTR_ERR(ctx->noc_regmap); don't return for debug
	}

	ctx->irq = platform_get_irq(pdev, 0);
	if (ctx->irq < 0) {
		DRM_ERROR("failed to get irq\n");
		return -ENODEV;
	}

	ctx->vo_core_clk = devm_clk_get(dev, "clk_vo_core");
	if (IS_ERR(ctx->vo_core_clk)) {
		DRM_ERROR("failed to parse clk VO_CORE\n");
		//return PTR_ERR(ctx->vo_core_clk);
	}

	ctx->media_noc_clk = devm_clk_get(dev, "clk_codec_jpeg");
	if (IS_ERR(ctx->media_noc_clk)) {
		DRM_ERROR("failed to parse clk CODEC_JPEG\n");
		//return PTR_ERR(ctx->media_noc_clk);
	}

	ctx->vo_pix_clk = devm_clk_get(dev, "clk_vo_pix");
	if (IS_ERR(ctx->vo_pix_clk)) {
		DRM_ERROR("failed to parse clk VO_PIX\n");
		//return PTR_ERR(ctx->vo_pix_clk);
	}
    printk("drm_vo debug, vo_dts_parse normally exit\n");

	return 0;
}

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/timer.h>

static struct timer_list sim_vblank; 
static struct vo_crtc *g_crtc = NULL;
void sim_vblank_timer_callback( struct timer_list *timer ) 
{ 
    struct drm_crtc *crtc; 
    int ret;
    
    //printk( "sim_vblank_timer_callback called (%ld).\n", jiffies ); 
    if(vblank_enable == 1) {
        
        if(g_crtc) {
            crtc = &g_crtc->base;
            drm_crtc_handle_vblank(crtc);
            //printk("drm_crtc_handle_vblank done\n");
        }
    }
    del_timer(&sim_vblank);
    timer_setup( &sim_vblank, sim_vblank_timer_callback, 0 ); 
    //printk( "Starting timer to fire in 500ms (%ld)\n", jiffies ); 
    ret = mod_timer( &sim_vblank, jiffies + msecs_to_jiffies(33) );
    if (ret) 
        printk("Error in mod_timer\n");

}
int init_timer_vblank( struct vo_crtc *acrtc ) 
{
    int ret; 
    printk("Timer module installing\n"); 
    timer_setup( &sim_vblank, sim_vblank_timer_callback, 0 ); 
    printk( "Starting timer to fire in 500ms (%ld)\n", jiffies ); 
    ret = mod_timer( &sim_vblank, jiffies + msecs_to_jiffies(33) );
    if (ret) 
        printk("Error in mod_timer\n");
    g_crtc = acrtc;
    return 0;
}
void cleanup_timer_vblank( void )
{
    int ret;
    ret = del_timer( &sim_vblank );
    if (ret) 
        printk("The timer is still in use...\n");
    printk("Timer uninstalled\n");
    return; 
}

static int vo_drm_init(struct platform_device *pdev)
{
	struct drm_device *dev = platform_get_drvdata(pdev);
	struct vo_data *vo;
	struct vo_hw_ctx *ctx;
	struct vo_crtc *acrtc;
	struct vo_plane *aplane;
	enum drm_plane_type type;
	int ret;
	int i;

    printk("vo_drm_init enter\n");
	vo = devm_kzalloc(dev->dev, sizeof(*vo), GFP_KERNEL);
	if (!vo) {
		DRM_ERROR("failed to alloc vo_data\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, vo);

	ctx = &vo->ctx;
	acrtc = &vo->acrtc;
	acrtc->ctx = ctx;
	acrtc->out_format = LDI_OUT_RGB_888;

	ret = vo_dts_parse(pdev, ctx);
	if (ret)
    {
		DRM_ERROR("failed to parse dts\n");        
		//return ret; not return for debug
    }

    printk("vo dts parse done\n");
	/*
	 * plane init
	 * TODO: Now only support primary plane, overlay planes
	 * need to do.
	 */
	for (i = 0; i < VO_CH_NUM; i++) {
		aplane = &vo->aplane[i];
		aplane->ch = i;
		aplane->ctx = ctx;
		type = i == PRIMARY_CH ? DRM_PLANE_TYPE_PRIMARY :
			DRM_PLANE_TYPE_OVERLAY;

		ret = vo_plane_init(dev, aplane, type);
		if (ret)
        {      
            DRM_ERROR("failed to init vo plane %d\n", i);        
            //return ret; not return for debug
        }
    }
	/* crtc init */
	ret = vo_crtc_init(dev, &acrtc->base, &vo->aplane[PRIMARY_CH].base);
    if (ret)
    {      
        DRM_ERROR("failed to init vo crc\n");        
        //return ret; not return for debug
    }

	/* vblank irq init */
	ret = devm_request_irq(dev->dev, ctx->irq, vo_irq_handler,
			       IRQF_SHARED, dev->driver->name, acrtc);
    if (ret)
    {      
        DRM_ERROR("failed to init request irq for vblank\n");        
        //return ret; not return for debug
    }

    /*init simu vblank timer for debug*/
    init_timer_vblank( acrtc);

    /* temp put vo hw init here for debug */
    InitDisplay_720p50fps(ctx);

    printk("vo_drm_init normally exit\n");

	return 0;
}

static void vo_drm_cleanup(struct platform_device *pdev)
{
}

const struct kendryte_dc_ops vo_dc_ops = {
	.init = vo_drm_init,
	.cleanup = vo_drm_cleanup
};

