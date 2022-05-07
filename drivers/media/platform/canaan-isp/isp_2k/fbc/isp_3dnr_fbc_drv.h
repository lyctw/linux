/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _ISP_3DNR_FBC_DRV_H_
#define _ISP_3DNR_FBC_DRV_H_

#include "isp_3dnr_fbc_reg.h"

typedef struct _ISP_FBC_CTL_S
{
    unsigned int  fbc_out_format_cfg;
}ISP_FBC_CTL_S;

typedef struct _ISP_FBC_SIZE_S
{  
	unsigned int fbc_input_height;
	unsigned int fbc_input_width;
}ISP_FBC_SIZE_S;

typedef struct _ISP_FBC_Y_BUF_S
{
	unsigned int fbc_y_data_buf_base0;
	unsigned int fbc_y_data_buf_base1;
	unsigned int fbc_y_data_stride;
	unsigned int fbc_y_data_wr_blen;
	unsigned int fbc_y_head_buf_base0;
	unsigned int fbc_y_head_buf_base1;
	unsigned int fbc_y_head_stride;
	unsigned int fbc_y_head_wr_blen;
} ISP_FBC_Y_BUF_S;

typedef struct _ISP_FBC_UV_BUF_S
{
	unsigned int fbc_uv_data_buf_base0;
	unsigned int fbc_uv_data_buf_base1;
	unsigned int fbc_uv_data_stride;
	unsigned int fbc_uv_data_wr_blen;
	unsigned int fbc_uv_head_buf_base0;
	unsigned int fbc_uv_head_buf_base1;
	unsigned int fbc_uv_head_stride;
	unsigned int fbc_uv_head_wr_blen;
	
} ISP_FBC_UV_BUF_S;

typedef struct _ISP_FBC_YL_BUF_S
{
	unsigned int fbc_yl_data_buf_base0;
	unsigned int fbc_yl_data_buf_base1;
	unsigned int fbc_yl_data_stride;
	unsigned int fbc_yl_data_wr_blen;
	unsigned int fbc_yl_head_buf_base0;
	unsigned int fbc_yl_head_buf_base1;
	unsigned int fbc_yl_head_stride;
	unsigned int fbc_yl_head_wr_blen;
	
} ISP_FBC_YL_BUF_S;

typedef struct _ISP_FBC_BUF_S
{
	ISP_FBC_Y_BUF_S		stIspFbcYBuf;
	ISP_FBC_UV_BUF_S	stIspFbcUVBuf;
	ISP_FBC_YL_BUF_S	stIspFbcYLBuf;	
} ISP_FBC_BUF_S;

typedef struct _ISP_FBC_ATTR_S
{
	ISP_FBC_CTL_S	stIspfbcCtl;
	ISP_FBC_SIZE_S  stIspfbcSize;
	ISP_FBC_BUF_S	stIspfbcBuf;	
} ISP_FBC_ATTR_S;
/*
*
*/
int Isp_Drv_Fbc_SetBuf(struct k510_isp_device *isp,ISP_FBC_BUF_S *pstIspFbcBuf);
int Isp_Drv_Fbc_SetCtl(struct k510_isp_device *isp,ISP_FBC_CTL_S *pstIspFbcCtl);
int Isp_Drv_Fbc_SetSize(struct k510_isp_device *isp,ISP_FBC_SIZE_S *pstIspFbcSize);

#endif /*_ISP_3DNR_FBC_DRV_H_*/