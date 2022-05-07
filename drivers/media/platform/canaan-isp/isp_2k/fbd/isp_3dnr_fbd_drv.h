/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _ISP_3DNR_FBD_DRV_H_
#define _ISP_3DNR_FBD_DRV_H_

#include "isp_3dnr_fbd_reg.h"

typedef struct _ISP_FBD_CTL_S
{
    unsigned int  fbd_en;
    unsigned int  fbd_format_cfg;
}ISP_FBD_CTL_S;

typedef struct _ISP_FBD_SIZE_S
{
	unsigned int fbd_input_height;
	unsigned int fbd_input_width;
}ISP_FBD_SIZE_S;

typedef struct _ISP_FBD_Y_BUF_S
{
	unsigned int fbd_y_data_buf_base0;
	unsigned int fbd_y_data_buf_base1;
	unsigned int fbd_y_data_stride;
	unsigned int fbd_y_head_buf_base0;
	unsigned int fbd_y_head_buf_base1;
	unsigned int fbd_y_head_stride;
}ISP_FBD_Y_BUF_S;

typedef struct _ISP_FBD_UV_BUF_S
{
	unsigned int fbd_uv_data_buf_base0;
	unsigned int fbd_uv_data_buf_base1;
	unsigned int fbd_uv_data_stride;
	unsigned int fbd_uv_head_buf_base0;
	unsigned int fbd_uv_head_buf_base1;
	unsigned int fbd_uv_head_stride;
}ISP_FBD_UV_BUF_S;

typedef struct _ISP_FBD_YL_BUF_S
{
	unsigned int fbd_yl_data_buf_base0;
	unsigned int fbd_yl_data_buf_base1;
	unsigned int fbd_yl_data_stride;
	unsigned int fbd_yl_head_buf_base0;
	unsigned int fbd_yl_head_buf_base1;
	unsigned int fbd_yl_head_stride;
}ISP_FBD_YL_BUF_S;

typedef struct _ISP_FBD_BUF_S
{
	ISP_FBD_Y_BUF_S  	stIspFbdYBuf;
	ISP_FBD_UV_BUF_S 	stIspFbdUVBuf; 
	ISP_FBD_YL_BUF_S 	stIspFbdYLBuf;	
} ISP_FBD_BUF_S;

typedef struct _ISP_FBD_ATTR_S
{
	ISP_FBD_CTL_S		stIspFbdCtl;	
	ISP_FBD_SIZE_S		stIspFbdSize;
	ISP_FBD_BUF_S		stIspFbdBuf;	
} ISP_FBD_ATTR_S;
//
int Isp_Drv_Fbd_SetBuf(struct k510_isp_device *isp,ISP_FBD_BUF_S *pstFbdBuf);
int Isp_Drv_Fbd_SetCtl(struct k510_isp_device *isp,ISP_FBD_CTL_S *pstIspFbdCtl);
int Isp_Drv_Fbd_SetSize(struct k510_isp_device *isp,ISP_FBD_SIZE_S *pstIspFbdSize);

#endif /*_ISP_3DNR_FBD_DRV_H_*/