/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _ISP_F2K_H_
#define _ISP_F2K_H_

#include <linux/videodev2.h>
#include <linux/types.h>
#include "../k510isp_video.h"
#include "isp.h"
#include "../k510isp_com.h"

struct k510_isp_device;

#define F2K_MIN_WIDTH		32
#define F2K_MIN_HEIGHT		32

/* Sink and source isp f2k pads */
#define ISP_F2K_PAD_SINK			0
#define ISP_F2K_PAD_MAIN_SOURCE		1
#define ISP_F2K_PAD_DS0_SOURCE		2
#define ISP_F2K_PAD_DS1_SOURCE		3
#define ISP_F2K_PAD_DS2_SOURCE		4
#define ISP_F2K_PADS_NUM			5

#define	ISP_F2K_NEVENTS				16

#define F2K_STOP_NOT_REQUESTED		0x00
#define F2K_STOP_REQUEST		0x01
#define F2K_STOP_EXECUTED		(0x02 | F2K_STOP_REQUEST)
#define F2K_STOP_F2K_FINISHED		0x04
#define F2K_STOP_FINISHED		\
	(F2K_STOP_EXECUTED | F2K_STOP_F2K_FINISHED)

/* isp_f2k input media entity */
enum isp_f2k_input_entity {
	ISP_F2K_INPUT_NONE,
	ISP_F2K_INPUT_VI,
};

/* isp_f2k output media entity */

#define ISP_F2K_OUTPUT_NONE    	(0)
#define ISP_F2K_OUTPUT_FBC	   	(1<<0)
#define ISP_F2K_OUTPUT_MAIN_MEM	(1<<1)
#define ISP_F2K_OUTPUT_DS0_MEM	(1<<2)
#define ISP_F2K_OUTPUT_DS1_MEM	(1<<3)
#define ISP_F2K_OUTPUT_DS2_MEM	(1<<4)



/* Logical channel configuration */
struct isp_interface_lcx_config {
	int crc;
	unsigned int data_start;
	unsigned int data_size;
	unsigned int format;
};

/* Memory channel configuration */
struct isp_interface_mem_config {
	unsigned int dst_port;
	unsigned int vsize_count;
	unsigned int hsize_count;
	unsigned int src_ofst;
	unsigned int dst_ofst;
};

struct isp_nr3d_dma{
	void *addr;
	dma_addr_t dma;
	unsigned int dma_size;
};

/*f2k device*/
struct isp_f2k_device{
	struct k510_isp_device *isp;
	struct v4l2_subdev subdev;
	struct media_pad pads[ISP_F2K_PADS_NUM];
	struct v4l2_mbus_framefmt formats[ISP_F2K_PADS_NUM];
	struct v4l2_rect crop_in;
	struct v4l2_rect crop_out;
	//struct video_sink_pad sink_pads;
			
	enum isp_f2k_input_entity input;
	struct k510isp_video video_in;
	struct k510isp_video video_out[VIDEO_NUM_MAX];
	spinlock_t lock;
	wait_queue_head_t wait;
	unsigned int nr3d_en;
	struct isp_nr3d_dma nr3d_dma;
	struct mutex ioctl_lock;
	unsigned int output;
	unsigned int underrun:1;
	enum k510isp_pipeline_stream_state state;
	atomic_t stopping;
	bool running;
	unsigned int fields;
	/* mem resources - enums as defined in enum isp_mem_resources */
	//u8 regs1;	
	struct isp_cfg_info isp_cfg; 
};
/* Function declarations */
int k510isp_f2k_init(struct k510_isp_device *isp);
void k510isp_f2k_cleanup(struct k510_isp_device *isp);
int  k510isp_f2k_register_entities(struct isp_f2k_device *f2k,
			struct v4l2_device *vdev);
void k510isp_f2k_unregister_entities(struct isp_f2k_device *f2k);
int k510isp_f2k_main_isr(struct isp_f2k_device *f2k,u32 events);
int k510isp_f2k_ds0_isr(struct isp_f2k_device *f2k,u32 events);
int k510isp_f2k_ds1_isr(struct isp_f2k_device *f2k,u32 events);
int k510isp_f2k_ds2_isr(struct isp_f2k_device *f2k,u32 events);
void k510isp_f2k_irq_enable(struct k510_isp_device *isp,struct isp_irq_info *irq_info);
int k510isp_f2k_reset(struct k510_isp_device *isp);
#endif /*_ISP_F2K_H_*/