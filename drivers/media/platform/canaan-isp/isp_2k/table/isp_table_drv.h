/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _ISP_TABLE_DRV_H_
#define _ISP_TABLE_DRV_H_

#include "../../k510isp_com.h"

void isp_f2k_core_table_init(struct k510_isp_device *isp,struct isp_core_wdr_Info *wdrCoreInfo);
void isp_r2k_core_table_init(struct k510_isp_device *isp);

#endif /*_ISP_TABLE_DRV_H_*/