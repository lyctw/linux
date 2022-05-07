/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _MIPI_DPHY_
#define _MIPI_DPHY_

#include <linux/videodev2.h>
#include "../../k510isp_video.h"
#include "../../isp_2k/isp.h"
#include "../../k510isp_com.h"
#include "../../k510_isp.h"

struct k510isp_device;

#define ISP_DPHY_NUM_DATA_LANES	4

enum rxdphy_speed_mode_e
{
    RXDPHY_SPEED_MODE_2500M,
    RXDPHY_SPEED_MODE_1500M,
    RXDPHY_SPEED_MODE_MAX,
};

enum rxdphy_chcfg_e
{
    RXDPHY_CHCFG_2X2,
    RXDPHY_CHCFG_1X4,
    RXDPHY_CHCFG_MAX,
};

struct isp_dphy_info
{
	enum rxdphy_chcfg_e channelcfg;
	enum rxdphy_speed_mode_e rxdphyspeed;
	unsigned int Tclk_term_en;
	unsigned int Ths_term_en;
	unsigned int Ths_settle;
	unsigned int Tclk_miss;
	unsigned int Tclk_settle;
};

struct isp_dphy_device{
	struct k510_isp_device *isp;
	struct mutex mutex;	/* serialize csiphy configuration */
	struct isp_csi2_device *csi2;
	/* the entity that acquired the phy */
	struct media_entity *entity;

	/* mem resources - enums as defined in enum isp_mem_resources */
	unsigned int corner_regs;
	unsigned int phy_regs;

	u8 num_data_lanes;	/* number of CSI2 Data Lanes supported */
	struct isp_dphy_info dphy_info;
};

void k510isp_dphy_print_cfg(struct k510_isp_device *isp);
void mipicorner_print_status(struct isp_dphy_device *dphy);
void dphy_print_status(struct isp_dphy_device *dphy);
int k510isp_dphy_acquire(struct isp_dphy_device *phy, struct media_entity *entity);
void k510isp_dphy_release(struct isp_dphy_device *phy);
int k510isp_dphy_init(struct k510_isp_device *isp);
void k510isp_dphy_cleanup(struct k510_isp_device *isp);
#endif	/* _MIPI_DPHY_ */
