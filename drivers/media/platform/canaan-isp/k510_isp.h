/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _K510_ISP_H_
#define _K510_ISP_H_

#include <media/media-entity.h>
#include <media/v4l2-async.h>
#include <media/v4l2-device.h>
#include <linux/clk-provider.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/v4l2-mediabus.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-v4l2.h>
#include <linux/k510isp.h>

#include "vi/vi.h"
#include "isp_2k/isp_f2k.h"
#include "isp_2k/isp_r2k.h"
#include "isp_mipi/dphy/mipi_dphy.h"
#include "isp_mipi/csi2/mipi_csi2.h"
#include "vi/vi_com.h"
#include "k510isp_stat.h"

#define K510ISP_VIDEO_DRIVER_NAME		"k510isp-video"
#define K510ISP_VIDEO_DRIVER_VERSION	    "0.0.1"
#define DRIVER_NAME	  "k510-isp"
#define PIXEL_7425_CLK    (74250000)
#define PIXEL_14850_CLK    (148500000)
#define SYSTEM_16600_CLK    (166000000)

struct k510_isp_device;
//struct vi_device;
struct v4l2_mbus_framefmt;
struct v4l2_pix_format;
//core
#define CORE_STS_RAW_IN_IRQ					(1 << 0)
#define CORE_STS_3A_OUT_IRQ					(1 << 1)
#define CORE_STS_RAW_OUT_IRQ				(1 << 2)
#define CORE_STS_RGB_OUT_IRQ				(1 << 3)
#define CORE_STS_YUV_OUT_IRQ				(1 << 4)
#define CORE_STS_LDC_OUT_IRQ				(1 << 5)
#define CORE_STS_ISP_OUT_IRQ				(1 << 6)
//WR0
#define IRQW0_STS_3DNR_Y_FRAME_IRQ			(1 << 0)
#define IRQW0_STS_3DNR_Y_LINE_IRQ			(1 << 1)
#define IRQW0_STS_3DNR_Y_FRAME_ERR_IRQ		(1 << 2)
#define IRQW0_STS_3DNR_Y_IMM_ERR_IRQ		(1 << 3)
#define IRQW0_STS_3DNR_UV_FRAME_IRQ			(1 << 4)
#define IRQW0_STS_3DNR_UV_LINE_IRQ			(1 << 5)
#define IRQW0_STS_3DNR_UV_FRAME_ERR_IRQ		(1 << 6)
#define IRQW0_STS_3DNR_UV_IMM_ERR_IRQ		(1 << 7)
#define IRQW0_STS_LDC_Y_FRAME_IRQ			(1 << 8)
#define IRQW0_STS_LDC_Y_LINE_IRQ			(1 << 9)
#define IRQW0_STS_LDC_Y_FRAME_ERR_IRQ		(1 << 10)
#define IRQW0_STS_LDC_Y_IMM_ERR_IRQ			(1 << 11)
#define IRQW0_STS_LDC_UV_FRAME_IRQ			(1 << 12)
#define IRQW0_STS_LDC_UV_LINE_IRQ			(1 << 13)
#define IRQW0_STS_LDC_UV_FRAME_ERR_IRQ		(1 << 14)
#define IRQW0_STS_LDC_UV_IMM_ERR_IRQ		(1 << 15)
#define IRQW0_STS_WDR_RAW_FRAME_IRQ			(1 << 16)
#define IRQW0_STS_WDR_RAW_LINE_IRQ			(1 << 17)
#define IRQW0_STS_WDR_RAW_FRAME_ERR_IRQ		(1 << 18)
#define IRQW0_STS_WDR_RAW_IMM_ERR_IRQ		(1 << 19)

#define IRQW0_STS_MAIN_Y_FRAME_IRQ			(1 << 24)
#define IRQW0_STS_MAIN_Y_LINE_IRQ			(1 << 25)
#define IRQW0_STS_MAIN_Y_FRAME_ERR_IRQ		(1 << 26)
#define IRQW0_STS_MAIN_Y_IMM_ERR_IRQ		(1 << 27)
#define IRQW0_STS_MAIN_UV_FRAME_IRQ			(1 << 28)
#define IRQW0_STS_MAIN_UV_LINE_IRQ			(1 << 29)
#define IRQW0_STS_MAIN_UV_FRAME_ERR_IRQ		(1 << 30)
#define IRQW0_STS_MAIN_UV_IMM_ERR_IRQ		(1 << 31)
//WR1
#define IRQW1_STS_OUT0_Y_FRAME_IRQ			(1 << 0)
#define IRQW1_STS_OUT0_Y_LINE_IRQ			(1 << 1)
#define IRQW1_STS_OUT0_Y_FRAME_ERR_IRQ		(1 << 2)
#define IRQW1_STS_OUT0_Y_IMM_ERR_IRQ		(1 << 3)
#define IRQW1_STS_OUT0_UV_FRAME_IRQ			(1 << 4)
#define IRQW1_STS_OUT0_UV_LINE_IRQ			(1 << 5)
#define IRQW1_STS_OUT0_UV_FRAME_ERR_IRQ		(1 << 6)
#define IRQW1_STS_OUT0_UV_IMM_ERR_IRQ		(1 << 7)
#define IRQW1_STS_OUT1_Y_FRAME_IRQ			(1 << 8)
#define IRQW1_STS_OUT1_Y_LINE_IRQ			(1 << 9)
#define IRQW1_STS_OUT1_Y_FRAME_ERR_IRQ		(1 << 10)
#define IRQW1_STS_OUT1_Y_IMM_ERR_IRQ		(1 << 11)
#define IRQW1_STS_OUT1_UV_FRAME_IRQ			(1 << 12)
#define IRQW1_STS_OUT1_UV_LINE_IRQ			(1 << 13)
#define IRQW1_STS_OUT1_UV_FRAME_ERR_IRQ		(1 << 14)
#define IRQW1_STS_OUT1_UV_IMM_ERR_IRQ		(1 << 15)
#define IRQW1_STS_OUT2_R_FRAME_IRQ			(1 << 16)
#define IRQW1_STS_OUT2_R_LINE_IRQ			(1 << 17)
#define IRQW1_STS_OUT2_R_FRAME_ERR_IRQ		(1 << 18)
#define IRQW1_STS_OUT2_R_IMM_ERR_IRQ		(1 << 19)
#define IRQW1_STS_OUT2_G_FRAME_IRQ			(1 << 20)
#define IRQW1_STS_OUT2_G_LINE_IRQ			(1 << 21)
#define IRQW1_STS_OUT2_G_FRAME_ERR_IRQ		(1 << 22)
#define IRQW1_STS_OUT2_G_IMM_ERR_IRQ		(1 << 23)
#define IRQW1_STS_OUT2_B_FRAME_IRQ			(1 << 24)
#define IRQW1_STS_OUT2_B_LINE_IRQ			(1 << 25)
#define IRQW1_STS_OUT2_B_FRAME_ERR_IRQ		(1 << 26)
#define IRQW1_STS_OUT2_B_IMM_ERR_IRQ		(1 << 27)

enum isp_mem_resources {
	ISP_IOMEM_CSI0_HOST,
	ISP_IOMEM_CSI1_HOST,
	ISP_IOMEM_MIPI_DPHY,
	ISP_IOMEM_MIPI_CORNER,
	ISP_IOMEM_VI_WRAP,
	ISP_IOMEM_VI_PIPE_CSI00,
	ISP_IOMEM_VI_PIPE_CSI01,
	ISP_IOMEM_VI_PIPE_CSI02,
	ISP_IOMEM_VI_PIPE_CSI10,
	ISP_IOMEM_VI_PIPE_CSI11,
	ISP_IOMEM_VI_PIPE_CSI12,
	ISP_IOMEM_VI_PIPE_DVP0,	
	ISP_IOMEM_VI_PIPE_DVP1,		
	ISP_IOMEM_F2K_WRAP,
	ISP_IOMEM_F2K_CORE,
	ISP_IOMEM_F2K_CORE_TABLE,
	ISP_IOMEM_F2K_FBC,
	ISP_IOMEM_F2K_FBD,
	ISP_IOMEM_F2K_DS,
    ISP_IOMEM_F2K_MAIN_REMAP,
	ISP_IOMEM_F2K_OUT0_REMAP,
    ISP_IOMEM_F2K_OUT1_REMAP,
	ISP_IOMEM_R2K_WRAP,
	ISP_IOMEM_R2K_CORE,
	ISP_IOMEM_R2K_CORE_TABLE,
	ISP_IOMEM_R2K_DS,
	ISP_IOMEM_R2K_MAIN_REMAP,
    ISP_IOMEM_R2K_OUT0_REMAP,
    ISP_IOMEM_R2K_OUT1_REMAP,
	ISP_IOMEM_TOF_WRAP,
	ISP_IOMEM_TOF_CORE,
	ISP_IOMEM_TOF_TABLE,
	ISP_IOMEM_FBC_WRAP,
    ISP_IOMEM_FBC_CORE,
	ISP_IOMEM_VO_CORE,
	ISP_IOMEM_VO_REMAP,
	ISP_IOMEM_VO_HSCALE,
	ISP_IOMEM_VO_VSCALE,
	ISP_IOMEM_VO_GAMMA,	
	ISP_IOMEM_BT1120,
	ISP_IOMEM_LAST,
};

#define ISP_MAX_SUBDEVS		5

#define ISP_DS_CH_NUM 3
#define ISP_DS_CH_OSD_NUM 3

enum k510isp_subclk_resource {
	K510_ISP_SUBCLK_RXDPHY 	= (1 << 0),
	K510_ISP_SUBCLK_CSI0 	= (1 << 1),
	K510_ISP_SUBCLK_CSI1 	= (1 << 2),
	K510_ISP_SUBCLK_VI 	 	= (1 << 3),
	K510_ISP_SUBCLK_VI_TPG	= (1 << 4),
	K510_ISP_SUBCLK_F2K 	= (1 << 5),
	K510_ISP_SUBCLK_R2K 	= (1 << 6),
	K510_ISP_SUBCLK_TOF 	= (1 << 7),
	K510_ISP_SUBCLK_FBC 	= (1 << 8),
};

enum SUBDEV_GRP_ID{
	MIPI0_PHY_SUBDEV,
	MIPI1_PHY_SUBDEV,
	VI_SUBDEV,
	ISP_F2K_SUBDEV,
	ISP_R2K_SUBDEV,
	ISP_TOF_SUBDEV,
	ISP_MFBC_SUBDEV,
};

enum k510isp_interface_type {
	ISP_INTERFACE_DVP,	
	ISP_INTERFACE_CSI1,
	ISP_INTERFACE_CSI2,
};

struct k510isp_bus_cfg {
	enum k510isp_interface_type interface;
};

/*
 * struct isp_reg - Structure for ISP register values.
 * @reg: 32-bit Register address.
 * @val: 32-bit Register value.
 */
struct k510isp_reg {
	enum isp_mem_resources mmio_range;
	u32 reg;
	u32 val;
};

struct k510isp_async_subdev {
	struct v4l2_async_subdev asd;
	struct k510isp_bus_cfg bus;
};

enum isp_clks_e{
	MIPI_CORNER,
	MIPI_REF,
	MIPI_RXPHY_REF,
	CSI0_SYSTEM,
	CSI0_APB, 
	CSI0_PIXEL,
	CSI1_SYSTEM,
	CSI1_APB, 
	CSI1_PIXEL,
	VI_AXI,
	VI_APB,
	TPG_PIXEL,
	ISP_F2K_APB,
	ISP_F2K_AXI,
	ISP_R2K_APB,
	ISP_R2K_AXI,
	ISP_TOF_APB,
	ISP_TOF_AXI,
	FBC_APB,
	FBC_AXI,
	ISP_CLK_MAX,
};

enum isp_rst_e{
	VI_RST,
	ISP_F4K_RST,
	ISP_F2K_RST,
	ISP_R2K_RST,
	ISP_TOF_RST,
	CSI0_RST,
	CSI1_RST,
	CSI2_RST,
	CSI3_RST,
	SENSOR_RST,
	FBC_RST,
	MIPI_CORNER_RST,
	ISP_RST_MAX,
};

#define v4l2_subdev_to_bus_cfg(sd) \
	(&container_of((sd)->asd, struct k510isp_async_subdev, asd)->bus)

#define v4l2_dev_to_isp_device(dev) \
	container_of(dev, struct k510_isp_device, v4l2_dev)

#define to_isp_device(ptr_module)				\
	container_of(ptr_module, struct k510_isp_device, isp_##ptr_module)
#define to_device(ptr_module)	(to_isp_device(ptr_module)->dev)

int k510isp_pipeline_set_stream(struct k510isp_pipeline *pipe,enum k510isp_pipeline_stream_state state,struct k510isp_video *video);
void k510isp_pipeline_cancel_stream(struct k510isp_pipeline *pipe);
struct k510_isp_device *k510isp_get(struct k510_isp_device *isp);
void k510isp_put(struct k510_isp_device *isp);
//
void k510isp_subclk_enable(struct k510_isp_device *isp,enum k510isp_subclk_resource res);
void k510isp_subclk_disable(struct k510_isp_device *isp,enum k510isp_subclk_resource res);
//
int k510isp_module_sync_idle(struct media_entity *me, wait_queue_head_t *wait,atomic_t *stopping);
int k510isp_module_sync_is_stopping(wait_queue_head_t *wait,atomic_t *stopping);
//int isp_register_entities(struct platform_device *pdev,struct v4l2_device *v4l2_dev);
//void isp_unregister_entities(struct platform_device *pdev);
int k510isp_enable_clocks(struct k510_isp_device *isp);
void k510isp_disable_clocks(struct k510_isp_device *isp);
//
struct k510_isp_device {
	struct v4l2_device v4l2_dev;
	struct v4l2_async_notifier notifier;
	struct media_device media_dev;
	struct device *dev;
	unsigned int  f2k_revision;
	unsigned int  r2k_revision;
	unsigned int  tof_revision;
	unsigned int irq_num[2];
	void __iomem *mmio_base[ISP_IOMEM_LAST];
	struct regmap *syscon;
	unsigned int  syscon_offset;
	unsigned int  phy_type;
	struct dma_iommu_mapping *mapping;
	spinlock_t 		stat_lock;	/* common lock for statistic drivers */
	struct mutex 	isp_mutex;	/* For handling ref_count field */
	bool 			stop_failure;
	struct media_entity_enum crashed;//unsigned int  	crashed;
	int 			has_context;
	int 			ref_count;
	unsigned int 	autoidle;
	unsigned int subclk_resources;
	struct v4l2_subdev *subdevs[ISP_MAX_SUBDEVS];
    void __iomem    *isp_regs;
	unsigned int isp_addr;
	struct clk *clock[ISP_CLK_MAX];
	struct reset_control *reset[ISP_RST_MAX];
	//
	unsigned int dualcamera_used[2];
	unsigned int f2k_used[2];
	unsigned int r2k_used[2];
	/* ISP modules */
	struct vi_device isp_vi;
	struct isp_f2k_device isp_f2k;
	struct isp_r2k_device isp_r2k;
	struct isp_csi2_device isp_csi2;
	struct isp_dphy_device isp_dphy;
	struct k510isp_stat	isp_f2k_ae;
	struct k510isp_stat	isp_f2k_af;
	struct k510isp_stat	isp_f2k_awb;
	struct k510isp_stat	isp_r2k_ae;
	struct k510isp_stat	isp_r2k_af;
	struct k510isp_stat	isp_r2k_awb;
};

enum isp_pad_def{
	ISP_PAD_SINK,
	ISP_PAD_SOURCE_OF,
	ISP_PAD_SOURCE_VP,
	ISP_PADS_NUM,
};
enum isp_input_entity {
	ISP_INPUT_NONE,
	ISP_INPUT_CSI1,
	ISP_INPUT_CSI2,
	ISP_INPUT_DVP,
};

static inline
unsigned int  isp_reg_readl(struct k510_isp_device *isp, enum isp_mem_resources isp_mmio_range,
		  unsigned int  reg_offset)
{
	return __raw_readl(isp->mmio_base[isp_mmio_range] + reg_offset);
}
static inline
void isp_reg_writel(struct k510_isp_device *isp, unsigned int  reg_value,
		    enum isp_mem_resources isp_mmio_range, unsigned int  reg_offset)
{
	__raw_writel(reg_value, isp->mmio_base[isp_mmio_range] + reg_offset);
}
static inline
void isp_reg_clr(struct k510_isp_device *isp, enum isp_mem_resources mmio_range,
		 unsigned int  reg, unsigned int  clr_bits)
{
	unsigned int  v = isp_reg_readl(isp, mmio_range, reg);
	isp_reg_writel(isp, v & ~clr_bits, mmio_range, reg);
}
static inline
void isp_reg_set(struct k510_isp_device *isp, enum isp_mem_resources mmio_range,
		 unsigned int  reg, unsigned int  set_bits)
{
	unsigned int  v = isp_reg_readl(isp, mmio_range, reg);
	isp_reg_writel(isp, v | set_bits, mmio_range, reg);
}
static inline
void isp_reg_clr_set(struct k510_isp_device *isp, enum isp_mem_resources mmio_range,
		     unsigned int  reg, unsigned int  clr_bits, unsigned int  set_bits)
{
	unsigned int  v = isp_reg_readl(isp, mmio_range, reg);
	isp_reg_writel(isp, (v & ~clr_bits) | set_bits, mmio_range, reg);
}

#endif /*_K510_ISP_H_*/