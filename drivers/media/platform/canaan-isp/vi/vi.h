/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _VI_H_
#define _VI_H_

#include <media/media-entity.h>
#include <media/v4l2-async.h>
#include <media/v4l2-device.h>
#include <linux/clk-provider.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/wait.h>

#include "../k510isp_com.h"
#include "../k510isp_video.h"
#include "vi_com.h"

struct k510_isp_device;

/* Sink and source VI pads */
#define VI_PAD_SINK_DVP		0
#define VI_PAD_SINK_CSI0	1
#define VI_PAD_SINK_CSI1	2
#define VI_PAD_SOURCE_F2K	3
#define VI_PAD_SOURCE_R2K	4
#define VI_PAD_SOURCE_TOF	5
#define VI_PADS_NUM         6
#define VI_INPUT_NUM        3
#define VI_OUTPUT_NUM       3
/*
*
*/
#define  VI_INPUT_NONE  0
#define  VI_INPUT_DVP   1<<0
#define  VI_INPUT_CSI0  1<<1
#define  VI_INPUT_CSI1  1<<2
/*
*
*/
#define  VI_OUTPUT_NONE  0
#define  VI_OUTPUT_F2K  1<<0
#define  VI_OUTPUT_R2K  1<<1
#define  VI_OUTPUT_TOF  1<<2

//#define VI_OUTPUT_NONE		(1 << 0)
#define VI_OUTPUT_W_MEMORY	(1 << 1)
#define VI_OUTPUT_R_MEMORY	(1 << 2)
#define VI_OUTPUT_WR_MEMORY	(1 << 3)

#define	k510ISP_VI_NEVENTS	1

struct vi_device{
    struct k510_isp_device *isp;

	struct v4l2_subdev subdev;
	struct media_pad pads[VI_PADS_NUM];
	struct v4l2_mbus_framefmt formats[VI_PADS_NUM];
	struct v4l2_rect crop;

	unsigned int input;
	unsigned int output;
	//unsigned int output;
	unsigned int tpg_mode;
	struct k510isp_video video_in;
	struct k510isp_video video_out;

	enum k510isp_pipeline_stream_state state;
	spinlock_t lock;
	wait_queue_head_t wait;
	unsigned int stopping;
	bool running;
	struct mutex ioctl_lock;
    unsigned int underrun;
//
	struct vi_cfg_info	vi_cfg;
    //struct vi_wrap_cfg_info  vi_wrap_cfg;
    //struct vi_pipe_cfg_info  vi_pipe_cfg[8];
};

void k510isp_vi_wrap_print_cfg(struct k510_isp_device *isp);
int k510isp_vi_register_entities(struct vi_device *vi,struct v4l2_device *vdev);
void k510isp_vi_unregister_entities(struct vi_device *vi);
int k510isp_vi_init(struct k510_isp_device *isp);
void k510isp_vi_cleanup(struct k510_isp_device *isp);
#endif /*_VI_H_*/