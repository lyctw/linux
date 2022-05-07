/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/k510isp.h>

#include "isp_3a.h"
#include "../../k510_isp.h"
#include "../../k510isp_stat.h"
#include "../../isp_2k/core/isp_core_drv.h"

/*
 * f2k_awb_update_regs - Helper function to update h3a registers.
 */
static void f2k_awb_setup_regs(struct k510isp_stat *awb, void *priv)
{
	struct k510isp_awb_config *conf = priv;

	if (awb->state == ISPSTAT_DISABLED)
		return;

	if (!awb->update)
		return;

	/* Converting config metadata into reg values */
	union U_ISP_CORE_AWB_CTL stAwbCtl;
	stAwbCtl.u32 = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_CTL);
	stAwbCtl.bits.awb_en = conf->awb_en;
	stAwbCtl.bits.awb_mode_sel = conf->awb_md_sl;
	stAwbCtl.bits.awb_hist_mode_sel = conf->awb_win_sl;
	stAwbCtl.bits.awb_veri_en = conf->awb_vrf_en;
	stAwbCtl.bits.awb_fb_en = conf->awb_fb_en;
	stAwbCtl.bits.awb_value_save_en = conf->awb_sv_en;
	stAwbCtl.bits.awb_stab_en = conf->awb_sbz_en;
	isp_reg_writel(awb->isp,stAwbCtl.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_CTL);

	union U_ISP_CORE_AWB_WIN_H_START stHStart;
	stHStart.u32 = 0;
	stHStart.bits.awb_win_h_start = conf->awb_win_stth;
	isp_reg_writel(awb->isp,stHStart.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_WIN_H_START);

    union U_ISP_CORE_AWB_WIN_V_START stVStart;
	stVStart.u32 = 0;
	stVStart.bits.awb_win_v_start = conf->awb_win_endh;
	isp_reg_writel(awb->isp,stVStart.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_WIN_V_START);

	union U_ISP_CORE_AWB_WIN_H_END stHEnd;
	stHEnd.u32 = 0;
	stHEnd.bits.awb_win_h_end = conf->awb_win_sttv;
	isp_reg_writel(awb->isp,stHEnd.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_WIN_H_END);

	union U_ISP_CORE_AWB_WIN_V_END stVEnd;
	stVEnd.u32 = 0;
	stVEnd.bits.awb_win_v_end = conf->awb_win_endv;
	isp_reg_writel(awb->isp,stVEnd.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_WIN_V_END);

    union U_ISP_CORE_AWB_CORRECT_DIFF_TH stCorDiffTh;
	stCorDiffTh.u32 = 0;
	stCorDiffTh.bits.awb_correct_diff_th = conf->awb_fb_th;
	isp_reg_writel(awb->isp, stCorDiffTh.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_CORRECT_DIFF_TH);

	union U_ISP_CORE_AWB_RES_TIME stResTime;
	stResTime.u32 = 0;
	stResTime.bits.awb_color_changeres_time = conf->awb_exch_th;
	isp_reg_writel(awb->isp, stResTime.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_RES_TIME);
    
 	union U_ISP_CORE_AWB_HIST_TH stHistTh;
	stHistTh.u32 = 0;
	stHistTh.bits.awb_historgram_th = conf->awb_hist_th;
	isp_reg_writel(awb->isp, stHistTh.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_HIST_TH);

	union U_ISP_CORE_AWB_RED_GAIN_ADJUST stRedGainAd;
	stRedGainAd.u32 = 0;
	stRedGainAd.bits.awb_red_gain_adjust = conf->awb_rk;
	isp_reg_writel(awb->isp, stRedGainAd.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_RED_GAIN_ADJUST);

	union U_ISP_CORE_AWB_GREEN_GAIN_ADJUST stGreenGainAd;
	stGreenGainAd.u32 = 0;
	stGreenGainAd.bits.awb_green_gain_adjust = conf->awb_gk;
	isp_reg_writel(awb->isp, stGreenGainAd.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_GREEN_GAIN_ADJUST);

	union U_ISP_CORE_AWB_BLUE_GAIN_ADJUST stBlueGainAd;
	stBlueGainAd.u32 = 0;
	stBlueGainAd.bits.awb_blue_gain_adjust = conf->awb_bk;
	isp_reg_writel(awb->isp, stBlueGainAd.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BLUE_GAIN_ADJUST);

	union U_ISP_CORE_AWB_RED_MAX_VALUE stRedMaxValue;
	stRedMaxValue.u32 = 0;
	stRedMaxValue.bits.awb_red_max_value = conf->awb_rmax;
	isp_reg_writel(awb->isp, stRedMaxValue.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_RED_MAX_VALUE);

	union U_ISP_CORE_AWB_BLUE_MAX_VALUE stBlueMaxValue;
	stBlueMaxValue.u32 = 0;
	stBlueMaxValue.bits.awb_blue_max_value = conf->awb_bmax;
	isp_reg_writel(awb->isp, stBlueMaxValue.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BLUE_MAX_VALUE);

	union U_ISP_CORE_AWB_RED_MIN_VALUE stRedMinValue;
	stRedMinValue.u32 = 0;
	stRedMinValue.bits.awb_red_min_value = conf->awb_rmin;
	isp_reg_writel(awb->isp, stRedMinValue.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_RED_MIN_VALUE);

	union U_ISP_CORE_AWB_BLUE_MIN_VALUE stBlueMinValue;
	stBlueMinValue.u32 = 0;
	stBlueMinValue.bits.awb_blue_min_value = conf->awb_bmin;
	isp_reg_writel(awb->isp, stBlueMinValue.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BLUE_MIN_VALUE);	

	union U_ISP_CORE_AWB_RED_OBJ_VALUE stRedObjValue;
	stRedObjValue.u32 = 0;
	stRedObjValue.bits.awb_red_obj_value = conf->awb_r_obj;
	isp_reg_writel(awb->isp, stRedObjValue.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_RED_OBJ_VALUE);

	union U_ISP_CORE_AWB_BLUE_OBJ_VALUE stBlueObjValue;
	stBlueObjValue.u32 = 0;
	stBlueObjValue.bits.awb_blue_obj_value = conf->awb_b_obj;
	isp_reg_writel(awb->isp, stBlueObjValue.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BLUE_OBJ_VALUE);

	awb->update = 0;
	awb->config_counter += awb->inc_config;
	awb->inc_config = 0;
	//awb->buf_size = conf->buf_size;
}

static void f2k_awb_enable(struct k510isp_stat *awb, int enable)
{
	dev_info(awb->isp->dev,"%s:start\n",__func__);
	union U_ISP_CORE_AWB_CTL stAwbCtl;
	stAwbCtl.u32 = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_CTL);
	stAwbCtl.bits.awb_en = enable;
	isp_reg_writel(awb->isp,stAwbCtl.u32,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_CTL);
}

static int f2k_awb_get_stats(struct k510isp_stat *awb, void *priv)
{
	struct k510isp_awb_stats *awb_stats = priv;

  	awb_stats->bfb_pos = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BLUE_HIST_VALUE);
   	awb_stats->bfb_pot = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BLUE_HIST_PIXEL);
  	awb_stats->rfb_pos = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_RED_HIST_VALUE);
  	awb_stats->rfb_pot = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_RED_HIST_VALUE );
  	awb_stats->bin_pos = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BYPASS_BLUE_HIST_VALUE);
  	awb_stats->bin_pot = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BYPASS_BLUE_HIST_PIXEL);
  	awb_stats->rin_pos = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BYPASS_RED_HIST_VALUE);
  	awb_stats->rin_pot = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BYPASS_RED_HIST_PIXEL);
  	awb_stats->awb_ar = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_RED_VALUE);
  	awb_stats->awb_ag = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_GREEN_VALUE);
  	awb_stats->awb_ab = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_BLUE_VALUE);
  	awb_stats->awb_inr = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_ORG_RED_VALUE);
  	awb_stats->awb_ing = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_ORG_GREEN_VALUE);
  	awb_stats->awb_inb = isp_reg_readl(awb->isp,ISP_IOMEM_F2K_CORE,ISP_CORE_AWB_ORG_BLUE_VALUE);

	return 0;
}

static int f2k_awb_validate_params(struct k510isp_stat *awb, void *new_conf)
{
	struct k510isp_awb_config *user_cfg = new_conf;
	u32 buf_size;

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_win_stth,
			     K510ISP_WIN_HZSTART_MIN,
			     K510ISP_WIN_HZSTART_MAX))
	{
		dev_err(awb->isp->dev,"%s:awb_win_stth\n",__func__);
		return -EINVAL;
	}	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_win_endh,
			     K510ISP_WIN_HZSTART_MIN,
			     K510ISP_WIN_HZSTART_MAX))
	{
		dev_err(awb->isp->dev,"%s:awb_win_endh\n",__func__);
		return -EINVAL;
	}	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_win_sttv,
			     K510ISP_WIN_HZSTART_MIN,
			     K510ISP_WIN_HZSTART_MAX))
	{
		dev_err(awb->isp->dev,"%s:awb_win_sttv\n",__func__);
		return -EINVAL;
	}	
	
	if (IS_OUT_OF_BOUNDS(user_cfg->awb_win_endv,
			     K510ISP_WIN_HZSTART_MIN,
			     K510ISP_WIN_HZSTART_MAX))
	{
		dev_err(awb->isp->dev,"%s:awb_win_endv\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_fb_th,
			     0x10,
			     0xFF))
	{
		dev_err(awb->isp->dev,"%s:awb_fb_th\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_hist_th,
			     0x10,
			     0x80))
	{
		dev_err(awb->isp->dev,"%s:awb_hist_th\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_rk,
			     0x40,
			     0x3FF))
	{
		dev_err(awb->isp->dev,"%s:awb_rk\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_gk,
			     0x40,
			     0x3FF))
	{
		dev_err(awb->isp->dev,"%s:awb_gk\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_bk,
			     0x40,
			     0x3FF))
	{
		dev_err(awb->isp->dev,"%s:awb_bk\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_rmax,
			     0x200,
			     0x3FF))
	{
		dev_err(awb->isp->dev,"%s:awb_rmax\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_bmax,
			     0x200,
			     0x3FF))
	{
		dev_err(awb->isp->dev,"%s:awb_bmax\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_rmin,
			     0x40,
			     0x80))
	{
		dev_err(awb->isp->dev,"%s:awb_rmin\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_bmin,
			     0x40,
			     0x80))
	{
		dev_err(awb->isp->dev,"%s:awb_bmin\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_r_obj,
			     0x80,
			     0x1ff))
	{
		dev_err(awb->isp->dev,"%s:awb_r_obj\n",__func__);
		return -EINVAL;
	}	
	

	if (IS_OUT_OF_BOUNDS(user_cfg->awb_b_obj,
			     0x80,
			     0x1ff))
	{
		dev_err(awb->isp->dev,"%s:awb_b_obj\n",__func__);
		return -EINVAL;
	}	
	
	return 0;
}

/*
 * f2k_awb_set_params - Helper function to check & store user given params.
 * @new_conf: Pointer to AE and AWB parameters struct.
 *
 * As most of them are busy-lock registers, need to wait until AEW_BUSY = 0 to
 * program them during ISR.
 */
static void f2k_awb_set_params(struct k510isp_stat *awb, void *new_conf)
{
	struct k510isp_awb_config *user_cfg = new_conf;
	struct k510isp_awb_config *cur_cfg = awb->priv;
	int update = 0;

	if (cur_cfg->awb_en != user_cfg->awb_en) {
		cur_cfg->awb_en = user_cfg->awb_en;
		update = 1;
	}

	if (cur_cfg->awb_md_sl != user_cfg->awb_md_sl) {
		cur_cfg->awb_md_sl = user_cfg->awb_md_sl;
		update = 1;
	}

	if (cur_cfg->awb_win_sl != user_cfg->awb_win_sl) {
		cur_cfg->awb_win_sl = user_cfg->awb_win_sl;
		update = 1;
	}

	if (cur_cfg->awb_vrf_en != user_cfg->awb_vrf_en) {
		cur_cfg->awb_vrf_en = user_cfg->awb_vrf_en;
		update = 1;
	}

	if (cur_cfg->awb_fb_en != user_cfg->awb_fb_en) {
		cur_cfg->awb_fb_en = user_cfg->awb_fb_en;
		update = 1;
	}

	if (cur_cfg->awb_sv_en != user_cfg->awb_sv_en) {
		cur_cfg->awb_sv_en = user_cfg->awb_sv_en;
		update = 1;
	}

	if (cur_cfg->awb_sbz_en != user_cfg->awb_sbz_en) {
		cur_cfg->awb_sbz_en = user_cfg->awb_sbz_en;
		update = 1;
	}

	if (cur_cfg->awb_win_stth != user_cfg->awb_win_stth) {
		cur_cfg->awb_win_stth = user_cfg->awb_win_stth;
		update = 1;
	}

	if (cur_cfg->awb_win_endh != user_cfg->awb_win_endh) {
		cur_cfg->awb_win_endh = user_cfg->awb_win_endh;
		update = 1;
	}

	if (cur_cfg->awb_win_sttv != user_cfg->awb_win_sttv) {
		cur_cfg->awb_win_sttv = user_cfg->awb_win_sttv;
		update = 1;
	}

	if (cur_cfg->awb_win_endv != user_cfg->awb_win_endv) {
		cur_cfg->awb_win_endv = user_cfg->awb_win_endv;
		update = 1;
	}

	if (cur_cfg->awb_fb_th != user_cfg->awb_fb_th) {
		cur_cfg->awb_fb_th = user_cfg->awb_fb_th;
		update = 1;
	}

	if (cur_cfg->awb_exch_th != user_cfg->awb_exch_th) {
		cur_cfg->awb_exch_th = user_cfg->awb_exch_th;
		update = 1;
	}

	if (cur_cfg->awb_hist_th != user_cfg->awb_hist_th) {
		cur_cfg->awb_hist_th = user_cfg->awb_hist_th;
		update = 1;
	}

	if (cur_cfg->awb_rk != user_cfg->awb_rk) {
		cur_cfg->awb_rk = user_cfg->awb_rk;
		update = 1;
	}

	if (cur_cfg->awb_gk != user_cfg->awb_gk) {
		cur_cfg->awb_gk = user_cfg->awb_gk;
		update = 1;
	}

	if (cur_cfg->awb_bk != user_cfg->awb_bk) {
		cur_cfg->awb_bk = user_cfg->awb_bk;
		update = 1;
	}

	if (cur_cfg->awb_rmax != user_cfg->awb_rmax) {
		cur_cfg->awb_rmax = user_cfg->awb_rmax;
		update = 1;
	}

	if (cur_cfg->awb_bmax != user_cfg->awb_bmax) {
		cur_cfg->awb_bmax = user_cfg->awb_bmax;
		update = 1;
	}

	if (cur_cfg->awb_rmin != user_cfg->awb_rmin) {
		cur_cfg->awb_rmin = user_cfg->awb_rmin;
		update = 1;
	}

	if (cur_cfg->awb_bmin != user_cfg->awb_bmin) {
		cur_cfg->awb_bmin = user_cfg->awb_bmin;
		update = 1;
	}

	if (cur_cfg->awb_r_obj != user_cfg->awb_r_obj) {
		cur_cfg->awb_r_obj = user_cfg->awb_r_obj;
		update = 1;
	}

	if (cur_cfg->awb_b_obj != user_cfg->awb_b_obj) {
		cur_cfg->awb_b_obj = user_cfg->awb_b_obj;
		update = 1;
	}

	if (update || !awb->configured) {
		awb->inc_config++;
		awb->update = 1;
	}
}

static long f2k_awb_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct k510isp_stat *stat = v4l2_get_subdevdata(sd);

	switch (cmd) {
		case VIDIOC_K510ISP_F2K_AWB_CFG:
			return k510isp_stat_config(stat, arg);
		case VIDIOC_K510ISP_F2K_AWB_STAT_REQ:
			//return k510isp_stat_request_statistics(stat, arg);
			return f2k_awb_get_stats(stat, arg);
		case VIDIOC_K510ISP_F2K_AWB_STAT_EN: 
			{
				unsigned long *en = arg;
				return k510isp_stat_enable(stat, !!*en);
			}
	}

	return -ENOIOCTLCMD;
}

static const struct k510isp_stat_ops f2k_awb_ops = {
	.validate_params	= f2k_awb_validate_params,
	.set_params		= f2k_awb_set_params,
	.setup_regs		= f2k_awb_setup_regs,
	.enable			= f2k_awb_enable,
	.get_stats		= f2k_awb_get_stats,
};

static const struct v4l2_subdev_core_ops f2k_awb_subdev_core_ops = {
	.ioctl = f2k_awb_ioctl,
	.subscribe_event = k510isp_stat_subscribe_event,
	.unsubscribe_event = k510isp_stat_unsubscribe_event,
};

static const struct v4l2_subdev_video_ops f2k_awb_subdev_video_ops = {
	.s_stream = k510isp_stat_s_stream,
};

static const struct v4l2_subdev_ops f2k_awb_subdev_ops = {
	.core = &f2k_awb_subdev_core_ops,
	.video = &f2k_awb_subdev_video_ops,
};

/*
 * k510isp_f2k_awb_init - Module Initialisation.
 */
int k510isp_f2k_awb_init(struct k510_isp_device *isp)
{
	struct k510isp_stat *awb = &isp->isp_f2k_awb;
	struct k510isp_awb_config *awb_cfg;
	struct k510isp_awb_config *awb_recover_cfg;

	awb_cfg = devm_kzalloc(isp->dev, sizeof(*awb_cfg), GFP_KERNEL);
	if (!awb_cfg)
		return -ENOMEM;

	awb->ops = &f2k_awb_ops;
	awb->priv = awb_cfg;
	awb->event_type = V4L2_EVENT_K510ISP_F2K_AWB;
	awb->isp = isp;

	/* Set recover state configuration */
	awb_recover_cfg = devm_kzalloc(isp->dev, sizeof(*awb_recover_cfg),
					GFP_KERNEL);
	if (!awb_recover_cfg) {
		dev_err(awb->isp->dev,
			"awb: cannot allocate memory for recover configuration.\n");
		return -ENOMEM;
	}

	awb_recover_cfg->awb_en = 1;
	awb_recover_cfg->awb_md_sl = 0;
	awb_recover_cfg->awb_win_sl = 0;
	awb_recover_cfg->awb_vrf_en = 0;
	awb_recover_cfg->awb_fb_en = 0;
	awb_recover_cfg->awb_sv_en = 0;
	awb_recover_cfg->awb_sbz_en = 0;
	awb_recover_cfg->awb_win_stth = K510ISP_WIN_HZSTART_MIN;
	awb_recover_cfg->awb_win_endh = K510ISP_WIN_HZSTART_MAX;
	awb_recover_cfg->awb_win_sttv = K510ISP_WIN_VTSTART_MIN;
	awb_recover_cfg->awb_win_endv = K510ISP_WIN_VTSTART_MAX;
	awb_recover_cfg->awb_fb_th =0x20;
	awb_recover_cfg->awb_exch_th =0x08;
	awb_recover_cfg->awb_hist_th =0x40;
	awb_recover_cfg->awb_rk =0x100;
	awb_recover_cfg->awb_gk =0x100;
	awb_recover_cfg->awb_bk =0x100;
	awb_recover_cfg->awb_rmax =0x200;
	awb_recover_cfg->awb_bmax =0x3ff;
	awb_recover_cfg->awb_rmin =0x40;
	awb_recover_cfg->awb_bmin =0x80;
	awb_recover_cfg->awb_r_obj =0x120;
	awb_recover_cfg->awb_b_obj =0x120;

	if (f2k_awb_validate_params(awb, awb_recover_cfg)) {
		dev_err(awb->isp->dev,
			"awb: recover configuration is invalid.\n");
		return -EINVAL;
	}

	awb->recover_priv = awb_recover_cfg;

	return k510isp_stat_init(awb, "f2k_awb", &f2k_awb_subdev_ops);
}
/*
 * k510isp_f2k_awb_cleanup - Module exit.
 */
void k510isp_f2k_awb_cleanup(struct k510_isp_device *isp)
{
	k510isp_stat_cleanup(&isp->isp_f2k_awb);
}