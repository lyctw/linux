/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <asm/cacheflush.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/omap-iommu.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/of_graph.h>
#include <linux/reset.h>

//#include <asm/dma-iommu.h>

#include <media/v4l2-common.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mc.h>

#include "k510video_subsystem.h"
#include "k510isp_video.h"
#include "k510_isp.h"
#include "k510isp_stat.h"
#include "k510isp_com.h"
#include "isp_2k/wrap/isp_wrap_drv.h"
#include "isp_2k/table/isp_table_drv.h"
#include "isp_2k/isp_3a/isp_3a.h"

static int v4l2_init_flag = 0;
extern const struct dma_map_ops swiotlb_noncoh_dma_ops;
static void k510isp_save_ctx(struct k510_isp_device *isp);
static void k510isp_restore_ctx(struct k510_isp_device *isp);

/* Structure for saving/restoring ISP module registers */
static struct k510isp_reg k510isp_reg_list[] = {
	//{k510_ISP_IOMEM_MAIN, ISP_SYSCONFIG, 0},
	//{k510_ISP_IOMEM_MAIN, ISP_CTRL, 0},
	//{k510_ISP_IOMEM_MAIN, ISP_TCTRL_CTRL, 0},
	//{0, ISP_TOK_TERM, 0}
};

static void k510isp_addr_init(struct k510_isp_device *isp)
{
	dev_dbg(isp->dev,"%s:start\n",__func__);
	isp->mmio_base[ISP_IOMEM_MIPI_DPHY] = isp->isp_regs + MIPI_DPHY_BASE;
	isp->mmio_base[ISP_IOMEM_MIPI_CORNER] = isp->isp_regs + MIPI_CORNER_BASE;
	isp->mmio_base[ISP_IOMEM_CSI0_HOST] = isp->isp_regs + CSI_HOST_BASE;
	isp->mmio_base[ISP_IOMEM_CSI1_HOST] = isp->isp_regs + CSI1_HOST_BASE;	
	isp->mmio_base[ISP_IOMEM_VI_WRAP] = isp->isp_regs + VI_WRAP_BASE;
	isp->mmio_base[ISP_IOMEM_VI_PIPE_CSI00] = isp->isp_regs + VI_PIPE_CSI_0_0_REG_BASE;
	isp->mmio_base[ISP_IOMEM_VI_PIPE_CSI01] = isp->isp_regs + VI_PIPE_CSI_0_1_REG_BASE;
	isp->mmio_base[ISP_IOMEM_VI_PIPE_CSI02] = isp->isp_regs + VI_PIPE_CSI_0_2_REG_BASE;
	isp->mmio_base[ISP_IOMEM_VI_PIPE_CSI10] = isp->isp_regs + VI_PIPE_CSI_1_0_REG_BASE;
	isp->mmio_base[ISP_IOMEM_VI_PIPE_CSI11] = isp->isp_regs + VI_PIPE_CSI_1_1_REG_BASE;
	isp->mmio_base[ISP_IOMEM_VI_PIPE_CSI12] = isp->isp_regs + VI_PIPE_CSI_1_2_REG_BASE;
	isp->mmio_base[ISP_IOMEM_VI_PIPE_DVP0] = isp->isp_regs + VI_PIPE_DVP_0_REG_BASE;	
	isp->mmio_base[ISP_IOMEM_VI_PIPE_DVP1] = isp->isp_regs + VI_PIPE_DVP_1_REG_BASE;
	isp->mmio_base[ISP_IOMEM_F2K_WRAP] = isp->isp_regs + ISP_F2K_WRAP_BASE;
	isp->mmio_base[ISP_IOMEM_F2K_CORE] = isp->isp_regs + ISP_F2K_CORE_BASE;
	isp->mmio_base[ISP_IOMEM_F2K_CORE_TABLE] = isp->isp_regs + ISP_F2K_CORE_TABLE_BASE;
	isp->mmio_base[ISP_IOMEM_F2K_FBC] = isp->isp_regs + ISP_F2K_FBC_BASE;
	isp->mmio_base[ISP_IOMEM_F2K_FBD] = isp->isp_regs + ISP_F2K_FBD_BASE;
	isp->mmio_base[ISP_IOMEM_F2K_DS] = isp->isp_regs + ISP_F2K_DS_BASE;
    isp->mmio_base[ISP_IOMEM_F2K_MAIN_REMAP] = isp->isp_regs + ISP_F2K_MAIN_REMAP_BASE;
	isp->mmio_base[ISP_IOMEM_F2K_OUT0_REMAP] = isp->isp_regs + ISP_F2K_OUT0_REMAP_BASE;
    isp->mmio_base[ISP_IOMEM_F2K_OUT1_REMAP] = isp->isp_regs + ISP_F2K_OUT1_REMAP_BASE;
	isp->mmio_base[ISP_IOMEM_R2K_WRAP] = isp->isp_regs + ISP_R2K_WRAP_BASE;
	isp->mmio_base[ISP_IOMEM_R2K_CORE] = isp->isp_regs + ISP_R2K_CORE_BASE;
	isp->mmio_base[ISP_IOMEM_R2K_CORE_TABLE] = isp->isp_regs + ISP_R2K_CORE_TABLE_BASE;
	isp->mmio_base[ISP_IOMEM_R2K_DS] = isp->isp_regs + ISP_R2K_DS_BASE;
    isp->mmio_base[ISP_IOMEM_R2K_MAIN_REMAP] = isp->isp_regs + ISP_R2K_MAIN_REMAP_BASE;
	isp->mmio_base[ISP_IOMEM_R2K_OUT0_REMAP] = isp->isp_regs + ISP_R2K_OUT0_REMAP_BASE;
    isp->mmio_base[ISP_IOMEM_R2K_OUT1_REMAP] = isp->isp_regs + ISP_R2K_OUT1_REMAP_BASE;	
}
/* -----------------------------------------------------------------------------
 * Interrupts
 */

/*
 * isp_enable_interrupts - Enable ISP interrupts.
 * @isp: CANAAN ISP device
 */
static void k510isp_enable_interrupts(struct k510_isp_device *isp)
{
	struct isp_irq_info irq_info;

	memset(&irq_info,0,sizeof(struct isp_irq_info));
	irq_info.main_dma_en = 1;
	//irq_info.ds0_en = 1;
	//irq_info.ds1_en = 1;
	//irq_info.ds2_en = 1;
	//irq_info.ae_en = 1;
	//irq_info.awb_en = 1;
	//irq_info.af_en = 1;
	k510isp_f2k_irq_enable(isp,&irq_info);
	//isp_r2k_irq_enable(isp,&irq_info);

}
/*
 * isp_disable_interrupts - Disable ISP interrupts.
 * @isp: CANAAN ISP device
 */
static void k510isp_disable_interrupts(struct k510_isp_device *isp)
{
	struct isp_irq_info irq_info;
	memset(&irq_info,0,sizeof(struct isp_irq_info));
	k510isp_f2k_irq_enable(isp,&irq_info);
	k510isp_r2k_irq_enable(isp,&irq_info);
}
/*
 * isp_core_init - ISP core settings
 * @isp: k510 ISP device
 * @idle: Consider idle state.
 *
 * Set the power settings for the ISP and SBL bus and configure the HS/VS
 * interrupt source.
 *
 * We need to configure the HS/VS interrupt source before interrupts get
 * enabled, as the sensor might be free-running and the ISP default setting
 * (HS edge) would put an unnecessary burden on the CPU.
 */
static void k510isp_core_init(struct k510_isp_device *isp, int idle)
{

}
/* --------------------------------------------------------------------------
 * Clock management
 *--------------------------------------------------------------------------*/

static void __k510isp_subclk_update(struct k510_isp_device *isp)
{
	u32 clk = 0;


}

void k510isp_subclk_enable(struct k510_isp_device *isp,
			    enum k510isp_subclk_resource res)
{
	isp->subclk_resources |= res;

	__k510isp_subclk_update(isp);
}

void k510isp_subclk_disable(struct k510_isp_device *isp,
			     enum k510isp_subclk_resource res)
{
	isp->subclk_resources &= ~res;

	__k510isp_subclk_update(isp);
}
/*
 * isp_enable_clocks - Enable ISP clocks
 * @isp: CANAAN ISP device
 *
 * Return 0 if successful, or clk_prepare_enable return value if any of them
 * fails.
 */
int k510isp_enable_clocks(struct k510_isp_device *isp)
{
	int ret;
	unsigned long rate;
	ret = clk_prepare_enable(isp->clock[MIPI_CORNER]);
	if(ret){
		dev_err(isp->dev, "failed to enable mipi corner clk\n");
		goto out_mipi_corner_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[MIPI_REF]);
	if(ret){
		dev_err(isp->dev, "failed to enable mipi ref clk\n");
		goto out_mipi_ref_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[MIPI_RXPHY_REF]);
	if(ret){
		dev_err(isp->dev, "failed to enable mipi rxphy ref clk\n");
		goto out_mipi_rxphy_ref_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[CSI0_SYSTEM]);
	if(ret){
		dev_err(isp->dev, "failed to enable csi0 system clk\n");
		goto out_csi0_system_clk;		
	}

	ret = clk_set_rate(isp->clock[CSI0_SYSTEM], SYSTEM_16600_CLK);
	if (ret) {
		dev_err(isp->dev, "clk_set_rate for csi0 system clk failed\n");
		goto out_csi0_system_clk;
	}
	//
	ret = clk_prepare_enable(isp->clock[CSI0_APB]);
	if(ret){
		dev_err(isp->dev, "failed to enable csi0 apb clk\n");
		goto out_csi0_apb_clk;		
	}
	//	 
	ret = clk_prepare_enable(isp->clock[CSI0_PIXEL]);
	if(ret){
		dev_err(isp->dev, "failed to enable csi0 pixel clk\n");
		goto out_csi0_pixel_clk;		
	}

	ret = clk_set_rate(isp->clock[CSI0_PIXEL], PIXEL_14850_CLK);
	if (ret) {
		dev_err(isp->dev, "clk_set_rate for csi0 pixel clk failed\n");
		goto out_csi0_pixel_clk;
	}
	//
	ret = clk_prepare_enable(isp->clock[CSI1_SYSTEM]);
	if(ret){
		dev_err(isp->dev, "failed to enable csi1 system clk\n");
		goto out_csi1_system_clk;		
	}

	ret = clk_set_rate(isp->clock[CSI1_SYSTEM], SYSTEM_16600_CLK);
	if (ret) {
		dev_err(isp->dev, "clk_set_rate for csi1 system clk failed\n");
		goto out_csi1_system_clk;
	}
	//
	ret = clk_prepare_enable(isp->clock[CSI1_APB]);
	if(ret){
		dev_err(isp->dev, "failed to enable csi1 apb clk\n");
		goto out_csi1_apb_clk;		
	} 
	//
	ret = clk_prepare_enable(isp->clock[CSI1_PIXEL]);
	if(ret){
		dev_err(isp->dev, "failed to enable csi1 pixel clk\n");
		goto out_csi1_pixel_clk;		
	}

	ret = clk_set_rate(isp->clock[CSI1_PIXEL], PIXEL_14850_CLK);
	if (ret) {
		dev_err(isp->dev, "clk_set_rate for csi1 pixel clk failed\n");
		goto out_csi1_pixel_clk;
	}
	//
	ret = clk_prepare_enable(isp->clock[VI_APB]);
	if(ret){
		dev_err(isp->dev, "failed to enable vi apb clk\n");
		goto out_vi_apb_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[VI_AXI]);
	if(ret){
		dev_err(isp->dev, "failed to enable vi axi clk\n");
		goto out_vi_axi_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[TPG_PIXEL]);
	if(ret){
		dev_err(isp->dev, "failed to enable tpg pixel clk\n");
		goto out_tpg_pixel_clk;		
	}

	ret = clk_set_rate(isp->clock[TPG_PIXEL], PIXEL_7425_CLK);
	if (ret) {
		dev_err(isp->dev, "clk_set_rate for tpg pixel clk failed\n");
		goto out_tpg_pixel_clk;
	}
	//
	ret = clk_prepare_enable(isp->clock[ISP_F2K_APB]);
	if(ret){
		dev_err(isp->dev, "failed to enable isp f2k apb clk\n");
		goto out_isp_f2k_apb_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[ISP_F2K_AXI]);
	if(ret){
		dev_err(isp->dev, "failed to enable isp f2k axi clk\n");
		goto out_isp_f2k_axi_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[ISP_R2K_APB]);
	if(ret){
		dev_err(isp->dev, "failed to enable isp r2k apb clk\n");
		goto out_isp_r2k_apb_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[ISP_R2K_AXI]);
	if(ret){
		dev_err(isp->dev, "failed to enable isp r2k axi clk\n");
		goto out_isp_r2k_axi_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[ISP_TOF_APB]);
	if(ret){
		dev_err(isp->dev, "failed to enable isp tof apb clk\n");
		goto out_isp_tof_apb_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[ISP_TOF_AXI]);
	if(ret){
		dev_err(isp->dev, "failed to enable isp tof axi clk\n");
		goto out_isp_tof_axi_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[FBC_APB]);
	if(ret){
		dev_err(isp->dev, "failed to enable fbc apb clk\n");
		goto out_fbc_apb_clk;		
	}
	//
	ret = clk_prepare_enable(isp->clock[FBC_AXI]);
	if(ret){
		dev_err(isp->dev, "failed to enable fbc axi clk\n");
		goto out_fbc_axi_clk;		
	}
	return 0;

out_fbc_axi_clk:
	clk_disable_unprepare(isp->clock[FBC_APB]);
out_fbc_apb_clk:
	clk_disable_unprepare(isp->clock[ISP_TOF_AXI]);
out_isp_tof_axi_clk:
	clk_disable_unprepare(isp->clock[ISP_TOF_APB]);
out_isp_tof_apb_clk:
	clk_disable_unprepare(isp->clock[ISP_R2K_AXI]);
out_isp_r2k_axi_clk:
	clk_disable_unprepare(isp->clock[ISP_R2K_APB]);
out_isp_r2k_apb_clk:	
	clk_disable_unprepare(isp->clock[ISP_F2K_AXI]);
out_isp_f2k_axi_clk:
	clk_disable_unprepare(isp->clock[ISP_F2K_APB]);
out_isp_f2k_apb_clk:
	clk_disable_unprepare(isp->clock[TPG_PIXEL]);
out_tpg_pixel_clk:
	clk_disable_unprepare(isp->clock[VI_AXI]);
out_vi_axi_clk:
	clk_disable_unprepare(isp->clock[VI_APB]);
out_vi_apb_clk:
	clk_disable_unprepare(isp->clock[CSI1_PIXEL]);
out_csi1_pixel_clk:
	clk_disable_unprepare(isp->clock[CSI1_APB]);
out_csi1_apb_clk:
	clk_disable_unprepare(isp->clock[CSI1_SYSTEM]);
out_csi1_system_clk:
	clk_disable_unprepare(isp->clock[CSI0_PIXEL]);
out_csi0_pixel_clk:
	clk_disable_unprepare(isp->clock[CSI0_APB]);
out_csi0_apb_clk:
	clk_disable_unprepare(isp->clock[CSI0_SYSTEM]);
out_csi0_system_clk:
	clk_disable_unprepare(isp->clock[MIPI_RXPHY_REF]);
out_mipi_rxphy_ref_clk:
	clk_disable_unprepare(isp->clock[MIPI_REF]);	
out_mipi_ref_clk:
	clk_disable_unprepare(isp->clock[MIPI_CORNER]);
out_mipi_corner_clk:
	return ret;
#if 0
	r = clk_set_rate(isp->clock[ISP_CLK_CAM_MCLK], CM_CAM_MCLK_HZ);
	if (r) {
		dev_err(isp->dev, "clk_set_rate for cam_mclk failed\n");
		goto out_clk_enable_mclk;
	}

	rate = clk_get_rate(isp->clock[ISP_CLK_CAM_MCLK]);
	if (rate != CM_CAM_MCLK_HZ)
		dev_warn(isp->dev, "unexpected cam_mclk rate:\n"
				   " expected : %d\n"
				   " actual   : %ld\n", CM_CAM_MCLK_HZ, rate);



#endif

}

/*
 * isp_disable_clocks - Disable ISP clocks
 * @isp: CANAAN ISP device
 */
void k510isp_disable_clocks(struct k510_isp_device *isp)
{
	clk_disable_unprepare(isp->clock[MIPI_CORNER]);
	clk_disable_unprepare(isp->clock[MIPI_REF]);
	clk_disable_unprepare(isp->clock[MIPI_RXPHY_REF]);
	clk_disable_unprepare(isp->clock[CSI0_SYSTEM]);
	clk_disable_unprepare(isp->clock[CSI0_APB]); 
	clk_disable_unprepare(isp->clock[CSI0_PIXEL]);
	clk_disable_unprepare(isp->clock[CSI1_SYSTEM]);
	clk_disable_unprepare(isp->clock[CSI1_APB]); 
	clk_disable_unprepare(isp->clock[CSI1_PIXEL]);
	clk_disable_unprepare(isp->clock[VI_AXI]);
	clk_disable_unprepare(isp->clock[VI_APB]);
	clk_disable_unprepare(isp->clock[TPG_PIXEL]);
	clk_disable_unprepare(isp->clock[ISP_F2K_APB]);
	clk_disable_unprepare(isp->clock[ISP_F2K_AXI]);
	clk_disable_unprepare(isp->clock[ISP_R2K_APB]);
	clk_disable_unprepare(isp->clock[ISP_R2K_AXI]);
	clk_disable_unprepare(isp->clock[ISP_TOF_APB]);
	clk_disable_unprepare(isp->clock[ISP_TOF_AXI]);
	clk_disable_unprepare(isp->clock[FBC_APB]);
	clk_disable_unprepare(isp->clock[FBC_AXI]);
}

static const char *k510isp_clocks[] = {
	"mipi_corner",
	"mipi_ref",
	"mipi_rxphy_ref",
	"csi0_system",
	"csi0_apb", 
	"csi0_pixel",
	"csi1_system",
	"csi1_apb", 
	"csi1_pixel",
	"vi_axi",
	"vi_apb",
	"tpg_pixel",
	"isp_f2k_apb",
	"isp_f2k_axi",
	"isp_r2k_apb",
	"isp_r2k_axi",
	"isp_tof_apb",
	"isp_tof_axi",
	"fbc_apb",
	"fbc_axi",
};

static int k510isp_get_clocks(struct k510_isp_device *isp)
{
	struct clk *clk;
	unsigned int i;
	dev_dbg(isp->dev,"%s:start\n",__func__);
	for (i = 0; i < ARRAY_SIZE(k510isp_clocks); ++i) {
		clk = devm_clk_get(isp->dev, k510isp_clocks[i]);
		if (IS_ERR(clk)) {
			dev_err(isp->dev, "clk_get %s failed\n", k510isp_clocks[i]);
			return PTR_ERR(clk);
		}

		isp->clock[i] = clk;
	}
	dev_dbg(isp->dev,"%s:end\n",__func__);
	return 0;
}

static const char *k510isp_resets[] = {
	"vi_rst",
	"isp_f4k_rst",
	"isp_f2k_rst",
	"isp_r2k_rst",
	"isp_tof_rst",
	"csi0_rst",
	"csi1_rst",
	"csi2_rst",
	"csi3_rst",
	"sensor_rst",
	"mfbc_rst",
	"mipi_corner_rst",
};

static int k510isp_get_resets(struct k510_isp_device *isp)
{
	struct reset_control *reset_control;
	unsigned int i;
	dev_dbg(isp->dev,"%s:start\n",__func__);
	for (i = 0; i < ARRAY_SIZE(k510isp_resets); ++i) {
		reset_control = devm_reset_control_get(isp->dev, k510isp_resets[i]);
		if (IS_ERR(reset_control)) {
			dev_err(isp->dev, "reset_control_get %s failed\n", k510isp_resets[i]);
			return PTR_ERR(reset_control);
		}

		isp->reset[i] = reset_control;
	}
	dev_dbg(isp->dev,"%s:end\n",__func__);
	return 0;
}
/*
 * k510isp_reset - Reset ISP with a timeout wait for idle.
 * @isp: CANAAN ISP device
 */
static int k510isp_reset(struct k510_isp_device *isp)
{
	unsigned long timeout = 0;

	//isp_f2k_wrap_reset(isp);
	//isp_r2k_wrap_reset(isp);
	reset_control_reset(isp->reset[CSI0_RST]);
	reset_control_reset(isp->reset[CSI1_RST]);
	reset_control_reset(isp->reset[CSI2_RST]);
	reset_control_reset(isp->reset[CSI3_RST]);
	reset_control_reset(isp->reset[VI_RST]);
	reset_control_reset(isp->reset[SENSOR_RST]);
	reset_control_reset(isp->reset[ISP_F2K_RST]);
	reset_control_reset(isp->reset[FBC_RST]);
	reset_control_reset(isp->reset[ISP_R2K_RST]);
	reset_control_reset(isp->reset[ISP_TOF_RST]);	
	isp->stop_failure = false;
	media_entity_enum_zero(&isp->crashed);
	return 0;
}

/*
 * k510isp_save_context - Saves the values of the ISP module registers.
 * @isp: CANAAN ISP device
 * @reg_list: Structure containing pairs of register address and value to
 *            modify on OMAP.
 */
static void k510isp_save_context(struct k510_isp_device *isp, struct k510isp_reg *reg_list)
{
	struct k510isp_reg *next = reg_list;

	//for (; next->reg != ISP_TOK_TERM; next++)
	//	next->val = isp_reg_readl(isp, next->mmio_range, next->reg);
}

/*
 * isp_restore_context - Restores the values of the ISP module registers.
 * @isp: CANAAN ISP device
 * @reg_list: Structure containing pairs of register address and value to
 *            modify on OMAP.
 */
static void k510isp_restore_context(struct k510_isp_device *isp, struct k510isp_reg *reg_list)
{
	struct k510isp_reg *next = reg_list;

	//for (; next->reg != ISP_TOK_TERM; next++)
	//	isp_reg_writel(isp, next->val, next->mmio_range, next->reg);
}

/*
 * isp_save_ctx - Saves ISP, CCDC, HIST, H3A, PREV, RESZ & MMU context.
 * @isp: CANAAN ISP device
 *
 * Routine for saving the context of each module in the ISP.
 * CCDC, HIST, H3A, PREV, RESZ and MMU.
 */
static void k510isp_save_ctx(struct k510_isp_device *isp)
{
	k510isp_save_context(isp, k510isp_reg_list);
	//omap_iommu_save_ctx(isp->dev);
}

/*
 * k510isp_restore_ctx - Restores ISP, CCDC, HIST, H3A, PREV, RESZ & MMU context.
 * @isp: CANAAN ISP device
 *
 * Routine for restoring the context of each module in the ISP.
 */
static void k510isp_restore_ctx(struct k510_isp_device *isp)
{
	k510isp_restore_context(isp, k510isp_reg_list);
	//omap_iommu_restore_ctx(isp->dev);
	//k510isp_ccdc_restore_context(isp);
	//k510isp_preview_restore_context(isp);
}
/*
 * k510isp_get - Acquire the ISP resource.
 *
 * Initializes the clocks for the first acquire.
 *
 * Increment the reference count on the ISP. If the first reference is taken,
 * enable clocks and power-up all submodules.
 *
 * Return a pointer to the ISP device structure, or NULL if an error occurred.
 */
static struct k510_isp_device *__k510isp_get(struct k510_isp_device *isp, bool irq)
{
	struct k510_isp_device *__isp = isp;
	dev_dbg(isp->dev,"%s:start\n",__func__);
	if (isp == NULL)
		return NULL;

	mutex_lock(&isp->isp_mutex);
	if (isp->ref_count > 0)
		goto out;

	if (k510isp_enable_clocks(isp) < 0) {
		__isp = NULL;
		goto out;
	}

	/* We don't want to restore context before saving it! */
	if (isp->has_context)
		k510isp_restore_ctx(isp);

	if (irq)
		k510isp_enable_interrupts(isp);

out:
	if (__isp != NULL)
		isp->ref_count++;
	mutex_unlock(&isp->isp_mutex);

	return __isp;
}
/**
 * @brief 
 * 
 * @param isp 
 * @return struct k510_isp_device* 
 */
struct k510_isp_device *k510isp_get(struct k510_isp_device *isp)
{
	dev_dbg(isp->dev,"%s:start\n",__func__);
	return __k510isp_get(isp, true);
}
/*
 * k510isp_put - Release the ISP
 *
 * Decrement the reference count on the ISP. If the last reference is released,
 * power-down all submodules, disable clocks and free temporary buffers.
 */
static void __k510isp_put(struct k510_isp_device *isp, bool save_ctx)
{
	dev_dbg(isp->dev,"%s:start\n",__func__);
	if (isp == NULL)
		return;

	mutex_lock(&isp->isp_mutex);

	BUG_ON(isp->ref_count == 0);
	if (--isp->ref_count == 0) {
		k510isp_disable_interrupts(isp);
		if (save_ctx) {
			k510isp_save_ctx(isp);
			isp->has_context = 1;
		}
		/* Reset the ISP if an entity has failed to stop. This is the
		 * only way to recover from such conditions.
		 */
		if (!media_entity_enum_empty(&isp->crashed) ||
		    isp->stop_failure)
			k510isp_reset(isp);
		k510isp_disable_clocks(isp);
	}

	mutex_unlock(&isp->isp_mutex);
}

/**
 * @brief 
 * 
 * @param isp 
 */
void k510isp_put(struct k510_isp_device *isp)
{
	dev_dbg(isp->dev,"%s:start\n",__func__);
	__k510isp_put(isp, true);
}
/*
 * k510isp_f2k_isr - Interrupt Service Routine for Camera ISP module.
 * @irq: Not used currently.
 * @_isp: Pointer to the k510 ISP device
 *
 * Handles the corresponding callback if plugged in.
 */
static irqreturn_t k510isp_f2k_isr(int irq, void *_isp) 
{
	struct k510_isp_device *isp = _isp;
	u32 f2k_core_irqstatus;
	u32 f2k_irqstatus0;
	u32 f2k_irqstatus1;
	if(v4l2_init_flag == 0)
		return IRQ_HANDLED;
	dev_dbg(isp->dev,"%s:start\n",__func__);
	static const u32 events  = IRQW0_STS_MAIN_Y_FRAME_IRQ|IRQW0_STS_MAIN_UV_FRAME_IRQ;
	static const u32 ds0_events  = (IRQW1_STS_OUT0_Y_FRAME_IRQ|IRQW1_STS_OUT0_UV_FRAME_IRQ);
	static const u32 ds1_events  = (IRQW1_STS_OUT1_Y_FRAME_IRQ|IRQW1_STS_OUT1_UV_FRAME_IRQ);
	static const u32 ds2_events  = (IRQW1_STS_OUT2_R_FRAME_IRQ|IRQW1_STS_OUT2_G_FRAME_IRQ|IRQW1_STS_OUT2_B_FRAME_IRQ);

	f2k_core_irqstatus = isp_reg_readl(isp, ISP_IOMEM_F2K_WRAP, ISP_WRAP_CORE_INT_CTL);
	isp_reg_writel(isp, f2k_core_irqstatus, ISP_IOMEM_F2K_WRAP, ISP_WRAP_CORE_INT_CTL);

	f2k_irqstatus0 = isp_reg_readl(isp, ISP_IOMEM_F2K_WRAP, ISP_WRAP_DMA_WR_INT_STATUS0);
	isp_reg_writel(isp, f2k_irqstatus0, ISP_IOMEM_F2K_WRAP, ISP_WRAP_DMA_WR_INT_STATUS0);

	f2k_irqstatus1 = isp_reg_readl(isp, ISP_IOMEM_F2K_WRAP, ISP_WRAP_DMA_WR_INT_STATUS1);
	isp_reg_writel(isp, f2k_irqstatus1, ISP_IOMEM_F2K_WRAP, ISP_WRAP_DMA_WR_INT_STATUS1);

	if(f2k_core_irqstatus & CORE_STS_3A_OUT_IRQ)
	{
		k510isp_stat_isr(&isp->isp_f2k_ae);
		k510isp_stat_isr(&isp->isp_f2k_awb);
	//	k510isp_stat_isr(&isp->isp_f2k_af);
	}	

	if(f2k_irqstatus0 & events)
		k510isp_f2k_main_isr(&isp->isp_f2k, f2k_irqstatus0 & events);

	if(f2k_irqstatus1 & ds0_events)
		k510isp_f2k_ds0_isr(&isp->isp_f2k, f2k_irqstatus1 & ds0_events);

	if(f2k_irqstatus1 & ds1_events)
		k510isp_f2k_ds1_isr(&isp->isp_f2k, f2k_irqstatus1 & ds1_events);

	if(f2k_irqstatus1 & ds2_events)
		k510isp_f2k_ds2_isr(&isp->isp_f2k, f2k_irqstatus1 & ds2_events);

	return IRQ_HANDLED;
}
/*
 * k510isp_r2k_isr - Interrupt Service Routine for Camera ISP module.
 * @irq: Not used currently.
 * @_isp: Pointer to the k510 ISP device
 *
 * Handles the corresponding callback if plugged in.
 */
static irqreturn_t k510isp_r2k_isr(int irq, void *_isp) 
{
	struct k510_isp_device *isp = _isp;
	u32 r2k_core_irqstatus;
	u32 r2k_irqstatus0;
	u32 r2k_irqstatus1;
	dev_dbg(isp->dev,"%s:start\n",__func__);
	static const u32 events  = IRQW0_STS_MAIN_Y_FRAME_IRQ|IRQW0_STS_MAIN_UV_FRAME_IRQ;
	static const u32 ds0_events  = (IRQW1_STS_OUT0_Y_FRAME_IRQ|IRQW1_STS_OUT0_UV_FRAME_IRQ);
	static const u32 ds1_events  = (IRQW1_STS_OUT1_Y_FRAME_IRQ|IRQW1_STS_OUT1_UV_FRAME_IRQ);
	static const u32 ds2_events  = (IRQW1_STS_OUT2_R_FRAME_IRQ|IRQW1_STS_OUT2_G_FRAME_IRQ|IRQW1_STS_OUT2_B_FRAME_IRQ);

	r2k_core_irqstatus = isp_reg_readl(isp, ISP_IOMEM_R2K_WRAP, ISP_WRAP_CORE_INT_CTL);
	isp_reg_writel(isp, r2k_core_irqstatus, ISP_IOMEM_R2K_WRAP, ISP_WRAP_CORE_INT_CTL);

	r2k_irqstatus0 = isp_reg_readl(isp, ISP_IOMEM_R2K_WRAP, ISP_WRAP_DMA_WR_INT_STATUS0);
	isp_reg_writel(isp, r2k_irqstatus0, ISP_IOMEM_R2K_WRAP, ISP_WRAP_DMA_WR_INT_STATUS0);

	r2k_irqstatus1 = isp_reg_readl(isp, ISP_IOMEM_R2K_WRAP, ISP_WRAP_DMA_WR_INT_STATUS1);
	isp_reg_writel(isp, r2k_irqstatus1, ISP_IOMEM_R2K_WRAP, ISP_WRAP_DMA_WR_INT_STATUS1);

	if(r2k_core_irqstatus & CORE_STS_3A_OUT_IRQ)
	{
		k510isp_stat_isr(&isp->isp_r2k_ae);
		k510isp_stat_isr(&isp->isp_r2k_awb);
	//	k510isp_stat_isr(&isp->isp_r2k_af);
	}	

	if(r2k_irqstatus0 & events)
		k510isp_r2k_main_isr(&isp->isp_r2k, r2k_irqstatus0 & events);

	if(r2k_irqstatus1 & ds0_events)
		k510isp_r2k_ds0_isr(&isp->isp_r2k, r2k_irqstatus1 & ds0_events);

	if(r2k_irqstatus1 & ds1_events)
		k510isp_r2k_ds1_isr(&isp->isp_r2k, r2k_irqstatus1 & ds1_events);

	if(r2k_irqstatus1 & ds2_events)
		k510isp_r2k_ds2_isr(&isp->isp_r2k, r2k_irqstatus1 & ds2_events);

	return IRQ_HANDLED;
}
/**
 * @brief 
 * 
 */
static const struct media_device_ops k510isp_media_ops = {
	.link_notify = v4l2_pipeline_link_notify,
};
/* -----------------------------------------------------------------------------
 * Pipeline stream management
 * ----------------------------------------------------------------------------*/
/*
 * k510isp_pipeline_enable - Enable streaming on a pipeline
 * @pipe: ISP pipeline
 * @mode: Stream mode (single shot or continuous)
 *
 * Walk the entities chain starting at the pipeline output video node and start
 * all modules in the chain in the given mode.
 *
 * Return 0 if successful, or the return value of the failed video::s_stream
 * operation otherwise.
 */
static int k510isp_pipeline_enable(struct k510isp_pipeline *pipe,
			       enum k510isp_pipeline_stream_state mode)
{
	struct k510_isp_device *isp = pipe->output->isp;
	struct media_entity *entity;
	struct media_pad *pad;
	struct v4l2_subdev *subdev;
	unsigned long flags;
	int ret;
	unsigned long  use_f2k = 0;
	unsigned long  use_r2k = 0;
	struct video_device  *cur_video;
	struct v4l2_subdev *sensor;
	struct media_pad *sensor_pad;

	dev_dbg(isp->dev,"%s:start\n",__func__);

	v4l2_init_flag = 1;
	k510isp_enable_clocks(isp);
	/* Refuse to start streaming if an entity included in the pipeline has
	 * crashed. This check must be performed before the loop below to avoid
	 * starting entities if the pipeline won't start anyway (those entities
	 * would then likely fail to stop, making the problem worse).
	 */
	if (media_entity_enum_intersects(&pipe->ent_enum, &isp->crashed))
	{
		dev_err(isp->dev,"%s:media_entity_enum_intersects\n",__func__);
		return -EIO;
	}	

	spin_lock_irqsave(&pipe->lock, flags);
	pipe->state &= ~(ISP_PIPELINE_IDLE_INPUT | ISP_PIPELINE_IDLE_OUTPUT);
	spin_unlock_irqrestore(&pipe->lock, flags);

	pipe->do_propagation = false;

	entity = &pipe->output->video.entity;
	cur_video = &pipe->output->video;
	unsigned int sink_pads[] ={0,0,VI_PAD_SINK_CSI0,CSI2_PAD_SINK0};//{0,0,1,0};

	//unsigned int sink_pads[] ={0,0,2,1};
	//memcpy((void*)&sink_pads[0],(void*)&pipe->output->sink_pads,sizeof(struct video_sink_pad));
	unsigned int i = 0;
	while(1) {
		pad = &entity->pads[sink_pads[i]];
		//pad = &entity->pads[isp->isp_f2k.source_pad_nums[i]];
		if (!(pad->flags & MEDIA_PAD_FL_SINK))
		{
			dev_warn(isp->dev,"%s:is_media_entity_v4l2_subdev pad->flags(0x%x)\n",__func__,pad->flags);
			break;
		}	

		pad = media_entity_remote_pad(pad);
		if (!pad || !is_media_entity_v4l2_subdev(pad->entity))
		{
			dev_warn(isp->dev,"%s:is_media_entity_v4l2_subdev pad(0x%x)\n",__func__,pad);
			break;	
		}

		entity = pad->entity;
		subdev = media_entity_to_v4l2_subdev(entity);

		ret = v4l2_subdev_call(subdev, video, s_stream, mode);
		dev_info(isp->dev,"%s:i(%d)ret(%d)\n",__func__,i,ret);
		if (ret < 0 && ret != -ENOIOCTLCMD)
		{
			dev_err(isp->dev,"%s:ret(%d)\n",__func__,ret);
			return ret;
		}	

		if(i == 0)
		{
			if((cur_video == &isp->isp_f2k.video_out[0].video)||(cur_video == &isp->isp_f2k.video_out[1].video)
			||(cur_video == &isp->isp_f2k.video_out[1].video) ||(cur_video == &isp->isp_f2k.video_out[3].video))
			{
				dev_info(isp->dev,"%s:f2k cur_video(%p) f2k_used[0] (%d)\n",__func__,cur_video,isp->f2k_used[0]);
				if(isp->f2k_used[0] == 1) 
				{
					dev_info(isp->dev,"%s:i(%d) f2k_used[0] (%d)f2k multivideo!\n",__func__,i,isp->f2k_used[0]);
					break;			
				}
			}
			if((cur_video == &isp->isp_r2k.video_out[0].video)||(cur_video == &isp->isp_r2k.video_out[1].video)
			||(cur_video == &isp->isp_r2k.video_out[1].video) ||(cur_video == &isp->isp_r2k.video_out[3].video))
			{
				dev_info(isp->dev,"%s:r2k cur_video(%p) r2k_used[0](%d)\n",__func__,cur_video,isp->r2k_used[0]);
				if(isp->r2k_used[0] == 1)
				{
					dev_info(isp->dev,"%s:i(%d) r2k_used[0](%d) r2k multivideo!\n",__func__,i,isp->r2k_used[0]);
					break;			
				}
	
			}
		}

		if (subdev == &isp->isp_f2k.subdev) {
			v4l2_subdev_call(&isp->isp_f2k_awb.subdev, video,
					s_stream, mode);
			v4l2_subdev_call(&isp->isp_f2k_ae.subdev, video,
					s_stream, mode);
			v4l2_subdev_call(&isp->isp_f2k_af.subdev, video,
					s_stream, mode);
			pipe->do_propagation = true;
			use_f2k = 1;
		}
//
		if (subdev == &isp->isp_r2k.subdev) {
			v4l2_subdev_call(&isp->isp_r2k_awb.subdev, video,
					s_stream, mode);
			v4l2_subdev_call(&isp->isp_r2k_ae.subdev, video,
					s_stream, mode);
			v4l2_subdev_call(&isp->isp_r2k_af.subdev, video,
					s_stream, mode);
			pipe->do_propagation = true;
		  	use_r2k = 1;
			sink_pads[2] = VI_PAD_SINK_CSI1;//2;
			sink_pads[3] = CSI2_PAD_SINK1;//1;
		}

		i++;
		if( isp->dualcamera_used[0] == 1)
		{
			if(1 == use_f2k)
			{
				sensor_pad = media_entity_remote_pad(&isp->isp_csi2.pads[CSI2_PAD_SINK0]);
				sensor = media_entity_to_v4l2_subdev(sensor_pad->entity);
				v4l2_subdev_call(sensor, video, s_stream, mode);				
			}

			if(1 == use_r2k)
			{
				sensor_pad = media_entity_remote_pad(&isp->isp_csi2.pads[CSI2_PAD_SINK1]);
				sensor = media_entity_to_v4l2_subdev(sensor_pad->entity);
				v4l2_subdev_call(sensor, video, s_stream, mode);				
			}
			if(i >= 2) 
			{
				dev_info(isp->dev,"%s:i(%d) use the second camera!\n",__func__,i);
				break;  //dual camera
			}
		}
		else		
		{
			if(i >= 4) 
			{
				dev_info(isp->dev,"%s:i(%d) use the first camera!\n",__func__,i);
				break;  //dual camera
			}
		}
	}
	dev_dbg(isp->dev,"%s:i(%d) end while\n",__func__,i);
	//
	struct isp_cfg_info *isp_cfg = &isp->isp_f2k.isp_cfg;
	struct isp_core_wdr_Info *wdrCoreInfo = &isp_cfg->isp_core_cfg.wdrInfo; 
	if((1 == use_f2k) && (isp->f2k_used[0] == 0))
	{
		isp_f2k_core_table_init(isp,wdrCoreInfo);
		isp->f2k_used[0] = 1;
		isp->dualcamera_used[0] = 1;
	}

	if((1 == use_r2k) && (isp->r2k_used[0] == 0))
	{
		isp_r2k_core_table_init(isp);
		isp->r2k_used[0] = 1;
		isp->dualcamera_used[0] = 1;
	}

	isp->dualcamera_used[1] = 0;
	isp->f2k_used[1] = 0;
	isp->r2k_used[1] = 0;
	dev_info(isp->dev,"%s:end\n",__func__);
	return 0;
}

static int  k510isp_pipeline_wait_csi2(struct k510_isp_device *isp)
{
	return 0;
}

static int  k510isp_pipeline_wait_vi(struct k510_isp_device *isp)
{
	return 0;
}

static int  k510isp_pipeline_wait_f2k(struct k510_isp_device *isp)
{
	return 0;
}

static int  k510isp_pipeline_wait_r2k(struct k510_isp_device *isp)
{
	return 0;
}

#define ISP_STOP_TIMEOUT	msecs_to_jiffies(1000)

static int k510isp_pipeline_wait(struct k510_isp_device *isp,
			     int(*busy)(struct k510_isp_device *isp))
{
	unsigned long timeout = jiffies + ISP_STOP_TIMEOUT;
	dev_dbg(isp->dev,"%s:start\n",__func__);
	while (!time_after(jiffies, timeout)) {
		if (!busy(isp))
			return 0;
	}

	return 1;
}
/*
 * k510isp_pipeline_disable - Disable streaming on a pipeline
 * @pipe: ISP pipeline
 *
 * Walk the entities chain starting at the pipeline output video node and stop
 * all modules in the chain. Wait synchronously for the modules to be stopped if
 * necessary.
 *
 * Return 0 if all modules have been properly stopped, or -ETIMEDOUT if a module
 * can't be stopped (in which case a software reset of the ISP is probably
 * necessary).
 */
static int k510isp_pipeline_disable(struct k510isp_pipeline *pipe,struct k510isp_video *video)
{
	struct k510_isp_device *isp = pipe->output->isp;
	struct media_entity *entity;
	struct media_pad *pad;
	struct v4l2_subdev *subdev;
	int failure = 0;
	int ret;
	struct video_device *cur_video;
	struct v4l2_subdev *sensor;
	struct media_pad *sensor_pad;
	unsigned long  use_f2k = 0;
	unsigned long  use_r2k = 0;

	v4l2_init_flag = 0;
	cur_video = &pipe->output->video;	
	dev_info(isp->dev,"%s:cur_video(%p) video (%p) pipe(%p)start\n",__func__,cur_video,video,pipe);
	cur_video = (struct video_device*)video;
	dev_info(isp->dev,"%s:cur_video(%p) video (%p) pipe(%p)start\n",__func__,cur_video,video,pipe);	
	/*
	 * We need to stop all the modules after CCDC first or they'll
	 * never stop since they may not get a full frame from CCDC.
	 */
	entity = &pipe->output->video.entity;
	unsigned int sink_pads[] ={0,0,VI_PAD_SINK_CSI0,CSI2_PAD_SINK0};//{0,0,1,0};
	//unsigned int r2k_sink_pads[] ={0,0,2,1};
	//memcpy((void*)&sink_pads[0],(void*)&pipe->output->sink_pads,sizeof(struct video_sink_pad));
	unsigned int i = 0;
	unsigned int j = 0;
	while (1) {

		pad = &entity->pads[sink_pads[i]];//&entity->pads[0];

		if (!(pad->flags & MEDIA_PAD_FL_SINK))
			break;

		pad = media_entity_remote_pad(pad);

		if (!pad || !is_media_entity_v4l2_subdev(pad->entity))
			break;

		entity = pad->entity;

		subdev = media_entity_to_v4l2_subdev(entity);

		//if (subdev == &isp->isp_f2k.subdev)
		if( j == 0)
		{
			if((cur_video == &isp->isp_f2k.video_out[0].video)||(cur_video == &isp->isp_f2k.video_out[1].video)
			||(cur_video == &isp->isp_f2k.video_out[1].video) ||(cur_video == &isp->isp_f2k.video_out[3].video))			
			{
			v4l2_subdev_call(&isp->isp_f2k_awb.subdev,
					 video, s_stream, 0);
			v4l2_subdev_call(&isp->isp_f2k_af.subdev,
					 video, s_stream, 0);
			v4l2_subdev_call(&isp->isp_f2k_ae.subdev,
					 video, s_stream, 0);	
			use_f2k = 1;			
				subdev = &isp->isp_f2k.subdev;			
			}
		}

		//if (subdev == &isp->isp_r2k.subdev)
		if( j == 0)
		{
			if((cur_video == &isp->isp_r2k.video_out[0].video)||(cur_video == &isp->isp_r2k.video_out[1].video)
			||(cur_video == &isp->isp_r2k.video_out[1].video) ||(cur_video == &isp->isp_r2k.video_out[3].video))
			{
			v4l2_subdev_call(&isp->isp_r2k_awb.subdev,
					 video, s_stream, 0);
			v4l2_subdev_call(&isp->isp_r2k_af.subdev,
					 video, s_stream, 0);
			v4l2_subdev_call(&isp->isp_r2k_ae.subdev,
					 video, s_stream, 0);
			sink_pads[2] = VI_PAD_SINK_CSI1;//2;
			sink_pads[3] = CSI2_PAD_SINK1;//1;
			use_r2k = 1;
				subdev = &isp->isp_r2k.subdev;
			}
		}
		j++;

		ret = v4l2_subdev_call(subdev, video, s_stream, 0);

		if( i == 0)
		{
			if((cur_video == &isp->isp_f2k.video_out[0].video)||(cur_video == &isp->isp_f2k.video_out[1].video)
			||(cur_video == &isp->isp_f2k.video_out[1].video) ||(cur_video == &isp->isp_f2k.video_out[3].video))
			{
				if(isp->f2k_used[1] == 1) 
				{
					dev_info(isp->dev,"%s:i(%d) isp->f2k_used[1](%d)f2k multivideo!\n",__func__,i,isp->f2k_used[1]);
					break;			
				}
			}
			if((cur_video == &isp->isp_r2k.video_out[0].video)||(cur_video == &isp->isp_r2k.video_out[1].video)
			||(cur_video == &isp->isp_r2k.video_out[1].video) ||(cur_video == &isp->isp_r2k.video_out[3].video))
			{
				if(isp->r2k_used[1] == 1)
				{
					dev_info(isp->dev,"%s:i(%d) isp->r2k_used[1](%d)r2k multivideo!\n",__func__,i,isp->r2k_used[1]);
					break;			
				}
			}
		}

		if (subdev == &isp->isp_csi2.subdev)
			ret |= k510isp_pipeline_wait(isp, k510isp_pipeline_wait_csi2);
		if (subdev == &isp->isp_vi.subdev)
			ret |= k510isp_pipeline_wait(isp, k510isp_pipeline_wait_vi);
		if (subdev == &isp->isp_f2k.subdev)
			ret |= k510isp_pipeline_wait(isp, k510isp_pipeline_wait_f2k);
		if (subdev == &isp->isp_r2k.subdev)
			ret |= k510isp_pipeline_wait(isp, k510isp_pipeline_wait_r2k);
			
		/* Handle stop failures. An entity that fails to stop can
		 * usually just be restarted. Flag the stop failure nonetheless
		 * to trigger an ISP reset the next time the device is released,
		 * just in case.
		 *
		 * The preview engine is a special case. A failure to stop can
		 * mean a hardware crash. When that happens the preview engine
		 * won't respond to read/write operations on the L4 bus anymore,
		 * resulting in a bus fault and a kernel oops next time it gets
		 * accessed. Mark it as crashed to prevent pipelines including
		 * it from being started.
		 */

		if (ret) {
			dev_err(isp->dev, "Unable to stop %s\n", subdev->name);
			isp->stop_failure = true;
			failure = -ETIMEDOUT;
		}
		i++;
		if( isp->dualcamera_used[1] == 1)
		{
			if(1 == use_f2k)
			{
				sensor_pad = media_entity_remote_pad(&isp->isp_csi2.pads[CSI2_PAD_SINK0]);
				sensor = media_entity_to_v4l2_subdev(sensor_pad->entity);
				v4l2_subdev_call(sensor, video, s_stream, 0);				
			}

			if(1 == use_r2k)
			{
				sensor_pad = media_entity_remote_pad(&isp->isp_csi2.pads[CSI2_PAD_SINK1]);
				sensor = media_entity_to_v4l2_subdev(sensor_pad->entity);
				v4l2_subdev_call(sensor, video, s_stream, 0);				
			}
			if(i >= 2) 
			{
				dev_info(isp->dev,"%s:i(%d) use the second camera!\n",__func__,i);
				break;  //dual camera
			}
		}
		else		
		{
			if(i >= 4) 
			{
				dev_info(isp->dev,"%s:i(%d) use the first camera!\n",__func__,i);
				break;  //dual camera
			}
		}
		//
		if( i == 0)
		{
			if((cur_video == &isp->isp_f2k.video_out[0].video)||(cur_video == &isp->isp_f2k.video_out[1].video)
			||(cur_video == &isp->isp_f2k.video_out[1].video) ||(cur_video == &isp->isp_f2k.video_out[3].video))
			{
				isp->f2k_used[1] = 1;
				isp->dualcamera_used[1] = 1;
			}

			if((cur_video == &isp->isp_r2k.video_out[0].video)||(cur_video == &isp->isp_r2k.video_out[1].video)
			||(cur_video == &isp->isp_r2k.video_out[1].video) ||(cur_video == &isp->isp_r2k.video_out[3].video))
			{
				isp->r2k_used[1] = 1;
				isp->dualcamera_used[1] = 1;
			}
		}
	}

	isp->dualcamera_used[0] = 0;
	isp->f2k_used[0] = 0;
	isp->r2k_used[0] = 0;
	k510isp_disable_clocks(isp);
	dev_dbg(isp->dev,"%s:end ret(%d)\n",__func__,failure);
	return failure;
}

/*
 * k510isp_pipeline_set_stream - Enable/disable streaming on a pipeline
 * @pipe: ISP pipeline
 * @state: Stream state (stopped, single shot or continuous)
 *
 * Set the pipeline to the given stream state. Pipelines can be started in
 * single-shot or continuous mode.
 *
 * Return 0 if successful, or the return value of the failed video::s_stream
 * operation otherwise. The pipeline state is not updated when the operation
 * fails, except when stopping the pipeline.
 */
int k510isp_pipeline_set_stream(struct k510isp_pipeline *pipe,enum k510isp_pipeline_stream_state state,struct k510isp_video *video)
{
	int ret;
	dev_info(pipe->output->isp->dev,"%s:state(%d)\n",__func__,state);
	if (state == ISP_PIPELINE_STREAM_STOPPED)
		ret = k510isp_pipeline_disable(pipe,video);
	else
		ret = k510isp_pipeline_enable(pipe, state);

	if (ret == 0 || state == ISP_PIPELINE_STREAM_STOPPED)
		pipe->stream_state = state;

	return ret;
}

/*
 * k510isp_pipeline_cancel_stream - Cancel stream on a pipeline
 * @pipe: k510 ISP pipeline
 *
 * Cancelling a stream mark all buffers on all video nodes in the pipeline as
 * erroneous and makes sure no new buffer can be queued. This function is called
 * when a fatal error that prevents any further operation on the pipeline
 * occurs.
 */
void k510isp_pipeline_cancel_stream(struct k510isp_pipeline *pipe)
{
	if (pipe->input)
		k510isp_video_cancel_stream(pipe->input);
	if (pipe->output)
		k510isp_video_cancel_stream(pipe->output);
}

/*
 * isp_pipeline_resume - Resume streaming on a pipeline
 * @pipe: ISP pipeline
 *
 * Resume video output and input and re-enable pipeline.
 */
static void k510isp_pipeline_resume(struct k510isp_pipeline *pipe)
{
	int singleshot = pipe->stream_state == ISP_PIPELINE_STREAM_SINGLESHOT;

	k510isp_video_resume(pipe->output, !singleshot);
	if (singleshot)
		k510isp_video_resume(pipe->input, 0);
	k510isp_pipeline_enable(pipe, pipe->stream_state);
}

/*
 * k510isp_pipeline_suspend - Suspend streaming on a pipeline
 * @pipe: ISP pipeline
 *
 * Suspend pipeline.
 */
static void k510isp_pipeline_suspend(struct k510isp_pipeline *pipe)
{
	struct k510isp_video *video = pipe->output;
	k510isp_pipeline_disable(pipe,video);
}

/*
 * k510isp_pipeline_is_last - Verify if entity has an enabled link to the output
 *			  video node
 * @me: ISP module's media entity
 *
 * Returns 1 if the entity has an enabled link to the output video node or 0
 * otherwise. It's true only while pipeline can have no more than one output
 * node.
 */
static int k510isp_pipeline_is_last(struct media_entity *me)
{
	struct k510isp_pipeline *pipe;
	struct media_pad *pad;

	if (!me->pipe)
		return 0;
	pipe = to_isp_pipeline(me);
	if (pipe->stream_state == ISP_PIPELINE_STREAM_STOPPED)
		return 0;
	pad = media_entity_remote_pad(&pipe->output->pad);
	return pad->entity == me;
}

/*
 * k510isp_suspend_module_pipeline - Suspend pipeline to which belongs the module
 * @me: ISP module's media entity
 *
 * Suspend the whole pipeline if module's entity has an enabled link to the
 * output video node. It works only while pipeline can have no more than one
 * output node.
 */
static void k510isp_suspend_module_pipeline(struct media_entity *me)
{
	if (k510isp_pipeline_is_last(me))
		k510isp_pipeline_suspend(to_isp_pipeline(me));
}

/*
 * isp_resume_module_pipeline - Resume pipeline to which belongs the module
 * @me: ISP module's media entity
 *
 * Resume the whole pipeline if module's entity has an enabled link to the
 * output video node. It works only while pipeline can have no more than one
 * output node.
 */
static void k510isp_resume_module_pipeline(struct media_entity *me)
{
	if (k510isp_pipeline_is_last(me))
		k510isp_pipeline_resume(to_isp_pipeline(me));
}

/*
 * isp_module_sync_idle - Helper to sync module with its idle state
 * @me: ISP submodule's media entity
 * @wait: ISP submodule's wait queue for streamoff/interrupt synchronization
 * @stopping: flag which tells module wants to stop
 *
 * This function checks if ISP submodule needs to wait for next interrupt. If
 * yes, makes the caller to sleep while waiting for such event.
 */
int k510isp_module_sync_idle(struct media_entity *me, wait_queue_head_t *wait,
			      atomic_t *stopping)
{
	struct k510isp_pipeline *pipe = to_isp_pipeline(me);

	if (pipe->stream_state == ISP_PIPELINE_STREAM_STOPPED ||
	    (pipe->stream_state == ISP_PIPELINE_STREAM_SINGLESHOT &&
	     !k510isp_pipeline_ready(pipe)))
		return 0;

	/*
	 * atomic_set() doesn't include memory barrier on ARM platform for SMP
	 * scenario. We'll call it here to avoid race conditions.
	 */
	atomic_set(stopping, 1);
	smp_mb();

	/*
	 * If module is the last one, it's writing to memory. In this case,
	 * it's necessary to check if the module is already paused due to
	 * DMA queue underrun or if it has to wait for next interrupt to be
	 * idle.
	 * If it isn't the last one, the function won't sleep but *stopping
	 * will still be set to warn next submodule caller's interrupt the
	 * module wants to be idle.
	 */
	if (k510isp_pipeline_is_last(me)) {
		struct k510isp_video *video = pipe->output;
		unsigned long flags;
		spin_lock_irqsave(&video->irqlock, flags);
		if (video->dmaqueue_flags & ISP_VIDEO_DMAQUEUE_UNDERRUN) {
			spin_unlock_irqrestore(&video->irqlock, flags);
			atomic_set(stopping, 0);
			smp_mb();
			return 0;
		}
		spin_unlock_irqrestore(&video->irqlock, flags);
		if (!wait_event_timeout(*wait, !atomic_read(stopping),
					msecs_to_jiffies(1000))) {
			atomic_set(stopping, 0);
			smp_mb();
			return -ETIMEDOUT;
		}
	}

	return 0;
}

/*
 * k510isp_module_sync_is_stopping - Helper to verify if module was stopping
 * @wait: ISP submodule's wait queue for streamoff/interrupt synchronization
 * @stopping: flag which tells module wants to stop
 *
 * This function checks if ISP submodule was stopping. In case of yes, it
 * notices the caller by setting stopping to 0 and waking up the wait queue.
 * Returns 1 if it was stopping or 0 otherwise.
 */
int k510isp_module_sync_is_stopping(wait_queue_head_t *wait,
				     atomic_t *stopping)
{
	if (atomic_cmpxchg(stopping, 1, 0)) {
		wake_up(wait);
		return 1;
	}

	return 0;
}
/*
 * k510isp_suspend_modules - Suspend ISP submodules.
 * @isp: CANAAN ISP device
 *
 * Returns 0 if suspend left in idle state all the submodules properly,
 * or returns 1 if a general Reset is required to suspend the submodules.
 */
static int k510isp_suspend_modules(struct k510_isp_device *isp)
{
	unsigned long timeout;

	k510isp_stat_suspend(&isp->isp_f2k_af);
	k510isp_stat_suspend(&isp->isp_f2k_ae);
	k510isp_stat_suspend(&isp->isp_f2k_awb);
	k510isp_stat_suspend(&isp->isp_r2k_af);
	k510isp_stat_suspend(&isp->isp_r2k_ae);
	k510isp_stat_suspend(&isp->isp_r2k_awb);
	k510isp_suspend_module_pipeline(&isp->isp_csi2.subdev.entity);
	k510isp_suspend_module_pipeline(&isp->isp_vi.subdev.entity);	
	k510isp_suspend_module_pipeline(&isp->isp_f2k.subdev.entity);
	k510isp_suspend_module_pipeline(&isp->isp_r2k.subdev.entity);

	return 1;//0;
}

/*
 * k510isp_resume_modules - Resume ISP submodules.
 * @isp: CANAAN ISP device
 */
static void k510isp_resume_modules(struct k510_isp_device *isp)
{
	k510isp_stat_resume(&isp->isp_f2k_af);
	k510isp_stat_resume(&isp->isp_f2k_ae);
	k510isp_stat_resume(&isp->isp_f2k_awb);
	k510isp_stat_resume(&isp->isp_r2k_af);
	k510isp_stat_resume(&isp->isp_r2k_ae);
	k510isp_stat_resume(&isp->isp_r2k_awb);
	k510isp_resume_module_pipeline(&isp->isp_csi2.subdev.entity);
	k510isp_resume_module_pipeline(&isp->isp_vi.subdev.entity);
	k510isp_resume_module_pipeline(&isp->isp_f2k.subdev.entity);
	k510isp_resume_module_pipeline(&isp->isp_r2k.subdev.entity);

}
/**
 * @brief 
 * 
 * @param isp 
 */
static void k510isp_unregister_entities(struct k510_isp_device *isp)
{

	k510isp_csi2_unregister_entities(&isp->isp_csi2);
	k510isp_vi_unregister_entities(&isp->isp_vi);
	k510isp_f2k_unregister_entities(&isp->isp_f2k);
	k510isp_r2k_unregister_entities(&isp->isp_r2k);
	k510isp_stat_unregister_entities(&isp->isp_f2k_af);
	k510isp_stat_unregister_entities(&isp->isp_f2k_ae);
	k510isp_stat_unregister_entities(&isp->isp_f2k_awb);
	k510isp_stat_unregister_entities(&isp->isp_r2k_af);
	k510isp_stat_unregister_entities(&isp->isp_r2k_ae);
	k510isp_stat_unregister_entities(&isp->isp_r2k_awb);

	v4l2_device_unregister(&isp->v4l2_dev);
	media_device_unregister(&isp->media_dev);
	media_device_cleanup(&isp->media_dev);
}
/**
 * @brief 
 * 
 * @param isp 
 * @param entity 
 * @param interface 
 * @return int 
 */
static int k510isp_link_entity(struct k510_isp_device *isp, struct media_entity *entity,
	enum k510isp_interface_type interface)
{
	struct media_entity *input;
	unsigned int flags;
	unsigned int pad;
	unsigned int i;
	dev_dbg(isp->dev,"%s:entity(0x%x),interface(%d)\n",__func__,entity,interface);
	/* Connect the sensor to the correct interface module.
	 * Parallel sensors are connected directly to the VI, while
	 * serial sensors are connected to the CSI2a, or CSI2b
	 * receiver through CSIPHY.
	 */

	switch (interface) {

	case ISP_INTERFACE_DVP:
		input = &isp->isp_f2k.subdev.entity;
		pad = ISP_F2K_PAD_SINK;
		flags = 0;
		break;

	case ISP_INTERFACE_CSI1:
		input = &isp->isp_csi2.subdev.entity;
		pad = CSI2_PAD_SINK0;
		flags = MEDIA_LNK_FL_IMMUTABLE | MEDIA_LNK_FL_ENABLED;
		break;
	case ISP_INTERFACE_CSI2:
		input = &isp->isp_csi2.subdev.entity;
		pad = CSI2_PAD_SINK1;
		flags = MEDIA_LNK_FL_IMMUTABLE | MEDIA_LNK_FL_ENABLED;
		break;

	default:
		dev_err(isp->dev, "%s: invalid interface type %u\n", __func__,
			interface);
		return -EINVAL;
	}

	/*
	 * Not all interfaces are available on all revisions of the
	 * ISP. The sub-devices of those interfaces aren't initialised
	 * in such a case. Check this by ensuring the num_pads is
	 * non-zero.
	 */
	if (!input->num_pads) {
		dev_err(isp->dev, "%s: invalid input %u\n", entity->name,
			interface);
		return -EINVAL;
	}

	for (i = 0; i < entity->num_pads; i++) {
		if (entity->pads[i].flags & MEDIA_PAD_FL_SOURCE)
			break;
	}
	if (i == entity->num_pads) {
		dev_err(isp->dev, "%s: no source pad in external entity %s\n",
			__func__, entity->name);
		return -EINVAL;
	}

	dev_dbg(isp->dev,"%s:flags(0x%x)\n",__func__,flags);
	return media_create_pad_link(entity, i, input, pad, flags);
}
/**
 * @brief 
 * 
 * @param isp 
 * @return int 
 */
static int k510isp_register_entities(struct k510_isp_device *isp)
{
	int ret = 0;
	dev_dbg(isp->dev,"%s:start\n",__func__);

	isp->media_dev.dev = isp->dev;
	strlcpy(isp->media_dev.model, "CANAAN K510 ISP",
		sizeof(isp->media_dev.model));
	isp->media_dev.hw_revision = isp->f2k_revision;
	isp->media_dev.ops = &k510isp_media_ops;
	media_device_init(&isp->media_dev);

	isp->v4l2_dev.mdev = &isp->media_dev;
	ret = v4l2_device_register(isp->dev, &isp->v4l2_dev);
	if (ret < 0) {
		dev_err(isp->dev, "%s: V4L2 device registration failed (%d)\n",
			__func__, ret);
		goto done;
	}

	dev_dbg(isp->dev,"%s: Register internal entities\n",__func__);	
	/* Register internal entities */
	dev_dbg(isp->dev,"%s: k510isp_csi2_register_entities\n",__func__);
	ret = k510isp_csi2_register_entities(&isp->isp_csi2, &isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: k510isp_csi2_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	dev_dbg(isp->dev,"%s: k510isp_vi_register_entities\n",__func__);
	ret = k510isp_vi_register_entities(&isp->isp_vi, &isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: k510isp_vi_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	dev_dbg(isp->dev,"%s: k510isp_f2k_register_entities\n",__func__);	
	ret = k510isp_f2k_register_entities(&isp->isp_f2k,&isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: k510isp_f2k_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	dev_dbg(isp->dev,"%s: k510isp_r2k_register_entities\n",__func__);
	ret = k510isp_r2k_register_entities(&isp->isp_r2k,&isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: k510isp_r2k_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	ret = k510isp_stat_register_entities(&isp->isp_f2k_af,&isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: isp_f2k_af k510isp_stat_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	ret = k510isp_stat_register_entities(&isp->isp_f2k_ae,&isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: isp_f2k_ae k510isp_stat_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	ret = k510isp_stat_register_entities(&isp->isp_f2k_awb,&isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: isp_f2k_awb k510isp_stat_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	ret = k510isp_stat_register_entities(&isp->isp_r2k_af,&isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: isp_r2k_af k510isp_stat_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	ret = k510isp_stat_register_entities(&isp->isp_r2k_ae,&isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: isp_r2k_ae k510isp_stat_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	ret = k510isp_stat_register_entities(&isp->isp_r2k_awb,&isp->v4l2_dev);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: isp_r2k_awb k510isp_stat_register_entities failed (%d)\n",
			__func__, ret);
		goto done;
	}

	dev_dbg(isp->dev,"%s:end\n",__func__);
	return 0;
done:
	k510isp_unregister_entities(isp);
	dev_err(isp->dev, "%s: k510isp_register_entities end(%d)\n",
			__func__, ret);	
	return ret;
}

/*
 * k510isp_create_links() - Create links for internal and external ISP entities
 * @isp : Pointer to ISP device
 *
 * This function creates all links between ISP internal and external entities.
 *
 * Return: A negative error code on failure or zero on success. Possible error
 * codes are those returned by media_create_pad_link().
 */
static int k510isp_create_links(struct k510_isp_device *isp)
{
	int ret = 0;
	unsigned int flags = 0;
	dev_dbg(isp->dev,"%s:start!!\n",__func__);
	flags = MEDIA_LNK_FL_ENABLED;
	/* Create links between entities. */
	ret = media_create_pad_link(
			&isp->isp_csi2.subdev.entity, CSI2_PAD_SOURCE0,
			&isp->isp_vi.subdev.entity, VI_PAD_SINK_CSI0, flags);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_csi2.subdev.entity, CSI2_PAD_SOURCE1,
			&isp->isp_vi.subdev.entity, VI_PAD_SINK_CSI1, flags);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_vi.subdev.entity, VI_PAD_SOURCE_F2K,
			&isp->isp_f2k.subdev.entity, ISP_F2K_PAD_SINK, flags);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_vi.subdev.entity, VI_PAD_SOURCE_R2K,
			&isp->isp_r2k.subdev.entity, ISP_R2K_PAD_SINK, flags);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	/* Create links between entities and video nodes.*/
	dev_dbg(isp->dev,"isp->isp_f2k.subdev.entity (0x%x),isp->isp_f2k.video_out.video.entity (0x%x)\n",&isp->isp_f2k.subdev.entity,&isp->isp_f2k.video_out[MAIN_VIDEO].video.entity);

	ret = media_create_pad_link(
			&isp->isp_f2k.subdev.entity, ISP_F2K_PAD_MAIN_SOURCE,
			&isp->isp_f2k.video_out[MAIN_VIDEO].video.entity, 0, flags);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_f2k.subdev.entity, ISP_F2K_PAD_DS0_SOURCE,
			&isp->isp_f2k.video_out[DS0_VIDEO].video.entity, 0, flags);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_f2k.subdev.entity, ISP_F2K_PAD_DS1_SOURCE,
			&isp->isp_f2k.video_out[DS1_VIDEO].video.entity, 0, flags);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_f2k.subdev.entity, ISP_F2K_PAD_DS2_SOURCE,
			&isp->isp_f2k.video_out[DS2_VIDEO].video.entity, 0, flags);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_f2k.subdev.entity, ISP_F2K_PAD_MAIN_SOURCE,
			&isp->isp_f2k_ae.subdev.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_f2k.subdev.entity, ISP_F2K_PAD_MAIN_SOURCE,
			&isp->isp_f2k_awb.subdev.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_f2k.subdev.entity, ISP_F2K_PAD_MAIN_SOURCE,
			&isp->isp_f2k_af.subdev.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}
	//
	ret = media_create_pad_link(
			&isp->isp_r2k.subdev.entity, ISP_R2K_PAD_MAIN_SOURCE,
			&isp->isp_r2k.video_out[MAIN_VIDEO].video.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_r2k.subdev.entity, ISP_R2K_PAD_DS0_SOURCE,
			&isp->isp_r2k.video_out[DS0_VIDEO].video.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_r2k.subdev.entity, ISP_R2K_PAD_DS1_SOURCE,
			&isp->isp_r2k.video_out[DS1_VIDEO].video.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_r2k.subdev.entity, ISP_R2K_PAD_DS2_SOURCE,
			&isp->isp_r2k.video_out[DS2_VIDEO].video.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_r2k.subdev.entity, ISP_R2K_PAD_MAIN_SOURCE,
			&isp->isp_r2k_ae.subdev.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_r2k.subdev.entity, ISP_R2K_PAD_MAIN_SOURCE,
			&isp->isp_r2k_awb.subdev.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}

	ret = media_create_pad_link(
			&isp->isp_r2k.subdev.entity, ISP_R2K_PAD_MAIN_SOURCE,
			&isp->isp_r2k_af.subdev.entity, 0, 0);
	if (ret < 0)
	{
		dev_err(isp->dev, "%s: media_create_pad_link failed (%d)\n",
			__func__, ret);
		return ret;	
	}
	return 0;
}
/**
 * @brief 
 * 
 * @param isp 
 */
static void k510isp_cleanup_modules(struct k510_isp_device *isp)
{
	k510isp_f2k_af_cleanup(isp);
	k510isp_f2k_awb_cleanup(isp);
	k510isp_f2k_ae_cleanup(isp);
	k510isp_r2k_af_cleanup(isp);
	k510isp_r2k_awb_cleanup(isp);
	k510isp_r2k_ae_cleanup(isp);
	k510isp_dphy_cleanup(isp);
	k510isp_csi2_cleanup(isp);
	k510isp_vi_cleanup(isp);
	k510isp_f2k_cleanup(isp);
	k510isp_r2k_cleanup(isp);
}
/**
 * @brief 
 * 
 * @param isp 
 * @return int 
 */
static int k510isp_initialize_modules(struct k510_isp_device *isp)
{
	int ret = 0;

	dev_dbg(isp->dev,"%s:start\n",__func__);

	ret = k510isp_dphy_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_dphy_init failed (%d)\n",
			__func__, ret);
		return ret;
	}

	ret = k510isp_csi2_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_csi2_init failed (%d)\n",
			__func__, ret);
		goto error_csi2;
	}

	ret = k510isp_vi_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_vi_init failed (%d)\n",
			__func__, ret);
		goto error_vi;
	}

	ret = k510isp_f2k_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_f2k_init failed (%d)\n",
			__func__, ret);
		goto error_f2k;
	}
//
	ret = k510isp_r2k_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_r2k_init failed (%d)\n",
			__func__, ret);
		goto error_r2k;
	}
//
	ret = k510isp_f2k_ae_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_f2k_ae_init failed (%d)\n",
			__func__, ret);
		goto error_f2k_ae;
	}

	ret = k510isp_f2k_awb_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_f2k_awb_init failed (%d)\n",
			__func__, ret);
		goto error_f2k_awb;
	}

	ret = k510isp_f2k_af_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_f2k_af_init failed (%d)\n",
			__func__, ret);
		goto error_f2k_af;
	}

	ret = k510isp_r2k_ae_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_r2k_ae_init failed (%d)\n",
			__func__, ret);
		goto error_r2k_ae;
	}

	ret = k510isp_r2k_awb_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_r2k_awb_init failed (%d)\n",
			__func__, ret);
		goto error_r2k_awb;
	}

	ret = k510isp_r2k_af_init(isp);
	if (ret < 0) {
		dev_err(isp->dev, "%s: k510isp_r2k_af_init failed (%d)\n",
			__func__, ret);
		goto error_r2k_af;
	}

	dev_dbg(isp->dev,"%s:start\n",__func__);
	return 0;

error_r2k_af:
	k510isp_r2k_awb_cleanup(isp);
error_r2k_awb:
	k510isp_r2k_ae_cleanup(isp);	
error_r2k_ae:
	k510isp_f2k_af_cleanup(isp);
error_f2k_af:
	k510isp_f2k_awb_cleanup(isp);
error_f2k_awb:
	k510isp_f2k_ae_cleanup(isp);
error_f2k_ae:
	k510isp_r2k_cleanup(isp);
error_r2k:
	k510isp_f2k_cleanup(isp);
error_f2k:
	k510isp_vi_cleanup(isp);
error_vi:
	k510isp_csi2_cleanup(isp);
error_csi2:
	k510isp_dphy_cleanup(isp);

	dev_err(isp->dev,"%s:end ret：(%d)\n",__func__,ret);
	return ret;
}
/*****************************************************************
 * 
 * **************************************************************/
static void k510isp_detach_iommu(struct k510_isp_device *isp)
{
	//arm_iommu_release_mapping(isp->mapping);
	isp->mapping = NULL;
}

static int k510isp_attach_iommu(struct k510_isp_device *isp)
{
	struct dma_iommu_mapping *mapping;
	int ret;

	/*
	 * Create the ARM mapping, used by the ARM DMA mapping core to allocate
	 * VAs. This will allocate a corresponding IOMMU domain.
	 */
	//mapping = arm_iommu_create_mapping(&platform_bus_type, SZ_1G, SZ_2G);
	if (IS_ERR(mapping)) {
		dev_err(isp->dev, "failed to create ARM IOMMU mapping\n");
		ret = PTR_ERR(mapping);
		goto error;
	}

	isp->mapping = mapping;

	/* Attach the ARM VA mapping to the device. */
	//ret = arm_iommu_attach_device(isp->dev, mapping);
	if (ret < 0) {
		dev_err(isp->dev, "failed to attach device to VA mapping\n");
		goto error;
	}

	return 0;

error:
	k510isp_detach_iommu(isp);
	return ret;
}
/*
 * k510isp_remove - Remove ISP platform device
 * @pdev: Pointer to ISP platform device
 *
 * Always returns 0.
 */
static int k510isp_remove(struct platform_device *pdev)
{
	struct k510_isp_device *isp = platform_get_drvdata(pdev);

	v4l2_async_notifier_unregister(&isp->notifier);
	k510isp_unregister_entities(isp);
	k510isp_cleanup_modules(isp);
	//isp_xclk_cleanup(isp);

	__k510isp_get(isp, false);
	k510isp_detach_iommu(isp);
	__k510isp_put(isp, false);

	media_entity_enum_cleanup(&isp->crashed);
	v4l2_async_notifier_cleanup(&isp->notifier);

	return 0;
}

/**
 * @brief 
 * 
 * @param isp 
 * @return int 
 */
static int k510isp_parse_of(struct k510_isp_device *isp)//,struct k510isp_bus_cfg *buscfg)
{
	struct device *dev = isp->dev;
	struct device_node *endpoint;
	struct fwnode_handle *fwnode;
	unsigned int rval;
	u32	out_val;
	u32 sensor_num;
	u32 lane_num[1 + V4L2_FWNODE_CSI2_MAX_DATA_LANES];

	struct isp_dphy_device *dphy = &isp->isp_dphy;
	struct isp_dphy_info *dphy_info = &dphy->dphy_info;
	struct isp_csi2_device *csi2 = &isp->isp_csi2;
	struct isp_csi2_info *csi2_info = &csi2->csi2_info;
	struct vi_device *vi = &isp->isp_vi;
	struct vi_wrap_cfg_info  *vi_wrap_cfg = &vi->vi_cfg.vi_wrap_cfg;

	dev_dbg(isp->dev,"%s:start\n",__func__);
	rval = of_property_read_u32(dev->of_node,"sensor_num",&out_val);
	if(rval != 0)
	{
		dev_err(dev, "%s:Failed to get sensor_num\n",__func__);
		return -EINVAL;
	}
	if((sensor_num <= 0 ) && (sensor_num > 2))
	{
		dev_err(dev, "%s:set sensor_num err!\n",__func__);
		return -EINVAL;
	}
	sensor_num = out_val;
	//
	rval = of_property_read_u32(dev->of_node,"dphy_speed",&out_val);
	if(rval != 0)
	{
		dev_err(dev, "%s:Failed to get dphy_speed\n",__func__);
		return -EINVAL;
	}
	dphy_info->rxdphyspeed = (enum rxdphy_speed_mode_e)out_val;
	//
	rval = of_property_read_u32(dev->of_node,"dphy_mode",&out_val);
	if(rval != 0)
	{
		dev_err(dev, "%s:Failed to get dphy_mode\n",__func__);
		return -EINVAL;
	}
	dphy_info->channelcfg = (enum rxdphy_chcfg_e)out_val;
	vi_wrap_cfg->dphy_mode = out_val;
	//
	rval = of_property_read_u32(dev->of_node,"sony_mode",&out_val);
	if(rval != 0)
	{
		dev_err(dev, "%s:Failed to get sony_mode\n",__func__);
		return -EINVAL;
	}
	vi_wrap_cfg->sony_mode = out_val;
	//point0
	if(sensor_num > 0)
	{
		endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
		if (!endpoint) {
			dev_err(dev, "%s:Failed to get endpoint\n",__func__);
			return -EINVAL;
		}
		fwnode = of_fwnode_handle(endpoint);

		rval = fwnode_property_match_string(fwnode,"status","okay");
		if(rval < 0)
		{
			csi2_info->csi0_used = 0;
		}
		else
		{
			csi2_info->csi0_used = 1;
		}

		rval = fwnode_property_read_u32_array(fwnode, "data-lanes",NULL, 0);
		if (rval > 0) 
		{
			csi2_info->csi0_lane_nb = min_t(int, V4L2_FWNODE_CSI2_MAX_DATA_LANES, rval);
			fwnode_property_read_u32_array(fwnode, "data-lanes",lane_num, csi2_info->csi0_lane_nb);
			dev_dbg(isp->dev,"%s:lane0 num= %d,%d,%d\n",__func__,rval,lane_num[0],lane_num[1]);
			csi2_info->csi0_dl0_map = lane_num[0];
			csi2_info->csi0_dl1_map = lane_num[1];
		}
		else{
			dev_err(dev, "%s:Get mipi lane num failed!\n",__func__);
			return -EINVAL;
		}


		rval = fwnode_property_read_u32_array(fwnode, "data-type",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get data-type\n",__func__);
			return -EINVAL;		
		}
		csi2_info->csi00_datatype_select0 = out_val;
		csi2_info->csi00_datatype_select1 = out_val;
		csi2_info->csi01_datatype_select0 = out_val;
		csi2_info->csi01_datatype_select1 = out_val;
		csi2_info->csi02_datatype_select0 = out_val;
		csi2_info->csi02_datatype_select1 = out_val;

		rval = fwnode_property_read_u32_array(fwnode, "tpg_r_en",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get tpg_r_en\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[0].tpg_r_en = out_val;

		rval = fwnode_property_read_u32_array(fwnode, "tpg_w_en",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get tpg_w_en\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[0].tpg_w_en = out_val;

		rval = fwnode_property_read_u32_array(fwnode, "wdr_sensor_vendor",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get wdr_sensor_vendor\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[0].wdr_sensor_vendor = (WDR_SENSOR_VENDOR_E)out_val;
		csi2_info->csi0_sony_wdr = 0;
		if(0 == out_val)
		{
			csi2_info->csi0_sony_wdr = 1;
		}

		rval = fwnode_property_read_u32_array(fwnode, "wdr_mode",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get wdr_mode\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[0].wdr_mode = (VI_PIPE_WDR_MODE_E)out_val;

		rval = fwnode_property_read_u32_array(fwnode, "mipi_csi_mode",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get mipi_csi_mode\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[0].mipi_mode = (MIPI_CSI_MODE_E)out_val;

		rval = fwnode_property_read_u32_array(fwnode, "isp_pipeline",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get isp_pipeline\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[0].isp_pipeline = (ISP_PIPELINE_E)out_val;
	}
	//point1
	if((sensor_num-1) > 0)
	{

		endpoint = of_graph_get_next_endpoint(dev->of_node, endpoint);
		if (!endpoint) {
			dev_err(dev, "Failed to get endpoint\n");
			return -EINVAL;
		}
		fwnode = of_fwnode_handle(endpoint);

		rval = fwnode_property_match_string(fwnode,"status","okay");
		if(rval < 0)
		{
			csi2_info->csi1_used = 0;
		}
		else
		{
			csi2_info->csi1_used = 1;
		}

		rval = fwnode_property_read_u32_array(fwnode, "data-lanes",NULL,0);
		if (rval > 0) 
		{
			csi2_info->csi1_lane_nb = min_t(int, V4L2_FWNODE_CSI2_MAX_DATA_LANES, rval);
			fwnode_property_read_u32_array(fwnode, "data-lanes",lane_num, csi2_info->csi1_lane_nb);
			dev_dbg(isp->dev,"%s:lane1 num= %d,%d,%d\n",__func__,rval,lane_num[0],lane_num[1]);
			csi2_info->csi1_dl0_map = lane_num[0];
			csi2_info->csi1_dl1_map = lane_num[1];
		}
		else{
			dev_err(dev, "%s:Get mipi lane num failed!\n",__func__);
			return -EINVAL;
		}

		rval = fwnode_property_read_u32_array(fwnode, "data-type",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get data-type\n",__func__);
			return -EINVAL;		
		}
		csi2_info->csi10_datatype_select0 = out_val;
		csi2_info->csi10_datatype_select1 = out_val;


		rval = fwnode_property_read_u32_array(fwnode, "tpg_r_en",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get tpg_r_en\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[1].tpg_r_en = out_val;

		rval = fwnode_property_read_u32_array(fwnode, "tpg_w_en",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get tpg_w_en\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[1].tpg_w_en = out_val;

		rval = fwnode_property_read_u32_array(fwnode, "wdr_sensor_vendor",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get wdr_sensor_vendor\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[1].wdr_sensor_vendor = (WDR_SENSOR_VENDOR_E)out_val;
		csi2_info->csi1_sony_wdr = 0;
		if(0 == out_val)
		{
			csi2_info->csi1_sony_wdr = 1;
		}

		rval = fwnode_property_read_u32_array(fwnode, "wdr_mode",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get wdr_mode\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[1].wdr_mode = (VI_PIPE_WDR_MODE_E)out_val;

		rval = fwnode_property_read_u32_array(fwnode, "mipi_csi_mode",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get mipi_csi_mode\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[1].mipi_mode = (MIPI_CSI_MODE_E)out_val;

		rval = fwnode_property_read_u32_array(fwnode, "isp_pipeline",&out_val,1);
		if(rval != 0)
		{
			dev_err(dev, "%s:Failed to get isp_pipeline\n",__func__);
			return -EINVAL;		
		}
		vi_wrap_cfg->sensor_info[1].isp_pipeline = (ISP_PIPELINE_E)out_val;
	}

	k510isp_dphy_print_cfg(isp);
	k510isp_csi2_print_cfg(isp);
	k510isp_vi_wrap_print_cfg(isp);
	return 0;
}

/**
 * @brief 
 * 
 * @param dev 
 * @param vep 
 * @param asd 
 * @return int 
 */
static int k510isp_fwnode_parse(struct device *dev,struct v4l2_fwnode_endpoint *vep,
			    struct v4l2_async_subdev *asd)
{
	struct k510isp_async_subdev *isd =
		container_of(asd, struct k510isp_async_subdev, asd);
	struct k510isp_bus_cfg *buscfg = &isd->bus;
	bool csi1 = false;
	unsigned int i;

	dev_dbg(dev, "%s:parsing %pOF, interface %u\n",__func__,to_of_node(vep->base.local_fwnode), vep->base.port);

	switch (vep->base.port) {
	case 0:
		buscfg->interface = ISP_INTERFACE_DVP;
		break;
	case 1:
		buscfg->interface = ISP_INTERFACE_CSI1;
		break;
	case 2:
		buscfg->interface = ISP_INTERFACE_CSI2;
		break;
	default:
		dev_warn(dev, "%pOF: invalid interface %u\n",
			 to_of_node(vep->base.local_fwnode), vep->base.port);
		return -EINVAL;
	}

	return 0;
}
/*-----------------------------------------------------------------------------
*
*-----------------------------------------------------------------------------*/
/**
 * @brief 
 * 
 * @param async 
 * @return int 
 */
static int k510isp_subdev_notifier_complete(struct v4l2_async_notifier *async)
{
	struct k510_isp_device *isp = container_of(async, struct k510_isp_device,
					      notifier);
	struct v4l2_device *v4l2_dev = &isp->v4l2_dev;
	struct v4l2_subdev *sd;
	int ret;

	ret = media_entity_enum_init(&isp->crashed, &isp->media_dev);
	if (ret)
		return ret;

	list_for_each_entry(sd, &v4l2_dev->subdevs, list) {
		if (sd->notifier != &isp->notifier)
			continue;

		ret = k510isp_link_entity(isp, &sd->entity,
				      v4l2_subdev_to_bus_cfg(sd)->interface);
		if (ret < 0)
			return ret;
	}

	ret = v4l2_device_register_subdev_nodes(&isp->v4l2_dev);
	if (ret < 0)
		return ret;

	return media_device_register(&isp->media_dev);
}
/**
 * @brief 
 * 
 */
static const struct v4l2_async_notifier_operations k510isp_subdev_notifier_ops = {
	.complete = k510isp_subdev_notifier_complete,
};
/*-----------------------------------------------------------------------------
*probe
*-----------------------------------------------------------------------------*/
/*
 * isp_probe - Probe ISP platform device
 * @pdev: Pointer to ISP platform device
 *
 * Returns 0 if successful,
 *   -ENOMEM if no memory available,
 *   -ENODEV if no platform device resources found
 *     or no space for remapping registers,
 *   -EINVAL if couldn't install ISR,
 *   or clk_get return error value.
 */
static int k510isp_probe(struct platform_device *pdev)
{
	struct k510_isp_device *isp;
	struct resource *mem;
	int ret = -ENOMEM;
	int i, m;

	dev_info(&pdev->dev,"%s:start\n",__func__);

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!mem) {
        dev_err(&pdev->dev,"get resource failed \n");
        return -ENOMEM;
    }

	isp = devm_kzalloc(&pdev->dev, sizeof(*isp), GFP_KERNEL);
	if (!isp) {
		dev_err(&pdev->dev, "could not allocate memory\n");
		return -ENOMEM;
	}

	isp->isp_addr = mem->start;
    isp->isp_regs = ioremap(mem->start, resource_size(mem));
    if(!isp->isp_regs) {
        dev_err(&pdev->dev,"ioremap failed \n");
        goto error;
    }	

	dev_info(&pdev->dev,"mem->start = 0x%x,isp->isp_regs= 0x%x\n",mem->start,isp->isp_regs);

	mutex_init(&isp->isp_mutex);
	spin_lock_init(&isp->stat_lock);

	ret = v4l2_async_notifier_parse_fwnode_endpoints(
		&pdev->dev, &isp->notifier, sizeof(struct k510isp_async_subdev),
		k510isp_fwnode_parse);
	if (ret < 0)
	{
		dev_err(&pdev->dev,"%s:v4l2_async_notifier_parse_fwnode_endpoints ret:(%d)\n",__func__,ret);
		goto error;
	}

	isp->dev = &pdev->dev;
	isp->ref_count = 0;
	isp->dualcamera_used[0] = 0;
	isp->f2k_used[0] = 0;
	isp->r2k_used[0] = 0;
	isp->dualcamera_used[1] = 0;
	isp->f2k_used[1] = 0;
	isp->r2k_used[1] = 0;
	ret = k510isp_parse_of(isp);
	if (ret < 0)
	{
		dev_err(&pdev->dev,"%s:k510isp_parse_of ret:(%d)\n",__func__,ret);
		goto error;
	}

	ret = dma_coerce_mask_and_coherent(isp->dev, DMA_BIT_MASK(32));
	if (ret)
	{
		dev_err(&pdev->dev,"%s:dma_coerce_mask_and_coherent ret:(%d)\n",__func__,ret);
		goto error;
	}
	set_dma_ops(isp->dev, &swiotlb_noncoh_dma_ops);

	platform_set_drvdata(pdev, isp);

	k510isp_addr_init(isp);
	/* Clocks
	 *
	 * The ISP clock tree is revision-dependent. We thus need to enable ICLK
	 * manually to read the revision before calling __k510isp_get().
	 *
	 * Start by mapping the ISP MMIO area, which is in two pieces.
	 * The ISP IOMMU is in between. Map both now, and fill in the
	 * ISP revision specific portions a little later in the
	 * function.
	 */
	ret = k510isp_get_clocks(isp);
	if (ret < 0)
		goto error;

	ret = clk_enable(isp->clock[ISP_F2K_APB]);
	if (ret < 0)
		goto error;

	isp->f2k_revision = Isp_Drv_F2k_Wrap_GetRevison(isp);
	dev_info(isp->dev, "f2k_revision %d.%d.%d.%d.%d found\n",
		(isp->f2k_revision & 0xf000000) >> 24,(isp->f2k_revision & 0xff0000) >> 16, 
		(isp->f2k_revision & 0xff00) >> 8, (isp->f2k_revision & 0xf0) >> 4, isp->f2k_revision & 0x0f);
	clk_disable(isp->clock[ISP_F2K_APB]);
	//
	if (__k510isp_get(isp, false) == NULL) {
		dev_info(isp->dev,"%s:ret(%d)\n",__func__,-ENODEV);
		ret = -ENODEV;
		goto error;
	}	

	/*
	*resets
	*/
	ret = k510isp_get_resets(isp);
	if (ret < 0)
		goto error;

	ret = k510isp_reset(isp);
	if (ret < 0)
		goto error_isp; 

	/* IOMMU */
	ret = k510isp_attach_iommu(isp);
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to attach to IOMMU\n");
		goto error_isp;
	}
	/* Interrupt f2k*/
	ret = platform_get_irq(pdev, 0);
	if (ret <= 0) {
		dev_err(isp->dev, "%s:No IRQ resource\n",__func__);
		ret = -ENODEV;
		goto error_iommu;
	}
	isp->irq_num[0] = ret;
	dev_dbg(&pdev->dev,"f2k_irq_num =%d\n",isp->irq_num[0]);
	if (devm_request_irq(isp->dev, isp->irq_num[0], k510isp_f2k_isr, IRQF_SHARED,
			     "k510 ISP F2K", isp)) {
		dev_err(isp->dev, "Unable to request IRQ\n");
		ret = -EINVAL;
		goto error_iommu;
	}

	/* Interrupt r2k*/
	ret = platform_get_irq(pdev, 1);
	if (ret <= 0) {
		dev_err(isp->dev, "%s:No IRQ resource\n",__func__);
		ret = -ENODEV;
		goto error_irq0;
	}
	isp->irq_num[1] = ret;
	dev_dbg(&pdev->dev,"r2k_irq_num =%d\n",isp->irq_num[1]);

	if (devm_request_irq(isp->dev, isp->irq_num[1], k510isp_r2k_isr, IRQF_SHARED,
			     "k510 ISP R2K", isp)) {
		dev_err(isp->dev, "Unable to request IRQ\n");
		ret = -EINVAL;
		goto error_irq0;
	}

	/* Entities */
	ret = k510isp_initialize_modules(isp);
	if (ret < 0)
	{
		dev_err(&pdev->dev,"%s:k510isp_initialize_modules ret:(%d)\n",__func__,ret);
		goto error_irq0;
	}

	ret = k510isp_register_entities(isp);
	if (ret < 0)
	{
		dev_err(&pdev->dev,"%s:k510isp_register_entities ret:(%d)\n",__func__,ret);
		goto error_modules;
	}
	ret = k510isp_create_links(isp);
	if (ret < 0)
	{
		dev_err(&pdev->dev,"%s:k510isp_create_links ret:(%d)\n",__func__,ret);
		goto error_register_entities;
	}
	
	isp->notifier.ops = &k510isp_subdev_notifier_ops;
	ret = v4l2_async_notifier_register(&isp->v4l2_dev, &isp->notifier);
	if (ret)
	{
		dev_err(&pdev->dev,"%s:v4l2_async_notifier_register ret:(%d)\n",__func__,ret);
		goto error_register_entities;
	}
	k510isp_core_init(isp, 1);
	k510isp_put(isp);

	dev_dbg(&pdev->dev,"%s:end\n",__func__);
	return 0;

error_register_entities:
	k510isp_unregister_entities(isp);
error_modules:
	k510isp_cleanup_modules(isp);
error_irq0:
	devm_free_irq(isp->dev,isp->irq_num[0],isp);	
error_iommu:
	k510isp_detach_iommu(isp);
error_isp:
	__k510isp_put(isp, false);
error:
	v4l2_async_notifier_cleanup(&isp->notifier);
	mutex_destroy(&isp->isp_mutex);	
	kfree(isp);
	dev_dbg(&pdev->dev,"%s:end ret:(%d)\n",__func__,ret);
	return ret;
}
/*-----------------------------------------------------------------------------
*Power management support
*-----------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*
 * Power management support.
 *
 * As the ISP can't properly handle an input video stream interruption on a non
 * frame boundary, the ISP pipelines need to be stopped before sensors get
 * suspended. However, as suspending the sensors can require a running clock,
 * which can be provided by the ISP, the ISP can't be completely suspended
 * before the sensor.
 *
 * To solve this problem power management support is split into prepare/complete
 * and suspend/resume operations. The pipelines are stopped in prepare() and the
 * ISP clocks get disabled in suspend(). Similarly, the clocks are reenabled in
 * resume(), and the the pipelines are restarted in complete().
 *
 * TODO: PM dependencies between the ISP and sensors are not modelled explicitly
 * yet.
 */
static int k510isp_pm_prepare(struct device *dev)
{
	struct k510_isp_device *isp = dev_get_drvdata(dev);
	int reset;

	WARN_ON(mutex_is_locked(&isp->isp_mutex));

	if (isp->ref_count == 0)
		return 0;

	reset = k510isp_suspend_modules(isp);
	k510isp_disable_interrupts(isp);
	k510isp_save_ctx(isp);
	if (reset)
		k510isp_reset(isp);

	return 0;
}
/**
 * @brief 
 * 
 * @param dev 
 * @return int 
 */
static int k510isp_pm_suspend(struct device *dev)
{
	struct k510_isp_device *isp = dev_get_drvdata(dev);

	WARN_ON(mutex_is_locked(&isp->isp_mutex));

	if (isp->ref_count)
		k510isp_disable_clocks(isp);

	return 0;
}
/**
 * @brief 
 * 
 * @param dev 
 * @return int 
 */
static int k510isp_pm_resume(struct device *dev)
{
	struct k510_isp_device *isp = dev_get_drvdata(dev);

	if (isp->ref_count == 0)
		return 0;

	return k510isp_enable_clocks(isp);
}
/**
 * @brief 
 * 
 * @param dev 
 */
static void k510isp_pm_complete(struct device *dev)
{
	struct k510_isp_device *isp = dev_get_drvdata(dev);

	if (isp->ref_count == 0)
		return;

	k510isp_restore_ctx(isp);
	k510isp_enable_interrupts(isp);
	k510isp_resume_modules(isp);
}

#else

#define k510isp_pm_prepare	NULL
#define k510isp_pm_suspend	NULL
#define k510isp_pm_resume	NULL
#define k510isp_pm_complete	NULL

#endif /* CONFIG_PM */
/**
 * @brief 
 * 
 */
static const struct dev_pm_ops k510isp_pm_ops = {
	.prepare = k510isp_pm_prepare,
	.suspend = k510isp_pm_suspend,
	.resume = k510isp_pm_resume,
	.complete = k510isp_pm_complete,
};
/*-----------------------------------------------------------------------------
*
*-----------------------------------------------------------------------------*/
/**
 * @brief 
 * 
 */
static struct platform_device_id k510isp_id_table[] = {
	{ "k510-isp1", 0 },
	{ },
};
MODULE_DEVICE_TABLE(platform, k510isp_id_table);
/**
 * @brief 
 * 
 */
static const struct of_device_id k510isp_of_table[] = {
	{ .compatible = "canaan,k510-isp1" },
	{ },
};
MODULE_DEVICE_TABLE(of, k510isp_of_table);
/**
 * @brief 
 * 
 */
static struct platform_driver k510isp_driver = {
	.probe = k510isp_probe,
	.remove = k510isp_remove,
	.id_table = k510isp_id_table,
	.driver = {
		.name = DRIVER_NAME,
		.pm	= &k510isp_pm_ops,
		.of_match_table = k510isp_of_table,
	},
};

module_platform_driver(k510isp_driver);

MODULE_DESCRIPTION("CANAAN K510 ISP driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(K510ISP_VIDEO_DRIVER_VERSION);
