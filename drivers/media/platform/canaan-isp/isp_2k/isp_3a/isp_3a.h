/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ISP_3A_H__
#define __ISP_3A_H__

#include "../../k510_isp.h"

#define IS_OUT_OF_BOUNDS(value, min, max)		\
	(((value) < (min)) || ((value) > (max)))
//
int k510isp_f2k_af_init(struct k510_isp_device *isp);
void k510isp_f2k_af_cleanup(struct k510_isp_device *isp);
int k510isp_f2k_ae_init(struct k510_isp_device *isp);
void k510isp_f2k_ae_cleanup(struct k510_isp_device *isp);
int k510isp_f2k_awb_init(struct k510_isp_device *isp);
void k510isp_f2k_awb_cleanup(struct k510_isp_device *isp);
//
int k510isp_r2k_af_init(struct k510_isp_device *isp);
void k510isp_r2k_af_cleanup(struct k510_isp_device *isp);
int k510isp_r2k_ae_init(struct k510_isp_device *isp);
void k510isp_r2k_ae_cleanup(struct k510_isp_device *isp);
int k510isp_r2k_awb_init(struct k510_isp_device *isp);
void k510isp_r2k_awb_cleanup(struct k510_isp_device *isp);
#endif /*__ISP_3A_H__*/