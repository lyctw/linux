/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
/* Linux specific include files */
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/k510isp.h>

#include "isp_3a.h"
#include "../../k510_isp.h"
#include "../../k510isp_stat.h"
#include "../../isp_2k/core/isp_core_drv.h"



static void f2k_af_setup_regs(struct k510isp_stat *af, void *priv)
{
	struct k510isp_af_config *conf = priv;

	if (af->state == ISPSTAT_DISABLED)
		return;

	if (!af->update)
		return;

	/* Configure Hardware Registers */
    union U_ISP_CORE_AF_CTL stAfCtl;
	stAfCtl.u32 = 0;
    stAfCtl.bits.af_stat_en = conf->af_sts_en;
	stAfCtl.bits.af_stat_mode_sel = conf->af_sts_md;
	isp_reg_writel(af->isp,stAfCtl.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_CTL);

	union U_ISP_CORE_AF_STAT_WIN_H_START stAfWinHStart;
	stAfWinHStart.u32 = 0;
	stAfWinHStart.bits.af_stat_win_h_start = conf->af_win_stth;
	isp_reg_writel(af->isp,stAfWinHStart.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_STAT_WIN_H_START);

	union U_ISP_CORE_AF_STAT_WIN_V_START stAfWinVStart;
	stAfWinVStart.u32 = 0;
	stAfWinVStart.bits.af_stat_win_v_start = conf->af_win_sttv;
	isp_reg_writel(af->isp,stAfWinVStart.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_STAT_WIN_V_START);

	union U_ISP_CORE_AF_STAT_WIN_H_END stAfWinHEnd;
	stAfWinHEnd.u32 = 0;
	stAfWinHEnd.bits.af_stat_win_h_end = conf->af_win_endh;	
	isp_reg_writel(af->isp,stAfWinHEnd.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_STAT_WIN_H_END);

	union U_ISP_CORE_AF_STAT_WIN_V_END stAfWinVEnd;
	stAfWinVEnd.u32 = 0;
	stAfWinVEnd.bits.af_stat_win_v_end = conf->af_win_endv;	
	isp_reg_writel(af->isp,stAfWinVEnd.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_STAT_WIN_V_END);

	af->update = 0;
	af->config_counter += af->inc_config;
	af->inc_config = 0;
}
/**
 * @brief 
 * 
 * @param af 
 * @param enable 
 */
static void f2k_af_enable(struct k510isp_stat *af, int enable)
{
	union U_ISP_CORE_AF_CTL stAfCtl;
	stAfCtl.u32 = isp_reg_readl(af->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_CTL);
	stAfCtl.bits.af_stat_en = enable;
	isp_reg_writel(af->isp,stAfCtl.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_CTL);
}

static int f2k_af_get_stats(struct k510isp_stat *af, void *priv)
{
    struct k510isp_af_stats *af_stats = priv;

	af_stats->af_mf = isp_reg_readl(af->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_MID_FRQ_DATA);
	af_stats->af_hf = isp_reg_readl(af->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_HIGH_FRQ_DATA);
	af_stats->af_pxl = isp_reg_readl(af->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AF_STAT_PIXEL_NUM);	
	return 0;
}
/* Function to check paxel parameters */
static int f2k_af_validate_params(struct k510isp_stat *af, void *new_conf)
{
	struct k510isp_af_config *user_cfg = new_conf;
	int index;
	u32 buf_size;

	/* Check horizontal WIN */
	if (IS_OUT_OF_BOUNDS(user_cfg->af_win_stth,
			     K510ISP_WIN_HZSTART_MIN,
			     K510ISP_WIN_HZSTART_MAX))
	{
		dev_err(af->isp->dev,"%s:af_win_stth\n",__func__);
		return -EINVAL;
	}
	
	if (IS_OUT_OF_BOUNDS(user_cfg->af_win_endh,
			     user_cfg->af_win_stth,
			     K510ISP_WIN_HZSTART_MAX))
	{
		dev_err(af->isp->dev,"%s:af_win_endh\n",__func__);
		return -EINVAL;
	}
	
	if (IS_OUT_OF_BOUNDS(user_cfg->af_win_sttv,
			     K510ISP_WIN_VTSTART_MIN,
			     K510ISP_WIN_VTSTART_MAX))
	{
		dev_err(af->isp->dev,"%s:af_win_sttv\n",__func__);
		return -EINVAL;
	}
	
	if (IS_OUT_OF_BOUNDS(user_cfg->af_win_endv,
			     user_cfg->af_win_sttv,
			     K510ISP_WIN_VTSTART_MAX))
	{
		dev_err(af->isp->dev,"%s:af_win_endv\n",__func__);
		return -EINVAL;
	}
	

	return 0;
}

/* Update local parameters */
static void f2k_af_set_params(struct k510isp_stat *af, void *new_conf)
{
	struct k510isp_af_config *user_cfg = new_conf;
	struct k510isp_af_config *cur_cfg = af->priv;
	int update = 0;
	int index;

	/* alaw */
	if (cur_cfg->af_sts_en != user_cfg->af_sts_en) {
		update = 1;
		goto out;
	}

	/* paxel */
	if ((cur_cfg->af_win_stth != user_cfg->af_win_stth) ||
	    (cur_cfg->af_win_endh != user_cfg->af_win_endh) ||
	    (cur_cfg->af_win_sttv != user_cfg->af_win_sttv) ||
	    (cur_cfg->af_win_endv != user_cfg->af_win_endv)) {
		update = 1;
		goto out;
	}

	/* af_mode */
	if (cur_cfg->af_sts_md != user_cfg->af_sts_md)
		update = 1;

out:
	if (update || !af->configured) {
		memcpy(cur_cfg, user_cfg, sizeof(*cur_cfg));
		af->inc_config++;
		af->update = 1;
	}
}

static long f2k_af_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct k510isp_stat *stat = v4l2_get_subdevdata(sd);

	switch (cmd) {
	case VIDIOC_K510ISP_F2K_AF_CFG:
		return k510isp_stat_config(stat, arg);
	case VIDIOC_K510ISP_F2K_AF_STAT_REQ:
		//return k510isp_stat_request_statistics(stat, arg);
		return f2k_af_get_stats(stat, arg);
	case VIDIOC_K510ISP_F2K_AF_STAT_EN: {
		int *en = arg;
		return k510isp_stat_enable(stat, !!*en);
	}
	}

	return -ENOIOCTLCMD;

}

static const struct k510isp_stat_ops f2k_af_ops = {
	.validate_params	= f2k_af_validate_params,
	.set_params		= f2k_af_set_params,
	.setup_regs		= f2k_af_setup_regs,
	.enable			= f2k_af_enable,
	.get_stats			= f2k_af_get_stats,
};

static const struct v4l2_subdev_core_ops f2k_af_subdev_core_ops = {
	.ioctl = f2k_af_ioctl,
	.subscribe_event = k510isp_stat_subscribe_event,
	.unsubscribe_event = k510isp_stat_unsubscribe_event,
};

static const struct v4l2_subdev_video_ops f2k_af_subdev_video_ops = {
	.s_stream = k510isp_stat_s_stream,
};

static const struct v4l2_subdev_ops f2k_af_subdev_ops = {
	.core = &f2k_af_subdev_core_ops,
	.video = &f2k_af_subdev_video_ops,
};
/**
 * @brief 
 * Function to register the AF character device driver.
 */
int k510isp_f2k_af_init(struct k510_isp_device *isp)
{
	struct k510isp_stat *af = &isp->isp_f2k_af;
	struct k510isp_af_config *af_cfg;
	struct k510isp_af_config *af_recover_cfg;

	af_cfg = devm_kzalloc(isp->dev, sizeof(*af_cfg), GFP_KERNEL);
	if (af_cfg == NULL)
    {		
        dev_err(af->isp->dev,
			"F2K_AF: cannot allocate memory for configuration.\n");
		return -ENOMEM;
    }
	af->ops = &f2k_af_ops;
	af->priv = af_cfg;
	af->event_type = V4L2_EVENT_K510ISP_F2K_AF;
	af->isp = isp;

	/* Set recover state configuration */
	af_recover_cfg = devm_kzalloc(isp->dev, sizeof(*af_recover_cfg),
				      GFP_KERNEL);
	if (!af_recover_cfg) {
		dev_err(af->isp->dev,
			"F2K_AF: cannot allocate memory for recover configuration.\n");
		return -ENOMEM;
	}

	af_recover_cfg->af_sts_en = 0;
	af_recover_cfg->af_sts_md = 0;
	af_recover_cfg->af_win_sttv = K510ISP_WIN_HZSTART_MIN;
    af_recover_cfg->af_win_endh = K510ISP_WIN_HZSTART_MAX;
    af_recover_cfg->af_win_sttv = K510ISP_WIN_VTSTART_MIN;
    af_recover_cfg->af_win_endv = K510ISP_WIN_VTSTART_MAX;

	if (f2k_af_validate_params(af, af_recover_cfg)) {
		dev_err(af->isp->dev,
			"AF: recover configuration is invalid.\n");
		return -EINVAL;
	}

	af->recover_priv = af_recover_cfg;

	return k510isp_stat_init(af, "f2k_af", &f2k_af_subdev_ops);
}
/**
 * @brief 
 * 
 * @param isp 
 */
void k510isp_f2k_af_cleanup(struct k510_isp_device *isp)
{
	k510isp_stat_cleanup(&isp->isp_f2k_af);
}