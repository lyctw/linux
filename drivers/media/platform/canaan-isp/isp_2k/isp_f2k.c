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
#include "../k510video_subsystem.h"
#include "isp.h"
#include "wrap/isp_wrap_drv.h"
#include "core/isp_core_drv.h"
#include "ds/isp_ds_drv.h"
#include "fbc/isp_3dnr_fbc_drv.h"
#include "fbd/isp_3dnr_fbd_drv.h"
#include "remap/isp_remap_drv.h"
#include "table/isp_table_drv.h"
/****************************************************************************
*wrap
*****************************************************************************/
//
void isp_f2k_wrap_SetComRst(struct k510_isp_device *isp)
{    
    ISP_WRAP_RST_CTL_S stRstCtl;
	stRstCtl.sw_3dnr_rst = 1;    
	stRstCtl.wdr_2_frame_rst = 1;
	stRstCtl.wdr_3_frame_rst =1;
	stRstCtl.ldc_rst = 1; 
	stRstCtl.main_out_rst = 1;   
	stRstCtl.ds0_out_rst = 1;    
	stRstCtl.ds1_out_rst = 1;   
	stRstCtl.ds2_out_rst = 1; 
	stRstCtl.wrap_com_rst = 1;   
	stRstCtl.wrap_cfg_rst = 1;   
	stRstCtl.core_com_rst = 1;   
	stRstCtl.core_cfg_rst = 1; 
	stRstCtl.axi_wr_ch_rst = 1;  
	stRstCtl.axi_rd_ch_rst = 1;     
    Isp_Drv_F2k_Wrap_SetPipeReset(isp,&stRstCtl);
}
//
void isp_f2k_wrap_SetDmaRst(struct k510_isp_device *isp)
{
    
    ISP_WRAP_DMA_RST_CTL_S stDmaRstCtl;
    stDmaRstCtl.y_3dnr_wr_ch_rst = 1;     
    stDmaRstCtl.uv_3dnr_wr_ch_rst = 1;    
    stDmaRstCtl.ldc_y_wr_ch_rst = 1;      
    stDmaRstCtl.ldc_uv_wr_ch_rst = 1;     
    stDmaRstCtl.wdr_raw_wr_ch_rst = 1;    
    stDmaRstCtl.main_out_y_wr_ch_rst = 1; 
    stDmaRstCtl.main_out_uv_wr_ch_rst = 1;
    stDmaRstCtl.y_3dnr_rd_ch_rst = 1;     
    stDmaRstCtl.uv_3dnr_rd_ch_rst = 1;    
    stDmaRstCtl.ldc_y_rd_ch_rst = 1;      
    stDmaRstCtl.ldc_uv_rd_ch_rst = 1;     
    stDmaRstCtl.wdr_raw_rd_ch_rst = 1;    
    stDmaRstCtl.ds0_out_y_wr_ch_rst = 1;  
    stDmaRstCtl.ds0_out_uv_wr_ch_rst = 1; 
    stDmaRstCtl.ds1_out_y_wr_ch_rst = 1;  
    stDmaRstCtl.ds1_out_uv_wr_ch_rst = 1; 
    stDmaRstCtl.ds2_out_r_wr_ch_rst = 1;  
    stDmaRstCtl.ds2_out_g_wr_ch_rst = 1;  
    stDmaRstCtl.ds2_out_b_wr_ch_rst = 1;  
    stDmaRstCtl.ds0_osd0_rd_ch_rst = 1;   
    stDmaRstCtl.ds0_osd1_rd_ch_rst = 1;   
    stDmaRstCtl.ds1_osd0_rd_ch_rst = 1;   
    stDmaRstCtl.ds1_osd1_rd_ch_rst = 1;   
    stDmaRstCtl.ds2_osd0_rd_ch_rst = 1;   
    stDmaRstCtl.ds2_osd1_rd_ch_rst = 1;   
    stDmaRstCtl.ds1_osd2_rd_ch_rst = 1;   
    stDmaRstCtl.ds2_osd2_rd_ch_rst = 1;   
    Isp_Drv_F2k_Wrap_SetDmaReset(isp,&stDmaRstCtl);
}
//
void isp_f2k_wrap_SetPipeClkCtl(struct k510_isp_device *isp)
{
    
    ISP_WRAP_PIPE_CLK_CTL_S pstPipeClkCtl;
    pstPipeClkCtl.wrap_com_clk_en = 1;
    pstPipeClkCtl.wrap_cfg_clk_en = 1;
    pstPipeClkCtl.core_com_clk_en = 1;
    pstPipeClkCtl.core_cfg_clk_en = 1;
    pstPipeClkCtl.axi_wr_ch_en = 1;   
    pstPipeClkCtl.axi_rd_ch_en = 1;    
	Isp_Drv_F2k_Wrap_SetComClkCtl(isp,&pstPipeClkCtl);
}
//
void isp_f2k_wrap_SetWdr(struct k510_isp_device *isp,struct isp_wrap_wdr_info *wdrInfo)
{
    ISP_WRAP_WDR_ATTR_S pstWdrAttr;
    ISP_WRAP_WDR_CLK_CTL_S     *stWdrClkCtl = &pstWdrAttr.stWdrClkCtl;
	ISP_WRAP_WDR_DMA_EN_CTL_S  *stWdrDmaEn = &pstWdrAttr.stWdrDmaEn;
    ISP_WRAP_WDR_MODE_CTL_S    *stWdrModeCtl = &pstWdrAttr.stWdrModeCtl;
	if( ISP_PIPE_WDR_2_FRAME == wdrInfo->wdr_mode)
	{
		stWdrClkCtl->wdr_2_frame_clk_en = 1;
		stWdrClkCtl->wdr_3_frame_clk_en = 0;
		stWdrDmaEn->wdr_short_dma_en = 1;//1
		stWdrDmaEn->wdr_long_dma_en  = 0;
		stWdrModeCtl->wdr_dynamic_switch_en = 0;
		stWdrModeCtl->wdr_long_l2_buf_depth = 0;
		stWdrModeCtl->wdr_long_ch_mode = 0;
		stWdrModeCtl->wdr_long_l2_buf_en = 0;
		stWdrModeCtl->wdr_short_s1_buf_en = 0;//1
	}
	else if(ISP_PIPE_WDR_3_FRAME == wdrInfo->wdr_mode)
	{
		stWdrClkCtl->wdr_2_frame_clk_en = 1;//0;
		stWdrClkCtl->wdr_3_frame_clk_en = 1;//0;//1;
		stWdrDmaEn->wdr_short_dma_en = 1; //1?
		stWdrDmaEn->wdr_long_dma_en  = 1;
		stWdrModeCtl->wdr_dynamic_switch_en = 1;//0;
		stWdrModeCtl->wdr_long_l2_buf_depth = 20;
		stWdrModeCtl->wdr_long_ch_mode = 0;
		stWdrModeCtl->wdr_long_l2_buf_en = 0; //1?
		stWdrModeCtl->wdr_short_s1_buf_en = 0;//1
	}
	else
	{
		stWdrClkCtl->wdr_2_frame_clk_en = 0;
		stWdrClkCtl->wdr_3_frame_clk_en = 0;
		stWdrDmaEn->wdr_short_dma_en = 0;
		stWdrDmaEn->wdr_long_dma_en  = 0;
		stWdrModeCtl->wdr_dynamic_switch_en = 0;
		stWdrModeCtl->wdr_long_l2_buf_depth = 0;
		stWdrModeCtl->wdr_long_ch_mode = 0;
		stWdrModeCtl->wdr_long_l2_buf_en = 0;
		stWdrModeCtl->wdr_short_s1_buf_en = 0;				
	}
    ISP_WRAP_WDR_PIXEL_FORMAT_CTL_S *stWdrPixelFormat = &pstWdrAttr.stWdrPixelFormat;
	stWdrPixelFormat->wdr_long_img_format = ISP_RGBRAW_DATA;
	stWdrPixelFormat->wdr_long_img_out_format = IN_YUV422;
	stWdrPixelFormat->wdr_long_pixel_width = PIXEL_12;
	stWdrPixelFormat->wdr_long_yuv422_pxl_order = YUYV;
	stWdrPixelFormat->wdr_long_yuv_in_format =IN_YUV422;
    ISP_WRAP_WDR_BUF_S              *stWdrBuf = &pstWdrAttr.stWdrBuf;
	stWdrBuf->wdr_buf_base = 0x0;
	stWdrBuf->wdr_buf_size = 0x0;
	stWdrBuf->wdr_line_stride = 0x0;
    Isp_Drv_F2k_Wrap_SetWdr(isp,&pstWdrAttr); 
}
//
void isp_f2k_wrap_Set3dnr(struct k510_isp_device *isp,struct isp_wrap_3dnr_info *nr3dInfo)
{
    ISP_WRAP_3DNR_ATTR_S pst3dnrAttr;
	if( 1 == nr3dInfo->nr3d_en) 
	{
		pst3dnrAttr.clk_3dnr_en = 1;
		pst3dnrAttr.pipe_3dnr_dma_en = 1;
		pst3dnrAttr.nr3d_fbcd_en = 0;
		pst3dnrAttr.nr3d_mv_out_en = 0;
		if( 1 == nr3dInfo->nr3d_fbcd_en)
		{
			pst3dnrAttr.nr3d_fbcd_en = 1;
		}
		if( 1 == nr3dInfo->nr3d_mv_out_en)
		{
			pst3dnrAttr.nr3d_mv_out_en = 1;
		}
	}
	else
	{
		pst3dnrAttr.clk_3dnr_en = 0;
		pst3dnrAttr.pipe_3dnr_dma_en = 0;
		pst3dnrAttr.nr3d_fbcd_en = 0;
		pst3dnrAttr.nr3d_mv_out_en = 0;
	}
    ISP_WRAP_3DNR_PIXEL_FORMAT_CTL_S *st3DnrPixelFormat = &pst3dnrAttr.st3DnrPixelFormat;
	st3DnrPixelFormat->y_3dnr_img_format = nr3dInfo->nr3d_y_img_format;//ISP_RGBRAW_DATA;
	st3DnrPixelFormat->y_3dnr_img_out_format = nr3dInfo->nr3d_y_img_out_format;//IN_YUV422;
	st3DnrPixelFormat->y_3dnr_pixel_width = nr3dInfo->nr3d_y_pixel_width;//PIXEL_12;
	st3DnrPixelFormat->y_3dnr_yuv422_pxl_order = nr3dInfo->nr3d_y_yuv422_pxl_order;//YUYV;
	st3DnrPixelFormat->y_3dnr_yuv_in_format = nr3dInfo->nr3d_y_yuv_in_format;//IN_YUV422;
	st3DnrPixelFormat->uv_3dnr_img_format = nr3dInfo->nr3d_uv_img_format;//ISP_RGBRAW_DATA;
	st3DnrPixelFormat->uv_3dnr_img_out_format = nr3dInfo->nr3d_uv_mig_out_format;//IN_YUV422;
	st3DnrPixelFormat->uv_3dnr_pixel_width = nr3dInfo->nr3d_uv_yuv422_pxl_order;//PIXEL_8;
	st3DnrPixelFormat->uv_3dnr_yuv422_pxl_order = nr3dInfo->nr3d_uv_yuv422_pxl_order;//YUYV;
	st3DnrPixelFormat->uv_3dnr_yuv_in_format = nr3dInfo->nr3d_uv_yuv_in_format;//IN_YUV422;
    ISP_WRAP_3DNR_BUF_S   *st3dnrBuf = &pst3dnrAttr.st3dnrBuf;
    st3dnrBuf->y_3dnr_buf_base = nr3dInfo->nr3d_y_buf_base;//ISP_BUF_3DNR_Y;
    st3dnrBuf->uv_3dnr_buf_base = nr3dInfo->nr3d_uv_buf_base;//ISP_BUF_3DNR_UV;
    st3dnrBuf->y_3dnr_line_stride = nr3dInfo->nr3d_y_line_stride;//ISP_BUF_3DNR_Y_STRIDE;
    st3dnrBuf->uv_3dnr_line_stride = nr3dInfo->nr3d_uv_line_stride;//ISP_BUF_3DNR_UV_STRIDE;
    Isp_Drv_F2k_Wrap_Set3Dnr(isp,&pst3dnrAttr);
}
//
void isp_f2k_wrap_SetLdc(struct k510_isp_device *isp,struct isp_wrap_ldc_info *ldcInfo)
{
    ISP_WRAP_LDC_ATTR_S pstLdcAttr;
	if( 1 == ldcInfo->ldc_en)
	{
		pstLdcAttr.ldc_clk_en = 1;
		pstLdcAttr.ldc_dma_en = 1;
	}
	else
	{
		pstLdcAttr.ldc_clk_en = 0;
		pstLdcAttr.ldc_dma_en = 0;
	}	
    ISP_WRAP_LDC_BUF_S              *stLdcBuf = &pstLdcAttr.stLdcBuf;
    stLdcBuf->ldc_y_buf_base = ldcInfo->ldc_y_buf_base;//ISP_BUF_LDC_Y;
    stLdcBuf->ldc_uv_buf_base = ldcInfo->ldc_uv_buf_base;//ISP_BUF_LDC_UV;
    stLdcBuf->ldc_y_line_stride = ldcInfo->ldc_line_stride;//ISP_BUF_LDC_Y_STRIDE;
    stLdcBuf->ldc_uv_line_stride = ldcInfo->ldc_line_stride;//ISP_BUF_LDC_UV_STRIDE;
    Isp_Drv_F2k_Wrap_SetLdc(isp,&pstLdcAttr);
}
//
void isp_f2k_wrap_SetMainOut(struct k510_isp_device *isp,struct isp_wrap_main_info *mainInfo)
{
    ISP_WRAP_MAINOUT_ATTR_S pstMainOutAttr;
    pstMainOutAttr.main_out_clk_en = 1;
	if( 1 == mainInfo->main_out_en)
	{
		pstMainOutAttr.main_out_dma_en = 1;
		pstMainOutAttr.pix_remap_main_en = mainInfo->main_pix_remap_en;
	}
	else
	{
		//pstMainOutAttr.main_out_clk_en = 0;
		pstMainOutAttr.main_out_dma_en = 0;
        pstMainOutAttr.pix_remap_main_en = 0;
	}
    ISP_WRAP_MAIN_PIXEL_FORMAT_CTL_S *stMainPixelFormat = &pstMainOutAttr.stMainPixelFormat;
	stMainPixelFormat->main_out_img_format = mainInfo->main_out_img_format;//ISP_YUV_DATA;
	stMainPixelFormat->main_out_img_out_format = mainInfo->main_out_img_out_format;//IN_YUV420;
	stMainPixelFormat->main_out_pixel_width = mainInfo->main_out_pxl_width;//PIXEL_8;
	stMainPixelFormat->main_out_yuv422_pxl_order = mainInfo->main_out_yuv422_pxl_order;//YUYV;
	stMainPixelFormat->main_out_yuv_in_format = mainInfo->main_out_yuv_in_format;//IN_YUV422;
    ISP_WRAP_MAIN_OUT_SIZE_S        *stMainOutSize = &pstMainOutAttr.stMainOutSize;
	stMainOutSize->main_out_h_size = mainInfo->main_size.Width - 1;
	stMainOutSize->main_out_v_size = mainInfo->main_size.Height - 1;
    ISP_WRAP_MAIN_BUF_S             *stMainBuf = &pstMainOutAttr.stMainBuf;
    stMainBuf->main_y_buf0_base = mainInfo->main_y_buf0_base;//ISP_BUF_MAIN_Y;
    stMainBuf->main_y_buf1_base = mainInfo->main_y_buf1_base;//ISP_BUF_MAIN_Y;
    stMainBuf->main_uv_buf0_base = mainInfo->main_uv_buf0_base;//ISP_BUF_MAIN_UV;
    stMainBuf->main_uv_buf1_base = mainInfo->main_uv_buf1_base;//ISP_BUF_MAIN_UV;
    stMainBuf->main_y_line_stride = mainInfo->main_line_stride;//SP_BUF_MAIN_Y_STRIDE;
    stMainBuf->main_uv_line_stride = mainInfo->main_line_stride;//ISP_BUF_MAIN_UV_STRIDE;
    Isp_Drv_F2k_Wrap_SetMainOut(isp,&pstMainOutAttr);
}
//
void isp_f2k_wrap_SetDs0Out(struct k510_isp_device *isp,struct isp_wrap_ds0_info *ds0Info)
{
	unsigned int Width = ds0Info->ds0_size.Width;
	unsigned int Height = ds0Info->ds0_size.Height;
    ISP_WRAP_DS0OUT_ATTR_S pstDs0OutAttr;
    ISP_WRAP_DS0_CLK_CTL_S          *stDs0ClkCtl = &pstDs0OutAttr.stDs0ClkCtl;
    ISP_WRAP_DS0_DMA_EN_CTL_S       *stDs0DmaEn = &pstDs0OutAttr.stDs0DmaEn;

	stDs0ClkCtl->ds0_out_clk_en = 1;
	stDs0ClkCtl->ds0_out_y_ch_clk_en = 1;
	stDs0ClkCtl->ds0_out_uv_ch_clk_en = 1;

	if(1 == ds0Info->ds0_out_en)
	{
		stDs0ClkCtl->ds_out0_osd0_ch_clk_en = 0;
		stDs0ClkCtl->ds_out0_osd1_ch_clk_en = 0;
		stDs0ClkCtl->ds_out0_osd2_ch_clk_en = 0;
		stDs0DmaEn->ds_out0_dma_en = 1;
		stDs0DmaEn->ds_out0_y_ch_dma_en = 1;
		stDs0DmaEn->ds_out0_uv_ch_dma_en = 1;
		stDs0DmaEn->ds0_osd0_ch_dma_en =0;
		stDs0DmaEn->ds0_osd1_ch_dma_en = 0;
		stDs0DmaEn->ds0_osd2_ch_dma_en = 0;
		if( 1 == ds0Info->ds0_osd0_en)
		{
			stDs0ClkCtl->ds_out0_osd0_ch_clk_en = 1;
			stDs0DmaEn->ds0_osd0_ch_dma_en = 1;
		}
		if( 1 == ds0Info->ds0_osd1_en)
		{
			stDs0ClkCtl->ds_out0_osd1_ch_clk_en = 1;
			stDs0DmaEn->ds0_osd1_ch_dma_en = 1;
		}
		if( 1 == ds0Info->ds0_osd2_en)
		{
			stDs0ClkCtl->ds_out0_osd2_ch_clk_en = 1;
			stDs0DmaEn->ds0_osd2_ch_dma_en = 1;
		}
	}
	else
	{
		//stDs0ClkCtl->ds0_out_clk_en = 0;
		//stDs0ClkCtl->ds0_out_y_ch_clk_en = 0;
		//stDs0ClkCtl->ds0_out_uv_ch_clk_en = 0;
		stDs0ClkCtl->ds_out0_osd0_ch_clk_en = 0;
		stDs0ClkCtl->ds_out0_osd1_ch_clk_en = 0;
		stDs0ClkCtl->ds_out0_osd2_ch_clk_en = 0;
		stDs0DmaEn->ds_out0_dma_en = 0;	
		stDs0DmaEn->ds_out0_y_ch_dma_en =0;
		stDs0DmaEn->ds_out0_uv_ch_dma_en = 0;	
		stDs0DmaEn->ds0_osd0_ch_dma_en =0;
		stDs0DmaEn->ds0_osd1_ch_dma_en = 0;
		stDs0DmaEn->ds0_osd2_ch_dma_en = 0;
	}
    pstDs0OutAttr.pix_remap_out0_en = ds0Info->pix_remap_out0_en;
    ISP_WRAP_DS0_PIXEL_FORMAT_CTL_S *stDs0PixelFormat = &pstDs0OutAttr.stDs0PixelFormat;
	stDs0PixelFormat->ds0_out_img_format = ds0Info->ds0_out_img_format;//ISP_YUV_DATA;
	stDs0PixelFormat->ds0_out_img_out_format = ds0Info->ds0_out_img_out_format;//IN_YUV420;
	stDs0PixelFormat->ds0_out_pixel_width = ds0Info->ds0_out_pxl_width;//PIXEL_8;
	stDs0PixelFormat->ds0_out_yuv422_pxl_order = ds0Info->ds0_out_yuv422_pxl_order;//YUYV;
	stDs0PixelFormat->ds0_out_yuv_in_format = ds0Info->ds0_out_yuv_in_format;//IN_YUV422;
    ISP_WRAP_DS0_OUT_SIZE_S         *stDs0OutSize = &pstDs0OutAttr.stDs0OutSize;
	stDs0OutSize->ds0_out_h_size = Width - 1;
	stDs0OutSize->ds0_out_v_size = Height - 1;
    ISP_WRAP_DS0_BUF_S              *stDs0Buf = &pstDs0OutAttr.stDs0Buf;
    stDs0Buf->ds0_y_buf0_base = ds0Info->ds0_y_buf0_base;//ISP_BUF_DS0_Y;
    stDs0Buf->ds0_y_buf1_base = ds0Info->ds0_y_buf1_base;//ISP_BUF_DS0_Y;
    stDs0Buf->ds0_y_line_stride = ds0Info->ds0_line_stride;//(Width + 15)/16*16;//ISP_BUF_DS0_Y_STRIDE;
    stDs0Buf->ds0_uv_line_stride = ds0Info->ds0_line_stride;//(Width + 15)/16*16;//ISP_BUF_DS0_UV_STRIDE;	
    stDs0Buf->ds0_uv_buf0_base = ds0Info->ds0_uv_buf0_base;//ISP_BUF_DS0_Y + stDs0Buf->ds0_y_line_stride * Height;//ISP_BUF_DS0_UV;
    stDs0Buf->ds0_uv_buf1_base = ds0Info->ds0_uv_buf1_base;//ISP_BUF_DS0_Y + stDs0Buf->ds0_y_line_stride * Height;//ISP_BUF_DS0_UV;
    Isp_Drv_F2k_Wrap_SetDs0Out(isp,&pstDs0OutAttr);
}
//
void isp_f2k_wrap_SetDs1Out(struct k510_isp_device *isp,struct isp_wrap_ds1_info *ds1Info)
{
    ISP_WRAP_DS1OUT_ATTR_S pstDs1OutAttr;
	unsigned int Width = ds1Info->ds1_size.Width;
	unsigned int Height = ds1Info->ds1_size.Height;
    ISP_WRAP_DS1_CLK_CTL_S          *stDs1ClkCtl = &pstDs1OutAttr.stDs1ClkCtl;
    ISP_WRAP_DS1_DMA_EN_CTL_S       *stDs1DmaEn = &pstDs1OutAttr.stDs1DmaEn;

	stDs1ClkCtl->ds1_out_clk_en  = 1;
	stDs1ClkCtl->ds1_out_y_ch_clk_en = 1;
	stDs1ClkCtl->ds1_out_uv_ch_clk_en = 1;
    
	if( 1 == ds1Info->ds1_out_en)	{

		stDs1ClkCtl->ds_out1_osd0_ch_clk_en = 0;
		stDs1ClkCtl->ds_out1_osd1_ch_clk_en = 0;
		stDs1ClkCtl->ds_out1_osd2_ch_clk_en = 0;
		stDs1DmaEn->ds_out1_dma_en = 1;
		stDs1DmaEn->ds_out1_y_ch_dma_en = 1;
		stDs1DmaEn->ds_out1_uv_ch_dma_en = 1;
		stDs1DmaEn->ds1_osd0_ch_dma_en = 0;
		stDs1DmaEn->ds1_osd1_ch_dma_en = 0;
		stDs1DmaEn->ds1_osd2_ch_dma_en = 0;
		if( 1 == ds1Info->ds1_osd0_en)
		{
			stDs1ClkCtl->ds_out1_osd0_ch_clk_en = 0;
			stDs1DmaEn->ds1_osd0_ch_dma_en = 1;
		}
		if( 1 == ds1Info->ds1_osd1_en)
		{
			stDs1ClkCtl->ds_out1_osd1_ch_clk_en = 0;
			stDs1DmaEn->ds1_osd1_ch_dma_en = 1;
		}
		if( 1 == ds1Info->ds1_osd2_en)
		{
			stDs1ClkCtl->ds_out1_osd2_ch_clk_en = 0;
			stDs1DmaEn->ds1_osd2_ch_dma_en = 1;
		}
	}
	else
	{
		//stDs1ClkCtl->ds1_out_clk_en  = 0;
		//stDs1ClkCtl->ds1_out_y_ch_clk_en = 0;
		//stDs1ClkCtl->ds1_out_uv_ch_clk_en = 0;
		stDs1ClkCtl->ds_out1_osd0_ch_clk_en = 0;
		stDs1ClkCtl->ds_out1_osd1_ch_clk_en = 0;
		stDs1ClkCtl->ds_out1_osd2_ch_clk_en = 0;
		stDs1DmaEn->ds_out1_dma_en = 0;
		stDs1DmaEn->ds_out1_y_ch_dma_en = 0;
		stDs1DmaEn->ds_out1_uv_ch_dma_en = 0;
		stDs1DmaEn->ds1_osd0_ch_dma_en = 0;
		stDs1DmaEn->ds1_osd1_ch_dma_en = 0;
		stDs1DmaEn->ds1_osd2_ch_dma_en = 0;
	}
    pstDs1OutAttr.pix_remap_out1_en = ds1Info->pix_remap_out1_en;
    ISP_WRAP_DS1_PIXEL_FORMAT_CTL_S *stDs1PixelFormat = &pstDs1OutAttr.stDs1PixelFormat;
	stDs1PixelFormat->ds1_out_img_format = ds1Info->ds1_out_img_format;//ISP_YUV_DATA;
	stDs1PixelFormat->ds1_out_img_out_format = ds1Info->ds1_out_img_out_format;//IN_YUV420;
	stDs1PixelFormat->ds1_out_pixel_width = ds1Info->ds1_out_pxl_width;//PIXEL_8;
	stDs1PixelFormat->ds1_out_yuv422_pxl_order = ds1Info->ds1_out_yuv422_pxl_order;//YUYV;
	stDs1PixelFormat->ds1_out_yuv_in_format = ds1Info->ds1_out_yuv_in_format;//IN_YUV422;
    ISP_WRAP_DS1_OUT_SIZE_S         *stDs1OutSize = &pstDs1OutAttr.stDs1OutSize;
	stDs1OutSize->ds1_out_h_size = ds1Info->ds1_size.Width - 1;
	stDs1OutSize->ds1_out_v_size = ds1Info->ds1_size.Height - 1;
    ISP_WRAP_DS1_BUF_S              *stDs1Buf = &pstDs1OutAttr.stDs1Buf;
    stDs1Buf->ds1_y_buf0_base = ds1Info->ds1_y_buf0_base;//ISP_BUF_DS1_Y;
    stDs1Buf->ds1_y_buf1_base = ds1Info->ds1_y_buf1_base;//ISP_BUF_DS1_Y;
    stDs1Buf->ds1_y_line_stride = ds1Info->ds1_line_stride;//(Width +15)/16*16;//ISP_BUF_DS1_Y_STRIDE;
    stDs1Buf->ds1_uv_line_stride = ds1Info->ds1_line_stride;//(Width +15)/16*16;//ISP_BUF_DS1_Y_STRIDE;	
    stDs1Buf->ds1_uv_buf0_base = ds1Info->ds1_uv_buf0_base;//ISP_BUF_DS1_Y + stDs1Buf->ds1_y_line_stride *Height;//ISP_BUF_DS1_UV;
    stDs1Buf->ds1_uv_buf1_base = ds1Info->ds1_uv_buf1_base;//ISP_BUF_DS1_Y + stDs1Buf->ds1_y_line_stride *Height;//ISP_BUF_DS1_UV;
    Isp_Drv_F2k_Wrap_SetDs1Out(isp,&pstDs1OutAttr);
}
//
void isp_f2k_wrap_SetDs2Out(struct k510_isp_device *isp,struct isp_wrap_ds2_info *ds2Info)
{
	unsigned int Width = ds2Info->ds2_size.Width;
	unsigned int Height = ds2Info->ds2_size.Height;
    ISP_WRAP_DS2OUT_ATTR_S pstDs2OutAttr;
    ISP_WRAP_DS2_CLK_CTL_S          *stDs2ClkCtl = &pstDs2OutAttr.stDs2ClkCtl;
    ISP_WRAP_DS2_DMA_EN_CTL_S       *stDs2DmaEn = &pstDs2OutAttr.stDs2DmaEn;

	stDs2ClkCtl->ds2_out_clk_en = 1;
	stDs2ClkCtl->ds2_out_r_ch_clk_en = 1;
	stDs2ClkCtl->ds2_out_g_ch_clk_en = 1;
	stDs2ClkCtl->ds2_out_b_ch_clk_en = 1;

	if( 1 == ds2Info->ds2_out_en )
	{
		stDs2ClkCtl->ds_out2_osd0_ch_clk_en = 0;
		stDs2ClkCtl->ds_out2_osd1_ch_clk_en = 0;
		stDs2DmaEn->ds_out2_dma_en = 1 ;
		stDs2DmaEn->ds_out2_r_ch_dma_en = 1;
		stDs2DmaEn->ds_out2_g_ch_dma_en = 1;
		stDs2DmaEn->ds_out2_b_ch_dma_en = 1;
		stDs2DmaEn->ds2_osd0_ch_dma_en = 0;
		stDs2DmaEn->ds2_osd1_ch_dma_en = 0;
		if( 1 == ds2Info->ds2_osd0_en)
		{
			stDs2ClkCtl->ds_out2_osd0_ch_clk_en = 1;
			stDs2DmaEn->ds2_osd0_ch_dma_en = 1;
		}
		if( 1 == ds2Info->ds2_osd1_en)
		{
			stDs2ClkCtl->ds_out2_osd1_ch_clk_en = 1;
			stDs2DmaEn->ds2_osd1_ch_dma_en = 1;
		}
	}
	else
	{
		//stDs2ClkCtl->ds2_out_clk_en = 0;
		//stDs2ClkCtl->ds2_out_r_ch_clk_en = 0;
		//stDs2ClkCtl->ds2_out_g_ch_clk_en = 0;
		//stDs2ClkCtl->ds2_out_b_ch_clk_en = 0;
		stDs2ClkCtl->ds_out2_osd0_ch_clk_en = 0;
		stDs2ClkCtl->ds_out2_osd1_ch_clk_en = 0;
		stDs2DmaEn->ds_out2_dma_en = 0 ;
		stDs2DmaEn->ds_out2_r_ch_dma_en = 0;
		stDs2DmaEn->ds_out2_g_ch_dma_en = 0;
		stDs2DmaEn->ds_out2_b_ch_dma_en = 0;
		stDs2DmaEn->ds2_osd0_ch_dma_en = 0;
		stDs2DmaEn->ds2_osd1_ch_dma_en = 0;
	}
    ISP_WRAP_DS2_PIXEL_FORMAT_CTL_S *stDs2PixelFormat = &pstDs2OutAttr.stDs2PixelFormat;
	stDs2PixelFormat->ds2_out_img_format = ds2Info->ds2_out_img_format;//ISP_RGBRAW_DATA;
	stDs2PixelFormat->ds2_out_img_out_format = ds2Info->ds2_out_img_out_format;//OUT_ARGB;     
	stDs2PixelFormat->ds2_out_pixel_width = ds2Info->ds2_out_pxl_width;//PIXEL_8;
	stDs2PixelFormat->ds2_out_yuv422_pxl_order = ds2Info->ds2_out_yuv422_pxl_order;//YUYV; //not need
	stDs2PixelFormat->ds2_out_yuv_in_format = ds2Info->ds2_out_yuv_in_format;//IN_YUV422; //not need
    ISP_WRAP_DS2_OUT_SIZE_S         *stDs2OutSize = &pstDs2OutAttr.stDs2OutSize;
	stDs2OutSize->ds2_out_h_size = ds2Info->ds2_size.Width - 1;
	stDs2OutSize->ds2_out_v_size = ds2Info->ds2_size.Height - 1;
    ISP_WRAP_DS2_BUF_S              *stDs2Buf = &pstDs2OutAttr.stDs2Buf;
	stDs2Buf->ds2_r_buf0_base = ds2Info->ds2_r_buf0_base;
	stDs2Buf->ds2_r_buf1_base = ds2Info->ds2_r_buf1_base;
	stDs2Buf->ds2_g_buf0_base = ds2Info->ds2_g_buf0_base;
	stDs2Buf->ds2_g_buf1_base = ds2Info->ds2_g_buf1_base;
	stDs2Buf->ds2_b_buf0_base = ds2Info->ds2_b_buf0_base;
	stDs2Buf->ds2_b_buf1_base = ds2Info->ds2_b_buf1_base;
    stDs2Buf->ds2_r_line_stride = ds2Info->ds2_line_stride;
    stDs2Buf->ds2_g_line_stride = ds2Info->ds2_line_stride;
    stDs2Buf->ds2_b_line_stride = ds2Info->ds2_line_stride;
	Isp_Drv_F2k_Wrap_SetDs2Out(isp,&pstDs2OutAttr);	
}
//
void isp_f2k_wrap_SetDmaConfig(struct k510_isp_device *isp)
{
    
    ISP_WRAP_DMA_ATTR_S pstDmaAttr;
    unsigned int DmaChIndex = 0;
	ISP_WRAP_DMA_MODE_S *stDmaChMode = &pstDmaAttr.stDmaChMode;
    stDmaChMode->rd_dma_arb_mode = 0;//1;
    stDmaChMode->wr_dma_arb_mode = 0;//1;
    for(DmaChIndex=0; DmaChIndex<ISP_DMA_CH_NUM; DmaChIndex++)
    {
        stDmaChMode->rd_dma_ch_id[DmaChIndex] = DmaChIndex;
        stDmaChMode->wr_dma_ch_id[DmaChIndex] = DmaChIndex;
        stDmaChMode->rd_dma_ch_weight[DmaChIndex] = 0x01;
        stDmaChMode->wr_dma_ch_weight[DmaChIndex] = 0x01;
        stDmaChMode->rd_dma_ch_priority[DmaChIndex] = ISP_DMA_CH_NUM-DmaChIndex-1;
        stDmaChMode->wr_dma_ch_priority[DmaChIndex] = ISP_DMA_CH_NUM-DmaChIndex-1;
	if(DmaChIndex < 8)
        {
        	stDmaChMode->rd_dma_ch_priority[DmaChIndex] = DmaChIndex;
        	stDmaChMode->wr_dma_ch_priority[DmaChIndex] = DmaChIndex;
        }
    }
	ISP_WRAP_DMA_CFG_S *stDmaCfg = &pstDmaAttr.stDmaCfg;
    for(DmaChIndex=0; DmaChIndex < ISP_DMA_CH_NUM; DmaChIndex++)
    {
        stDmaCfg->stDmaChcfg[DmaChIndex].wr_ch_outst = 1;
        stDmaCfg->stDmaChcfg[DmaChIndex].rd_ch_outst = 1;
        stDmaCfg->stDmaChcfg[DmaChIndex].wr_ch_burstl = 0;
        stDmaCfg->stDmaChcfg[DmaChIndex].rd_ch_burstl = 0;
        stDmaCfg->stDmaChcfg[DmaChIndex].y_uv_noncon_en = 0;
        stDmaCfg->stDmaChcfg[DmaChIndex].y_uv_swap_en = 0;
        stDmaCfg->stDmaChcfg[DmaChIndex].wr_int_line_num = 8;
        stDmaCfg->stDmaChcfg[DmaChIndex].wr_err_dec_en = 0;
        stDmaCfg->stDmaChcfg[DmaChIndex].rd_err_dec_en = 0;
        if((DmaChIndex > 5) && (DmaChIndex < 8))
        {
            stDmaCfg->stDmaChcfg[DmaChIndex].y_uv_noncon_en = 0;
            stDmaCfg->stDmaChcfg[DmaChIndex].y_uv_swap_en = 1;
        }
        if((DmaChIndex > 7) && (DmaChIndex < 13))
        {
            stDmaCfg->stDmaChcfg[DmaChIndex].y_uv_noncon_en = 1;
            stDmaCfg->stDmaChcfg[DmaChIndex].y_uv_swap_en = 1;
        }
        if((DmaChIndex > 12))
        {
            stDmaCfg->stDmaChcfg[DmaChIndex].y_uv_noncon_en = 1;
            stDmaCfg->stDmaChcfg[DmaChIndex].y_uv_swap_en = 1;
        }
	if((DmaChIndex > 1) && (DmaChIndex < 4))
        {
            stDmaCfg->stDmaChcfg[DmaChIndex].wr_ch_outst = 14;
            stDmaCfg->stDmaChcfg[DmaChIndex].rd_ch_outst = 15;
            stDmaCfg->stDmaChcfg[DmaChIndex].wr_outst_en = 1;
            stDmaCfg->stDmaChcfg[DmaChIndex].rd_outst_en = 1;
        }
	if((DmaChIndex > 5) && (DmaChIndex < 8))
        {
            stDmaCfg->stDmaChcfg[DmaChIndex].wr_ch_outst = 6;
            stDmaCfg->stDmaChcfg[DmaChIndex].rd_ch_outst = 1;
            stDmaCfg->stDmaChcfg[DmaChIndex].wr_outst_en = 1;
            stDmaCfg->stDmaChcfg[DmaChIndex].rd_outst_en = 0;
        }
    }
    ISP_WRAP_DMA_ERR_DEC_CFG_S *stErrDecCfg = &pstDmaAttr.stErrDecCfg;
    for(DmaChIndex=0; DmaChIndex < ISP_DMA_CH_NUM; DmaChIndex++)
    {
        stErrDecCfg->stDmaChErrDecCfg[DmaChIndex].wr_err_dec_unit = 1;
        stErrDecCfg->stDmaChErrDecCfg[DmaChIndex].rd_err_dec_unit = 1;
        stErrDecCfg->stDmaChErrDecCfg[DmaChIndex].wr_err_gen_thr = 1;
        stErrDecCfg->stDmaChErrDecCfg[DmaChIndex].rd_err_gen_thr = 1;
    }
	ISP_WRAP_DMA_ERR_STATUS_S *stErrStatus = &pstDmaAttr.stErrStatus;
    for(DmaChIndex=0; DmaChIndex < ISP_DMA_CH_NUM; DmaChIndex++)
    {
        stErrStatus->stDmaErrStatus[DmaChIndex].err_y_wr_status = 0;
        stErrStatus->stDmaErrStatus[DmaChIndex].err_uv_wr_status = 0;
        stErrStatus->stDmaErrStatus[DmaChIndex].err_raw_read_status = 0;
        stErrStatus->stDmaErrStatus[DmaChIndex].err_wr_rst_req = 0;
        stErrStatus->stDmaErrStatus[DmaChIndex].err_rd_rst_req = 0;
    }
    Isp_Drv_F2k_Wrap_SetDma(isp,&pstDmaAttr);
}
//
int isp_f2k_wrap_SetAxiCtl(struct k510_isp_device *isp)
{
	
    ISP_WRAP_AXI_CTL_S pstAxiCtl;
	pstAxiCtl.axi_wr_ch_rst_req = 1;
	pstAxiCtl.axi_rd_ch_rst_req = 1;
	pstAxiCtl.axi_wr_ch_rst_req = 1;
	pstAxiCtl.axi_rd_ch_state = 1;
    Isp_Drv_F2k_Wrap_SetAxiCtl(isp,&pstAxiCtl);
	return 0;
}
/*
*int mask
*/
int isp_f2k_wrap_SetIntMask(struct k510_isp_device *isp)
{
	
	ISP_WRAP_CORE_INT_CTL_S stCoreIntCtl;
    stCoreIntCtl.int_raw_in_mask= 1 ;  
    stCoreIntCtl.int_3a_mask= 1 ;      
    stCoreIntCtl.raw_int_mask= 1 ;     
    stCoreIntCtl.rgb_int_mask= 1 ;     
    stCoreIntCtl.yuv_int_mask= 1 ;     
    stCoreIntCtl.ldc_int_mask= 1 ;     
    stCoreIntCtl.main_out_int_mask= 1 ;
    stCoreIntCtl.isp_awb_int_mask= 1 ; 
    stCoreIntCtl.isp_ae_int_mask= 1 ;  
    stCoreIntCtl.isp_af_int_mask= 1 ; 
    Isp_Drv_F2k_Wrap_SetCoreIntCtlMask(isp,&stCoreIntCtl);

    ISP_WRAP_DMA_WR_INT_MASK0_S stDmaWRMask0;
    stDmaWRMask0.wr_3dnr_y_frm_end_int_mask= 1 ;    
    stDmaWRMask0.wr_3dnr_y_line_base_int_mask= 1 ;  
    stDmaWRMask0.wr_3dnr_y_err_frm_end_int_mask= 1 ;
    stDmaWRMask0.wr_3dnr_y_err_immediate_int_mask= 1 ;     
    stDmaWRMask0.wr_3dnr_uv_frm_end_int_mask= 1 ;   
    stDmaWRMask0.wr_3dnr_uv_line_base_int_mask= 1 ; 
    stDmaWRMask0.wr_3dnr_uv_err_frm_end_int_mask= 1 ;      
    stDmaWRMask0.wr_3dnr_uv_err_immediate_int_mask= 1 ;    
    stDmaWRMask0.ldc_wr_y_frm_end_int_mask= 1 ;     
    stDmaWRMask0.ldc_wr_y_line_base_int_mask= 1 ;   
    stDmaWRMask0.ldc_wr_y_err_frm_end_int_mask= 1 ; 
    stDmaWRMask0.ldc_wr_y_err_immediate_int_mask= 1 ;      
    stDmaWRMask0.ldc_wr_uv_frm_end_int_mask= 1 ;    
    stDmaWRMask0.ldc_wr_uv_line_base_int_mask= 1 ;  
    stDmaWRMask0.ldc_wr_uv_err_frm_end_int_mask= 1 ;
    stDmaWRMask0.ldc_wr_uv_err_immediate_int_mask= 1 ;     
    stDmaWRMask0.wdr_wr_raw_frm_end_int_mask= 1 ;   
    stDmaWRMask0.wdr_wr_raw_line_base_int_mask= 1 ; 
    stDmaWRMask0.wdr_wr_raw_err_frm_end_int_mask= 1 ;      
    stDmaWRMask0.wdr_wr_raw_err_immediate_int_mask= 1 ;    
    stDmaWRMask0.main_out_wr_y_frm_end_int_mask= 1 ;
    stDmaWRMask0.main_out_wr_y_line_base_int_mask= 1 ;     
    stDmaWRMask0.main_out_wr_y_err_frm_end_int_mask= 1 ;   
    stDmaWRMask0.main_out_wr_y_err_immediate_int_mask= 1 ; 
    stDmaWRMask0.main_out_wr_uv_frm_end_int_mask= 1 ;      
    stDmaWRMask0.main_out_wr_uv_line_base_int_mask= 1 ;    
    stDmaWRMask0.main_out_wr_uv_err_frm_end_int_mask= 1 ;  
    stDmaWRMask0.main_out_wr_uv_err_immediate_int_mask= 1 ;
    Isp_Drv_F2k_Wrap_SetDmaWRIntMask0(isp,&stDmaWRMask0);
    ISP_WRAP_DMA_WR_INT_MASK1_S stDmaWRMask1;
    stDmaWRMask1.ds0_out_wr_y_frm_end_mask= 1 ; 
    stDmaWRMask1.ds0_out_wr_y_line_base_mask= 1 ;      
    stDmaWRMask1.ds0_out_wr_y_err_frm_end_mask= 1 ;    
    stDmaWRMask1.ds0_out_wr_y_err_immediate_mask= 1 ;  
    stDmaWRMask1.ds0_out_wr_uv_frm_end_mask= 1 ;
    stDmaWRMask1.ds0_out_wr_uv_line_base_mask= 1 ;     
    stDmaWRMask1.ds0_out_wr_uv_err_frm_end_mask= 1 ;   
    stDmaWRMask1.ds0_out_wr_uv_err_immediate_mask= 1 ; 
    stDmaWRMask1.ds1_out_wr_y_frm_end_mask= 1 ; 
    stDmaWRMask1.ds1_out_wr_y_line_base_mask= 1 ;      
    stDmaWRMask1.ds1_out_wr_y_err_frm_end_mask= 1 ;    
    stDmaWRMask1.ds1_out_wr_y_err_immediate_mask= 1 ;  
    stDmaWRMask1.ds1_out_wr_uv_frm_end_mask= 1 ;
    stDmaWRMask1.ds1_out_wr_uv_line_base_mask= 1 ;     
    stDmaWRMask1.ds1_out_wr_uv_err_frm_end_mask= 1 ;   
    stDmaWRMask1.ds1_out_wr_uv_err_immediate_mask= 1 ; 
    stDmaWRMask1.ds2_out_wr_r_frm_end_mask= 1 ; 
    stDmaWRMask1.ds2_out_wr_r_line_base_mask= 1 ;      
    stDmaWRMask1.ds2_out_wr_r_err_frm_end_mask= 1 ;    
    stDmaWRMask1.ds2_out_wr_r_err_immediate_mask= 1 ;  
    stDmaWRMask1.ds2_out_wr_g_frm_end_mask= 1 ; 
    stDmaWRMask1.ds2_out_wr_g_line_base_mask= 1 ;      
    stDmaWRMask1.ds2_out_wr_g_err_frm_end_mask= 1 ;    
    stDmaWRMask1.ds2_out_wr_g_err_immediate_mask= 1 ;  
    stDmaWRMask1.ds2_out_wr_b_frm_end_mask= 1 ; 
    stDmaWRMask1.ds2_out_wr_b_line_base_mask= 1 ;      
    stDmaWRMask1.ds2_out_wr_b_err_frm_end_mask= 1 ;    
    stDmaWRMask1.ds2_out_wr_b_err_immediate_mask= 1 ; 
    Isp_Drv_F2k_Wrap_SetDmaWRIntMask1(isp,&stDmaWRMask1);
    ISP_WRAP_DMA_RD_INT_MASK0_S stDmaRDMask0;
    stDmaRDMask0.rd_3dnr_y_frm_end_int_mask= 1 ;
    stDmaRDMask0.rd_3dnr_y_line_base_int_mask= 1 ;     
    stDmaRDMask0.rd_3dnr_y_err_frm_end_int_mask= 1 ;   
    stDmaRDMask0.rd_3dnr_y_err_immediate_int_mask= 1 ; 
    stDmaRDMask0.rd_3dnr_uv_frm_end_int_mask= 1 ;      
    stDmaRDMask0.rd_3dnr_uv_line_base_int_mask= 1 ;    
    stDmaRDMask0.rd_3dnr_uv_err_frm_end_int_mask= 1 ;  
    stDmaRDMask0.rd_3dnr_uv_err_immediate_int_mask= 1 ;
    stDmaRDMask0.ldc_rd_y_frm_end_int_mask= 1 ; 
    stDmaRDMask0.ldc_rd_y_line_base_int_mask= 1 ;      
    stDmaRDMask0.ldc_rd_y_err_frm_end_int_mask= 1 ;    
    stDmaRDMask0.ldc_rd_y_err_immediate_int_mask= 1 ;  
    stDmaRDMask0.ldc_rd_uv_frm_end_int_mask= 1 ;
    stDmaRDMask0.ldc_rd_uv_line_base_int_mask= 1 ;     
    stDmaRDMask0.ldc_rd_uv_err_frm_end_int_mask= 1 ;   
    stDmaRDMask0.ldc_rd_uv_err_immediate_int_mask= 1 ; 
    stDmaRDMask0.wdr_rd_raw_frm_end_int_mask= 1 ;      
    stDmaRDMask0.wdr_rd_raw_line_base_int_mask= 1 ;    
    stDmaRDMask0.wdr_rd_raw_err_frm_end_int_mask= 1 ;  
    stDmaRDMask0.wdr_rd_raw_err_immediate_int_mask= 1 ;
    Isp_Drv_F2k_Wrap_SetDmaRDIntMask0(isp,&stDmaRDMask0);  
}
//
int isp_f2k_wrap_SetConfigDone(struct k510_isp_device *isp,unsigned int wp_en)
{
	
    ISP_WRAP_CONFIG_DONE_S pstConfigDone;
	pstConfigDone.int_polarity = 1;
	pstConfigDone.sen_reg_pro_en = wp_en;//0;
	if( wp_en == 1)
	{
		pstConfigDone.wrap_config_done_en = 0;
	}
	else
	{
	pstConfigDone.wrap_config_done_en = 1;
	}
    Isp_Drv_F2k_Wrap_SetConfigdoneCtl(isp,&pstConfigDone);
	return 0;
}
//
int isp_f2k_wrap_reset(struct k510_isp_device *isp)
{
	unsigned int stData;
	isp_f2k_wrap_SetConfigDone(isp,1);
	//
	stData = 0;
	isp_reg_writel(isp,stData,ISP_IOMEM_F2K_WRAP,ISP_WRAP_PIPE_CLK_CTL);
	isp_reg_writel(isp,stData,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DMA_EN_CTL);
	//
	stData = 0xffffffff;
	isp_reg_writel(isp,stData,ISP_IOMEM_F2K_WRAP,ISP_WRAP_SWRST_CTL);
	isp_reg_writel(isp,stData,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DMA_SWRST_CTL);
	//
	isp_f2k_wrap_SetConfigDone(isp,0);
	return 0;
}
//
static int isp_f2k_wrap_config(struct isp_f2k_device *f2k,struct isp_wrap_cfg_info *isp_wrap_cfg)
{
	struct k510_isp_device *isp = to_isp_device(f2k);
    //struct isp_wrap_cfg_info *isp_wrap_cfg =&isp_cfg->isp_wrap_cfg;
	isp_f2k_wrap_SetConfigDone(isp,1); 
    //
    isp_f2k_wrap_SetComRst(isp);
    //
    isp_f2k_wrap_SetDmaRst(isp);
    //
	unsigned int external_out_ch_sel = isp_wrap_cfg->isp_out_sel;
	Isp_Drv_F2k_Wrap_SetModeCtl_OutSel(isp,external_out_ch_sel);
    //
	unsigned int dvp_input_ch_type = isp_wrap_cfg->dvp_ch_mode;
	Isp_Drv_F2k_Wrap_SetModeCtl_DvpMode(isp,dvp_input_ch_type);
    //
    isp_f2k_wrap_SetPipeClkCtl(isp);
    //
    struct isp_wrap_wdr_info  *wdrInfo = &isp_wrap_cfg->wdrInfo;
	isp_f2k_wrap_SetWdr(isp,wdrInfo);
    //
	struct isp_wrap_3dnr_info *nr3dInfo = &isp_wrap_cfg->nr3dInfo;
	isp_f2k_wrap_Set3dnr(isp,nr3dInfo);
    //
	struct isp_wrap_ldc_info *ldcInfo = &isp_wrap_cfg->ldcInfo;
	isp_f2k_wrap_SetLdc(isp,ldcInfo);
    //
	struct isp_wrap_main_info	*mainInfo = &isp_wrap_cfg->mainInfo;
	isp_f2k_wrap_SetMainOut(isp,mainInfo);
    //
	struct isp_wrap_ds0_info	  *ds0Info = &isp_wrap_cfg->ds0Info;
	isp_f2k_wrap_SetDs0Out(isp,ds0Info);
    //
	struct isp_wrap_ds1_info	  *ds1Info = &isp_wrap_cfg->ds1Info;
	isp_f2k_wrap_SetDs1Out(isp,ds1Info);
    //
	struct isp_wrap_ds2_info	  *ds2Info = &isp_wrap_cfg->ds2Info;

	isp_f2k_wrap_SetDs2Out(isp,ds2Info);
	//
	isp_f2k_wrap_SetDmaConfig(isp);
    //
	isp_f2k_wrap_SetAxiCtl(isp);
    //
	isp_f2k_wrap_SetIntMask(isp);
    //
	isp_f2k_wrap_SetConfigDone(isp,0); 

    return 0; 
}
//wrap int
static int isp_f2k_wrap_setIntCore(struct k510_isp_device *isp,struct isp_wrap_intcore_info *intCoreEn)
{
    ISP_WRAP_CORE_INT_CTL_S coreIntMask;
    coreIntMask.int_raw_in_mask = ~intCoreEn->int_raw_in_en;  
    coreIntMask.int_3a_mask = ~intCoreEn->int_3a_en;      
    coreIntMask.raw_int_mask = ~intCoreEn->raw_int_en;     
    coreIntMask.rgb_int_mask = ~intCoreEn->rgb_int_en;     
    coreIntMask.yuv_int_mask = ~intCoreEn->yuv_int_en;     
    coreIntMask.ldc_int_mask = ~intCoreEn->ldc_int_en;     
    coreIntMask.main_out_int_mask = ~intCoreEn->main_out_int_en;
    coreIntMask.isp_awb_int_mask = ~intCoreEn->isp_awb_int_en; 
    coreIntMask.isp_ae_int_mask = ~intCoreEn->isp_ae_int_en;  
    coreIntMask.isp_af_int_mask = ~intCoreEn->isp_af_int_en;  
	Isp_Drv_F2k_Wrap_SetCoreIntCtlMask(isp,&coreIntMask);
	return 0;
};
//
static int isp_f2k_wrap_setIntWr0(struct k510_isp_device *isp,struct isp_wrap_intwr0_info *intWr0En)
{
    ISP_WRAP_DMA_WR_INT_MASK0_S wrIntMask0;
    wrIntMask0.wr_3dnr_y_frm_end_int_mask = ~intWr0En->wr_3dnr_y_frm_end_int_en;  
    wrIntMask0.wr_3dnr_y_line_base_int_mask = 1;  
    wrIntMask0.wr_3dnr_y_err_frm_end_int_mask = 1;
    wrIntMask0.wr_3dnr_y_err_immediate_int_mask = 1;     
    wrIntMask0.wr_3dnr_uv_frm_end_int_mask = ~intWr0En->wr_3dnr_uv_frm_end_int_en; 
    wrIntMask0.wr_3dnr_uv_line_base_int_mask = 1; 
    wrIntMask0.wr_3dnr_uv_err_frm_end_int_mask = 1;      
    wrIntMask0.wr_3dnr_uv_err_immediate_int_mask = 1;    
    wrIntMask0.ldc_wr_y_frm_end_int_mask = ~intWr0En->ldc_wr_y_frm_end_int_en; 
    wrIntMask0.ldc_wr_y_line_base_int_mask = 1;   
    wrIntMask0.ldc_wr_y_err_frm_end_int_mask = 1; 
    wrIntMask0.ldc_wr_y_err_immediate_int_mask = 1;      
    wrIntMask0.ldc_wr_uv_frm_end_int_mask = ~intWr0En->ldc_wr_uv_frm_end_int_en;   
    wrIntMask0.ldc_wr_uv_line_base_int_mask = 1;  
    wrIntMask0.ldc_wr_uv_err_frm_end_int_mask = 1;
    wrIntMask0.ldc_wr_uv_err_immediate_int_mask = 1;     
    wrIntMask0.wdr_wr_raw_frm_end_int_mask = ~intWr0En->wdr_wr_raw_frm_end_int_en;   
    wrIntMask0.wdr_wr_raw_line_base_int_mask = 1; 
    wrIntMask0.wdr_wr_raw_err_frm_end_int_mask = 1;      
    wrIntMask0.wdr_wr_raw_err_immediate_int_mask = 1; 
    wrIntMask0.main_out_wr_y_frm_end_int_mask = ~intWr0En->main_out_wr_y_frm_end_int_en;
    wrIntMask0.main_out_wr_y_line_base_int_mask = 1;     
    wrIntMask0.main_out_wr_y_err_frm_end_int_mask = 1;   
    wrIntMask0.main_out_wr_y_err_immediate_int_mask = 1; 
    wrIntMask0.main_out_wr_uv_frm_end_int_mask = ~intWr0En->main_out_wr_uv_frm_end_int_en;      
    wrIntMask0.main_out_wr_uv_line_base_int_mask = 1;    
    wrIntMask0.main_out_wr_uv_err_frm_end_int_mask = 1;  
    wrIntMask0.main_out_wr_uv_err_immediate_int_mask = 1;
	Isp_Drv_F2k_Wrap_SetDmaWRIntMask0(isp,&wrIntMask0);
	return 0;
}
//
static int isp_f2k_wrap_setIntWr1(struct k510_isp_device *isp,struct isp_wrap_intwr1_info *intWr1En)
{
    ISP_WRAP_DMA_WR_INT_MASK1_S wrIntMask1;
	wrIntMask1.ds0_out_wr_y_frm_end_mask = ~intWr1En->ds0_out_wr_y_frm_end_en;
	wrIntMask1.ds0_out_wr_y_line_base_mask = 1;     
	wrIntMask1.ds0_out_wr_y_err_frm_end_mask = 1;   
	wrIntMask1.ds0_out_wr_y_err_immediate_mask = 1; 
	wrIntMask1.ds0_out_wr_uv_frm_end_mask = ~intWr1En->ds0_out_wr_uv_frm_end_en;      
	wrIntMask1.ds0_out_wr_uv_line_base_mask = 1;    
	wrIntMask1.ds0_out_wr_uv_err_frm_end_mask = 1;  
	wrIntMask1.ds0_out_wr_uv_err_immediate_mask = 1;
	wrIntMask1.ds1_out_wr_y_frm_end_mask = ~intWr1En->ds1_out_wr_y_frm_end_en;
	wrIntMask1.ds1_out_wr_y_line_base_mask = 1;     
	wrIntMask1.ds1_out_wr_y_err_frm_end_mask = 1;   
	wrIntMask1.ds1_out_wr_y_err_immediate_mask = 1; 
	wrIntMask1.ds1_out_wr_uv_frm_end_mask = ~intWr1En->ds1_out_wr_uv_frm_end_en;      
	wrIntMask1.ds1_out_wr_uv_line_base_mask = 1;    
	wrIntMask1.ds1_out_wr_uv_err_frm_end_mask = 1;  
	wrIntMask1.ds1_out_wr_uv_err_immediate_mask = 1;
	wrIntMask1.ds2_out_wr_r_frm_end_mask = ~intWr1En->ds2_out_wr_r_frm_end_en;
	wrIntMask1.ds2_out_wr_r_line_base_mask = 1;     
	wrIntMask1.ds2_out_wr_r_err_frm_end_mask = 1;   
	wrIntMask1.ds2_out_wr_r_err_immediate_mask = 1; 
	wrIntMask1.ds2_out_wr_g_frm_end_mask = ~intWr1En->ds2_out_wr_g_frm_end_en;
	wrIntMask1.ds2_out_wr_g_line_base_mask = 1;     
	wrIntMask1.ds2_out_wr_g_err_frm_end_mask = 1;   
	wrIntMask1.ds2_out_wr_g_err_immediate_mask = 1; 
	wrIntMask1.ds2_out_wr_b_frm_end_mask = ~intWr1En->ds2_out_wr_b_frm_end_en; 
	wrIntMask1.ds2_out_wr_b_line_base_mask = 1;     
	wrIntMask1.ds2_out_wr_b_err_frm_end_mask = 1;   
	wrIntMask1.ds2_out_wr_b_err_immediate_mask = 1;   
	Isp_Drv_F2k_Wrap_SetDmaWRIntMask1(isp,&wrIntMask1);
	return 0;
}
//
static int isp_f2k_wrap_setIntRd0(struct k510_isp_device *isp,struct isp_wrap_intrd0_info *intRd0En)
{
    ISP_WRAP_DMA_RD_INT_MASK0_S rdIntMask0;
    rdIntMask0.rd_3dnr_y_frm_end_int_mask = ~intRd0En->rd_3dnr_y_frm_end_int_en;
    rdIntMask0.rd_3dnr_y_line_base_int_mask = 1;     
    rdIntMask0.rd_3dnr_y_err_frm_end_int_mask = 1;   
    rdIntMask0.rd_3dnr_y_err_immediate_int_mask = 1; 
    rdIntMask0.rd_3dnr_uv_frm_end_int_mask = ~intRd0En->rd_3dnr_uv_frm_end_int_en;      
    rdIntMask0.rd_3dnr_uv_line_base_int_mask = 1;    
    rdIntMask0.rd_3dnr_uv_err_frm_end_int_mask = 1;  
    rdIntMask0.rd_3dnr_uv_err_immediate_int_mask = 1;
    rdIntMask0.ldc_rd_y_frm_end_int_mask = ~intRd0En->ldc_rd_y_frm_end_int_en; 
    rdIntMask0.ldc_rd_y_line_base_int_mask = 1;      
    rdIntMask0.ldc_rd_y_err_frm_end_int_mask = 1;    
    rdIntMask0.ldc_rd_y_err_immediate_int_mask = 1;  
    rdIntMask0.ldc_rd_uv_frm_end_int_mask = ~intRd0En->ldc_rd_uv_frm_end_int_en;
    rdIntMask0.ldc_rd_uv_line_base_int_mask = 1;     
    rdIntMask0.ldc_rd_uv_err_frm_end_int_mask = 1;   
    rdIntMask0.ldc_rd_uv_err_immediate_int_mask = 1; 
    rdIntMask0.wdr_rd_raw_frm_end_int_mask = ~intRd0En->wdr_rd_raw_frm_end_int_en;      
    rdIntMask0.wdr_rd_raw_line_base_int_mask = 1;    
    rdIntMask0.wdr_rd_raw_err_frm_end_int_mask = 1;  
    rdIntMask0.wdr_rd_raw_err_immediate_int_mask = 1;
	Isp_Drv_F2k_Wrap_SetDmaRDIntMask0(isp,&rdIntMask0);
	return 0;
}

void k510isp_f2k_irq_enable(struct k510_isp_device *isp,struct isp_irq_info *irq_info) 
{
	dev_dbg(isp->dev,"%s:start\n",__func__);
	struct isp_wrap_intcore_info intCoreEn;
	intCoreEn.int_raw_in_en = irq_info->raw_en;
	if((1 ==irq_info->awb_en)||(1 ==irq_info->ae_en)||(1 ==irq_info->af_en))
	{
		intCoreEn.int_3a_en = 1; 
	}
	else 
		intCoreEn.int_3a_en = 0;      
	intCoreEn.raw_int_en = irq_info->raw_en;     
	intCoreEn.rgb_int_en = irq_info->rgb_en;     
	intCoreEn.yuv_int_en = irq_info->yuv_en;     
	intCoreEn.ldc_int_en = irq_info->ldc_core_en;     
	intCoreEn.main_out_int_en = irq_info->main_core_en;
	intCoreEn.isp_awb_int_en = irq_info->awb_en; 
	intCoreEn.isp_ae_int_en = irq_info->ae_en;  
	intCoreEn.isp_af_int_en = irq_info->af_en;  
	isp_f2k_wrap_setIntCore(isp,&intCoreEn);

	struct isp_wrap_intwr0_info intWr0En;
	intWr0En.wr_3dnr_y_frm_end_int_en = irq_info->nr3d_w_en; 
	intWr0En.wr_3dnr_uv_frm_end_int_en = irq_info->nr3d_w_en;
	intWr0En.ldc_wr_y_frm_end_int_en = irq_info->ldc_w_en;
	intWr0En.ldc_wr_uv_frm_end_int_en = irq_info->ldc_w_en;
	intWr0En.wdr_wr_raw_frm_end_int_en = irq_info->wdr_w_en;
	intWr0En.main_out_wr_y_frm_end_int_en = irq_info->main_dma_en;     
	intWr0En.main_out_wr_uv_frm_end_int_en = irq_info->main_dma_en;
	isp_f2k_wrap_setIntWr0(isp,&intWr0En);

	struct isp_wrap_intwr1_info intWr1En;
	intWr1En.ds0_out_wr_y_frm_end_en = irq_info->ds0_en;
	intWr1En.ds0_out_wr_uv_frm_end_en = irq_info->ds0_en;
	intWr1En.ds1_out_wr_y_frm_end_en = irq_info->ds1_en;
	intWr1En.ds1_out_wr_uv_frm_end_en = irq_info->ds1_en;
	intWr1En.ds2_out_wr_r_frm_end_en = irq_info->ds2_en;
	intWr1En.ds2_out_wr_g_frm_end_en = irq_info->ds2_en;
	intWr1En.ds2_out_wr_b_frm_end_en = irq_info->ds2_en;
	isp_f2k_wrap_setIntWr1(isp,&intWr1En);

	struct isp_wrap_intrd0_info intRd0En;
	intRd0En.rd_3dnr_y_frm_end_int_en = irq_info->nr3d_r_en;
	intRd0En.rd_3dnr_uv_frm_end_int_en = irq_info->nr3d_r_en;
	intRd0En.ldc_rd_y_frm_end_int_en = irq_info->ldc_r_en; 
	intRd0En.ldc_rd_uv_frm_end_int_en = irq_info->ldc_r_en;
	intRd0En.wdr_rd_raw_frm_end_int_en = irq_info->wdr_r_en;
	isp_f2k_wrap_setIntRd0(isp,&intRd0En);
}
/*********************************************************************************
 * ISP CORE
**********************************************************************************/
//ITC
static int isp_f2k_core_SetItc(struct k510_isp_device *isp,ITC_INFO_S *itcInfo)
{
    ISP_CORE_ITC_CTL_S stItcCtl;
	stItcCtl.hsync_pol = itcInfo->hsync_pol;    
    stItcCtl.vsync_pol = itcInfo->vsync_pol;    
    stItcCtl.hsync_input_timing = itcInfo->hsync_input_timing;  
    stItcCtl.vsync_input_timing = itcInfo->vsync_input_timing;
    stItcCtl.mirror_ctl = itcInfo->flip_ctl;  
    stItcCtl.video_format_ctl = itcInfo->video_fmt_sel; 
    Isp_Drv_F2k_Core_SetItcCtl(isp,&stItcCtl);
	return 0;
}
//IMAGE
static int isp_f2k_core_SetImage(struct k510_isp_device *isp,ITC_INFO_S *itcInfo)
{

    ISP_CORE_IMAGE_ATTR_S stImgAttr;
    stImgAttr.image_height = itcInfo->total_size.Height - 1;
    stImgAttr.image_width = itcInfo->total_size.Width - 1; 
    stImgAttr.image_v_start = itcInfo->itc_size.Height_st;      
    stImgAttr.image_h_start = itcInfo->itc_size.Width_st;      
    stImgAttr.image_active_width = itcInfo->itc_size.Width; 
    stImgAttr.image_active_height = itcInfo->itc_size.Height;
    Isp_Drv_F2k_Core_SetImageAttr(isp,&stImgAttr);
	return 0;
}
//TPG
static int isp_f2k_core_SetTpgCtl(struct k510_isp_device *isp,TPG_INFO_S *tpgInfo)
{
    ISP_CORE_TEST_CTL_S stTestCtl;
	stTestCtl.test_pattern_en = tpgInfo->tpg_en;
	stTestCtl.bayer_mode_sel = tpgInfo->bayer_mode_sel;
	stTestCtl.motion_mode_sel = tpgInfo->motion_mode_sel;
	stTestCtl.test_pattern_sel = tpgInfo->tpg_sel;
	stTestCtl.wdr_l_mul_data = tpgInfo->wdr_l_mul_data;
	stTestCtl.wdr_m_mul_data = tpgInfo->wdr_m_mul_data;
	stTestCtl.wdr_s_mul_data = tpgInfo->wdr_s_mul_data; 
    Isp_Drv_F2k_Core_SetTestCtl(isp,&stTestCtl);
	return 0;
}
//BLC
static int isp_f2k_core_SetBlcCtl(struct k510_isp_device *isp,BLC_INFO_S *blcInfo)
{

    ISP_CORE_BLC_CTL_S stBlcCtl;
	stBlcCtl.blc_en = blcInfo->blc_en;
	stBlcCtl.blc_offset = blcInfo->blc_offset;
	stBlcCtl.blc_ratio = blcInfo->blc_ratio;
    Isp_Drv_F2k_Core_SetBlcCtl(isp,&stBlcCtl);
	return 0;
}
//LSC
static int isp_f2k_core_SetLscCtl(struct k510_isp_device *isp,LSC_INFO_S *lscInfo)
{

    ISP_CORE_LSC_CTL_S stLscCtl;
    stLscCtl.lsc_en = lscInfo->lsc_en; 
    stLscCtl.lsc_h_center = lscInfo->lsc_h_center;     
    stLscCtl.lsc_v_center = lscInfo->lsc_v_center;
    stLscCtl.lsc_red_ratio = lscInfo->lsc_r_ratio;
    stLscCtl.lsc_green_ratio = lscInfo->lsc_g_ratio;  
    stLscCtl.lsc_blue_ratio = lscInfo->lsc_b_ratio;   
    stLscCtl.lsc_ir_ratio = lscInfo->lsc_ir_ratio;  
    Isp_Drv_F2k_Core_SetLscCtl(isp,&stLscCtl);
	return 0;
}
//AE
static int isp_f2k_core_SetAeCtl(struct k510_isp_device *isp,AE_INFO_S *aeInfo)
{

    ISP_CORE_AE_CTL_S stAeCtl;
    stAeCtl.ae_as_en = aeInfo->ae_as_en;       
    stAeCtl.ae_ag_en = aeInfo->ae_ag_en;       
    stAeCtl.ae_airis_en = aeInfo->ae_airis_en;    
    stAeCtl.ae_enter_ls_sel = aeInfo->ae_enter_ls_sel;
    stAeCtl.ae_exit_ls_sel = aeInfo->ae_exit_ls_sel; 
    stAeCtl.ae_win_mode_sel = aeInfo->ae_win_mode_sel;
    stAeCtl.ae_back_light_mode_sel = aeInfo->ae_back_light_mode_sel;
    stAeCtl.ae_day_change_en = aeInfo->ae_day_change_en;    
    stAeCtl.ae_day_change_sel = aeInfo->ae_day_change_sel;     
    stAeCtl.ae_win_h_start = aeInfo->ae_win_size.h_start;
    stAeCtl.ae_win_v_start = aeInfo->ae_win_size.v_start;
    stAeCtl.ae_win_h_end = aeInfo->ae_win_size.h_end;  
    stAeCtl.ae_win_v_end = aeInfo->ae_win_size.v_end;  
    stAeCtl.ae_tar_bright = aeInfo->ae_tar_bright;      
    stAeCtl.ae_tar_bright_range = aeInfo->ae_tar_bright_range;
    stAeCtl.ae_l_ex_time = aeInfo->ae_l_ex_time;
    stAeCtl.ae_m_ex_time = aeInfo->ae_m_ex_time;
    stAeCtl.ae_s_ex_time = aeInfo->ae_s_ex_time;
    stAeCtl.ae_agc = aeInfo->ae_agc;     
    stAeCtl.ae_ad_shuttle_freq = aeInfo->ae_ad_shuttle_freq;
    stAeCtl.ae_ad_gain_freq = aeInfo->ae_ad_gain_freq;
    stAeCtl.ae_adjust_step_max = aeInfo->ae_adjust_step_max;
    stAeCtl.ae_ex_value_max = aeInfo->ae_ex_value_max;  
    stAeCtl.ae_ex_value_mid = aeInfo->ae_ex_value_mid;    
    stAeCtl.ae_ex_value_min = aeInfo->ae_ex_value_min; 
    stAeCtl.ae_gain_value_max = aeInfo->ae_gain_value_max;    
    stAeCtl.ae_gain_value_mid = aeInfo->ae_gain_value_mid;
    stAeCtl.ae_gain_value_min = aeInfo->ae_gain_value_min;
    stAeCtl.ae_dn_switch_ad_step_max = aeInfo->ae_dn_switch_ad_step_max;
    stAeCtl.ae_dn_switch_wait_time = aeInfo->ae_dn_switch_wait_time;
    stAeCtl.ape_max_diff = aeInfo->ape_max_diff;
    stAeCtl.ape_drv_signal_max = aeInfo->ape_drv_signal_max;    
    stAeCtl.ape_coeff_distance = aeInfo->ape_coeff_distance;    
    stAeCtl.ape_coeff_speed = aeInfo->ape_coeff_speed;
    stAeCtl.ape_coeff_acceleration = aeInfo->ape_coeff_acceleration;
    stAeCtl.ape_drv_manual_value = aeInfo->ape_drv_manual_value;  
    stAeCtl.ape_damp_manual_value = aeInfo->ape_damp_manual_value;
    Isp_Drv_F2k_Core_SetAeCtl(isp,&stAeCtl);
	return 0;
}

static int isp_f2k_core_GetAeSts(struct k510_isp_device *isp,struct k510isp_ae_stats *ae_stats)
{
	ISP_CORE_AE_STS_S pAeSts;
	Isp_Drv_F2k_Core_GetAeSts(isp,&pAeSts);
	ae_stats->ae_wren = pAeSts.ae_value_ready;
	ae_stats->ae_expl = pAeSts.ae_long_cur_ex;
	ae_stats->ae_expm = pAeSts.ae_mid_cur_ex;
	ae_stats->ae_exps = pAeSts.ae_short_cur_ex;
	ae_stats->ae_agco = pAeSts.ae_digital_gain;
	ae_stats->y_av	  = pAeSts.ave_brightness;
	ae_stats->dn_st	   = pAeSts.ae_day_night_status;
	ae_stats->ae_exp_st = pAeSts.ae_ex_status;
	ae_stats->ae_sum = pAeSts.ae_sum;
	ae_stats->ae_pxl = pAeSts.ae_pixel_sum;
	return 0;
}
//AWB
static int isp_f2k_core_SetAwbCtl(struct k510_isp_device *isp,AWB_INFO_S *awbInfo)
{
    ISP_CORE_AWB_CTL_S stAwbCtl;
    stAwbCtl.awb_d65_en = awbInfo->awb_d65_en; 
    stAwbCtl.awb_ccm_en = awbInfo->awb_ccm_en; 
    stAwbCtl.awb_en = awbInfo->awb_en;     
    stAwbCtl.awb_mode_sel = awbInfo->awb_mode_sel;
    stAwbCtl.awb_hist_mode_sel = awbInfo->awb_hist_mode_sel;
    stAwbCtl.awb_veri_en = awbInfo->awb_veri_en;      
    stAwbCtl.awb_mode_sel = awbInfo->awb_mode_sel;     
    stAwbCtl.awb_value_save_en = awbInfo->awb_value_save_en;
    stAwbCtl.awb_ccm_adp_adjust_en = awbInfo->awb_ccm_adp_adjust_en;
    stAwbCtl.awb_stab_en = awbInfo->awb_stab_en;   
    stAwbCtl.awb_d65_red_gain = awbInfo->awb_d65_red_gain;
    stAwbCtl.awb_d65_blue_gain = awbInfo->awb_d65_blue_gain;
    stAwbCtl.ccm_rr_coff = awbInfo->ccm_coff[0][0];
    stAwbCtl.ccm_rg_coff = awbInfo->ccm_coff[0][1];
    stAwbCtl.ccm_rb_coff = awbInfo->ccm_coff[0][2];
    stAwbCtl.ccm_gr_coff = awbInfo->ccm_coff[1][0];
    stAwbCtl.ccm_gg_coff = awbInfo->ccm_coff[1][1];
    stAwbCtl.ccm_gb_coff = awbInfo->ccm_coff[1][2];
    stAwbCtl.ccm_br_coff = awbInfo->ccm_coff[2][0];
    stAwbCtl.ccm_bg_coff = awbInfo->ccm_coff[2][1];
    stAwbCtl.ccm_bb_coff = awbInfo->ccm_coff[2][2];
    stAwbCtl.ccm_correct_coff = awbInfo->ccm_correct_coff;
    stAwbCtl.awb_win_h_start = awbInfo->awb_win_size.h_start;
    stAwbCtl.awb_win_v_start = awbInfo->awb_win_size.v_start;
    stAwbCtl.awb_win_h_end = awbInfo->awb_win_size.h_end;  
    stAwbCtl.awb_win_v_end = awbInfo->awb_win_size.v_end;  
    stAwbCtl.awb_correct_diff_th = awbInfo->awb_correct_diff_th;     
    stAwbCtl.awb_color_changeres_time = awbInfo->awb_color_changeres_time;
    stAwbCtl.awb_historgram_th = awbInfo->awb_historgram_th;
    stAwbCtl.awb_red_gain_adjust = awbInfo->awb_red_gain_adjust;     
    stAwbCtl.awb_green_gain_adjust = awbInfo->awb_green_gain_adjust;   
    stAwbCtl.awb_blue_gain_adjust = awbInfo->awb_blue_gain_adjust;
    stAwbCtl.awb_red_max_value = awbInfo->awb_red_max_value;
    stAwbCtl.awb_blue_max_value = awbInfo->awb_blue_max_value;      
    stAwbCtl.awb_red_min_value = awbInfo->awb_red_min_value;
    stAwbCtl.awb_blue_min_value = awbInfo->awb_blue_min_value;
    stAwbCtl.awb_red_obj_value = awbInfo->awb_red_obj_value;
    stAwbCtl.awb_blue_obj_value = awbInfo->awb_blue_obj_value;
    Isp_Drv_F2k_Core_SetAwbCtl(isp,&stAwbCtl);
	return 0;
}
//WDR
static int isp_f2k_core_SetWdrCtl(struct k510_isp_device *isp,struct isp_core_wdr_Info *wdrInfo)
{

    ISP_CORE_WDR_CTL_S stWdrCtl;
    stWdrCtl.wdr_fusion_en = wdrInfo->wdr_fusion_en;      
    stWdrCtl.wdr_frame_sel = wdrInfo->wdr_frame_sel;      
    stWdrCtl.wdr_adp_adjust_en = wdrInfo->wdr_adp_adjust_en;  
    stWdrCtl.wdr_stab_en = wdrInfo->wdr_stab_en;
    stWdrCtl.wdr_en = wdrInfo->wdr_en;      
    stWdrCtl.wdr_ghost_remove_en = wdrInfo->wdr_ghost_remove_en; 
    stWdrCtl.wdr_3frame_out_mode = wdrInfo->wdr_3frame_out_mode;
    stWdrCtl.wdr_mode_sel = wdrInfo->wdr_mode_sel;
    stWdrCtl.wdr_2frame_ex_ratio = wdrInfo->wdr_2frame_ex_ratio;
    stWdrCtl.wdr_3frame_ex_ratio = wdrInfo->wdr_3frame_ex_ratio; 
    stWdrCtl.wdr_stat_img_sel = wdrInfo->wdr_stat_img_sel;   
    stWdrCtl.wdr_ltm_data_sel = wdrInfo->wdr_ltm_data_sel;
    stWdrCtl.wdr_tz_data_sel = wdrInfo->wdr_tz_data_sel;
    stWdrCtl.wdr_remove_purple_en = wdrInfo->wdr_remove_purple_en;
    stWdrCtl.wdr_over_ex_ratio_th1 = wdrInfo->wdr_over_ex_ratio_th1;
    stWdrCtl.wdr_over_ex_ratio_th2 = wdrInfo->wdr_over_ex_ratio_th2;
    stWdrCtl.wdr_fusion_ratio_th = wdrInfo->wdr_fusion_ratio_th;
    stWdrCtl.wdr_fusion_value1 = wdrInfo->wdr_fusion_value1;  
    stWdrCtl.wdr_fusion_value2 = wdrInfo->wdr_fusion_value2;
    Isp_Drv_F2k_Core_SetWdrCtl(isp,&stWdrCtl);
	return 0;
}
//CSC
static int isp_f2k_core_SetCscCtl(struct k510_isp_device *isp,CSC_INFO_S *cscInfo)
{

    ISP_CORE_CSC_CTL_S stCscCtl;
    stCscCtl.csc_r2y_00 = cscInfo->csc_r2y[0][0];
    stCscCtl.csc_r2y_01 = cscInfo->csc_r2y[0][1];
    stCscCtl.csc_r2y_02 = cscInfo->csc_r2y[0][2];
    stCscCtl.csc_r2y_10 = cscInfo->csc_r2y[1][0];
    stCscCtl.csc_r2y_11 = cscInfo->csc_r2y[1][1];
    stCscCtl.csc_r2y_12 = cscInfo->csc_r2y[1][2];
    stCscCtl.csc_r2y_20 = cscInfo->csc_r2y[2][0];
    stCscCtl.csc_r2y_21 = cscInfo->csc_r2y[2][1];
    stCscCtl.csc_r2y_22 = cscInfo->csc_r2y[2][2];
    Isp_Drv_F2k_Core_SetCscCtl(isp,&stCscCtl);
	return 0;
}
//ADA
static int isp_f2k_core_SetAdaCtl(struct k510_isp_device *isp,ADA_INFO_S *adaInfo)
{
    ISP_CORE_ADA_CTL_S stAdaCtl;
    stAdaCtl.ada_rgb_gamma_en = adaInfo->ada_rgb_gamma_en; 
    stAdaCtl.ada_yuv_gamma_en = adaInfo->ada_yuv_gamma_en; 
    stAdaCtl.ada_adjust_en = adaInfo->ada_adjust_en;    
    stAdaCtl.ada_img_stab_en = adaInfo->ada_img_stab_en;
    stAdaCtl.ada_ccr_en = adaInfo->ada_ccr_en;
    stAdaCtl.ada_adp_en = adaInfo->ada_adp_en;
    stAdaCtl.ada_adp_ccr_en = adaInfo->ada_adp_ccr_en;   
    stAdaCtl.ada_stat_mode_sel = adaInfo->ada_stat_mode_sel;
    stAdaCtl.ada_enh_mode_sel = adaInfo->ada_enh_mode_sel;
    stAdaCtl.ada_stat_max_value = adaInfo->ada_stat_max_value; 
    stAdaCtl.ada_ad_stren_max_value = adaInfo->ada_ad_stren_max_value;
    stAdaCtl.ada_win_h_start = adaInfo->ada_win_size.h_start;
    stAdaCtl.ada_win_v_start = adaInfo->ada_win_size.v_start;
    stAdaCtl.ada_win_h_end = adaInfo->ada_win_size.h_end;  
    stAdaCtl.ada_win_v_end = adaInfo->ada_win_size.v_end; 
    Isp_Drv_F2k_Core_SetAdaCtl(isp,&stAdaCtl);
	return 0;
}
//RGBIR
static int isp_f2k_core_SetRgbirCtl(struct k510_isp_device *isp,RGBIR_INFO_S *rgbirInfo)
{
    ISP_CORE_RGBIR_CTL_S stRgbirCtl;
    stRgbirCtl.rgbir_en = rgbirInfo->rgbir_en; 
    stRgbirCtl.rgbir_rtf_en = rgbirInfo->rgbir_rtf_en;    
    stRgbirCtl.rgbir_rpc_en = rgbirInfo->rgbir_rpc_en;    
    stRgbirCtl.rgbir_fusion_en = rgbirInfo->rgbir_fusion_en;
    stRgbirCtl.rgbir_output_sel = rgbirInfo->rgbir_output_sel;
    stRgbirCtl.rgbir_rpc_max_value = rgbirInfo->rgbir_rpc_max_value; 
    stRgbirCtl.rgbir_rpc_color_coff = rgbirInfo->rgbir_rpc_color_coff;
    stRgbirCtl.rgbir_rpc_luma_coff = rgbirInfo->rgbir_rpc_luma_coff; 
    stRgbirCtl.rgbir_rpc_th = rgbirInfo->rgbir_rpc_th;
    stRgbirCtl.rgbir_rpc_th1 = rgbirInfo->rgbir_rpc_th1; 
    Isp_Drv_F2k_Core_SetRgbIrCtl(isp,&stRgbirCtl);
	return 0;
}
//2DNR
static int isp_f2k_core_Set2dnrCtl(struct k510_isp_device *isp,NR2D_INFO_S *nr2dInfo)
{
    ISP_CORE_2DNR_CTL_S st2dnrCtl;
    st2dnrCtl.core_2dnr_pcf_en = nr2dInfo->d2nr_pcf_en;
    st2dnrCtl.core_2dnr_raw_en = nr2dInfo->d2nr_raw_en;
    st2dnrCtl.core_2dnr_edge_en = nr2dInfo->d2nr_edge_en;      
    st2dnrCtl.core_2dnr_bap_en = nr2dInfo->d2nr_bap_en;
    st2dnrCtl.core_2dnr_luma_en = nr2dInfo->d2nr_luma_en;      
    st2dnrCtl.core_2dnr_chroma_en = nr2dInfo->d2nr_chroma_en;    
    st2dnrCtl.core_2dnr_pcf_adp_en = nr2dInfo->d2nr_pcf_adp_en;   
    st2dnrCtl.core_2dnr_raw_adp_en = nr2dInfo->d2nr_raw_adp_en;   
    st2dnrCtl.core_2dnr_luma_adp_en = nr2dInfo->d2nr_luma_adp_en;  
    st2dnrCtl.core_2dnr_chroma_adp_en = nr2dInfo->d2nr_chroma_adp_en;
    st2dnrCtl.core_2dnr_raw_intensity = nr2dInfo->d2nr_raw_intensity;
    st2dnrCtl.core_2dnr_bap_intensity = nr2dInfo->d2nr_bap_intensity;
    st2dnrCtl.core_2dnr_edge_intensity = nr2dInfo->d2nr_edge_intensity;
    st2dnrCtl.core_2dnr_luma_intensity = nr2dInfo->d2nr_luma_intensity;
    st2dnrCtl.core_2dnr_chroma_intensity = nr2dInfo->d2nr_chroma_intensity;
    Isp_Drv_F2k_Core_Set2DnrCtl(isp,&st2dnrCtl);
	return 0;
}
//3DNR
static int isp_f2k_core_Set3dnrCtl(struct k510_isp_device *isp,NR3D_INFO_S *nr3dInfo)
{
    ISP_CORE_3DNR_CTL_S st3dnrCtl;
    st3dnrCtl.core_3dnr_en = nr3dInfo->d3nr_en;     
    st3dnrCtl.core_3dnr_pre_luma_en = nr3dInfo->d3nr_pre_luma_en;  
    st3dnrCtl.core_3dnr_pre_chroma_en = nr3dInfo->d3nr_pre_chroma_en; 
    st3dnrCtl.core_3dnr_main_luma_en = nr3dInfo->d3nr_main_luma_en;
    st3dnrCtl.core_3dnr_main_chroma_en = nr3dInfo->d3nr_main_chroma_en;
    st3dnrCtl.core_3dnr_post_luma_en = nr3dInfo->d3nr_post_luma_en;  
    st3dnrCtl.core_3dnr_post_chroma_en = nr3dInfo->d3nr_post_chroma_en;
    st3dnrCtl.core_3dnr_2d_luma_en = nr3dInfo->d3nr_2d_luma_en;
    st3dnrCtl.core_3dnr_2d_chroma_en = nr3dInfo->d3nr_2d_luma_en;
    st3dnrCtl.core_3dnr_wb_en = nr3dInfo->d3nr_wb_en;  
    st3dnrCtl.core_3dnr_wb_sel = nr3dInfo->d3nr_wb_sel;
    st3dnrCtl.core_3dnr_adp_luma_en = nr3dInfo->d3nr_adp_luma_en;
    st3dnrCtl.core_3dnr_adp_chroma_en = nr3dInfo->d3nr_adp_chroma_en;
    st3dnrCtl.core_3dnr_pre_luma_th = nr3dInfo->d3nr_pre_luma_th; 
    st3dnrCtl.core_3dnr_pre_luma_intensity = nr3dInfo->d3nr_pre_luma_intensity;   
    st3dnrCtl.core_3dnr_pre_chroma_intensity = nr3dInfo->d3nr_pre_chroma_intensity;
    st3dnrCtl.core_3dnr_mid_filter_th = nr3dInfo->d3nr_mid_filter_th;
    st3dnrCtl.core_3dnr_pre_mid_filter_th = nr3dInfo->d3nr_pre_mid_filter_th;    
    st3dnrCtl.core_3dnr_cur_mid_filter_th = nr3dInfo->d3nr_cur_mid_filter_th;    
    st3dnrCtl.core_3dnr_low_pass_filter_th = nr3dInfo->d3nr_low_pass_filter_th;
    st3dnrCtl.core_3dnr_luma_th = nr3dInfo->d3nr_luma_th;      
    st3dnrCtl.core_3dnr_min_value = nr3dInfo->d3nr_min_value;
    st3dnrCtl.core_3dnr_luma_intensity = nr3dInfo->d3nr_luma_intensity;
    st3dnrCtl.core_3dnr_chroma_intensity = nr3dInfo->d3nr_chroma_intensity;    
    st3dnrCtl.core_3dnr_post_edge_th = nr3dInfo->d3nr_post_edge_th; 
    st3dnrCtl.core_3dnr_post_luma_intensity = nr3dInfo->d3nr_post_luma_intensity;  
    st3dnrCtl.core_3dnr_post_chroma_intensity = nr3dInfo->d3nr_post_chroma_intensity;
    Isp_Drv_F2k_Core_Set3DnrCtl(isp,&st3dnrCtl);
	return 0;
}
//ENH
static int isp_f2k_core_SetEnhCtl(struct k510_isp_device *isp,ENH_INFO_S *enhInfo)
{
    ISP_CORE_ENH_CTL_S stEnhCtl;
    stEnhCtl.enh_ltm_en = enhInfo->enh_ltm_en;
    stEnhCtl.enh_sharp_en = enhInfo->enh_sharp_en;     
    stEnhCtl.enh_cc_en = enhInfo->enh_cc_en; 
    stEnhCtl.enh_adp_ltm_en = enhInfo->enh_adp_ltm_en;    
    stEnhCtl.enh_adp_sharp_en = enhInfo->enh_adp_sharp_en; 
    stEnhCtl.enh_adp_cc_en = enhInfo->enh_adp_cc_en;    
    stEnhCtl.ltm_gain = enhInfo->ltm_gain;  
    stEnhCtl.ltm_th = enhInfo->ltm_th; 
    stEnhCtl.enh_nr_th = enhInfo->enh_nr_th; 
    stEnhCtl.enh_th1 = enhInfo->enh_th1;
    stEnhCtl.enh_th2 = enhInfo->enh_th2;
    stEnhCtl.sharp_gain = enhInfo->sharp_gain; 
    Isp_Drv_F2k_Core_SetEnhLtmCtl(isp,&stEnhCtl);
    Isp_Drv_F2k_Core_SetEnhCCCtl(isp,&stEnhCtl);
    Isp_Drv_F2k_Core_SetEnhSharpenCtl(isp,&stEnhCtl);
	return 0;
}
//POST
static int isp_f2k_core_SetPostCtl(struct k510_isp_device *isp,POST_INFO_S *postInfo)
{
    ISP_CORE_POST_CTL_S stPostCtl;
    stPostCtl.post_cont_ad_en = postInfo->post_cont_ad_en;   
    stPostCtl.post_luma_ad_en = postInfo->post_luma_ad_en;   
    stPostCtl.post_satu_ad_en = postInfo->post_satu_ad_en;
    stPostCtl.cont_ad_intensity = postInfo->cont_ad_intensity;    
    stPostCtl.luma_ad_intensity = postInfo->luma_ad_intensity;
    stPostCtl.satu_ad_intensity = postInfo->satu_ad_intensity;
    Isp_Drv_F2k_Core_SetPostContCtl(isp,&stPostCtl);
    Isp_Drv_F2k_Core_SetPostLumaCtl(isp,&stPostCtl);
    Isp_Drv_F2k_Core_SetPostSatuCtl(isp,&stPostCtl);
	return 0;
}
//OTC
static int isp_f2k_core_SetOtcCtl(struct k510_isp_device *isp,OTC_INFO_S *otcInfo)
{
    ISP_CORE_OTC_CTL_S stOtcCtl;
    stOtcCtl.post_otc_en = otcInfo->post_otc_en;
   	stOtcCtl.otc_yc_sel = otcInfo->otc_yc_sel; 
   	stOtcCtl.otc_uv_format_sel = otcInfo->otc_uv_format_sel; 
   	stOtcCtl.otc_hsync_pol_sel = otcInfo->otc_hsync_pol_sel; 
   	stOtcCtl.otc_vsync_pol_sel = otcInfo->otc_vsync_pol_sel;    
   	stOtcCtl.otc_stt_vr = otcInfo->otc_out_size.Width_st;   
   	stOtcCtl.otc_stt_hr = otcInfo->otc_out_size.Height_st;   
   	stOtcCtl.otc_height = otcInfo->otc_out_size.Height;   
   	stOtcCtl.otc_width = otcInfo->otc_out_size.Width; 
    Isp_Drv_F2k_Core_SetOtcCtl(isp,&stOtcCtl);
	return 0;
}
//LDC
static int isp_f2k_core_SetLdcCtl(struct k510_isp_device *isp,LDC_INFO_S *ldcInfo)
{
    ISP_CORE_LDC_CTL_S stLdcCtl;
    stLdcCtl.ldc_en = ldcInfo->ldc_en; 
    stLdcCtl.ldc_arith_en = ldcInfo->ldc_arith_en;   
    stLdcCtl.ldc_req_freq = ldcInfo->ldc_req_freq;
    stLdcCtl.ldc_stt_ln = ldcInfo->ldc_stt_ln;
    stLdcCtl.ldc_h_center_pos = ldcInfo->ldc_h_center_pos;
    stLdcCtl.ldc_v_center_pos = ldcInfo->ldc_v_center_pos;
    stLdcCtl.ldc_rectify_cr = ldcInfo->ldc_rectify_cr;
    stLdcCtl.ldc_rectify_cz = ldcInfo->ldc_rectify_cz; 
    Isp_Drv_F2k_Core_SetLdcCtl(isp,&stLdcCtl);
	return 0;
}
//AF
static int isp_f2k_core_SetAfCtl(struct k510_isp_device *isp,AF_INFO_S *afInfo)
{
    ISP_CORE_AF_CTL_S stAfCtl;
	stAfCtl.af_stat_en = afInfo->af_stat_en; 
   	stAfCtl.af_stat_mode_sel= afInfo->af_stat_mode_sel; 
    stAfCtl.af_stat_win_h_start= afInfo->af_win_size.h_start;    
    stAfCtl.af_stat_win_v_start= afInfo->af_win_size.v_start; 
    stAfCtl.af_stat_win_h_end= afInfo->af_win_size.h_end;  
    stAfCtl.af_stat_win_v_end= afInfo->af_win_size.v_end;
    Isp_Drv_F2k_Core_SetAfCtl(isp,&stAfCtl);
	return 0;
}
//3DNR FBC
static int isp_f2k_core_SetFbc(struct k510_isp_device *isp,FBC_INFO_S *fbcInfo)
{
    ISP_FBC_ATTR_S pstfbcAttr;
	ISP_FBC_CTL_S	*stIspFbcCtl = &pstfbcAttr.stIspfbcCtl;
	stIspFbcCtl->fbc_out_format_cfg = fbcInfo->fbc_out_format_cfg;
    Isp_Drv_Fbc_SetCtl(isp,stIspFbcCtl);
	ISP_FBC_SIZE_S *stIspFbcSize = &pstfbcAttr.stIspfbcSize;
	stIspFbcSize->fbc_input_width = fbcInfo->fbc_input_size.Width;
	stIspFbcSize->fbc_input_height = fbcInfo->fbc_input_size.Height;
    Isp_Drv_Fbc_SetSize(isp,stIspFbcSize);
    ISP_FBC_BUF_S *stIspFbcBuf = &pstfbcAttr.stIspfbcBuf;
	ISP_FBC_Y_BUF_S *stIspFbcYBuf = &stIspFbcBuf->stIspFbcYBuf;
	FBCD_BUF_S  *yDataBufInfo = &fbcInfo->yDataBufInfo;
	stIspFbcYBuf->fbc_y_data_buf_base0 = yDataBufInfo->data_buf_base0;
	stIspFbcYBuf->fbc_y_data_buf_base1 = yDataBufInfo->data_buf_base1;
	stIspFbcYBuf->fbc_y_data_stride = yDataBufInfo->data_stride;
	stIspFbcYBuf->fbc_y_data_wr_blen = yDataBufInfo->data_wr_blen;
	FBCD_BUF_S  *yHeadBufInfo = &fbcInfo->yHeadBufInfo;
	stIspFbcYBuf->fbc_y_head_buf_base0 = yHeadBufInfo->data_buf_base0;
	stIspFbcYBuf->fbc_y_head_buf_base1 = yHeadBufInfo->data_buf_base1;
	stIspFbcYBuf->fbc_y_head_stride = yHeadBufInfo->data_stride;
	stIspFbcYBuf->fbc_y_head_wr_blen = yHeadBufInfo->data_wr_blen;
	ISP_FBC_YL_BUF_S *stIspFbcYLBuf = &stIspFbcBuf->stIspFbcYLBuf;
	FBCD_BUF_S *ylDataBufInfo = &fbcInfo->ylDataBufInfo;
	stIspFbcYLBuf->fbc_yl_data_buf_base0 = ylDataBufInfo->data_buf_base0;
	stIspFbcYLBuf->fbc_yl_data_buf_base1 = ylDataBufInfo->data_buf_base1;
	stIspFbcYLBuf->fbc_yl_data_stride = ylDataBufInfo->data_stride;
	stIspFbcYLBuf->fbc_yl_data_wr_blen = ylDataBufInfo->data_wr_blen;
	FBCD_BUF_S *ylHeadBufInfo = &fbcInfo->ylHeadBufInfo;
	stIspFbcYLBuf->fbc_yl_head_buf_base0 = ylHeadBufInfo->data_buf_base0;
	stIspFbcYLBuf->fbc_yl_head_buf_base1 = ylHeadBufInfo->data_buf_base1;
	stIspFbcYLBuf->fbc_yl_head_stride = ylHeadBufInfo->data_stride;
	stIspFbcYLBuf->fbc_yl_head_wr_blen = ylHeadBufInfo->data_wr_blen;
	ISP_FBC_UV_BUF_S *stIspFbcUVBuf = &stIspFbcBuf->stIspFbcUVBuf;
	FBCD_BUF_S *uvDataBufInfo = &fbcInfo->uvDataBufInfo;
	stIspFbcUVBuf->fbc_uv_data_buf_base0 = uvDataBufInfo->data_buf_base0;
	stIspFbcUVBuf->fbc_uv_data_buf_base1 = uvDataBufInfo->data_buf_base1;
	stIspFbcUVBuf->fbc_uv_data_stride = uvDataBufInfo->data_stride;
	stIspFbcUVBuf->fbc_uv_data_wr_blen = uvDataBufInfo->data_wr_blen;
	FBCD_BUF_S *uvHeadBufInfo = &fbcInfo->uvHeadBufInfo;
	stIspFbcUVBuf->fbc_uv_head_buf_base0 = uvHeadBufInfo->data_buf_base0;
	stIspFbcUVBuf->fbc_uv_head_buf_base1 = uvHeadBufInfo->data_buf_base1;
	stIspFbcUVBuf->fbc_uv_head_stride = uvHeadBufInfo->data_stride;
	stIspFbcUVBuf->fbc_uv_head_wr_blen = uvHeadBufInfo->data_wr_blen;
    Isp_Drv_Fbc_SetBuf(isp,stIspFbcBuf);
	return 0;
}
//3DNR FBD
static int isp_f2k_core_SetFbd(struct k510_isp_device *isp,FBD_INFO_S *fbdInfo)
{
    ISP_FBD_ATTR_S  pstIspFbdAttr;
	ISP_FBD_CTL_S		*stIspFbdCtl = &pstIspFbdAttr.stIspFbdCtl;
	stIspFbdCtl->fbd_en = fbdInfo->fbd_en;
	stIspFbdCtl->fbd_format_cfg = fbdInfo->fbd_format_cfg;
    Isp_Drv_Fbd_SetCtl(isp,stIspFbdCtl);
	ISP_FBD_SIZE_S		*stIspFbdSize = &pstIspFbdAttr.stIspFbdSize;
	stIspFbdSize->fbd_input_width = fbdInfo->fbd_input_size.Width;
	stIspFbdSize->fbd_input_height = fbdInfo->fbd_input_size.Height;
    Isp_Drv_Fbd_SetSize(isp,stIspFbdSize);
	ISP_FBD_BUF_S		*stIspFbdBuf = &pstIspFbdAttr.stIspFbdBuf;
	ISP_FBD_Y_BUF_S  	*stIspFbdYBuf = &stIspFbdBuf->stIspFbdYBuf;
	FBCD_BUF_S *yDataBufInfo = &fbdInfo->yDataBufInfo;
	stIspFbdYBuf->fbd_y_data_buf_base0 = yDataBufInfo->data_buf_base0;
	stIspFbdYBuf->fbd_y_data_buf_base0 = yDataBufInfo->data_buf_base1;
	stIspFbdYBuf->fbd_y_data_stride = yDataBufInfo->data_stride;	
	FBCD_BUF_S *yHeadBufInfo = &fbdInfo->yHeadBufInfo;
	stIspFbdYBuf->fbd_y_head_buf_base0 = yHeadBufInfo->data_buf_base0;
	stIspFbdYBuf->fbd_y_head_buf_base1 = yHeadBufInfo->data_buf_base1;
	stIspFbdYBuf->fbd_y_head_stride = yHeadBufInfo->data_stride;
	ISP_FBD_YL_BUF_S 	*stIspFbdYLBuf = &stIspFbdBuf->stIspFbdYLBuf;
	FBCD_BUF_S  *ylDataBufInfo = &fbdInfo->ylDataBufInfo;
	stIspFbdYLBuf->fbd_yl_data_buf_base0 = ylDataBufInfo->data_buf_base0;
	stIspFbdYLBuf->fbd_yl_data_buf_base1 = ylDataBufInfo->data_buf_base1;
	stIspFbdYLBuf->fbd_yl_data_stride = ylDataBufInfo->data_stride;
	FBCD_BUF_S  *ylHeadBufInfo = &fbdInfo->ylHeadBufInfo;
	stIspFbdYLBuf->fbd_yl_head_buf_base0 = ylHeadBufInfo->data_buf_base0;
	stIspFbdYLBuf->fbd_yl_head_buf_base1 = ylHeadBufInfo->data_buf_base1;
	stIspFbdYLBuf->fbd_yl_head_stride = ylHeadBufInfo->data_stride;
	ISP_FBD_UV_BUF_S 	*stIspFbdUVBuf = &stIspFbdBuf->stIspFbdUVBuf; 
	FBCD_BUF_S  *uvDataBufInfo = &fbdInfo->uvDataBufInfo;
	stIspFbdUVBuf->fbd_uv_data_buf_base0 = uvDataBufInfo->data_buf_base0;
	stIspFbdUVBuf->fbd_uv_data_buf_base1 = uvDataBufInfo->data_buf_base1;
	stIspFbdUVBuf->fbd_uv_data_stride = uvDataBufInfo->data_stride;
	FBCD_BUF_S  *uvHeadBufInfo = &fbdInfo->uvHeadBufInfo;
	stIspFbdUVBuf->fbd_uv_head_buf_base0 = uvHeadBufInfo->data_buf_base0;
	stIspFbdUVBuf->fbd_uv_head_buf_base1 = uvHeadBufInfo->data_buf_base1;
	stIspFbdUVBuf->fbd_uv_head_stride = uvHeadBufInfo->data_stride;
    Isp_Drv_Fbd_SetBuf(isp,stIspFbdBuf);
	return 0;
}
//
static int isp_f2k_core_config(struct isp_f2k_device *f2k,struct isp_core_cfg_info *isp_core_cfg)
{

	struct k510_isp_device *isp = to_isp_device(f2k);
    //struct isp_core_cfg_info *isp_core_cfg =&isp_cfg->isp_core_cfg;
    //
	ITC_INFO_S	*itcInfo = &isp_core_cfg->itcInfo;
	isp_f2k_core_SetItc(isp,itcInfo);
    //
	isp_f2k_core_SetImage(isp,itcInfo);
    //
	TPG_INFO_S	*tpgInfo = &isp_core_cfg->tpgInfo;
	isp_f2k_core_SetTpgCtl(isp,tpgInfo);
    //
	BLC_INFO_S *blcInfo = &isp_core_cfg->blcInfo;
	isp_f2k_core_SetBlcCtl(isp,blcInfo);
    //
	LSC_INFO_S *lscInfo = &isp_core_cfg->lscInfo;
	isp_f2k_core_SetLscCtl(isp,lscInfo);
    //
	AE_INFO_S	*aeInfo = &isp_core_cfg->aeInfo;
	isp_f2k_core_SetAeCtl(isp,aeInfo);
	AWB_INFO_S  *awbInfo = &isp_core_cfg->awbInfo;
    //
	isp_f2k_core_SetAwbCtl(isp,awbInfo);
    //
	struct isp_core_wdr_Info *wdrInfo = &isp_core_cfg->wdrInfo;
	isp_f2k_core_SetWdrCtl(isp,wdrInfo);
    //
	CSC_INFO_S	*cscInfo = &isp_core_cfg->cscInfo;
	isp_f2k_core_SetCscCtl(isp,cscInfo);
    //
	ADA_INFO_S  *adaInfo = &isp_core_cfg->adaInfo;
	isp_f2k_core_SetAdaCtl(isp,adaInfo);
	RGBIR_INFO_S	*rgbirInfo = &isp_core_cfg->rgbirInfo;
    //
	isp_f2k_core_SetRgbirCtl(isp,rgbirInfo);
    //
	NR2D_INFO_S	*nr2dInfo = &isp_core_cfg->nr2dInfo;
	isp_f2k_core_Set2dnrCtl(isp,nr2dInfo);
    //
	NR3D_INFO_S	*nr3dInfo = &isp_core_cfg->nr3dInfo;
	isp_f2k_core_Set3dnrCtl(isp,nr3dInfo);
    //
	ENH_INFO_S *enhInfo= &isp_core_cfg->enhInfo;
	isp_f2k_core_SetEnhCtl(isp,enhInfo);
    //
	POST_INFO_S *postInfo= &isp_core_cfg->postInfo;
	isp_f2k_core_SetPostCtl(isp,postInfo);
    //
	OTC_INFO_S *otcInfo= &isp_core_cfg->otcInfo;
	isp_f2k_core_SetOtcCtl(isp,otcInfo);
    //
	LDC_INFO_S *ldcInfo= &isp_core_cfg->ldcInfo;
	isp_f2k_core_SetLdcCtl(isp,ldcInfo);
    //
	AF_INFO_S	*afInfo= &isp_core_cfg->afInfo;
	isp_f2k_core_SetAfCtl(isp,afInfo);
    //
	FBC_INFO_S *fbcInfo = &isp_core_cfg->fbcInfo;
	isp_f2k_core_SetFbc(isp,fbcInfo);
    //
	FBD_INFO_S *fbdInfo = &isp_core_cfg->fbdInfo;
	isp_f2k_core_SetFbd(isp,fbdInfo);
    return 0;
}
/****************************************************************************************
 *ISP DS
****************************************************************************************/
static void isp_f2k_ds_calc_scale(IMAGE_SIZE *in_size,IMAGE_SIZE *ds_out_size,ISP_S_DS_ATTR_S *stDsAttr)
{
    unsigned int InputWidth = in_size->Width;
    unsigned int  InputHeight = in_size->Height;
    unsigned int  OutputWidth = ds_out_size->Width;
    unsigned int  OutputHeight = ds_out_size->Height;
	//printk("%s:InputWidth(%d),InputHeight(%d),OutputWidth(%d),OutputHeight(%d)\n",__func__,InputWidth,InputHeight,OutputWidth,OutputHeight);
    stDsAttr->hscalePram.hscale_pstep = (InputWidth / OutputWidth);
    stDsAttr->hscalePram.hscale_dstep = ((InputWidth % OutputWidth) * 65536 / OutputWidth);
    stDsAttr->vscalePram.vscale_pstep = (InputHeight / OutputHeight);
    stDsAttr->vscalePram.vscale_dstep = ((InputHeight % OutputHeight) * 65536 / OutputHeight);
	//printk("%s:hscale_pstep(0x%x),hscale_dstep(0x%x),vscale_pstep(0x%x),vscale_dstep(0x%x)\n",__func__,stDsAttr->hscalePram.hscale_pstep,stDsAttr->hscalePram.hscale_dstep,stDsAttr->vscalePram.vscale_pstep,stDsAttr->vscalePram.vscale_dstep);
    return;
}
//
void isp_f2k_ds_SetInputSize(struct k510_isp_device *isp,IMAGE_SIZE *dsInSizeInfo)
{
    IMAGE_SIZE inputsize;
    inputsize.Width = dsInSizeInfo->Width - 1;
    inputsize.Height = dsInSizeInfo->Height - 1;
    Isp_Drv_F2k_Ds_SetInputSize(isp,&inputsize);
}
//
void isp_f2k_ds_SetRgb2YuvCoff(struct k510_isp_device *isp)
{    
    unsigned int osd_rgb2yuv_coeff[3][4];
    osd_rgb2yuv_coeff[0][0]= 0x00000132;
    osd_rgb2yuv_coeff[0][1]= 0x00000259;
    osd_rgb2yuv_coeff[0][2]= 0x00000075;
    osd_rgb2yuv_coeff[0][3]= 0x00000000;
    osd_rgb2yuv_coeff[1][0]= 0x00000f50;
    osd_rgb2yuv_coeff[1][1]= 0x00000ea5;
    osd_rgb2yuv_coeff[1][2]= 0x0000020b;
    osd_rgb2yuv_coeff[1][3]= 0x00000080;
    osd_rgb2yuv_coeff[2][0]= 0x0000020b;
    osd_rgb2yuv_coeff[2][1]= 0x00000e4a;
    osd_rgb2yuv_coeff[2][2]= 0x00000fab;
    osd_rgb2yuv_coeff[2][3]= 0x00000080;
    Isp_Drv_F2k_Ds_SetRgb2YuvCoff(isp,&osd_rgb2yuv_coeff[0][0]);
}
//
void isp_f2k_ds_SetYuv2RgbCoff(struct k510_isp_device *isp)
{    
    unsigned int osd_yuv2rgb_coeff[3][4];
  	osd_yuv2rgb_coeff[0][0] = 0x00000400;
	osd_yuv2rgb_coeff[0][1] = 0x00000000;
	osd_yuv2rgb_coeff[0][2] = 0x000005a1;
	osd_yuv2rgb_coeff[0][3] = 0x00000f4c;
	osd_yuv2rgb_coeff[1][0] = 0x00000400;
	osd_yuv2rgb_coeff[1][1] = 0x00000e9e;
	osd_yuv2rgb_coeff[1][2] = 0x00000d22;
	osd_yuv2rgb_coeff[1][3] = 0x00000088;
	osd_yuv2rgb_coeff[2][0] = 0x00000400;
	osd_yuv2rgb_coeff[2][1] = 0x0000071e;
	osd_yuv2rgb_coeff[2][2] = 0x00000000;
	osd_yuv2rgb_coeff[2][3] = 0x00000f1c;
    Isp_Drv_F2k_Ds_SetYuv2RgbCoff(isp,&osd_yuv2rgb_coeff[0][0]);
}
//
void isp_f2k_ds_SetOSD(ISP_DS_OSD_ATTR_S *stDsOsdAttr,ISP_OSD_INFO_S *osdInfo)
{
    ISP_DS_OSD_INFO_S *stOsdInfo = &stDsOsdAttr->OsdInfo;
	stOsdInfo->osd_enable = osdInfo->osd_enable;
	stOsdInfo->osd_type = osdInfo->osd_type;
	stOsdInfo->osd_alpha_tpye = osdInfo->osd_alpha_tpye;
    IMAGE_SIZE *stOsdSize = &stDsOsdAttr->OsdSize;
	stOsdSize->Width = osdInfo->osd_size.Width;
	stOsdSize->Height = osdInfo->osd_size.Height;
    ISP_DS_OSD_BUF_S *stOsdBuf   = &stDsOsdAttr->OsdBuf;
	stOsdBuf->osd_rgb_addr0 = osdInfo->osd_rgb_addr[0];
	stOsdBuf->osd_rgb_addr1 =  osdInfo->osd_rgb_addr[1];
	stOsdBuf->osd_stride = osdInfo->osd_stride;
	stOsdBuf->osd_alpha_addr0 = osdInfo->osd_alpha_addr[0];
	stOsdBuf->osd_alpha_addr1 = osdInfo->osd_alpha_addr[1];
	stOsdBuf->osd_alpha_stride = osdInfo->osd_alpha_stride;
	stOsdBuf->osd_position_start_x = osdInfo->osd_position_win.h_start;
	stOsdBuf->osd_position_stop_x = osdInfo->osd_position_win.h_end;
	stOsdBuf->osd_position_start_y =osdInfo->osd_position_win.v_start;
	stOsdBuf->osd_position_stop_y = osdInfo->osd_position_win.v_end;
    ISP_DS_OSD_DMA_CTL_S *stOsdDmaCtl = &stDsOsdAttr->OsdDmaCtl;
	stOsdDmaCtl->osd_bd_limit_en = osdInfo->osd_bd_limit_en;
	stOsdDmaCtl->osd_dma_map = osdInfo->osd_dma_map;
	stOsdDmaCtl->osd_dma_request_length = osdInfo->osd_dma_request_length;
	stOsdDmaCtl->osd_global_alpha = osdInfo->osd_global_alpha;
	stOsdDmaCtl->osd_outstanding_num = osdInfo->osd_outstanding_num;
	stOsdDmaCtl->osd_rgb_rev = osdInfo->osd_rgb_rev;
	stOsdDmaCtl->osd_swap_64 = osdInfo->osd_swap_64;
    return;
}
//
void isp_f2k_ds_SetSingleDS(struct k510_isp_device *isp,unsigned int u8Index,struct isp_ds_cfg_info	*isp_ds_cfg)
{
    IMAGE_SIZE in_size;
    in_size.Width = isp_ds_cfg->dsInSizeInfo.Width;
    in_size.Height = isp_ds_cfg->dsInSizeInfo.Height;
    ISP_S_DS_ATTR_S stDsAttr;
    IMAGE_SIZE *dsOutSize = &stDsAttr.dsOutSize;
    ISP_DS_INFO_S *dsInfo = &isp_ds_cfg->dsInfo[u8Index];
    dsOutSize->Width = dsInfo->ds_out_size.Width - 1;
    dsOutSize->Height = dsInfo->ds_out_size.Height - 1;
    isp_f2k_ds_calc_scale(&in_size,&dsInfo->ds_out_size,&stDsAttr);
    stDsAttr.vscalePram.scale_en = dsInfo->scale_en;
    stDsAttr.vscalePram.vscale_filter_en = dsInfo->vscale_filter_en;
    stDsAttr.hscalePram.hscale_filter_en = dsInfo->hscale_filter_en;
    ISP_DS_FORMAT_S *stDsFormat = &stDsAttr.DsFormat;
    stDsFormat->out_rgb_en =  dsInfo->out_rgb_en;
    stDsFormat->out_rgb_mode =  dsInfo->out_rgb_mode;
    stDsFormat->out_yuv_mode =  dsInfo->out_yuv_mode;
    stDsFormat->out_uv_swap =  dsInfo->out_uv_swap;
    unsigned int osdIndex = 0;     
    ISP_DS_OSD_ATTR_S *stDsOsdAttr = &stDsAttr.DsOsdAttr[osdIndex];
    ISP_OSD_INFO_S *osdInfo = &dsInfo->osdInfo[osdIndex];
    isp_f2k_ds_SetOSD(stDsOsdAttr,osdInfo);
    osdIndex = 1;     
    stDsOsdAttr = &stDsAttr.DsOsdAttr[osdIndex];
    osdInfo = &dsInfo->osdInfo[osdIndex];
    isp_f2k_ds_SetOSD(stDsOsdAttr,osdInfo);
    osdIndex = 2;     
    stDsOsdAttr = &stDsAttr.DsOsdAttr[osdIndex];
    osdInfo = &dsInfo->osdInfo[osdIndex];
    isp_f2k_ds_SetOSD(stDsOsdAttr,osdInfo);
    Isp_Drv_F2k_Ds_SetSingleDS(isp,u8Index,&stDsAttr);
    return;
}
//
static int isp_f2k_ds_config(struct isp_f2k_device *f2k,struct isp_ds_cfg_info *isp_ds_cfg)
{
	struct k510_isp_device *isp = to_isp_device(f2k);
    //
    Isp_Drv_F2k_Ds_SetscaleCoeff(isp);
    //
    IMAGE_SIZE *dsInSizeInfo = &isp_ds_cfg->dsInSizeInfo;
    isp_f2k_ds_SetInputSize(isp,dsInSizeInfo);
    //
    isp_f2k_ds_SetRgb2YuvCoff(isp);
    //
    isp_f2k_ds_SetYuv2RgbCoff(isp);
    //
    unsigned int u8Index = 0;
    isp_f2k_ds_SetSingleDS(isp,u8Index,isp_ds_cfg);
    //
    u8Index = 1;
    isp_f2k_ds_SetSingleDS(isp,u8Index,isp_ds_cfg);
    //
    u8Index = 2;
    isp_f2k_ds_SetSingleDS(isp,u8Index,isp_ds_cfg);

    return 0;
}
/****************************************************************************
*
*****************************************************************************/
void isp_f2k_config(struct k510_isp_device *isp,struct isp_cfg_info *isp_cfg)
{
    struct isp_f2k_device *f2k = &isp->isp_f2k;
	struct isp_wrap_cfg_info *isp_wrap_cfg = &isp_cfg->isp_wrap_cfg;
	struct isp_core_cfg_info *isp_core_cfg = &isp_cfg->isp_core_cfg;
	struct isp_ds_cfg_info *isp_ds_cfg =&isp_cfg->isp_ds_cfg;
	struct isp_nr3d_dma nr3d_dma_old,nr3d_dma_new;
    dev_dbg(isp->dev,"%s:start\n",__func__);
	if( 1 == isp_wrap_cfg->nr3dInfo.nr3d_en )
	{
		nr3d_dma_new.dma_size = 2048*2048*20/8;
		nr3d_dma_new.addr = dma_alloc_coherent(isp->dev, nr3d_dma_new.dma_size,
						  &nr3d_dma_new.dma,
						  GFP_KERNEL);
		if (nr3d_dma_new.addr == NULL)
			return -ENOMEM;
		
		nr3d_dma_old = isp->isp_f2k.nr3d_dma;
		isp->isp_f2k.nr3d_dma = nr3d_dma_new;

		if (nr3d_dma_old.addr != NULL)
			dma_free_coherent(isp->dev,nr3d_dma_new.dma_size,
					  nr3d_dma_old.addr, nr3d_dma_old.dma);
		//
		isp_wrap_cfg->nr3dInfo.nr3d_y_line_stride = (isp_core_cfg->itcInfo.itc_size.Width + 15)/16*16*12/8;//0xb40;
		isp_wrap_cfg->nr3dInfo.nr3d_uv_line_stride = (isp_core_cfg->itcInfo.itc_size.Width + 15)/16*16;//0x780;
		isp_wrap_cfg->nr3dInfo.nr3d_y_buf_base = nr3d_dma_new.dma;
		isp_wrap_cfg->nr3dInfo.nr3d_uv_buf_base = nr3d_dma_new.dma + isp_wrap_cfg->nr3dInfo.nr3d_y_line_stride * isp_core_cfg->itcInfo.itc_size.Height;	
	}
	//
    isp_f2k_core_config(f2k,isp_core_cfg);
	//
    isp_f2k_ds_config(f2k,isp_ds_cfg);
	//
	dev_dbg(isp->dev,"%s:main_y_buf0_base(0x%x)\n",__func__,isp_wrap_cfg->mainInfo.main_y_buf0_base);
    isp_f2k_wrap_config(f2k,isp_wrap_cfg);

    //printk("isp_f2k_config end \n");
    return 0;
}

/*
 * isp_f2k_set_fbc_outaddr - Set memory address to save output image
 * @f2k: Pointer to ISP f2k device.
 * @addr: ISP MMU Mapped 32-bit memory address aligned on 32 byte boundary.
 *
 * Sets the memory address where the output will be saved.
 */
static void isp_f2k_set_fbc_outaddr(struct isp_f2k_device *f2k, u32 addr)
{
	struct k510_isp_device *isp = to_isp_device(f2k);

	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_MAIN_Y_BUF0_BASE);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_MAIN_Y_BUF1_BASE);

	addr = addr+1920*1080;
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_MAIN_UV_BUF0_BASE);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_MAIN_UV_BUF1_BASE);
	isp_reg_writel(isp,0x30001,ISP_IOMEM_F2K_WRAP,ISP_WRAP_CONFIG_DONE);	
}

/*
 * isp_f2k_set_main_outaddr - Set memory address to save output image
 * @f2k: Pointer to ISP f2k device.
 * @addr: ISP MMU Mapped 32-bit memory address aligned on 32 byte boundary.
 *
 * Sets the memory address where the output will be saved.
 */
static void isp_f2k_set_main_outaddr(struct isp_f2k_device *f2k, u32 addr)
{
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct isp_wrap_cfg_info *isp_wrap_cfg = &f2k->isp_cfg.isp_wrap_cfg;
	struct isp_wrap_main_info *main_info = &isp_wrap_cfg->mainInfo;
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_MAIN_Y_BUF0_BASE);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_MAIN_Y_BUF1_BASE);
	//dev_dbg(f2k->isp->dev,"%s:line_stride (%d),Width(%d),Height(%d)\n",\
	__func__,main_info->main_line_stride,main_info->main_size.Width,main_info->main_size.Height);
	addr = addr + main_info->main_line_stride*main_info->main_size.Height;//1920*1080;
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_MAIN_UV_BUF0_BASE);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_MAIN_UV_BUF1_BASE);	
	isp_reg_writel(isp,0x30001,ISP_IOMEM_F2K_WRAP,ISP_WRAP_CONFIG_DONE);
}

/*
 * isp_f2k_set_out0_outaddr - Set memory address to save output image
 * @f2k: Pointer to ISP f2k device.
 * @addr: ISP MMU Mapped 32-bit memory address aligned on 32 byte boundary.
 *
 * Sets the memory address where the output will be saved.
 */
static void isp_f2k_set_out0_outaddr(struct isp_f2k_device *f2k, u32 addr)
{
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct isp_wrap_cfg_info *isp_wrap_cfg = &f2k->isp_cfg.isp_wrap_cfg;
	struct isp_wrap_ds0_info *ds0_info = &isp_wrap_cfg->ds0Info;
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS0_Y_BUF0_BASE);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS0_Y_BUF1_BASE);
	//dev_dbg(f2k->isp->dev,"%s:line_stride (%d),Width(%d),Height(%d)\n",\
	__func__,ds0_info->ds0_line_stride,ds0_info->ds0_size.Width,ds0_info->ds0_size.Height);
	addr = addr + ds0_info->ds0_line_stride*ds0_info->ds0_size.Height;//1920*1080;
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS0_UV_BUF0_BASE);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS0_UV_BUF1_BASE);	
	isp_reg_writel(isp,0x30001,ISP_IOMEM_F2K_WRAP,ISP_WRAP_CONFIG_DONE); //fix split screen
}
/*
 * isp_f2k_set_out1_outaddr - Set memory address to save output image
 * @f2k: Pointer to ISP f2k device.
 * @addr: ISP MMU Mapped 32-bit memory address aligned on 32 byte boundary.
 *
 * Sets the memory address where the output will be saved.
 */
static void isp_f2k_set_out1_outaddr(struct isp_f2k_device *f2k, u32 addr)
{
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct isp_wrap_cfg_info *isp_wrap_cfg = &f2k->isp_cfg.isp_wrap_cfg;
	struct isp_wrap_ds1_info *ds1_info = &isp_wrap_cfg->ds1Info;

	//dev_dbg(f2k->isp->dev,"%s:line_stride (%d),Width(%d),Height(%d) addr(0x%x)\n",\
	__func__,ds1_info->ds1_line_stride,ds1_info->ds1_size.Width,ds1_info->ds1_size.Height,addr);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS1_Y_BUF0_BASE);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS1_Y_BUF1_BASE);

	addr = addr + ds1_info->ds1_line_stride*ds1_info->ds1_size.Height;//1920*1080;
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS1_UV_BUF0_BASE);
	isp_reg_writel(isp,addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS1_UV_BUF1_BASE);
	isp_reg_writel(isp,0x30001,ISP_IOMEM_F2K_WRAP,ISP_WRAP_CONFIG_DONE);	
}

/*
 * isp_f2k_set_out2_outaddr - Set memory address to save output image
 * @f2k: Pointer to ISP f2k device.
 * @addr: ISP MMU Mapped 32-bit memory address aligned on 32 byte boundary.
 *
 * Sets the memory address where the output will be saved.
 */
static void isp_f2k_set_out2_outaddr(struct isp_f2k_device *f2k, u32 addr)
{
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct isp_wrap_cfg_info *isp_wrap_cfg = &f2k->isp_cfg.isp_wrap_cfg;
	struct isp_wrap_ds2_info *ds2_info = &isp_wrap_cfg->ds2Info;
	u32 r_addr,g_addr,b_addr;
	unsigned int height = ds2_info->ds2_video_height;//ds2_info->ds2_size.Height
	//
	//dev_dbg(f2k->isp->dev,"%s:line_stride (%d),Width(%d),Height(%d) addr(0x%x)\n",\
	__func__,ds2_info->ds2_line_stride,ds2_info->ds2_size.Width,height,addr);
	r_addr = addr;		
	isp_reg_writel(isp,r_addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS2_R_BUF0_BASE);
	isp_reg_writel(isp,r_addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS2_R_BUF1_BASE);

	if( DS2_S_RGB == ds2_info->ds2_out_img_out_format)
		g_addr = r_addr + ds2_info->ds2_line_stride * height;
	else	
		g_addr = r_addr + ds2_info->ds2_line_stride/4*2;
	isp_reg_writel(isp,g_addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS2_G_BUF0_BASE);
	isp_reg_writel(isp,g_addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS2_G_BUF1_BASE);
	//
	if( DS2_S_RGB == ds2_info->ds2_out_img_out_format)
		b_addr = g_addr + ds2_info->ds2_line_stride * height;
	else	
		b_addr = g_addr + ds2_info->ds2_line_stride/4*3;
	isp_reg_writel(isp,b_addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS2_B_BUF0_BASE);
	isp_reg_writel(isp,b_addr,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DS2_B_BUF1_BASE);
	isp_reg_writel(isp,0x30001,ISP_IOMEM_F2K_WRAP,ISP_WRAP_CONFIG_DONE);
}

static void isp_f2k_enable(struct isp_f2k_device *f2k)
{
	struct k510_isp_device *isp = to_isp_device(f2k);
}
/*
 * isp_f2k_wrap_print_status - Prints the values of the f2k wrap module registers.
 */
#define ISP_F2K_WRAP_PRINT_REGISTER(isp, name)\
	dev_info(isp->dev, "###WRAP: " #name"(0x%x) = 0x%x\n", \
		isp->mmio_base[ISP_IOMEM_F2K_WRAP] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp, ISP_IOMEM_F2K_WRAP, name))


static void isp_f2k_wrap_print_status(struct isp_f2k_device *f2k)
{

	struct k510_isp_device *isp = to_isp_device(f2k);

	dev_info(isp->dev, "-------------ISP F2K WRAP Register dump start----------\n");
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_SWRST_CTL     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_SWRST_CTL     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_CTL_MODE     			);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_PIPE_CLK_CTL     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_EN_CTL     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_3DNR_WDR_FORMAT_CTL   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_PIXEL_FORMAT_CTL     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_MAIN_OUT_SIZE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS0_OUT_SIZE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS1_OUT_SIZE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_OUT_SIZE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_BUF_SIZE_MASK0    	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_BUF_SIZE_MASK1    	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_BUF_SIZE_MASK2    	);

	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_ARB_MODE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_WEIGHT_0     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_WEIGHT_1     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_RD_WEIGHT_0     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_RD_WEIGHT_1     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_PRIORITY_0     );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_RD_PRIORITY_0     );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_DMA_WR_CH_ID_0     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_DMA_RD_CH_ID_0     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_WEIGHT_2     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_WEIGHT_3     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_RD_WEIGHT_2     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_RD_WEIGHT_3     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_PRIORITY_1     );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_RD_PRIORITY_1     );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_DMA_WR_CH_ID_1     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_DMA_RD_CH_ID_1     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_WDR_LONG_BUF_BASE     );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_WDR_LONG_SIZE_CFG     );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_3DNR_Y_BUF_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_3DNR_UV_BUF_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_3DNR_STRIDE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_LDC_Y_BUF_BASE    	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_LDC_UV_BUF_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_LDC_STRIDE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_MAIN_Y_BUF0_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_MAIN_Y_BUF1_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_MAIN_UV_BUF0_BASE     );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_MAIN_UV_BUF1_BASE     );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_MAIN_STRIDE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS0_Y_BUF0_BASE    	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS0_Y_BUF1_BASE    	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS0_UV_BUF0_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS0_UV_BUF1_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS0_STRIDE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS1_Y_BUF0_BASE    	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS1_Y_BUF1_BASE    	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS1_UV_BUF0_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS1_UV_BUF1_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS1_STRIDE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_R_BUF0_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_R_BUF1_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_G_BUF0_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_G_BUF1_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_B_BUF0_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_B_BUF1_BASE     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_STRIDE0    		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DS2_STRIDE1    		);

	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH0_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH1_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH2_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH3_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH4_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH5_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH6_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH7_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH8_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH9_CFG     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH10_CFG    		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH11_CFG    		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH12_CFG    		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH13_CFG    		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH14_CFG    		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH15_CFG    		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH0_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH1_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH2_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH3_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH4_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH5_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH6_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH7_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH8_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH9_ERR_DEC_CFG   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH10_ERR_DEC_CFG  );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH11_ERR_DEC_CFG  );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH12_ERR_DEC_CFG  );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH13_ERR_DEC_CFG  );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH14_ERR_DEC_CFG  );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH15_ERR_DEC_CFG  );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH0_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH1_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH2_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH3_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH4_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH5_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH6_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH7_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH8_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH9_ERR_STATUS    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH10_ERR_STATUS   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH11_ERR_STATUS   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH12_ERR_STATUS   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH13_ERR_STATUS   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH14_ERR_STATUS   );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_CH15_ERR_STATUS   );

	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_CONFIG_DONE     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_CORE_INT_CTL     		);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_INT_STATUS0    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_INT_MASK0     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_INT_STATUS1    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_WR_INT_MASK1     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_RD_INT_STATUS0    );
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_DMA_RD_INT_MASK0     	);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_WRAP_AXI_CTL     			);

	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_VERSION     				);
	ISP_F2K_WRAP_PRINT_REGISTER(isp,ISP_RELEASE_INFO     			);
	dev_info(isp->dev, "-------------ISP F2K WRAP Register dump end----------\n");
}
/*-----------------------------------------------------------------------------
*CORE
-----------------------------------------------------------------------------*/
/*
 * isp_f2k_core_print_status - Prints the values of the isp f2k core module registers.
 */
#define ISP_F2K_CORE_PRINT_REGISTER(isp, name)\
	dev_info(isp->dev, "###CORE:" #name "(0x%x) = 0x%x\n", \
		isp->mmio_base[ISP_IOMEM_F2K_CORE] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp, ISP_IOMEM_F2K_CORE, name))


static void isp_f2k_core_print_status(struct isp_f2k_device *f2k)
{

	struct k510_isp_device *isp = to_isp_device(f2k);

	dev_info(isp->dev, "-------------ISP F2K CORE Register dump start----------\n");
	//itc
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ITC_CTL               		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_IMAGE_HEIGHT          		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_IMAGE_WIDTH           		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_IMAGE_V_START         		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_IMAGE_H_START         		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_IMAGE_ACTIVE_WIDTH    		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_IMAGE_ACTIVE_HEIGHT   		);
	//tpg	                                                        
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_TEST_CTL              		);
	//blc		                                                    
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_BLC_CTL               		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_BLC_OFFSET            		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_BLC_RATIO             		);
	//lsc		                                                    
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LSC_CTL               		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LSC_H_CENTER          		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LSC_V_CENTER          		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LSC_R_RATIO           		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LSC_G_RATIO           		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LSC_B_RATIO           		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LSC_IR_RATIO          		);
	//ae		                                                    
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_CTL                		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_WIN_H_START        		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_WIN_V_START        		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_WIN_H_END          		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_WIN_V_END          		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_TAR_BR             		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_TAR_BR_RANGE       		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_L_EX_TIME          		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_M_EX_TIME          		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_S_EX_TIME          		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_AGC                		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_ADJUST_CTL         		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_ADJUST_STEP_MAX    		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_EX_VALUE_MAX       		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_EX_VALUE_MID       		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_EX_VALUE_MIN       		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_GAIN_MAX           		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_GAIN_MID           		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_GAIN_MIN           		);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_DN_SWITCH_ADJUST_STEP_MAX);
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_DN_SWITCH_WAIT_TIME     );
																	
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_APE_DIFF_MAX               );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_APE_DRV_SIGNAL_MAX         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_APE_COEFF_DIS              );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_APE_COEFF_SPEED            );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_APE_COEFF_ACCE             );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_APE_DRV_MANUAL_VALUE       );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_APE_DAMP_MANUAL_VALUE      );
																
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_VALUE_READY             );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_LONG_CUR_EX             );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_MID_CUR_EX              );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_SHORT_CUR_EX            );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_CUR_DIGITAL_GAIN        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_CUR_AVE_BRIGHTNESS      );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_CUR_DN_STATUS           );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_EX_STATUS               );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_SUM                     );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AE_PIXEL_SUM               );
	//awb                                                          
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_CTL                    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_D65_RED_GAIN           );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_D65_BLUE_GAIN          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_RR_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_RG_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_RB_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_GR_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_GG_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_GB_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_BR_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_BG_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_BB_COFF                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CCM_CORRECT_COFF           );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_WIN_H_START            );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_WIN_V_START            );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_WIN_H_END              );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_WIN_V_END              );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_CORRECT_DIFF_TH        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_RES_TIME               );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_HIST_TH                );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_RED_GAIN_ADJUST        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_GREEN_GAIN_ADJUST      );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BLUE_GAIN_ADJUST       );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_RED_MAX_VALUE          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BLUE_MAX_VALUE         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_RED_MIN_VALUE          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BLUE_MIN_VALUE         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_RED_OBJ_VALUE          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BLUE_OBJ_VALUE         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BLUE_HIST_VALUE        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BLUE_HIST_PIXEL        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_RED_HIST_VALUE         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_RED_HIST_PIXEL         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BYPASS_BLUE_HIST_VALUE );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BYPASS_BLUE_HIST_PIXEL );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BYPASS_RED_HIST_VALUE  );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BYPASS_RED_HIST_PIXEL  );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_RED_VALUE              );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_GREEN_VALUE            );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_BLUE_VALUE             );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_ORG_RED_VALUE          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_ORG_GREEN_VALUE        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AWB_ORG_BLUE_VALUE         );
	//wdr                                                           
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_WDR_CTL                    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_WDR_OVER_EX_RATIO_TH1      );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_WDR_OVER_EX_RATIO_TH2      );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_WDR_FUSION_RATIO_TH        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_WDR_FUSION_VALUE1          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_WDR_FUSION_VALUE2          );
	//csc                                                          
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_00                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_01                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_02                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_10                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_11                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_12                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_20                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_21                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CSC_R2Y_22                 );
	//ada                                                           
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ADA_CTL                    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ADA_STAT_MAX_VALUE         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ADA_AD_STREN_MAX_VALUE     );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ADA_WIN_H_START            );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ADA_WIN_V_START            );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ADA_WIN_H_END              );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ADA_WIN_V_END              );
	//rgbir                                                         
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RGBIR_CTL                  );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RGBIR_RPC_MAX_VALUE        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RGBIR_RPC_COLOR_COFF       );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RGBIR_RPC_LUMA_COFF        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RGBIR_RPC_TH               );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RGBIR_RPC_TH1              );
	//2dnr                                                         
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_2DNR_CTL                   );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_2DNR_RAW_INTENSITY         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_2DNR_BAP_INTENSITY         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_2DNR_EDGE_INTENSITY        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_2DNR_LUMA_INTENSITY        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_2DNR_CHROMA_INTENSITY      );
	//3dnr                                                          
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_CTL                   );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_PRE_LUMA_TH           );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_PRE_LUMA_INTENSITY    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_PRE_CHROMA_INTENSITY  );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_MID_FILTER_TH         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_PRE_MID_FILTER_TH     );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_CUR_FILTER_TH         );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_LOW_PASS_FILTER_TH    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_LUMA_TH               );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_MIN_VALUE             );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_LUMA_INTENSITY        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_CHROMA_INTENSITY      );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_POST_EDGE_TH          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_POST_LUMA_INTENSITY   );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_3DNR_POST_CHROMA_INTENSITY );
	//enh                                                          
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ENH_CTL                    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LTM_GAIN                   );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LTM_TH                     );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ENH_NR_TH                  );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ENH_TH1                    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_ENH_TH2                    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_SHARP_GAIN                 );
	//post                                                          
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_POST_CTL                   );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_CONT_GAIN                  );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LUMA_GAIN                  );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_SATU_GAIN                  );
	//otc                                                          
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_OTC_STT_VR                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_OTC_STT_HR                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_OTC_HEIGHT                 );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_OTC_WIDTH                  );
	//ldc                                                          
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LDC_CTL                    );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LDC_REQ_FREQ               );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LDC_H_CENTER_POS           );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LDC_V_CENTER_POS           );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LDC_RECTIFY_CR             );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_LDC_RECTIFY_CZ             );
	//ram table                                                     
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RAM_WR_STATUS              );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RAM_RD_STATUS              );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_RAM_READ_LOCK              );
	//af                                                            
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AF_CTL                     );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AF_STAT_WIN_H_START        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AF_STAT_WIN_V_START        );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AF_STAT_WIN_H_END          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AF_STAT_WIN_V_END          );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AF_MID_FRQ_DATA            );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AF_HIGH_FRQ_DATA           );
	ISP_F2K_CORE_PRINT_REGISTER(isp,ISP_CORE_AF_STAT_PIXEL_NUM          );

	dev_info(isp->dev, "-------------ISP F2K CORE Register dump end----------\n");
}
/*--------------------------------------------------------------------------
*DS
--------------------------------------------------------------------------*/
/*
 * isp_f2k_ds_print_status - Prints the values of the isp f2k ds module registers.
 */
#define ISP_F2K_DS_PRINT_REGISTER(isp, name)\
	dev_info(isp->dev, "###DS: " #name "(0x%x) = 0x%x\n", \
		isp->mmio_base[ISP_IOMEM_F2K_DS] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp, ISP_IOMEM_F2K_DS, name))


static void isp_f2k_ds_print_status(struct isp_f2k_device *f2k)
{
	struct k510_isp_device *isp = to_isp_device(f2k);

	dev_info(isp->dev, "-------------isp f2k ds Register dump start----------\n");
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_INPUT_SIZE);
	//RGB2YUV
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF00);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF01);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF02);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF03);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF10);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF11);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF12);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF13);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF20);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF21);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF22);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_RGB2YUV_COEFF23);
	//YUV2RGB
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF00);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF01);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF02);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF03);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF10);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF11);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF12);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF13);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF20);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF21);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF22);
	ISP_F2K_DS_PRINT_REGISTER(isp,ISP_DS_OSD_YUV2RGB_COEFF23);
	//DS0
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_VSCALE_PARM);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_HSCALE_PARM);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_SIZE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_FORMAT     );
	//DS0 OSD0
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_INFO     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_SIZE     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_X_RANGE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD0_Y_RANGE     	);
	//DS0 OSD1
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_INFO     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_SIZE     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_X_RANGE    	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD1_Y_RANGE    	);
	//DS0 OSD2
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_INFO     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_SIZE     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_X_RANGE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS0_OSD2_Y_RANGE     	);
	//DS1
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_VSCALE_PARM);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_HSCALE_PARM);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_SIZE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_FORMAT     );
	//DS1 0SD0
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_INFO     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_SIZE     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_X_RANGE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD0_Y_RANGE     	);
	//DS1 0SD1
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_INFO     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_SIZE     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_X_RANGE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD1_Y_RANGE     	);
	//DS1 0SD2
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_INFO     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_SIZE     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_X_RANGE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS1_OSD2_Y_RANGE     	);
	//DS2
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_VSCALE_PARM);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_HSCALE_PARM);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_SIZE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_FORMAT     );
	//DS2 OSD0
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_INFO    		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_SIZE    		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_X_RANGE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD0_Y_RANGE     	);
	//DS2 OSD1
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_INFO    		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_SIZE    		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_X_RANGE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD1_Y_RANGE     	);
	//DS2 OSD2
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_INFO     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_SIZE     		);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_BUF_ADDR0     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_ALPHA_BUF_ADDR0);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_BUF_ADDR1     );
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_ALPHA_BUF_ADDR1);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_DMA_CTL     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_STRIDE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_X_RANGE     	);
	ISP_F2K_DS_PRINT_REGISTER(isp, ISP_DS2_OSD2_Y_RANGE     	);

	dev_info(isp->dev, "-------------isp f2k ds Register dump end----------\n");
}
/*--------------------------------------------------------------------------
*3DNR FBC
--------------------------------------------------------------------------*/
/*
 * isp_3dnr_mfbc_print_status - Prints the values of the isp 3dnr mfbc module registers.
 */
#define ISP_3DNR_MFBC_PRINT_REGISTER(isp, name)\
	dev_dbg(isp->dev, "###3DNR_FBC: " #name "= 0x%x\n", \
		isp_reg_readl(isp, ISP_IOMEM_F2K_FBC, name))


static void isp_3dnr_mfbc_print_status(struct isp_f2k_device *f2k)
{
	struct k510_isp_device *isp = to_isp_device(f2k);

	dev_dbg(isp->dev, "-------------ISP 3DNR MFBC Register dump start----------\n");
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_INPUT_SIZE     		);
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_OUT_FORMAT     		);
																	
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_Y_DATA_BUF_BASE0    	);
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_Y_DATA_BUF_BASE1    	);
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_Y_DATA_LINE_STRIDE    );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_Y_DATA_WR_BLEN     	);
																	
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_Y_HEAD_BUF_BASE0     	);
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_Y_HEAD_BUF_BASE1     	);
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_Y_HEAD_LINE_STRIDE    );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_Y_HEAD_WR_BLEN     	);
																	
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_UV_DATA_BUF_BASE0     );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_UV_DATA_BUF_BASE1     );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_UV_DATA_LINE_STRIDE   );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_UV_DATA_WR_BLEN       );
																	
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_UV_HEAD_BUF_BASE0     );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_UV_HEAD_BUF_BASE1     );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_UV_HEAD_LINE_STRIDE   );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_UV_HEAD_WR_BLEN     	);
																	
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_YL_DATA_BUF_BASE0     );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_YL_DATA_BUF_BASE1     );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_YL_DATA_LINE_STRIDE   );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_YL_DATA_WR_BLEN       );
																	
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_YL_HEAD_BUF_BASE0     );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_YL_HEAD_BUF_BASE1     );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_YL_HEAD_LINE_STRIDE   );
	ISP_3DNR_MFBC_PRINT_REGISTER(isp,ISP_FBC_YL_HEAD_WR_BLEN       );

    dev_dbg(isp->dev, "-------------ISP 3DNR MFBC Register dump end----------\n");
}
/*--------------------------------------------------------------------------
*3DNR MFBD
--------------------------------------------------------------------------*/
/*
 * isp_3dnr_mfbd_print_status - Prints the values of the 3dnr mfbd module registers.
 */
#define ISP_3DNR_MFBD_PRINT_REGISTER(isp, name)\
	dev_dbg(isp->dev, "###3DNR_FBD: " #name "= 0x%x\n", \
		isp_reg_readl(isp, ISP_IOMEM_F2K_FBD, name))


static void isp_3dnr_mfbd_print_status(struct isp_f2k_device *f2k)
{
	struct k510_isp_device *isp = to_isp_device(f2k);

	dev_dbg(isp->dev, "-------------ISP 3DNR MFBD Register dump start----------\n");
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_CTL     				);
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_INPUT_SIZE     		);

	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_Y_DATA_BUF_BASE0     	);
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_Y_DATA_BUF_BASE1     	);
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_Y_DATA_LINE_STRIDE    );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_Y_HEAD_BUF_BASE0     	);
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_Y_HEAD_BUF_BASE1     	);
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_Y_HEAD_LINE_STRIDE    );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_UV_DATA_BUF_BASE0     );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_UV_DATA_BUF_BASE1     );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_UV_DATA_LINE_STRIDE   );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_UV_HEAD_BUF_BASE0     );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_UV_HEAD_BUF_BASE1     );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_UV_HEAD_LINE_STRIDE   );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_YL_DATA_BUF_BASE0     );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_YL_DATA_BUF_BASE1     );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_YL_DATA_LINE_STRIDE   );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_YL_HEAD_BUF_BASE0     );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_YL_HEAD_BUF_BASE1     );
	ISP_3DNR_MFBD_PRINT_REGISTER(isp,ISP_FBD_YL_HEAD_LINE_STRIDE   );

    dev_dbg(isp->dev, "-------------ISP 3DNR MFBD Register dump end----------\n");
}
/*--------------------------------------------------------------------------
*REMAP
--------------------------------------------------------------------------*/
static int isp_f2k_remap_main_config(struct isp_f2k_device *f2k,struct isp_remap_cfg_info *isp_remap_cfg)
{
	struct k510_isp_device *isp = to_isp_device(f2k);

	ISP_DRV_LINE_DRAW_U DrawAreaNum;
	ISP_REMAP_ATTR_S  stRemapCtl;
	Isp_Drv_F2k_Remap_SetMainLine(isp,DrawAreaNum,&stRemapCtl);
	return 0;
}

static int isp_f2k_remap_out0_config(struct isp_f2k_device *f2k,struct isp_remap_cfg_info *isp_remap_cfg)
{
	struct k510_isp_device *isp = to_isp_device(f2k);

	ISP_DRV_LINE_DRAW_U DrawAreaNum;
	ISP_REMAP_ATTR_S  stRemapCtl;
	Isp_Drv_F2k_Remap_SetOut0Line(isp,DrawAreaNum,&stRemapCtl);

	return 0;
}

static int isp_f2k_remap_out1_config(struct isp_f2k_device *f2k,struct isp_remap_cfg_info *isp_remap_cfg)
{
	struct k510_isp_device *isp = to_isp_device(f2k);

	ISP_DRV_LINE_DRAW_U DrawAreaNum;
	ISP_REMAP_ATTR_S  stRemapCtl;
	Isp_Drv_F2k_Remap_SetOut1Line(isp,DrawAreaNum,&stRemapCtl);

	return 0;
}

/*
 * isp_f2k_remap_print_status - Prints the values of the f2k remap module registers.
 */
#define ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp, name)\
	dev_dbg(isp->dev, "###MAIN_REMAP: " #name "= 0x%x\n", \
		isp_reg_readl(isp, ISP_IOMEM_F2K_MAIN_REMAP, name))
#define ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp, name)\
	dev_dbg(isp->dev, "###OUT0_REMAP: " #name "= 0x%x\n", \
		isp_reg_readl(isp, ISP_IOMEM_F2K_OUT0_REMAP, name))
#define ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp, name)\
	dev_dbg(isp->dev, "###OUT1_REMAP:" #name "= 0x%x\n", \
		isp_reg_readl(isp, ISP_IOMEM_F2K_OUT1_REMAP, name))

static void isp_f2k_remap_print_status(struct isp_f2k_device *f2k)
{
	struct k510_isp_device *isp = to_isp_device(f2k);
    //
	dev_dbg(isp->dev, "-------------ISP F2K MAIN REMAP Register dump start----------\n");
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_00_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_00_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_01_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_01_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_02_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_02_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_03_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_03_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_04_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_04_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_05_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_05_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_06_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_06_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_07_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_07_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_08_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_08_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_09_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_09_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_10_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_10_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_11_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_11_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_12_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_12_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_13_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_13_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_14_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_14_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_15_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_15_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_16_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_16_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_17_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_17_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_18_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_18_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_19_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_19_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_20_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_20_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_21_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_21_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_22_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_22_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_23_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_23_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_24_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_24_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_25_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_25_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_26_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_26_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_27_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_27_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_28_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_28_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_29_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_29_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_30_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_30_1_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_31_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_31_1_CTRL);
						
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_00_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_01_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_02_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_03_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_04_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_05_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_06_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_07_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_08_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_09_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_10_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_11_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_12_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_13_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_14_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_15_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_16_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_17_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_18_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_19_0_CTRL);
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_20_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_21_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_22_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_23_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_24_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_25_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_26_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_27_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_28_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_29_0_CTRL);    
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_30_0_CTRL); 
	ISP_F2K_MAIN_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_31_0_CTRL); 
    dev_dbg(isp->dev, "-------------ISP F2K MAIN REMAP Register dump end----------\n");
    //
	dev_dbg(isp->dev, "-------------ISP F2K OUT0 REMAP Register dump start----------\n");
		ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_00_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_00_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_01_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_01_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_02_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_02_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_03_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_03_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_04_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_04_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_05_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_05_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_06_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_06_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_07_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_07_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_08_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_08_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_09_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_09_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_10_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_10_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_11_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_11_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_12_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_12_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_13_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_13_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_14_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_14_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_15_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_15_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_16_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_16_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_17_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_17_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_18_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_18_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_19_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_19_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_20_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_20_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_21_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_21_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_22_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_22_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_23_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_23_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_24_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_24_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_25_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_25_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_26_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_26_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_27_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_27_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_28_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_28_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_29_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_29_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_30_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_30_1_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_31_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_31_1_CTRL);
						
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_00_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_01_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_02_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_03_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_04_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_05_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_06_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_07_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_08_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_09_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_10_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_11_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_12_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_13_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_14_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_15_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_16_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_17_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_18_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_19_0_CTRL);
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_20_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_21_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_22_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_23_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_24_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_25_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_26_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_27_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_28_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_29_0_CTRL);    
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_30_0_CTRL); 
	ISP_F2K_OUT0_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_31_0_CTRL); 
    dev_dbg(isp->dev, "-------------ISP F2K OUT0 REMAP Register dump end----------\n");
    //
	dev_dbg(isp->dev, "-------------ISP F2K OUT1 REMAP Register dump start----------\n");
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_00_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_00_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_01_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_01_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_02_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_02_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_03_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_03_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_04_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_04_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_05_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_05_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_06_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_06_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_07_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_07_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_08_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_08_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_09_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_09_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_10_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_10_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_11_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_11_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_12_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_12_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_13_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_13_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_14_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_14_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_15_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_15_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_16_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_16_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_17_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_17_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_18_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_18_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_19_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_19_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_20_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_20_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_21_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_21_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_22_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_22_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_23_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_23_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_24_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_24_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_25_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_25_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_26_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_26_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_27_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_27_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_28_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_28_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_29_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_29_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_30_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_30_1_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_31_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_INFO_AREA_31_1_CTRL);
						
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_00_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_01_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_02_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_03_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_04_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_05_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_06_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_07_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_08_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_09_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_10_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_11_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_12_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_13_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_14_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_15_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_16_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_17_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_18_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_19_0_CTRL);
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_20_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_21_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_22_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_23_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_24_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_25_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_26_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_27_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_28_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_29_0_CTRL);    
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_30_0_CTRL); 
	ISP_F2K_OUT1_REMAP_PRINT_REGISTER(isp,ISP_FILL_DATA_AREA_31_0_CTRL); 
    dev_dbg(isp->dev, "-------------ISP F2K OUT1 REMAP Register dump end----------\n");
}
/* -----------------------------------------------------------------------------
 * Interrupt handling
 -----------------------------------------------------------------------------*/
static void f2k_isr_main_buffer(struct isp_f2k_device *f2k)
{
	struct k510isp_pipeline *pipe = to_isp_pipeline(&f2k->subdev.entity);
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct k510isp_buffer *buffer;
	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	if (list_empty(&f2k->video_out[MAIN_VIDEO].dmaqueue))
	{
		//dev_err(f2k->isp->dev,"%s:list_empty\n",__func__);
		return 0;
	}	

	if (f2k->state == ISP_PIPELINE_STREAM_CONTINUOUS && f2k->underrun) {
		dev_err(f2k->isp->dev,"%s:f2k->state(0x%x)\n",__func__,f2k->state);
		f2k->underrun = 0;
		return 1;
	}

	buffer = k510isp_video_buffer_next(&f2k->video_out[MAIN_VIDEO]);
	if (buffer != NULL)
	{
		//dev_dbg(f2k->isp->dev,"%s:buffer->dma(0x%x)\n",__func__,buffer->dma);
		isp_f2k_set_main_outaddr(f2k, buffer->dma);
	}

	pipe->state |= ISP_PIPELINE_IDLE_OUTPUT;

	return buffer != NULL;
}

static void f2k_isr_out0_buffer(struct isp_f2k_device *f2k)
{
	struct k510isp_pipeline *pipe = to_isp_pipeline(&f2k->subdev.entity);
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct k510isp_buffer *buffer;

	if (list_empty(&f2k->video_out[DS0_VIDEO].dmaqueue))
	{
		//dev_dbg(f2k->isp->dev,"%s:list_empty\n",__func__);
		return 0;
	}	

	if (f2k->state == ISP_PIPELINE_STREAM_CONTINUOUS && f2k->underrun) {
		f2k->underrun = 0;
		return 1;
	}

	buffer = k510isp_video_buffer_next(&f2k->video_out[DS0_VIDEO]);
	if (buffer != NULL)
		isp_f2k_set_out0_outaddr(f2k, buffer->dma);

	pipe->state |= ISP_PIPELINE_IDLE_OUTPUT;

	return buffer != NULL;
}

static void f2k_isr_out1_buffer(struct isp_f2k_device *f2k)
{
	struct k510isp_pipeline *pipe = to_isp_pipeline(&f2k->subdev.entity);
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct k510isp_buffer *buffer;

	if (list_empty(&f2k->video_out[DS1_VIDEO].dmaqueue))
	{
		//dev_dbg(f2k->isp->dev,"%s:list_empty\n",__func__);
		return 0;
	}	

	if (f2k->state == ISP_PIPELINE_STREAM_CONTINUOUS && f2k->underrun) {
		f2k->underrun = 0;
		return 1;
	}

	buffer = k510isp_video_buffer_next(&f2k->video_out[DS1_VIDEO]);
	if (buffer != NULL)
		isp_f2k_set_out1_outaddr(f2k, buffer->dma);

	pipe->state |= ISP_PIPELINE_IDLE_OUTPUT;

	return buffer != NULL;
}

static void f2k_isr_out2_buffer(struct isp_f2k_device *f2k)
{
	struct k510isp_pipeline *pipe = to_isp_pipeline(&f2k->subdev.entity);
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct k510isp_buffer *buffer;

	if (list_empty(&f2k->video_out[DS2_VIDEO].dmaqueue))
	{
		//dev_dbg(f2k->isp->dev,"%s:list_empty\n",__func__);
		return 0;
	}	

	if (f2k->state == ISP_PIPELINE_STREAM_CONTINUOUS && f2k->underrun) {
		f2k->underrun = 0;
		return 1;
	}

	buffer = k510isp_video_buffer_next(&f2k->video_out[DS2_VIDEO]);
	if (buffer != NULL)
		isp_f2k_set_out2_outaddr(f2k, buffer->dma);

	pipe->state |= ISP_PIPELINE_IDLE_OUTPUT;

	return buffer != NULL;
}
/*
 * f2k_isr - Handle ISP F2K interrupts
 * @isp_f2k: Pointer to ISP F2K device
 *
 * This will handle the F2K interrupts
 */
int k510isp_f2k_main_isr(struct isp_f2k_device *f2k,u32 events)
{
	dev_dbg(f2k->isp->dev,"%s:events(0x%x)start\n",__func__,events);
	if (f2k->state == ISP_PIPELINE_STREAM_STOPPED)
		return 0;

	/* Handle queued buffers on frame end interrupts */
	//if(events & (IRQW0_STS_MAIN_Y_FRAME_IRQ|IRQW0_STS_MAIN_UV_FRAME_IRQ))
	f2k_isr_main_buffer(f2k);

	return 0;
}

int k510isp_f2k_ds0_isr(struct isp_f2k_device *f2k,u32 events)
{
	dev_dbg(f2k->isp->dev,"%s:events(0x%x)start\n",__func__,events);
	if (f2k->state == ISP_PIPELINE_STREAM_STOPPED)
		return 0;

	/* Handle queued buffers on frame end interrupts */
	//if(events & (IRQW1_STS_OUT0_Y_FRAME_IRQ|IRQW1_STS_OUT0_UV_FRAME_IRQ))
	f2k_isr_out0_buffer(f2k);
		
	return 0;
}

int k510isp_f2k_ds1_isr(struct isp_f2k_device *f2k,u32 events)
{
	dev_dbg(f2k->isp->dev,"%s:events(0x%x)start\n",__func__,events);
	if (f2k->state == ISP_PIPELINE_STREAM_STOPPED)
		return 0;

	/* Handle queued buffers on frame end interrupts */
	//if(events & (IRQW1_STS_OUT1_Y_FRAME_IRQ|IRQW1_STS_OUT1_UV_FRAME_IRQ))
	f2k_isr_out1_buffer(f2k);
		
	return 0;
}

int k510isp_f2k_ds2_isr(struct isp_f2k_device *f2k,u32 events)
{
	dev_dbg(f2k->isp->dev,"%s:events(0x%x) state(%d) start\n",__func__,events,f2k->state);
	if (f2k->state == ISP_PIPELINE_STREAM_STOPPED)
		return 0;

	/* Handle queued buffers on frame end interrupts */
	//if(events & (IRQW1_STS_OUT2_R_FRAME_IRQ|IRQW1_STS_OUT2_G_FRAME_IRQ|IRQW1_STS_OUT2_B_FRAME_IRQ))
	f2k_isr_out2_buffer(f2k);
		
	return 0;
}
/* -----------------------------------------------------------------------------
 * ISP F2K video device node
 *-----------------------------------------------------------------------------/
/*
 * f2k_video_queue - Queue video buffer.
 * @video : Pointer to isp video structure
 * @buffer: Pointer to isp_buffer structure
 * return -EIO or zero on success
 */
static int f2k_video_queue(struct k510isp_video *video, struct k510isp_buffer *buffer)
{
	struct isp_f2k_device *f2k = &video->isp->isp_f2k;
	unsigned long flags;
	bool restart = false;
	dev_dbg(video->isp->dev,"%s:f2k->output(0x%x) start\n",__func__,f2k->output);
	if (f2k->output == ISP_F2K_OUTPUT_NONE)
	{
		dev_err(video->isp->dev,"%s:no mem dev\n",__func__);
		return -ENODEV;
	}
	if (f2k->output & ISP_F2K_OUTPUT_FBC)
	{
		isp_f2k_set_fbc_outaddr(f2k, buffer->dma);
	}
	if (f2k->output & ISP_F2K_OUTPUT_MAIN_MEM)
	{
		//dev_dbg(video->isp->dev,"%s:buffer->dma(0x%x)\n",__func__,buffer->dma);
		isp_f2k_set_main_outaddr(f2k, buffer->dma);
	}
	if (f2k->output & ISP_F2K_OUTPUT_DS0_MEM)
	{
		isp_f2k_set_out0_outaddr(f2k, buffer->dma);
	}
	if (f2k->output & ISP_F2K_OUTPUT_DS1_MEM)
	{
		isp_f2k_set_out1_outaddr(f2k, buffer->dma);
	}
	if (f2k->output & ISP_F2K_OUTPUT_DS2_MEM)
	{
		isp_f2k_set_out2_outaddr(f2k, buffer->dma);
	}


	/* We now have a buffer queued on the output, restart the pipeline
	 * on the next f2k interrupt if running in continuous mode (or when
	 * starting the stream) in external sync mode, or immediately in BT.656
	 * sync mode as no f2k interrupt is generated when the f2k is stopped
	 * in that case.
	 */
	spin_lock_irqsave(&f2k->lock, flags);
	if (f2k->state == ISP_PIPELINE_STREAM_CONTINUOUS && !f2k->running )
		restart = true;
	else
		f2k->underrun = 1;
	spin_unlock_irqrestore(&f2k->lock, flags);

	if (restart)
		isp_f2k_enable(f2k);

	return 0;
}

static const struct k510isp_video_operations f2k_video_ops = {
	.queue = f2k_video_queue,
};
/* --------------------------------------------------------------------------
 * V4L2 subdev operations
 --------------------------------------------------------------------------*/
int k510isp_f2k_reset(struct k510_isp_device *isp)
{
	isp_f2k_wrap_reset(isp);
	reset_control_reset(isp->reset[FBC_RST]);
	reset_control_reset(isp->reset[ISP_F2K_RST]);	
	return 0;
}
/*
 * f2k_ioctl - f2k module private ioctl's
 * @sd: ISP f2k V4L2 subdevice
 * @cmd: ioctl command
 * @arg: ioctl argument
 *
 * Return 0 on success or a negative error code otherwise.
 */
static long f2k_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	int ret;	
	dev_dbg(f2k->isp->dev,"%s:cmd(0x%x)\n",__func__,cmd);
	struct isp_wrap_cfg_info *isp_wrap_cfg = &f2k->isp_cfg.isp_wrap_cfg;
	struct isp_core_cfg_info *isp_core_cfg = &f2k->isp_cfg.isp_core_cfg;
	struct isp_ds_cfg_info	 *isp_ds_cfg   = &f2k->isp_cfg.isp_ds_cfg;

	switch (cmd) {
	case VIDIOC_K510ISP_F2K_WRAP_CFG:
		mutex_lock(&f2k->ioctl_lock);
		//ret = isp_f2k_wrap_config(f2k, arg);
		memcpy(isp_wrap_cfg,arg,sizeof(struct isp_wrap_cfg_info));
		//dev_dbg(f2k->isp->dev,"%s:main_y_buf0_base(0x%x)\n",__func__,isp_wrap_cfg->mainInfo.main_y_buf0_base);
		mutex_unlock(&f2k->ioctl_lock);
		break;
	case VIDIOC_K510ISP_F2K_CORE_CFG:
		mutex_lock(&f2k->ioctl_lock);
		memcpy((void*)isp_core_cfg,arg,sizeof(struct isp_core_cfg_info));
		ret = isp_f2k_core_config(f2k, arg);
		mutex_unlock(&f2k->ioctl_lock);
		break;
	case VIDIOC_K510ISP_F2K_DS_CFG:
		mutex_lock(&f2k->ioctl_lock);
		//ret = isp_f2k_ds_config(f2k, arg);
		memcpy((void*)isp_ds_cfg,arg,sizeof(struct isp_ds_cfg_info));
		mutex_unlock(&f2k->ioctl_lock);
		break;
	case VIDIOC_K510ISP_F2K_REMAP_MAIN_CFG:
		mutex_lock(&f2k->ioctl_lock);
		ret = isp_f2k_remap_main_config(f2k, arg);
		mutex_unlock(&f2k->ioctl_lock);
		break;
	case VIDIOC_K510ISP_F2K_REMAP_OUT0_CFG:
		mutex_lock(&f2k->ioctl_lock);
		ret = isp_f2k_remap_out0_config(f2k, arg);
		mutex_unlock(&f2k->ioctl_lock);
		break;
	case VIDIOC_K510ISP_F2K_REMAP_OUT1_CFG:
		mutex_lock(&f2k->ioctl_lock);
		ret = isp_f2k_remap_out1_config(f2k, arg);
		mutex_unlock(&f2k->ioctl_lock);
		break; 
	case VIDIOC_K510ISP_F2K_AE_STAT_REQ:
		mutex_lock(&f2k->ioctl_lock);
		isp_f2k_core_GetAeSts(f2k->isp,arg);
		mutex_unlock(&f2k->ioctl_lock);
		break;
	case VIDIOC_K510ISP_F2K_CFG_SET:
		mutex_lock(&f2k->ioctl_lock);
		isp_f2k_config(f2k->isp,&f2k->isp_cfg);
		mutex_unlock(&f2k->ioctl_lock);
		break;
	case VIDIOC_K510ISP_F2K_CORE_CFG_GET:
		mutex_lock(&f2k->ioctl_lock);
		
		mutex_unlock(&f2k->ioctl_lock);
		break;
	case VIDIOC_K510ISP_SYSCTL_RST_F2K:
		mutex_lock(&f2k->ioctl_lock);
		reset_control_reset(f2k->isp->reset[ISP_F2K_RST]);
		reset_control_reset(f2k->isp->reset[FBC_RST]);		
		mutex_unlock(&f2k->ioctl_lock);
		break;
	default:
		dev_err(f2k->isp->dev,"%s:cmd(0x%x) err!\n",__func__,cmd);
		return -ENOIOCTLCMD;
	}

	return 0;
}

static int f2k_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				struct v4l2_event_subscription *sub)
{
	if (sub->type != V4L2_EVENT_FRAME_SYNC)
		return -EINVAL;

	/* line number is zero at frame start */
	if (sub->id != 0)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, ISP_F2K_NEVENTS, NULL);
}

static int f2k_unsubscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	return v4l2_event_unsubscribe(fh, sub);
}
/* --------------------------------------------------------------------------
 * f2k video device node
 *--------------------------------------------------------------------------/

/*
 * f2k_s_stream - Enable/Disable streaming on f2k subdev
 * @sd    : pointer to v4l2 subdev structure
 * @enable: 1 == Enable, 0 == Disable
 * return zero
 */
static int f2k_set_stream(struct v4l2_subdev *sd, int enable)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	struct k510_isp_device *isp = to_isp_device(f2k);
	struct device *dev = to_device(f2k);
	int ret;
	dev_dbg(f2k->isp->dev,"%s:enable(0x%d)\n",__func__,enable);

	struct isp_cfg_info *isp_cfg = &f2k->isp_cfg;
	struct isp_irq_info irq_info;  

	if (f2k->state == ISP_PIPELINE_STREAM_STOPPED) {
		if (enable == ISP_PIPELINE_STREAM_STOPPED)
		{
			memset(&irq_info,0,sizeof(struct isp_irq_info));
			mutex_lock(&f2k->ioctl_lock);
			k510isp_f2k_irq_enable(isp,&irq_info);
			k510isp_f2k_reset(isp);
			mutex_unlock(&f2k->ioctl_lock);
			return 0;
		}	
		atomic_set(&f2k->stopping, 0);
	}

	switch (enable) {
	case ISP_PIPELINE_STREAM_CONTINUOUS:
		mutex_lock(&f2k->ioctl_lock);
		isp_f2k_config(isp,isp_cfg);
		mutex_unlock(&f2k->ioctl_lock);
		dev_dbg(f2k->isp->dev,"%s:main_out_en(0x%x)\n",__func__,isp_cfg->isp_wrap_cfg.mainInfo.main_out_en);
		dev_dbg(f2k->isp->dev,"%s:ds0_out_en(0x%x)\n",__func__,isp_cfg->isp_wrap_cfg.ds0Info.ds0_out_en);
		dev_dbg(f2k->isp->dev,"%s:ds1_out_en(0x%x)\n",__func__,isp_cfg->isp_wrap_cfg.ds1Info.ds1_out_en);
		dev_dbg(f2k->isp->dev,"%s:ds2_out_en(0x%x)\n",__func__,isp_cfg->isp_wrap_cfg.ds2Info.ds2_out_en);
		memset(&irq_info,0,sizeof(struct isp_irq_info));
		irq_info.main_dma_en = isp_cfg->isp_wrap_cfg.mainInfo.main_out_en;
		irq_info.ds0_en = isp_cfg->isp_wrap_cfg.ds0Info.ds0_out_en;
		irq_info.ds1_en = isp_cfg->isp_wrap_cfg.ds1Info.ds1_out_en;
		irq_info.ds2_en = isp_cfg->isp_wrap_cfg.ds2Info.ds2_out_en;
		//irq_info.ae_en = 1;
		//irq_info.awb_en = 1;
		//irq_info.af_en = 1;
		k510isp_f2k_irq_enable(isp,&irq_info);
		unsigned int intmask0 = isp_reg_readl(isp,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DMA_WR_INT_MASK0);
		unsigned int intmask1 = isp_reg_readl(isp,ISP_IOMEM_F2K_WRAP,ISP_WRAP_DMA_WR_INT_MASK1);
		dev_dbg(f2k->isp->dev,"%s:intmask0(0x%x),intmask1(0x%x)\n",__func__,intmask0,intmask1);
		break;

	case ISP_PIPELINE_STREAM_STOPPED:
		//if (isp_module_sync_idle(&sd->entity, &f2k->wait,
		//			      &f2k->stopping))
		//	dev_dbg(dev, "%s: module stop timeout.\n", sd->name);
		//if (f2k->input == ISP_F2K_INPUT_VI) {
		//	f2k_if_enable(f2k, 0);
		//}
		memset(&irq_info,0,sizeof(struct isp_irq_info));
		mutex_lock(&f2k->ioctl_lock);
		k510isp_f2k_irq_enable(isp,&irq_info);
		k510isp_f2k_reset(isp);
		mutex_unlock(&f2k->ioctl_lock);
		break;
	}

	f2k->state = enable;
	return 0;
}

static const unsigned int f2k_fmts[] = {
	MEDIA_BUS_FMT_SGRBG8_1X8,
	MEDIA_BUS_FMT_SRGGB8_1X8,
	MEDIA_BUS_FMT_SBGGR8_1X8,
	MEDIA_BUS_FMT_SGBRG8_1X8,
	MEDIA_BUS_FMT_SGRBG10_1X10,
	MEDIA_BUS_FMT_SRGGB10_1X10,
	MEDIA_BUS_FMT_SBGGR10_1X10,
	MEDIA_BUS_FMT_SGBRG10_1X10,
	MEDIA_BUS_FMT_SGRBG12_1X12,
	MEDIA_BUS_FMT_SRGGB12_1X12,
	MEDIA_BUS_FMT_SBGGR12_1X12,
	MEDIA_BUS_FMT_SGBRG12_1X12,
	MEDIA_BUS_FMT_YUYV8_2X8,
	MEDIA_BUS_FMT_YVYU8_2X8,
	MEDIA_BUS_FMT_UYVY8_1X16,
	MEDIA_BUS_FMT_YUYV8_1X16,
	MEDIA_BUS_FMT_YVYU8_1X16,
	MEDIA_BUS_FMT_VYUY8_1X16,
	MEDIA_BUS_FMT_UYVY8_1_5X8,
	MEDIA_BUS_FMT_VYUY8_1_5X8,
	MEDIA_BUS_FMT_YUYV8_1_5X8,
	MEDIA_BUS_FMT_YVYU8_1_5X8,
	MEDIA_BUS_FMT_Y12_1X12,
	MEDIA_BUS_FMT_RBG888_1X24,
	MEDIA_BUS_FMT_BGR888_1X24,
	MEDIA_BUS_FMT_GBR888_1X24,
	MEDIA_BUS_FMT_RGB888_1X24,
	MEDIA_BUS_FMT_ARGB8888_1X32,
};

static struct v4l2_mbus_framefmt *
__f2k_get_format(struct isp_f2k_device *f2k, struct v4l2_subdev_pad_config *cfg,
		  unsigned int pad, enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return v4l2_subdev_get_try_format(&f2k->subdev, cfg, pad);
	else
		return &f2k->formats[pad];
}

static struct v4l2_rect *
__f2k_get_crop(struct isp_f2k_device *f2k, struct v4l2_subdev_pad_config *cfg,
		enum v4l2_subdev_format_whence which)
{
	if (which == V4L2_SUBDEV_FORMAT_TRY)
		return v4l2_subdev_get_try_crop(&f2k->subdev, cfg, ISP_F2K_PAD_MAIN_SOURCE);
	else
		return &f2k->crop_in;
}

/*
 * f2k_try_format - Try video format on a pad
 * @f2k: ISP F2K device
 * @cfg : V4L2 subdev pad configuration
 * @pad: Pad number
 * @fmt: Format
 */
static void
f2k_try_format(struct isp_f2k_device *f2k, struct v4l2_subdev_pad_config *cfg,
		unsigned int pad, struct v4l2_mbus_framefmt *fmt,
		enum v4l2_subdev_format_whence which)
{
	const struct isp_format_info *info;
	u32 pixelcode;
	unsigned int width = fmt->width;
	unsigned int height = fmt->height;
	struct v4l2_rect *crop;
	enum v4l2_field field;
	unsigned int i;
	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	switch (pad) {
	case ISP_F2K_PAD_SINK:
		for (i = 0; i < ARRAY_SIZE(f2k_fmts); i++) {
			if (fmt->code == f2k_fmts[i])
				break;
		}

		/* If not found, use SGRBG10 as default */
		if (i >= ARRAY_SIZE(f2k_fmts))
			fmt->code = MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;

		/* Clamp the input size. */
		fmt->width = clamp_t(u32, width, 32, 4096);
		fmt->height = clamp_t(u32, height, 32, 4096);

		/* Default to progressive field order. */
		if (fmt->field == V4L2_FIELD_ANY)
			fmt->field = V4L2_FIELD_NONE;

		break;

	case ISP_F2K_PAD_MAIN_SOURCE:
	case ISP_F2K_PAD_DS0_SOURCE:
	case ISP_F2K_PAD_DS1_SOURCE:
	case ISP_F2K_PAD_DS2_SOURCE:
		pixelcode = fmt->code;
		field = fmt->field;
		*fmt = *__f2k_get_format(f2k, cfg, ISP_F2K_PAD_SINK, which);

		/* In SYNC mode the bridge converts YUV formats from 2X8 to
		 * 1X16. In BT.656 no such conversion occurs. As we don't know
		 * at this point whether the source will use SYNC or BT.656 mode
		 * let's pretend the conversion always occurs. The f2k will be
		 * configured to pack bytes in BT.656, hiding the inaccuracy.
		 * In all cases bytes can be swapped.
		 */
		if (fmt->code == MEDIA_BUS_FMT_YUYV8_2X8 ||
		    fmt->code == MEDIA_BUS_FMT_UYVY8_2X8) {
			/* Use the user requested format if YUV. */
			if (pixelcode == MEDIA_BUS_FMT_YUYV8_2X8 ||
			    pixelcode == MEDIA_BUS_FMT_UYVY8_2X8 ||
			    pixelcode == MEDIA_BUS_FMT_YUYV8_1X16 ||
			    pixelcode == MEDIA_BUS_FMT_UYVY8_1X16)
				fmt->code = pixelcode;

			if (fmt->code == MEDIA_BUS_FMT_YUYV8_2X8)
				fmt->code = MEDIA_BUS_FMT_YUYV8_1X16;
			else if (fmt->code == MEDIA_BUS_FMT_UYVY8_2X8)
				fmt->code = MEDIA_BUS_FMT_UYVY8_1X16;
		}
	
		/* Hardcode the output size to the crop rectangle size. */
		crop = __f2k_get_crop(f2k, cfg, which);
		fmt->width = crop->width;
		fmt->height = crop->height;

		/* When input format is interlaced with alternating fields the
		 * F2K can interleave the fields.
		 */
		if (fmt->field == V4L2_FIELD_ALTERNATE &&
		    (field == V4L2_FIELD_INTERLACED_TB ||
		     field == V4L2_FIELD_INTERLACED_BT)) {
			fmt->field = field;
			fmt->height *= 2;
		}

		break;
    }
	/* Data is written to memory unpacked, each 10-bit or 12-bit pixel is
	 * stored on 2 bytes.
	 */
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
	dev_dbg(f2k->isp->dev,"%s:end\n",__func__);
}

/*
 * f2k_try_crop - Validate a crop rectangle
 * @f2k: ISP F2K device
 * @sink: format on the sink pad
 * @crop: crop rectangle to be validated
 */
static void f2k_try_crop(struct isp_f2k_device *f2k,
			  const struct v4l2_mbus_framefmt *sink,
			  struct v4l2_rect *crop)
{
	const struct k510isp_format_info *info;
	unsigned int max_width;
	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	/* For Bayer formats, restrict left/top and width/height to even values
	 * to keep the Bayer pattern.
	 */
	info = k510isp_video_format_info(sink->code);
	if (info->flavor != MEDIA_BUS_FMT_Y8_1X8) {
		crop->left &= ~1;
		crop->top &= ~1;
	}

	crop->left = clamp_t(u32, crop->left, 0, sink->width - F2K_MIN_WIDTH);
	crop->top = clamp_t(u32, crop->top, 0, sink->height - F2K_MIN_HEIGHT);

	/* The data formatter truncates the number of horizontal output pixels
	 * to a multiple of 16. To avoid clipping data, allow callers to request
	 * an output size bigger than the input size up to the nearest multiple
	 * of 16.
	 */
	max_width = (sink->width - crop->left + 15) & ~15;
	crop->width = clamp_t(u32, crop->width, F2K_MIN_WIDTH, max_width)
		    & ~15;
	crop->height = clamp_t(u32, crop->height, F2K_MIN_HEIGHT,
			       sink->height - crop->top);

	/* Odd width/height values don't make sense for Bayer formats. */
	if (info->flavor != MEDIA_BUS_FMT_Y8_1X8) {
		crop->width &= ~1;
		crop->height &= ~1;
	}
	dev_dbg(f2k->isp->dev,"%s:end\n",__func__);
}

/*
 * f2k_enum_mbus_code - Handle pixel format enumeration
 * @sd     : pointer to v4l2 subdev structure
 * @cfg : V4L2 subdev pad configuration
 * @code   : pointer to v4l2_subdev_mbus_code_enum structure
 * return -EINVAL or zero on success
 */
static int f2k_enum_mbus_code(struct v4l2_subdev *sd,
			       struct v4l2_subdev_pad_config *cfg,
			       struct v4l2_subdev_mbus_code_enum *code)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	switch (code->pad) {
	case ISP_F2K_PAD_SINK:
		if (code->index >= ARRAY_SIZE(f2k_fmts))
		{
			dev_err(f2k->isp->dev,"%s:code->index %d\n",__func__,code->index);	
			return -EINVAL;
		}
		code->code = f2k_fmts[code->index];
		break;

	case ISP_F2K_PAD_MAIN_SOURCE:
	case ISP_F2K_PAD_DS0_SOURCE:
	case ISP_F2K_PAD_DS1_SOURCE:
	case ISP_F2K_PAD_DS2_SOURCE:
		format = __f2k_get_format(f2k, cfg, code->pad,
					   code->which);

		if (format->code == MEDIA_BUS_FMT_YUYV8_2X8 ||
		    format->code == MEDIA_BUS_FMT_UYVY8_2X8) {
			/* In YUV mode the f2k can swap bytes. */
			if (code->index == 0)
				code->code = MEDIA_BUS_FMT_YUYV8_1X16;
			else if (code->index == 1)
				code->code = MEDIA_BUS_FMT_UYVY8_1X16;
			else
			{
				dev_err(f2k->isp->dev,"%s:code->index1 %d\n",__func__,code->index);	
				return -EINVAL;
			}
		} else {
			/* In raw mode, no configurable format confversion is
			 * available.
			 */
			if (code->index == 0)
				code->code = format->code;
			else
			{
				dev_err(f2k->isp->dev,"%s:code->index2 %d\n",__func__,code->index);	
				return -EINVAL;
			}
		}
		break;

	default:
		return -EINVAL;
	}
	dev_dbg(f2k->isp->dev,"%s:end\n",__func__);
	return 0;
}

static int f2k_enum_frame_size(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_frame_size_enum *fse)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt format;

	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	if (fse->index != 0)
	{
		dev_err(f2k->isp->dev,"%s:fse->index%d\n",__func__,fse->index);
		return -EINVAL;
	}	

	format.code = fse->code;
	format.width = 1;
	format.height = 1;
	f2k_try_format(f2k, cfg, fse->pad, &format, fse->which);
	fse->min_width = format.width;
	fse->min_height = format.height;

	if (format.code != fse->code)	
	{
		dev_err(f2k->isp->dev,"%s:format.code,fse->code %d\n",__func__,fse->code);
		return -EINVAL;
	}	

	format.code = fse->code;
	format.width = -1;
	format.height = -1;
	f2k_try_format(f2k, cfg, fse->pad, &format, fse->which);
	fse->max_width = format.width;
	fse->max_height = format.height;
	dev_dbg(f2k->isp->dev,"%s:end\n",__func__);
	return 0;
}
/*
 * f2k_get_selection - Retrieve a selection rectangle on a pad
 * @sd: ISP f2k V4L2 subdevice
 * @cfg: V4L2 subdev pad configuration
 * @sel: Selection rectangle
 *
 * The only supported rectangles are the crop rectangles on the output formatter
 * source pad.
 *
 * Return 0 on success or a negative error code otherwise.
 */
static int f2k_get_selection(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_selection *sel)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	if (sel->pad != ISP_F2K_PAD_MAIN_SOURCE ||
		sel->pad != ISP_F2K_PAD_DS0_SOURCE ||
		sel->pad != ISP_F2K_PAD_DS1_SOURCE ||
		sel->pad != ISP_F2K_PAD_DS2_SOURCE )
	{
		dev_err(f2k->isp->dev,"%s:no pad\n",__func__);
		return -EINVAL;
	}	

	switch (sel->target) {
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = INT_MAX;
		sel->r.height = INT_MAX;

		format = __f2k_get_format(f2k, cfg, ISP_F2K_PAD_SINK, sel->which);
		f2k_try_crop(f2k, format, &sel->r);
		break;

	case V4L2_SEL_TGT_CROP:
		sel->r = *__f2k_get_crop(f2k, cfg, sel->which);
		break;

	default:
		dev_err(f2k->isp->dev,"%s:no target\n",__func__);
		return -EINVAL;
	}
	dev_dbg(f2k->isp->dev,"%s:end\n",__func__);
	return 0;
}
/*
 * f2k_set_selection - Set a selection rectangle on a pad
 * @sd: ISP F2K V4L2 subdevice
 * @cfg: V4L2 subdev pad configuration
 * @sel: Selection rectangle
 *
 * The only supported rectangle is the actual crop rectangle on the output
 * formatter source pad.
 *
 * Return 0 on success or a negative error code otherwise.
 */
static int f2k_set_selection(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_selection *sel)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	if (sel->target != V4L2_SEL_TGT_CROP ||
	    sel->pad != ISP_F2K_PAD_MAIN_SOURCE ||
		sel->pad != ISP_F2K_PAD_DS0_SOURCE ||
		sel->pad != ISP_F2K_PAD_DS1_SOURCE ||
		sel->pad != ISP_F2K_PAD_DS2_SOURCE )
	{
		dev_err(f2k->isp->dev,"%s:pad == NULL\n",__func__);
		return -EINVAL;
	}		

	/* The crop rectangle can't be changed while streaming. */
	if (f2k->state != ISP_PIPELINE_STREAM_STOPPED)
	{
		dev_err(f2k->isp->dev,"%s:ISP_PIPELINE_STREAM_RUNNING\n",__func__);
		return -EBUSY;
	}	

	/* Modifying the crop rectangle always changes the format on the source
	 * pad. If the KEEP_CONFIG flag is set, just return the current crop
	 * rectangle.
	 */
	if (sel->flags & V4L2_SEL_FLAG_KEEP_CONFIG) {
		sel->r = *__f2k_get_crop(f2k, cfg, sel->which);
		dev_dbg(f2k->isp->dev,"%s:V4L2_SEL_FLAG_KEEP_CONFIG\n",__func__);
		return 0;
	}

	format = __f2k_get_format(f2k, cfg, ISP_F2K_PAD_SINK, sel->which);
	f2k_try_crop(f2k, format, &sel->r);
	*__f2k_get_crop(f2k, cfg, sel->which) = sel->r;

	/* Update the source format. */
	if( sel->pad == ISP_F2K_PAD_MAIN_SOURCE)
	{
		format = __f2k_get_format(f2k, cfg, sel->pad, sel->which);
		f2k_try_format(f2k, cfg, sel->pad, format, sel->which);
	}

	if( sel->pad == ISP_F2K_PAD_DS0_SOURCE)
	{
		format = __f2k_get_format(f2k, cfg, sel->pad, sel->which);
		f2k_try_format(f2k, cfg, sel->pad, format, sel->which);
	}

	if( sel->pad == ISP_F2K_PAD_DS1_SOURCE)
	{
		format = __f2k_get_format(f2k, cfg, sel->pad, sel->which);
		f2k_try_format(f2k, cfg, sel->pad, format, sel->which);
	}

	if( sel->pad == ISP_F2K_PAD_DS2_SOURCE)
	{
		format = __f2k_get_format(f2k, cfg, sel->pad, sel->which);
		f2k_try_format(f2k, cfg, sel->pad, format, sel->which);
	}

	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	return 0;
}
/*
 * f2k_get_format - Retrieve the video format on a pad
 * @sd : ISP f2k V4L2 subdevice
 * @cfg: V4L2 subdev pad configuration
 * @fmt: Format
 *
 * Return 0 on success or -EINVAL if the pad is invalid or doesn't correspond
 * to the format type.
 */
static int f2k_get_format(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *fmt)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;

	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	format = __f2k_get_format(f2k, cfg, fmt->pad, fmt->which);
	if (format == NULL)
	{
		dev_err(f2k->isp->dev,"%s:format == NULL\n",__func__);
		return -EINVAL;
	}

	fmt->format = *format;
	dev_dbg(f2k->isp->dev,"%s:end\n",__func__);
	return 0;
}
/*
 * f2k_set_format - Set the video format on a pad
 * @sd : ISP F2K V4L2 subdevice
 * @cfg: V4L2 subdev pad configuration
 * @fmt: Format
 *
 * Return 0 on success or -EINVAL if the pad is invalid or doesn't correspond
 * to the format type.
 */
static int f2k_set_format(struct v4l2_subdev *sd, struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_format *fmt)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *format;
	struct v4l2_rect *crop;

	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		return 0;
	}

	if (fmt->pad >ISP_F2K_PADS_NUM)
	{
		dev_warn(f2k->isp->dev, "%s:pad error!\n",__func__);
		return -EINVAL;
	}
	if (fmt->pad == ISP_F2K_PAD_SINK) {
		/* Reset the crop rectangle. */
		crop = &f2k->crop_in;
		crop->left = 0;
		crop->top = 0;
		crop->width = fmt->format.width;
		crop->height = fmt->format.height;
		//	
		format = &fmt->format;
		f2k->formats[ISP_F2K_PAD_SINK].width = format->width;
		f2k->formats[ISP_F2K_PAD_SINK].height = format->height;
		f2k->formats[ISP_F2K_PAD_SINK].field = V4L2_FIELD_NONE;
		f2k->formats[ISP_F2K_PAD_SINK].colorspace =V4L2_COLORSPACE_SRGB;
		f2k->formats[ISP_F2K_PAD_SINK].code = format->code;
	}

	if (fmt->pad == ISP_F2K_PAD_MAIN_SOURCE) {
		/* Reset the crop rectangle. */
		crop = &f2k->crop_out;
		crop->left = 0;
		crop->top = 0;
		crop->width = fmt->format.width;
		crop->height = fmt->format.height;

		format = &fmt->format;
		f2k->formats[ISP_F2K_PAD_MAIN_SOURCE].width = format->width;
		f2k->formats[ISP_F2K_PAD_MAIN_SOURCE].height = format->height;
		f2k->formats[ISP_F2K_PAD_MAIN_SOURCE].field = V4L2_FIELD_NONE;
		f2k->formats[ISP_F2K_PAD_MAIN_SOURCE].colorspace =V4L2_COLORSPACE_SRGB;
		f2k->formats[ISP_F2K_PAD_MAIN_SOURCE].code = format->code;	
	}

	if (fmt->pad == ISP_F2K_PAD_DS0_SOURCE) {

		format = &fmt->format;
		f2k->formats[ISP_F2K_PAD_DS0_SOURCE].width = format->width;
		f2k->formats[ISP_F2K_PAD_DS0_SOURCE].height = format->height;
		f2k->formats[ISP_F2K_PAD_DS0_SOURCE].field = V4L2_FIELD_NONE;
		f2k->formats[ISP_F2K_PAD_DS0_SOURCE].colorspace =V4L2_COLORSPACE_SRGB;
		f2k->formats[ISP_F2K_PAD_DS0_SOURCE].code = format->code;
	}

	if (fmt->pad == ISP_F2K_PAD_DS1_SOURCE) {
		format = &fmt->format;
		f2k->formats[ISP_F2K_PAD_DS1_SOURCE].width = format->width;
		f2k->formats[ISP_F2K_PAD_DS1_SOURCE].height = format->height;
		f2k->formats[ISP_F2K_PAD_DS1_SOURCE].field = V4L2_FIELD_NONE;
		f2k->formats[ISP_F2K_PAD_DS1_SOURCE].colorspace =V4L2_COLORSPACE_SRGB;
		f2k->formats[ISP_F2K_PAD_DS1_SOURCE].code = format->code;
	}

	if (fmt->pad == ISP_F2K_PAD_DS2_SOURCE) {

		format = &fmt->format;
		f2k->formats[ISP_F2K_PAD_DS2_SOURCE].width = format->width;
		f2k->formats[ISP_F2K_PAD_DS2_SOURCE].height = format->height;
		f2k->formats[ISP_F2K_PAD_DS2_SOURCE].field = V4L2_FIELD_NONE;
		f2k->formats[ISP_F2K_PAD_DS2_SOURCE].colorspace =V4L2_COLORSPACE_SRGB;
		f2k->formats[ISP_F2K_PAD_DS2_SOURCE].code = format->code;
	}

	dev_dbg(f2k->isp->dev,"%s:end\n",__func__);
	return 0;
}

static int f2k_link_validate(struct v4l2_subdev *sd,
			      struct media_link *link,
			      struct v4l2_subdev_format *source_fmt,
			      struct v4l2_subdev_format *sink_fmt)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);

	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	/* Check if the two ends match */
	if (source_fmt->format.width != sink_fmt->format.width ||
	    source_fmt->format.height != sink_fmt->format.height)
	{
		dev_err(f2k->isp->dev,"%s:format error\n",__func__);
		return -EPIPE;
	}	

	return 0;
}
/*
 * f2k_init_formats - Initialize formats on all pads
 * @sd: ISP F2K V4L2 subdevice
 * @fh: V4L2 subdev file handle
 *
 * Initialize all pad formats with default values. If fh is not NULL, try
 * formats are initialized on the file handle. Otherwise active formats are
 * initialized on the device.
 */
static int f2k_init_formats(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	struct v4l2_subdev_format format;
	int pad;

	for(pad = ISP_F2K_PAD_SINK;pad<ISP_F2K_PAD_DS2_SOURCE+1;pad++)
	{
		memset(&format, 0, sizeof(format));
		format.pad = pad;
		format.which = fh ? V4L2_SUBDEV_FORMAT_TRY : V4L2_SUBDEV_FORMAT_ACTIVE;
		format.format.code = f2k->formats[pad].code;//MEDIA_BUS_FMT_SRGGB10_1X10;//MEDIA_BUS_FMT_SGRBG10_1X10;
		format.format.width = f2k->formats[pad].width;//1920;//4096;
		format.format.height = f2k->formats[pad].height;//1080;//4096;
		f2k_set_format(sd, fh ? fh->pad : NULL, &format);		
	}

	return 0;
}
/* V4L2 subdev core operations */
static const struct v4l2_subdev_core_ops f2k_v4l2_core_ops = {
	.ioctl = f2k_ioctl,
	.subscribe_event = f2k_subscribe_event,
	.unsubscribe_event = f2k_unsubscribe_event,
};
/* subdev video operations */
static const struct v4l2_subdev_video_ops f2k_v4l2_video_ops = {
	.s_stream = f2k_set_stream,
};

/* subdev pad operations */
static const struct v4l2_subdev_pad_ops f2k_v4l2_pad_ops = {
	.enum_mbus_code = f2k_enum_mbus_code,
	.enum_frame_size = f2k_enum_frame_size,
	.get_fmt = f2k_get_format,
	.set_fmt = f2k_set_format,
	.get_selection = f2k_get_selection,
	.set_selection = f2k_set_selection,
	.link_validate = f2k_link_validate,
};

/* subdev operations */
static const struct v4l2_subdev_ops f2k_v4l2_ops = {
	.core = &f2k_v4l2_core_ops,
	.video = &f2k_v4l2_video_ops,
	.pad = &f2k_v4l2_pad_ops,
};

/* subdev internal operations */
static const struct v4l2_subdev_internal_ops f2k_v4l2_internal_ops = {
	.open = f2k_init_formats,
};

/* -----------------------------------------------------------------------------
 * Media entity operations
 *-----------------------------------------------------------------------------/
/*
 * f2k_link_setup - Setup f2k connections.
 * @entity : Pointer to media entity structure
 * @local  : Pointer to local pad array
 * @remote : Pointer to remote pad array
 * @flags  : Link flags
 * return -EINVAL on error or zero on success
 */
static int f2k_link_setup(struct media_entity *entity,
			   const struct media_pad *local,
			   const struct media_pad *remote, u32 flags)
{
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
	struct isp_f2k_device *f2k = v4l2_get_subdevdata(sd);
	unsigned int index = local->index;

	dev_dbg(f2k->isp->dev,"%s:index (%d) start\n",__func__,index);
	/* FIXME: this is actually a hack! */
	if (is_media_entity_v4l2_subdev(remote->entity))
		index |= 2 << 16;

	switch (index) {
	case ISP_F2K_PAD_SINK:
	case ISP_F2K_PAD_SINK | 2 << 16:
		/* Read from vi */
		if (!(flags & MEDIA_LNK_FL_ENABLED)) {
			f2k->input = ISP_F2K_INPUT_NONE;
			break;
		}

		if (f2k->input != ISP_F2K_INPUT_NONE)
		{
			dev_err(f2k->isp->dev,"%s:!ISP_F2K_INPUT_NONE\n",__func__);
			return -EBUSY;
		}	

		if (remote->entity == &f2k->subdev.entity)
			f2k->input = ISP_F2K_INPUT_VI;
		else
			f2k->input = ISP_F2K_INPUT_NONE;
		break;
	case ISP_F2K_PAD_MAIN_SOURCE:
	case ISP_F2K_PAD_MAIN_SOURCE | 2 << 16:
		/* Write to memory */
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (f2k->output & ISP_F2K_OUTPUT_MAIN_MEM)
			{
				dev_err(f2k->isp->dev,"%s:!ISP_F2K_PAD_MAIN_SOURCE\n",__func__);
				return -EBUSY;
			}	
			f2k->output |= ISP_F2K_OUTPUT_MAIN_MEM;
		} else {
			f2k->output &= ~ISP_F2K_OUTPUT_MAIN_MEM;
		}
		break;
	case ISP_F2K_PAD_DS0_SOURCE:
	case ISP_F2K_PAD_DS0_SOURCE | 2 << 16:	
		/* Write to memory */
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (f2k->output & ISP_F2K_OUTPUT_DS0_MEM)
			{
				dev_err(f2k->isp->dev,"%s:!ISP_F2K_PAD_DS0_SOURCE\n",__func__);
				return -EBUSY;
			}	
			f2k->output |= ISP_F2K_OUTPUT_DS0_MEM;
		} else {
			f2k->output &= ~ISP_F2K_OUTPUT_DS0_MEM;
		}
		break;
	case ISP_F2K_PAD_DS1_SOURCE:
	case ISP_F2K_PAD_DS1_SOURCE | 2 << 16:
		/* Write to memory */
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (f2k->output & ISP_F2K_OUTPUT_DS1_MEM)
			{
				dev_err(f2k->isp->dev,"%s:!ISP_F2K_PAD_DS1_SOURCE\n",__func__);
				return -EBUSY;
			}	
			f2k->output |= ISP_F2K_OUTPUT_DS1_MEM;
		} else {
			f2k->output &= ~ISP_F2K_OUTPUT_DS1_MEM;
		}
		break;
	case ISP_F2K_PAD_DS2_SOURCE:
	case ISP_F2K_PAD_DS2_SOURCE | 2 << 16:
		/* Write to memory */
		if (flags & MEDIA_LNK_FL_ENABLED) {
			if (f2k->output & ISP_F2K_OUTPUT_DS2_MEM)
			{
				dev_err(f2k->isp->dev,"%s:!ISP_F2K_PAD_DS2_SOURCE\n",__func__);
				return -EBUSY;
			}	
			f2k->output |= ISP_F2K_OUTPUT_DS2_MEM;
		} else {
			f2k->output &= ~ISP_F2K_OUTPUT_DS2_MEM;
		}
		break;
	default:
		dev_dbg(f2k->isp->dev,"%s:!no index\n",__func__);
		return -EINVAL;			
	}

	dev_dbg(f2k->isp->dev,"%s:end\n",__func__);
	return 0;
}
/* media operations */
static const struct media_entity_operations f2k_media_ops = {
	.link_setup = f2k_link_setup,
	.link_validate = v4l2_subdev_link_validate,
};

/*
 * k510isp_f2k_unregister_entities - Unregister media entities: subdev
 * @ccp2: Pointer to ISP f2k device
 */
void k510isp_f2k_unregister_entities(struct isp_f2k_device *f2k)
{
	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	v4l2_device_unregister_subdev(&f2k->subdev);
	k510isp_video_unregister(&f2k->video_out[MAIN_VIDEO]);
	k510isp_video_unregister(&f2k->video_out[DS0_VIDEO]);
	k510isp_video_unregister(&f2k->video_out[DS1_VIDEO]);
	k510isp_video_unregister(&f2k->video_out[DS2_VIDEO]);
}
/*
 * k510isp_f2k_register_entities - Register the subdev media entity
 * @f2k: Pointer to ISP f2k device
 * @vdev: Pointer to v4l device
 * return negative error code or zero on success
 */
int k510isp_f2k_register_entities(struct isp_f2k_device *f2k,
				    struct v4l2_device *vdev)
{
	int ret;
	dev_dbg(f2k->isp->dev,"%s:start\n",__func__);
	/* Register the subdev and video nodes. */
	ret = v4l2_device_register_subdev(vdev, &f2k->subdev);
	if (ret < 0)
	{
		dev_err(f2k->isp->dev, "%s: v4l2_device_register_subdev failed (%d)\n",
			__func__, ret);
		goto error;
	}		

	ret = k510isp_video_register(&f2k->video_out[MAIN_VIDEO], vdev);
	if (ret < 0)
	{
		dev_err(f2k->isp->dev, "%s: k510isp_video_register failed (%d)\n",
			__func__, ret);
		goto error;
	}

	ret = k510isp_video_register(&f2k->video_out[DS0_VIDEO], vdev);
	if (ret < 0)
	{
		dev_err(f2k->isp->dev, "%s: k510isp_video_register failed (%d)\n",
			__func__, ret);
		goto error_main;
	}

	ret = k510isp_video_register(&f2k->video_out[DS1_VIDEO], vdev);
	if (ret < 0)
	{
		dev_err(f2k->isp->dev, "%s: k510isp_video_register failed (%d)\n",
			__func__, ret);
		goto error_ds0;
	}

	ret = k510isp_video_register(&f2k->video_out[DS2_VIDEO], vdev);
	if (ret < 0)
	{
		dev_err(f2k->isp->dev, "%s: k510isp_video_register failed (%d)\n",
			__func__, ret);
		goto error_ds1;
	}
	dev_dbg(f2k->isp->dev,"k510isp_f2k_register_entities end\n");
	return 0;

error_ds1:
	k510isp_video_unregister(&f2k->video_out[DS1_VIDEO]);
error_ds0:
	k510isp_video_unregister(&f2k->video_out[DS0_VIDEO]);
error_main:	
	k510isp_video_unregister(&f2k->video_out[MAIN_VIDEO]);
error:
	k510isp_f2k_unregister_entities(f2k);
	return ret;
}
/* -----------------------------------------------------------------------------
 * ISP f2k initialisation and cleanup
 *-----------------------------------------------------------------------------/
/*
 * k510isp_f2k_init_entities - Initialize f2k subdev and media entity.
 * @f2k: Pointer to ISP f2k device
 * return negative error code or zero on success
 */
static int k510isp_f2k_init_entities(struct isp_f2k_device *f2k)
{
	struct v4l2_subdev *sd = &f2k->subdev;
	struct media_pad *pads = f2k->pads;
	struct media_entity *me = &sd->entity;
	int ret = 0;
	struct k510_isp_device *isp = to_isp_device(f2k);

	dev_info(isp->dev,"%s:start\n",__func__);
	f2k->input = ISP_F2K_INPUT_NONE;

	v4l2_subdev_init(sd, &f2k_v4l2_ops);
	sd->internal_ops = &f2k_v4l2_internal_ops;
	strlcpy(sd->name, "K510 ISP F2K", sizeof(sd->name));
	sd->grp_id = 1 << 16;   /* group ID for isp subdevs */
	v4l2_set_subdevdata(sd, f2k);

	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS|V4L2_SUBDEV_FL_HAS_DEVNODE;

	pads[ISP_F2K_PAD_SINK].flags = MEDIA_PAD_FL_SINK | MEDIA_PAD_FL_MUST_CONNECT;
	pads[ISP_F2K_PAD_MAIN_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	pads[ISP_F2K_PAD_DS0_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	pads[ISP_F2K_PAD_DS1_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	pads[ISP_F2K_PAD_DS2_SOURCE].flags = MEDIA_PAD_FL_SOURCE;

	me->function = MEDIA_ENT_F_PROC_VIDEO_SCALER;//MEDIA_ENT_F_PROC_VIDEO_SCALER;//MEDIA_ENT_F_PROC_VIDEO_COMPOSER;//MEDIA_ENT_F_IO_V4L;
	me->ops = &f2k_media_ops;
	dev_dbg(isp->dev,"%s:media_entity_pads_init start\n",__func__);
	ret = media_entity_pads_init(me,ISP_F2K_PADS_NUM, pads);
	if (ret < 0)
	{
		dev_err(isp->dev,"%s:media_entity_pads_init ret:%d\n",__func__,ret);
		return ret;
	}	
	dev_dbg(isp->dev,"%s:media_entity_pads_init end\n",__func__);
	f2k_init_formats(sd, NULL);
//
	f2k->video_out[MAIN_VIDEO].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//V4L2_BUF_TYPE_VIDEO_CAPTURE;//V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;//V4L2_BUF_TYPE_VIDEO_OUTPUT;
	f2k->video_out[MAIN_VIDEO].ops = &f2k_video_ops;
	f2k->video_out[MAIN_VIDEO].isp = to_isp_device(f2k);
	f2k->video_out[MAIN_VIDEO].capture_mem = PAGE_ALIGN(4096 * 4096) * 3;
	f2k->video_out[MAIN_VIDEO].bpl_alignment = 32; //16
	f2k->video_out[MAIN_VIDEO].bpl_max = 0xffffffe0;//0xfffffff0; //0xffffffe0
	//f2k->output |= ISP_F2K_OUTPUT_MAIN_MEM; 
	
	ret = k510isp_video_init(&f2k->video_out[MAIN_VIDEO], "F2K");
	if (ret < 0)
	{
		dev_err(isp->dev,"%s:k510isp_video_init ret:%d\n",__func__,ret);
		goto error_video;
	}	
//
	f2k->video_out[DS0_VIDEO].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//V4L2_BUF_TYPE_VIDEO_CAPTURE;//V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;//V4L2_BUF_TYPE_VIDEO_OUTPUT;
	f2k->video_out[DS0_VIDEO].ops = &f2k_video_ops;
	f2k->video_out[DS0_VIDEO].isp = to_isp_device(f2k);
	f2k->video_out[DS0_VIDEO].capture_mem = PAGE_ALIGN(4096 * 4096) * 3;
	f2k->video_out[DS0_VIDEO].bpl_alignment = 32; //16
	f2k->video_out[DS0_VIDEO].bpl_max = 0xffffffe0;//0xfffffff0; //0xffffffe0
	//f2k->output |= ISP_F2K_OUTPUT_DS0_MEM;

	ret = k510isp_video_init(&f2k->video_out[DS0_VIDEO], "F2K_DS0");
	if (ret < 0)
	{
		dev_err(isp->dev,"%s:k510isp_video_init ret:%d\n",__func__,ret);
		goto error_main_video;
	}
//
	f2k->video_out[DS1_VIDEO].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//V4L2_BUF_TYPE_VIDEO_CAPTURE;//V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;//V4L2_BUF_TYPE_VIDEO_OUTPUT;
	f2k->video_out[DS1_VIDEO].ops = &f2k_video_ops;
	f2k->video_out[DS1_VIDEO].isp = to_isp_device(f2k);
	f2k->video_out[DS1_VIDEO].capture_mem = PAGE_ALIGN(4096 * 4096) * 3;
	f2k->video_out[DS1_VIDEO].bpl_alignment = 32; //16
	f2k->video_out[DS1_VIDEO].bpl_max = 0xffffffe0;//0xfffffff0; //0xffffffe0
	//f2k->output |= ISP_F2K_OUTPUT_DS1_MEM;

	ret = k510isp_video_init(&f2k->video_out[DS1_VIDEO], "F2K_DS1");
	if (ret < 0)
	{
		dev_err(isp->dev,"%s:k510isp_video_init ret:%d\n",__func__,ret);
		goto error_ds0_video;
	}

//
	f2k->video_out[DS2_VIDEO].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//V4L2_BUF_TYPE_VIDEO_CAPTURE;//V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;//V4L2_BUF_TYPE_VIDEO_OUTPUT;
	f2k->video_out[DS2_VIDEO].ops = &f2k_video_ops;
	f2k->video_out[DS2_VIDEO].isp = to_isp_device(f2k);
	f2k->video_out[DS2_VIDEO].capture_mem = PAGE_ALIGN(4096 * 4096) * 3;
	f2k->video_out[DS2_VIDEO].bpl_alignment = 32; //16
	f2k->video_out[DS2_VIDEO].bpl_max = 0xffffffe0;//0xfffffff0; //0xffffffe0
	//f2k->output |= ISP_F2K_OUTPUT_DS2_MEM;

	ret = k510isp_video_init(&f2k->video_out[DS2_VIDEO], "F2K_DS2");
	if (ret < 0)
	{
		dev_err(isp->dev,"%s:k510isp_video_init ret:%d\n",__func__,ret);
		goto error_ds1_video;
	}

	dev_dbg(isp->dev,"%s:end\n",__func__);
	return 0;

error_ds1_video:
	k510isp_video_cleanup(&f2k->video_out[DS1_VIDEO]);	
error_ds0_video:
	k510isp_video_cleanup(&f2k->video_out[DS0_VIDEO]);
error_main_video:
	k510isp_video_cleanup(&f2k->video_out[MAIN_VIDEO]);
error_video:
	media_entity_cleanup(&f2k->subdev.entity); 
	return ret;
}
/*
 * f2k_init - f2k initialization.
 * @isp : Pointer to ISP device
 * return negative error code or zero on success
 */
int k510isp_f2k_init(struct k510_isp_device *isp)
{
	struct isp_f2k_device *f2k = &isp->isp_f2k;
	int ret;
	dev_info(isp->dev,"%s:start\n",__func__);

	f2k->isp = isp;
	spin_lock_init(&f2k->lock);
	init_waitqueue_head(&f2k->wait);
	mutex_init(&f2k->ioctl_lock);
	f2k->state = ISP_PIPELINE_STREAM_STOPPED;
	//f2k->stopping = F2K_STOP_NOT_REQUESTED;
	//configure f2k
	//
	unsigned int pad;
	for(pad = ISP_F2K_PAD_SINK;pad < ISP_F2K_PAD_DS2_SOURCE+1;pad++)
	{
		f2k->formats[pad].code = MEDIA_BUS_FMT_SRGGB10_1X10;
		f2k->formats[pad].width = 1920;
		f2k->formats[pad].height = 1080;
	}

	//
	ret = k510isp_f2k_init_entities(f2k);
	if (ret < 0){
		dev_err(isp->dev,"%s:k510isp_f2k_init_entities\n",__func__);
		mutex_destroy(&f2k->ioctl_lock);
		return ret;
	}	
	dev_dbg(isp->dev,"%s:end\n",__func__);
	return 0;
}
/*
 * k510isp_f2k_cleanup - isp f2k module cleanup.
 * @isp: Device pointer specific to the ISP.
 */
void k510isp_f2k_cleanup(struct k510_isp_device *isp)
{
	struct isp_f2k_device *f2k = &isp->isp_f2k;

	dev_info(isp->dev,"%s:start\n",__func__);
	k510isp_video_cleanup(&f2k->video_out[MAIN_VIDEO]);
	k510isp_video_cleanup(&f2k->video_out[DS0_VIDEO]);
	k510isp_video_cleanup(&f2k->video_out[DS1_VIDEO]);
	k510isp_video_cleanup(&f2k->video_out[DS2_VIDEO]);
	media_entity_cleanup(&f2k->subdev.entity);

	if (f2k->nr3d_dma.addr != NULL)
		dma_free_coherent(isp->dev, f2k->nr3d_dma.dma_size, f2k->nr3d_dma.addr,
				  f2k->nr3d_dma.dma);
//
	mutex_destroy(&f2k->ioctl_lock);			  	
}
