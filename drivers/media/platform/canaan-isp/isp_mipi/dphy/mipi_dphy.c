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

#include "../../k510isp_com.h"
#include "../../k510isp_video.h"
#include "../../k510_isp.h"
#include "../../k510video_subsystem.h"
#include "mipi_corner_reg.h"
#include "mipi_dphy_reg.h"
#include "mipi_dphy.h"

/**
 * @brief 
 * 
 * @param isp 
 */
void k510isp_dphy_print_cfg(struct k510_isp_device *isp)
{
	struct isp_dphy_device *dphy = &isp->isp_dphy;
	struct isp_dphy_info *dphy_info = &dphy->dphy_info;

	dev_dbg(isp->dev,"%s:channelcfg:%d\n",__func__,dphy_info->channelcfg);
	dev_dbg(isp->dev,"%s:rxdphyspeed:%d\n",__func__,dphy_info->rxdphyspeed);
}
/**
 * @brief 
 * 
 * @param dphy 
 * @return int 
 */
static int k510isp_mipicorner_config(struct isp_dphy_device *dphy)
{
	struct k510_isp_device *isp = to_isp_device(dphy);
	
	dev_dbg(isp->dev,"%s:start\n",__func__);
	union U_MIPI_CORNER_CFG cfg_data;
	cfg_data.udata = isp_reg_readl(isp,ISP_IOMEM_MIPI_CORNER,MIPI_CORNER_CFG);
	cfg_data.bits.rst_n = 1;
	isp_reg_writel(isp,cfg_data.udata,ISP_IOMEM_MIPI_CORNER,MIPI_CORNER_CFG);

	cfg_data.udata = isp_reg_readl(isp,ISP_IOMEM_MIPI_CORNER,MIPI_CORNER_CFG);
	cfg_data.bits.pvtcal_start = 1;
	isp_reg_writel(isp,cfg_data.udata,ISP_IOMEM_MIPI_CORNER,MIPI_CORNER_CFG);

	union U_MIPI_CORNER_STA sts_data;
	do{
		sts_data.udata = isp_reg_readl(isp,ISP_IOMEM_MIPI_CORNER,MIPI_CORNER_STA);
		dev_dbg(isp->dev,"%s:0x%x\n",__func__,sts_data.udata);
	}while((sts_data.bits.pvtcal_done & 0x1) != 0x1);
	dev_dbg(isp->dev,"%send\n",__func__);
	return 0;
}

/*
 * mipicorner_print_status - Prints MIPICORNER debug information.
 */
#define MIPICORNER_PRINT_REGISTER(isp, name)\
	dev_dbg(isp->dev, #name"(0x%x) =0x%08x\n", \
		isp->mmio_base[ISP_IOMEM_MIPI_CORNER] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp, ISP_IOMEM_MIPI_CORNER, name))

void mipicorner_print_status(struct isp_dphy_device *dphy)
{
	struct k510_isp_device *isp = to_isp_device(dphy);


	dev_dbg(isp->dev, "-------------MIPICORNER Register dump-------------\n");

	MIPICORNER_PRINT_REGISTER(isp, MIPI_CORNER_CFG);
	MIPICORNER_PRINT_REGISTER(isp, MIPI_CORNER_STA);  
}
/**
 * @brief 
 * 
 * @param phy 
 * @return int 
 */
static int k510isp_dphy_config(struct isp_dphy_device *dphy)
{
	struct k510_isp_device *isp = to_isp_device(dphy);
	dev_dbg(isp->dev,"%s:start\n",__func__);
	//mipi corner 
	k510isp_mipicorner_config(dphy);

	//mipi phy
	struct isp_dphy_info *dphy_info = &dphy->dphy_info;

	union U_MIPI_DPHY_RX_REG0 dphy_reg0;
	dphy_reg0.udata = 0x0003a002;
	unsigned int dllrange = 208;
	if(RXDPHY_CHCFG_1X4 == dphy_info->channelcfg)
	{
		dphy_reg0.bits.channelcfg = 1; //one 4x1 mode
	}
	else
	{
		dphy_reg0.bits.channelcfg = 0; //one 2x2 mode
	}

	if(RXDPHY_SPEED_MODE_2500M == dphy_info->rxdphyspeed)
	{
		dphy_reg0.bits.DeSkewEn = 1;
		dllrange = 124;
	}
	else
	{
		dphy_reg0.bits.DeSkewEn = 0;
	}
	//dphy_reg0.bits.t_clk_settle = dphy_info->Tclk_settle;
	//dphy_reg0.bits.t_clk_miss = dphy_info->Tclk_miss;
	//dphy_reg0.bits.t_hs_settle = dphy_info->Ths_settle;
	//dphy_reg0.bits.t_term_en = dphy_info->Tclk_term_en;
	isp_reg_writel(isp,dphy_reg0.udata,ISP_IOMEM_MIPI_DPHY,MIPI_DPHY_RX_REG0);

	union U_MIPI_DPHY_RX_REG1 dphy_reg1;
	dphy_reg1.bits.madj_dll01 = dllrange;
	dphy_reg1.bits.madj_dll23 = dllrange; 
	isp_reg_writel(isp,dphy_reg1.udata,ISP_IOMEM_MIPI_DPHY,MIPI_DPHY_RX_REG1);

	return 0;
}

/*
 * dphy_print_status - Prints dphy debug information.
 */
#define DPHY_PRINT_REGISTER(isp, name)\
	dev_dbg(isp->dev, #name"(0x%x) =0x%08x\n", \
		isp->mmio_base[ISP_IOMEM_MIPI_DPHY] + name-isp->isp_regs+isp->isp_addr,\
		isp_reg_readl(isp, ISP_IOMEM_MIPI_DPHY, name))

void dphy_print_status(struct isp_dphy_device *dphy)
{
	struct k510_isp_device *isp = to_isp_device(dphy);

	dev_dbg(isp->dev, "-------------DPHY Register dump-------------\n");

	DPHY_PRINT_REGISTER(isp, MIPI_DPHY_RX_REG0);
	DPHY_PRINT_REGISTER(isp, MIPI_DPHY_RX_REG1); 
	DPHY_PRINT_REGISTER(isp, MIPI_DPHY_RX_REG2); 
	DPHY_PRINT_REGISTER(isp, MIPI_DPHY_RX_REG3); 
	DPHY_PRINT_REGISTER(isp, MIPI_DPHY_RX_REG4); 
	DPHY_PRINT_REGISTER(isp, MIPI_DPHY_RX_REG5); 	
}
/**
 * @brief 
 * 
 * @param dphy 
 * @param entity 
 * @return int 
 */
int k510isp_dphy_acquire(struct isp_dphy_device *dphy, struct media_entity *entity)
{
	int rval;

	mutex_lock(&dphy->mutex);
	rval = k510isp_csi2_reset(dphy->csi2);
	if (rval < 0)
		goto done;

	dphy->entity = entity;

	rval = k510isp_dphy_config(dphy);
	if (rval < 0)
		goto done;

done:
	if (rval < 0)
		dphy->entity = NULL;

	//mipicorner_print_status(dphy);
	//dphy_print_status(dphy);
	mutex_unlock(&dphy->mutex);
	return rval;
}
/**
 * @brief 
 * 
 * @param dphy 
 */
void k510isp_dphy_release(struct isp_dphy_device *dphy)
{
	struct k510_isp_device *isp = to_isp_device(dphy);
	mutex_lock(&dphy->mutex);
	if (dphy->entity) {
		struct k510isp_pipeline *pipe = to_isp_pipeline(dphy->entity);
		struct k510isp_bus_cfg *buscfg =
			v4l2_subdev_to_bus_cfg(pipe->external);

		//mipi phy
		union U_MIPI_DPHY_RX_REG0 dphy_reg0;
		dphy_reg0.bits.Stdby_n = 0;
		isp_reg_writel(isp,dphy_reg0.udata,ISP_IOMEM_MIPI_DPHY,MIPI_DPHY_RX_REG0);

		dphy->entity = NULL;
	}
	mutex_unlock(&dphy->mutex);
}
/*
 * k510isp_dphy_init - Initialize the CSI PHY frontends
 */
int k510isp_dphy_init(struct k510_isp_device *isp)
{
	struct isp_dphy_device *dphy = &isp->isp_dphy;

	dphy->isp = isp;
	dphy->csi2 = &isp->isp_csi2;
	dphy->num_data_lanes = ISP_DPHY_NUM_DATA_LANES;
	dphy->corner_regs = MIPI_CORNER_BASE;
	dphy->phy_regs = MIPI_DPHY_BASE;
	mutex_init(&dphy->mutex);

	return 0;
}
/*
*
*/
void k510isp_dphy_cleanup(struct k510_isp_device *isp)
{
	mutex_destroy(&isp->isp_dphy.mutex);
}
