/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/reset.h>
#include <media/v4l2-event.h>
#include <media/media-entity.h>

#include "../k510isp_com.h"
#include "../k510_isp.h"
#include "../k510isp_video.h"
#include "../k510video_subsystem.h"
#include "vi.h"
#include "wrap/vi_wrap_reg.h"
#include "wrap/vi_wrap_drv.h"
#include "pipe/vi_pipe_reg.h"
#include "pipe/vi_pipe_drv.h"


extern unsigned int reg_vi_pipe_base[];

#define VI_DAT_START_MIN		0
#define VI_DAT_START_MAX		2047
#define VI_DAT_SIZE_MIN			0
#define VI_DAT_SIZE_MAX			2047

/**************************************************************************************
 * debug registers
 * ***********************************************************************************/
void k510isp_vi_wrap_print_cfg(struct k510_isp_device *isp)
{
	struct vi_device *vi = &isp->isp_vi;
	struct vi_wrap_cfg_info  *vi_wrap_cfg = &vi->vi_cfg.vi_wrap_cfg;
	struct _SENSOR_INFO *sensor_info;

	dev_dbg(vi->isp->dev,"%s:dphy_mode:%d\n",__func__,vi_wrap_cfg->dphy_mode);
	dev_dbg(vi->isp->dev,"%s:sony_mode:%d\n",__func__,vi_wrap_cfg->sony_mode);
	//MIPI0
	sensor_info = &vi_wrap_cfg->sensor_info[0];
	dev_dbg(vi->isp->dev,"%s:sensor_interface_en:%d\n",__func__,sensor_info->sensor_interface_en);
	dev_dbg(vi->isp->dev,"%s:tpg_w_en:%d\n",__func__,sensor_info->tpg_w_en);
	dev_dbg(vi->isp->dev,"%s:tpg_r_en:%d\n",__func__,sensor_info->tpg_r_en);
	dev_dbg(vi->isp->dev,"%s:wdr_sensor_vendor:%d\n",__func__,sensor_info->wdr_sensor_vendor);
	dev_dbg(vi->isp->dev,"%s:wdr_mode:%d\n",__func__,sensor_info->wdr_mode);
	dev_dbg(vi->isp->dev,"%s:mipi_mode:%d\n",__func__,sensor_info->mipi_mode);
	dev_dbg(vi->isp->dev,"%s:isp_pipeline:%d\n",__func__,sensor_info->isp_pipeline);
	//MIPI1
	sensor_info = &vi_wrap_cfg->sensor_info[1];
	dev_dbg(vi->isp->dev,"%s:sensor_interface_en:%d\n",__func__,sensor_info->sensor_interface_en);
	dev_dbg(vi->isp->dev,"%s:tpg_w_en:%d\n",__func__,sensor_info->tpg_w_en);
	dev_dbg(vi->isp->dev,"%s:tpg_r_en:%d\n",__func__,sensor_info->tpg_r_en);
	dev_dbg(vi->isp->dev,"%s:wdr_sensor_vendor:%d\n",__func__,sensor_info->wdr_sensor_vendor);
	dev_dbg(vi->isp->dev,"%s:wdr_mode:%d\n",__func__,sensor_info->wdr_mode);
	dev_dbg(vi->isp->dev,"%s:mipi_mode:%d\n",__func__,sensor_info->mipi_mode);
	dev_dbg(vi->isp->dev,"%s:isp_pipeline:%d\n",__func__,sensor_info->isp_pipeline);
	//DVP
	sensor_info = &vi_wrap_cfg->sensor_info[2];
	dev_dbg(vi->isp->dev,"%s:sensor_interface_en:%d\n",sensor_info->sensor_interface_en);
	dev_dbg(vi->isp->dev,"%s:tpg_w_en:%d\n",sensor_info->tpg_w_en);
	dev_dbg(vi->isp->dev,"%s:tpg_r_en:%d\n",sensor_info->tpg_r_en);
	dev_dbg(vi->isp->dev,"%s:wdr_sensor_vendor:%d\n",sensor_info->wdr_sensor_vendor);
	dev_dbg(vi->isp->dev,"%s:wdr_mode:%d\n",sensor_info->wdr_mode);
	dev_dbg(vi->isp->dev,"%s:mipi_mode:%d\n",sensor_info->mipi_mode);
	dev_dbg(vi->isp->dev,"%s:isp_pipeline:%d\n",sensor_info->isp_pipeline);
}
/*
 * isp_vi_wrap_print_status - Prints the values of the isp vi wrap module registers.
 */

#define VI_WRAP_PRINT_REGISTER(isp, name)\
	dev_dbg(isp->dev, "###WRAP:" #name"(0x%x) = 0x%x\n", \
		isp->mmio_base[ISP_IOMEM_VI_WRAP] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp, ISP_IOMEM_VI_WRAP, name))

void isp_vi_wrap_print_status(struct vi_device *vi)
{
	struct k510_isp_device  *isp = to_isp_device(vi);

	dev_dbg(isp->dev, "-------------VI WRAP Register dump start----------\n");

	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_SWRST_CTL);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_CSI_MODE_CTL);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_ISP_CH_SEL);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_CLOCK_CTL);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_ARB_MODE);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_WR_WEIGHT_0);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_WR_WEIGHT_1);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_RD_WEIGHT_0);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_RD_WEIGHT_1);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_WR_PRIORITY);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_RD_PRIORITY);
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_WR_CH_ID   );
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_RD_CH_ID   );
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_DMA_AXI_CTL    );
	VI_WRAP_PRINT_REGISTER(isp,VI_WRAP_CONFIG_CTL     );
	VI_WRAP_PRINT_REGISTER(isp,VI_WARP_INT_CTL     	);

    dev_dbg(isp->dev, "-------------VI WRAP Register dump end----------\n");

}

#define VI_PIPE_PRINT_REGISTER(isp,s8Index,name)\
	dev_dbg(isp->dev, "###PIPE(%d):" #name "(0x%x)= 0x%x\n", \
		s8Index,isp->mmio_base[reg_vi_pipe_base[s8Index]] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp,reg_vi_pipe_base[s8Index],name))

void isp_vi_pipe_print_status(struct vi_device *vi)
{
	struct k510_isp_device  *isp = to_isp_device(vi);
 
	dev_dbg(isp->dev, "-------------VI PIPE Register dump start----------\n");

	unsigned char s8Index = VI_MIPI_CSI00_PIPE_ID;
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CTRL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TIMING_CTRL    		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_SYNC_WIDTH     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_TOTAL   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_ACTIVE  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_START     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_LINE_STRIDE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_DMA_CTL     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_CTL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_TOF     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_TOTAL     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_ACTIVE    	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_START     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR0_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR1_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_LINE_STRIDE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_MODE     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_ERROR_MODE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_RST_REQ     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CONFIG_DONE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_INT_CTL     			);
	s8Index = VI_MIPI_CSI01_PIPE_ID;
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CTRL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TIMING_CTRL    		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_SYNC_WIDTH     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_TOTAL   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_ACTIVE  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_START     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_LINE_STRIDE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_DMA_CTL     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_CTL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_TOF     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_TOTAL     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_ACTIVE    	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_START     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR0_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR1_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_LINE_STRIDE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_MODE     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_ERROR_MODE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_RST_REQ     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CONFIG_DONE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_INT_CTL     			);
	s8Index = VI_MIPI_CSI02_PIPE_ID;
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CTRL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TIMING_CTRL    		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_SYNC_WIDTH     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_TOTAL   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_ACTIVE  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_START     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_LINE_STRIDE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_DMA_CTL     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_CTL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_TOF     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_TOTAL     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_ACTIVE    	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_START     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR0_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR1_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_LINE_STRIDE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_MODE     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_ERROR_MODE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_RST_REQ     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CONFIG_DONE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_INT_CTL     			);   
	s8Index = VI_MIPI_CSI10_PIPE_ID;
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CTRL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TIMING_CTRL    		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_SYNC_WIDTH     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_TOTAL   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_ACTIVE  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_START     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_LINE_STRIDE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_DMA_CTL     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_CTL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_TOF     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_TOTAL     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_ACTIVE    	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_START     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR0_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR1_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_LINE_STRIDE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_MODE     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_ERROR_MODE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_RST_REQ     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CONFIG_DONE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_INT_CTL     			); 
	s8Index = VI_MIPI_CSI11_PIPE_ID;
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CTRL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TIMING_CTRL    		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_SYNC_WIDTH     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_TOTAL   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_ACTIVE  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_START     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_LINE_STRIDE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_DMA_CTL     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_CTL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_TOF     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_TOTAL     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_ACTIVE    	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_START     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR0_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR1_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_LINE_STRIDE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_MODE     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_ERROR_MODE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_RST_REQ     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CONFIG_DONE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_INT_CTL     			);
	s8Index = VI_MIPI_CSI12_PIPE_ID;
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CTRL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TIMING_CTRL    		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_SYNC_WIDTH     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_TOTAL   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_ACTIVE  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_START     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_LINE_STRIDE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_DMA_CTL     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_CTL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_TOF     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_TOTAL     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_ACTIVE    	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_START     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR0_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR1_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_LINE_STRIDE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_MODE     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_ERROR_MODE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_RST_REQ     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CONFIG_DONE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_INT_CTL     			);
	s8Index = VI_DVP_0_PIPE_ID;
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CTRL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TIMING_CTRL    		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_SYNC_WIDTH     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_TOTAL   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_ACTIVE  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_START     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_LINE_STRIDE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_DMA_CTL     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_CTL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_TOF     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_TOTAL     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_ACTIVE    	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_START     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR0_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR1_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_LINE_STRIDE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_MODE     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_ERROR_MODE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_RST_REQ     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CONFIG_DONE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_INT_CTL     			);
	s8Index = VI_DVP_1_PIPE_ID;
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CTRL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TIMING_CTRL    		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_SYNC_WIDTH     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_TOTAL   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_SIZE_ACTIVE  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_IMAGE_START     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_Y   	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR0_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_FRM_BASE_ADDR1_UV  	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_LINE_STRIDE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_DMA_CTL     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_CTL     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_TOF     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_TOTAL     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_SIZE_ACTIVE    	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_START     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR0_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_FRM_BASE_ADDR1_Y   );
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_TPG_LINE_STRIDE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_MODE     			);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_ERROR_MODE     	);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_DMA_RST_REQ     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_CONFIG_DONE     		);
	VI_PIPE_PRINT_REGISTER(isp,s8Index,VI_PIPE_INT_CTL     			);

    dev_dbg(isp->dev, "-------------VI PIPE Register dump end----------\n");

}

void vi_print_status(struct vi_device *vi)
{
	isp_vi_wrap_print_status(vi);
	isp_vi_pipe_print_status(vi);
}
/**************************************************************************************
 * 
 ************************************************************************************/
void vi_wrap_rst(struct k510_isp_device  *isp)
{
	dev_dbg(isp->dev,"%s:start\n",__func__);
    VI_WRAP_RESET_CTL_S pstRstCtl;
    pstRstCtl.csi_00_rst_en    = VI_WRAP_SW_RST_EN;
    pstRstCtl.csi_01_rst_en    = VI_WRAP_SW_RST_EN;
    pstRstCtl.csi_02_rst_en    = VI_WRAP_SW_RST_EN;
    pstRstCtl.csi_10_rst_en    = VI_WRAP_SW_RST_EN;
    pstRstCtl.csi_11_rst_en    = VI_WRAP_SW_RST_EN;
    pstRstCtl.csi_12_rst_en    = VI_WRAP_SW_RST_EN;
    pstRstCtl.dvp_0_rst_en     = VI_WRAP_SW_RST_EN;
    pstRstCtl.dvp_1_rst_en     = VI_WRAP_SW_RST_EN;
    pstRstCtl.axi_wr_ch_rst_en = VI_WRAP_SW_RST_EN;
    pstRstCtl.axi_rd_ch_rst_en = VI_WRAP_SW_RST_EN;    
    VI_DRV_WRAP_SetRst(isp,&pstRstCtl);
	dev_dbg(isp->dev,"%s:end\n",__func__);
}

void vi_wrap_config_dma(struct k510_isp_device  *isp,VI_WRAP_DMA_ATTR_S *pstDmaAttr)
{

	VI_WRAP_DMA_ARB_MODE_S *stDmaArbMode = &pstDmaAttr->stDmaArbMode;
	stDmaArbMode->wr_dma_arb_mode = 0;//1;
	stDmaArbMode->rd_dma_arb_mode = 0;//1;

	VI_WRAP_DMA_WR_WEIGHT_0_S  *stWrChWeight0 = &pstDmaAttr->stWrChWeight0;
	stWrChWeight0->vi_dma_ch0_wr_weight = 1;
	stWrChWeight0->vi_dma_ch1_wr_weight = 1;
	stWrChWeight0->vi_dma_ch2_wr_weight = 1;
	stWrChWeight0->vi_dma_ch3_wr_weight = 1;
	VI_WRAP_DMA_WR_WEIGHT_1_S  *stWrChWeight1 = &pstDmaAttr->stWrChWeight1;
	stWrChWeight1->vi_dma_ch4_wr_weight = 1;
	stWrChWeight1->vi_dma_ch5_wr_weight = 1;
	stWrChWeight1->vi_dma_ch6_wr_weight = 1;
	stWrChWeight1->vi_dma_ch7_wr_weight = 1;
    VI_WRAP_DMA_RD_WEIGHT_0_S  *stRdChWeight0 = &pstDmaAttr->stRdChWeight0;
	stRdChWeight0->vi_dma_ch0_rd_weight = 1;
	stRdChWeight0->vi_dma_ch1_rd_weight = 1;
	stRdChWeight0->vi_dma_ch2_rd_weight = 1;
	stRdChWeight0->vi_dma_ch3_rd_weight = 1;
	VI_WRAP_DMA_RD_WEIGHT_1_S  *stRdChWeight1 = &pstDmaAttr->stRdChWeight1;
	stRdChWeight1->vi_dma_ch4_rd_weight = 1;
	stRdChWeight1->vi_dma_ch5_rd_weight = 1;
	stRdChWeight1->vi_dma_ch6_rd_weight = 1;
	stRdChWeight1->vi_dma_ch7_rd_weight = 1;
    VI_WRAP_DMA_WR_PRIORITY_S  *stWrChPriority = &pstDmaAttr->stWrChPriority;
	stWrChPriority->vi_dma_ch0_wr_priority = 7;//0;
	stWrChPriority->vi_dma_ch1_wr_priority = 6;//1;
	stWrChPriority->vi_dma_ch2_wr_priority = 5;//2;
	stWrChPriority->vi_dma_ch3_wr_priority = 4;//3;
	stWrChPriority->vi_dma_ch4_wr_priority = 3;//4;
	stWrChPriority->vi_dma_ch5_wr_priority = 2;//5;
	stWrChPriority->vi_dma_ch6_wr_priority = 1;//6;
	stWrChPriority->vi_dma_ch7_wr_priority = 0;//7;
    VI_WRAP_DMA_RD_PRIORITY_S  *stRdChPriority = &pstDmaAttr->stRdChPriority;
	stRdChPriority->vi_dma_ch0_rd_priority = 7;//0;
	stRdChPriority->vi_dma_ch1_rd_priority = 6;//1;
	stRdChPriority->vi_dma_ch2_rd_priority = 5;//2;
	stRdChPriority->vi_dma_ch3_rd_priority = 4;//3;
	stRdChPriority->vi_dma_ch4_rd_priority = 3;//4;
	stRdChPriority->vi_dma_ch5_rd_priority = 2;//5;
	stRdChPriority->vi_dma_ch6_rd_priority = 1;//6;
	stRdChPriority->vi_dma_ch7_rd_priority = 0;//7;
    VI_WRAP_DMA_WR_CH_ID_S     *stWriteChId = &pstDmaAttr->stWriteChId;
	stWriteChId->vi_dma_wr_ch0_id = 0;
	stWriteChId->vi_dma_wr_ch1_id = 1;
	stWriteChId->vi_dma_wr_ch2_id = 2;
	stWriteChId->vi_dma_wr_ch3_id = 3;
	stWriteChId->vi_dma_wr_ch4_id = 4;
	stWriteChId->vi_dma_wr_ch5_id = 5;
	stWriteChId->vi_dma_wr_ch6_id = 6;
	stWriteChId->vi_dma_wr_ch7_id = 7;
    VI_WRAP_DMA_RD_CH_ID_S     *stReadChId = &pstDmaAttr->stReadChId;	
	stReadChId->vi_dma_rd_ch0_id = 0;
	stReadChId->vi_dma_rd_ch1_id = 1;
	stReadChId->vi_dma_rd_ch2_id = 2;
	stReadChId->vi_dma_rd_ch3_id = 3;
	stReadChId->vi_dma_rd_ch4_id = 4;
	stReadChId->vi_dma_rd_ch5_id = 5;
	stReadChId->vi_dma_rd_ch6_id = 6;
	stReadChId->vi_dma_rd_ch7_id = 7; 
    VI_DRV_WRAP_SetDmaAttr(isp,pstDmaAttr);
}

void vi_wrap_config(struct vi_device *vi,struct vi_wrap_cfg_info *vi_wrap_cfg)
{
	dev_dbg(vi->isp->dev,"%s:start\n",__func__);
    struct k510_isp_device  *isp = to_isp_device(vi);
	//printk("struct canaan_isp_device = 0x%x,vi= 0x%x\n",isp,vi);
    //struct vi_wrap_cfg_info  *vi_wrap_cfg = &vi->vi_wrap_cfg;
    SENSOR_INFO *mipi0Info = &vi_wrap_cfg->sensor_info[MIPI0];
    SENSOR_INFO *mipi1Info = &vi_wrap_cfg->sensor_info[MIPI1];
    SENSOR_INFO   *dvpInfo = &vi_wrap_cfg->sensor_info[DVP0];
	VI_WRAP_CFG_DONE_S pstWrapCfgDone;
    pstWrapCfgDone.vi_wrap_config_done = 0;
    pstWrapCfgDone.vi_wrap_wp_clr = VI_DONE_WP_DIS;    
	VI_DRV_WRAP_SetCfgDone(isp,&pstWrapCfgDone);
    vi_wrap_rst(isp);
    VI_WRAP_CSI_MODE_CTL_S pstCsiModeCtl;
    pstCsiModeCtl.mipi_csi_0_mode = NORMAL_MODE;
    pstCsiModeCtl.mipi_csi_1_mode = NORMAL_MODE;
    pstCsiModeCtl.mipi_dphy_mode = TWO_LANES_MODE;
    pstCsiModeCtl.mipi_csi01_dbg_sel = CSI0_DEBUG_INFO;
    pstCsiModeCtl.isp_4k_clk_sel = F_CSI_0_CLK;
    pstCsiModeCtl.isp_2k_clk_sel = F_CSI_0_CLK;
    pstCsiModeCtl.isp_r_2k_clk_sel = R_TOF_CSI_1_CLK;
    pstCsiModeCtl.tof_clk_sel = R_TOF_CSI_1_CLK;
    pstCsiModeCtl.csi_0_sony_dol_en = SONY_POL_MODE_DIS;
    pstCsiModeCtl.dvp_clk_0_mode = DVP_RAISE_EGDE;
	if( ENABLE == mipi0Info->sensor_interface_en)
	{
		if( ENABLE == mipi0Info->tpg_w_en)
		{
			pstCsiModeCtl.mipi_csi_0_mode = DEBUG_MODE;
		}
    	if(SONY_WDR_SENSOR == mipi0Info->wdr_sensor_vendor)
		{
			if(( ISP_PIPE_WDR_2_FRAME == mipi0Info->wdr_mode) ||(ISP_PIPE_WDR_3_FRAME == mipi0Info->wdr_mode))
			{
				pstCsiModeCtl.csi_0_sony_dol_en = SONY_POL_MODE_EN;
			}			
		}
		if( ISP_F_2K == mipi0Info->isp_pipeline)
		{
			pstCsiModeCtl.isp_2k_clk_sel = F_CSI_0_CLK;
			if(ENABLE == mipi0Info->tpg_r_en)
			{
				pstCsiModeCtl.isp_2k_clk_sel = F_DPG_CLK;
			}
		}
		else if(ISP_R_2K == mipi0Info->isp_pipeline)
		{
			pstCsiModeCtl.isp_r_2k_clk_sel = R_TOF_CSI_0_CLK;
			if(ENABLE == mipi0Info->tpg_r_en)
			{
				pstCsiModeCtl.isp_r_2k_clk_sel = R_TOF_DPG_CLK;
			}		
		}
		else if(ISP_TOF == mipi0Info->isp_pipeline)
		{
			pstCsiModeCtl.tof_clk_sel = R_TOF_CSI_0_CLK;
			if(ENABLE == mipi0Info->tpg_r_en)
			{
				pstCsiModeCtl.tof_clk_sel = R_TOF_DPG_CLK;
			}	
		}
	}
	if( ENABLE == mipi1Info->sensor_interface_en)
	{
		if( ISP_F_2K == mipi1Info->isp_pipeline)
		{
			pstCsiModeCtl.isp_2k_clk_sel = F_CSI_1_CLK;
			if(ENABLE == mipi1Info->tpg_r_en)
			{
				pstCsiModeCtl.isp_2k_clk_sel = F_DPG_CLK;
			}
		}
		else if(ISP_R_2K == mipi1Info->isp_pipeline)
		{
			pstCsiModeCtl.isp_r_2k_clk_sel = R_TOF_CSI_1_CLK;
			if(ENABLE == mipi1Info->tpg_r_en)
			{
				pstCsiModeCtl.isp_r_2k_clk_sel = R_TOF_DPG_CLK;
			}		
		}
		else if(ISP_TOF == mipi1Info->isp_pipeline)
		{
			pstCsiModeCtl.tof_clk_sel = R_TOF_CSI_1_CLK;
			if(ENABLE == mipi1Info->tpg_r_en)
			{
				pstCsiModeCtl.tof_clk_sel = R_TOF_DPG_CLK;
			}	
		}
	}
	if(ENABLE == dvpInfo->sensor_interface_en)
	{
		pstCsiModeCtl.dvp_clk_0_mode = DVP_FALLING_EDGE;//RAISE_EGDE; //FALLING_EDGE;//
		if( ISP_F_2K == dvpInfo->isp_pipeline)
		{
			pstCsiModeCtl.isp_2k_clk_sel = F_DVP_0_CLK;//F_DPG_CLK;
			if(ENABLE == dvpInfo->tpg_r_en)
			{
				pstCsiModeCtl.isp_2k_clk_sel = F_DPG_CLK;
			}
		}
		else if(ISP_R_2K == dvpInfo->isp_pipeline)
		{
			pstCsiModeCtl.isp_r_2k_clk_sel = R_TOF_DVP_0_CLK;
			if(ENABLE == dvpInfo->tpg_r_en)
			{
				pstCsiModeCtl.isp_r_2k_clk_sel = R_TOF_DPG_CLK;
			}		
		}
		else if(ISP_TOF == dvpInfo->isp_pipeline)
		{
			pstCsiModeCtl.tof_clk_sel = R_TOF_DPG_CLK;
			if(ENABLE == dvpInfo->tpg_r_en)
			{
				pstCsiModeCtl.tof_clk_sel = R_TOF_DPG_CLK;
			}	
		} 
	}
    VI_DRV_WRAP_SetCtlMode(isp,&pstCsiModeCtl);
	VI_WRAP_ISP_CH_SEL_S pstIspChSel;
    pstIspChSel.isp_4k_l_ch_sel = CSI_10_PIPE;
    pstIspChSel.isp_4k_m_ch_sel = CSI_01_PIPE;
    pstIspChSel.isp_4k_s_ch_sel = CSI_02_PIPE;
    pstIspChSel.isp_2k_l_ch_sel = CSI_00_PIPE;
    pstIspChSel.isp_2k_m_ch_sel = CSI_11_PIPE;
    pstIspChSel.isp_2k_s_ch_sel = CSI_12_PIPE;
    pstIspChSel.isp_r_2k_ch_sel = DVP_0_PIPE;
    pstIspChSel.isp_3d_ch_sel   = DVP_1_PIPE;
	if( ENABLE == mipi0Info->sensor_interface_en)
	{
		if( ISP_F_2K == mipi0Info->isp_pipeline)
		{
    		pstIspChSel.isp_2k_l_ch_sel = CSI_00_PIPE;
    		pstIspChSel.isp_2k_m_ch_sel = CSI_01_PIPE;
    		pstIspChSel.isp_2k_s_ch_sel = CSI_02_PIPE;
		}
		else if(ISP_R_2K == mipi0Info->isp_pipeline)
		{
			pstIspChSel.isp_r_2k_ch_sel = CSI_00_PIPE;		
		}
		else if(ISP_TOF == mipi0Info->isp_pipeline)
		{
		    pstIspChSel.isp_3d_ch_sel = CSI_00_PIPE;//CSI_10_PIPE;
		}		
	}
	if( ENABLE == mipi1Info->sensor_interface_en)
	{
		if( ISP_F_2K == mipi1Info->isp_pipeline)
		{
    		pstIspChSel.isp_2k_l_ch_sel = CSI_10_PIPE;
    		pstIspChSel.isp_2k_m_ch_sel = CSI_11_PIPE;
    		pstIspChSel.isp_2k_s_ch_sel = CSI_12_PIPE;
		}
		else if(ISP_R_2K == mipi1Info->isp_pipeline)
		{
			pstIspChSel.isp_r_2k_ch_sel = CSI_10_PIPE;		
		}
		else if(ISP_TOF == mipi1Info->isp_pipeline)
		{
			pstIspChSel.isp_3d_ch_sel = CSI_10_PIPE; 	
		}		
	}
	if( ENABLE == dvpInfo->sensor_interface_en)
	{
		if( ISP_F_2K == dvpInfo->isp_pipeline)
		{
    		pstIspChSel.isp_2k_l_ch_sel = DVP_0_PIPE;
		}
		else if(ISP_R_2K == dvpInfo->isp_pipeline)
		{
			pstIspChSel.isp_r_2k_ch_sel = DVP_0_PIPE;		
		}
		else if(ISP_TOF == dvpInfo->isp_pipeline)
		{
			pstIspChSel.isp_3d_ch_sel = DVP_0_PIPE; 	
		}		
	}
	VI_DRV_WRAP_SetIspChSel(isp,&pstIspChSel);
	VI_WRAP_CLOCK_CTL_S pstClkCtl;
    pstClkCtl.csi_00_pixel_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_01_pixel_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_02_pixel_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_10_pixel_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_11_pixel_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_12_pixel_clk_en = VI_CLK_DIS;
    pstClkCtl.dvp_0_pixel_clk_en = VI_CLK_DIS;
    pstClkCtl.dvp_1_pixel_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_00_dma_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_01_dma_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_02_dma_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_10_dma_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_11_dma_clk_en = VI_CLK_DIS;
    pstClkCtl.csi_12_dma_clk_en = VI_CLK_DIS;
    pstClkCtl.dvp_0_dma_clk_en = VI_CLK_DIS;
    pstClkCtl.dvp_1_dma_clk_en = VI_CLK_DIS;
    pstClkCtl.axi_wr_ch_clk_en = VI_CLK_EN;
    pstClkCtl.axi_rd_ch_clk_en = VI_CLK_EN;
	if( ENABLE == mipi0Info->sensor_interface_en)
	{
		pstClkCtl.csi_00_pixel_clk_en = VI_CLK_EN;
		if(( ISP_PIPE_WDR_2_FRAME == mipi0Info->wdr_mode) ||(ISP_PIPE_WDR_3_FRAME == mipi0Info->wdr_mode))
		{
			pstClkCtl.csi_01_pixel_clk_en = VI_CLK_EN;
    		pstClkCtl.csi_02_pixel_clk_en = VI_CLK_EN;
		}
		if((ENABLE == mipi0Info->tpg_w_en ) ||(ENABLE == mipi0Info->tpg_r_en ))
		{
		    pstClkCtl.csi_00_dma_clk_en = VI_CLK_EN;
			pstClkCtl.csi_01_dma_clk_en = VI_CLK_EN;
    		pstClkCtl.csi_02_dma_clk_en = VI_CLK_EN;	
		}
	}
	if( ENABLE == mipi1Info->sensor_interface_en)
	{
		pstClkCtl.csi_10_pixel_clk_en = VI_CLK_EN;
		if((ENABLE == mipi0Info->tpg_w_en ) ||(ENABLE == mipi0Info->tpg_r_en ))
		{
	    	pstClkCtl.csi_10_dma_clk_en = VI_CLK_EN;
		}
	}
	if( ENABLE == dvpInfo->sensor_interface_en)
	{
		pstClkCtl.dvp_0_pixel_clk_en = VI_CLK_EN;
		if((ENABLE == mipi0Info->tpg_w_en ) ||(ENABLE == mipi0Info->tpg_r_en ))
		{
			pstClkCtl.dvp_0_dma_clk_en = VI_CLK_EN;
		}
	}
	VI_DRV_WRAP_SetClockCtl(isp,&pstClkCtl);
	//
	VI_WRAP_DMA_ATTR_S pstDmaAttr;    
	vi_wrap_config_dma(isp,&pstDmaAttr);
	//
    VI_WRAP_INT_EN_S  pstIntEn;
    pstIntEn.csi_0_host_int_en = VI_INT_DIS;    
    pstIntEn.csi_0_host_err_int_en = VI_INT_DIS;
    pstIntEn.csi_1_host_int_en = VI_INT_DIS;    
    pstIntEn.csi_1_host_err_int_en = VI_INT_DIS;
    pstIntEn.csi_0_ctrl_0_int_en = VI_INT_DIS;  
    pstIntEn.csi_0_ctrl_1_int_en = VI_INT_DIS;  
    pstIntEn.csi_0_ctrl_2_int_en = VI_INT_DIS;  
    pstIntEn.dvp_0_ctrl_int_en = VI_INT_DIS;    
    pstIntEn.csi_1_ctrl_0_int_en = VI_INT_DIS;  
    pstIntEn.csi_1_ctrl_1_int_en = VI_INT_DIS;  
    pstIntEn.csi_1_ctrl_2_int_en = VI_INT_DIS;  
    pstIntEn.dvp_1_ctrl_int_en = VI_INT_DIS;       
	VI_DRV_WRAP_SetIntEn(isp,&pstIntEn);
	//VI_WRAP_CFG_DONE_S pstWrapCfgDone;
    pstWrapCfgDone.vi_wrap_config_done = 1;
    pstWrapCfgDone.vi_wrap_wp_clr = VI_DONE_WP_EN;    
	VI_DRV_WRAP_SetCfgDone(isp,&pstWrapCfgDone);
	//printk("vi_wrap_config end\n");
}
//
void vi_pipe_SetIntEn(struct vi_device *vi,unsigned int s8Index,struct VI_PIPE_INT_EN *int_en)
{
	struct k510_isp_device  *isp = to_isp_device(vi);
	VI_PIPE_INT_CTL_S pstIntCtl;
    pstIntCtl.win_ctl_frame_end_en           = VI_INT_DIS; 
    pstIntCtl.emb_ctl_frame_end_en           = VI_INT_DIS; 
    pstIntCtl.emb_ctl_time_out_en            = VI_INT_DIS; 
    pstIntCtl.tpg_ctl_frame_end_en           = VI_INT_DIS; 
    pstIntCtl.dma_y_wr_frame_end_en          = VI_INT_DIS; 
    pstIntCtl.dma_y_wr_ch_line_base_en       = VI_INT_DIS; 
    pstIntCtl.dma_y_wr_ch_err_frame_end_en   = VI_INT_DIS; 
    pstIntCtl.dma_y_wr_ch_err_immediate_en   = VI_INT_DIS; 
    pstIntCtl.dma_uv_wr_ch_frame_end_en      = VI_INT_DIS; 
    pstIntCtl.dma_uv_wr_ch_line_base_en      = VI_INT_DIS; 
    pstIntCtl.dma_uv_wr_ch_err_frame_end_en  = VI_INT_DIS;
    pstIntCtl.dma_uv_wr_ch_err_immediate_en  = VI_INT_DIS;
    pstIntCtl.dma_raw_rd_ch_frame_end_en     = VI_INT_DIS; 
    pstIntCtl.dma_raw_rd_ch_line_base_en     = VI_INT_DIS; 
    pstIntCtl.dma_raw_rd_ch_err_frame_end_en = VI_INT_DIS;
    pstIntCtl.dma_raw_rd_ch_err_immediate_en = VI_INT_DIS;
	VI_DRV_PIPE_SetInt(isp,s8Index,&pstIntCtl);
}
unsigned int vi_pipe_GetIntSts(struct vi_device *vi,unsigned int s8Index)
{
	struct k510_isp_device  *isp = to_isp_device(vi);
    return VI_DRV_PIPE_GetPipeIntSts(isp,s8Index);
}
//
void vi_pipe_config(struct vi_device *vi,unsigned int s8Index,struct vi_pipe_cfg_info *vi_pipe_cfg)
{
	struct k510_isp_device  *isp = to_isp_device(vi);
	dev_dbg(vi->isp->dev,"%s:start\n",__func__);
	VI_PIPE_CONFIG_DONE_S pstCfgDone;
    pstCfgDone.pipe_config_done      = 0;
    pstCfgDone.sens_reg_wr_protect   = VI_DONE_WP_DIS;//VI_DONE_WP_EN;
    pstCfgDone.int_pol               = VI_INT_HIGH_ACT; //must high
	VI_DRV_PIPE_SetCfgDone(isp,s8Index,&pstCfgDone);
    //struct vi_pipe_cfg_info  *vi_pipe_cfg =&vi->vi_pipe_cfg[s8Index];
	VI_PIPE_CTRL_S pstPipeCtl;
    pstPipeCtl.win_mode_en= vi_pipe_cfg->win_mode_en;//TRUE;//1;
    pstPipeCtl.emb_en= FALSE;//0;
    pstPipeCtl.hsync_vsync_out_en= TRUE;//1;
    pstPipeCtl.sensor_input_swap_en= FALSE;//0; 
    pstPipeCtl.isp_vsync_pol= vi_pipe_cfg->isp_vsync_pol;//TRUE;//1;
    pstPipeCtl.isp_hsync_pol= vi_pipe_cfg->isp_hsync_pol;//TRUE;//1;
    pstPipeCtl.isp_field_pol= vi_pipe_cfg->isp_field_pol;//TRUE;//1;
    pstPipeCtl.isp_clk_pol= vi_pipe_cfg->isp_clk_pol;//TRUE;//1;
    pstPipeCtl.sen_mipi_vsync_pol = vi_pipe_cfg->sen_mipi_vsync_pol;//FALSE;//0;//1;
    pstPipeCtl.sen_mipi_hsync_pol = vi_pipe_cfg->sen_mipi_hsync_pol;//FALSE;//0;//1;
    pstPipeCtl.sen_mipi_field_pol = vi_pipe_cfg->sen_mipi_field_pol;//TRUE;//1;
    pstPipeCtl.sen_mipi_clk_pol= vi_pipe_cfg->sen_mipi_clk_pol;//TRUE;//1;
    pstPipeCtl.sync_source_sel= EXTERNAL_SYNC;//0;//1;//0;
    pstPipeCtl.input_ch_sel= vi_pipe_cfg->input_ch_sel;//MIPI_INPUT;//1;
    pstPipeCtl.ch_mode_sel= vi_pipe_cfg->ch_mode_sel;//VI_MIPI_BT1120;//1;
    pstPipeCtl.emb_enable= EMB_DIS;//0;
    pstPipeCtl.emb_mode_sel= EMB_8BIT_656_MODE;//0;
    pstPipeCtl.emb_ecc_mode_en= FALSE;//0;
    pstPipeCtl.sync_code_remove_en= FALSE;//0;
    pstPipeCtl.sync_code_ext_en= FALSE;//0;    
    VI_DRV_PIPE_SetPipeCtl(isp,s8Index, &pstPipeCtl);
	
	VI_PIPE_TIMING_CTRL_S pstTimingCtl;
    pstTimingCtl.input_pixel_type= vi_pipe_cfg->pixel_type ;//SENSOR_INPUT_RAWRGB;//0; /*RAW input*/
    pstTimingCtl.input_yuv_format= vi_pipe_cfg->yuv_in_format;//VI_INPUT_YUV422;//0;
    pstTimingCtl.output_yuv_format= vi_pipe_cfg->yuv_out_format;//VI_OUTPUT_YUV422;//1;
    pstTimingCtl.yuv_order= vi_pipe_cfg->yuv422_order;//VI_YUV422_YUYV;//0;
    pstTimingCtl.raw_pixel_width= vi_pipe_cfg->pixel_width;//PIXEL_WIDTH_12BIT;//1;//2; /*RAW 10bit*/
    pstTimingCtl.data_out_timming_ctrl= vi_pipe_cfg->data_out_timming_ctrl;//CTRL_CYCLE_DELAY_4;//3;//2;//1;//0;//3;/*three cycle delay*/
    pstTimingCtl.sync_mode= SYNC_V_DATA_EN;//4;//6;//5;//3;//2;//1;//0;//4;
    pstTimingCtl.sync_pulse_mode= vi_pipe_cfg->sync_pulse_mode;//0;    
	VI_DRV_PIPE_SetTimingCtl(isp,s8Index,&pstTimingCtl);//
	VI_PIPE_SYNC_WIDTH_S pstSyncWidth;
	pstSyncWidth.image_hsync_width = 0x7;
	pstSyncWidth.image_vsync_width = 0x7;   
	VI_DRV_PIPE_SetSyncWidthCtl(isp,s8Index, &pstSyncWidth);
	VI_PIPE_IMAGE_ATTR_S pstImgAttr;
	//printk("%s:total_size.Width(%d),total_size.Height(%d)\n",__func__,vi_pipe_cfg->total_size.Width,vi_pipe_cfg->total_size.Height);
    pstImgAttr.stImageSizeTotal.image_v_size_total    = vi_pipe_cfg->total_size.Height - 1;
    pstImgAttr.stImageSizeTotal.image_h_size_total    = vi_pipe_cfg->total_size.Width  - 1;
    pstImgAttr.stImageSizeActive.image_v_size_active  = vi_pipe_cfg->in_size.Height - 1;
    pstImgAttr.stImageSizeActive.image_h_size_active  = vi_pipe_cfg->in_size.Width - 1;
    pstImgAttr.stImageStart.image_v_start             = vi_pipe_cfg->w_size_st.Height_st;
    pstImgAttr.stImageStart.image_h_start             = vi_pipe_cfg->w_size_st.Width_st;
	VI_DRV_PIPE_SetImageAttr(isp,s8Index,&pstImgAttr);
	VI_PIPE_TPG_S pstTpg;
    VI_PIPE_BUF_S *stPipeBuf = &pstTpg.stPipeBuf;
    stPipeBuf->y_base_addr0   = vi_pipe_cfg->vi_pipe_w_addr_y0;
    stPipeBuf->y_base_addr1   = vi_pipe_cfg->vi_pipe_w_addr_y1;
    stPipeBuf->y_line_stride  = vi_pipe_cfg->vi_pipe_addr_stride;
    stPipeBuf->uv_base_addr0  = vi_pipe_cfg->vi_pipe_w_addr_uv0;
    stPipeBuf->uv_base_addr1  = vi_pipe_cfg->vi_pipe_w_addr_uv1;
    stPipeBuf->uv_line_stride = vi_pipe_cfg->vi_pipe_addr_stride;
    VI_PIPE_TPG_IMAGE_ATTR_S *stPipeTpgImage = &pstTpg.stPipeTpgImage;
    stPipeTpgImage->stTpgSizeTotal.tpg_vsize_total   = vi_pipe_cfg->total_size.Height -1;
    stPipeTpgImage->stTpgSizeTotal.tpg_hsize_total   = vi_pipe_cfg->total_size.Width -1;
    stPipeTpgImage->stTpgSizeActive.tpg_vsize_active = vi_pipe_cfg->in_size.Height -1;
    stPipeTpgImage->stTpgSizeActive.tpg_hsize_active = vi_pipe_cfg->in_size.Width -1; 
    stPipeTpgImage->stTpgStart.tpg_v_start = vi_pipe_cfg->r_size_st.Height_st;
    stPipeTpgImage->stTpgStart.tpg_h_start = vi_pipe_cfg->r_size_st.Width_st;    
    VI_PIPE_TPG_BUF_S *stPipeTpgBuf = &pstTpg.stPipeTpgBuf;
    stPipeTpgBuf->tpg_y_base_addr0  = vi_pipe_cfg->vi_pipe_r_addr_y0;
    stPipeTpgBuf->tpg_y_base_addr1  = vi_pipe_cfg->vi_pipe_r_addr_y1;
    stPipeTpgBuf->tpg_y_line_stride = vi_pipe_cfg->vi_pipe_addr_stride;  
	VI_DRV_PIPE_SetTpgAttr(isp,s8Index,&pstTpg);
	VI_PIPE_TPG_DMA_CTL_S stTpgDmaCtrl;
    stTpgDmaCtrl.dma_y_module_en      = vi_pipe_cfg->tpg_w_en;
    stTpgDmaCtrl.dma_uv_module_en     = FALSE;
    stTpgDmaCtrl.dma_tpg_read_en      = vi_pipe_cfg->tpg_r_en;
    stTpgDmaCtrl.sync_out_en          = vi_pipe_cfg->tpg_w_en;
    stTpgDmaCtrl.sw_triger_en         = vi_pipe_cfg->tpg_w_en;
    stTpgDmaCtrl.dma_wr_ch_err_dec_en = vi_pipe_cfg->tpg_w_en;
    stTpgDmaCtrl.dma_rd_ch_err_dec_en = vi_pipe_cfg->tpg_r_en;
    VI_DRV_PIPE_SetTpgDmaCtl(isp,s8Index,&stTpgDmaCtrl); 
	VI_PIPE_TPG_CTL_S pstPipeTpgCtl;
    pstPipeTpgCtl.tpg_en                    = vi_pipe_cfg->tpg_r_en;
    pstPipeTpgCtl.tpg_frame_ratio_mode      = 0x0;
    pstPipeTpgCtl.tpg_sync_timing_gen_mode  = 0x1;
    pstPipeTpgCtl.tpg_frame_ratio_fast_slow = 0x1;
	VI_DRV_PIPE_SetIspTpg(isp,s8Index,&pstPipeTpgCtl);
    VI_PIPE_DMA_MODE_S DmaCtrl;
    DmaCtrl.dma_y_outstand   = 0x8;
    DmaCtrl.dma_uv_outstand  = 0x8;
    DmaCtrl.dma_raw_outstand = 0x8;
    DmaCtrl.dma_y_blen       = 0;
    DmaCtrl.dma_uv_blen      = 0;
    DmaCtrl.dma_raw_blen     = 0;
    DmaCtrl.dma_y_uv_out_swap= 0;
    DmaCtrl.dma_int_line_num = 0x10;    
    VI_DRV_PIPE_SetDmaCtrl(isp,s8Index,&DmaCtrl);
	VI_PIPE_TPG_TOF_S pstPipeTpgTof;
    pstPipeTpgTof.tof_mode_enable            = vi_pipe_cfg->tof_mode_enable;
    pstPipeTpgTof.vi_pipe_tpg_tof_frm_stride = vi_pipe_cfg->vi_pipe_tpg_tof_frm_stride;
    pstPipeTpgTof.vi_pipe_tpg_tof_frm_num    = vi_pipe_cfg->vi_pipe_tpg_tof_frm_num;
	VI_DRV_PIPE_SetTofTpg(isp,s8Index,&pstPipeTpgTof);
	VI_PIPE_INT_CTL_S pstIntCtl;
    pstIntCtl.win_ctl_frame_end_en         = VI_INT_DIS; 
    pstIntCtl.emb_ctl_frame_end_en         = VI_INT_DIS; 
    pstIntCtl.emb_ctl_time_out_en          = VI_INT_DIS; 
    pstIntCtl.tpg_ctl_frame_end_en         = VI_INT_DIS; 
    pstIntCtl.dma_y_wr_frame_end_en        = VI_INT_DIS; 
    pstIntCtl.dma_y_wr_ch_line_base_en     = VI_INT_DIS; 
    pstIntCtl.dma_y_wr_ch_err_frame_end_en = VI_INT_DIS; 
    pstIntCtl.dma_y_wr_ch_err_immediate_en = VI_INT_DIS; 
    pstIntCtl.dma_uv_wr_ch_frame_end_en    = VI_INT_DIS; 
    pstIntCtl.dma_uv_wr_ch_line_base_en    = VI_INT_DIS; 
    pstIntCtl.dma_uv_wr_ch_err_frame_end_en = VI_INT_DIS;
    pstIntCtl.dma_uv_wr_ch_err_immediate_en = VI_INT_DIS;
    pstIntCtl.dma_raw_rd_ch_frame_end_en   = VI_INT_DIS; 
    pstIntCtl.dma_raw_rd_ch_line_base_en   = VI_INT_DIS; 
    pstIntCtl.dma_raw_rd_ch_err_frame_end_en= VI_INT_DIS;
    pstIntCtl.dma_raw_rd_ch_err_immediate_en= VI_INT_DIS;
	VI_DRV_PIPE_SetInt(isp,s8Index,&pstIntCtl);
	//VI_PIPE_CONFIG_DONE_S pstCfgDone;
    pstCfgDone.pipe_config_done      = 1;
    pstCfgDone.sens_reg_wr_protect   = VI_DONE_WP_EN;//VI_DONE_WP_EN;
    pstCfgDone.int_pol               = VI_INT_HIGH_ACT; //must high
	VI_DRV_PIPE_SetCfgDone(isp,s8Index,&pstCfgDone);

	dev_dbg(vi->isp->dev,"%s:end\n",__func__);
}
/**
 * @brief 
 * 
 */
static int vi_config(struct vi_device *vi)
{
	dev_dbg(vi->isp->dev,"%s:start\n",__func__);
	struct vi_cfg_info *vi_cfg = &vi->vi_cfg;
	struct vi_wrap_cfg_info *vi_wrap_cfg = &vi_cfg->vi_wrap_cfg;
	vi_wrap_config(vi,vi_wrap_cfg);
	//
	struct vi_pipe_cfg_info *vi_pipe_cfg = &vi_cfg->vi_pipe_cfg[0];
	//csi0
	vi_pipe_config(vi,VI_MIPI_CSI00_PIPE_ID,&vi_pipe_cfg[VI_MIPI_CSI00_PIPE_ID]);
	vi_pipe_config(vi,VI_MIPI_CSI01_PIPE_ID,&vi_pipe_cfg[VI_MIPI_CSI01_PIPE_ID]);
	vi_pipe_config(vi,VI_MIPI_CSI02_PIPE_ID,&vi_pipe_cfg[VI_MIPI_CSI02_PIPE_ID]);
	//csi1
	vi_pipe_config(vi,VI_MIPI_CSI10_PIPE_ID,&vi_pipe_cfg[VI_MIPI_CSI10_PIPE_ID]);
	//dvp
	vi_pipe_config(vi,VI_DVP_0_PIPE_ID,&vi_pipe_cfg[VI_MIPI_CSI00_PIPE_ID]);
	dev_dbg(vi->isp->dev,"%s:end\n",__func__);
    return 0;
}
/**
 * @brief 
 * 
 * @param vi 
 */
static void vi_default_cfg(struct vi_device *vi)
{
	struct vi_cfg_info *vi_cfg = &vi->vi_cfg;
	//pipe00
	struct vi_pipe_cfg_info *pipe_info = &vi_cfg->vi_pipe_cfg[VI_MIPI_CSI00_PIPE_ID];
	pipe_info->win_mode_en = TRUE;	
	pipe_info->input_ch_sel = MIPI_INPUT;
	pipe_info->ch_mode_sel = VI_MIPI_BT1120;
    pipe_info->pixel_type = SENSOR_INPUT_RAWRGB;
    pipe_info->yuv_in_format = VI_INPUT_YUV422;
    pipe_info->yuv_out_format = VI_OUTPUT_YUV422;
    pipe_info->yuv422_order = VI_YUV422_YUYV;
    pipe_info->pixel_width = PIXEL_WIDTH_10BIT;
	pipe_info->data_out_timming_ctrl = CTRL_CYCLE_DELAY_4;
	pipe_info->sync_pulse_mode = 0;//0;
	pipe_info->sen_mipi_clk_pol = TRUE;
	pipe_info->sen_mipi_vsync_pol = FALSE;
	pipe_info->sen_mipi_hsync_pol = FALSE;
	pipe_info->sen_mipi_field_pol = TRUE;
	pipe_info->isp_clk_pol = TRUE;
	pipe_info->isp_vsync_pol = TRUE;
	pipe_info->isp_hsync_pol = TRUE;
	pipe_info->isp_field_pol = TRUE;
	//
    //pipe_info->tpg_w_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->tpg_r_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->total_size.Width = isp_size->total_size.Width;//2200;
	//pipe_info->total_size.Height = isp_size->total_size.Height;//1125;
    //pipe_info->in_size.Width = isp_size->in_size.Width;//1920;
	//pipe_info->in_size.Height = isp_size->in_size.Height;//1080;
    pipe_info->w_size_st.Width_st = 0x0;
	pipe_info->w_size_st.Height_st = 0x3fff;
    pipe_info->r_size_st.Width_st = 0x117;
	pipe_info->r_size_st.Height_st = 0x2c;
    //pipe_info->vi_pipe_w_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_addr_stride = isp_addr->vi_tpg_stride;
	pipe_info->tof_mode_enable = DISABLE;
	pipe_info->vi_pipe_tpg_tof_frm_num = 0x3;
	pipe_info->vi_pipe_tpg_tof_frm_stride = 0x10380;
	//pipe01
	pipe_info = &vi_cfg->vi_pipe_cfg[VI_MIPI_CSI01_PIPE_ID];
	pipe_info->win_mode_en = TRUE;	
	pipe_info->input_ch_sel = MIPI_INPUT;
	pipe_info->ch_mode_sel = VI_MIPI_BT1120;
    pipe_info->pixel_type = SENSOR_INPUT_RAWRGB;
    pipe_info->yuv_in_format = VI_INPUT_YUV422;
    pipe_info->yuv_out_format = VI_OUTPUT_YUV422;
    pipe_info->yuv422_order = VI_YUV422_YUYV;
    pipe_info->pixel_width = PIXEL_WIDTH_10BIT;
	pipe_info->data_out_timming_ctrl = CTRL_CYCLE_DELAY_4;
	pipe_info->sync_pulse_mode = 0;//0;
	pipe_info->sen_mipi_clk_pol = TRUE;
	pipe_info->sen_mipi_vsync_pol = FALSE;
	pipe_info->sen_mipi_hsync_pol = FALSE;
	pipe_info->sen_mipi_field_pol = TRUE;
	pipe_info->isp_clk_pol = TRUE;
	pipe_info->isp_vsync_pol = TRUE;
	pipe_info->isp_hsync_pol = TRUE;
	pipe_info->isp_field_pol = TRUE;
	//
    //pipe_info->tpg_w_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->tpg_r_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->total_size.Width = isp_size->total_size.Width;//2200;
	//pipe_info->total_size.Height = isp_size->total_size.Height;//1125;
    //pipe_info->in_size.Width = isp_size->in_size.Width;//1920;
	//pipe_info->in_size.Height = isp_size->in_size.Height;//1080;
    pipe_info->w_size_st.Width_st = 0x0;
	pipe_info->w_size_st.Height_st = 0x3fff;
    pipe_info->r_size_st.Width_st = 0x117;
	pipe_info->r_size_st.Height_st = 0x2c;
    //pipe_info->vi_pipe_w_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_addr_stride = isp_addr->vi_tpg_stride;
	pipe_info->tof_mode_enable = DISABLE;
	pipe_info->vi_pipe_tpg_tof_frm_num = 0x3;
	pipe_info->vi_pipe_tpg_tof_frm_stride = 0x10380;
	//pipe02
	pipe_info = &vi_cfg->vi_pipe_cfg[VI_MIPI_CSI02_PIPE_ID];
	pipe_info->win_mode_en = TRUE;	
	pipe_info->input_ch_sel = MIPI_INPUT;
	pipe_info->ch_mode_sel = VI_MIPI_BT1120;
    pipe_info->pixel_type = SENSOR_INPUT_RAWRGB;
    pipe_info->yuv_in_format = VI_INPUT_YUV422;
    pipe_info->yuv_out_format = VI_OUTPUT_YUV422;
    pipe_info->yuv422_order = VI_YUV422_YUYV;
    pipe_info->pixel_width = PIXEL_WIDTH_10BIT;
	pipe_info->data_out_timming_ctrl = CTRL_CYCLE_DELAY_4;
	pipe_info->sync_pulse_mode = 0;//0;
	pipe_info->sen_mipi_clk_pol = TRUE;
	pipe_info->sen_mipi_vsync_pol = FALSE;
	pipe_info->sen_mipi_hsync_pol = FALSE;
	pipe_info->sen_mipi_field_pol = TRUE;
	pipe_info->isp_clk_pol = TRUE;
	pipe_info->isp_vsync_pol = TRUE;
	pipe_info->isp_hsync_pol = TRUE;
	pipe_info->isp_field_pol = TRUE;
	//
    //pipe_info->tpg_w_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->tpg_r_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->total_size.Width = isp_size->total_size.Width;//2200;
	//pipe_info->total_size.Height = isp_size->total_size.Height;//1125;
    //pipe_info->in_size.Width = isp_size->in_size.Width;//1920;
	//pipe_info->in_size.Height = isp_size->in_size.Height;//1080;
    pipe_info->w_size_st.Width_st = 0x0;
	pipe_info->w_size_st.Height_st = 0x3fff;
    pipe_info->r_size_st.Width_st = 0x117;
	pipe_info->r_size_st.Height_st = 0x2c;
    //pipe_info->vi_pipe_w_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_addr_stride = isp_addr->vi_tpg_stride;
	pipe_info->tof_mode_enable = DISABLE;
	pipe_info->vi_pipe_tpg_tof_frm_num = 0x3;
	pipe_info->vi_pipe_tpg_tof_frm_stride = 0x10380;
	//pipe10
	pipe_info = &vi_cfg->vi_pipe_cfg[VI_MIPI_CSI10_PIPE_ID];
	pipe_info->win_mode_en = TRUE;	
	pipe_info->input_ch_sel = MIPI_INPUT;
	pipe_info->ch_mode_sel = VI_MIPI_BT1120;
    pipe_info->pixel_type = SENSOR_INPUT_RAWRGB;
    pipe_info->yuv_in_format = VI_INPUT_YUV422;
    pipe_info->yuv_out_format = VI_OUTPUT_YUV422;
    pipe_info->yuv422_order = VI_YUV422_YUYV;
    pipe_info->pixel_width = PIXEL_WIDTH_10BIT;
	pipe_info->data_out_timming_ctrl = CTRL_CYCLE_DELAY_4;
	pipe_info->sync_pulse_mode = 0;//0;
	pipe_info->sen_mipi_clk_pol = TRUE;
	pipe_info->sen_mipi_vsync_pol = FALSE;
	pipe_info->sen_mipi_hsync_pol = FALSE;
	pipe_info->sen_mipi_field_pol = TRUE;
	pipe_info->isp_clk_pol = TRUE;
	pipe_info->isp_vsync_pol = TRUE;
	pipe_info->isp_hsync_pol = TRUE;
	pipe_info->isp_field_pol = TRUE;
	//
    //pipe_info->tpg_w_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->tpg_r_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->total_size.Width = isp_size->total_size.Width;//2200;
	//pipe_info->total_size.Height = isp_size->total_size.Height;//1125;
    //pipe_info->in_size.Width = isp_size->in_size.Width;//1920;
	//pipe_info->in_size.Height = isp_size->in_size.Height;//1080;
    pipe_info->w_size_st.Width_st = 0x0;
	pipe_info->w_size_st.Height_st = 0x3fff;
    pipe_info->r_size_st.Width_st = 0x117;
	pipe_info->r_size_st.Height_st = 0x2c;
    //pipe_info->vi_pipe_w_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_addr_stride = isp_addr->vi_tpg_stride;
	pipe_info->tof_mode_enable = DISABLE;
	pipe_info->vi_pipe_tpg_tof_frm_num = 0x3;
	pipe_info->vi_pipe_tpg_tof_frm_stride = 0x10380;
	//dvp
	pipe_info = &vi_cfg->vi_pipe_cfg[VI_DVP_0_PIPE_ID];
	pipe_info->win_mode_en = FALSE;		
	pipe_info->input_ch_sel = DVP_INPUT;
	pipe_info->ch_mode_sel = VI_MIPI_BT1120;
    pipe_info->pixel_type = SENSOR_INPUT_RAWRGB;
    pipe_info->yuv_in_format = VI_INPUT_YUV422;
    pipe_info->yuv_out_format = VI_OUTPUT_YUV422;
    pipe_info->yuv422_order = VI_YUV422_YUYV;
    pipe_info->pixel_width = PIXEL_WIDTH_10BIT;
	pipe_info->data_out_timming_ctrl = CTRL_CYCLE_DELAY_1;
	pipe_info->sync_pulse_mode = 1;//0;
	pipe_info->sen_mipi_clk_pol = TRUE;
	pipe_info->sen_mipi_vsync_pol = TRUE;
	pipe_info->sen_mipi_hsync_pol = TRUE;
	pipe_info->sen_mipi_field_pol = TRUE;
	pipe_info->isp_clk_pol = TRUE;
	pipe_info->isp_vsync_pol = TRUE;
	pipe_info->isp_hsync_pol = TRUE;
	pipe_info->isp_field_pol = TRUE;
	//
    //pipe_info->tpg_w_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->tpg_r_en = isp_addr->vi_tpg_en;//DISABLE;
    //pipe_info->total_size.Width = isp_size->total_size.Width;//2200;
	//pipe_info->total_size.Height = isp_size->total_size.Height;//1125;
    //pipe_info->in_size.Width = isp_size->in_size.Width;//1920;
	//pipe_info->in_size.Height = isp_size->in_size.Height;//1080;
    pipe_info->w_size_st.Width_st = 0x24f;//0x0;
	pipe_info->w_size_st.Height_st = 0x3fff;
    pipe_info->r_size_st.Width_st = 0x0;//0x117;
	pipe_info->r_size_st.Height_st = 0x0;//0x2c;
    //pipe_info->vi_pipe_w_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_w_addr_uv1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y0 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_r_addr_y1 = isp_addr->vi_tpg_baseaddr;
    //pipe_info->vi_pipe_addr_stride = isp_addr->vi_tpg_stride;
	pipe_info->tof_mode_enable = DISABLE;
	pipe_info->vi_pipe_tpg_tof_frm_num = 0x0;
	pipe_info->vi_pipe_tpg_tof_frm_stride = 0x0;

}
/* -----------------------------------------------------------------------------
 * Format- and pipeline-related configuration helpers
 *-----------------------------------------------------------------------------/
/*
 * vi_set_outaddr - Set memory address to save output image
 * @vi: Pointer to ISP vi device.
 * @addr: ISP MMU Mapped 32-bit memory address aligned on 32 byte boundary.
 *
 * Sets the memory address where the output will be saved.
 */
static void vi_set_outaddr(struct vi_device *vi, u32 addr)
{
	struct k510_isp_device *isp = to_isp_device(vi);

}

static void __vi_enable(struct vi_device *vi, int enable)
{
	struct k510_isp_device *isp = to_isp_device(vi);

	vi->running = enable;
}

static int vi_disable(struct vi_device *vi)
{
	unsigned long flags;
	int ret = 0;
}

static void vi_enable(struct vi_device *vi)
{
	__vi_enable(vi, 1);
}

static void vi_reset(struct vi_device *vi)
{
	struct k510_isp_device  *isp = to_isp_device(vi);
	vi_wrap_rst(isp);
	reset_control_reset(isp->reset[VI_RST]);
	reset_control_reset(isp->reset[SENSOR_RST]);
}
/* -----------------------------------------------------------------------------
 * Interrupt handling
 *-----------------------------------------------------------------------------*/
/*
 * k510isp_vi_isr - Configure vi during interframe time.
 * @vi: Pointer to ISP vi device.
 * @events: vi events
 */
int k510isp_vi_isr(struct vi_device *vi, u32 events)
{
	if (vi->state == ISP_PIPELINE_STREAM_STOPPED)
		return 0;

	//if (events & IRQ0STATUS_vi_VD1_IRQ)
	//	vi_vd1_isr(vi);
//
	//vi_lsc_isr(vi, events);
//
	//if (events & IRQ0STATUS_vi_VD0_IRQ)
	//	vi_vd0_isr(vi);
//
	//if (events & IRQ0STATUS_HS_VS_IRQ)
	//	vi_hs_vs_isr(vi);

	return 0;
}
/* -----------------------------------------------------------------------------
 * ISP video operations
 *-----------------------------------------------------------------------------*/
static int vi_video_queue(struct k510isp_video *video, struct k510isp_buffer *buffer)
{
	struct vi_device *vi = &video->isp->isp_vi;
	unsigned long flags;
	bool restart = false;

	if (!(vi->tpg_mode & VI_OUTPUT_W_MEMORY)||!(vi->tpg_mode & VI_OUTPUT_R_MEMORY)||!(vi->tpg_mode & VI_OUTPUT_WR_MEMORY))
		return -ENODEV;

	vi_set_outaddr(vi, buffer->dma);

	/* We now have a buffer queued on the output, restart the pipeline
	 * on the next VI interrupt if running in continuous mode (or when
	 * starting the stream) in external sync mode, or immediately in BT.656
	 * sync mode as no VI interrupt is generated when the VI is stopped
	 * in that case.
	 */
	spin_lock_irqsave(&vi->lock, flags);
	if (vi->state == ISP_PIPELINE_STREAM_CONTINUOUS && !vi->running)
		restart = true;
	else
		vi->underrun = 1;
	spin_unlock_irqrestore(&vi->lock, flags);

	if (restart)
		vi_enable(vi);

	return 0;
}

static const struct k510isp_video_operations vi_video_ops = {
	.queue = vi_video_queue,
};
/* -----------------------------------------------------------------------------
 * V4L2 subdev operations
 *-----------------------------------------------------------------------------*/
/*
 * vi_ioctl - vi module private ioctl's
 * @sd: ISP vi V4L2 subdevice
 * @cmd: ioctl command
 * @arg: ioctl argument
 *
 * Return 0 on success or a negative error code otherwise.
 */
static long vi_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	dev_dbg(vi->isp->dev,"%s:cmd(0x%x)\n",__func__,cmd);
	struct vi_cfg_info *vi_cfg = &vi->vi_cfg;
	switch (cmd) {
	case VIDIOC_K510ISP_VI_CFG:
		//dev_dbg(vi->isp->dev,"%s:cmd(0x%x)\n",__func__,VIDIOC_K510ISP_VI_CFG);
		mutex_lock(&vi->ioctl_lock);
		//ret = vi_config(vi,arg);
		memcpy((void*)vi_cfg,arg,sizeof(struct vi_cfg_info));
		vi_config(vi);
		mutex_unlock(&vi->ioctl_lock);
		break;
	case VIDIOC_K510ISP_SYSCTL_RST_VI:
		mutex_lock(&vi->ioctl_lock);
		reset_control_reset(vi->isp->reset[VI_RST]);
		reset_control_reset(vi->isp->reset[SENSOR_RST]);
		mutex_unlock(&vi->ioctl_lock);
		break;
	default:
		dev_err(vi->isp->dev,"%s:cmd(0x%x) err!\n",__func__,cmd);
		return -ENOIOCTLCMD;
	}

	return 0;
}

static int vi_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				struct v4l2_event_subscription *sub)
{
	if (sub->type != V4L2_EVENT_FRAME_SYNC)
		return -EINVAL;

	/* line number is zero at frame start */
	if (sub->id != 0)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, k510ISP_VI_NEVENTS, NULL);
}

static int vi_unsubscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	return v4l2_event_unsubscribe(fh, sub);
}
/*
 * vi_set_stream - Enable/Disable streaming on the vi module
 * @sd: ISP vi V4L2 subdevice
 * @enable: Enable/disable stream
 *
 * When writing to memory, the vi hardware can't be enabled without a memory
 * buffer to write to. As the s_stream operation is called in response to a
 * STREAMON call without any buffer queued yet, just update the enabled field
 * and return immediately. The vi will be enabled in vi_isr_buffer().
 *
 * When not writing to memory enable the vi immediately.
 */
static int vi_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct k510_isp_device *isp = to_isp_device(vi);
	int ret = 0;

	dev_dbg(vi->isp->dev,"%s:start\n",__func__);
	if (vi->state == ISP_PIPELINE_STREAM_STOPPED) {
		if (enable == ISP_PIPELINE_STREAM_STOPPED)
		{
			vi_reset(vi);
			return 0;
		}			

		//k510isp_subclk_enable(isp, K510_ISP_SUBCLK_VI);
		//struct vi_cfg_info *vi_cfg = &vi->vi_cfg;
		//vi_config(vi);
		//vi_configure(vi);
		//struct vi_cfg_info *vi_cfg = &vi->vi_cfg;

	}

	switch (enable) {
	case ISP_PIPELINE_STREAM_CONTINUOUS:
		k510isp_subclk_enable(isp, K510_ISP_SUBCLK_VI);
		vi_config(vi);
		if (vi->underrun)
			vi_enable(vi);
		vi->underrun = 0;
		break;
	case ISP_PIPELINE_STREAM_STOPPED:
		vi_reset(vi);
		ret = vi_disable(vi);
		k510isp_subclk_disable(isp, K510_ISP_SUBCLK_VI);
		vi->underrun = 0;
		break;
	}

	vi->state = enable;
	return ret;
}

static struct v4l2_mbus_framefmt *
__vi_get_format(struct vi_device *vi, struct v4l2_subdev_pad_config *cfg,
		  unsigned int pad, enum v4l2_subdev_format_whence which)
{
	dev_dbg(vi->isp->dev,"%s:start\n",__func__);
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return v4l2_subdev_get_try_format(&vi->subdev, cfg, pad);
	else
		return &vi->formats[pad];
}

static struct v4l2_rect *
__vi_get_crop(struct vi_device *vi, struct v4l2_subdev_pad_config *cfg,
		enum v4l2_subdev_format_whence which)
{
	//if (which == V4L2_SUBDEV_FORMAT_TRY)
	//	return v4l2_subdev_get_try_crop(&vi->subdev, cfg, CCDC_PAD_SOURCE_OF);
	//else
	//	return &vi->crop;
}
/*
 * vi_try_format - Try video format on a pad
 * @vi: ISP vi device
 * @cfg : V4L2 subdev pad configuration
 * @pad: Pad number
 * @fmt: Format
 */
static void vi_try_format(struct vi_device *vi, struct v4l2_subdev_pad_config *cfg,
		unsigned int pad, struct v4l2_mbus_framefmt *fmt,
		enum v4l2_subdev_format_whence which)
{
	struct v4l2_mbus_framefmt *format;

	return 0;
}
/*
 * vi_try_crop - Validate a crop rectangle
 * @vi: ISP VI device
 * @sink: format on the sink pad
 * @crop: crop rectangle to be validated
 */
static void vi_try_crop(struct vi_device *vi,
			  const struct v4l2_mbus_framefmt *sink,
			  struct v4l2_rect *crop)
{
	const struct isp_format_info *info;
	unsigned int max_width;

	return 0;
}
/*
 * vi_enum_mbus_code - Handle pixel format enumeration
 * @sd     : pointer to v4l2 subdev structure
 * @cfg : V4L2 subdev pad configuration
 * @code   : pointer to v4l2_subdev_mbus_code_enum structure
 * return -EINVAL or zero on success
 */
static int vi_enum_mbus_code(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_mbus_code_enum *code)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	return 0;
}

static int vi_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_size_enum *fse)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt format;

	//no used
	return 0;
}
/*
 * vi_get_selection - Retrieve a selection rectangle on a pad
 * @sd: ISP vi V4L2 subdevice
 * @cfg: V4L2 subdev pad configuration
 * @sel: Selection rectangle
 *
 * The only supported rectangles are the crop rectangles on the output formatter
 * source pad.
 *
 * Return 0 on success or a negative error code otherwise.
 */
static int vi_get_selection(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_selection *sel)
{	
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	//no used
	return 0;
}
/*
 * vi_set_selection - Set a selection rectangle on a pad
 * @sd: ISP vi V4L2 subdevice
 * @cfg: V4L2 subdev pad configuration
 * @sel: Selection rectangle
 *
 * The only supported rectangle is the actual crop rectangle on the output
 * formatter source pad.
 *
 * Return 0 on success or a negative error code otherwise.
 */
static int vi_set_selection(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_selection *sel)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	//no used

	return 0;
}
/*
 * vi_get_format - Retrieve the video format on a pad
 * @sd : ISP vi V4L2 subdevice
 * @cfg: V4L2 subdev pad configuration
 * @fmt: Format
 *
 * Return 0 on success or -EINVAL if the pad is invalid or doesn't correspond
 * to the format type.
 */
static int vi_get_format(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *fmt)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	dev_dbg(vi->isp->dev,"%s:fmt->pad(%d),fmt->which(0x%x),start\n",__func__,fmt->pad,fmt->which);
	format = __vi_get_format(vi, cfg, fmt->pad, fmt->which);
	if (format == NULL)
	{
		dev_err(vi->isp->dev, "%s:pad %d,which %d start\n",__func__,fmt->pad,fmt->which);
		return -EINVAL;
	}	

	fmt->format = *format;
	return 0;
}
/*
 * vi_set_format - Set the video format on a pad
 * @sd : ISP vi V4L2 subdevice
 * @cfg: V4L2 subdev pad configuration
 * @fmt: Format
 *
 * Return 0 on success or -EINVAL if the pad is invalid or doesn't correspond
 * to the format type.
 */
static int vi_set_format(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *fmt)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	struct v4l2_rect *crop;

	dev_dbg(vi->isp->dev, "%s:pad %d,which %d start\n",__func__,fmt->pad,fmt->which);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		return 0;
	}

	if(( fmt->pad != VI_PAD_SINK_CSI0)&&( fmt->pad != VI_PAD_SINK_CSI1)&&( fmt->pad != VI_PAD_SINK_DVP) 
		&&( fmt->pad != VI_PAD_SOURCE_F2K)&&( fmt->pad != VI_PAD_SOURCE_R2K) &&( fmt->pad != VI_PAD_SOURCE_TOF))
	{
		dev_warn(vi->isp->dev, "%s:pad error!\n",__func__);
		return -EINVAL;
	}
	format = &fmt->format;
	vi->formats[fmt->pad].width = format->width;
	vi->formats[fmt->pad].height = format->height;
	vi->formats[fmt->pad].field = V4L2_FIELD_NONE;
	vi->formats[fmt->pad].colorspace =V4L2_COLORSPACE_SRGB;
	vi->formats[fmt->pad].code = format->code;//MEDIA_BUS_FMT_SRGGB10_1X10;

	return 0;
}
/*
 * Decide whether desired output pixel code can be obtained with
 * the lane shifter by shifting the input pixel code.
 * @in: input pixelcode to shifter
 * @out: output pixelcode from shifter
 * @additional_shift: # of bits the sensor's LSB is offset from CAMEXT[0]
 *
 * return true if the combination is possible
 * return false otherwise
 */
static bool vi_is_shiftable(u32 in, u32 out, unsigned int additional_shift)
{
	//noused
}

static int vi_link_validate(struct v4l2_subdev *sd,
			      struct media_link *link,
			      struct v4l2_subdev_format *source_fmt,
			      struct v4l2_subdev_format *sink_fmt)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	unsigned long parallel_shift;

	dev_dbg(vi->isp->dev,"%s:start\n",__func__);
	/* Check if the two ends match */
	if (source_fmt->format.width != sink_fmt->format.width ||
	    source_fmt->format.height != sink_fmt->format.height)
	{
		dev_err(vi->isp->dev,"%s:source_fmt->format.width(%d),sink_fmt->format.width(%d),source_fmt->format.height(%d),sink_fmt->format.height(%d),format error\n",
		__func__,source_fmt->format.width,sink_fmt->format.width,source_fmt->format.height,sink_fmt->format.height);
		return -EPIPE;
	}

	/* We've got a parallel sensor here. */
	//if (vi->input == VI_INPUT_DVP) {
	//	struct v4l2_subdev *sd =
	//		media_entity_to_v4l2_subdev(link->source->entity);
	//	struct K510isp_bus_cfg *bus_cfg = v4l2_subdev_to_bus_cfg(sd);
//
	//	parallel_shift = bus_cfg->bus.parallel.data_lane_shift;
	//} else {
	//	parallel_shift = 0;
	//}

	/* Lane shifter may be used to drop bits on CCDC sink pad */
	//if (!vi_is_shiftable(source_fmt->format.code,
	//		       sink_fmt->format.code, parallel_shift))
	//	return -EPIPE;

	return 0;
}
/*
 * vi_init_formats - Initialize formats on all pads
 * @sd: ISP vi V4L2 subdevice
 * @fh: V4L2 subdev file handle
 *
 * Initialize all pad formats with default values. If fh is not NULL, try
 * formats are initialized on the file handle. Otherwise active formats are
 * initialized on the device.
 */
static int vi_init_formats(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct v4l2_subdev_format format;
	dev_dbg(vi->isp->dev, "%s:start\n",__func__);
	//
	unsigned int pad;
	for(pad = VI_PAD_SINK_DVP;pad < VI_PAD_SOURCE_TOF+1;pad++)
	{
		memset(&format, 0, sizeof(format));
		format.pad = pad;
		format.which = fh ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
		format.format.code = vi->formats[pad].code;//MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;
		format.format.width = vi->formats[pad].width;//1920;
		format.format.height = vi->formats[pad].height;//1080;
		vi_set_format(sd, fh ? fh->pad : NULL, &format);		
	}
	
	dev_dbg(vi->isp->dev, "%s:end\n",__func__);
	return 0;
}
/* V4L2 subdev core operations */
static const struct v4l2_subdev_core_ops vi_v4l2_core_ops = {
	.ioctl = vi_ioctl,
	.subscribe_event = vi_subscribe_event,
	.unsubscribe_event = vi_unsubscribe_event,
};
/* V4L2 subdev video operations */
static const struct v4l2_subdev_video_ops vi_v4l2_video_ops = {
	.s_stream = vi_set_stream,
};
/* V4L2 subdev pad operations */
static const struct v4l2_subdev_pad_ops vi_v4l2_pad_ops = {
	.enum_mbus_code = vi_enum_mbus_code,
	.enum_frame_size = vi_enum_frame_size,
	.get_fmt = vi_get_format,
	.set_fmt = vi_set_format,
	.get_selection = vi_get_selection,
	.set_selection = vi_set_selection,
	.link_validate = vi_link_validate,
};

/* V4L2 subdev operations */
static const struct v4l2_subdev_ops vi_v4l2_ops = {
	.core = &vi_v4l2_core_ops,
	.video = &vi_v4l2_video_ops,
	.pad = &vi_v4l2_pad_ops,
};

/* V4L2 subdev internal operations */
static const struct v4l2_subdev_internal_ops vi_v4l2_internal_ops = {
	.open = vi_init_formats,
};
/* -----------------------------------------------------------------------------
 * Media entity operations
 *-----------------------------------------------------------------------------*/
/*
 * vi_link_setup - Setup vi connections
 * @entity: vi media entity
 * @local: Pad at the local end of the link
 * @remote: Pad at the remote end of the link
 * @flags: Link flags
 *
 * return -EINVAL or zero on success
 */
static int vi_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
	struct vi_device *vi = v4l2_get_subdevdata(sd);
	struct k510_isp_device *isp = to_isp_device(vi);
	unsigned int index = local->index;

	dev_dbg(isp->dev,"%s:index(%d) flags(0x%x)start\n",__func__,index,flags);
	/* FIXME: this is actually a hack! */
	if (is_media_entity_v4l2_subdev(remote->entity))
		index |= 2 << 16;

	switch (index) {
		
	case VI_PAD_SINK_DVP:
	case VI_PAD_SINK_DVP | 2 << 16:
		/* Read from the sensor (parallel interface) */
		if (flags & MEDIA_LNK_FL_ENABLED) {
			vi->input |= VI_INPUT_DVP;
		} else {
			if (vi->input == VI_INPUT_DVP)
				vi->input &= ~VI_INPUT_DVP;
		}
		break;
	case VI_PAD_SINK_CSI0:
	case VI_PAD_SINK_CSI0 | 2 << 16:
		/* Read from the sensor  CSI0  */
		if (flags & MEDIA_LNK_FL_ENABLED) {
			vi->input |= VI_INPUT_CSI0;
		} else {
			if (vi->input == VI_INPUT_CSI0)
				vi->input &= ~VI_INPUT_CSI0;
		}
		break;
	case VI_PAD_SINK_CSI1:
	case VI_PAD_SINK_CSI1 | 2 << 16:
		/* Read from the sensor  CSI1  */
		if (flags & MEDIA_LNK_FL_ENABLED) {
			vi->input |= VI_INPUT_CSI1;
		} else {
			if (vi->input == VI_INPUT_CSI1)
				vi->input &= ~VI_INPUT_CSI1;
		}
		break;
	case VI_PAD_SOURCE_F2K:
	case VI_PAD_SOURCE_F2K | 2 << 16:
		/* write to video port f2k */
		if (flags & MEDIA_LNK_FL_ENABLED)
			vi->output |= VI_OUTPUT_F2K;
		else
			vi->output &= ~VI_OUTPUT_F2K;
		break;
	case VI_PAD_SOURCE_R2K:
	case VI_PAD_SOURCE_R2K | 2 << 16:
		/* write to video port r2k */
		if (flags & MEDIA_LNK_FL_ENABLED)
			vi->output |= VI_OUTPUT_R2K;
		else
			vi->output &= ~VI_OUTPUT_R2K;
		break;
	case VI_PAD_SOURCE_TOF:
	case VI_PAD_SOURCE_TOF | 2 << 16:
		/* write to video port tof */
		if (flags & MEDIA_LNK_FL_ENABLED)
			vi->output |= VI_OUTPUT_TOF;
		else
			vi->output &= ~VI_OUTPUT_TOF;
		break;
	default:
		dev_err(isp->dev,"%s:ret(%d)\n",__func__,EINVAL);
		return -EINVAL;
	}

	dev_dbg(isp->dev,"%s:vi->input(%d) end\n",__func__,vi->input);
	return 0;
}
/* media operations */
static const struct media_entity_operations vi_media_ops = {
	.link_setup = vi_link_setup,
	.link_validate = v4l2_subdev_link_validate,
};
/**
 * @brief 
 *  k510isp_vi_unregister_entities- Unregister media entities: subdev
 * @param vi  : Pointer to ISP vi device
 */
void k510isp_vi_unregister_entities(struct vi_device *vi)
{
	dev_dbg(vi->isp->dev,"%s:start\n",__func__);
	v4l2_device_unregister_subdev(&vi->subdev);
	k510isp_video_unregister(&vi->video_in);
	k510isp_video_unregister(&vi->video_out);
}
/*
 * k510isp_vi_register_entities - Register the subdev media entity
 * @vi: Pointer to ISP vi device
 * @vdev: Pointer to v4l device
 * return negative error code or zero on success
 */
int k510isp_vi_register_entities(struct vi_device *vi,
	struct v4l2_device *vdev)
{
	int ret = 0;
	dev_dbg(vi->isp->dev,"%s:start\n",__func__);
	/* Register the subdev and video node. */
	ret = v4l2_device_register_subdev(vdev, &vi->subdev);
	if (ret < 0)
	{
		dev_err(vi->isp->dev, "%s: v4l2_device_register_subdev failed (%d)\n",
			__func__, ret);
		goto error;
	}

	ret = k510isp_video_register(&vi->video_in, vdev);
	if (ret < 0)
	{
		dev_err(vi->isp->dev, "%s: k510isp_video_register failed (%d)\n",
			__func__, ret);
		goto error;
	}

	ret = k510isp_video_register(&vi->video_out, vdev);
	if (ret < 0)
	{
		dev_err(vi->isp->dev, "%s: k510isp_video_register failed (%d)\n",
			__func__, ret);
		goto error;
	}

	dev_dbg(vi->isp->dev,"k510isp_vi_register_entities end\n");
	return 0;

error:
	k510isp_vi_unregister_entities(vi);
	dev_err(vi->isp->dev, "%s: failed (%d)\n",__func__, ret);
	return ret;
}
/* -----------------------------------------------------------------------------
 * video in initialisation and cleanup
 *-----------------------------------------------------------------------------*/
/*
 * vi_init_entities - Initialize V4L2 subdev and media entity
 * @vi: ISP VI module
 *
 * Return 0 on success and a negative error code on failure.
 */
static int vi_init_entities(struct vi_device *vi)
{
	struct v4l2_subdev *sd = &vi->subdev;
	struct media_pad *pads = vi->pads;
	struct media_entity *me = &sd->entity;
	int ret = 0;

	dev_dbg(vi->isp->dev, "%s:start\n",__func__);
	vi->tpg_mode = VI_OUTPUT_NONE;

	v4l2_subdev_init(sd, &vi_v4l2_ops);
	sd->internal_ops = &vi_v4l2_internal_ops;
	strlcpy(sd->name, "K510 ISP VI", sizeof(sd->name));
	sd->grp_id = 1 << 16;	/* group ID for isp subdevs */
	v4l2_set_subdevdata(sd, vi);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	pads[VI_PAD_SINK_DVP].flags = MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	pads[VI_PAD_SINK_CSI0].flags = MEDIA_PAD_FL_SINK| MEDIA_PAD_FL_MUST_CONNECT;
	pads[VI_PAD_SINK_CSI1].flags = MEDIA_PAD_FL_SINK| MEDIA_PAD_FL_MUST_CONNECT;
	pads[VI_PAD_SOURCE_F2K].flags = MEDIA_PAD_FL_SOURCE;
	pads[VI_PAD_SOURCE_R2K].flags = MEDIA_PAD_FL_SOURCE;
	pads[VI_PAD_SOURCE_TOF].flags = MEDIA_PAD_FL_SOURCE;

	me->function = MEDIA_ENT_F_IO_V4L;
	me->ops = &vi_media_ops;
	ret = media_entity_pads_init(me, VI_PADS_NUM, pads);
	if (ret < 0)
	{
		dev_err(vi->isp->dev,"%s: could not media_entity_pads_init (%d)\n",
			__func__, ret);		
		return ret;
	}	

	vi_init_formats(sd, NULL);

	vi->video_in.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vi->video_in.ops = &vi_video_ops;
	vi->video_in.isp = vi->isp;//to_isp_device(vi);
	vi->video_in.capture_mem = PAGE_ALIGN(4096 * 4096) * 3;
	vi->video_in.bpl_alignment = 32;

	ret = k510isp_video_init(&vi->video_in, "VI");
	if (ret < 0)
	{
		dev_err(vi->isp->dev,"%s: could not k510isp_video_init in(%d)\n",
			__func__, ret);	
		goto error;
	}

	vi->video_out.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vi->video_out.ops = &vi_video_ops;
	vi->video_out.isp = vi->isp;//to_isp_device(vi);
	vi->video_out.capture_mem = PAGE_ALIGN(4096 * 4096) * 3;
	vi->video_out.bpl_alignment = 32;

	ret = k510isp_video_init(&vi->video_out, "VI");
	if (ret < 0)
	{
		dev_err(vi->isp->dev,"%s: could not k510isp_video_init in(%d)\n",
			__func__, ret);	
		goto error;
	}

	dev_dbg(vi->isp->dev, "%s:end\n",__func__);
	return 0;
error:
	media_entity_cleanup(me);
	dev_dbg(vi->isp->dev, "%s:end ret:(%d)\n",__func__,ret);
	return ret;
}

/*
 * k510isp_vi_init - VI module initialization.
 * @isp: Device pointer specific to the OMAP3 ISP.
 *
 * TODO: Get the initialisation values from platform data.
 *
 * Return 0 on success or a negative error code otherwise.
 */
int k510isp_vi_init(struct k510_isp_device *isp)
{
	struct vi_device *vi = &isp->isp_vi;
	int ret = 0;
	dev_info(isp->dev,"%s:start\n",__func__);

	vi->isp = isp;
	vi->input = VI_INPUT_NONE;
	vi->output = VI_OUTPUT_NONE;	
	spin_lock_init(&vi->lock);
	init_waitqueue_head(&vi->wait);
	mutex_init(&vi->ioctl_lock);
//
//	vi->stopping = VI_STOP_NOT_REQUESTED;
//
	unsigned int pad;
	for(pad = VI_PAD_SINK_DVP;pad < VI_PAD_SOURCE_TOF+1;pad++)
	{
		vi->formats[pad].code = MEDIA_BUS_FMT_SRGGB10_1X10;
		vi->formats[pad].width = 1920;
		vi->formats[pad].height = 1080;
	}

	ret = vi_init_entities(vi);
	if (ret < 0) {
		mutex_destroy(&vi->ioctl_lock);
		dev_err(isp->dev,"%s: could not vi_init_entities out(%d)\n",
			__func__, ret);	
		return ret;
	}
//
	vi_reset(vi);
	dev_dbg(isp->dev,"%s:end\n",__func__);
	return 0;
}
/*
 * k510isp_vi_cleanup - VI module cleanup.
 * @isp: Device pointer specific to the K510 ISP.
 */
void k510isp_vi_cleanup(struct k510_isp_device *isp)
{
	struct vi_device *vi = &isp->isp_vi;
	dev_info(isp->dev,"%s:start\n",__func__);
	k510isp_video_cleanup(&vi->video_in);
	k510isp_video_cleanup(&vi->video_out);
	media_entity_cleanup(&vi->subdev.entity);

	mutex_destroy(&vi->ioctl_lock);
}