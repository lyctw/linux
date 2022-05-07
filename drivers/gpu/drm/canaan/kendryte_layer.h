/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _KENDRYTE_LAYER_H_
#define _KENDRYTE_LAYER_H_


#define KENDRYTE_MAX_FB_BUFFER	3
#define KENDRYTE_MAX_CONNECTOR	2
#define KENDRYTE_MAX_CRTC	2


struct vop_zpos {
	int win_id;
	int zpos;
};

struct kendryte_layer_state {

    struct drm_plane_state  state;
    unsigned int            pipe;
    bool                    uses_frontend;

    int format;
	int zpos;
    dma_addr_t yrgb_mst;
	dma_addr_t uv_mst;

    struct drm_rect src;
	struct drm_rect dest;

    int color_space;
    int eotf;
    int global_alpha;
    int blend_mode;
    int async_commit;

};


enum vop_data_format {
	VOP_FMT_ARGB8888 = 0,
	VOP_FMT_RGB888,
	VOP_FMT_RGB565,
	VOP_FMT_ARGB4444,
	VOP_FMT_ARGB1555,
	VOP_FMT_YUYV,
	VOP_FMT_YUV420SP,
	VOP_FMT_YUV422SP,
	VOP_FMT_YUV444SP,
};


/*
 * kendryte drm private crtc funcs.
 * @loader_protect: protect loader logo crtc's power
 * @enable_vblank: enable crtc vblank irq.
 * @disable_vblank: disable crtc vblank irq.
 * @bandwidth: report present crtc bandwidth consume.
 */
struct kendryte_crtc_funcs {
	int (*loader_protect)(struct drm_crtc *crtc, bool on);
	int (*enable_vblank)(struct drm_crtc *crtc);
	void (*disable_vblank)(struct drm_crtc *crtc);
	size_t (*bandwidth)(struct drm_crtc *crtc,
			    struct drm_crtc_state *crtc_state,
			    unsigned int *plane_num_total);
	void (*cancel_pending_vblank)(struct drm_crtc *crtc,
				      struct drm_file *file_priv);
	int (*debugfs_init)(struct drm_minor *minor, struct drm_crtc *crtc);
	int (*debugfs_dump)(struct drm_crtc *crtc, struct seq_file *s);
	void (*regs_dump)(struct drm_crtc *crtc, struct seq_file *s);
	enum drm_mode_status (*mode_valid)(struct drm_crtc *crtc,
					   const struct drm_display_mode *mode,
					   int output_type);
	void (*crtc_close)(struct drm_crtc *crtc);
	void (*crtc_send_mcu_cmd)(struct drm_crtc *crtc, u32 type, u32 value);
};



struct kendryte_atomic_commit {
	struct drm_atomic_state *state;
	struct drm_device *dev;
	size_t bandwidth;
	unsigned int plane_num;
};


struct kendryte_logo {
	dma_addr_t dma_addr;
	void *kvaddr;
	phys_addr_t start;
	phys_addr_t size;
	int count;
};

/*
 * kendryte drm private structure.
 *
 * @crtc: array of enabled CRTCs, used to map from "pipe" to drm_crtc.
 * @num_pipe: number of pipes for this device.
 * @mm_lock: protect drm_mm on multi-threads.
 */
struct kendryte_drm_private {
	struct kendryte_logo *logo;
	struct drm_property *eotf_prop;
	struct drm_property *color_space_prop;
	struct drm_property *global_alpha_prop;
	struct drm_property *blend_mode_prop;
	struct drm_property *alpha_scale_prop;
	struct drm_property *async_commit_prop;
	struct drm_property *share_id_prop;
	struct drm_fb_helper *fbdev_helper;
	struct drm_gem_object *fbdev_bo;
	const struct kendryte_crtc_funcs *crtc_funcs[KENDRYTE_MAX_CRTC];
	struct drm_atomic_state *state;

	struct kendryte_atomic_commit *commit;

    struct drm_fbdev_cma *fbdev;

    
	/* protect async commit */
	struct mutex commit_lock;
	struct work_struct commit_work;
	struct gen_pool *secure_buffer_pool;
	/* protect drm_mm on multi-threads */
	struct mutex mm_lock;
	struct drm_mm mm;
//	struct rockchip_dclk_pll default_pll;
//	struct rockchip_dclk_pll hdmi_pll;
	struct devfreq *devfreq;
	u8 dmc_support;
	struct list_head psr_list;
	struct mutex psr_list_lock;

	/**
	 * @loader_protect
	 * ignore restore_fbdev_mode_atomic when in logo on state
	 */
	bool loader_protect;
};


struct kendryte_crtc_state {
	struct drm_crtc_state base;
	struct drm_tv_connector_state *tv_state;
	int left_margin;
	int right_margin;
	int top_margin;
	int bottom_margin;
	int afbdc_win_format;
	int afbdc_win_width;
	int afbdc_win_height;
	int afbdc_win_ptr;
	int afbdc_win_id;
	int afbdc_en;
	int afbdc_win_vir_width;
	int afbdc_win_xoffset;
	int afbdc_win_yoffset;
	int dsp_layer_sel;
	int output_type;
	int output_mode;
	int output_bpc;
	int output_flags;
	u32 bus_format;
	u32 bus_flags;
	int yuv_overlay;
	int post_r2y_en;
	int post_y2r_en;
	int post_csc_mode;
	int bcsh_en;
	int color_space;
	int eotf;
	u8 mode_update;
//	struct rockchip_hdr_state hdr;
};

#define to_kendryte_crtc_state(s) \
		container_of(s, struct kendryte_crtc_state, base)


static inline struct kendryte_layer_state *
state_to_kendryte_layer_state(struct drm_plane_state *state)
{
    return container_of(state, struct kendryte_layer_state, state);
}



extern struct platform_driver kendryte_layer_driver;

/*
struct drm_plane **kendryte_layers_init(struct drm_device *drm,
                                     struct sunxi_engine *engine);
*/
#endif 
