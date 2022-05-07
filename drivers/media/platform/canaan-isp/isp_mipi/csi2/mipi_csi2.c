/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/delay.h>
#include <media/v4l2-common.h>
#include <linux/v4l2-mediabus.h>
#include <linux/mm.h>
#include <linux/reset.h>
#include <media/media-entity.h>

#include "../../k510isp_com.h"
#include "../../k510isp_video.h"
#include "../../k510_isp.h"
#include "../../k510video_subsystem.h"
#include "mipi_csi2_reg.h"
#include "mipi_csi2.h"

/**
 * @brief 
 * 
 * @param isp 
 */
void k510isp_csi2_print_cfg(struct k510_isp_device *isp)
{
	struct isp_csi2_device *csi2 = &isp->isp_csi2;
	struct isp_csi2_info *csi2_info = &csi2->csi2_info;
	dev_dbg(csi2->isp->dev,"%s:csi0_used:%d\n",__func__,csi2_info->csi0_used);
	dev_dbg(csi2->isp->dev,"%s:csi0_sony_wdr:%d\n",__func__,csi2_info->csi0_sony_wdr);
	dev_dbg(csi2->isp->dev,"%s:csi0_lane_nb:%d\n",__func__,csi2_info->csi0_lane_nb);
	dev_dbg(csi2->isp->dev,"%s:csi0_dl0_map:%d\n",__func__,csi2_info->csi0_dl0_map);
	dev_dbg(csi2->isp->dev,"%s:csi0_dl1_map:%d\n",__func__,csi2_info->csi0_dl1_map);
	dev_dbg(csi2->isp->dev,"%s:csi0_dl2_map:%d\n",__func__,csi2_info->csi0_dl2_map);
	dev_dbg(csi2->isp->dev,"%s:csi0_dl3_map:%d\n",__func__,csi2_info->csi0_dl3_map);
	dev_dbg(csi2->isp->dev,"%s:csi00_datatype_select0:%d\n",__func__,csi2_info->csi00_datatype_select0);
	dev_dbg(csi2->isp->dev,"%s:csi00_datatype_select1:%d\n",__func__,csi2_info->csi00_datatype_select1);
	dev_dbg(csi2->isp->dev,"%s:csi00_vc_select:%d\n",__func__,csi2_info->csi00_vc_select);
	dev_dbg(csi2->isp->dev,"%s:csi01_datatype_select0:%d\n",__func__,csi2_info->csi01_datatype_select0);
	dev_dbg(csi2->isp->dev,"%s:csi01_datatype_select1:%d\n",__func__,csi2_info->csi01_datatype_select1);
	dev_dbg(csi2->isp->dev,"%s:csi01_vc_select:%d\n",__func__,csi2_info->csi01_vc_select); 
	dev_dbg(csi2->isp->dev,"%s:csi02_datatype_select0:%d\n",__func__,csi2_info->csi02_datatype_select0);
	dev_dbg(csi2->isp->dev,"%s:csi02_datatype_select1:%d\n",__func__,csi2_info->csi02_datatype_select1);
	dev_dbg(csi2->isp->dev,"%s:csi02_vc_select:%d\n",__func__,csi2_info->csi02_vc_select);
	dev_dbg(csi2->isp->dev,"%s:csi1_used:%d\n",__func__,csi2_info->csi1_used);
	dev_dbg(csi2->isp->dev,"%s:csi1_sony_wdr:%d\n",__func__,csi2_info->csi1_sony_wdr);
	dev_dbg(csi2->isp->dev,"%s:csi1_lane_nb:%d\n",__func__,csi2_info->csi1_lane_nb);
	dev_dbg(csi2->isp->dev,"%s:csi1_dl0_map:%d\n",__func__,csi2_info->csi1_dl0_map);
	dev_dbg(csi2->isp->dev,"%s:csi1_dl1_map:%d\n",__func__,csi2_info->csi1_dl1_map);
	dev_dbg(csi2->isp->dev,"%s:csi1_dl2_map:%d\n",__func__,csi2_info->csi1_dl2_map);
	dev_dbg(csi2->isp->dev,"%s:csi1_dl3_map:%d\n",__func__,csi2_info->csi1_dl3_map);
	dev_dbg(csi2->isp->dev,"%s:csi10_datatype_select0:%d\n",__func__,csi2_info->csi10_datatype_select0);
	dev_dbg(csi2->isp->dev,"%s:csi10_datatype_select1:%d\n",__func__,csi2_info->csi10_datatype_select1);
	dev_dbg(csi2->isp->dev,"%s:csi10_vc_select:%d\n",__func__,csi2_info->csi10_vc_select);    
}
/*
 * csi2_reset - Resets the CSI2 module.
 *
 * Must be called with the phy lock held.
 *
 * Returns 0 if successful, or -EBUSY if power command didn't respond.
 */
int k510isp_csi2_reset(struct isp_csi2_device *csi2)
{
	struct k510_isp_device *isp = to_isp_device(csi2);
	u8 soft_reset_retries = 0;
	u32 reg;
	int i;

	if (!csi2->available)
		return -ENODEV;

	if (csi2->dphy->entity)
		return -EBUSY;

//csi0
    union U_CSI_RX_STREAM_SFT_RST csi0_sft_rst;
    csi0_sft_rst.udata = 0;
    csi0_sft_rst.bits.front = 1;
    csi0_sft_rst.bits.protocol = 1;
    isp_reg_writel(isp,csi0_sft_rst.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_SFT_RST);
//
    csi0_sft_rst.udata = 0;
    csi0_sft_rst.bits.front = 0;
    csi0_sft_rst.bits.protocol = 0;
    isp_reg_writel(isp,csi0_sft_rst.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_SFT_RST);
//csi1
    union U_CSI_RX_STREAM_SFT_RST csi1_sft_rst;
    csi1_sft_rst.udata = 0;
    csi1_sft_rst.bits.front = 1;
    csi1_sft_rst.bits.protocol = 1;
    isp_reg_writel(isp,csi1_sft_rst.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_SFT_RST);
//
    csi1_sft_rst.udata = 0;
    csi1_sft_rst.bits.front = 0;
    csi1_sft_rst.bits.protocol = 0;
    isp_reg_writel(isp,csi1_sft_rst.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_SFT_RST);

	return 0;
}

int csi2_enable(struct k510_isp_device *isp,unsigned int enable)
{
	union U_CSI_RX_STREAM_CTRL  csi00_ctrl;
    csi00_ctrl.udata = 0;
    csi00_ctrl.bits.start = enable; 
    isp_reg_writel(isp,csi00_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_CTRL);

	union U_CSI_RX_STREAM_CTRL  csi01_ctrl;
    csi01_ctrl.udata = 0;
    csi01_ctrl.bits.start = enable; 
    isp_reg_writel(isp,csi01_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_CTRL);

	union U_CSI_RX_STREAM_CTRL  csi02_ctrl;
    csi02_ctrl.udata = 0;
    csi02_ctrl.bits.start = enable; 
    isp_reg_writel(isp,csi02_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_CTRL);

	union U_CSI_RX_STREAM_CTRL  csi10_ctrl;
    csi10_ctrl.udata = 0;
    csi10_ctrl.bits.start = enable; 
    isp_reg_writel(isp,csi10_ctrl.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_CTRL);	
	return 0;
}
/*
*
*/
void csi2_configure(struct k510_isp_device *isp)
{
	struct isp_csi2_device *csi2 = &isp->isp_csi2;
	struct isp_csi2_info *csi2_info = &csi2->csi2_info;
    //csi0
    union U_CSI_RX_STREAM_SFT_RST csi0_sft_rst;
    csi0_sft_rst.udata = 0;
    csi0_sft_rst.bits.front = 1;
    csi0_sft_rst.bits.protocol = 1;
    isp_reg_writel(isp,csi0_sft_rst.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_SFT_RST);
///
    csi0_sft_rst.udata = 0;
    csi0_sft_rst.bits.front = 0;
    csi0_sft_rst.bits.protocol = 0;
    isp_reg_writel(isp,csi0_sft_rst.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_SFT_RST);

    union U_CSI_RX_STREAM_STATIC_CFG csi0_static_cfg;
    csi0_static_cfg.udata = 0;    
    csi0_static_cfg.bits.lane_nb            = csi2_info->csi0_lane_nb;
    csi0_static_cfg.bits.sony_wdr           = csi2_info->csi0_sony_wdr;
    csi0_static_cfg.bits.dl0_map            = csi2_info->csi0_dl0_map;
    csi0_static_cfg.bits.dl1_map            = csi2_info->csi0_dl1_map;
    csi0_static_cfg.bits.dl2_map            = csi2_info->csi0_dl2_map;
    csi0_static_cfg.bits.dl3_map            = csi2_info->csi0_dl3_map;
    isp_reg_writel(isp,csi0_static_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_STATIC_CFG);
//
    union U_CSI_RX_STREAM_ERR_BYPASS_CFG csi0_bypass_cfg;
    csi0_bypass_cfg.udata = 0;
    csi0_bypass_cfg.bits.crc = 1;    
    csi0_bypass_cfg.bits.ecc = 1;    
    csi0_bypass_cfg.bits.data_id = 1;  
    isp_reg_writel(isp,csi0_bypass_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_ERR_BYPASS_CFG);
//
    union U_CSI_RX_STREAM_MONITOR_IRQS csi0_irqs_mon_info;
    csi0_irqs_mon_info.udata = 0;
    isp_reg_writel(isp,csi0_irqs_mon_info.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_MONITOR_IRQS);
//
    union U_CSI_RX_STREAM_MONITOR_IRQS_MASK_CFG  csi0_irqs_mon_mask_info;
    csi0_irqs_mon_mask_info.udata = 0;
    isp_reg_writel(isp,csi0_irqs_mon_mask_info.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_MONITOR_IRQS_MASK_CFG);          
//
    union U_CSI_RX_STREAM_INFO_IRQS  csi0_irqs_info;
    csi0_irqs_info.udata = 0;
    isp_reg_writel(isp,csi0_irqs_info.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_INFO_IRQS); 
//
    union U_CSI_RX_STREAM_INFO_IRQS_MASK_CFG  csi0_irqs_mask_info;
    csi0_irqs_mask_info.udata = 0;
    isp_reg_writel(isp,csi0_irqs_mask_info.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_INFO_IRQS_MASK_CFG);  
//
    union U_CSI_RX_STREAM_ERROR_IRQS  csi0_irqs_err_info;
    csi0_irqs_err_info.udata = 0;
    isp_reg_writel(isp,csi0_irqs_err_info.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_ERROR_IRQS);                 
//   
    union U_CSI_RX_STREAM_ERROR_IRQS_MASK_CFG  csi0_irqs_err_mask_info;
    csi0_irqs_err_mask_info.udata = 0;
    isp_reg_writel(isp,csi0_irqs_err_mask_info.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM_ERROR_IRQS_MASK_CFG);              
//
    union U_CSI_RX_DPHY_LANE_CONTROL  csi0_lane_ctl;
    csi0_lane_ctl.udata = 0;
    csi0_lane_ctl.bits.dl0_enable = csi2_info->csi0_used;
    csi0_lane_ctl.bits.dl1_enable = csi2_info->csi0_used;
	csi0_lane_ctl.bits.cl_enable  = csi2_info->csi0_used;
    csi0_lane_ctl.bits.dl0_reset  = csi2_info->csi0_used;
    csi0_lane_ctl.bits.dl1_reset  = csi2_info->csi0_used;
	csi0_lane_ctl.bits.cl_reset   = csi2_info->csi0_used; 
	csi0_lane_ctl.bits.dl2_enable = 0;
	csi0_lane_ctl.bits.dl3_enable = 0;
	csi0_lane_ctl.bits.dl2_reset  = 0;
	csi0_lane_ctl.bits.dl3_reset  = 0;
	if(csi2_info->csi0_lane_nb == 4)
	{
    	csi0_lane_ctl.bits.dl2_enable = csi2_info->csi0_used;
    	csi0_lane_ctl.bits.dl3_enable = csi2_info->csi0_used;
    	csi0_lane_ctl.bits.dl2_reset  = csi2_info->csi0_used;
    	csi0_lane_ctl.bits.dl3_reset  = csi2_info->csi0_used;
	}        
    isp_reg_writel(isp,csi0_lane_ctl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_DPHY_LANE_CONTROL);
//
    union U_CSI_RX_DPHY_ERR_STATUS_IRQ  csi0_status_irq;
    csi0_status_irq.udata = 0;
    isp_reg_writel(isp,csi0_status_irq.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_DPHY_ERR_STATUS_IRQ);
//
    union U_CSI_RX_DPHY_ERR_IRQ_MASK  csi0_status_mask_irq;
    csi0_status_mask_irq.udata = 0;
    isp_reg_writel(isp,csi0_status_mask_irq.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_DPHY_ERR_IRQ_MASK);
//stream 0
    union U_CSI_RX_STREAM_CTRL  csi00_ctrl;
    csi00_ctrl.udata = 0;
    csi00_ctrl.bits.start = csi2_info->csi0_used;   
    csi00_ctrl.bits.stop = 0;    
    csi00_ctrl.bits.soft_rst= csi2_info->csi0_used; 
    isp_reg_writel(isp,csi00_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_CTRL);

    union U_CSI_RX_STREAM_DATA_CFG  csi00_data_cfg;
    csi00_data_cfg.udata = 0;
    csi00_data_cfg.bits.datatype_select0 = csi2_info->csi00_datatype_select0;   
    csi00_data_cfg.bits.enable_dt0       = 1;
    csi00_data_cfg.bits.datatype_select1 = csi2_info->csi00_datatype_select1;    
    csi00_data_cfg.bits.enable_dt1       = 0;
    csi00_data_cfg.bits.vc_select        = 1;
	//printk("udata(0x%x),csi00_datatype_select0(0x%x),csi00_datatype_select1(0x%x)\n",csi00_data_cfg.udata,csi2_info->csi00_datatype_select0,csi2_info->csi00_datatype_select1);
    isp_reg_writel(isp,csi00_data_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_DATA_CFG);

    union U_CSI_RX_STREAM_CFG  csi00_cfg;
    csi00_cfg.udata = 0;
    isp_reg_writel(isp,csi00_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_CFG);

    union U_CSI_RX_STREAM_MONITOR_CTRL  csi00_mon_ctrl;
    csi00_mon_ctrl.udata = 0;
    isp_reg_writel(isp,csi00_mon_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_MONITOR_CTRL);

    union U_CSI_RX_STREAM_TIMER  csi00_timer;
    csi00_timer.udata = 0;
    isp_reg_writel(isp,csi00_timer.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_TIMER);

    union U_CSI_RX_STREAM_FCC_CFG  csi00_fcc_cfg;
    csi00_fcc_cfg.udata = 0;
    isp_reg_writel(isp,csi00_fcc_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_FCC_CFG);

    union U_CSI_RX_STREAM_FCC_CTRL  csi00_fcc_ctrl;
    csi00_fcc_ctrl.udata = 0;
    isp_reg_writel(isp,csi00_fcc_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_FCC_CTRL);

    union U_CSI_RX_STREAM_FCC_FILL_LVL csi00_fcc_fill_lvl;
    csi00_fcc_fill_lvl.udata = 0;
    isp_reg_writel(isp,csi00_fcc_fill_lvl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_FCC_FILL_LVL);

    csi00_ctrl.udata = 0;
    csi00_ctrl.bits.start = csi2_info->csi0_used; 
    isp_reg_writel(isp,csi00_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM0_CTRL);
//stream 1
    union U_CSI_RX_STREAM_CTRL  csi01_ctrl;
    csi01_ctrl.udata = 0;
    csi01_ctrl.bits.start = csi2_info->csi0_used;   
    csi01_ctrl.bits.stop = 0;    
    csi01_ctrl.bits.soft_rst= csi2_info->csi0_used; 
    isp_reg_writel(isp,csi01_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_CTRL);

    union U_CSI_RX_STREAM_DATA_CFG  csi01_data_cfg;
    csi01_data_cfg.udata = 0;
    csi01_data_cfg.bits.datatype_select0 = csi2_info->csi01_datatype_select0;   
    csi01_data_cfg.bits.enable_dt0       = 1;
    csi01_data_cfg.bits.datatype_select1 = csi2_info->csi01_datatype_select1;    
    csi01_data_cfg.bits.enable_dt1       = 0;
    csi01_data_cfg.bits.vc_select        = 2;
    isp_reg_writel(isp,csi01_data_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_DATA_CFG);

    union U_CSI_RX_STREAM_CFG  csi01_cfg;
    csi01_cfg.udata = 0;
    isp_reg_writel(isp,csi01_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_CFG);

    union U_CSI_RX_STREAM_MONITOR_CTRL  csi01_mon_ctrl;
    csi01_mon_ctrl.udata = 0;
    isp_reg_writel(isp,csi01_mon_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_MONITOR_CTRL);

    union U_CSI_RX_STREAM_TIMER  csi01_timer;
    csi01_timer.udata = 0;
    isp_reg_writel(isp,csi01_timer.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_TIMER);

    union U_CSI_RX_STREAM_FCC_CFG  csi01_fcc_cfg;
    csi01_fcc_cfg.udata = 0;
    isp_reg_writel(isp,csi01_fcc_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_FCC_CFG);

    union U_CSI_RX_STREAM_FCC_CTRL  csi01_fcc_ctrl;
    csi01_fcc_ctrl.udata = 0;
    isp_reg_writel(isp,csi01_fcc_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_FCC_CTRL);

    union U_CSI_RX_STREAM_FCC_FILL_LVL csi01_fcc_fill_lvl;
    csi01_fcc_fill_lvl.udata = 0;
    isp_reg_writel(isp,csi01_fcc_fill_lvl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_FCC_FILL_LVL);

    csi01_ctrl.udata = 0;
    csi01_ctrl.bits.start = csi2_info->csi0_used; 
    isp_reg_writel(isp,csi01_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM1_CTRL);
//stream 2
    union U_CSI_RX_STREAM_CTRL  csi02_ctrl;
    csi02_ctrl.udata = 0;
    csi02_ctrl.bits.start = csi2_info->csi0_used;   
    csi02_ctrl.bits.stop = 0;    
    csi02_ctrl.bits.soft_rst= csi2_info->csi0_used; 
    isp_reg_writel(isp,csi02_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_CTRL);

    union U_CSI_RX_STREAM_DATA_CFG  csi02_data_cfg;
    csi02_data_cfg.udata = 0;
    csi02_data_cfg.bits.datatype_select0 = csi2_info->csi02_datatype_select0;   
    csi02_data_cfg.bits.enable_dt0       = 1;
    csi02_data_cfg.bits.datatype_select1 = csi2_info->csi02_datatype_select1;    
    csi02_data_cfg.bits.enable_dt1       = 0;
    csi02_data_cfg.bits.vc_select        = 4;
    isp_reg_writel(isp,csi02_data_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_DATA_CFG);

    union U_CSI_RX_STREAM_CFG  csi02_cfg;
    csi02_cfg.udata = 0;
    isp_reg_writel(isp,csi02_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_CFG);

    union U_CSI_RX_STREAM_MONITOR_CTRL  csi02_mon_ctrl;
    csi02_mon_ctrl.udata = 0;
    isp_reg_writel(isp,csi02_mon_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_MONITOR_CTRL);

    union U_CSI_RX_STREAM_TIMER  csi02_timer;
    csi02_timer.udata = 0;
    isp_reg_writel(isp,csi02_timer.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_TIMER);

    union U_CSI_RX_STREAM_FCC_CFG  csi02_fcc_cfg;
    csi02_fcc_cfg.udata = 0;
    isp_reg_writel(isp,csi02_fcc_cfg.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_FCC_CFG);

    union U_CSI_RX_STREAM_FCC_CTRL  csi02_fcc_ctrl;
    csi02_fcc_ctrl.udata = 0;
    isp_reg_writel(isp,csi02_fcc_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_FCC_CTRL);

    union U_CSI_RX_STREAM_FCC_FILL_LVL csi02_fcc_fill_lvl;
    csi02_fcc_fill_lvl.udata = 0;
    isp_reg_writel(isp,csi02_fcc_fill_lvl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_FCC_FILL_LVL);

    csi02_ctrl.udata = 0;
    csi02_ctrl.bits.start = csi2_info->csi0_used; 
    isp_reg_writel(isp,csi02_ctrl.udata,ISP_IOMEM_CSI0_HOST,CSI_RX_STREAM2_CTRL); 

    //csi1
    union U_CSI_RX_STREAM_SFT_RST csi1_sft_rst;
    csi1_sft_rst.udata = 0;
    csi1_sft_rst.bits.front = 1;
    csi1_sft_rst.bits.protocol = 1;
    isp_reg_writel(isp,csi1_sft_rst.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_SFT_RST);
//
    csi1_sft_rst.udata = 0;
    csi1_sft_rst.bits.front = 0;
    csi1_sft_rst.bits.protocol = 0;
    isp_reg_writel(isp,csi1_sft_rst.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_SFT_RST);
//
    union U_CSI_RX_STREAM_STATIC_CFG csi1_static_cfg;
    csi1_static_cfg.udata = 0;    
    csi1_static_cfg.bits.lane_nb            = csi2_info->csi1_lane_nb;
    csi1_static_cfg.bits.sony_wdr           = csi2_info->csi1_sony_wdr;
    csi1_static_cfg.bits.dl0_map            = csi2_info->csi1_dl0_map;
    csi1_static_cfg.bits.dl1_map            = csi2_info->csi1_dl1_map;
    csi1_static_cfg.bits.dl2_map            = csi2_info->csi1_dl2_map;
    csi1_static_cfg.bits.dl3_map            = csi2_info->csi1_dl3_map;
    isp_reg_writel(isp,csi1_static_cfg.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_STATIC_CFG);
//
    union U_CSI_RX_STREAM_ERR_BYPASS_CFG csi1_bypass_cfg;
    csi1_bypass_cfg.udata = 0;
    csi1_bypass_cfg.bits.crc = 1;    
    csi1_bypass_cfg.bits.ecc = 1;    
    csi1_bypass_cfg.bits.data_id = 1;  
    isp_reg_writel(isp,csi1_bypass_cfg.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_ERR_BYPASS_CFG);
//
    union U_CSI_RX_STREAM_MONITOR_IRQS csi1_irqs_mon_info;
    csi1_irqs_mon_info.udata = 0;
    isp_reg_writel(isp,csi1_irqs_mon_info.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_MONITOR_IRQS);
//
    union U_CSI_RX_STREAM_MONITOR_IRQS_MASK_CFG  csi1_irqs_mon_mask_info;
    csi1_irqs_mon_mask_info.udata = 0;
    isp_reg_writel(isp,csi1_irqs_mon_mask_info.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_MONITOR_IRQS_MASK_CFG);          
//
    union U_CSI_RX_STREAM_INFO_IRQS  csi1_irqs_info;
    csi1_irqs_info.udata = 0;
    isp_reg_writel(isp,csi1_irqs_info.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_INFO_IRQS); 
//
    union U_CSI_RX_STREAM_INFO_IRQS_MASK_CFG  csi1_irqs_mask_info;
    csi1_irqs_mask_info.udata = 0;
    isp_reg_writel(isp,csi1_irqs_mask_info.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_INFO_IRQS_MASK_CFG);  
//
    union U_CSI_RX_STREAM_ERROR_IRQS  csi1_irqs_err_info;
    csi1_irqs_err_info.udata = 0;
    isp_reg_writel(isp,csi1_irqs_err_info.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_ERROR_IRQS);                 
//   
    union U_CSI_RX_STREAM_ERROR_IRQS_MASK_CFG  csi1_irqs_err_mask_info;
    csi1_irqs_err_mask_info.udata = 0;
    isp_reg_writel(isp,csi1_irqs_err_mask_info.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM_ERROR_IRQS_MASK_CFG);              
//
    union U_CSI_RX_DPHY_LANE_CONTROL  csi1_lane_ctl;
    csi1_lane_ctl.udata = 0;
    csi1_lane_ctl.bits.dl0_enable = 0;
    csi1_lane_ctl.bits.dl1_enable = 0;
    csi1_lane_ctl.bits.cl_enable  = csi2_info->csi1_used;
    csi1_lane_ctl.bits.dl0_reset  = 0;
    csi1_lane_ctl.bits.dl1_reset  = 0;
    csi1_lane_ctl.bits.cl_reset   = csi2_info->csi1_used; 
    csi1_lane_ctl.bits.dl2_enable = csi2_info->csi1_used;
    csi1_lane_ctl.bits.dl3_enable = csi2_info->csi1_used;
    csi1_lane_ctl.bits.dl2_reset  = csi2_info->csi1_used;
    csi1_lane_ctl.bits.dl3_reset  = csi2_info->csi1_used;
    isp_reg_writel(isp,csi1_lane_ctl.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_DPHY_LANE_CONTROL);
//
    union U_CSI_RX_DPHY_ERR_STATUS_IRQ  csi1_status_irq;
    csi1_status_irq.udata = 0;
    isp_reg_writel(isp,csi1_status_irq.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_DPHY_ERR_STATUS_IRQ);
//
    union U_CSI_RX_DPHY_ERR_IRQ_MASK  csi1_status_mask_irq;
    csi1_status_mask_irq.udata = 0;
    isp_reg_writel(isp,csi1_status_mask_irq.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_DPHY_ERR_IRQ_MASK);
//stream 0
    union U_CSI_RX_STREAM_CTRL  csi10_ctrl;
    csi10_ctrl.udata = 0;
    csi10_ctrl.bits.start = csi2_info->csi1_used;   
    csi10_ctrl.bits.stop = 0;    
    csi10_ctrl.bits.soft_rst= csi2_info->csi1_used; 
    isp_reg_writel(isp,csi10_ctrl.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_CTRL);

    union U_CSI_RX_STREAM_DATA_CFG  csi10_data_cfg;
    csi10_data_cfg.udata = 0;
    csi10_data_cfg.bits.datatype_select0 = csi2_info->csi10_datatype_select0;   
    csi10_data_cfg.bits.enable_dt0       = 1;
    csi10_data_cfg.bits.datatype_select1 = csi2_info->csi10_datatype_select1;    
    csi10_data_cfg.bits.enable_dt1       = 0;
    csi10_data_cfg.bits.vc_select        = 1;
    isp_reg_writel(isp,csi10_data_cfg.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_DATA_CFG);

    union U_CSI_RX_STREAM_CFG  csi10_cfg;
    csi10_cfg.udata = 0;
    isp_reg_writel(isp,csi10_cfg.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_CFG);

    union U_CSI_RX_STREAM_MONITOR_CTRL  csi10_mon_ctrl;
    csi10_mon_ctrl.udata = 0;
    isp_reg_writel(isp,csi10_mon_ctrl.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_MONITOR_CTRL);

    union U_CSI_RX_STREAM_TIMER  csi10_timer;
    csi10_timer.udata = 0;
    isp_reg_writel(isp,csi10_timer.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_TIMER);

    union U_CSI_RX_STREAM_FCC_CFG  csi10_fcc_cfg;
    csi10_fcc_cfg.udata = 0;
    isp_reg_writel(isp,csi10_fcc_cfg.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_FCC_CFG);

    union U_CSI_RX_STREAM_FCC_CTRL  csi10_fcc_ctrl;
    csi10_fcc_ctrl.udata = 0;
    isp_reg_writel(isp,csi10_fcc_ctrl.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_FCC_CTRL);

    union U_CSI_RX_STREAM_FCC_FILL_LVL csi10_fcc_fill_lvl;
    csi10_fcc_fill_lvl.udata = 0;
    isp_reg_writel(isp,csi10_fcc_fill_lvl.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_FCC_FILL_LVL);

    csi10_ctrl.udata = 0;
    csi10_ctrl.bits.start = csi2_info->csi1_used; 
    isp_reg_writel(isp,csi10_ctrl.udata,ISP_IOMEM_CSI1_HOST,CSI_RX_STREAM0_CTRL);        

    return;
}
/*
 * csi2_print_status - Prints CSI2 debug information.
 */
#define CSI2A_PRINT_REGISTER(isp, name)\
	dev_info(isp->dev, "###CSI2A:" #name "(0x%x) = 0x%x\n", \
		isp->mmio_base[ISP_IOMEM_CSI0_HOST] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp, ISP_IOMEM_CSI0_HOST, name))

#define CSI2B_PRINT_REGISTER(isp, name)\
	dev_info(isp->dev, "###CSI2B:" #name "(0x%x) = 0x%x\n", \
		isp->mmio_base[ISP_IOMEM_CSI1_HOST] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp, ISP_IOMEM_CSI1_HOST, name))

static void csi2_print_status(struct isp_csi2_device *csi2)
{
	struct k510_isp_device *isp = to_isp_device(csi2);

	if (!csi2->available)
		return;

	dev_info(isp->dev, "-------------CSI2 Register dump-------------\n");

	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_DEVICE_CFG            );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_SFT_RST               );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_STATIC_CFG            );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_CFG_ADDR              );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_ERR_BYPASS_CFG        );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_MONITOR_FRAME         );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_MONITOR_IRQS          );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_MONITOR_IRQS_MASK_CFG );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_INFO_IRQS             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_INFO_IRQS_MASK_CFG    );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_ERROR_IRQS            );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM_ERROR_IRQS_MASK_CFG   );  

	CSI2A_PRINT_REGISTER(isp, CSI_RX_DPHY_LANE_CONTROL            );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_DPHY_LANE_CONFIG             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_DPHY_STATUS                  );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_DPHY_ERR_STATUS_IRQ          );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_DPHY_ERR_IRQ_MASK            );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_DPHY_TEST                    );  

	CSI2A_PRINT_REGISTER(isp, CSI_RX_INTEGRATION_DEBUG            );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_ERROR_DEBUG                  );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_TEST_GENERIC                 );  

	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_CTRL                 );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_STATUS               );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_DATA_CFG             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_CFG                  );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_MONITOR_CTRL         );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_MONITOR_FRAME        );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_MONITOR_LB           );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_TIMER                );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_FCC_CFG              );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_FCC_CTRL             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM0_FCC_FILL_LVL         );  

	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_CTRL                 );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_STATUS               );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_DATA_CFG             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_CFG                  );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_MONITOR_CTRL         );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_MONITOR_FRAME        );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_MONITOR_LB           );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_TIMER                );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_FCC_CFG              );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_FCC_CTRL             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM1_FCC_FILL_LVL         );  

	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_CTRL                 );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_STATUS               );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_DATA_CFG             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_CFG                  );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_CTRL         );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_FRAME        );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_LB           );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_TIMER                );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_CFG              );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_CTRL             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_FILL_LVL         );  

	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_CTRL                 );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_STATUS               );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_DATA_CFG             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_CFG                  );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_CTRL         );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_FRAME        );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_LB           );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_TIMER                );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_CFG              );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_CTRL             );  
	CSI2A_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_FILL_LVL         ); 
	//csi2b
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_DEVICE_CFG            );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_SFT_RST               );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_STATIC_CFG            );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_CFG_ADDR              );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_ERR_BYPASS_CFG        );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_MONITOR_FRAME         );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_MONITOR_IRQS          );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_MONITOR_IRQS_MASK_CFG );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_INFO_IRQS             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_INFO_IRQS_MASK_CFG    );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_ERROR_IRQS            );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM_ERROR_IRQS_MASK_CFG   );  
	
	CSI2B_PRINT_REGISTER(isp, CSI_RX_DPHY_LANE_CONTROL            );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_DPHY_LANE_CONFIG             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_DPHY_STATUS                  );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_DPHY_ERR_STATUS_IRQ          );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_DPHY_ERR_IRQ_MASK            );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_DPHY_TEST                    );  
	
	CSI2B_PRINT_REGISTER(isp, CSI_RX_INTEGRATION_DEBUG            );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_ERROR_DEBUG                  );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_TEST_GENERIC                 );  
	
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_CTRL                 );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_STATUS               );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_DATA_CFG             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_CFG                  );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_MONITOR_CTRL         );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_MONITOR_FRAME        );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_MONITOR_LB           );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_TIMER                );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_FCC_CFG              );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_FCC_CTRL             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM0_FCC_FILL_LVL         );  
	
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_CTRL                 );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_STATUS               );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_DATA_CFG             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_CFG                  );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_MONITOR_CTRL         );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_MONITOR_FRAME        );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_MONITOR_LB           );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_TIMER                );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_FCC_CFG              );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_FCC_CTRL             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM1_FCC_FILL_LVL         );  
	
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_CTRL                 );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_STATUS               );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_DATA_CFG             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_CFG                  );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_CTRL         );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_FRAME        );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_LB           );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_TIMER                );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_CFG              );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_CTRL             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_FILL_LVL         );  
	
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_CTRL                 );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_STATUS               );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_DATA_CFG             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_CFG                  );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_CTRL         );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_FRAME        );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_MONITOR_LB           );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_TIMER                );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_CFG              );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_CTRL             );  
	CSI2B_PRINT_REGISTER(isp, CSI_RX_STREAM2_FCC_FILL_LVL         );  
}
/* -----------------------------------------------------------------------------
 * Interrupt handling
 *-----------------------------------------------------------------------------*/

/*
 * k510isp_csi2_isr - CSI2 interrupt handling.
 */
void k510isp_csi2_isr(struct isp_csi2_device *csi2)
{
	struct k510isp_pipeline *pipe = to_isp_pipeline(&csi2->subdev.entity);
	u32 csi2_irqstatus;
	struct k510_isp_device *isp = to_isp_device(csi2);
    //no used
}    
/* -----------------------------------------------------------------------------
 * ISP video operations
 *-----------------------------------------------------------------------------*/
/*
 * csi2_queue - Queues the first buffer when using memory output
 * @video: The video node
 * @buffer: buffer to queue
 */
static int csi2_queue(struct k510isp_video *video, struct k510isp_buffer *buffer)
{
    //no used
	return 0;
}

static const struct k510isp_video_operations csi2_ispvideo_ops = {
	.queue = csi2_queue,
};
/* -----------------------------------------------------------------------------
 * V4L2 subdev operations
 *-----------------------------------------------------------------------------*/
/*
 * csi2_ioctl - csi2 module private ioctl's
 * @sd: ISP csi2 V4L2 subdevice
 * @cmd: ioctl command
 * @arg: ioctl argument
 *
 * Return 0 on success or a negative error code otherwise.
 */
static long csi2_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct isp_csi2_device *csi2 = v4l2_get_subdevdata(sd);
	dev_dbg(csi2->isp->dev,"%s:cmd(0x%x)\n",__func__,cmd);
	struct isp_csi2_info *csi2_info = &csi2->csi2_info;
	switch (cmd) {
	case VIDIOC_K510ISP_CSI2_CFG:
		//dev_dbg(csi2->isp->dev,"%s:cmd(0x%x)\n",__func__,VIDIOC_K510ISP_CSI2_CFG);	
		mutex_lock(&csi2->ioctl_lock);
		memcpy((void*)csi2_info,arg,sizeof(struct isp_csi2_info));
		csi2_configure(csi2->isp);
		mutex_unlock(&csi2->ioctl_lock);
		break;
	case VIDIOC_K510ISP_SYSCTL_RST_CSI:
		mutex_lock(&csi2->ioctl_lock);
		reset_control_reset(csi2->isp->reset[CSI0_RST]);
		reset_control_reset(csi2->isp->reset[CSI1_RST]);
		reset_control_reset(csi2->isp->reset[CSI2_RST]);
		reset_control_reset(csi2->isp->reset[CSI3_RST]);
		mutex_unlock(&csi2->ioctl_lock);
		break;
	default:
		dev_err(csi2->isp->dev,"%s:cmd(0x%x) err!\n",__func__,cmd);
		return -ENOIOCTLCMD;
	}

	return 0;
}

static const unsigned int csi2_input_fmts[] = {
	MEDIA_BUS_FMT_SRGGB8_1X8,
	MEDIA_BUS_FMT_SRGGB10_1X10,
	MEDIA_BUS_FMT_SRGGB12_1X12,
	MEDIA_BUS_FMT_SRGGB14_1X14,
	MEDIA_BUS_FMT_YUYV8_2X8,
};
static struct v4l2_mbus_framefmt *
__csi2_get_format(struct isp_csi2_device *csi2, struct v4l2_subdev_pad_config *cfg,
		  unsigned int pad, enum v4l2_subdev_format_whence which)
{
	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return v4l2_subdev_get_try_format(&csi2->subdev, cfg, pad);
	else
		return &csi2->formats[pad];
}

static void
csi2_try_format(struct isp_csi2_device *csi2, struct v4l2_subdev_pad_config *cfg,
		unsigned int pad, struct v4l2_mbus_framefmt *fmt,
		enum v4l2_subdev_format_whence which)
{
	u32 pixelcode;
	struct v4l2_mbus_framefmt *format;
	const struct isp_format_info *info;
	unsigned int i;
	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);
	switch (pad) {
	case CSI2_PAD_SINK0:
		/* Clamp the width and height to valid range (1-8191). */
		for (i = 0; i < ARRAY_SIZE(csi2_input_fmts); i++) {
			if (fmt->code == csi2_input_fmts[i])
				break;
		}

		/* If not found, use SGRBG10 as default */
		if (i >= ARRAY_SIZE(csi2_input_fmts))
			fmt->code = MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;

		fmt->width = clamp_t(u32, fmt->width, 1, 8191);
		fmt->height = clamp_t(u32, fmt->height, 1, 8191);
		break;
	case CSI2_PAD_SINK1:
		/* Clamp the width and height to valid range (1-8191). */
		for (i = 0; i < ARRAY_SIZE(csi2_input_fmts); i++) {
			if (fmt->code == csi2_input_fmts[i])
				break;
		}

		/* If not found, use SGRBG10 as default */
		if (i >= ARRAY_SIZE(csi2_input_fmts))
			fmt->code = MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;

		fmt->width = clamp_t(u32, fmt->width, 1, 8191);
		fmt->height = clamp_t(u32, fmt->height, 1, 8191);
		break;
	case CSI2_PAD_SOURCE0:
		/* Source format same as sink format, except for DPCM
		 * compression.
		 */
		pixelcode = fmt->code;
		format = __csi2_get_format(csi2, cfg, CSI2_PAD_SINK0, which);
		memcpy(fmt, format, sizeof(*fmt));
	case CSI2_PAD_SOURCE1:
		/* Source format same as sink format, except for DPCM
		 * compression.
		 */
		pixelcode = fmt->code;
		format = __csi2_get_format(csi2, cfg, CSI2_PAD_SINK1, which);
		memcpy(fmt, format, sizeof(*fmt));

		break;
	}

	/* RGB, non-interlaced */
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
	fmt->field = V4L2_FIELD_NONE;
	dev_dbg(csi2->isp->dev,"%s:end\n",__func__);
}
/*
 * csi2_enum_mbus_code - Handle pixel format enumeration
 * @sd     : pointer to v4l2 subdev structure
 * @cfg: V4L2 subdev pad configuration
 * @code   : pointer to v4l2_subdev_mbus_code_enum structure
 * return -EINVAL or zero on success
 */
static int csi2_enum_mbus_code(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_mbus_code_enum *code)
{
	struct isp_csi2_device *csi2 = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	const struct k510isp_format_info *info;

	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);
	if (code->pad == CSI2_PAD_SINK0) {
		if (code->index >= ARRAY_SIZE(csi2_input_fmts))
		{
			dev_err(csi2->isp->dev,"%s:code->index %d \n",__func__,code->index);
			return -EINVAL;
		}	

		code->code = csi2_input_fmts[code->index];
	} else {
		format = __csi2_get_format(csi2, cfg, CSI2_PAD_SINK0,
					   code->which);
		switch (code->index) {
		case 0:
			/* Passthrough sink pad code */
			code->code = format->code;
			break;
		case 1:
			/* Uncompressed code */
			info = k510isp_video_format_info(format->code);
			if (info->uncompressed == format->code)
			{
				dev_err(csi2->isp->dev,"%s:format->code %d \n",__func__,format->code);
				return -EINVAL;
			}

			code->code = info->uncompressed;
			break;
		default:		
			dev_err(csi2->isp->dev,"%s:code->index %d \n",__func__,code->index);
			return -EINVAL;
	
		}
	}

	dev_dbg(csi2->isp->dev,"%s:end\n",__func__);
	return 0;
}


static int csi2_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_size_enum *fse)
{
	struct isp_csi2_device *csi2 = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt format;
	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);
	if (fse->index != 0)
	{	
		dev_err(csi2->isp->dev,"%s:fse->index %d\n",__func__,fse->index);
		return -EINVAL;
	}

	format.code = fse->code;
	format.width = 1;
	format.height = 1;
	csi2_try_format(csi2, cfg, fse->pad, &format, fse->which);
	fse->min_width = format.width;
	fse->min_height = format.height;

	if (format.code != fse->code)
	{	
		dev_err(csi2->isp->dev,"%s:format.code %d,fse->code %d\n",__func__,format.code,fse->code);
		return -EINVAL;
	}

	format.code = fse->code;
	format.width = -1;
	format.height = -1;
	csi2_try_format(csi2, cfg, fse->pad, &format, fse->which);
	fse->max_width = format.width;
	fse->max_height = format.height;
	dev_dbg(csi2->isp->dev,"%s:end\n",__func__);
	return 0;
}
/*
 * csi2_get_format - Handle get format by pads subdev method
 * @sd : pointer to v4l2 subdev structure
 * @cfg: V4L2 subdev pad configuration
 * @fmt: pointer to v4l2 subdev format structure
 * return -EINVAL or zero on success
 */
static int csi2_get_format(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *fmt)
{
	struct isp_csi2_device *csi2 = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);
	format = __csi2_get_format(csi2, cfg, fmt->pad, fmt->which);
	if (format == NULL)
	{	
		dev_err(csi2->isp->dev,"%s:format NULL\n",__func__);
		return -EINVAL;
	}

	fmt->format = *format;
	dev_dbg(csi2->isp->dev,"%s:format->width(%d),format->heightend(%d)\n",__func__,format->width,format->height);
	return 0;
}
/*
 * csi2_set_format - Handle set format by pads subdev method
 * @sd : pointer to v4l2 subdev structure
 * @cfg: V4L2 subdev pad configuration
 * @fmt: pointer to v4l2 subdev format structure
 * return -EINVAL or zero on success
 */
static int csi2_set_format(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *fmt)
{
	struct isp_csi2_device *csi2 = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	dev_dbg(csi2->isp->dev,"%s:width(%d),height(%d)start\n",__func__,fmt->format.width,fmt->format.height);
#if 0
	format = __csi2_get_format(csi2, cfg, fmt->pad, fmt->which);
	if (format == NULL)
	{	
		dev_err(csi2->isp->dev,"%s:format NULL\n",__func__);
		return -EINVAL;
	}
	csi2_try_format(csi2, cfg, fmt->pad, &fmt->format, fmt->which);
	*format = fmt->format;
	dev_info(csi2->isp->dev,"%s:width(%d),height(%d)end\n",__func__,fmt->format.width,fmt->format.height);

	/* Propagate the format from sink to source */
	if (fmt->pad == CSI2_PAD_SINK0) {
		format = __csi2_get_format(csi2, cfg, CSI2_PAD_SOURCE0,
					   fmt->which);
		*format = fmt->format;
		csi2_try_format(csi2, cfg, CSI2_PAD_SOURCE0, format, fmt->which);
	}

	if (fmt->pad == CSI2_PAD_SINK1) {
		format = __csi2_get_format(csi2, cfg, CSI2_PAD_SOURCE1,
					   fmt->which);
		*format = fmt->format;
		csi2_try_format(csi2, cfg, CSI2_PAD_SOURCE1, format, fmt->which);
	}
#endif

	if(( fmt->pad != CSI2_PAD_SINK0)&&( fmt->pad != CSI2_PAD_SINK1) 
		&&( fmt->pad != CSI2_PAD_SOURCE0)&&( fmt->pad != CSI2_PAD_SOURCE1))
	{
		dev_warn(csi2->isp->dev, "%s:pad error!\n",__func__);
		return -EINVAL;
	}
	dev_dbg(csi2->isp->dev, "%s:pad(%d),width(%d),height(%d) before!\n",__func__,fmt->pad,csi2->formats[fmt->pad].width,csi2->formats[fmt->pad].height);
	format = &fmt->format;
	csi2->formats[fmt->pad].width = format->width;
	csi2->formats[fmt->pad].height = format->height;
	csi2->formats[fmt->pad].field = V4L2_FIELD_NONE;
	csi2->formats[fmt->pad].colorspace =V4L2_COLORSPACE_SRGB;
	csi2->formats[fmt->pad].code = format->code;//MEDIA_BUS_FMT_SRGGB10_1X10;
	dev_dbg(csi2->isp->dev, "%s:pad(%d),width(%d),height(%d) code(0x%x) end!\n",__func__,fmt->pad,csi2->formats[fmt->pad].width,csi2->formats[fmt->pad].height,csi2->formats[fmt->pad].code);
	return 0;
}
/*
 * csi2_init_formats - Initialize formats on all pads
 * @sd: ISP CSI2 V4L2 subdevice
 * @fh: V4L2 subdev file handle
 *
 * Initialize all pad formats with default values. If fh is not NULL, try
 * formats are initialized on the file handle. Otherwise active formats are
 * initialized on the device.
 */
static int csi2_init_formats(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct isp_csi2_device *csi2 = v4l2_get_subdevdata(sd);
	struct v4l2_subdev_format format;
	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);
	memset(&format, 0, sizeof(format));
	format.pad = CSI2_PAD_SINK0;
	format.which = fh ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	format.format.code = csi2->formats[CSI2_PAD_SINK0].code;//MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;
	format.format.width = csi2->formats[CSI2_PAD_SINK0].width;//4096
	format.format.height = csi2->formats[CSI2_PAD_SINK0].height;//1080;//4096
	csi2_set_format(sd, fh ? fh->pad : NULL, &format);

	format.pad = CSI2_PAD_SINK1;
	format.which = fh ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	format.format.code = csi2->formats[CSI2_PAD_SINK1].code;//MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;
	format.format.width = csi2->formats[CSI2_PAD_SINK1].width;//1920;//4096
	format.format.height = csi2->formats[CSI2_PAD_SINK1].height;//1080;//4096
	csi2_set_format(sd, fh ? fh->pad : NULL, &format);

	format.pad = CSI2_PAD_SOURCE0;
	format.which = fh ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	format.format.code = csi2->formats[CSI2_PAD_SOURCE0].code;//MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;
	format.format.width = csi2->formats[CSI2_PAD_SOURCE0].width;//1920;//4096
	format.format.height = csi2->formats[CSI2_PAD_SOURCE0].height;//1080;//4096
	csi2_set_format(sd, fh ? fh->pad : NULL, &format);

	format.pad = CSI2_PAD_SOURCE1;
	format.which = fh ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
	format.format.code = csi2->formats[CSI2_PAD_SOURCE1].code;//MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;
	format.format.width = csi2->formats[CSI2_PAD_SOURCE1].width;//1920;//4096
	format.format.height = csi2->formats[CSI2_PAD_SOURCE1].height;//1080;//4096
	csi2_set_format(sd, fh ? fh->pad : NULL, &format);

	dev_dbg(csi2->isp->dev,"%s:end\n",__func__);
	return 0;
}
/*
 * csi2_set_stream - Enable/Disable streaming on the CSI2 module
 * @sd: ISP CSI2 V4L2 subdevice
 * @enable: ISP pipeline stream state
 *
 * Return 0 on success or a negative error code otherwise.
 */
static int csi2_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct isp_csi2_device *csi2 = v4l2_get_subdevdata(sd);
	struct k510_isp_device *isp = csi2->isp;
	//struct k510isp_video *video_out = &csi2->video_out;
	struct isp_csi2_info *csi2_info = &isp->isp_csi2.csi2_info;
	dev_info(csi2->isp->dev,"%s:start\n",__func__);

	switch (enable) {
	case ISP_PIPELINE_STREAM_CONTINUOUS:
		if (k510isp_dphy_acquire(csi2->dphy, &sd->entity) < 0)
		{	
			dev_dbg(csi2->isp->dev,"%s:k510isp_dphy_acquire failed\n",__func__);
			return -ENODEV;
		}
		csi2_configure(isp);
		//csi2_print_status(csi2);
		/*
		 * When outputting to memory with no buffer available, let the
		 * buffer queue handler start the hardware. A DMA queue flag
		 * ISP_VIDEO_DMAQUEUE_QUEUED will be set as soon as there is
		 * a buffer available.
		 */
#if 0
		if (!(video_out->dmaqueue_flags & ISP_VIDEO_DMAQUEUE_QUEUED))
			break;
		/* Enable context 0 and IRQs */
		atomic_set(&csi2->stopping, 0);
		csi2_enable(isp,1);
		k510isp_video_dmaqueue_flags_clr(video_out);
#endif
		//csi2_print_status(csi2);
		break;
	case ISP_PIPELINE_STREAM_STOPPED:
		if (csi2->state == ISP_PIPELINE_STREAM_STOPPED)
		{
			dev_dbg(csi2->isp->dev,"%s:ISP_PIPELINE_STREAM_STOPPED\n",__func__);
			return 0;
		}	
		if (k510isp_module_sync_idle(&sd->entity, &csi2->wait,
					      &csi2->stopping))
			dev_dbg(isp->dev, "%s: module stop timeout.\n",
				sd->name);
		csi2_enable(isp,0);
		reset_control_reset(isp->reset[CSI0_RST]);
		reset_control_reset(isp->reset[CSI1_RST]);
		reset_control_reset(isp->reset[CSI2_RST]);
		reset_control_reset(isp->reset[CSI3_RST]);
		k510isp_dphy_release(csi2->dphy);
	//	k510isp_video_dmaqueue_flags_clr(video_out);
		break;
	}

	csi2->state = enable;
	dev_dbg(csi2->isp->dev,"%s:end\n",__func__);
	return 0;
}
/* V4L2 subdev core operations */
static const struct v4l2_subdev_core_ops csi2_v4l2_core_ops = {
	.ioctl = csi2_ioctl,
};
/* subdev video operations */
static const struct v4l2_subdev_video_ops csi2_video_ops = {
	.s_stream = csi2_set_stream,
};
/* subdev pad operations */
static const struct v4l2_subdev_pad_ops csi2_pad_ops = {
	.enum_mbus_code = csi2_enum_mbus_code,
	.enum_frame_size = csi2_enum_frame_size,
	.get_fmt = csi2_get_format,
	.set_fmt = csi2_set_format,
};
/* subdev operations */
static const struct v4l2_subdev_ops csi2_ops = {
	.core = &csi2_v4l2_core_ops,
	.video = &csi2_video_ops,
	.pad = &csi2_pad_ops,
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops csi2_internal_ops = {
	.open = csi2_init_formats,
};
/* -----------------------------------------------------------------------------
 * Media entity operations
 *-----------------------------------------------------------------------------/
/*
 * csi2_link_setup - Setup CSI2 connections.
 * @entity : Pointer to media entity structure
 * @local  : Pointer to local pad array
 * @remote : Pointer to remote pad array
 * @flags  : Link flags
 * return -EINVAL or zero on success
 */
static int csi2_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
	struct isp_csi2_device *csi2 = v4l2_get_subdevdata(sd);
	//struct isp_csi2_ctrl_cfg *ctrl = &csi2->ctrl;
	unsigned int index = local->index;

	dev_dbg(csi2->isp->dev,"%s:index (%d) flags(0x%x) csi2->output(0x%x)start\n",__func__,index,flags,csi2->output);
	/*
	 * The ISP core doesn't support pipelines with multiple video outputs.
	 * Revisit this when it will be implemented, and return -EBUSY for now.
	 */

	/* FIXME: this is actually a hack! */
	if (is_media_entity_v4l2_subdev(remote->entity))
		index |= 2 << 16;

	switch (index) {
	case CSI2_PAD_SOURCE0:
	case CSI2_PAD_SOURCE0 | 2 << 16:
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (csi2->output & CSI2_OUTPUT_VI0)
			{
				dev_err(csi2->isp->dev,"%s:csi2->output (0x%x)ret(%d)\n",__func__,csi2->output,-EBUSY);
				return -EBUSY;
			}	
			csi2->output |= CSI2_OUTPUT_VI0;
		} else {
			csi2->output &= ~CSI2_OUTPUT_VI0;
		}
		break;
	case CSI2_PAD_SOURCE1:
	case CSI2_PAD_SOURCE1 | 2 << 16:
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (csi2->output & CSI2_OUTPUT_VI1)
			{
				dev_err(csi2->isp->dev,"%s:csi2->output (0x%x)ret(%d)\n",__func__,csi2->output,-EBUSY);
				return -EBUSY;
			}	
			csi2->output |= CSI2_OUTPUT_VI1;
		} else {
			csi2->output &= ~CSI2_OUTPUT_VI1;
		}
		break;
	default:
		/* Link from camera to CSI2 is fixed... */
		dev_err(csi2->isp->dev,"%s:end\n",__func__);
		return -EINVAL;
	}
	dev_dbg(csi2->isp->dev,"%s:index (0x%x)\n",__func__,index);
	return 0;
}
/* media operations */
static const struct media_entity_operations csi2_media_ops = {
	.link_setup = csi2_link_setup,
	.link_validate = v4l2_subdev_link_validate,
};

void k510isp_csi2_unregister_entities(struct isp_csi2_device *csi2)
{
	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);
	v4l2_device_unregister_subdev(&csi2->subdev);
//	k510isp_video_unregister(&csi2->video_out);
}

int k510isp_csi2_register_entities(struct isp_csi2_device *csi2,
				    struct v4l2_device *vdev)
{
	int ret;
	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);
	/* Register the subdev and video nodes. */
	ret = v4l2_device_register_subdev(vdev, &csi2->subdev);
	if (ret < 0)
	{	
		dev_err(csi2->isp->dev,"%s:v4l2_device_register_subdev failed\n",__func__);
		goto error;
	}
#if 0
	ret = k510isp_video_register(&csi2->video_out, vdev);
	if (ret < 0)
	{	
		dev_err(csi2->isp->dev,"%s:k510isp_video_register failed\n",__func__);
		goto error;
	}
#endif
	return 0;

error:
	//k510isp_csi2_unregister_entities(csi2);
	dev_dbg(csi2->isp->dev,"%s:end\n",__func__);
	return ret;
}
/* -----------------------------------------------------------------------------
 * ISP CSI2 initialisation and cleanup
 *-----------------------------------------------------------------------------*/

/*
 * csi2_init_entities - Initialize subdev and media entity.
 * @csi2: Pointer to csi2 structure.
 * return -ENOMEM or zero on success
 */
static int csi2_init_entities(struct isp_csi2_device *csi2)
{
	struct v4l2_subdev *sd = &csi2->subdev;
	struct media_pad *pads = csi2->pads;
	struct media_entity *me = &sd->entity;
	int ret = 0;

	dev_dbg(csi2->isp->dev,"%s:start\n",__func__);

	v4l2_subdev_init(sd, &csi2_ops);
	sd->internal_ops = &csi2_internal_ops;
	strlcpy(sd->name, "K510 ISP CSI2", sizeof(sd->name));

	sd->grp_id = 1 << 16;	/* group ID for isp subdevs */
	v4l2_set_subdevdata(sd, csi2);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	pads[CSI2_PAD_SOURCE0].flags = MEDIA_PAD_FL_SOURCE| MEDIA_PAD_FL_MUST_CONNECT;
	pads[CSI2_PAD_SOURCE1].flags = MEDIA_PAD_FL_SOURCE| MEDIA_PAD_FL_MUST_CONNECT;
	pads[CSI2_PAD_SINK0].flags = MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	pads[CSI2_PAD_SINK1].flags = MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;

	me->function = MEDIA_ENT_F_VID_IF_BRIDGE;//MEDIA_ENT_F_V4L2_SUBDEV_UNKNOWN;//MEDIA_ENT_F_VID_IF_BRIDGE;//MEDIA_ENT_F_IO_V4L;
	me->ops = &csi2_media_ops;
	ret = media_entity_pads_init(me, CSI2_PADS_NUM, pads);
	if (ret < 0)
	{
		dev_err(csi2->isp->dev,"%s:media_entity_pads_init failed (%d)\n",__func__,ret);
		return ret;
	}
	csi2_init_formats(sd, NULL);
#if 0
	/* Video device node */
	csi2->video_out.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	csi2->video_out.ops = &csi2_ispvideo_ops;
	csi2->video_out.isp = to_isp_device(csi2);//csi2->isp;
	csi2->video_out.capture_mem = PAGE_ALIGN(2048 * 2048) * 3;
	csi2->video_out.bpl_alignment = 16;//32;
	csi2->video_out.bpl_zero_padding = 1;
	csi2->video_out.bpl_max = 0x1fff0;//0x1ffe0;

	ret = k510isp_video_init(&csi2->video_out, "CSI2");
	if (ret < 0)
	{
		dev_err(csi2->isp->dev,"%s:k510isp_video_init failed(%d)\n",__func__,ret);
		goto error_video;
	}
#endif
	dev_dbg(csi2->isp->dev,"%s:end\n",__func__);
	return 0;

#if 0
error_video:
	media_entity_cleanup(&csi2->subdev.entity);
	dev_err(csi2->isp->dev,"%s:error_video (%d)\n",__func__,ret);
	return ret;
#endif
}
/*
 * k510isp_csi2_init - Routine for module driver init
 */
int k510isp_csi2_init(struct k510_isp_device *isp)
{
	struct isp_csi2_device *csi2a = &isp->isp_csi2;
	int ret = 0;
	dev_info(isp->dev,"%s:start\n",__func__);

	csi2a->isp = isp;
	csi2a->available = 1;
	csi2a->regs0 = CSI_HOST_BASE;
	csi2a->regs1 = CSI1_HOST_BASE;
	csi2a->dphy = &isp->isp_dphy;
	csi2a->state = ISP_PIPELINE_STREAM_STOPPED;
	csi2a->output = CSI2_OUTPUT_NONE;
	init_waitqueue_head(&csi2a->wait);

	csi2a->formats[CSI2_PAD_SINK0].code=MEDIA_BUS_FMT_SRGGB10_1X10;
	csi2a->formats[CSI2_PAD_SINK0].width = 1920;
	csi2a->formats[CSI2_PAD_SINK0].height = 1080;
	//
	csi2a->formats[CSI2_PAD_SINK1].code=MEDIA_BUS_FMT_SRGGB10_1X10;
	csi2a->formats[CSI2_PAD_SINK1].width = 1920;
	csi2a->formats[CSI2_PAD_SINK1].height = 1080;
	//
	csi2a->formats[CSI2_PAD_SOURCE0].code=MEDIA_BUS_FMT_SRGGB10_1X10;
	csi2a->formats[CSI2_PAD_SOURCE0].width = 1920;
	csi2a->formats[CSI2_PAD_SOURCE0].height = 1080;
	//
	csi2a->formats[CSI2_PAD_SOURCE1].code=MEDIA_BUS_FMT_SRGGB10_1X10;
	csi2a->formats[CSI2_PAD_SOURCE1].width = 1920;
	csi2a->formats[CSI2_PAD_SOURCE1].height = 1080;	
	ret = csi2_init_entities(csi2a);
	dev_dbg(isp->dev,"%s:end\n",__func__);
	return ret;
}
/*
 * k510isp_csi2_cleanup - Routine for module driver cleanup
 */
void k510isp_csi2_cleanup(struct k510_isp_device *isp)
{
	struct isp_csi2_device *csi2a = &isp->isp_csi2;
	dev_info(isp->dev,"%s:start\n",__func__);
//	k510isp_video_cleanup(&csi2a->video_out);
	media_entity_cleanup(&csi2a->subdev.entity);
}