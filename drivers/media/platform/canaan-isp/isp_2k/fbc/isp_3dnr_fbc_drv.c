/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "../../k510isp_com.h"
#include "../../k510_isp.h"
#include "isp_3dnr_fbc_drv.h"
#include "isp_3dnr_fbc_reg.h"
/*
*ybuf
*/
static int Isp_Drv_Fbc_SetYBuf(struct k510_isp_device *isp,ISP_FBC_Y_BUF_S *pstIspFbcYBuf)
{
    union U_ISP_FBC_Y_DATA_BUF_BASE0 stYBuf0;
    stYBuf0.u32 = 0;
	stYBuf0.bits.fbc_y_data_buf_base0 = pstIspFbcYBuf->fbc_y_data_buf_base0;
	isp_reg_writel(isp,stYBuf0.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_Y_DATA_BUF_BASE0);

	union U_ISP_FBC_Y_DATA_BUF_BASE1 stYBuf1;
	stYBuf1.u32 = 0;
	stYBuf1.bits.fbc_y_data_buf_base1 = pstIspFbcYBuf->fbc_y_data_buf_base1;
	isp_reg_writel(isp,stYBuf1.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_Y_DATA_BUF_BASE1);

	union U_ISP_FBC_Y_DATA_LINE_STRIDE stYStride;
	stYStride.u32 = 0;
	stYStride.bits.fbc_y_data_stride = pstIspFbcYBuf->fbc_y_data_stride;
	isp_reg_writel(isp,stYStride.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_Y_DATA_LINE_STRIDE);

	union U_ISP_FBC_Y_DATA_WR_BLEN stYWriteBlen;
	stYWriteBlen.u32 = 0;
	stYWriteBlen.bits.fbc_y_data_wr_blen = pstIspFbcYBuf->fbc_y_data_wr_blen;
	isp_reg_writel(isp,stYWriteBlen.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_Y_DATA_WR_BLEN);

	union U_ISP_FBC_Y_HEAD_BUF_BASE0 stYHeadBuf0;
	stYHeadBuf0.u32 = 0;
	stYHeadBuf0.bits.fbc_y_head_buf_base0 = pstIspFbcYBuf->fbc_y_head_buf_base0;
	isp_reg_writel(isp,stYHeadBuf0.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_Y_HEAD_BUF_BASE0);

	union U_ISP_FBC_Y_HEAD_BUF_BASE1 stYHeadBuf1;
	stYHeadBuf1.u32 = 0;
	stYHeadBuf1.bits.fbc_y_head_buf_base1 = pstIspFbcYBuf->fbc_y_head_buf_base1;
    isp_reg_writel(isp,stYHeadBuf1.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_Y_HEAD_BUF_BASE1);

	union U_ISP_FBC_Y_HEAD_LINE_STRIDE stYHeadStride;
	stYHeadStride.u32 = 0;
	stYHeadStride.bits.fbc_y_head_stride = pstIspFbcYBuf->fbc_y_head_stride;
	isp_reg_writel(isp,stYHeadStride.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_Y_HEAD_LINE_STRIDE);

	union U_ISP_FBC_Y_HEAD_WR_BLEN stYHeadWriteBlen;
	stYHeadWriteBlen.u32 = 0;
	stYHeadWriteBlen.bits.fbc_y_head_wr_blen = pstIspFbcYBuf->fbc_y_head_wr_blen;
	isp_reg_writel(isp,stYHeadWriteBlen.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_Y_HEAD_WR_BLEN);

    return 0;
}
/*
*UV buf
*/
static int Isp_Drv_Fbc_SetUVBuf(struct k510_isp_device *isp,ISP_FBC_UV_BUF_S *pstIspFbcUVBuf)
{
	union U_ISP_FBC_UV_DATA_BUF_BASE0 stUvBuf0;
	stUvBuf0.u32 = 0;
	stUvBuf0.bits.fbc_uv_data_buf_base0 = pstIspFbcUVBuf->fbc_uv_data_buf_base0;
	isp_reg_writel(isp,stUvBuf0.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_UV_DATA_BUF_BASE0);

	union U_ISP_FBC_UV_DATA_BUF_BASE1 stUvBuf1;	
	stUvBuf1.u32 = 0;
	stUvBuf1.bits.fbc_uv_data_buf_base1 = pstIspFbcUVBuf->fbc_uv_data_buf_base1;
	isp_reg_writel(isp,stUvBuf1.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_UV_DATA_BUF_BASE1);

	union U_ISP_FBC_UV_DATA_LINE_STRIDE stUvStride;	
	stUvStride.u32 = 0;
	stUvStride.bits.fbc_uv_data_stride = pstIspFbcUVBuf->fbc_uv_data_stride;
	isp_reg_writel(isp,stUvStride.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_UV_DATA_LINE_STRIDE);

	union U_ISP_FBC_UV_DATA_WR_BLEN stUvWriteBlen;
	stUvWriteBlen.u32 = 0;
	stUvWriteBlen.bits.fbc_uv_data_wr_blen = pstIspFbcUVBuf->fbc_uv_data_wr_blen;
	isp_reg_writel(isp,stUvWriteBlen.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_UV_DATA_WR_BLEN);

	union U_ISP_FBC_UV_HEAD_BUF_BASE0 stUvHeadBuf0;
	stUvHeadBuf0.u32 = 0;
	stUvHeadBuf0.bits.fbc_uv_head_buf_base0 = pstIspFbcUVBuf->fbc_uv_head_buf_base0;
	isp_reg_writel(isp,stUvHeadBuf0.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_UV_HEAD_BUF_BASE0);

	union U_ISP_FBC_UV_HEAD_BUF_BASE1 stUvHeadBuf1;
	stUvHeadBuf1.u32 = 0;
	stUvHeadBuf1.bits.fbc_uv_head_buf_base1 = pstIspFbcUVBuf->fbc_uv_head_buf_base1;
	isp_reg_writel(isp,stUvHeadBuf1.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_UV_HEAD_BUF_BASE1);

	union U_ISP_FBC_UV_HEAD_LINE_STRIDE stUvHeadStride;		
	stUvHeadStride.u32 = 0;
	stUvHeadStride.bits.fbc_uv_head_stride = pstIspFbcUVBuf->fbc_uv_head_stride;
	isp_reg_writel(isp,stUvHeadStride.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_UV_HEAD_LINE_STRIDE);

	union U_ISP_FBC_UV_HEAD_WR_BLEN stUvHeadWriteBlen;
	stUvHeadWriteBlen.u32 = 0;
	stUvHeadWriteBlen.bits.fbc_uv_head_wr_blen = pstIspFbcUVBuf->fbc_uv_head_wr_blen;
	isp_reg_writel(isp,stUvHeadWriteBlen.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_UV_HEAD_WR_BLEN);

    return 0;
}
/*
*YL buf
*/
static int Isp_Drv_Fbc_SetYLBuf(struct k510_isp_device *isp,ISP_FBC_YL_BUF_S	*pstIspFbcYLBuf)
{
	union U_ISP_FBC_YL_DATA_BUF_BASE0 stYlBuf0;
	stYlBuf0.u32 = 0;
	stYlBuf0.bits.fbc_yl_data_buf_base0 = pstIspFbcYLBuf->fbc_yl_data_buf_base0;
	isp_reg_writel(isp,stYlBuf0.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_YL_DATA_BUF_BASE0);

	union U_ISP_FBC_YL_DATA_BUF_BASE1 stYlBuf1;
	stYlBuf1.u32 = 0;
	stYlBuf1.bits.fbc_yl_data_buf_base1 = pstIspFbcYLBuf->fbc_yl_data_buf_base1;
	isp_reg_writel(isp,stYlBuf1.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_YL_DATA_BUF_BASE1);

	union U_ISP_FBC_YL_DATA_LINE_STRIDE stYlStride;
	stYlStride.u32 = 0;
	stYlStride.bits.fbc_yl_data_stride = pstIspFbcYLBuf->fbc_yl_data_stride;
	isp_reg_writel(isp,stYlStride.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_YL_DATA_LINE_STRIDE);

	union U_ISP_FBC_YL_DATA_WR_BLEN stYlWriteBlen;
	stYlWriteBlen.u32 = 0;
	stYlWriteBlen.bits.fbc_yl_data_wr_blen = pstIspFbcYLBuf->fbc_yl_data_wr_blen;
	isp_reg_writel(isp,stYlWriteBlen.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_YL_DATA_WR_BLEN);

	union U_ISP_FBC_YL_HEAD_BUF_BASE0 stYlHeadBuf0;
	stYlHeadBuf0.u32 = 0;
	stYlHeadBuf0.bits.fbc_yl_head_buf_base0 = pstIspFbcYLBuf->fbc_yl_head_buf_base0;
	isp_reg_writel(isp,stYlHeadBuf0.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_YL_HEAD_BUF_BASE0);

	union U_ISP_FBC_YL_HEAD_BUF_BASE1 stYlHeadBuf1;
	stYlHeadBuf1.u32 = 0;
	stYlHeadBuf1.bits.fbc_yl_head_buf_base1 = pstIspFbcYLBuf->fbc_yl_head_buf_base1;
	isp_reg_writel(isp,stYlHeadBuf1.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_YL_HEAD_BUF_BASE1);

	union U_ISP_FBC_YL_HEAD_LINE_STRIDE stYlHeadStride;
	stYlHeadStride.u32 = 0;
	stYlHeadStride.bits.fbc_yl_head_stride = pstIspFbcYLBuf->fbc_yl_head_stride;
	isp_reg_writel(isp,stYlHeadStride.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_YL_HEAD_LINE_STRIDE);	

	union U_ISP_FBC_YL_HEAD_WR_BLEN stYlHeadWriteBlen;	
	stYlHeadWriteBlen.u32 = 0;
	stYlHeadWriteBlen.bits.fbc_yl_head_wr_blen = pstIspFbcYLBuf->fbc_yl_head_wr_blen;
	isp_reg_writel(isp,stYlHeadWriteBlen.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_YL_HEAD_WR_BLEN);

    return 0;
}
/*
*
*/
int Isp_Drv_Fbc_SetBuf(struct k510_isp_device *isp,ISP_FBC_BUF_S *pstIspFbcBuf)
{
    //Y daya
	ISP_FBC_Y_BUF_S *pstIspFbcYBuf = &pstIspFbcBuf->stIspFbcYBuf;
    Isp_Drv_Fbc_SetYBuf(isp,pstIspFbcYBuf);
    //UV data
	ISP_FBC_UV_BUF_S *pstIspFbcUVBuf = &pstIspFbcBuf->stIspFbcUVBuf;
    Isp_Drv_Fbc_SetUVBuf(isp,pstIspFbcUVBuf);
    //YL data
	ISP_FBC_YL_BUF_S *pstIspFbcYLBuf  = &pstIspFbcBuf->stIspFbcYLBuf;
    Isp_Drv_Fbc_SetYLBuf(isp,pstIspFbcYLBuf);

	return 0;	
}
/*
*
*/
int Isp_Drv_Fbc_SetCtl(struct k510_isp_device *isp,ISP_FBC_CTL_S *pstIspFbcCtl)
{
	union U_ISP_FBC_OUT_FORMAT stOutFormat;
	stOutFormat.u32 = 0;
	stOutFormat.bits.fbc_out_format_cfg = pstIspFbcCtl->fbc_out_format_cfg;
	isp_reg_writel(isp,stOutFormat.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_OUT_FORMAT);

	return 0;	
}
/*
*
*/
int Isp_Drv_Fbc_SetSize(struct k510_isp_device *isp,ISP_FBC_SIZE_S *pstIspFbcSize)
{
	union U_ISP_FBC_INPUT_SIZE stInputSize;	
	stInputSize.u32 = 0;
	stInputSize.bits.fbc_input_height = pstIspFbcSize->fbc_input_height;
	stInputSize.bits.fbc_input_width = pstIspFbcSize->fbc_input_width;
	isp_reg_writel(isp,stInputSize.u32,ISP_IOMEM_F2K_FBC,ISP_FBC_INPUT_SIZE);

	return 0;	
}