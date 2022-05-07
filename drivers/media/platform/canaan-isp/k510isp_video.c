/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 * Copyright (C) 2009-2010 Nokia Corporation
 *
 * Contacts: Canaan Bright Sight Co., Ltd
 * 	     Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 *           Sakari Ailus <sakari.ailus@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <asm/cacheflush.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mc.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>

#include "k510isp_video.h"
#include "k510_isp.h"
/* -----------------------------------------------------------------------------
 * Helper functions
 *-----------------------------------------------------------------------------*/

/*
 * NOTE: When adding new media bus codes, always remember to add
 * corresponding in-memory formats to the table below!!!
 */
static struct k510isp_format_info formats[] = { 
	{ MEDIA_BUS_FMT_SBGGR8_1X8, MEDIA_BUS_FMT_SBGGR8_1X8,
	  MEDIA_BUS_FMT_SBGGR8_1X8, MEDIA_BUS_FMT_SBGGR8_1X8,
	  V4L2_PIX_FMT_SBGGR8, 8, 1, },
	{ MEDIA_BUS_FMT_SGBRG8_1X8, MEDIA_BUS_FMT_SGBRG8_1X8,
	  MEDIA_BUS_FMT_SGBRG8_1X8, MEDIA_BUS_FMT_SGBRG8_1X8,
	  V4L2_PIX_FMT_SGBRG8, 8, 1, },
	{ MEDIA_BUS_FMT_SGRBG8_1X8, MEDIA_BUS_FMT_SGRBG8_1X8,
	  MEDIA_BUS_FMT_SGRBG8_1X8, MEDIA_BUS_FMT_SGRBG8_1X8,
	  V4L2_PIX_FMT_SGRBG8, 8, 1, },
	{ MEDIA_BUS_FMT_SRGGB8_1X8, MEDIA_BUS_FMT_SRGGB8_1X8,
	  MEDIA_BUS_FMT_SRGGB8_1X8, MEDIA_BUS_FMT_SRGGB8_1X8,
	  V4L2_PIX_FMT_SRGGB8, 8, 1, },
	{ MEDIA_BUS_FMT_SBGGR10_1X10, MEDIA_BUS_FMT_SBGGR10_1X10,
	  MEDIA_BUS_FMT_SBGGR10_1X10, MEDIA_BUS_FMT_SBGGR8_1X8,
	  V4L2_PIX_FMT_SBGGR10, 10, 2, },
	{ MEDIA_BUS_FMT_SGBRG10_1X10, MEDIA_BUS_FMT_SGBRG10_1X10,
	  MEDIA_BUS_FMT_SGBRG10_1X10, MEDIA_BUS_FMT_SGBRG8_1X8,
	  V4L2_PIX_FMT_SGBRG10, 10, 2, },
	{ MEDIA_BUS_FMT_SGRBG10_1X10, MEDIA_BUS_FMT_SGRBG10_1X10,
	  MEDIA_BUS_FMT_SGRBG10_1X10, MEDIA_BUS_FMT_SGRBG8_1X8,
	  V4L2_PIX_FMT_SGRBG10, 10, 2, },
	{ MEDIA_BUS_FMT_SRGGB10_1X10, MEDIA_BUS_FMT_SRGGB10_1X10,
	  MEDIA_BUS_FMT_SRGGB10_1X10, MEDIA_BUS_FMT_SRGGB8_1X8,
	  V4L2_PIX_FMT_SRGGB10, 10, 2, },
	{ MEDIA_BUS_FMT_SBGGR12_1X12, MEDIA_BUS_FMT_SBGGR10_1X10,
	  MEDIA_BUS_FMT_SBGGR12_1X12, MEDIA_BUS_FMT_SBGGR8_1X8,
	  V4L2_PIX_FMT_SBGGR12, 12, 2, },
	{ MEDIA_BUS_FMT_SGBRG12_1X12, MEDIA_BUS_FMT_SGBRG10_1X10,
	  MEDIA_BUS_FMT_SGBRG12_1X12, MEDIA_BUS_FMT_SGBRG8_1X8,
	  V4L2_PIX_FMT_SGBRG12, 12, 2, },
	{ MEDIA_BUS_FMT_SGRBG12_1X12, MEDIA_BUS_FMT_SGRBG10_1X10,
	  MEDIA_BUS_FMT_SGRBG12_1X12, MEDIA_BUS_FMT_SGRBG8_1X8,
	  V4L2_PIX_FMT_SGRBG12, 12, 2, },
	{ MEDIA_BUS_FMT_SRGGB12_1X12, MEDIA_BUS_FMT_SRGGB10_1X10,
	  MEDIA_BUS_FMT_SRGGB12_1X12, MEDIA_BUS_FMT_SRGGB8_1X8,
	  V4L2_PIX_FMT_SRGGB12, 12, 2, },
	//V4L2_PIX_FMT_YUV422P YUV422 2bpp
	{ MEDIA_BUS_FMT_YUYV8_2X8, MEDIA_BUS_FMT_YUYV8_2X8,
	  MEDIA_BUS_FMT_YUYV8_2X8, 0,
	  V4L2_PIX_FMT_YUV422P, 16, 2, },
	{ MEDIA_BUS_FMT_YVYU8_2X8, MEDIA_BUS_FMT_YVYU8_2X8,
	  MEDIA_BUS_FMT_YVYU8_2X8, 0,
	  V4L2_PIX_FMT_YUV422P, 16, 2, },
	{ MEDIA_BUS_FMT_UYVY8_1X16, MEDIA_BUS_FMT_UYVY8_1X16,
	  MEDIA_BUS_FMT_UYVY8_1X16, 0,
	  V4L2_PIX_FMT_YUV422P, 16, 2, },
	{ MEDIA_BUS_FMT_YUYV8_1X16, MEDIA_BUS_FMT_YUYV8_1X16,
	  MEDIA_BUS_FMT_YUYV8_1X16, 0,
	  V4L2_PIX_FMT_YUV422P, 16, 2, },
	{ MEDIA_BUS_FMT_YVYU8_1X16, MEDIA_BUS_FMT_YVYU8_1X16,
	  MEDIA_BUS_FMT_YVYU8_1X16, 0,
	  V4L2_PIX_FMT_YUV422P, 16, 2, },
	{ MEDIA_BUS_FMT_VYUY8_1X16, MEDIA_BUS_FMT_VYUY8_1X16,
	  MEDIA_BUS_FMT_VYUY8_1X16, 0,
	  V4L2_PIX_FMT_YUV422P, 16, 2, },
	//V4L2_PIX_FMT_NV16 YUV422 2bpp
	{ MEDIA_BUS_FMT_YUYV8_2X8, MEDIA_BUS_FMT_YUYV8_2X8,
	  MEDIA_BUS_FMT_YUYV8_2X8, 0,
	  V4L2_PIX_FMT_NV16, 16, 2, },
	{ MEDIA_BUS_FMT_YVYU8_2X8, MEDIA_BUS_FMT_YVYU8_2X8,
	  MEDIA_BUS_FMT_YVYU8_2X8, 0,
	  V4L2_PIX_FMT_NV16, 16, 2, },
	{ MEDIA_BUS_FMT_UYVY8_1X16, MEDIA_BUS_FMT_UYVY8_1X16,
	  MEDIA_BUS_FMT_UYVY8_1X16, 0,
	  V4L2_PIX_FMT_NV16, 16, 2, },
	{ MEDIA_BUS_FMT_YUYV8_1X16, MEDIA_BUS_FMT_YUYV8_1X16,
	  MEDIA_BUS_FMT_YUYV8_1X16, 0,
	  V4L2_PIX_FMT_NV16, 16, 2, },
	{ MEDIA_BUS_FMT_YVYU8_1X16, MEDIA_BUS_FMT_YVYU8_1X16,
	  MEDIA_BUS_FMT_YVYU8_1X16, 0,
	  V4L2_PIX_FMT_NV16, 16, 2, },
	{ MEDIA_BUS_FMT_VYUY8_1X16, MEDIA_BUS_FMT_VYUY8_1X16,
	  MEDIA_BUS_FMT_VYUY8_1X16, 0,
	  V4L2_PIX_FMT_NV16, 16, 2, },
	//V4L2_PIX_FMT_NV61 YUV422 2bpp
	{ MEDIA_BUS_FMT_YUYV8_2X8, MEDIA_BUS_FMT_YUYV8_2X8,
	  MEDIA_BUS_FMT_YUYV8_2X8, 0,
	  V4L2_PIX_FMT_NV61, 16, 2, },
	{ MEDIA_BUS_FMT_YVYU8_2X8, MEDIA_BUS_FMT_YVYU8_2X8,
	  MEDIA_BUS_FMT_YVYU8_2X8, 0,
	  V4L2_PIX_FMT_NV61, 16, 2, },
	{ MEDIA_BUS_FMT_UYVY8_1X16, MEDIA_BUS_FMT_UYVY8_1X16,
	  MEDIA_BUS_FMT_UYVY8_1X16, 0,
	  V4L2_PIX_FMT_NV61, 16, 2, },
	{ MEDIA_BUS_FMT_YUYV8_1X16, MEDIA_BUS_FMT_YUYV8_1X16,
	  MEDIA_BUS_FMT_YUYV8_1X16, 0,
	  V4L2_PIX_FMT_NV61, 16, 2, },
	{ MEDIA_BUS_FMT_YVYU8_1X16, MEDIA_BUS_FMT_YVYU8_1X16,
	  MEDIA_BUS_FMT_YVYU8_1X16, 0,
	  V4L2_PIX_FMT_NV61, 16, 2, },
	{ MEDIA_BUS_FMT_VYUY8_1X16, MEDIA_BUS_FMT_VYUY8_1X16,
	  MEDIA_BUS_FMT_VYUY8_1X16, 0,
	  V4L2_PIX_FMT_NV61, 16, 2, },
	//V4L2_PIX_FMT_NV12
	{ MEDIA_BUS_FMT_UYVY8_1_5X8, MEDIA_BUS_FMT_UYVY8_1_5X8,
	  MEDIA_BUS_FMT_UYVY8_1_5X8, MEDIA_BUS_FMT_UYVY8_1_5X8,
	  V4L2_PIX_FMT_NV12, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_VYUY8_1_5X8, MEDIA_BUS_FMT_VYUY8_1_5X8,
	  MEDIA_BUS_FMT_VYUY8_1_5X8, MEDIA_BUS_FMT_VYUY8_1_5X8,
	  V4L2_PIX_FMT_NV12, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_YUYV8_1_5X8, MEDIA_BUS_FMT_YUYV8_1_5X8,
	  MEDIA_BUS_FMT_YUYV8_1_5X8, MEDIA_BUS_FMT_YUYV8_1_5X8,
	  V4L2_PIX_FMT_NV12, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_YVYU8_1_5X8, MEDIA_BUS_FMT_YVYU8_1_5X8,
	  MEDIA_BUS_FMT_YVYU8_1_5X8, MEDIA_BUS_FMT_YVYU8_1_5X8,
	  V4L2_PIX_FMT_NV12, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_Y12_1X12, MEDIA_BUS_FMT_Y12_1X12,
	  MEDIA_BUS_FMT_Y12_1X12, MEDIA_BUS_FMT_Y12_1X12,
	  V4L2_PIX_FMT_NV12, 12, 2, 3, 2, }, //YUV420 1.5bpp
	//V4L2_PIX_FMT_NV21
	{ MEDIA_BUS_FMT_UYVY8_1_5X8, MEDIA_BUS_FMT_UYVY8_1_5X8,
	  MEDIA_BUS_FMT_UYVY8_1_5X8, MEDIA_BUS_FMT_UYVY8_1_5X8,
	  V4L2_PIX_FMT_NV21, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_VYUY8_1_5X8, MEDIA_BUS_FMT_VYUY8_1_5X8,
	  MEDIA_BUS_FMT_VYUY8_1_5X8, MEDIA_BUS_FMT_VYUY8_1_5X8,
	  V4L2_PIX_FMT_NV21, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_YUYV8_1_5X8, MEDIA_BUS_FMT_YUYV8_1_5X8,
	  MEDIA_BUS_FMT_YUYV8_1_5X8, MEDIA_BUS_FMT_YUYV8_1_5X8,
	  V4L2_PIX_FMT_NV21, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_YVYU8_1_5X8, MEDIA_BUS_FMT_YVYU8_1_5X8,
	  MEDIA_BUS_FMT_YVYU8_1_5X8, MEDIA_BUS_FMT_YVYU8_1_5X8,
	  V4L2_PIX_FMT_NV21, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_Y12_1X12, MEDIA_BUS_FMT_Y12_1X12,
	  MEDIA_BUS_FMT_Y12_1X12, MEDIA_BUS_FMT_Y12_1X12,
	  V4L2_PIX_FMT_NV21, 12, 2, 3, 2, }, //YUV420 1.5bpp
	//V4L2_PIX_FMT_YUV420
	{ MEDIA_BUS_FMT_UYVY8_1_5X8, MEDIA_BUS_FMT_UYVY8_1_5X8,
	  MEDIA_BUS_FMT_UYVY8_1_5X8, MEDIA_BUS_FMT_UYVY8_1_5X8,
	  V4L2_PIX_FMT_YUV420, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_VYUY8_1_5X8, MEDIA_BUS_FMT_VYUY8_1_5X8,
	  MEDIA_BUS_FMT_VYUY8_1_5X8, MEDIA_BUS_FMT_VYUY8_1_5X8,
	  V4L2_PIX_FMT_YUV420, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_YUYV8_1_5X8, MEDIA_BUS_FMT_YUYV8_1_5X8,
	  MEDIA_BUS_FMT_YUYV8_1_5X8, MEDIA_BUS_FMT_YUYV8_1_5X8,
	  V4L2_PIX_FMT_YUV420, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_YVYU8_1_5X8, MEDIA_BUS_FMT_YVYU8_1_5X8,
	  MEDIA_BUS_FMT_YVYU8_1_5X8, MEDIA_BUS_FMT_YVYU8_1_5X8,
	  V4L2_PIX_FMT_YUV420, 12, 2, 3, 2, }, //YUV420 1.5bpp
	{ MEDIA_BUS_FMT_Y12_1X12, MEDIA_BUS_FMT_Y12_1X12,
	  MEDIA_BUS_FMT_Y12_1X12, MEDIA_BUS_FMT_Y12_1X12,
	  V4L2_PIX_FMT_YUV420, 12, 2, 3, 2, }, //YUV420 1.5bpp
	//V4L2_PIX_FMT_ABGR32 argb 4bpp
	{ MEDIA_BUS_FMT_ARGB8888_1X32, MEDIA_BUS_FMT_ARGB8888_1X32,
	  MEDIA_BUS_FMT_ARGB8888_1X32, MEDIA_BUS_FMT_ARGB8888_1X32,
	  V4L2_PIX_FMT_ABGR32, 8, 1, 4, 1, }, //argb 4bpp
	//V4L2_PIX_FMT_ARGB32
	{ MEDIA_BUS_FMT_ARGB8888_1X32, MEDIA_BUS_FMT_ARGB8888_1X32,
	  MEDIA_BUS_FMT_ARGB8888_1X32, MEDIA_BUS_FMT_ARGB8888_1X32,
	  V4L2_PIX_FMT_ARGB32, 8, 1, 4, 1, }, //argb 4bpp
	//
	//V4L2_PIX_FMT_BGR24
	{ MEDIA_BUS_FMT_RBG888_1X24, MEDIA_BUS_FMT_RBG888_1X24,
	  MEDIA_BUS_FMT_RBG888_1X24, MEDIA_BUS_FMT_RBG888_1X24,
	  V4L2_PIX_FMT_BGR24, 8, 1, 3, 1, }, //argb 3bpp
	{ MEDIA_BUS_FMT_BGR888_1X24, MEDIA_BUS_FMT_BGR888_1X24,
	  MEDIA_BUS_FMT_BGR888_1X24, MEDIA_BUS_FMT_BGR888_1X24,
	  V4L2_PIX_FMT_BGR24, 8, 1, 3, 1, }, //argb 3bpp
	{ MEDIA_BUS_FMT_GBR888_1X24, MEDIA_BUS_FMT_GBR888_1X24,
	  MEDIA_BUS_FMT_GBR888_1X24, MEDIA_BUS_FMT_GBR888_1X24,
	  V4L2_PIX_FMT_BGR24, 8, 1, 3, 1, }, //argb 3bpp
	{ MEDIA_BUS_FMT_RGB888_1X24, MEDIA_BUS_FMT_RGB888_1X24,
	  MEDIA_BUS_FMT_RGB888_1X24, MEDIA_BUS_FMT_RGB888_1X24,
	  V4L2_PIX_FMT_BGR24, 8, 1, 3, 1, }, //argb 3bpp
	//V4L2_PIX_FMT_RGB24
	{ MEDIA_BUS_FMT_RBG888_1X24, MEDIA_BUS_FMT_RBG888_1X24,
	  MEDIA_BUS_FMT_RBG888_1X24, MEDIA_BUS_FMT_RBG888_1X24,
	  V4L2_PIX_FMT_RGB24, 8, 1, 3, 1, }, //argb 3bpp
	{ MEDIA_BUS_FMT_BGR888_1X24, MEDIA_BUS_FMT_BGR888_1X24,
	  MEDIA_BUS_FMT_BGR888_1X24, MEDIA_BUS_FMT_BGR888_1X24,
	  V4L2_PIX_FMT_RGB24, 8, 1, 3, 1, }, //argb 3bpp
	{ MEDIA_BUS_FMT_GBR888_1X24, MEDIA_BUS_FMT_GBR888_1X24,
	  MEDIA_BUS_FMT_GBR888_1X24, MEDIA_BUS_FMT_GBR888_1X24,
	  V4L2_PIX_FMT_RGB24, 8, 1, 3, 1, }, //argb 3bpp
	{ MEDIA_BUS_FMT_RGB888_1X24, MEDIA_BUS_FMT_RGB888_1X24,
	  MEDIA_BUS_FMT_RGB888_1X24, MEDIA_BUS_FMT_RGB888_1X24,
	  V4L2_PIX_FMT_RGB24, 8, 1, 3, 1, }, //argb 3bpp
	/* Empty entry to catch the unsupported pixel code (0) used by the isp
	 * module and avoid NULL pointer dereferences.
	 */
	{ 0, }
};

const struct k510isp_format_info *k510isp_video_format_info(u32 code)
{
	unsigned int i;
	//printk("%s:code(0x%x) start\n",__func__,code);
	for (i = 0; i < ARRAY_SIZE(formats); ++i) {
		if (formats[i].code == code)
		{
			//printk("%s:successed i(%d) (%d)end\n",__func__,i,(formats));
			return &formats[i];
		}	
	}
	printk("%s:failed (%d) (%d)end\n",__func__,i,ARRAY_SIZE(formats));
	return NULL;
}

/*
 * k510isp_video_mbus_to_pix - Convert v4l2_mbus_framefmt to v4l2_pix_format
 * @video: ISP video instance
 * @mbus: v4l2_mbus_framefmt format (input)
 * @pix: v4l2_pix_format format (output)
 *
 * Fill the output pix structure with information from the input mbus format.
 * The bytesperline and sizeimage fields are computed from the requested bytes
 * per line value in the pix format and information from the video instance.
 *
 * Return the number of padding bytes at end of line.
 */
static unsigned int k510isp_video_mbus_to_pix(const struct k510isp_video *video,
					  const struct v4l2_mbus_framefmt *mbus,
					  struct v4l2_pix_format *pix)
{
	unsigned int bpl = pix->bytesperline;
	unsigned int min_bpl;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(formats) - 1; ++i) {
		if (formats[i].pixelformat == pix->pixelformat)
			break;
	}

	if (WARN_ON(i == ARRAY_SIZE(formats)))
		return 0;

	if ((formats[i].bpp_numerator != 0) && (formats[i].bpp_denominator)) {
		min_bpl = pix->width * formats[i].bpp_numerator / formats[i].bpp_denominator;
	} else {
		min_bpl = pix->width * formats[i].bpp;
	}

	memset(pix, 0, sizeof(*pix));
	pix->width = mbus->width;
	pix->height = mbus->height;

//	memset(pix, 0, sizeof(*pix));
//	pix->width = mbus->width;
//	pix->height = mbus->height;

//	for (i = 0; i < ARRAY_SIZE(formats); ++i) {
//		if (formats[i].code == mbus->code)
//			break;
//	}

//	if (WARN_ON(i == ARRAY_SIZE(formats)))
//		return 0;

//	min_bpl = pix->width * formats[i].bpp;

	/* Clamp the requested bytes per line value. If the maximum bytes per
	 * line value is zero, the module doesn't support user configurable line
	 * sizes. Override the requested value with the minimum in that case.
	 */
	if (video->bpl_max)
		bpl = clamp(bpl, min_bpl, video->bpl_max);
	else
		bpl = min_bpl;

	if (!video->bpl_zero_padding || bpl != min_bpl)
		bpl = ALIGN(bpl, video->bpl_alignment);

	pix->pixelformat = formats[i].pixelformat;
	pix->bytesperline = bpl;
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->colorspace = mbus->colorspace;
	pix->field = mbus->field;

	return bpl - min_bpl;
}
/**
 * @brief 
 * 
 * @param pix 
 * @param mbus 
 */
static void k510isp_video_pix_to_mbus(const struct v4l2_pix_format *pix,struct v4l2_mbus_framefmt *mbus)
{
	unsigned int i;

	memset(mbus, 0, sizeof(*mbus));
	mbus->width = pix->width;
	mbus->height = pix->height;

	/* Skip the last format in the loop so that it will be selected if no
	 * match is found.
	 */
	for (i = 0; i < ARRAY_SIZE(formats) - 1; ++i) {
		if (formats[i].pixelformat == pix->pixelformat)
			break;
	}

	mbus->code = formats[i].code;
	mbus->colorspace = pix->colorspace;
	mbus->field = pix->field;
}
/**
 * @brief 
 * 
 * @param video 
 * @param pad 
 * @return struct v4l2_subdev* 
 */
static struct v4l2_subdev *k510isp_video_remote_subdev(struct k510isp_video *video, u32 *pad)
{
	struct media_pad *remote;
	dev_dbg(video->isp->dev,"%s:start video->pad(0x%x)\n",__func__,&video->pad);
	remote = media_entity_remote_pad(&video->pad);

	if (!remote || !is_media_entity_v4l2_subdev(remote->entity))
	{
		dev_err(video->isp->dev,"%s:remote(0x%x)\n",__func__,remote);
		return NULL;
	}	

	if (pad)
		*pad = remote->index;

	return media_entity_to_v4l2_subdev(remote->entity);
}
/**
 * @brief 
 * 
 */
/* Return a pointer to the ISP video instance at the far end of the pipeline. */
static int k510isp_video_get_graph_data(struct k510isp_video *video,struct k510isp_pipeline *pipe)
{
	struct media_graph graph;
	struct media_entity *entity = &video->video.entity;
	struct media_device *mdev = entity->graph_obj.mdev;
	struct k510isp_video *far_end = NULL;
	int ret;
	dev_info(video->isp->dev,"%s:start\n",__func__);
	mutex_lock(&mdev->graph_mutex);
	ret = media_graph_walk_init(&graph, mdev);
	if (ret) {
		mutex_unlock(&mdev->graph_mutex);
		return ret;
	}

	media_graph_walk_start(&graph, entity);

	while ((entity = media_graph_walk_next(&graph))) {
		struct k510isp_video *__video;

		media_entity_enum_set(&pipe->ent_enum, entity);

		if (far_end != NULL)
			continue;

		if (entity == &video->video.entity)
			continue;

		if (!is_media_entity_v4l2_video_device(entity))
			continue;

		__video = to_isp_video(media_entity_to_video_device(entity));
		if (__video->type != video->type)
			far_end = __video;
	}

	mutex_unlock(&mdev->graph_mutex);

	media_graph_walk_cleanup(&graph);

	if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		pipe->input = far_end;
		pipe->output = video;
	} else {
		if (far_end == NULL)
			return -EPIPE;

		pipe->input = video;
		pipe->output = far_end;
	}
	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	return 0;
}
/**
 * @brief 
 * 
 * @param video 
 * @param format 
 * @return int 
 */
static int __k510isp_video_get_format(struct k510isp_video *video, struct v4l2_format *format)
{
	struct v4l2_subdev_format fmt;
	struct v4l2_subdev *subdev;
	u32 pad;
	int ret;
	dev_dbg(video->isp->dev,"%s:start video->video.entity(0x%x)\n",__func__,&video->video.entity);
	subdev = k510isp_video_remote_subdev(video, &pad);
	if (subdev == NULL)
	{
		dev_err(video->isp->dev,"%s:k510isp_video_remote_subdev\n",__func__);
		return -EINVAL;
	}	

	fmt.pad = pad;
	fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;

	mutex_lock(&video->mutex);
	ret = v4l2_subdev_call(subdev, pad, get_fmt, NULL, &fmt);
	mutex_unlock(&video->mutex);
	if (ret)
	{
		dev_err(video->isp->dev,"%s:v4l2_subdev_call ret(%d)\n",__func__,ret);
		return ret;
	}	

	format->type = video->type;
	return k510isp_video_mbus_to_pix(video, &fmt.format, &format->fmt.pix);
}
/**
 * @brief 
 * 
 * @param video 
 * @param vfh 
 * @return int 
 */
static int k510isp_video_check_format(struct k510isp_video *video, struct k510isp_video_fh *vfh)
{
	struct v4l2_format format;
	int ret;
	dev_info(video->isp->dev,"%s:start\n",__func__);
	memcpy(&format, &vfh->format, sizeof(format));
	ret = __k510isp_video_get_format(video, &format);
	if (ret < 0)
	{
		dev_err(video->isp->dev,"%s:__k510isp_video_get_format ret(%d)\n",__func__,ret);
		return ret;
	}	

	if (vfh->format.fmt.pix.pixelformat != format.fmt.pix.pixelformat ||
	    vfh->format.fmt.pix.height != format.fmt.pix.height ||
	    vfh->format.fmt.pix.width != format.fmt.pix.width ||
	    vfh->format.fmt.pix.bytesperline != format.fmt.pix.bytesperline ||
	    vfh->format.fmt.pix.sizeimage != format.fmt.pix.sizeimage ||
	    vfh->format.fmt.pix.field != format.fmt.pix.field)
	{
		dev_err(video->isp->dev,"%s:pixelformat(vfh 0x%x),(format 0x%x)\n",__func__,vfh->format.fmt.pix.pixelformat,format.fmt.pix.pixelformat);
		dev_err(video->isp->dev,"%s:height(vfh %d),(format %d)\n",__func__,vfh->format.fmt.pix.height,format.fmt.pix.height);
		dev_err(video->isp->dev,"%s:width(vfh %d),(format %d)\n",__func__,vfh->format.fmt.pix.width,format.fmt.pix.width);
		dev_err(video->isp->dev,"%s:bytesperline(vfh %d),(format %d)\n",__func__,vfh->format.fmt.pix.bytesperline,format.fmt.pix.bytesperline);
		dev_err(video->isp->dev,"%s:sizeimage(vfh %d),(format %d)\n",__func__,vfh->format.fmt.pix.sizeimage,format.fmt.pix.sizeimage);
		dev_err(video->isp->dev,"%s:field(vfh %d),(format %d)\n",__func__,vfh->format.fmt.pix.field,format.fmt.pix.field);
		return -EINVAL;
	}	

	return 0;
}
/* -----------------------------------------------------------------------------
 * Video queue operations
 *-----------------------------------------------------------------------------*/

/**
 * @brief 
 * 
 * @param queue 
 * @param count 
 * @param num_planes 
 * @param sizes 
 * @param alloc_devs 
 * @return int 
 */
static int k510isp_video_queue_setup(struct vb2_queue *queue,
				 unsigned int *count, unsigned int *num_planes,
				 unsigned int sizes[], struct device *alloc_devs[])
{
	struct k510isp_video_fh *vfh = vb2_get_drv_priv(queue);
	struct k510isp_video *video = vfh->video;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);

	*num_planes = 1;
	sizes[0] = vfh->format.fmt.pix.sizeimage;
	if (sizes[0] == 0)
	{
		dev_err(video->isp->dev,"%s:ret(%d)\n",__func__,-EINVAL);
		return -EINVAL;
	}	

	if(queue->memory == VB2_MEMORY_MMAP)
	{
		*count = min(*count, video->capture_mem / PAGE_ALIGN(sizes[0]));
	}

	return 0;
}
/**
 * @brief 
 * 
 * @param buf 
 * @return int 
 */
static int k510isp_video_buffer_prepare(struct vb2_buffer *buf)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
	struct k510isp_video_fh *vfh = vb2_get_drv_priv(buf->vb2_queue);
	struct k510isp_buffer *buffer = to_isp_buffer(vbuf);
	struct k510isp_video *video = vfh->video;
	dma_addr_t addr;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	/* Refuse to prepare the buffer is the video node has registered an
	 * error. We don't need to take any lock here as the operation is
	 * inherently racy. The authoritative check will be performed in the
	 * queue handler, which can't return an error, this check is just a best
	 * effort to notify userspace as early as possible.
	 */
	if (unlikely(video->error))
	{
		dev_err(video->isp->dev,"%s:video->error(%d)\n",__func__,-EIO);
		return -EIO;
	}	

	addr = vb2_dma_contig_plane_dma_addr(buf, 0);
	if (!IS_ALIGNED(addr, 32)) {
		dev_err(video->isp->dev,
			"Buffer address must be aligned to 32 bytes boundary.\n");
		return -EINVAL;
	}

	vb2_set_plane_payload(&buffer->vb.vb2_buf, 0,
			      vfh->format.fmt.pix.sizeimage);
	buffer->dma = addr;
	buffer->image_size = vfh->format.fmt.pix.sizeimage;
	buffer->buf_size = video->buf_size;
	//dev_info(video->isp->dev,"%s:image_size(%d),buf_size(%d)\n",__func__,buffer->image_size,buffer->buf_size);  
	return 0;
}

/*
 * k510isp_video_buffer_queue - Add buffer to streaming queue
 * @buf: Video buffer
 *
 * In memory-to-memory mode, start streaming on the pipeline if buffers are
 * queued on both the input and the output, if the pipeline isn't already busy.
 * If the pipeline is busy, it will be restarted in the output module interrupt
 * handler.
 */
static void k510isp_video_buffer_queue(struct vb2_buffer *buf)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(buf);
	struct k510isp_video_fh *vfh = vb2_get_drv_priv(buf->vb2_queue);
	struct k510isp_buffer *buffer = to_isp_buffer(vbuf);
	struct k510isp_video *video = vfh->video;
	struct k510isp_pipeline *pipe = to_isp_pipeline(&video->video.entity);
	enum k510isp_pipeline_state state;
	unsigned long flags;
	unsigned int empty;
	unsigned int start;

	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	spin_lock_irqsave(&video->irqlock, flags);

	if (unlikely(video->error)) {
		vb2_buffer_done(&buffer->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		spin_unlock_irqrestore(&video->irqlock, flags);
		dev_err(video->isp->dev,"%s:video->error\n",__func__);
		return;
	}

	empty = list_empty(&video->dmaqueue);
	list_add_tail(&buffer->irqlist, &video->dmaqueue);

	spin_unlock_irqrestore(&video->irqlock, flags);

	if (empty) {
		dev_dbg(video->isp->dev,"%s:empty!\n",__func__);
		if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
			state = ISP_PIPELINE_QUEUE_OUTPUT;
		else
			state = ISP_PIPELINE_QUEUE_INPUT;

		spin_lock_irqsave(&pipe->lock, flags);
		pipe->state |= state;
		video->ops->queue(video, buffer);
		video->dmaqueue_flags |= ISP_VIDEO_DMAQUEUE_QUEUED;

		start = k510isp_pipeline_ready(pipe);
		if (start)
			pipe->state |= ISP_PIPELINE_STREAM;
		spin_unlock_irqrestore(&pipe->lock, flags);

		if (start)
			k510isp_pipeline_set_stream(pipe,ISP_PIPELINE_STREAM_SINGLESHOT,video);
	}
}

/*
 * k510isp_video_return_buffers - Return all queued buffers to videobuf2
 * @video: ISP video object
 * @state: new state for the returned buffers
 *
 * Return all buffers queued on the video node to videobuf2 in the given state.
 * The buffer state should be VB2_BUF_STATE_QUEUED if called due to an error
 * when starting the stream, or VB2_BUF_STATE_ERROR otherwise.
 *
 * The function must be called with the video irqlock held.
 */
static void k510isp_video_return_buffers(struct k510isp_video *video,
					  enum vb2_buffer_state state)
{
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	while (!list_empty(&video->dmaqueue)) {
		dev_dbg(video->isp->dev,"%s:list_empty\n",__func__);
		struct k510isp_buffer *buf;

		buf = list_first_entry(&video->dmaqueue,
				       struct k510isp_buffer, irqlist);
		list_del(&buf->irqlist);
		vb2_buffer_done(&buf->vb.vb2_buf, state);
	}
}
/**
 * @brief 
 * 
 * @param queue 
 * @param count 
 * @return int 
 */
static int k510isp_video_start_streaming(struct vb2_queue *queue,
				     unsigned int count)
{
	struct k510isp_video_fh *vfh = vb2_get_drv_priv(queue);
	struct k510isp_video *video = vfh->video;
	struct k510isp_pipeline *pipe = to_isp_pipeline(&video->video.entity);
	unsigned long flags;
	int ret;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	/* In sensor-to-memory mode, the stream can be started synchronously
	 * to the stream on command. In memory-to-memory mode, it will be
	 * started when buffers are queued on both the input and output.
	 */
	if (pipe->input)
		return 0;

	ret = k510isp_pipeline_set_stream(pipe,
					   ISP_PIPELINE_STREAM_CONTINUOUS,video);
	if (ret < 0) {
		dev_err(video->isp->dev,"%s:ret(%d)\n",__func__,ret);
		spin_lock_irqsave(&video->irqlock, flags);
		k510isp_video_return_buffers(video, VB2_BUF_STATE_QUEUED);
		spin_unlock_irqrestore(&video->irqlock, flags);
		return ret;
	}

	spin_lock_irqsave(&video->irqlock, flags);
	if (list_empty(&video->dmaqueue))
		video->dmaqueue_flags |= ISP_VIDEO_DMAQUEUE_UNDERRUN;
	spin_unlock_irqrestore(&video->irqlock, flags);

	return 0;
}
/**
 * @brief 
 * 
 */
static const struct vb2_ops k510isp_video_queue_ops = {
	.queue_setup = k510isp_video_queue_setup,
	.buf_prepare = k510isp_video_buffer_prepare,
	.buf_queue = k510isp_video_buffer_queue,
	.start_streaming = k510isp_video_start_streaming,
};
/*
 * k510isp_video_buffer_next - Complete the current buffer and return the next
 * @video: ISP video object
 *
 * Remove the current video buffer from the DMA queue and fill its timestamp and
 * field count before handing it back to videobuf2.
 *
 * For capture video nodes the buffer state is set to VB2_BUF_STATE_DONE if no
 * error has been flagged in the pipeline, or to VB2_BUF_STATE_ERROR otherwise.
 * For video output nodes the buffer state is always set to VB2_BUF_STATE_DONE.
 *
 * The DMA queue is expected to contain at least one buffer.
 *
 * Return a pointer to the next buffer in the DMA queue, or NULL if the queue is
 * empty.
 */
struct k510isp_buffer *k510isp_video_buffer_next(struct k510isp_video *video)
{
	struct k510isp_pipeline *pipe = to_isp_pipeline(&video->video.entity);
	enum vb2_buffer_state vb_state;
	struct k510isp_buffer *buf;
	unsigned long flags;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	spin_lock_irqsave(&video->irqlock, flags);
	if (WARN_ON(list_empty(&video->dmaqueue))) {
		dev_err(video->isp->dev,"%s:WARN_ONlist_empty\n",__func__);
		spin_unlock_irqrestore(&video->irqlock, flags);
		return NULL;
	}

	if (list_is_singular(&video->dmaqueue)) {//only one buffer, for isp, return NULL
		spin_unlock_irqrestore(&video->irqlock, flags);
		return NULL;
	}

	buf = list_first_entry(&video->dmaqueue, struct k510isp_buffer,
			       irqlist);
	list_del(&buf->irqlist);
	spin_unlock_irqrestore(&video->irqlock, flags);

	buf->vb.vb2_buf.timestamp = ktime_get_ns();

	/* Do frame number propagation only if this is the output video node.
	 * Frame number either comes from the CSI receivers or it gets
	 * incremented here if H3A is not active.
	 * Note: There is no guarantee that the output buffer will finish
	 * first, so the input number might lag behind by 1 in some cases.
	 */
	if (video == pipe->output && !pipe->do_propagation)
		buf->vb.sequence =
			atomic_inc_return(&pipe->frame_number);
	else
		buf->vb.sequence = atomic_read(&pipe->frame_number);

	if (pipe->field != V4L2_FIELD_NONE)
		buf->vb.sequence /= 2;

	buf->vb.field = pipe->field;

	/* Report pipeline errors to userspace on the capture device side. */
	if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE && pipe->error) {
		vb_state = VB2_BUF_STATE_ERROR;
		pipe->error = false;
	} else {
		vb_state = VB2_BUF_STATE_DONE;
	}

	vb2_buffer_done(&buf->vb.vb2_buf, vb_state);

	spin_lock_irqsave(&video->irqlock, flags);

	if (list_empty(&video->dmaqueue)) {
		enum k510isp_pipeline_state state;

		spin_unlock_irqrestore(&video->irqlock, flags);

		if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
			state = ISP_PIPELINE_QUEUE_OUTPUT
			      | ISP_PIPELINE_STREAM;
		else
			state = ISP_PIPELINE_QUEUE_INPUT
			      | ISP_PIPELINE_STREAM;

		spin_lock_irqsave(&pipe->lock, flags);
		pipe->state &= ~state;
		if (video->pipe.stream_state == ISP_PIPELINE_STREAM_CONTINUOUS)
			video->dmaqueue_flags |= ISP_VIDEO_DMAQUEUE_UNDERRUN;
		spin_unlock_irqrestore(&pipe->lock, flags);
		dev_err(video->isp->dev,"%s:list_empty\n",__func__);
		return NULL;
	}

	if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE && pipe->input != NULL) {
		spin_lock(&pipe->lock);
		pipe->state &= ~ISP_PIPELINE_STREAM;
		spin_unlock(&pipe->lock);
	}

	buf = list_first_entry(&video->dmaqueue, struct k510isp_buffer,
			       irqlist);

	spin_unlock_irqrestore(&video->irqlock, flags);

	return buf;
}

/*
 * k510isp_video_cancel_stream - Cancel stream on a video node
 * @video: ISP video object
 *
 * Cancelling a stream returns all buffers queued on the video node to videobuf2
 * in the erroneous state and makes sure no new buffer can be queued.
 */
void k510isp_video_cancel_stream(struct k510isp_video *video)
{
	unsigned long flags;

	spin_lock_irqsave(&video->irqlock, flags);
	k510isp_video_return_buffers(video, VB2_BUF_STATE_ERROR);
	video->error = true;
	spin_unlock_irqrestore(&video->irqlock, flags);
}

/*
 * k510isp_video_resume - Perform resume operation on the buffers
 * @video: ISP video object
 * @continuous: Pipeline is in single shot mode if 0 or continuous mode otherwise
 *
 * This function is intended to be used on suspend/resume scenario. It
 * requests video queue layer to discard buffers marked as DONE if it's in
 * continuous mode and requests ISP modules to queue again the ACTIVE buffer
 * if there's any.
 */
void k510isp_video_resume(struct k510isp_video *video, int continuous)
{
	struct k510isp_buffer *buf = NULL;

	if (continuous && video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		mutex_lock(&video->queue_lock);
		vb2_discard_done(video->queue);
		mutex_unlock(&video->queue_lock);
	}

	if (!list_empty(&video->dmaqueue)) {
		buf = list_first_entry(&video->dmaqueue,
				       struct k510isp_buffer, irqlist);
		video->ops->queue(video, buf);
		video->dmaqueue_flags |= ISP_VIDEO_DMAQUEUE_QUEUED;
	} else {
		if (continuous)
			video->dmaqueue_flags |= ISP_VIDEO_DMAQUEUE_UNDERRUN;
	}
}

/* -----------------------------------------------------------------------------
 * V4L2 ioctls
 *-----------------------------------------------------------------------------*/

static int k510isp_video_querycap(struct file *file, void *fh, struct v4l2_capability *cap)
{
	struct k510isp_video *video = video_drvdata(file);
	dev_dbg(video->isp->dev,"%s:video->type %d start\n",__func__,video->type);
	strlcpy(cap->driver, K510ISP_VIDEO_DRIVER_NAME, sizeof(cap->driver));
	strlcpy(cap->card, video->video.name, sizeof(cap->card));
	strlcpy(cap->bus_info, "media", sizeof(cap->bus_info));

	//cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT
	//	| V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT_MPLANE|V4L2_CAP_DEVICE_CAPS;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT
	| V4L2_CAP_STREAMING | V4L2_CAP_DEVICE_CAPS;

	if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	//else if(video->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
	//	cap->device_caps = V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
	else
		cap->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;

	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param format 
 * @return int 
 */
static int k510isp_video_get_format(struct file *file, void *fh, struct v4l2_format *format)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);

	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	if (format->type != video->type)
	{
		dev_err(video->isp->dev,"%s:format->type(%d),video->type (%d)\n",format->type,video->type);
		return -EINVAL;
	}	

	mutex_lock(&video->mutex);
	*format = vfh->format;
	mutex_unlock(&video->mutex);

	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param format 
 * @return int 
 */
static int k510isp_video_set_format(struct file *file, void *fh, struct v4l2_format *format)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	struct v4l2_mbus_framefmt fmt;

	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	if (format->type != video->type)
	{	
		dev_err(video->isp->dev,"%s:format->type 0x%x,video->type 0x%x\n",__func__,format->type,video->type);
		return -EINVAL;
	}
	dev_dbg(video->isp->dev,"%s:pixelformat(0x%x),width(%d),height(%d),bytesperline(%d),start\n",__func__,format->fmt.pix.pixelformat,format->fmt.pix.width,format->fmt.pix.height,format->fmt.pix.bytesperline);
	/* Replace unsupported field orders with sane defaults. */
	switch (format->fmt.pix.field) {
	case V4L2_FIELD_NONE:
		/* Progressive is supported everywhere. */
		break;
	case V4L2_FIELD_ALTERNATE:
		/* ALTERNATE is not supported on output nodes. */
		if (video->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
			format->fmt.pix.field = V4L2_FIELD_NONE;
		break;
	case V4L2_FIELD_INTERLACED:
		/* The ISP has no concept of video standard, select the
		 * top-bottom order when the unqualified interlaced order is
		 * requested.
		 */
		format->fmt.pix.field = V4L2_FIELD_INTERLACED_TB;
		/* Fall-through */
	case V4L2_FIELD_INTERLACED_TB:
	case V4L2_FIELD_INTERLACED_BT:
		/* Interlaced orders are only supported at the CCDC output. */
		//if (video != &video->isp->isp_ccdc.video_out)
		//	format->fmt.pix.field = V4L2_FIELD_NONE;
		break;
	case V4L2_FIELD_TOP:
	case V4L2_FIELD_BOTTOM:
	case V4L2_FIELD_SEQ_TB:
	case V4L2_FIELD_SEQ_BT:
	default:
		/* All other field orders are currently unsupported, default to
		 * progressive.
		 */
		format->fmt.pix.field = V4L2_FIELD_NONE;
		break;
	}

	/* Fill the bytesperline and sizeimage fields by converting to media bus
	 * format and back to pixel format.
	 */
	k510isp_video_pix_to_mbus(&format->fmt.pix, &fmt);
	k510isp_video_mbus_to_pix(video, &fmt, &format->fmt.pix);

	dev_dbg(video->isp->dev,"%s:pixelformat(0x%x),width(%d),height(%d),bytesperline(%d),end\n",__func__,format->fmt.pix.pixelformat,format->fmt.pix.width,format->fmt.pix.height,format->fmt.pix.bytesperline);
	mutex_lock(&video->mutex);
	vfh->format = *format;
	mutex_unlock(&video->mutex);
	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param format 
 * @return int 
 */

static int k510isp_video_enum_format(struct file *file, void *fh,struct v4l2_fmtdesc *f)
{
	struct k510isp_video *video = video_drvdata(file);
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_subdev *subdev;
	u32 pad;
	int ret = 0;

	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	
	
	return ret;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param format 
 * @return int 
 */
static int k510isp_video_try_format(struct file *file, void *fh, struct v4l2_format *format)
{
	struct k510isp_video *video = video_drvdata(file);
	struct v4l2_subdev_format fmt;
	struct v4l2_subdev *subdev;
	u32 pad;
	int ret;

	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	if (format->type != video->type)
		return -EINVAL;

	subdev = k510isp_video_remote_subdev(video, &pad);
	if (subdev == NULL)
		return -EINVAL;

	k510isp_video_pix_to_mbus(&format->fmt.pix, &fmt.format);

	fmt.pad = pad;
	fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	ret = v4l2_subdev_call(subdev, pad, get_fmt, NULL, &fmt);
	if (ret)
	{
		dev_err(video->isp->dev,"%s:v4l2_subdev_call ret %d\n",__func__,ret);
		return ret == -ENOIOCTLCMD ? -ENOTTY : ret;
	}
	k510isp_video_mbus_to_pix(video, &fmt.format, &format->fmt.pix);
	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param sel 
 * @return int 
 */
static int k510isp_video_get_selection(struct file *file, void *fh, struct v4l2_selection *sel)
{
	struct k510isp_video *video = video_drvdata(file);
	struct v4l2_subdev_format format;
	struct v4l2_subdev *subdev;
	struct v4l2_subdev_selection sdsel = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
		.target = sel->target,
	};
	u32 pad;
	int ret;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
		if (video->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
			return -EINVAL;
		break;
	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
		if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	subdev = k510isp_video_remote_subdev(video, &pad);
	if (subdev == NULL)
		return -EINVAL;

	/* Try the get selection operation first and fallback to get format if not
	 * implemented.
	 */
	sdsel.pad = pad;
	ret = v4l2_subdev_call(subdev, pad, get_selection, NULL, &sdsel);
	if (!ret)
		sel->r = sdsel.r;
	if (ret != -ENOIOCTLCMD)
		return ret;

	format.pad = pad;
	format.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	ret = v4l2_subdev_call(subdev, pad, get_fmt, NULL, &format);
	if (ret < 0)
		return ret == -ENOIOCTLCMD ? -ENOTTY : ret;

	sel->r.left = 0;
	sel->r.top = 0;
	sel->r.width = format.format.width;
	sel->r.height = format.format.height;

	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param sel 
 * @return int 
 */
static int k510isp_video_set_selection(struct file *file, void *fh, struct v4l2_selection *sel)
{
	struct k510isp_video *video = video_drvdata(file);
	struct v4l2_subdev *subdev;
	struct v4l2_subdev_selection sdsel = {
		.which = V4L2_SUBDEV_FORMAT_ACTIVE,
		.target = sel->target,
		.flags = sel->flags,
		.r = sel->r,
	};
	u32 pad;
	int ret;
	
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	switch (sel->target) {
	case V4L2_SEL_TGT_CROP:
		if (video->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
			return -EINVAL;
		break;
	case V4L2_SEL_TGT_COMPOSE:
		if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}
	subdev = k510isp_video_remote_subdev(video, &pad);
	if (subdev == NULL)
		return -EINVAL;

	sdsel.pad = pad;
	mutex_lock(&video->mutex);
	ret = v4l2_subdev_call(subdev, pad, set_selection, NULL, &sdsel);
	mutex_unlock(&video->mutex);
	if (!ret)
		sel->r = sdsel.r;
	dev_dbg(video->isp->dev,"%s:end ret(%d)\n",__func__,ret);
	return ret == -ENOIOCTLCMD ? -ENOTTY : ret;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param a 
 * @return int 
 */
static int k510isp_video_get_param(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	if (video->type != V4L2_BUF_TYPE_VIDEO_OUTPUT ||video->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
	    video->type != a->type)
	{
		dev_err(video->isp->dev,"%s:video->type(%d)\n",__func__,video->type);
		return -EINVAL;
	}	

	memset(a, 0, sizeof(*a));
	a->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	a->parm.output.capability = V4L2_CAP_TIMEPERFRAME;
	a->parm.output.timeperframe = vfh->timeperframe;
//	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param a 
 * @return int 
 */
static int k510isp_video_set_param(struct file *file, void *fh, struct v4l2_streamparm *a)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	if (video->type != V4L2_BUF_TYPE_VIDEO_OUTPUT || video->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    video->type != a->type)
		return -EINVAL;

	if (a->parm.output.timeperframe.denominator == 0)
		a->parm.output.timeperframe.denominator = 1;

	vfh->timeperframe = a->parm.output.timeperframe;
	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param rb 
 * @return int 
 */
static int k510isp_video_reqbufs(struct file *file, void *fh, struct v4l2_requestbuffers *rb)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	int ret;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	mutex_lock(&video->queue_lock);
	ret = vb2_reqbufs(&vfh->queue, rb);
	mutex_unlock(&video->queue_lock);
//	dev_dbg(video->isp->dev,"%s:end ret(%d)\n",__func__,ret);
	return ret;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param b 
 * @return int 
 */
static int k510isp_video_querybuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	int ret;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	mutex_lock(&video->queue_lock);
	ret = vb2_querybuf(&vfh->queue, b);
	mutex_unlock(&video->queue_lock);
//	dev_dbg(video->isp->dev,"%s:end ret(%d)\n",__func__,ret);
	return ret;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param b 
 * @return int 
 */
static int k510isp_video_qbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	int ret;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	mutex_lock(&video->queue_lock);
	ret = vb2_qbuf(&vfh->queue, b);
	mutex_unlock(&video->queue_lock);
	video->buf_size = b->length;
//	dev_dbg(video->isp->dev,"%s:end ret(%d)\n",__func__,ret);
	return ret;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param b 
 * @return int 
 */
static int k510isp_video_dqbuf(struct file *file, void *fh, struct v4l2_buffer *b)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	int ret;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	mutex_lock(&video->queue_lock);
	ret = vb2_dqbuf(&vfh->queue, b, file->f_flags & O_NONBLOCK);
	mutex_unlock(&video->queue_lock);

	return ret;
}
/**
 * @brief 
 * 
 * @param video 
 * @param pipe 
 * @return int 
 */
unsigned int first_bmap_val = 0;
unsigned int fisrt_video = 0;
static int k510isp_video_check_external_subdevs(struct k510isp_video *video,struct k510isp_pipeline *pipe)
{
	struct k510_isp_device *isp = video->isp;
	struct media_entity *ents[] = {
		&isp->isp_csi2.subdev.entity,
		&isp->isp_vi.subdev.entity,
		&isp->isp_f2k.subdev.entity,
		&isp->isp_r2k.subdev.entity
	};
	struct media_pad *source_pad;
	struct media_entity *source = NULL;
	struct media_entity *sink;
	struct v4l2_subdev_format fmt;
	struct v4l2_ext_controls ctrls;
	struct v4l2_ext_control ctrl;
	unsigned int i;
	int ret;

	dev_info(video->isp->dev,"%s:start\n",__func__);
	/* Memory-to-memory pipelines have no external subdev. */
	if (pipe->input != NULL)
		return 0;

	unsigned long *first_bmap = pipe->ent_enum.bmap;
	if( fisrt_video == 0)
	{
		fisrt_video = 1;
		first_bmap_val = *first_bmap;
	}
	else
	{
		*first_bmap = first_bmap_val;
	}
	dev_info(video->isp->dev,"%s:fisrt_video(%d),first_bmap_val(0x%x),first_bmap(0x%x,%p)\n",__func__,fisrt_video,first_bmap_val,*first_bmap,first_bmap);
	for (i = 0; i < ARRAY_SIZE(ents); i++) {
		/* Is the entity part of the pipeline? */
		if (!media_entity_enum_test(&pipe->ent_enum, ents[i]))
			continue;

		/* ISP entities have always sink pad == 0. Find source. */
		source_pad = media_entity_remote_pad(&ents[i]->pads[0]);
		if (source_pad == NULL)
			continue;

		source = source_pad->entity;
		sink = ents[i];
		break;
	}

	if (!source) {
		dev_warn(isp->dev,"%s:(%d) can't find source, failing now\n",__func__,i);
		return -EINVAL;
	}

	if (!is_media_entity_v4l2_subdev(source))
		return 0;

	pipe->external = media_entity_to_v4l2_subdev(source);

	fmt.pad = source_pad->index;
	fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	ret = v4l2_subdev_call(media_entity_to_v4l2_subdev(sink),
			       pad, get_fmt, NULL, &fmt);
	if (unlikely(ret < 0)) {
		dev_warn(isp->dev, "%s:get_fmt returned null!\n",__func__);
		return ret;
	}
	//dev_info(video->isp->dev,"%s:k510isp_video_format_info\n",__func__);
	pipe->external_width =
		k510isp_video_format_info(fmt.format.code)->width;

	memset(&ctrls, 0, sizeof(ctrls));
	memset(&ctrl, 0, sizeof(ctrl));

	ctrl.id = V4L2_CID_PIXEL_RATE;

	ctrls.count = 1;
	ctrls.controls = &ctrl;

	ret = v4l2_g_ext_ctrls(pipe->external->ctrl_handler, &ctrls);
	if (ret < 0) {
		dev_warn(isp->dev, "%s:no pixel rate control in subdev %s\n",__func__,
			 pipe->external->name);
		return ret;
	}

	pipe->external_rate = ctrl.value64;

	if (media_entity_enum_test(&pipe->ent_enum,
				   &isp->isp_f2k.subdev.entity)) {
		unsigned int rate = UINT_MAX;

	}

	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	return 0;
}
/*-----------------------------------------------------------------------------
 * Stream management
 *
 * Every ISP pipeline has a single input and a single output. The input can be
 * either a sensor or a video node. The output is always a video node.
 *
 * As every pipeline has an output video node, the ISP video objects at the
 * pipeline output stores the pipeline state. It tracks the streaming state of
 * both the input and output, as well as the availability of buffers.
 *
 * In sensor-to-memory mode, frames are always available at the pipeline input.
 * Starting the sensor usually requires I2C transfers and must be done in
 * interruptible context. The pipeline is started and stopped synchronously
 * to the stream on/off commands. All modules in the pipeline will get their
 * subdev set stream handler called. The module at the end of the pipeline must
 * delay starting the hardware until buffers are available at its output.
 *
 * In memory-to-memory mode, starting/stopping the stream requires
 * synchronization between the input and output. ISP modules can't be stopped
 * in the middle of a frame, and at least some of the modules seem to become
 * busy as soon as they're started, even if they don't receive a frame start
 * event. For that reason frames need to be processed in single-shot mode. The
 * driver needs to wait until a frame is completely processed and written to
 * memory before restarting the pipeline for the next frame. Pipelined
 * processing might be possible but requires more testing.
 *
 * Stream start must be delayed until buffers are available at both the input
 * and output. The pipeline must be started in the videobuf queue callback with
 * the buffers queue spinlock held. The modules subdev set stream operation must
 * not sleep.
 *-----------------------------------------------------------------------------*/

/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param type 
 * @return int 
 */
static int k510isp_video_streamon(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	enum k510isp_pipeline_state state;
	struct k510isp_pipeline *pipe;
	unsigned long flags;
	int ret;

	dev_info(video->isp->dev,"%s:start\n",__func__);

	if (type != video->type)
	{
		dev_err(video->isp->dev,"%s:type(0x%x),video->type(0x%x),ret(%d)\n",__func__,type,video->type,-EINVAL);
		return -EINVAL;
	}	

	mutex_lock(&video->stream_lock);

	/* Start streaming on the pipeline. No link touching an entity in the
	 * pipeline can be activated or deactivated once streaming is started.
	 */
	pipe = video->video.entity.pipe
	     ? to_isp_pipeline(&video->video.entity) : &video->pipe;
	ret = media_entity_enum_init(&pipe->ent_enum, &video->isp->media_dev);
	if (ret)
	{
		dev_err(video->isp->dev,"%s:media_entity_enum_init pipe->ent_enum (%d),ret(%d)\n",__func__,pipe->ent_enum,ret);
		goto err_enum_init;
	}
	/* TODO: Implement PM QoS */
	//pipe->l3_ick = clk_get_rate(video->isp->clock[ISP_CLK_L3_ICK]);
	//pipe->max_rate = pipe->l3_ick;

	ret = media_pipeline_start(&video->video.entity, &pipe->pipe);
	if (ret < 0)
	{
		dev_err(video->isp->dev,"%s:media_pipeline_start ret(%d)\n",__func__,ret);
		goto err_pipeline_start;
	}	

	/* Verify that the currently configured format matches the output of
	 * the connected subdev.
	 */
	ret = k510isp_video_check_format(video, vfh);
	if (ret < 0)
	{
		dev_err(video->isp->dev,"%s:k510isp_video_check_format ret(%d)\n",__func__,ret);
		goto err_check_format;
	}	
	video->bpl_padding = ret;
	video->bpl_value = vfh->format.fmt.pix.bytesperline;

	ret = k510isp_video_get_graph_data(video, pipe);
	if (ret < 0)
	{
		dev_err(video->isp->dev,"%s:k510isp_video_get_graph_data ret(%d)\n",__func__,ret);
		goto err_check_format;
	}	

	if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		state = ISP_PIPELINE_STREAM_OUTPUT | ISP_PIPELINE_IDLE_OUTPUT;
	else
		state = ISP_PIPELINE_STREAM_INPUT | ISP_PIPELINE_IDLE_INPUT;

	ret = k510isp_video_check_external_subdevs(video, pipe);
	if (ret < 0)
	{
		dev_err(video->isp->dev,"%s:k510isp_video_check_external_subdevs ret(%d)\n",__func__,ret);
		goto err_check_format;
	}	

	pipe->error = false;

	spin_lock_irqsave(&pipe->lock, flags);
	pipe->state &= ~ISP_PIPELINE_STREAM;
	pipe->state |= state;
	spin_unlock_irqrestore(&pipe->lock, flags);

	/* Set the maximum time per frame as the value requested by userspace.
	 * This is a soft limit that can be overridden if the hardware doesn't
	 * support the request limit.
	 */
	if (video->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
		pipe->max_timeperframe = vfh->timeperframe;
	else if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		pipe->max_timeperframe = vfh->timeperframe;
	else if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		pipe->max_timeperframe = vfh->timeperframe;

	video->queue = &vfh->queue;
	INIT_LIST_HEAD(&video->dmaqueue);
	atomic_set(&pipe->frame_number, -1);
	pipe->field = vfh->format.fmt.pix.field;

	mutex_lock(&video->queue_lock);
	ret = vb2_streamon(&vfh->queue, type);
	mutex_unlock(&video->queue_lock);
	if (ret < 0)
	{
		dev_err(video->isp->dev,"%s:vb2_streamon ret(%d)\n",__func__,ret);
		goto err_check_format;
	}	

	
	mutex_unlock(&video->stream_lock);

	return 0;

err_check_format:
	media_pipeline_stop(&video->video.entity);
err_pipeline_start:
	/* TODO: Implement PM QoS */
	/* The DMA queue must be emptied here, otherwise CCDC interrupts that
	 * will get triggered the next time the CCDC is powered up will try to
	 * access buffers that might have been freed but still present in the
	 * DMA queue. This can easily get triggered if the above
	 * k510isp_pipeline_set_stream() call fails on a system with a
	 * free-running sensor.
	 */
	INIT_LIST_HEAD(&video->dmaqueue);
	video->queue = NULL;

	media_entity_enum_cleanup(&pipe->ent_enum);

err_enum_init:
	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	mutex_unlock(&video->stream_lock);

	return ret;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param type 
 * @return int 
 */
static int k510isp_video_streamoff(struct file *file, void *fh, enum v4l2_buf_type type)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(fh);
	struct k510isp_video *video = video_drvdata(file);
	struct k510isp_pipeline *pipe = to_isp_pipeline(&video->video.entity);
	enum k510isp_pipeline_state state;
	unsigned int streaming;
	unsigned long flags;

	dev_info(video->isp->dev,"%s:video(%p) video(%p) pipe(%p) start\n",__func__,video,&video->video,pipe);

	if (type != video->type)
	{
		dev_err(video->isp->dev,"%s:type(0x%x),video->type(0x%x),ret(%d)\n",__func__,type,video->type,EINVAL);
		return -EINVAL;
	}
	mutex_lock(&video->stream_lock);

	/* Make sure we're not streaming yet. */
	mutex_lock(&video->queue_lock);
	streaming = vb2_is_streaming(&vfh->queue);
	mutex_unlock(&video->queue_lock);

	if (!streaming)
	{	
		dev_err(video->isp->dev,"%s:streaming%d err!\n",__func__,streaming);
		goto done;
	}
	/* Update the pipeline state. */
	if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		state = ISP_PIPELINE_STREAM_OUTPUT
		      | ISP_PIPELINE_QUEUE_OUTPUT;
	else if (video->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		state = ISP_PIPELINE_STREAM_OUTPUT
		      | ISP_PIPELINE_QUEUE_OUTPUT;
	//else if (video->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
	//	state = ISP_PIPELINE_STREAM_INPUT
	//	      | ISP_PIPELINE_QUEUE_INPUT;	
	else
		state = ISP_PIPELINE_STREAM_INPUT
		      | ISP_PIPELINE_QUEUE_INPUT;

	spin_lock_irqsave(&pipe->lock, flags);
	pipe->state &= ~state;
	spin_unlock_irqrestore(&pipe->lock, flags);

	/* Stop the stream. */
	dev_info(video->isp->dev,"%s:video(%p) pipe(%p)\n",__func__,&pipe->output->video,pipe);
	k510isp_pipeline_set_stream(pipe, ISP_PIPELINE_STREAM_STOPPED,video);
	k510isp_video_cancel_stream(video);

	mutex_lock(&video->queue_lock);
	vb2_streamoff(&vfh->queue, type);
	mutex_unlock(&video->queue_lock);
	video->queue = NULL;
	video->error = false;

	/* TODO: Implement PM QoS */
	media_pipeline_stop(&video->video.entity);

	media_entity_enum_cleanup(&pipe->ent_enum);

done:
	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	mutex_unlock(&video->stream_lock);
	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param input 
 * @return int 
 */
static int k510isp_video_enum_input(struct file *file, void *fh, struct v4l2_input *input)
{
	if (input->index > 0)
		return -EINVAL;

	strlcpy(input->name, "camera", sizeof(input->name));
	input->type = V4L2_INPUT_TYPE_CAMERA;

	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param input 
 * @return int 
 */
static int k510isp_video_g_input(struct file *file, void *fh, unsigned int *input)
{
	*input = 0;

	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param fh 
 * @param input 
 * @return int 
 */
static int k510isp_video_s_input(struct file *file, void *fh, unsigned int input)
{
	return input == 0 ? 0 : -EINVAL;
}
/**
 * @brief 
 * 
 */
static const struct v4l2_ioctl_ops k510isp_video_ioctl_ops = {
	.vidioc_querycap		= k510isp_video_querycap,
	.vidioc_g_fmt_vid_cap		= k510isp_video_get_format,
	.vidioc_s_fmt_vid_cap		= k510isp_video_set_format,
	.vidioc_try_fmt_vid_cap		= k510isp_video_try_format,
	.vidioc_g_fmt_vid_out		= k510isp_video_get_format,
	.vidioc_s_fmt_vid_out		= k510isp_video_set_format,
	.vidioc_g_fmt_vid_out_mplane = k510isp_video_get_format,
    .vidioc_s_fmt_vid_out_mplane = k510isp_video_set_format,
	//.vidioc_enum_fmt_vid_out_mplane = k510isp_video_enum_format,//NULL,
	.vidioc_try_fmt_vid_out		= k510isp_video_try_format,
	.vidioc_g_selection		= k510isp_video_get_selection,
	.vidioc_s_selection		= k510isp_video_set_selection,
	.vidioc_g_parm			= k510isp_video_get_param,
	.vidioc_s_parm			= k510isp_video_set_param,
	.vidioc_reqbufs			= k510isp_video_reqbufs,
	.vidioc_querybuf		= k510isp_video_querybuf,
	.vidioc_qbuf			= k510isp_video_qbuf,
	.vidioc_dqbuf			= k510isp_video_dqbuf,
	.vidioc_streamon		= k510isp_video_streamon,
	.vidioc_streamoff		= k510isp_video_streamoff,
	.vidioc_enum_input		= k510isp_video_enum_input,
	.vidioc_g_input			= k510isp_video_g_input,
	.vidioc_s_input			= k510isp_video_s_input,
};
/* -----------------------------------------------------------------------------
 * V4L2 file operations
 *-----------------------------------------------------------------------------*/
/**
 * 
 */
static int k510isp_video_open(struct file *file)
{
	struct k510isp_video *video = video_drvdata(file);
	struct k510isp_video_fh *handle;
	struct vb2_queue *queue;
	struct v4l2_mbus_framefmt fmt;

	int ret = 0;
	dev_dbg(video->isp->dev,"%s:video->type %d start\n",__func__,video->type);
	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (handle == NULL)
	{
		dev_err(video->isp->dev,"%s:handle == NULL\n",__func__);
		return -ENOMEM;
	}	

	v4l2_fh_init(&handle->vfh, &video->video);
	v4l2_fh_add(&handle->vfh);

	/* If this is the first user, initialise the pipeline. */
	if (k510isp_get(video->isp) == NULL) {
		ret = -EBUSY;
		dev_err(video->isp->dev,"%s:k510isp_get ret(%d)\n",__func__,ret);
		goto done;
	}

	ret = v4l2_pipeline_pm_use(&video->video.entity, 1);
	if (ret < 0) {
		dev_err(video->isp->dev,"%s:v4l2_pipeline_pm_use ret(%d)\n",__func__,ret);
		k510isp_put(video->isp);
		goto done;
	}

	queue = &handle->queue;
	queue->type = video->type;
	queue->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	queue->drv_priv = handle;
	queue->ops = &k510isp_video_queue_ops;
	queue->mem_ops = &vb2_dma_contig_memops;
	queue->buf_struct_size = sizeof(struct k510isp_buffer);
	queue->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	queue->dev = video->isp->dev;


	ret = vb2_queue_init(&handle->queue);
	if (ret < 0) {
		dev_err(video->isp->dev,"%s:vb2_queue_init ret(%d)\n",__func__,ret);
		k510isp_put(video->isp);
		goto done;
	}
	//INIT_LIST_HEAD(&video->dmaqueue);
	memset(&handle->format, 0, sizeof(handle->format));
	handle->format.type = video->type;
	handle->timeperframe.denominator = 1;

	/* Init fomrmat */
	handle->format.fmt.pix.width       = 1920;
	handle->format.fmt.pix.height      = 1080;
	handle->format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	handle->format.fmt.pix.field       = V4L2_FIELD_ANY;

	handle->video = video;

	k510isp_video_set_format(file, &handle->vfh, &handle->format);

	file->private_data = &handle->vfh;

done:
	if (ret < 0) {
		v4l2_fh_del(&handle->vfh);
		v4l2_fh_exit(&handle->vfh);
		kfree(handle);
	}
	dev_dbg(video->isp->dev,"%s:end ret %d\n",__func__,ret);
	return ret;
}
/**
 * @brief 
 * 
 * @param file 
 * @return int 
 */
static int k510isp_video_release(struct file *file)
{
	struct k510isp_video *video = video_drvdata(file);
	struct v4l2_fh *vfh = file->private_data;
	struct k510isp_video_fh *handle = to_isp_video_fh(vfh);
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	/* Disable streaming and free the buffers queue resources. */
	k510isp_video_streamoff(file, vfh, video->type);

	mutex_lock(&video->queue_lock);
	vb2_queue_release(&handle->queue);
	mutex_unlock(&video->queue_lock);

	v4l2_pipeline_pm_use(&video->video.entity, 0);

	/* Release the file handle. */
	v4l2_fh_del(vfh);
	v4l2_fh_exit(vfh);
	kfree(handle);
	file->private_data = NULL;

	k510isp_put(video->isp);
	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	return 0;
}
/**
 * @brief 
 * 
 * @param file 
 * @param wait 
 * @return __poll_t 
 */
static __poll_t k510isp_video_poll(struct file *file, poll_table *wait)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(file->private_data);
	struct k510isp_video *video = video_drvdata(file);
	__poll_t ret;
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	mutex_lock(&video->queue_lock);
	ret = vb2_poll(&vfh->queue, file, wait);
	mutex_unlock(&video->queue_lock);

	return ret;
}
/**
 * @brief 
 * 
 * @param file 
 * @param vma 
 * @return int 
 */
static int k510isp_video_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct k510isp_video_fh *vfh = to_isp_video_fh(file->private_data);
	struct k510isp_video *video = video_drvdata(file);
	dev_dbg(video->isp->dev,"%s:start\n",__func__);
	return vb2_mmap(&vfh->queue, vma);
}
/**
 * @brief 
 * 
 */
static struct v4l2_file_operations k510isp_video_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = video_ioctl2,
	.open = k510isp_video_open,
	.release = k510isp_video_release,
	.poll = k510isp_video_poll,
	.mmap = k510isp_video_mmap,
};
/* -----------------------------------------------------------------------------
 * ISP video core
 *-----------------------------------------------------------------------------*/
static const struct k510isp_video_operations k510isp_video_dummy_ops = {

};
/**
 * @brief 
 * 
 * @param video 
 * @param name 
 * @return int 
*/
int k510isp_video_init(struct k510isp_video *video, const char *name)
{
	const char *direction;
	int ret;
	dev_dbg(video->isp->dev,"%s:start video->pad(0x%x)\n",__func__,&video->pad);
	switch (video->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		direction = "output";
		video->pad.flags = MEDIA_PAD_FL_SINK
				   | MEDIA_PAD_FL_MUST_CONNECT;
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
		direction = "output";
		video->pad.flags = MEDIA_PAD_FL_SINK
				   | MEDIA_PAD_FL_MUST_CONNECT;
		break;	
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		direction = "input";
		video->pad.flags = MEDIA_PAD_FL_SOURCE
				   | MEDIA_PAD_FL_MUST_CONNECT;
		video->video.vfl_dir = VFL_DIR_TX;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		direction = "input";
		video->pad.flags = MEDIA_PAD_FL_SOURCE
				   | MEDIA_PAD_FL_MUST_CONNECT;
		video->video.vfl_dir = VFL_DIR_TX;
		break;
	default:
		return -EINVAL;
	}

	ret = media_entity_pads_init(&video->video.entity, 1, &video->pad);
	if (ret < 0)
	{
		dev_err(video->isp->dev,"%s:ret(%d)\n",__func__,ret);
		return ret;
	}	

	mutex_init(&video->mutex);
	atomic_set(&video->active, 0);

	spin_lock_init(&video->pipe.lock);
	mutex_init(&video->stream_lock);
	mutex_init(&video->queue_lock);
	spin_lock_init(&video->irqlock);

	/* Initialize the video device. */
	if (video->ops == NULL)
		video->ops = &k510isp_video_dummy_ops;

	video->video.fops = &k510isp_video_fops;
	snprintf(video->video.name, sizeof(video->video.name),
		 "CANAAN K510 ISP %s %s", name, direction);
	video->video.vfl_type = VFL_TYPE_GRABBER;
	video->video.release = video_device_release_empty;
	video->video.ioctl_ops = &k510isp_video_ioctl_ops;
	video->pipe.stream_state = ISP_PIPELINE_STREAM_STOPPED;

	video_set_drvdata(&video->video, video);
	dev_dbg(video->isp->dev,"%s:end\n",__func__);
	return 0;
}
/**
 * @brief 
 * 
 * @param video 
 */
void k510isp_video_cleanup(struct k510isp_video *video)
{
	dev_info(video->isp->dev,"%s:start\n",__func__);
	media_entity_cleanup(&video->video.entity);
	mutex_destroy(&video->queue_lock);
	mutex_destroy(&video->stream_lock);
	mutex_destroy(&video->mutex);
}
/**
 * @brief 
 * 
 * @param video 
 * @param vdev 
 * @return int 
 */
int k510isp_video_register(struct k510isp_video *video, struct v4l2_device *vdev)
{
	int ret;
	dev_info(video->isp->dev,"%s:start\n",__func__);
	video->video.v4l2_dev = vdev;

	ret = video_register_device(&video->video, VFL_TYPE_GRABBER, -1); //VFL_TYPE_GRABBER
	if (ret < 0)
		dev_err(video->isp->dev,
			"%s: could not register video device (%d)\n",
			__func__, ret);

	return ret;
}
/**
 * @brief 
 * 
 * @param video 
 */
void k510isp_video_unregister(struct k510isp_video *video)
{
	dev_info(video->isp->dev,"%s:start\n",__func__);
	if (video_is_registered(&video->video))
		video_unregister_device(&video->video);
}
