/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _ISP_R2K_H_
#define _ISP_R2K_H_

#include <linux/videodev2.h>
#include "../k510isp_video.h"
#include "isp.h"
#include "../k510isp_com.h"
#include "../k510_isp.h"

struct k510_isp_device;

#define R2K_MIN_WIDTH		32
#define R2K_MIN_HEIGHT		32

/* Sink and source isp R2K pads */
#define ISP_R2K_PAD_SINK			0
#define ISP_R2K_PAD_MAIN_SOURCE		1
#define ISP_R2K_PAD_DS0_SOURCE		2
#define ISP_R2K_PAD_DS1_SOURCE		3
#define ISP_R2K_PAD_DS2_SOURCE		4
#define ISP_R2K_PADS_NUM			5

#define	ISP_R2K_NEVENTS				16

/* isp_r2k input media entity */
enum isp_r2k_input_entity {
	ISP_R2K_INPUT_NONE,
	ISP_R2K_INPUT_VI,
	ISP_R2K_INPUT_VI_TPG,
};

/* isp_R2K output media entity */
#define ISP_R2K_OUTPUT_NONE    	(0)
#define ISP_R2K_OUTPUT_FBC	   	(1<<0)
#define ISP_R2K_OUTPUT_MAIN_MEM	(1<<1)
#define ISP_R2K_OUTPUT_DS0_MEM	(1<<2)
#define ISP_R2K_OUTPUT_DS1_MEM	(1<<3)
#define ISP_R2K_OUTPUT_DS2_MEM	(1<<4)

/*r2k device*/
struct isp_r2k_device{
	struct k510_isp_device *isp;

	struct v4l2_subdev subdev;
	struct v4l2_mbus_framefmt formats[ISP_R2K_PADS_NUM];
	struct media_pad pads[ISP_R2K_PADS_NUM];
	struct v4l2_rect crop_in;
	struct v4l2_rect crop_out;
	
	enum isp_r2k_input_entity input;
	unsigned int output;
	struct k510isp_video video_in;
	struct k510isp_video video_out[VIDEO_NUM_MAX];

	unsigned int underrun:1;
	enum k510isp_pipeline_stream_state state;
	spinlock_t lock;
	wait_queue_head_t wait;
	atomic_t stopping;
	bool running;
	struct mutex ioctl_lock;
	unsigned int fields;

	struct isp_cfg_info isp_cfg;
};


/* Function declarations */
int k510isp_r2k_init(struct k510_isp_device *isp);
void k510isp_r2k_cleanup(struct k510_isp_device *isp);
int  k510isp_r2k_register_entities(struct isp_r2k_device *r2k,
			struct v4l2_device *vdev);
void k510isp_r2k_unregister_entities(struct isp_r2k_device *r2k);
int k510isp_r2k_main_isr(struct isp_r2k_device *r2k,u32 events);
int k510isp_r2k_ds0_isr(struct isp_r2k_device *r2k,u32 events);
int k510isp_r2k_ds1_isr(struct isp_r2k_device *r2k,u32 events);
int k510isp_r2k_ds2_isr(struct isp_r2k_device *r2k,u32 events);
void k510isp_r2k_irq_enable(struct k510_isp_device *isp,struct isp_irq_info *irq_info);
int k510isp_r2k_reset(struct k510_isp_device *isp);
#endif /*_ISP_R2K_H_*/