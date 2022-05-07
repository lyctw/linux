/*
 * Copyright (C) 2022, Canaan Bright Sight Co., Ltd
 * Copyright (C) Fuzhou Rockchip Electronics Co.Ltd
 * Author:Mark Yao <mark.yao@rock-chips.com>
 *
 * based on exynos_drm_drv.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <drm/drm.h>
#include <drm/drmP.h>
#include <drm/drm_atomic.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_fourcc.h>

#include <drm/drm_flip_work.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>

#ifdef CONFIG_DRM_ANALOGIX_DP
#include <drm/bridge/analogix_dp.h>
#endif

#include <linux/debugfs.h>
#include <linux/fixp-arith.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/component.h>
#include <linux/overflow.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

#include <linux/reset.h>
#include <linux/delay.h>
#include <linux/sort.h>

#include <uapi/linux/videodev2.h>

#include "kendryte_layer.h"
#include "kendryte_vo.h"




static int __maybe_unused
kendryte_atomic_helper_update_plane(struct drm_plane *plane,
				    struct drm_crtc *crtc,
				    struct drm_framebuffer *fb,
				    int crtc_x, int crtc_y,
				    unsigned int crtc_w, unsigned int crtc_h,
				    uint32_t src_x, uint32_t src_y,
				    uint32_t src_w, uint32_t src_h,
				    struct drm_modeset_acquire_ctx *ctx)
{
    struct drm_atomic_state *state;
	struct drm_plane_state *plane_state;
	struct kendryte_layer_state *vop_plane_state;
	int ret = 0;

	//printk("rockchip_atomic_helper_update_plane start \n");
	state = drm_atomic_state_alloc(plane->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = ctx;
	plane_state = drm_atomic_get_plane_state(state, plane);
	if (IS_ERR(plane_state)) {
		ret = PTR_ERR(plane_state);
		goto fail;
	}

    vop_plane_state = state_to_kendryte_layer_state(plane_state);

    ret = drm_atomic_set_crtc_for_plane(plane_state, crtc);
	if (ret != 0)
		goto fail;

    drm_atomic_set_fb_for_plane(plane_state, fb);
	plane_state->crtc_x = crtc_x;
	plane_state->crtc_y = crtc_y;
	plane_state->crtc_w = crtc_w;
	plane_state->crtc_h = crtc_h;
	plane_state->src_x = src_x;
	plane_state->src_y = src_y;
	plane_state->src_w = src_w;
	plane_state->src_h = src_h;

	ret = drm_atomic_commit(state);
fail:
	drm_atomic_state_put(state);
	return ret;
}


/**
 * drm_atomic_helper_disable_plane copy from drm_atomic_helper_disable_plane
 * be designed to support async commit at ioctl DRM_IOCTL_MODE_SETPLANE.
 *
 * @plane: plane to disable
 * @ctx: lock acquire context
 *
 * Provides a default plane disable handler using the atomic driver interface.
 *
 * RETURNS:
 * Zero on success, error code on failure
 */
static int __maybe_unused
kendryte_atomic_helper_disable_plane(struct drm_plane *plane,
				     struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_atomic_state *state;
	struct drm_plane_state *plane_state;
	struct kendryte_layer_state *vop_plane_state;
	int ret = 0;

	state = drm_atomic_state_alloc(plane->dev);
	if (!state)
		return -ENOMEM;

	state->acquire_ctx = ctx;
	plane_state = drm_atomic_get_plane_state(state, plane);
	if (IS_ERR(plane_state)) {
		ret = PTR_ERR(plane_state);
		goto fail;
	}
	vop_plane_state = state_to_kendryte_layer_state(plane_state);

	if ((plane_state->crtc && plane_state->crtc->cursor == plane) ||
	    vop_plane_state->async_commit)
		plane_state->state->legacy_cursor_update = true;
		
/*
	ret = __drm_atomic_helper_disable_plane(plane, plane_state);
	if (ret != 0)
		goto fail;
*/
	ret = drm_atomic_commit(state);
fail:
	drm_atomic_state_put(state);
	return ret;
}


static void kendryte_plane_destroy(struct drm_plane *plane)
{
	drm_plane_cleanup(plane);
}


static void kendryte_destroy_crtc(struct kendryte_vo *vop)
{
	struct drm_crtc *crtc = &vop->crtc;
	struct drm_device *drm_dev = vop->drm;
	struct drm_plane *plane, *tmp;

	of_node_put(crtc->port);

	/*
	 * We need to cleanup the planes now.  Why?
	 *
	 * The planes are "&vop->win[i].base".  That means the memory is
	 * all part of the big "struct vop" chunk of memory.  That memory
	 * was devm allocated and associated with this component.  We need to
	 * free it ourselves before vop_unbind() finishes.
	 */
	list_for_each_entry_safe(plane, tmp, &drm_dev->mode_config.plane_list,
				 head)
		kendryte_plane_destroy(plane);

	/*
	 * Destroy CRTC after vop_plane_destroy() since vop_disable_plane()
	 * references the CRTC.
	 */
	drm_crtc_cleanup(crtc);

}

static void kendryte_atomic_plane_reset(struct drm_plane *plane)
{
	struct kendryte_layer_state *vop_plane_state =
					state_to_kendryte_layer_state(plane->state);

	if (plane->state && plane->state->fb)
		__drm_atomic_helper_plane_destroy_state(plane->state);
	kfree(vop_plane_state);
	vop_plane_state = kzalloc(sizeof(*vop_plane_state), GFP_KERNEL);
	if (!vop_plane_state)
		return;


	vop_plane_state->global_alpha = 0xff;

	plane->state = &vop_plane_state->state;
	plane->state->plane = plane;
}


static struct drm_plane_state *
kendryte_atomic_plane_duplicate_state(struct drm_plane *plane)
{
	struct kendryte_layer_state *old_vop_plane_state;
	struct kendryte_layer_state *vop_plane_state;

	if (WARN_ON(!plane->state))
		return NULL;
	
//	printk("kendryte_atomic_plane_duplicate_state \n");
	old_vop_plane_state = state_to_kendryte_layer_state(plane->state);
	vop_plane_state = kmemdup(old_vop_plane_state,
				  sizeof(*vop_plane_state), GFP_KERNEL);
	if (!vop_plane_state)
		return NULL;

	__drm_atomic_helper_plane_duplicate_state(plane,
						  &vop_plane_state->state);

	return &vop_plane_state->state;
}


static void kendryte_atomic_plane_destroy_state(struct drm_plane *plane,
					   struct drm_plane_state *state)
{
	struct kendryte_layer_state *vop_state = state_to_kendryte_layer_state(state);

	__drm_atomic_helper_plane_destroy_state(state);

	kfree(vop_state);
}

static int kendryte_atomic_plane_set_property(struct drm_plane *plane,
					 struct drm_plane_state *state,
					 struct drm_property *property,
					 uint64_t val)
{
	struct kendryte_drm_private *private = plane->dev->dev_private;
//	struct vop_win *win = to_vop_win(plane);
	struct kendryte_layer_state *plane_state = state_to_kendryte_layer_state(state);

/*
	//printk("vop_atomic_plane_set_property \n");
	if (property == win->vop->plane_zpos_prop) {
		plane_state->zpos = val;
		return 0;
	}
*/

	if (property == private->eotf_prop) {
		plane_state->eotf = val;
		return 0;
	}

	if (property == private->color_space_prop) {
		plane_state->color_space = val;
		return 0;
	}

	if (property == private->global_alpha_prop) {
		plane_state->global_alpha = val;
		return 0;
	}

	if (property == private->blend_mode_prop) {
		plane_state->blend_mode = val;
		return 0;
	}

	if (property == private->async_commit_prop) {
		plane_state->async_commit = val;
		return 0;
	}

	DRM_ERROR("failed to set vop plane property id:%d, name:%s\n",
		   property->base.id, property->name);

	return -EINVAL;
}



static int kendryte_atomic_plane_get_property(struct drm_plane *plane,
					 const struct drm_plane_state *state,
					 struct drm_property *property,
					 uint64_t *val)
{
	struct kendryte_layer_state *plane_state = state_to_kendryte_layer_state(state);
//	struct vop_win *win = to_vop_win(plane);
	struct kendryte_drm_private *private = plane->dev->dev_private;

	//printk("vop_atomic_plane_get_property \n");
/*
	if (property == win->vop->plane_zpos_prop) {
		*val = plane_state->zpos;
		return 0;
	}
*/

	if (property == private->eotf_prop) {
		*val = plane_state->eotf;
		return 0;
	}

	if (property == private->color_space_prop) {
		*val = plane_state->color_space;
		return 0;
	}

	if (property == private->global_alpha_prop) {
		*val = plane_state->global_alpha;
		return 0;
	}

	if (property == private->blend_mode_prop) {
		*val = plane_state->blend_mode;
		return 0;
	}

	if (property == private->async_commit_prop) {
		*val = plane_state->async_commit;
		return 0;
	}

	if (property == private->share_id_prop) {
		int i;
		struct drm_mode_object *obj = &plane->base;

		for (i = 0; i < obj->properties->count; i++) {
			if (obj->properties->properties[i] == property) {
				*val = obj->properties->values[i];
				return 0;
			}
		}
	}

	DRM_ERROR("failed to get vop plane property id:%d, name:%s\n",
		   property->base.id, property->name);

	return -EINVAL;
}


static const struct drm_plane_funcs kendryte_layer_funcs = {
	.update_plane	= kendryte_atomic_helper_update_plane,
	.disable_plane	= kendryte_atomic_helper_disable_plane,
	.destroy = kendryte_plane_destroy,
	.reset = kendryte_atomic_plane_reset,
	.atomic_duplicate_state = kendryte_atomic_plane_duplicate_state,
	.atomic_destroy_state = kendryte_atomic_plane_destroy_state,
	.atomic_set_property = kendryte_atomic_plane_set_property,
	.atomic_get_property = kendryte_atomic_plane_get_property,
};





static void kendryte_plane_atomic_disable(struct drm_plane *plane,
				     struct drm_plane_state *old_state)
{
	struct kendryte_layer *win = to_vop_layer(plane);
	struct kendryte_vo *vop = to_vop(old_state->crtc);
#if defined(CONFIG_ROCKCHIP_DRM_DEBUG)
	struct vop_plane_state *vop_plane_state =
					to_vop_plane_state(plane->state);
#endif

	if (!old_state->crtc)
		return;


#if 0 
	spin_lock(&vop->reg_lock);

	vop_win_disable(vop, win);

	/*
	 * IC design bug: in the bandwidth tension environment when close win2,
	 * vop will access the freed memory lead to iommu pagefault.
	 * so we add this reset to workaround.
	 */
	if (VOP_MAJOR(vop->version) == 2 && VOP_MINOR(vop->version) == 5 &&
	    win->win_id == 2)
		VOP_WIN_SET(vop, win, yrgb_mst, 0);


	spin_unlock(&vop->reg_lock);
#endif
}


static void kendryte_plane_atomic_update(struct drm_plane *plane,
		struct drm_plane_state *old_state)
{
	struct drm_plane_state *state = plane->state;
	struct drm_crtc *crtc = state->crtc;
	struct drm_display_mode *mode = NULL;
	struct kendryte_layer *layer = to_vop_layer(plane);
	struct kendryte_layer_state *vop_plane_state = state_to_kendryte_layer_state(state);
	struct kendryte_crtc_state *s;
	struct kendryte_vo *vop = to_vop(state->crtc);
	struct drm_framebuffer *fb = state->fb;
	unsigned int actual_w, actual_h;
	unsigned int dsp_stx, dsp_sty;
	uint32_t act_info, dsp_info, dsp_st;
	struct drm_rect *src = &vop_plane_state->src;
	struct drm_rect *dest = &vop_plane_state->dest;
	uint32_t val;
	uint32_t stride;
	bool rb_swap, global_alpha_en;

    struct drm_format_info *format = fb->format;

	/*
	 * can't update plane when vop is disabled.
	 */
	if (WARN_ON(!crtc))
		return;

	mode = &crtc->state->adjusted_mode;
	actual_w = drm_rect_width(src) >> 16;
	actual_h = drm_rect_height(src) >> 16;
	act_info = (actual_h - 1) << 16 | ((actual_w - 1) & 0xffff);

	dsp_info = (drm_rect_height(dest) - 1) << 16;
	dsp_info |= (drm_rect_width(dest) - 1) & 0xffff;

	dsp_stx = dest->x1 + mode->crtc_htotal - mode->crtc_hsync_start;
	dsp_sty = dest->y1 + mode->crtc_vtotal - mode->crtc_vsync_start;
	dsp_st = dsp_sty << 16 | (dsp_stx & 0xffff);

	s = to_kendryte_crtc_state(crtc->state);
		
	if(plane->base.id == vop->layer[0].layer_id)
	{
		// config yuv1
		writel(0x11, vop->base + VO_LAYER0_PINGPANG_SEL_MODE);
		kendryte_set_layer_uv_endian(vop, LAYER0, VO_VIDEO_LAYER_UV_ENDIAN_MODE3);
		kendryte_set_layer_postion(vop, LAYER0, actual_h, actual_w, dest->y1, dest->x1);
		stride = (vop->layer[0].stride / 8 - 1) + ((actual_h) << 16);

		//stride = (actual_w / 8 - 1) + ((actual_h) << 16);
		kendryte_set_layer_stride(vop, LAYER0, stride);
		kendryte_set_layer_addr(vop, LAYER0, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->uv_mst, vop_plane_state->uv_mst);

		writel(0x1, vop->base + 0x174);
		writel(0x1, vop->base + 0x17c);

	}

	if(plane->base.id == vop->layer[1].layer_id)
	{
		// config yuv1
		writel(0x11, vop->base + VO_LAYER1_PINGPANG_SEL_MODE);
		kendryte_set_layer_uv_endian(vop, LAYER1, VO_VIDEO_LAYER_UV_ENDIAN_MODE3);

		kendryte_layer_set_uv_swap(vop, LAYER1, 0);
		kendryte_layer_set_in_offset(vop, LAYER1, 0);

		kendryte_set_layer_postion(vop, LAYER1, actual_h, actual_w, dest->y1, dest->x1);
		stride = (vop->layer[1].stride / 8 -1) + ((actual_h - 1) << 16);
		//stride = (actual_w / 8 - 1) + ((actual_h) << 16);
		kendryte_set_layer_stride(vop, LAYER1, stride);
		kendryte_set_layer_addr(vop, LAYER1, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->uv_mst, vop_plane_state->uv_mst);

	}

	if(plane->base.id == vop->layer[2].layer_id)
	{
		// config yuv2
		writel(0x11, vop->base + VO_LAYER2_PINGPANG_SEL_MODE);
		kendryte_set_layer_uv_endian(vop, LAYER2, VO_VIDEO_LAYER_UV_ENDIAN_MODE3);

		kendryte_layer_set_uv_swap(vop, LAYER2, 0);
		kendryte_layer_set_in_offset(vop, LAYER2, 0);

		kendryte_set_layer_postion(vop, LAYER2, actual_h, actual_w, dest->y1, dest->x1);
		stride = (vop->layer[2].stride / 8 -1) + ((actual_h - 1) << 16);
		//stride = (actual_w / 8 - 1) + ((actual_h) << 16);
		kendryte_set_layer_stride(vop, LAYER2, stride);
		kendryte_set_layer_addr(vop, LAYER2, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->uv_mst, vop_plane_state->uv_mst);
	}

	if(plane->base.id == vop->layer[3].layer_id)
	{
		// config yuv3
		writel(0x11, vop->base + VO_LAYER3_PINGPANG_SEL_MODE);
		kendryte_set_layer_uv_endian(vop, LAYER3, VO_VIDEO_LAYER_UV_ENDIAN_MODE3);

		kendryte_layer_set_uv_swap(vop, LAYER3, 0);
		kendryte_layer_set_in_offset(vop, LAYER3, 0);

		kendryte_set_layer_postion(vop, LAYER3, actual_h, actual_w, dest->y1, dest->x1);
		stride = (vop->layer[3].stride / 8 -1) + ((actual_h - 1) << 16);
		//stride = (actual_w / 8 - 1) + ((actual_h) << 16);
		kendryte_set_layer_stride(vop, LAYER3, stride);
		kendryte_set_layer_addr(vop, LAYER3, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->uv_mst, vop_plane_state->uv_mst);

	}

	if(plane->base.id == vop->layer[4].layer_id)
	{
		// osd0 argb
		writel(0x100, vop->base + VO_LAYER4_PINGPANG_SEL_MODE);
		kendryte_vo_osd_set_rgb2yuv_enable(vop, OSD0);
		kendryte_layer_set_uv_swap(vop, OSD0, 1);
		kendryte_vo_osd_set_format(vop, OSD0, vop_plane_state->format);
		kendryte_set_layer_postion(vop, OSD0, actual_h, actual_w, dest->y1, dest->x1);
		kendryte_set_layer_stride(vop, OSD0, (actual_w * 4) / 8);
		kendryte_set_osd_addr(vop, OSD0, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst);

	}

	if(plane->base.id == vop->layer[5].layer_id)
	{
		// osd1 argb
		writel(0x100, vop->base + VO_LAYER5_PINGPANG_SEL_MODE);
		kendryte_layer_set_uv_swap(vop, OSD1, 1);
		kendryte_vo_osd_set_rgb2yuv_enable(vop, OSD1);
		kendryte_vo_osd_set_format(vop, OSD1, vop_plane_state->format);
		kendryte_set_layer_postion(vop, OSD1, actual_h, actual_w, dest->y1, dest->x1);
		kendryte_set_layer_stride(vop, OSD1, (actual_w * 4) / 8);
		kendryte_set_osd_addr(vop, OSD1, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst);
	}

	if(plane->base.id == vop->layer[6].layer_id)
	{
		//osd2 argb
		writel(0x100, vop->base + VO_LAYER6_PINGPANG_SEL_MODE);
		kendryte_layer_set_uv_swap(vop, OSD2, 1);
		kendryte_vo_osd_set_rgb2yuv_enable(vop, OSD2);
		kendryte_vo_osd_set_format(vop, OSD2, vop_plane_state->format);
		kendryte_set_layer_postion(vop, OSD2, actual_h, actual_w, dest->y1, dest->x1);
		kendryte_set_layer_stride(vop, OSD2, (actual_w * 4) / 8);
		kendryte_set_osd_addr(vop, OSD2, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst, vop_plane_state->yrgb_mst);
	}
}


static void kendryte_layer_atomic_disable(struct drm_plane *plane,
					       struct drm_plane_state *old_state)
{
	struct kendryte_layer_state *layer_state = state_to_kendryte_layer_state(old_state);
	struct kendryte_layer *layer = to_vop_layer(plane);
	struct kendryte_vo *vo = layer->vo;
	int i = 0;

	// close all layer
	for(i = 0; i < KENDRYTE_VO_NUM_LAYERS; i++)
	{
		kendryte_set_layer_enable(vo, LAYER0 + i, 0);
	}
}


static enum vop_data_format vop_convert_format(uint32_t format)
{
	switch (format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
		return VOP_FMT_ARGB8888;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return VOP_FMT_RGB888;
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
		return VOP_FMT_RGB565;
	case DRM_FORMAT_ARGB4444:
	case DRM_FORMAT_ABGR4444:
	case DRM_FORMAT_RGBA4444:
	case DRM_FORMAT_BGRA4444:
		return VOP_FMT_ARGB4444;
	case DRM_FORMAT_ARGB1555:
	case DRM_FORMAT_ABGR1555:
	case DRM_FORMAT_RGBA5551:
		return VOP_FMT_ARGB1555;
	case DRM_FORMAT_NV12:
		return VOP_FMT_YUV420SP;
	case DRM_FORMAT_NV16:
		return VOP_FMT_YUV422SP;
	case DRM_FORMAT_NV24:
		return VOP_FMT_YUV444SP;
	case DRM_FORMAT_YUYV:
		return VOP_FMT_YUYV;
	
	default:
		DRM_ERROR("unsupported format[%08x]\n", format);
		return -EINVAL;
	}
}


static int kendryte_plane_atomic_check(struct drm_plane *plane,
                           struct drm_plane_state *state)
{       

	struct drm_crtc *crtc = state->crtc;
	struct drm_crtc_state *crtc_state;
	struct drm_framebuffer *fb = state->fb;
	struct kendryte_vo *vop;
	struct kendryte_layer *layer = to_vop_layer(plane);
	struct kendryte_layer_state *vop_plane_state = state_to_kendryte_layer_state(state);
	struct drm_rect *dest = &vop_plane_state->dest;
	struct drm_rect *src = &vop_plane_state->src;
	int ret;
	unsigned long offset;
	u64 paddr ;
	struct drm_gem_cma_object *obj ;//= drm_fb_cma_get_gem_obj(fb, 0);
	struct drm_mode_object *mode_obj = &plane->base;
	if(fb != NULL)
	{
		obj = drm_fb_cma_get_gem_obj(fb, 0);
	}
    paddr = obj->paddr;

	crtc = crtc ? crtc : plane->state->crtc;

	if (!crtc || !fb) {
		layer->status = 0;
		plane->state->visible = false;
		return 0;
	}
	
	layer->status = 1;	

	crtc_state = drm_atomic_get_existing_crtc_state(state->state, crtc);
	if (WARN_ON(!crtc_state))
		return -EINVAL;
	
	src->x1 = state->src_x;
	src->y1 = state->src_y;
	src->x2 = state->src_x + state->src_w;
	src->y2 = state->src_y + state->src_h;
	dest->x1 = state->crtc_x;
	dest->y1 = state->crtc_y;
	dest->x2 = state->crtc_x + state->crtc_w;
	dest->y2 = state->crtc_y + state->crtc_h;

#if 0
	// scale operate 
	ret = drm_atomic_helper_check_plane_state(state, crtc_state,
						  min_scale, max_scale,
						  false, false);
	if (ret)
		return ret;
#endif

	vop_plane_state->format = vop_convert_format(fb->format->format);
	if (vop_plane_state->format < 0)
		return vop_plane_state->format;


	//printk("vop_plane_state->format is %d \n", vop_plane_state->format);

	vop = to_vop(crtc);
	
#if 0
	if (drm_rect_width(src) >> 16 > vop_data->max_input.width ||
	    drm_rect_height(src) >> 16 > vop_data->max_input.height) {
		DRM_ERROR("Invalid source: %dx%d. max input: %dx%d\n",
			  drm_rect_width(src) >> 16,
			  drm_rect_height(src) >> 16,
			  vop_data->max_input.width,
			  vop_data->max_input.height);
		return -EINVAL;
	}
#endif

	// argb  srgb only y
	vop_plane_state->yrgb_mst = paddr;

	if (vop_plane_state->format == VOP_FMT_YUV420SP) 
	{

		int hsub = state->crtc_w;
		int vsub = state->crtc_h;

		layer->stride = fb->pitches[0];

		// 420 
		vop_plane_state->yrgb_mst = paddr;
		vop_plane_state->uv_mst = (layer->stride * vsub) + paddr;   // stride ???

	}	
	return 0;
}


static int kendryte_plane_prepare_fb(struct drm_plane *plane,
				struct drm_plane_state *new_state)
{

	if (plane->state->fb)
		drm_framebuffer_get(plane->state->fb);

	return 0;
}


static void kendryte_plane_cleanup_fb(struct drm_plane *plane, struct drm_plane_state *old_state)
{
	if (old_state->fb)
	{
		drm_framebuffer_put(old_state->fb);
	}
											
}

static const struct drm_plane_helper_funcs kendryte_layer_helper_funcs = {
	.prepare_fb	= kendryte_plane_prepare_fb,
	.atomic_check   = kendryte_plane_atomic_check,
    	.atomic_update	= kendryte_plane_atomic_update,
	.atomic_disable	= kendryte_layer_atomic_disable,
	.cleanup_fb     = kendryte_plane_cleanup_fb,
};



static bool kendryte_crtc_mode_fixup(struct drm_crtc *crtc,
				const struct drm_display_mode *mode,
				struct drm_display_mode *adj_mode)
{
	struct kendryte_vo *vop = to_vop(crtc);

    //printk("vop_crtc_mode_fixup start \n");
#if 0 
	const struct vop_data *vop_data = vop->data;

	//printk("vop_crtc_mode_fixup start \n");
	if (mode->hdisplay > vop_data->max_output.width)
		return false;

	drm_mode_set_crtcinfo(adj_mode,
			      CRTC_INTERLACE_HALVE_V | CRTC_STEREO_DOUBLE);

	if (mode->flags & DRM_MODE_FLAG_DBLCLK)
		adj_mode->crtc_clock *= 2;

	adj_mode->crtc_clock =
		DIV_ROUND_UP(clk_round_rate(vop->dclk, adj_mode->crtc_clock * 1000),
			     1000);
#endif

	return true;
}

static int kendryte_crtc_atomic_check(struct drm_crtc *crtc,
				 struct drm_crtc_state *crtc_state)
{
	struct drm_atomic_state *state = crtc_state->state;
	struct kendryte_crtc_state *s = to_kendryte_crtc_state(crtc_state);
	struct kendryte_vo *vop = to_vop(crtc);
//	const struct vop_data *vop_data = vop->data;
	struct drm_plane *plane;
	struct drm_plane_state *pstate;
	struct kendryte_layer_state *plane_state;
	struct vop_zpos *pzpos;
	int dsp_layer_sel = 0;
	int i, j, cnt = 0, ret = 0;



	return ret;
}


static void kendryte_crtc_atomic_enable(struct drm_crtc *crtc,
				   struct drm_crtc_state *old_state)
{
	struct kendryte_vo *vop = to_vop(crtc);
	struct kendryte_crtc_state *s = to_kendryte_crtc_state(crtc->state);
	struct drm_display_mode *adjusted_mode = &crtc->state->adjusted_mode;
	u16 hsync_len = adjusted_mode->crtc_hsync_end - adjusted_mode->crtc_hsync_start;
	u16 hdisplay = adjusted_mode->crtc_hdisplay;
	u16 htotal = adjusted_mode->crtc_htotal;
	u16 hact_st = adjusted_mode->crtc_htotal - adjusted_mode->crtc_hsync_start;
	u16 hact_end = hact_st + hdisplay;
	u16 vdisplay = adjusted_mode->crtc_vdisplay;
	u16 vtotal = adjusted_mode->crtc_vtotal;
	u16 vsync_len = adjusted_mode->crtc_vsync_end - adjusted_mode->crtc_vsync_start;
	u16 vact_st = adjusted_mode->crtc_vtotal - adjusted_mode->crtc_vsync_start;
	u16 vact_end = vact_st + vdisplay;
//	int sys_status = drm_crtc_index(crtc) ? SYS_STATUS_LCDC1 : SYS_STATUS_LCDC0;
				
	uint32_t val;
	int act_end;
	bool interlaced = !!(adjusted_mode->flags & DRM_MODE_FLAG_INTERLACE);
	int for_ddr_freq = 0;
	bool dclk_inv;

	printk("***###***********************************vop_crtc_atomic_enable start ---------------- \n");

#if 0 
	rockchip_set_system_status(sys_status);

    
	vop_lock(vop);
	DRM_DEV_INFO(vop->dev, "Update mode to %dx%d%s%d, type: %d\n",
		     hdisplay, vdisplay, interlaced ? "i" : "p",
		     adjusted_mode->vrefresh, s->output_type);
	vop_initial(crtc);
	vop_disable_allwin(vop);
	VOP_CTRL_SET(vop, standby, 0);
	s->mode_update = vop_crtc_mode_update(crtc);
	if (s->mode_update)
		vop_disable_all_planes(vop);
	/*
	 * restore the lut table.
	 */
	if (vop->lut_active)
		vop_crtc_load_lut(crtc);

	if (vop->mcu_timing.mcu_pix_total)
		vop_mcu_mode(crtc);

	dclk_inv = (s->bus_flags & DRM_BUS_FLAG_PIXDATA_DRIVE_NEGEDGE) ? 1 : 0;

	VOP_CTRL_SET(vop, dclk_pol, dclk_inv);
	val = (adjusted_mode->flags & DRM_MODE_FLAG_NHSYNC) ?
		   0 : BIT(HSYNC_POSITIVE);
	val |= (adjusted_mode->flags & DRM_MODE_FLAG_NVSYNC) ?
		   0 : BIT(VSYNC_POSITIVE);
	VOP_CTRL_SET(vop, pin_pol, val);

	if (vop->dclk_source && vop->pll && vop->pll->pll) {
		if (clk_set_parent(vop->dclk_source, vop->pll->pll))
			DRM_DEV_ERROR(vop->dev,
				      "failed to set dclk's parents\n");
	}

	switch (s->output_type) {
	case DRM_MODE_CONNECTOR_DPI:
	case DRM_MODE_CONNECTOR_LVDS:
		//output LVDS

		break;
	case DRM_MODE_CONNECTOR_eDP:
		//opt put ePD


		break;
	case DRM_MODE_CONNECTOR_HDMIA:
        //output hdmi


		break;
	case DRM_MODE_CONNECTOR_DSI:
		VOP_CTRL_SET(vop, mipi_en, 1);
		VOP_CTRL_SET(vop, mipi_pin_pol, val);
		VOP_CTRL_SET(vop, mipi_dclk_pol, dclk_inv);
		VOP_CTRL_SET(vop, mipi_dual_channel_en,
			!!(s->output_flags & ROCKCHIP_OUTPUT_DSI_DUAL_CHANNEL));
		VOP_CTRL_SET(vop, data01_swap,
			!!(s->output_flags & ROCKCHIP_OUTPUT_DSI_DUAL_LINK) ||
			vop->dual_channel_swap);
		break;
	case DRM_MODE_CONNECTOR_DisplayPort:
		VOP_CTRL_SET(vop, dp_dclk_pol, 0);
		VOP_CTRL_SET(vop, dp_pin_pol, val);
		VOP_CTRL_SET(vop, dp_en, 1);
		break;
	case DRM_MODE_CONNECTOR_TV:


		break;
	default:
		DRM_ERROR("unsupported connector_type[%d]\n", s->output_type);
	}
/*
	vop_update_csc(crtc);
	VOP_CTRL_SET(vop, htotal_pw, (htotal << 16) | hsync_len);
	val = hact_st << 16;
	val |= hact_end;
	VOP_CTRL_SET(vop, hact_st_end, val);
	VOP_CTRL_SET(vop, hpost_st_end, val);

	val = vact_st << 16;
	val |= vact_end;
	VOP_CTRL_SET(vop, vact_st_end, val);
	VOP_CTRL_SET(vop, vpost_st_end, val);

	if (adjusted_mode->flags & DRM_MODE_FLAG_INTERLACE) {
		u16 vact_st_f1 = vtotal + vact_st + 1;
		u16 vact_end_f1 = vact_st_f1 + vdisplay;

		val = vact_st_f1 << 16 | vact_end_f1;
		VOP_CTRL_SET(vop, vact_st_end_f1, val);
		VOP_CTRL_SET(vop, vpost_st_end_f1, val);

		val = vtotal << 16 | (vtotal + vsync_len);
		VOP_CTRL_SET(vop, vs_st_end_f1, val);
		VOP_CTRL_SET(vop, dsp_interlace, 1);
		VOP_CTRL_SET(vop, p2i_en, 1);
		vtotal += vtotal + 1;
		act_end = vact_end_f1;
	} else {
		VOP_CTRL_SET(vop, dsp_interlace, 0);
		VOP_CTRL_SET(vop, p2i_en, 0);
		act_end = vact_end;
	}

	if (VOP_MAJOR(vop->version) == 3 &&
	    (VOP_MINOR(vop->version) == 2 || VOP_MINOR(vop->version) == 8))
		for_ddr_freq = 1000;
	VOP_INTR_SET(vop, line_flag_num[0], act_end);
	VOP_INTR_SET(vop, line_flag_num[1],
		     act_end - us_to_vertical_line(adjusted_mode, for_ddr_freq));

	VOP_CTRL_SET(vop, vtotal_pw, vtotal << 16 | vsync_len);

	VOP_CTRL_SET(vop, core_dclk_div,
		     !!(adjusted_mode->flags & DRM_MODE_FLAG_DBLCLK));

	VOP_CTRL_SET(vop, win_csc_mode_sel, 1);

	clk_set_rate(vop->dclk, adjusted_mode->crtc_clock * 1000);

	vop_cfg_done(vop);
*/
	drm_crtc_vblank_on(crtc);

//	vop_unlock(vop);
#endif
}


static void kendryte_crtc_atomic_disable(struct drm_crtc *crtc,
				    struct drm_crtc_state *old_state)
{
	struct kendryte_vo *vop = to_vop(crtc);
//	int sys_status = drm_crtc_index(crtc) ? SYS_STATUS_LCDC1 : SYS_STATUS_LCDC0;
				

#if 0 
	vop_lock(vop);
	VOP_CTRL_SET(vop, reg_done_frm, 1);
	VOP_CTRL_SET(vop, dsp_interlace, 0);
	drm_crtc_vblank_off(crtc);
	VOP_CTRL_SET(vop, afbdc_en, 0);
	vop_disable_all_planes(vop);

	/*
	 * Vop standby will take effect at end of current frame,
	 * if dsp hold valid irq happen, it means standby complete.
	 *
	 * we must wait standby complete when we want to disable aclk,
	 * if not, memory bus maybe dead.
	 */
	reinit_completion(&vop->dsp_hold_completion);
	vop_dsp_hold_valid_irq_enable(vop);

	spin_lock(&vop->reg_lock);

	VOP_CTRL_SET(vop, standby, 1);

	spin_unlock(&vop->reg_lock);

	WARN_ON(!wait_for_completion_timeout(&vop->dsp_hold_completion,
					     msecs_to_jiffies(50)));

	vop_dsp_hold_valid_irq_disable(vop);

	vop->is_enabled = false;
	if (vop->is_iommu_enabled) {
		/*
		 * vop standby complete, so iommu detach is safe.
		 */
		VOP_CTRL_SET(vop, dma_stop, 1);
		rockchip_drm_dma_detach_device(vop->drm_dev, vop->dev);
		vop->is_iommu_enabled = false;
	}

	pm_runtime_put_sync(vop->dev);
	clk_disable_unprepare(vop->dclk);
	clk_disable_unprepare(vop->aclk);
	clk_disable_unprepare(vop->hclk);
	vop_unlock(vop);

	rockchip_clear_system_status(sys_status);
#endif
	if (crtc->state->event && !crtc->state->active) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock_irq(&crtc->dev->event_lock);

		crtc->state->event = NULL;
	}

}

void kendryte_vop_cfg_updata(struct kendryte_vo *vop)
{
	int i = 0;
	for(i =0 ; i < KENDRYTE_VO_NUM_LAYERS; i++)
	{
		kendryte_set_layer_enable(vop, i + LAYER0 , vop->layer[i].status);
	}
}


static void kendryte_crtc_atomic_flush(struct drm_crtc *crtc,
				  struct drm_crtc_state *old_crtc_state)
{
	struct drm_atomic_state *old_state = old_crtc_state->state;
	struct drm_plane_state *old_plane_state;
	struct kendryte_vo *vop = to_vop(crtc);
	struct drm_plane *plane;
	int i;
	unsigned long flags;
	struct kendryte_crtc_state *s = to_kendryte_crtc_state(crtc->state);
		
	kendryte_vop_cfg_updata(vop);

//	kendryte_vo_get_reg_val(vop);

	kendryte_vo_set_config_done(vop);

	spin_lock_irq(&crtc->dev->event_lock);
	if (crtc->state->event) {
		// vblank enable
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		WARN_ON(vop->event);

		vop->event = crtc->state->event;
		crtc->state->event = NULL;
	}
	spin_unlock_irq(&crtc->dev->event_lock);


#if 0
	for_each_plane_in_state(old_state, plane, old_plane_state, i) {
		if (!old_plane_state->fb)
			continue;

		if (old_plane_state->fb == plane->state->fb)
			continue;

		drm_framebuffer_get(old_plane_state->fb);
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		drm_flip_work_queue(&vop->fb_unref_work, old_plane_state->fb);
		set_bit(VOP_PENDING_FB_UNREF, &vop->pending);
	}
#else
	for_each_old_plane_in_state(old_state, plane, old_plane_state, i) {
		if (!old_plane_state->fb)
			continue;

		if (old_plane_state->fb == plane->state->fb)
			continue;

		drm_framebuffer_get(old_plane_state->fb);
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);
		drm_flip_work_queue(&vop->fb_unref_work, old_plane_state->fb);
		set_bit(VOP_PENDING_FB_UNREF, &vop->pending);
	}
#endif

}


static const struct drm_crtc_helper_funcs kendryte_crtc_helper_funcs = {
    .mode_fixup = kendryte_crtc_mode_fixup,
	.atomic_check = kendryte_crtc_atomic_check,
	.atomic_flush = kendryte_crtc_atomic_flush,
	.atomic_enable = kendryte_crtc_atomic_enable,
	.atomic_disable = kendryte_crtc_atomic_disable,
};



static int kendryte_crtc_gamma_set(struct drm_crtc *crtc, u16 *red, u16 *green,
			       u16 *blue, uint32_t size,
			       struct drm_modeset_acquire_ctx *ctx)
{

    printk("kendryte_crtc_gamma_set \n");
#if 0 
	struct vop *vop = to_vop(crtc);
	int len = min(size, vop->lut_len);
	int i;

	if (!vop->lut)
		return -EINVAL;

	for (i = 0; i < len; i++)
		rockchip_vop_crtc_fb_gamma_set(crtc, red[i], green[i],
					       blue[i], i);

	vop_crtc_load_lut(crtc);

#endif
	return 0;
}

static void kendryte_crtc_destroy(struct drm_crtc *crtc)
{
	drm_crtc_cleanup(crtc);
}


static void kendryte_crtc_reset(struct drm_crtc *crtc)
{
	struct kendryte_crtc_state *s = to_kendryte_crtc_state(crtc->state);

	if (crtc->state) {
		__drm_atomic_helper_crtc_destroy_state(crtc->state);
		kfree(s);
	}

	s = kzalloc(sizeof(*s), GFP_KERNEL);
	if (!s)
		return;
	crtc->state = &s->base;
	crtc->state->crtc = crtc;

	s->left_margin = 100;
	s->right_margin = 100;
	s->top_margin = 100;
	s->bottom_margin = 100;
}

static int kendryte_crtc_atomic_get_property(struct drm_crtc *crtc,
					const struct drm_crtc_state *state,
					struct drm_property *property,
					uint64_t *val)
{
	struct drm_device *drm_dev = crtc->dev;
	struct kendryte_drm_private *private = drm_dev->dev_private;
	struct drm_mode_config *mode_config = &drm_dev->mode_config;
	struct kendryte_crtc_state *s = to_kendryte_crtc_state(state);
	struct kendryte_vo *vop = to_vop(crtc);

	if (property == mode_config->tv_left_margin_property) {
		*val = s->left_margin;
		return 0;
	}

	if (property == mode_config->tv_right_margin_property) {
		*val = s->right_margin;
		return 0;
	}

	if (property == mode_config->tv_top_margin_property) {
		*val = s->top_margin;
		return 0;
	}

	if (property == mode_config->tv_bottom_margin_property) {
		*val = s->bottom_margin;
		return 0;
	}
/*
	if (property == private->alpha_scale_prop) {
		*val = (vop->data->feature & VOP_FEATURE_ALPHA_SCALE) ? 1 : 0;
		return 0;
	}
*/
	DRM_ERROR("failed to get vop crtc property\n");
	return -EINVAL;
}


static int kendryte_crtc_atomic_set_property(struct drm_crtc *crtc,
					struct drm_crtc_state *state,
					struct drm_property *property,
					uint64_t val)
{
	struct drm_device *drm_dev = crtc->dev;
	struct drm_mode_config *mode_config = &drm_dev->mode_config;
	struct kendryte_crtc_state *s = to_kendryte_crtc_state(state);
	//struct vop *vop = to_vop(crtc);

	if (property == mode_config->tv_left_margin_property) {
		s->left_margin = val;
		return 0;
	}

	if (property == mode_config->tv_right_margin_property) {
		s->right_margin = val;
		return 0;
	}

	if (property == mode_config->tv_top_margin_property) {
		s->top_margin = val;
		return 0;
	}

	if (property == mode_config->tv_bottom_margin_property) {
		s->bottom_margin = val;
		return 0;
	}

	DRM_ERROR("failed to set vop crtc property\n");
	return -EINVAL;
}

static struct drm_crtc_state *kendryte_crtc_duplicate_state(struct drm_crtc *crtc)
{
	struct kendryte_crtc_state *kendryte_state, *old_state;

//	printk("kendryte_crtc_duplicate_state ------------------ \n");

	old_state = to_kendryte_crtc_state(crtc->state);
	kendryte_state = kmemdup(old_state, sizeof(*old_state), GFP_KERNEL);
	if (!kendryte_state)
		return NULL;

	__drm_atomic_helper_crtc_duplicate_state(crtc, &kendryte_state->base);

	return &kendryte_state->base;
}


static void kendryte_crtc_destroy_state(struct drm_crtc *crtc,
				   struct drm_crtc_state *state)
{
	struct kendryte_crtc_state *s = to_kendryte_crtc_state(state);

	__drm_atomic_helper_crtc_destroy_state(&s->base);
	kfree(s);
}


static int kendryte_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct kendryte_vo * vop= to_vop(crtc);
	unsigned long flags;

	spin_lock_irqsave(&vop->irq_lock, flags);

	// open vo intr
	kendryte_vo_set_irq(vop, 1);

	spin_unlock_irqrestore(&vop->irq_lock, flags);

	return 0;
}

static void kendryte_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct kendryte_vo *vop = to_vop(crtc);
	unsigned long flags;

	spin_lock_irqsave(&vop->irq_lock, flags);

	//close intr 
	kendryte_vo_set_irq(vop, 0);

	spin_unlock_irqrestore(&vop->irq_lock, flags);

}





static const struct drm_crtc_funcs kendryte_crtc_funcs = {
    .gamma_set = kendryte_crtc_gamma_set,
	.set_config = drm_atomic_helper_set_config,
	.page_flip = drm_atomic_helper_page_flip,
	.destroy = kendryte_crtc_destroy,
	.reset = kendryte_crtc_reset,
	.atomic_get_property = kendryte_crtc_atomic_get_property,
	.atomic_set_property = kendryte_crtc_atomic_set_property,
	.atomic_duplicate_state = kendryte_crtc_duplicate_state,
	.atomic_destroy_state = kendryte_crtc_destroy_state,
	.enable_vblank = kendryte_crtc_enable_vblank,
	.disable_vblank = kendryte_crtc_disable_vblank,
};


static const uint32_t kendryte_layer_formats[] = {
	DRM_FORMAT_NV12,
	DRM_FORMAT_NV16,
	DRM_FORMAT_NV21,
	DRM_FORMAT_NV61,
	DRM_FORMAT_UYVY,
	DRM_FORMAT_VYUY,
	DRM_FORMAT_YUV420,
	DRM_FORMAT_YUV422,
	DRM_FORMAT_YUYV,
	DRM_FORMAT_YVU420,
	DRM_FORMAT_YVU422,
	DRM_FORMAT_YVYU,
};

static const uint32_t kendryte_osd_formats[] = {
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_XRGB8888,
};


int kendryte_create_crtc(struct kendryte_vo *vop)
{

	struct device *dev = vop->dev;
	struct drm_device *drm_dev = vop->drm;
	struct drm_plane *primary = NULL, *cursor = NULL, *plane, *tmp;
	struct drm_crtc *crtc = &vop->crtc;
	struct device_node *port;

	const uint32_t *formats = kendryte_layer_formats;
	unsigned int formats_len = ARRAY_SIZE(kendryte_layer_formats);

	const uint32_t *osd_formats = kendryte_osd_formats;
	unsigned int ods_formats_len = ARRAY_SIZE(kendryte_osd_formats);

	int ret;
	int i;

	/*
	 * Create drm_planes for overlay windows with possible_crtcs restricted
	 * to the newly created crtc.
	 */

	for (i = 0; i < 4; i++) {
		struct kendryte_layer *layer = &vop->layer[i];

		unsigned long possible_crtcs = drm_crtc_mask(crtc);
		if (layer->type != DRM_PLANE_TYPE_OVERLAY)
			continue;

		ret = drm_universal_plane_init(drm_dev, &layer->plane,
					       possible_crtcs,
					       &kendryte_layer_funcs,
					       formats,
					       formats_len,
					       NULL, layer->type, NULL);
		if (ret) {
			DRM_DEV_ERROR(vop->dev, "failed to init overlay %d\n",
				      ret);
			goto err_cleanup_crtc;
		}
		drm_plane_helper_add(&layer->plane, &kendryte_layer_helper_funcs);
		
		plane = &layer->plane;
		layer->layer_id = plane->base.id;
		layer->status = 0;
			
	}
	
	for (i = 0; i < 3; i++) {
		struct kendryte_layer *layer = &vop->layer[i + 4];

		unsigned long possible_crtcs = drm_crtc_mask(crtc);
		if (layer->type != DRM_PLANE_TYPE_OVERLAY)
			continue;

		ret = drm_universal_plane_init(drm_dev, &layer->plane,
					       possible_crtcs,
					       &kendryte_layer_funcs,
					       osd_formats,
					       ods_formats_len,
					       NULL, layer->type, NULL);
		if (ret) {
			DRM_DEV_ERROR(vop->dev, "failed to init overlay %d\n",
				      ret);
			goto err_cleanup_crtc;
		}
		drm_plane_helper_add(&layer->plane, &kendryte_layer_helper_funcs);
		
		plane = &layer->plane;
		layer->layer_id = plane->base.id;
		layer->status = 0;

		// osd2 for DRM_PLANE_TYPE_PRIMARY = 1
		if(i == 2)
		{
			plane->type = DRM_PLANE_TYPE_PRIMARY;
			layer->type = DRM_PLANE_TYPE_PRIMARY;
			primary = plane;
		}
			
		// osd1 for DRM_PLANE_TYPE_CURSOR = 2
		if(i == 1)
		{
			plane->type = DRM_PLANE_TYPE_CURSOR;
			layer->type = DRM_PLANE_TYPE_CURSOR;
			cursor = plane;
		}	
	}

	ret = drm_crtc_init_with_planes(drm_dev, crtc, primary, cursor,
					&kendryte_crtc_funcs, NULL);
	if (ret)
		goto err_cleanup_planes;

	drm_crtc_helper_add(crtc, &kendryte_crtc_helper_funcs);

	port = of_get_child_by_name(dev->of_node, "port");
	if (!port) {
		DRM_DEV_ERROR(vop->dev, "no port node found in %pOF\n",
			      dev->of_node);
		ret = -ENOENT;
		goto err_cleanup_crtc;
	}

	crtc->port = port;

	return 0;
err_cleanup_crtc:
	drm_crtc_cleanup(crtc);
err_cleanup_planes:

	drm_plane_cleanup(plane);
	return ret;
}

