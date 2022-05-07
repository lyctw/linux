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
 * r2k_ae_update_regs - Helper function to update h3a registers.
 */
static void r2k_ae_setup_regs(struct k510isp_stat *ae, void *priv)
{
	struct k510isp_ae_config *conf = priv;

	if (ae->state == ISPSTAT_DISABLED)
		return;

	if (!ae->update)
		return;

	/* Converting config metadata into reg values */
	union U_ISP_CORE_AE_CTL stAeCtl;
	stAeCtl.u32 = 0;
	stAeCtl.bits.ae_as_en = conf->ae_st_en;
	stAeCtl.bits.ae_ag_en = conf->ae_gain_en;
	stAeCtl.bits.ae_airis_en = conf->ae_iris_en;
	stAeCtl.bits.ae_enter_ls_sel = conf->ae_inlk_sl;
	stAeCtl.bits.ae_exit_ls_sel = conf->ae_otlk_sl;
	stAeCtl.bits.ae_win_mode_sel = conf->ae_win_sl;
	stAeCtl.bits.ae_back_light_mode_sel = conf->ae_md_sl;
	stAeCtl.bits.ae_day_change_en = conf->ae_dn_hd_en;
	stAeCtl.bits.ae_day_change_sel = conf->ae_dn_s;
	isp_reg_writel(ae->isp,stAeCtl.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_CTL);

	union U_ISP_CORE_AE_WIN_H_START stHStart;
	stHStart.u32 = 0;
	stHStart.bits.ae_win_h_start = conf->ae_win_stth;
	isp_reg_writel(ae->isp,stHStart.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_WIN_H_START);

	union U_ISP_CORE_AE_WIN_V_START stVStart;
	stVStart.u32 = 0;
	stVStart.bits.ae_win_v_start = conf->ae_win_sttv;
	isp_reg_writel(ae->isp,stVStart.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_WIN_V_START);

	union U_ISP_CORE_AE_WIN_H_END stHEnd;
	stHEnd.u32 = 0;
	stHEnd.bits.ae_win_h_end = conf->ae_win_endh;
	isp_reg_writel(ae->isp,stHEnd.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_WIN_H_END);

	union U_ISP_CORE_AE_WIN_V_END stVEnd;
	stVEnd.u32 = 0;
	stVEnd.bits.ae_win_v_end = conf->ae_win_endv;
	isp_reg_writel(ae->isp,stVEnd.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_WIN_V_END);

	union U_ISP_CORE_AE_TAR_BR stTarBr;
	stTarBr.u32 = 0;
	stTarBr.bits.ae_tar_bright = conf->ae_yobj;
	isp_reg_writel(ae->isp,stTarBr.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_TAR_BR);

	union U_ISP_CORE_AE_TAR_BR_RANGE stTarBrRange;
	stTarBrRange.u32 = 0;
	stTarBrRange.bits.ae_tar_bright_range = conf->ae_av_rg;
	isp_reg_writel(ae->isp,stTarBrRange.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_TAR_BR_RANGE);

	union U_ISP_CORE_AE_L_EX_TIME stLExTime;
	stLExTime.u32 = 0;
	stLExTime.bits.ae_l_ex_time = conf->ae_exp_l;
	isp_reg_writel(ae->isp,stLExTime.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_L_EX_TIME);

	union U_ISP_CORE_AE_M_EX_TIME stMExTime;
	stMExTime.u32 = 0;
	stMExTime.bits.ae_m_ex_time = conf->ae_exp_m;
	isp_reg_writel(ae->isp,stMExTime.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_M_EX_TIME);

	union U_ISP_CORE_AE_S_EX_TIME stSExTime;
	stSExTime.u32 = 0;
	stSExTime.bits.ae_s_ex_time = conf->ae_exp_s;
	isp_reg_writel(ae->isp,stSExTime.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_S_EX_TIME);

	union U_ISP_CORE_AE_AGC stAgc;
	stAgc.u32 = 0;
	stAgc.bits.ae_agc = conf->ae_agc;
	isp_reg_writel(ae->isp,stAgc.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_AGC);

	union U_ISP_CORE_AE_ADJUST_CTL stAdCtl;
	stAdCtl.u32 = 0;
	stAdCtl.bits.ae_ad_shuttle_freq = conf->ae_chg_cnt_exp;
	stAdCtl.bits.ae_ad_gain_freq = conf->ae_chg_cnt_agc;
	isp_reg_writel(ae->isp,stAdCtl.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_ADJUST_CTL);

	union U_ISP_CORE_AE_ADJUST_STEP_MAX stAdStepMax;
	stAdStepMax.u32 = 0;
	stAdStepMax.bits.ae_adjust_step_max = conf->ae_num_max;
	isp_reg_writel(ae->isp,stAdStepMax.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_ADJUST_STEP_MAX);

    union U_ISP_CORE_AE_EX_VALUE_MAX stExValueMax;
	stExValueMax.u32 = 0;
	stExValueMax.bits.ae_ex_value_max = conf->ae_exp_max;
	isp_reg_writel(ae->isp,stExValueMax.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_EX_VALUE_MAX);

	union U_ISP_CORE_AE_EX_VALUE_MID stExValueMid;
	stExValueMid.u32 = 0;
	stExValueMid.bits.ae_ex_value_mid = conf->ae_exp_mid;
	isp_reg_writel(ae->isp,stExValueMid.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_EX_VALUE_MID);

	union U_ISP_CORE_AE_EX_VALUE_MIN stExValueMin;
	stExValueMin.u32 = 0;
	stExValueMin.bits.ae_ex_value_min = conf->ae_exp_min;
	isp_reg_writel(ae->isp,stExValueMin.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_EX_VALUE_MIN);

    union U_ISP_CORE_AE_GAIN_MAX stGainMax;
	stGainMax.u32 = 0;
	stGainMax.bits.ae_gain_value_max = conf->ae_agc_max;
	isp_reg_writel(ae->isp,stGainMax.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_GAIN_MAX);

	union U_ISP_CORE_AE_GAIN_MID stGainMid;
	stGainMid.u32 = 0;
	stGainMid.bits.ae_gain_value_mid = conf->ae_agc_mid;
	isp_reg_writel(ae->isp,stGainMid.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_GAIN_MID);

	union U_ISP_CORE_AE_GAIN_MIN stGainMin;
	stGainMin.u32 = 0;
	stGainMin.bits.ae_gain_value_min = conf->ae_agc_min;
	isp_reg_writel(ae->isp,stGainMin.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_GAIN_MIN);

	union U_ISP_CORE_AE_DN_SWITCH_ADJUST_STEP_MAX stDnAdStepMax;
	stDnAdStepMax.u32 = 0;
	stDnAdStepMax.bits.ae_dn_switch_ad_step_max = conf->ae_dn_agcth;
	isp_reg_writel(ae->isp,stDnAdStepMax.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_DN_SWITCH_ADJUST_STEP_MAX);

	union U_ISP_CORE_AE_DN_SWITCH_WAIT_TIME stDnWaitTime;
	stDnWaitTime.u32 = 0;
	stDnWaitTime.bits.ae_dn_switch_wait_time = conf->ae_dn_tmth;
	isp_reg_writel(ae->isp,stDnWaitTime.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_DN_SWITCH_WAIT_TIME);
 
 	union U_ISP_CORE_APE_DIFF_MAX stApeDiffMax;
	stApeDiffMax.u32 = 0;
	stApeDiffMax.bits.ape_max_diff = conf->apt_co_max;
	isp_reg_writel(ae->isp,stApeDiffMax.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_APE_DIFF_MAX);

	union U_ISP_CORE_APE_DRV_SIGNAL_MAX stDriveSignalMax;
	stDriveSignalMax.u32 = 0;
	stDriveSignalMax.bits.ape_drv_signal_max = conf->apt_drv_max;
	isp_reg_writel(ae->isp,stDriveSignalMax.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_APE_DRV_SIGNAL_MAX);

	union U_ISP_CORE_APE_COEFF_DIS stApeCoeffDis;
	stApeCoeffDis.u32 = 0;
	stApeCoeffDis.bits.ape_coeff_distance = conf->apt_ki;
	isp_reg_writel(ae->isp,stApeCoeffDis.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_APE_COEFF_DIS);

	union U_ISP_CORE_APE_COEFF_SPEED stApeCoeffSpeed;
	stApeCoeffSpeed.u32 = 0;
	stApeCoeffSpeed.bits.ape_coeff_speed = conf->apt_kp;
	isp_reg_writel(ae->isp,stApeCoeffSpeed.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_APE_COEFF_SPEED);

	union U_ISP_CORE_APE_COEFF_ACCE stApeCoeffAcce;
	stApeCoeffAcce.u32 = 0;
	stApeCoeffAcce.bits.ape_coeff_acceleration = conf->apt_kd;
	isp_reg_writel(ae->isp,stApeCoeffAcce.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_APE_COEFF_ACCE);

	union U_ISP_CORE_APE_DRV_MANUAL_VALUE stApeDriManValue;
	stApeDriManValue.u32 = 0;
	stApeDriManValue.bits.ape_drv_manual_value = conf->apt_drv;
	isp_reg_writel(ae->isp,stApeDriManValue.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_APE_DRV_MANUAL_VALUE);

	union U_ISP_CORE_APE_DAMP_MANUAL_VALUE stDampManValue;
	stDampManValue.u32 = 0;
	stDampManValue.bits.ape_damp_manual_value = conf->apt_cnt;
	isp_reg_writel(ae->isp,stDampManValue.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_APE_DAMP_MANUAL_VALUE);	

	ae->update = 0;
	ae->config_counter += ae->inc_config;
	ae->inc_config = 0;
}

static void r2k_ae_enable(struct k510isp_stat *ae, int enable)
{
	union U_ISP_CORE_AE_CTL stAeCtl;
	stAeCtl.u32 = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_CTL);
	stAeCtl.bits.ae_as_en = enable;
	stAeCtl.bits.ae_ag_en = enable;
	stAeCtl.bits.ae_airis_en = enable;
	isp_reg_writel(ae->isp,stAeCtl.u32,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_CTL);
}

static int r2k_ae_get_stats(struct k510isp_stat *ae, void *priv)
{
    struct k510isp_ae_stats *ae_stats = priv;

  	ae_stats->ae_wren = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_VALUE_READY);
  	ae_stats->ae_expl = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_LONG_CUR_EX);
   	ae_stats->ae_expm = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_MID_CUR_EX);
  	ae_stats->ae_exps = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_SHORT_CUR_EX);
  	ae_stats->ae_agco = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_CUR_DIGITAL_GAIN);
  	ae_stats->y_av = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_CUR_AVE_BRIGHTNESS);
  	ae_stats->dn_st = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_CUR_DN_STATUS);
  	ae_stats->ae_exp_st = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_EX_STATUS);
  	ae_stats->ae_sum = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_SUM);
  	ae_stats->ae_pxl = isp_reg_readl(ae->isp,ISP_IOMEM_R2K_CORE,ISP_CORE_AE_PIXEL_SUM);
	return 0;
}

static int r2k_ae_validate_params(struct k510isp_stat *ae, void *new_conf)
{
	struct k510isp_ae_config *user_cfg = new_conf;

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_win_stth,
			     K510ISP_WIN_HZSTART_MIN,
			     K510ISP_WIN_HZSTART_MAX))
	{
		dev_err(ae->isp->dev,"%s:ae_win_stth\n",__func__);
		return -EINVAL;
	}	

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_win_endh,
			     user_cfg->ae_win_stth,
			     K510ISP_WIN_HZSTART_MAX))
	{
		dev_err(ae->isp->dev,"%s:ae_win_endh\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_win_sttv,
			     K510ISP_WIN_VTSTART_MIN,
			     K510ISP_WIN_VTSTART_MAX))
	{
		dev_err(ae->isp->dev,"%s:ae_win_sttv\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_win_endv,
			     user_cfg->ae_win_sttv,
			     K510ISP_WIN_VTSTART_MAX))
	{
		dev_err(ae->isp->dev,"%s:ae_win_endv\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_yobj,
			     0x20,
			     0xFF))
	{
		dev_err(ae->isp->dev,"%s:ae_yobj\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_av_rg,
			     0x0,
			     0xF))
	{
		dev_err(ae->isp->dev,"%s:ae_av_rg\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_exp_l,
			     0x1,
			     K510ISP_WIN_VTSTART_MAX))
	{
		dev_err(ae->isp->dev,"%s:ae_exp_l\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_exp_m,
			     0x1,
			     K510ISP_WIN_VTSTART_MAX))
	{
		dev_err(ae->isp->dev,"%s:ae_exp_m\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_exp_s,
			     0x1,
			     K510ISP_WIN_VTSTART_MAX))
	{
		dev_err(ae->isp->dev,"%s:ae_exp_s\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_agc,
			     0x0,
			     0xFFF))
	{
		dev_err(ae->isp->dev,"%s:ae_agc\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_chg_cnt_exp,
			     0x0,
			     0xF))
	{
		dev_err(ae->isp->dev,"%s:ae_chg_cnt_exp\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_chg_cnt_agc,
			     0x0,
			     0xF))
	{
		dev_err(ae->isp->dev,"%s:ae_chg_cnt_agc\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_num_max,
			     0x8,
			     0xFF))
	{
		dev_err(ae->isp->dev,"%s:ae_num_max\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_exp_max,
			     0x1,
			     K510ISP_WIN_VTSTART_MAX))
	{
		dev_err(ae->isp->dev,"%s:ae_exp_max\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_exp_mid,
			     0x1,
			     user_cfg->ae_exp_max-1))
	{
		dev_err(ae->isp->dev,"%s:ae_exp_mid\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_exp_min,
			     0x1,
			     user_cfg->ae_exp_mid-1))
	{
		dev_err(ae->isp->dev,"%s:ae_exp_min\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_agc_max,
			     0x0,
			     0xFFF))
	{
		dev_err(ae->isp->dev,"%s:ae_agc_max\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_agc_mid,
			     0x0,
			     user_cfg->ae_agc_max-1))
	{
		dev_err(ae->isp->dev,"%s:ae_agc_mid\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_agc_min,
			     0x0,
			     user_cfg->ae_agc_mid-1))
	{
		dev_err(ae->isp->dev,"%s:ae_agc_min\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->ae_dn_agcth,
			     0x0,
			     0xFFF))
	{
		dev_err(ae->isp->dev,"%s:ae_dn_agcth\n",__func__);
		return -EINVAL;
	}
	if (IS_OUT_OF_BOUNDS(user_cfg->ae_dn_tmth,
			     0x0,
			     0xFF))
	{
		dev_err(ae->isp->dev,"%s:ae_dn_tmth\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->apt_co_max,
			     0x0,
			     0xFF))
	{
		dev_err(ae->isp->dev,"%s:apt_co_max\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->apt_drv_max,
			     0x0,
			     0xFFF))
	{
		dev_err(ae->isp->dev,"%s:apt_drv_max\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->apt_ki,
			     0x0,
			     0x3F))
	{
		dev_err(ae->isp->dev,"%s:apt_ki\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->apt_kp,
			     0x0,
			     0x3F))
	{
		dev_err(ae->isp->dev,"%s:apt_kp\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->apt_kd,
			     0x0,
			     0x3F))
	{
		dev_err(ae->isp->dev,"%s:apt_kd\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->apt_drv,
			     0x0,
			     0xFFF))
	{
		dev_err(ae->isp->dev,"%s:apt_drv\n",__func__);
		return -EINVAL;
	}

	if (IS_OUT_OF_BOUNDS(user_cfg->apt_cnt,
			     0x0,
			     0xFFF))
	{
		dev_err(ae->isp->dev,"%s:apt_cnt\n",__func__);
		return -EINVAL;
	}

	return 0;
}
/*
 * r2k_ae_set_params - Helper function to check & store user given params.
 * @new_conf: Pointer to AE and ae parameters struct.
 *
 * As most of them are busy-lock registers, need to wait until AEW_BUSY = 0 to
 * program them during ISR.
 */
static void r2k_ae_set_params(struct k510isp_stat *ae, void *new_conf)
{
	struct k510isp_ae_config *user_cfg = new_conf;
	struct k510isp_ae_config *cur_cfg = ae->priv;
	int update = 0;

	if (cur_cfg->ae_st_en != user_cfg->ae_st_en) {
		cur_cfg->ae_st_en = user_cfg->ae_st_en;
		update = 1;
	}

	if (cur_cfg->ae_gain_en != user_cfg->ae_gain_en) {
		cur_cfg->ae_gain_en = user_cfg->ae_gain_en;
		update = 1;
	}

	if (cur_cfg->ae_iris_en != user_cfg->ae_iris_en) {
		cur_cfg->ae_iris_en = user_cfg->ae_iris_en;
		update = 1;
	}

	if (cur_cfg->ae_inlk_sl != user_cfg->ae_inlk_sl) {
		cur_cfg->ae_inlk_sl = user_cfg->ae_inlk_sl;
		update = 1;
	}

	if (cur_cfg->ae_otlk_sl != user_cfg->ae_otlk_sl) {
		cur_cfg->ae_otlk_sl = user_cfg->ae_otlk_sl;
		update = 1;
	}

	if (cur_cfg->ae_win_sl != user_cfg->ae_win_sl) {
		cur_cfg->ae_win_sl = user_cfg->ae_win_sl;
		update = 1;
	}

	if (cur_cfg->ae_md_sl != user_cfg->ae_md_sl) {
		cur_cfg->ae_md_sl = user_cfg->ae_md_sl;
		update = 1;
	}

	if (cur_cfg->ae_dn_hd_en != user_cfg->ae_dn_hd_en) {
		cur_cfg->ae_dn_hd_en = user_cfg->ae_dn_hd_en;
		update = 1;
	}

	if (cur_cfg->ae_dn_s != user_cfg->ae_dn_s) {
		cur_cfg->ae_dn_s = user_cfg->ae_dn_s;
		update = 1;
	}

	if (cur_cfg->ae_win_stth != user_cfg->ae_win_stth) {
		cur_cfg->ae_win_stth = user_cfg->ae_win_stth;
		update = 1;
	}

	if (cur_cfg->ae_win_endh != user_cfg->ae_win_endh) {
		cur_cfg->ae_win_endh = user_cfg->ae_win_endh;
		update = 1;
	}
 
	if (cur_cfg->ae_win_sttv != user_cfg->ae_win_sttv) {
		cur_cfg->ae_win_sttv = user_cfg->ae_win_sttv;
		update = 1;
	}

	if (cur_cfg->ae_win_endv != user_cfg->ae_win_endv) {
		cur_cfg->ae_win_endv = user_cfg->ae_win_endv;
		update = 1;
	}  

	if (cur_cfg->ae_yobj != user_cfg->ae_yobj) {
		cur_cfg->ae_yobj = user_cfg->ae_yobj;
		update = 1;
	}

	if (cur_cfg->ae_av_rg != user_cfg->ae_av_rg) {
		cur_cfg->ae_av_rg = user_cfg->ae_av_rg;
		update = 1;
	} 

	if (cur_cfg->ae_exp_l != user_cfg->ae_exp_l) {
		cur_cfg->ae_exp_l = user_cfg->ae_exp_l;
		update = 1;
	} 

	if (cur_cfg->ae_exp_m != user_cfg->ae_exp_m) {
		cur_cfg->ae_exp_m = user_cfg->ae_exp_m;
		update = 1;
	} 

	if (cur_cfg->ae_exp_s != user_cfg->ae_exp_s) {
		cur_cfg->ae_exp_s = user_cfg->ae_exp_s;
		update = 1;
	} 

	if (cur_cfg->ae_agc != user_cfg->ae_agc) {
		cur_cfg->ae_agc = user_cfg->ae_agc;
		update = 1;
	} 

	if (cur_cfg->ae_chg_cnt_exp != user_cfg->ae_chg_cnt_exp) {
		cur_cfg->ae_chg_cnt_exp = user_cfg->ae_chg_cnt_exp;
		update = 1;
	} 

	if (cur_cfg->ae_chg_cnt_agc != user_cfg->ae_chg_cnt_agc) {
		cur_cfg->ae_chg_cnt_agc = user_cfg->ae_chg_cnt_agc;
		update = 1;
	} 

	if (cur_cfg->ae_num_max != user_cfg->ae_num_max) {
		cur_cfg->ae_num_max = user_cfg->ae_num_max;
		update = 1;
	} 

	if (cur_cfg->ae_exp_max != user_cfg->ae_exp_max) {
		cur_cfg->ae_exp_max = user_cfg->ae_exp_max;
		update = 1;
	} 

	if (cur_cfg->ae_exp_mid != user_cfg->ae_exp_mid) {
		cur_cfg->ae_exp_mid = user_cfg->ae_exp_mid;
		update = 1;
	} 

	if (cur_cfg->ae_exp_min != user_cfg->ae_exp_min) {
		cur_cfg->ae_exp_min = user_cfg->ae_exp_min;
		update = 1;
	} 

	if (cur_cfg->ae_agc_max != user_cfg->ae_agc_max) {
		cur_cfg->ae_agc_max = user_cfg->ae_agc_max;
		update = 1;
	}

	if (cur_cfg->ae_agc_mid != user_cfg->ae_agc_mid) {
		cur_cfg->ae_agc_mid = user_cfg->ae_agc_mid;
		update = 1;
	}

	if (cur_cfg->ae_agc_min != user_cfg->ae_agc_min) {
		cur_cfg->ae_agc_min = user_cfg->ae_agc_min;
		update = 1;
	}

	if (cur_cfg->ae_dn_agcth != user_cfg->ae_dn_agcth) {
		cur_cfg->ae_dn_agcth = user_cfg->ae_dn_agcth;
		update = 1;
	}

	if (cur_cfg->ae_dn_tmth != user_cfg->ae_dn_tmth) {
		cur_cfg->ae_dn_tmth = user_cfg->ae_dn_tmth;
		update = 1;
	}

	if (cur_cfg->apt_co_max != user_cfg->apt_co_max) {
		cur_cfg->apt_co_max = user_cfg->apt_co_max;
		update = 1;
	}

	if (cur_cfg->apt_drv_max != user_cfg->apt_drv_max) {
		cur_cfg->apt_drv_max = user_cfg->apt_drv_max;
		update = 1;
	}

	if (cur_cfg->apt_ki != user_cfg->apt_ki) {
		cur_cfg->apt_ki = user_cfg->apt_ki;
		update = 1;
	}

	if (cur_cfg->apt_kp != user_cfg->apt_kp) {
		cur_cfg->apt_kp = user_cfg->apt_kp;
		update = 1;
	}

	if (cur_cfg->apt_kd != user_cfg->apt_kd) {
		cur_cfg->apt_kd = user_cfg->apt_kd;
		update = 1;
	}

	if (cur_cfg->apt_drv != user_cfg->apt_drv) {
		cur_cfg->apt_drv = user_cfg->apt_drv;
		update = 1;
	}

	if (cur_cfg->apt_cnt != user_cfg->apt_cnt) {
		cur_cfg->apt_cnt = user_cfg->apt_cnt;
		update = 1;
	}

	if (update || !ae->configured) {
		ae->inc_config++;
		ae->update = 1;
	}
}

static const struct k510isp_stat_ops r2k_ae_ops = {
	.validate_params	= r2k_ae_validate_params,
	.set_params		= r2k_ae_set_params,
	.setup_regs		= r2k_ae_setup_regs,
	.enable			= r2k_ae_enable,
	.get_stats		= r2k_ae_get_stats,
};
/****************************************************************
 * 
 * **************************************************************/
static long r2k_ae_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct k510isp_stat *stat = v4l2_get_subdevdata(sd);

	switch (cmd) {
	    case VIDIOC_K510ISP_R2K_AE_CFG:
	    	return k510isp_stat_config(stat, arg);
	    case VIDIOC_K510ISP_R2K_AE_STAT_REQ:
	    	//return k510isp_stat_request_statistics(stat, arg);
			return r2k_ae_get_stats(stat, arg);
	    case VIDIOC_K510ISP_R2K_AE_STAT_EN: 
        {
		    unsigned long *en = arg;
		    return k510isp_stat_enable(stat, !!*en);
	    }
	}

	return -ENOIOCTLCMD;
}

static const struct v4l2_subdev_core_ops r2k_ae_subdev_core_ops = {
	.ioctl = r2k_ae_ioctl,
	.subscribe_event = k510isp_stat_subscribe_event,
	.unsubscribe_event = k510isp_stat_unsubscribe_event,
};

static const struct v4l2_subdev_video_ops r2k_ae_subdev_video_ops = {
	.s_stream = k510isp_stat_s_stream,
};

static const struct v4l2_subdev_ops r2k_ae_subdev_ops = {
	.core = &r2k_ae_subdev_core_ops,
	.video = &r2k_ae_subdev_video_ops,
};
/****************************************************************
 * 
 ***************************************************************/
/*
 * k510isp_r2k_ae_init - Module Initialisation.
 */
int k510isp_r2k_ae_init(struct k510_isp_device *isp)
{
	struct k510isp_stat *ae = &isp->isp_r2k_ae;
	struct k510isp_ae_config *ae_cfg;
	struct k510isp_ae_config *ae_recover_cfg;

	ae_cfg = devm_kzalloc(isp->dev, sizeof(*ae_cfg), GFP_KERNEL);
	if (!ae_cfg)
		return -ENOMEM;

	ae->ops = &r2k_ae_ops;
	ae->priv = ae_cfg;
	ae->event_type = V4L2_EVENT_K510ISP_R2K_AE;
	ae->isp = isp;

	/* Set recover state configuration */
	ae_recover_cfg = devm_kzalloc(isp->dev, sizeof(*ae_recover_cfg),
					GFP_KERNEL);
	if (!ae_recover_cfg) {
		dev_err(ae->isp->dev,
			"r2k_ae: cannot allocate memory for recover configuration.\n");
		return -ENOMEM;
	}

    ae_recover_cfg->ae_st_en = 1;
    ae_recover_cfg->ae_gain_en = 1;
    ae_recover_cfg->ae_iris_en = 0;
    ae_recover_cfg->ae_inlk_sl = 0;
    ae_recover_cfg->ae_otlk_sl = 0;
    ae_recover_cfg->ae_win_sl = 0;
    ae_recover_cfg->ae_md_sl = 0;
    ae_recover_cfg->ae_dn_hd_en = 0;
    ae_recover_cfg->ae_dn_s = 0;
    //
    ae_recover_cfg->ae_win_stth = K510ISP_WIN_HZSTART_MIN;
    ae_recover_cfg->ae_win_endh = K510ISP_WIN_HZSTART_MAX;
    ae_recover_cfg->ae_win_sttv = K510ISP_WIN_VTSTART_MIN;
    ae_recover_cfg->ae_win_endv = K510ISP_WIN_VTSTART_MAX;
    //
    ae_recover_cfg->ae_yobj = 0x80;
    ae_recover_cfg->ae_av_rg = 0x8;
    //
    ae_recover_cfg->ae_exp_l = 0x3e8;
    ae_recover_cfg->ae_exp_m = 32;
    ae_recover_cfg->ae_exp_s = 32;
    //
    ae_recover_cfg->ae_agc = 0x0; 
    ae_recover_cfg->ae_chg_cnt_exp = 0x1;
    ae_recover_cfg->ae_chg_cnt_agc = 0x1;
    ae_recover_cfg->ae_num_max = 0x20;
    ae_recover_cfg->ae_exp_max = 0x437;//0x840;//
    ae_recover_cfg->ae_exp_mid = 0x108;
    ae_recover_cfg->ae_exp_min = 0x100;
    ae_recover_cfg->ae_agc_max = 0x900;
    ae_recover_cfg->ae_agc_mid = 0x200;
    ae_recover_cfg->ae_agc_min = 0x0;
    ae_recover_cfg->ae_dn_agcth = 0x200;
    ae_recover_cfg->ae_dn_tmth = 0xff;
    ae_recover_cfg->apt_co_max = 0x0c;
    ae_recover_cfg->apt_drv_max = 0xf00;
	ae_recover_cfg->apt_ki = 0x0;
    ae_recover_cfg->apt_kp = 0x0;
    ae_recover_cfg->apt_kd = 0x0;
    ae_recover_cfg->apt_drv = 0xfff;
    ae_recover_cfg->apt_cnt = 0x800;

	if (r2k_ae_validate_params(ae, ae_recover_cfg)) {
		dev_err(ae->isp->dev,
			"ae: recover configuration is invalid.\n");
		return -EINVAL;
	}

	ae->recover_priv = ae_recover_cfg;

	return k510isp_stat_init(ae, "r2k_ae", &r2k_ae_subdev_ops);
}
/*
 * k510isp_r2k_ae_cleanup - Module exit.
 */
void k510isp_r2k_ae_cleanup(struct k510_isp_device *isp)
{
	k510isp_stat_cleanup(&isp->isp_r2k_ae);
}