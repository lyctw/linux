/*
 * Copyright (c) 2022, Canaan Bright Sight Co., Ltd
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
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>

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



#include "kendryte_vo.h"
#include "kendryte_crtc.h"
#include "kendryte_fb.h"
#include "kendryte_drm_fbdev.h"

#define PREFERRED_BPP		32



static int kendryte_gem_cma_dumb_create(struct drm_file *file,
				     struct drm_device *dev,
				     struct drm_mode_create_dumb *args)
{
    printk("kendryte_gem_cma_dumb_create called\n");
	return drm_gem_cma_dumb_create_internal(file, dev, args);
}

static int kendryte_fb_dev_mmap(struct drm_gem_object *obj,
			   struct vm_area_struct *vma)
{


	struct drm_gem_cma_object *cma_obj;
	int ret;

	ret = drm_gem_mmap_obj(obj, obj->size, vma);
	if (ret < 0)
		return ret;

	cma_obj = to_drm_gem_cma_obj(obj);

	/*
	 * Clear the VM_PFNMAP flag that was set by drm_gem_mmap(), and set the
	 * vm_pgoff (used as a fake buffer offset by DRM) to 0 as we want to map
	 * the whole buffer.
	 */
	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;
    ret = -ENXIO;

    unsigned long size = vma->vm_end - vma->vm_start;
    unsigned long user_count = vma_pages(vma);
    unsigned long count = PAGE_ALIGN(size) >> PAGE_SHIFT;
    unsigned long off = vma->vm_pgoff;
    void * cpu_addr = cma_obj->vaddr;

    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    if (dma_mmap_from_dev_coherent(cma_obj->base.dev->dev, vma, cpu_addr, size, &ret))
        return ret;

    printk("remap pfn range, pa_msb 0x%lx, off %d, pfn %x, page %x, virt_to_pfn %x,vmalloc_to_pfn %x\n",pa_msb, 
        off, page_to_pfn(virt_to_page(cpu_addr)), virt_to_page(cpu_addr), virt_to_pfn(cpu_addr), vmalloc_to_pfn(cpu_addr));


    if (off < count && user_count <= (count - off))
        ret = remap_pfn_range(vma, vma->vm_start,
                      (vmalloc_to_pfn(cpu_addr) + off)/* | pa_msb*/,
                      user_count << PAGE_SHIFT,
                      vma->vm_page_prot);

	if (ret)
		drm_gem_vm_close(vma);

    
	return ret;
}

static int kendryte_fbdev_mmap(struct fb_info *info,
			       struct vm_area_struct *vma)
{
    struct drm_fb_helper *helper = info->par;
	struct kendryte_drm_private *private = helper->dev->dev_private;

	return kendryte_fb_dev_mmap(private->fbdev_bo, vma);
}


static int kendryte_fbdev_blank(int blank, struct fb_info *info)
{
	struct drm_fb_helper *helper = info->par;

	drm_fb_helper_restore_fbdev_mode_unlocked(helper);

	return drm_fb_helper_blank(blank, info);
}

#if 0 
static struct dma_buf *kendryte_fbdev_get_dma_buf(struct fb_info *info)
{
	struct dma_buf *buf = NULL;
	struct drm_fb_helper *helper = info->par;
	struct rockchip_drm_private *private = helper->dev->dev_private;
	struct drm_device *dev = helper->dev;

	if (dev->driver->gem_prime_export) {
		buf = dev->driver->gem_prime_export(dev, private->fbdev_bo,
						    O_RDWR);
		if (buf)
			drm_gem_object_get(private->fbdev_bo);
	}

	return buf;
}
#endif

static struct fb_ops kendryte_drm_fbdev_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= drm_fb_helper_check_var,
	.fb_set_par	= drm_fb_helper_set_par,
	.fb_setcmap	= drm_fb_helper_setcmap,
	.fb_pan_display	= drm_fb_helper_pan_display,
	.fb_debug_enter = drm_fb_helper_debug_enter,
	.fb_debug_leave = drm_fb_helper_debug_leave,
	.fb_ioctl	= drm_fb_helper_ioctl,
	.fb_fillrect	= drm_fb_helper_cfb_fillrect,
	.fb_copyarea	= drm_fb_helper_cfb_copyarea,
	.fb_imageblit	= drm_fb_helper_cfb_imageblit,
	.fb_read	= drm_fb_helper_sys_read,
	.fb_write	= drm_fb_helper_sys_write,
	.fb_mmap		= kendryte_fbdev_mmap,
	.fb_blank		= kendryte_fbdev_blank,
//	.fb_dmabuf_export	= rockchip_fbdev_get_dma_buf,
};




static int kendryte_drm_fbdev_create(struct drm_fb_helper *helper,
				     struct drm_fb_helper_surface_size *sizes)
{
	struct kendryte_drm_private *private = helper->dev->dev_private;
	struct drm_mode_fb_cmd2 mode_cmd = { 0 };
	struct drm_device *dev = helper->dev;
    struct kendryte_gem_object *gem_obj;
    struct drm_gem_cma_object *obj;
    struct drm_fbdev_cma *fbdev;
	struct drm_framebuffer *fb;
	unsigned int bytes_per_pixel;
	unsigned long offset;
	struct fb_info *fbi;
	size_t size;
	int ret;
    

	bytes_per_pixel = DIV_ROUND_UP(sizes->surface_bpp, 8);

	mode_cmd.width = sizes->surface_width;
	mode_cmd.height = sizes->surface_height;
	mode_cmd.pitches[0] = sizes->surface_width * bytes_per_pixel;
	mode_cmd.pixel_format = drm_mode_legacy_fb_format(sizes->surface_bpp,
		sizes->surface_depth);

	size = mode_cmd.pitches[0] * mode_cmd.height;

#if 0 
    obj = drm_gem_cma_create(dev, size);
    if(IS_ERR(gem_obj->obj))
    {
        printk("drm fbdev create cma err \n");
        return -ENOMEM;
    }
#endif

    fbdev = drm_fbdev_cma_init(dev, 32,dev->mode_config.num_connector);
    if (IS_ERR(fbdev))
    {
        printk("drm_fbdev_cma_init init error \n");
    }

    obj = drm_fb_cma_get_gem_obj(helper->fb, 0);

	fbi = drm_fb_helper_alloc_fbi(helper);
	if (IS_ERR(fbi)) {
		DRM_DEV_ERROR(dev->dev, "Failed to create framebuffer info.\n");
		ret = PTR_ERR(fbi);
		goto err_drm_gem_cma_free_object;
	}
    gem_obj->obj = obj;
//    gem_obj->fbdev_cma = fbdev_cma;
    gem_obj->base = obj->base;
    private->fbdev_bo = &gem_obj->base;
    

    

	fbi->par = helper;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->fbops = &kendryte_drm_fbdev_ops;



    fb = helper->fb ;

	drm_fb_helper_fill_fix(fbi, fb->pitches[0], fb->format->depth);
	drm_fb_helper_fill_var(fbi, helper, sizes->fb_width, sizes->fb_height);

	offset = fbi->var.xoffset * bytes_per_pixel;
	offset += fbi->var.yoffset * fb->pitches[0];

	dev->mode_config.fb_base = 0;
	fbi->screen_base = obj->vaddr + offset;
	fbi->screen_size = size;
	fbi->fix.smem_len = size;

	DRM_DEBUG_KMS("FB [%dx%d]-%d kvaddr=%p offset=%ld size=%zu\n",
		      fb->width, fb->height, fb->format->depth,
		      obj->vaddr,
		      offset, size);

	fbi->skip_vt_switch = true;

	return 0;

err_drm_gem_cma_free_object:
    drm_gem_cma_free_object(&gem_obj->base);

err_framebuffer_release:
    framebuffer_release(fbi);

    return ret;
}


static const struct drm_fb_helper_funcs kendryte_drm_fb_helper_funcs = {
	.fb_probe = kendryte_drm_fbdev_create,
};

int kendryte_drm_fbdev_init(struct drm_device *dev)
{
	struct kendryte_drm_private *private = dev->dev_private;
	struct drm_fb_helper *helper;
	int ret;

    printk("kendryte_drm_fbdev_init start  \n");
	if (!dev->mode_config.num_crtc || !dev->mode_config.num_connector)
		return -EINVAL;

	helper = devm_kzalloc(dev->dev, sizeof(*helper), GFP_KERNEL);
	if (!helper)
		return -ENOMEM;
	private->fbdev_helper = helper;

	drm_fb_helper_prepare(dev, helper, &kendryte_drm_fb_helper_funcs);

    printk("kendryte_drm_fbdev_init start  ---------------------2\n");

	ret = drm_fb_helper_init(dev, helper, 7);
	if (ret < 0) {
		DRM_DEV_ERROR(dev->dev,
			      "Failed to initialize drm fb helper - %d.\n",
			      ret);
		return ret;
	}
    printk("kendryte_drm_fbdev_init start  ----------------------------3\n");
	ret = drm_fb_helper_single_add_all_connectors(helper);
	if (ret < 0) {
		DRM_DEV_ERROR(dev->dev,
			      "Failed to add connectors - %d.\n", ret);
		goto err_drm_fb_helper_fini;
	}
     printk("kendryte_drm_fbdev_init start  ----------------------------4\n");
	ret = drm_fb_helper_initial_config(helper, PREFERRED_BPP);
	if (ret < 0) {
		DRM_DEV_ERROR(dev->dev,
			      "Failed to set initial hw config - %d.\n",
			      ret);
		goto err_drm_fb_helper_fini;
	}
     printk("kendryte_drm_fbdev_init start  ----------------------------5\n");
	return 0;

err_drm_fb_helper_fini:
	drm_fb_helper_fini(helper);
	return ret;
}

void kendryte_drm_fbdev_fini(struct drm_device *dev)
{
	struct kendryte_drm_private *private = dev->dev_private;
	struct drm_fb_helper *helper = private->fbdev_helper;

	if (!helper)
		return;

	drm_fb_helper_unregister_fbi(helper);

	if (helper->fb)
		drm_framebuffer_put(helper->fb);

	drm_fb_helper_fini(helper);
}


