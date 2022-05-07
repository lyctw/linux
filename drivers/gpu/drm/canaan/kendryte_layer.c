/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/component.h>
#include <linux/crc-ccitt.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/log2.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/init.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>
#include <drm/drmP.h>
#include <video/mipi_display.h>

#include <linux/iopoll.h>
#include <linux/math64.h>
#include <linux/of_device.h>
#include <linux/mfd/syscon.h>
#include <linux/phy/phy.h>
#include <asm/unaligned.h>
#include <uapi/linux/videodev2.h>


#include "kendryte_vo.h"

#include "kendryte_crtc.h"

struct kendryte_vo_timing vo_timing = {

    .hsa_start = 0x5,
    .hsa_end = 0x19,

    .zone_x_start = 46,
    .zone_x_stop = 1080 + 46,

    .zone_y_start = 14,
    .zone_y_stop = 1920 + 14,

    .vas_start = 0x1,
    .vas_end = 0x5,

    .total_size.HT = 1958,
    .total_size.VT = 1340,

    .act_size.HA = 1080,
    .act_size.VA = 1920,
};


// init layer
static void kendryte_layer_init(struct kendryte_vo *vop)
{
    unsigned int i;

    for(i = 0; i < KENDRYTE_VO_NUM_LAYERS; i++)
    {
        // set pingpang mode 
        kendryte_vo_set_pingpang_mode(vop, i + LAYER0, VO_LAYER_PINGPANG_AUTO) ;
        kendryte_vo_set_layer_format(vop, i + 1, VO_FORMAT_YUV420);

        struct kendryte_layer *layer = &vop->layer[i];
        layer->vo = vop;
        layer->plane.type = DRM_PLANE_TYPE_PRIMARY;
        
    }

    // init layer 0 - 3
    for(i = 0; i < 4; i++)
    {
        // set layer uv endian
        kendryte_set_layer_uv_endian(vop, i + LAYER0, VO_VIDEO_LAYER_UV_ENDIAN_MODE2);
        // set layer y endian
        kendryte_set_layer_y_endian(vop, i + LAYER0, VO_VIDEO_LAYER_Y_ENDIAN_MODE1);
    }

    // init vo osd0 
    for(i = 0; i < 3; i++)
    {
        // set ARGB888 
        kendryte_vo_set_osd_alpha_tpye(vop, OSD0 + i, 5);

    } 


}

// vo init,
static int kendryte_vo_init(struct kendryte_vo *vop)
{
    int i, ret;

    //software reset 
//    kendryte_vo_software_reset(vop);

    msleep(100);
    
    // close all display 
//    kendryte_display_crtl(vop, 1);

    //config wrap
//    kendryte_vo_wrap_init(vop);
    //config table
//    kendryte_vo_table_init(vop);

    // config vo timing
 //   kendryte_vo_timing_init(vop, &vo_timing);

    //set vo burst len
//    kendryte_vo_blenth_init(vop);

    kendryte_layer_init(vop);

    // config back ground color 
 //   kendryte_vo_set_yuv_background(vop, 0x80, 0x80, 0x80);

    //config disp mix
//    kendryte_vo_set_config_mix(vop);

//    kendryte_vo_set_config_done(vop);

    return 0;
}




static int kendryte_layer_bind(struct device *dev, struct device *master, void *data)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct kendryte_vo *vop = dev_get_drvdata(dev);
    struct drm_device *drm_dev = data;
    int ret;

    vop->drm = drm_dev;

    ret = kendryte_vo_init(vop);
	if (ret < 0) {
		DRM_DEV_ERROR(&pdev->dev,
			      "cannot initial vop dev - err %d\n", ret);
		goto err_disable_pm_runtime;
	}

/*
	spin_lock_init(&vop->reg_lock);
	spin_lock_init(&vop->irq_lock);
	mutex_init(&vop->vop_lock);
*/
    // crtc
	ret = kendryte_create_crtc(vop);
	if (ret)
		return ret;
  
//   kendryte_vo_get_reg_val(vop);

	return 0;

err_disable_pm_runtime:

//	vop_destroy_crtc(vop);
	return ret;
}


static void kendryte_layer_unbind(struct device *dev, struct device *master, void *data)
{
	struct kendryte_vo *vop = dev_get_drvdata(dev);

    printk("kendryte_layer_unbind \n");
}

const struct component_ops kendryte_layer_component_ops = {
	.bind = kendryte_layer_bind,
	.unbind = kendryte_layer_unbind,
};


/*
     vo apb clk  is 2f 
[   35.936011] vo axi clk  is 3
*/
static void clk_get_reg(void)
{
    
    uint32_t *addr = ioremap(0x97001000 + 0x118, 0x08);         // crb 0x16               0x2f
    uint32_t *addr2 = ioremap(0x97001000 + 0x11c, 0x08);        // crb 0x02               0x3     没有问题 
  
    printk("vo apb clk  is %x \n", readl(addr));
    printk("vo axi clk  is %x \n", readl(addr2));

    iounmap(addr2);
    iounmap(addr);
}

static void vop_fb_unref_worker(struct drm_flip_work *work, void *val)
{
	struct kendryte_vo *vop = container_of(work, struct kendryte_vo, fb_unref_work);
	struct drm_framebuffer *fb = val;

	drm_crtc_vblank_put(&vop->crtc);
	drm_framebuffer_put(fb);
}


static void vop_handle_vblank(struct kendryte_vo *vop)
{
	struct drm_device *drm = vop->drm;
	struct drm_crtc *crtc = &vop->crtc;
	unsigned long flags;

	spin_lock_irqsave(&drm->event_lock, flags);
	if (vop->event) {
		drm_crtc_send_vblank_event(crtc, vop->event);
		drm_crtc_vblank_put(crtc);
		vop->event = NULL;
	}
	spin_unlock_irqrestore(&drm->event_lock, flags);

	if (test_and_clear_bit(VOP_PENDING_FB_UNREF, &vop->pending))
		drm_flip_work_commit(&vop->fb_unref_work, system_unbound_wq);
}


static irqreturn_t vop_isr(int irq, void *data)
{
	struct kendryte_vo *vop = data;
	struct drm_crtc *crtc = &vop->crtc;
	uint32_t active_irqs;
	unsigned long flags;
	int ret = IRQ_NONE;

    // clear irq flag ;
    kendryte_vo_clear_intr(vop);

//    printk("crtc  vop_isr is start \n");
    // pull event
    drm_crtc_handle_vblank(crtc);
	vop_handle_vblank(vop);

    return IRQ_HANDLED;
}


static int kendryte_layer_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    
	struct kendryte_vo *vop;
	struct resource *res;
    size_t alloc_size;
	int ret, irq;

    printk("kendryte_layer_probe start \n");

    if (!dev->of_node) {
		DRM_DEV_ERROR(dev, "can't find vop devices\n");
		return -ENODEV;
	}

    alloc_size = sizeof(*vop) + sizeof(*vop->layer) * KENDRYTE_VO_NUM_LAYERS;
//    vop->layer_size = alloc_size;

	/* Allocate vop struct and its vop_win array */
	vop = devm_kzalloc(dev, alloc_size, GFP_KERNEL);
	if (!vop)
		return -ENOMEM;

	vop->dev = dev;
	
    vop->vo_timing = &vo_timing;

//	dev_set_drvdata(dev, vop);
    platform_set_drvdata(pdev, vop);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	vop->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(vop->base))
		return PTR_ERR(vop->base);


	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		DRM_DEV_ERROR(dev, "cannot find irq for vop\n");
		return irq;
	}
	vop->irq_vo = (unsigned int)irq;

    vop->vo_apb = devm_clk_get(vop->dev, "vo_apb");
	if (IS_ERR(vop->vo_apb)) {
		DRM_DEV_ERROR(vop->dev, "failed to get vo_apb source\n");
		return PTR_ERR(vop->vo_apb);
	}

	vop->vo_axi = devm_clk_get(vop->dev, "vo_axi");
	if (IS_ERR(vop->vo_axi)) {
		DRM_DEV_ERROR(vop->dev, "failed to get vo_axi source\n");
		return PTR_ERR(vop->vo_axi);
	}
    
    ret = clk_prepare_enable(vop->vo_apb);
	if (ret < 0) {
		DRM_DEV_ERROR(vop->dev, "failed to prepare/enable vo_apb\n");
		return PTR_ERR(vop->vo_apb);
	}

    ret = clk_prepare_enable(vop->vo_axi);
	if (ret < 0) {
		DRM_DEV_ERROR(vop->dev, "failed to prepare/enable vo_apb\n");
		return PTR_ERR(vop->vo_axi);
	}

    spin_lock_init(&vop->irq_lock);
    
	drm_flip_work_init(&vop->fb_unref_work, "fb_unref", vop_fb_unref_worker);
			   
    clk_get_reg();

    platform_set_drvdata(pdev, vop);


	ret = devm_request_irq(dev, vop->irq_vo, vop_isr,
			       IRQF_SHARED, dev_name(dev), vop);
	if (ret)
    {
        printk("devm_request_irq error \n");
    }
		
    return component_add(dev, &kendryte_layer_component_ops);
}


static int kendryte_layer_remove(struct platform_device *pdev)
{
        
    return 0;
}



static int __maybe_unused kendryte_layer_runtime_resume(struct device *dev)
{


    return 0;
}


static int __maybe_unused kendryte_layer_runtime_suspend(struct device *dev)
{

        return 0;
}


static const struct dev_pm_ops kendryte_layer_pm_ops = {
    SET_RUNTIME_PM_OPS(kendryte_layer_runtime_suspend,
                        kendryte_layer_runtime_resume,
                        NULL)
};


static const struct of_device_id kendryte_layer_of_table[] = {
    { .compatible = "kendryte,kendryte-k510-vop" },
    { }
};



struct platform_driver kendryte_layer_driver = {
    .probe          = kendryte_layer_probe,
    .remove         = kendryte_layer_remove,
    .driver         = {
        .name           = "kendryte_layer",
        .of_match_table = kendryte_layer_of_table,
        .pm             = &kendryte_layer_pm_ops,
    },
};
EXPORT_SYMBOL(kendryte_layer_driver);
//module_platform_driver(kendryte_layer_driver);


MODULE_DESCRIPTION("kendryte 510 layer Driver");
MODULE_LICENSE("GPL");
