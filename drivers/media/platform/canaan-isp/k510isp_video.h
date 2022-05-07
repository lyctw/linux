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

#ifndef _K510ISP_VIDEO_H_
#define _K510ISP_VIDEO_H_

#include <linux/v4l2-mediabus.h>
#include <media/media-entity.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-v4l2.h>


struct k510_isp_device;
struct k510isp_video;
struct v4l2_mbus_framefmt;
struct v4l2_pix_format;

/*
 * struct k510isp_format_info - ISP media bus format information
 * @code: V4L2 media bus format code
 * @truncated: V4L2 media bus format code for the same format truncated to 10
 *	bits. Identical to @code if the format is 10 bits wide or less.
 * @uncompressed: V4L2 media bus format code for the corresponding uncompressed
 *	format. Identical to @code if the format is not DPCM compressed.
 * @flavor: V4L2 media bus format code for the same pixel layout but
 *	shifted to be 8 bits per pixel. =0 if format is not shiftable.
 * @pixelformat: V4L2 pixel format FCC identifier
 * @width: Bits per pixel (when transferred over a bus)
 * @bpp: Bytes per pixel (when stored in memory)
 */
struct k510isp_format_info {
	u32 code;
	u32 truncated;
	u32 uncompressed;
	u32 flavor;
	u32 pixelformat;
	unsigned int width;
	unsigned int bpp;
	unsigned int bpp_numerator;
	unsigned int bpp_denominator;
};

enum k510isp_pipeline_stream_state {
	ISP_PIPELINE_STREAM_STOPPED = 0,
	ISP_PIPELINE_STREAM_CONTINUOUS = 1,
	ISP_PIPELINE_STREAM_SINGLESHOT = 2,
};

/*
 * VIDEO_MAIN: module bypass output, no scale
 * VIDEO_DS0: scale0 output
 * VIDEO_DS1: scale1 output
 * VIDEO_DS2: scale2 output
 * STREAM_VIR: virtual output for debug
 */
enum K510_video_id {
	VIDEO_MAIN = 0,
	VIDEO_DS0,
	VIDEO_DS1,
	VIDEO_DS2,
	VIDEO_MAX,
};

enum k510isp_pipeline_state {
	/* The stream has been started on the input video node. */
	ISP_PIPELINE_STREAM_INPUT = 1,
	/* The stream has been started on the output video node. */
	ISP_PIPELINE_STREAM_OUTPUT = 2,
	/* At least one buffer is queued on the input video node. */
	ISP_PIPELINE_QUEUE_INPUT = 4,
	/* At least one buffer is queued on the output video node. */
	ISP_PIPELINE_QUEUE_OUTPUT = 8,
	/* The input entity is idle, ready to be started. */
	ISP_PIPELINE_IDLE_INPUT = 16,
	/* The output entity is idle, ready to be started. */
	ISP_PIPELINE_IDLE_OUTPUT = 32,
	/* The pipeline is currently streaming. */
	ISP_PIPELINE_STREAM = 64,
};

/*
 * struct k510isp_pipeline - An ISP hardware pipeline
 * @field: The field being processed by the pipeline
 * @error: A hardware error occurred during capture
 * @ent_enum: Entities in the pipeline
 */
struct k510isp_pipeline {
	struct media_pipeline pipe;
	spinlock_t lock;		/* Pipeline state and queue flags */
	unsigned int state;
	enum k510isp_pipeline_stream_state stream_state;
	struct k510isp_video *input;
	struct k510isp_video *output;
	struct media_entity_enum ent_enum;
	unsigned int max_rate;
	enum v4l2_field field;
	atomic_t frame_number;
	bool do_propagation; /* of frame number */
	bool error;
	struct v4l2_fract max_timeperframe;
	struct v4l2_subdev *external;
	unsigned int external_rate;
	unsigned int external_width;
};

#define to_isp_pipeline(__e) \
	container_of((__e)->pipe, struct k510isp_pipeline, pipe)

static inline int k510isp_pipeline_ready(struct k510isp_pipeline *pipe)
{
	return pipe->state == (ISP_PIPELINE_STREAM_INPUT |
			       ISP_PIPELINE_STREAM_OUTPUT |
			       ISP_PIPELINE_QUEUE_INPUT |
			       ISP_PIPELINE_QUEUE_OUTPUT |
			       ISP_PIPELINE_IDLE_INPUT |
			       ISP_PIPELINE_IDLE_OUTPUT);
}

/**
 * struct k510isp_buffer - ISP video buffer
 * @vb: videobuf2 buffer
 * @irqlist: List head for insertion into IRQ queue
 * @dma: DMA address
 */
struct k510isp_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head irqlist;
	dma_addr_t dma;
	unsigned int image_size;
	unsigned int buf_size;
};

struct video_sink_pad{
	unsigned int video_sink_pad;
	unsigned int isp_sink_pad;
	unsigned int vi_sink_pad;
	unsigned int csi2_sink_pad;
};

#define to_isp_buffer(buf)	container_of(buf, struct k510isp_buffer, vb)

enum k510isp_video_dmaqueue_flags {
	/* Set if DMA queue becomes empty when ISP_PIPELINE_STREAM_CONTINUOUS */
	ISP_VIDEO_DMAQUEUE_UNDERRUN = (1 << 0),
	/* Set when queuing buffer to an empty DMA queue */
	ISP_VIDEO_DMAQUEUE_QUEUED = (1 << 1),
};

#define k510isp_video_dmaqueue_flags_clr(video)	({ (video)->dmaqueue_flags = 0; })

/*
 * struct k510isp_video_operations - ISP video operations
 * @queue:	Resume streaming when a buffer is queued. Called on VIDIOC_QBUF
 *		if there was no buffer previously queued.
 */
struct k510isp_video_operations {
	int(*queue)(struct k510isp_video *video, struct k510isp_buffer *buffer);
};

struct k510isp_video {
	//struct video_device video[VIDEO_MAX];
	struct video_device video;
	enum v4l2_buf_type type;
	struct media_pad pad;

	struct mutex mutex;		/* format and crop settings */
	atomic_t active;

	struct k510_isp_device *isp;

	unsigned int capture_mem;
	unsigned int bpl_alignment;	/* alignment value */
	unsigned int bpl_zero_padding;	/* whether the alignment is optional */
	unsigned int bpl_max;		/* maximum bytes per line value */
	unsigned int bpl_value;		/* bytes per line value */
	unsigned int bpl_padding;	/* padding at end of line */
	struct video_sink_pad sink_pads;
	/* Pipeline state */
	struct k510isp_pipeline pipe;
	struct mutex stream_lock;	/* pipeline and stream states */
	bool error;

	/* Video buffers queue */
	struct vb2_queue *queue;
	struct mutex queue_lock;	/* protects the queue */
	spinlock_t irqlock;		/* protects dmaqueue */
	struct list_head dmaqueue;
	enum k510isp_video_dmaqueue_flags dmaqueue_flags;

	const struct k510isp_video_operations *ops;
	//
	unsigned int buf_size;
};

#define to_isp_video(vdev)	container_of(vdev, struct k510isp_video, video)

struct k510isp_video_fh {
	struct v4l2_fh vfh;
	struct k510isp_video *video;
	struct vb2_queue queue;
	struct v4l2_format format;
	struct v4l2_fract timeperframe;
};

#define to_isp_video_fh(fh)	container_of(fh, struct k510isp_video_fh, vfh)
#define isp_video_queue_to_isp_video_fh(q) \
				container_of(q, struct k510isp_video_fh, queue)

int k510isp_video_init(struct k510isp_video *video, const char *name);
void k510isp_video_cleanup(struct k510isp_video *video);
int k510isp_video_register(struct k510isp_video *video,struct v4l2_device *vdev);
void k510isp_video_unregister(struct k510isp_video *video);
struct k510isp_buffer *k510isp_video_buffer_next(struct k510isp_video *video);
void k510isp_video_cancel_stream(struct k510isp_video *video);
void k510isp_video_resume(struct k510isp_video *video, int continuous);
struct media_pad *k510isp_video_remote_pad(struct k510isp_video *video);

const struct k510isp_format_info *k510isp_video_format_info(u32 code);

#endif /* _K510ISP_VIDEO_H_ */
