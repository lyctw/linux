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

#include <linux/component.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_of.h>

#include <drm/drm_vblank.h>

#include <linux/of_platform.h>
#include <linux/reset.h>

#include <drm/drmP.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_crtc_helper.h>


#include "kendryte_drv.h"
#include "kendryte_layer.h"
#include "kendryte_fb.h"
#include "kendryte_dsi.h"
#include "kendryte_drm_fbdev.h"

#include "kendryte_vo.h"

static int num_kendryte_sub_drivers;
#define MAX_KENDRYTE_SUB_DRIVERS 				16
static struct platform_driver *kendryte_sub_drivers[MAX_KENDRYTE_SUB_DRIVERS];
//
static const char *k510disp_resets[] = {
	"dsi_rst",
	"bt1120_rst",
	"twdo_rst",
	"vo_rst",
};

static enum disp_rst_e{
	DSI_RST,
	BT1120_RST,
	TWDO_RST,
	VO_RST,
	DISP_MAX,
};
static struct reset_control *disp_reset_control[DISP_MAX];
//phys_addr_t pa_msb = 0x100000;

static int kendryte_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{

	struct drm_gem_cma_object *cma_obj;
	struct drm_gem_object *gem_obj;
	int ret;

	ret = drm_gem_mmap(filp, vma);
	if (ret)
	{
		printk("kendryte_gem_mmap enter 22222\n");
		return ret;
	}
		
	gem_obj = vma->vm_private_data;
	cma_obj = to_drm_gem_cma_obj(gem_obj);

	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;

    unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long user_count = vma_pages(vma);
	unsigned long count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	unsigned long off = vma->vm_pgoff;
    void * cpu_addr = cma_obj->vaddr;

//    printk("mmap pa addr 0x%lx, vaddr 0x%lx, size %d\n", cma_obj->paddr, cpu_addr,size);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (dma_mmap_from_dev_coherent(cma_obj->base.dev->dev, vma, cpu_addr, size, &ret))
		return ret;
	if (off < count && user_count <= (count - off))
    {   
//        printk("remap pfn range, pa_msb 0x%lx, off %d, pfn %x, page %x, virt_to_pfn %x,vmalloc_to_pfn %x\n",pa_msb, 
//            off, page_to_pfn(virt_to_page(cpu_addr)), virt_to_page(cpu_addr), virt_to_pfn(cpu_addr), vmalloc_to_pfn(cpu_addr));
        /*
		ret = remap_pfn_range(vma, vma->vm_start,
				      (page_to_pfn(virt_to_page(cpu_addr)) + off) | 0x100000,
				      user_count << PAGE_SHIFT,
				      vma->vm_page_prot);
        */
		ret = remap_pfn_range(vma, vma->vm_start,
		       (vmalloc_to_pfn(cpu_addr) + off)/*(cma_obj->paddr >> PAGE_SHIFT )*//*| 0x100000*/,
		      user_count << PAGE_SHIFT,
		      vma->vm_page_prot);
    }
  
	return ret;
}

static const struct file_operations kendryte_drm_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.mmap = kendryte_gem_mmap,
	.poll = drm_poll,
	.read = drm_read,
	.unlocked_ioctl = drm_ioctl,
	.compat_ioctl = drm_compat_ioctl,
	.release = drm_release,
	.llseek		= noop_llseek,
};

static int kendryte_gem_cma_dumb_create(struct drm_file *file,
				     struct drm_device *dev,
				     struct drm_mode_create_dumb *args)
{


	return drm_gem_cma_dumb_create_internal(file, dev, args);
}

static int kendryte_prime_mmap(struct drm_gem_object *obj,
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

//    printk("remap pfn range, pa_msb 0x%lx, off %d, pfn %x, page %x, virt_to_pfn %x,vmalloc_to_pfn %x\n",pa_msb, 
//        off, page_to_pfn(virt_to_page(cpu_addr)), virt_to_page(cpu_addr), virt_to_pfn(cpu_addr), vmalloc_to_pfn(cpu_addr));


    if (off < count && user_count <= (count - off))
        ret = remap_pfn_range(vma, vma->vm_start,
                      (vmalloc_to_pfn(cpu_addr) + off)/* | pa_msb*/,
                      user_count << PAGE_SHIFT,
                      vma->vm_page_prot);

    
	if (ret)
		drm_gem_vm_close(vma);

    
	return ret;
}


static int kendryte_drm_draw_frame_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_crtc *crtc;
	struct kendryte_vo *vo;
	struct vo_draw_frame *frame = data;
	int ret = 0;	
	crtc = drm_crtc_find(dev, file_priv, frame->crtc_id);

	vo = to_vop(crtc);

	ret = kendryte_vo_draw_frame(vo, frame);

	return ret;
}

static int kendryte_drm_reset_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	int ret = 0;
	reset_control_reset(disp_reset_control[VO_RST]);
	reset_control_reset(disp_reset_control[DSI_RST]);
	printk("%s:reset_control_reset\n",__func__);
	return ret;
}

static const struct drm_ioctl_desc kendryte_vo_ioctls[] = {
    DRM_IOCTL_DEF_DRV(KENDRYTE_DRAW_FRAME, kendryte_drm_draw_frame_ioctl,
                        DRM_AUTH | DRM_RENDER_ALLOW),
    DRM_IOCTL_DEF_DRV(KENDRYTE_RESET, kendryte_drm_reset_ioctl,
                        DRM_AUTH | DRM_RENDER_ALLOW),
};


static struct drm_driver kendryte_drm_driver = {
	.driver_features	= DRIVER_GEM | DRIVER_MODESET | DRIVER_PRIME |
				  DRIVER_ATOMIC,
	.fops			= &kendryte_drm_driver_fops, //&kendryte_drm_fops,
	.gem_free_object_unlocked = drm_gem_cma_free_object,
	.gem_vm_ops		= &drm_gem_cma_vm_ops,
	.dumb_create		= kendryte_gem_cma_dumb_create,
	//.dumb_destroy		= drm_gem_dumb_destroy,
	.prime_handle_to_fd	= drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle	= drm_gem_prime_fd_to_handle,
	.gem_prime_export	= drm_gem_prime_export,
	.gem_prime_import	= drm_gem_prime_import,
	.gem_prime_get_sg_table = drm_gem_cma_prime_get_sg_table,
	.gem_prime_import_sg_table = drm_gem_cma_prime_import_sg_table,
	.gem_prime_vmap		= drm_gem_cma_prime_vmap,
	.gem_prime_vunmap	= drm_gem_cma_prime_vunmap,
	.gem_prime_mmap		= kendryte_prime_mmap, // drm_gem_cma_prime_mmap,
	.ioctls				= kendryte_vo_ioctls, //kendryte_vo_ioctls,
	.num_ioctls			= ARRAY_SIZE(kendryte_vo_ioctls),

	.name			= "kendryte-drm",
	.desc			= "Canaan Kendryte DRM Driver",
	.date			= "2021104",
	.major			= 1,
	.minor			= 0,
};

static int kendryte_drm_kms_init(struct drm_device *dev)
{
	struct kendryte_drm_private *priv;
	int ret;

    printk("kendryte_drm_kms_init enter\n");
	priv = devm_kzalloc(dev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	dev->dev_private = priv;
	dev_set_drvdata(dev->dev, dev);

	/* dev->mode_config initialization */
	drm_mode_config_init(dev);

	/* bind and init sub drivers */
	ret = component_bind_all(dev->dev, dev);
	if (ret) {
		DRM_ERROR("failed to bind all component.\n");

	}
    
    printk("com bind all done, num_crtc %d\n",dev->mode_config.num_crtc );

	/* vblank init */
	ret = drm_vblank_init(dev, dev->mode_config.num_crtc);
	if (ret) {
		DRM_ERROR("failed to initialize vblank.\n");
		goto err_unbind_all;
	}

    
    printk("drm_vblank_init done\n");
	/* with irq_enabled = true, we can use the vblank feature. */
	dev->irq_enabled = true;

	/* reset all the states of crtc/plane/encoder/connector */
	drm_mode_config_reset(dev);

	/* init kms poll for handling hpd */
	drm_kms_helper_poll_init(dev);

/*temp comment out for debug
	priv->fbdev = drm_fbdev_cma_init(dev, 32,
					 dev->mode_config.num_connector);

	if (IS_ERR(priv->fbdev)) {
		DRM_ERROR("failed to initialize fbdev.\n");
		ret = PTR_ERR(priv->fbdev);
		goto err_cleanup_poll;
	}
*/

    printk("kendryte_drm_kms_init normally exit\n");
	return 0;

err_cleanup_poll:
	drm_kms_helper_poll_fini(dev);
err_unbind_all:
	component_unbind_all(dev->dev, dev);

err_mode_config_cleanup:
	drm_mode_config_cleanup(dev);
	devm_kfree(dev->dev, priv);
	dev->dev_private = NULL;

    printk("kendryte_drm_kms_init *ERR* exit\n");

	return ret;
}

static int kendryte_drm_kms_cleanup(struct drm_device *dev)
{
	struct kendryte_drm_private *priv = dev->dev_private;

	if (priv->fbdev) {
		drm_fbdev_cma_fini(priv->fbdev);
		priv->fbdev = NULL;
	}

	drm_kms_helper_poll_fini(dev);
//	dc_ops->cleanup(to_platform_device(dev->dev));
	drm_mode_config_cleanup(dev);
	devm_kfree(dev->dev, priv);
	dev->dev_private = NULL;

	return 0;
}


static int kendryte_drm_create_properties(struct drm_device *dev)
{
	struct drm_property *prop;
	struct kendryte_drm_private *private = dev->dev_private;
	int ret ;

	prop = drm_property_create_range(dev, DRM_MODE_PROP_ATOMIC,
					 "EOTF", 0, 5);
	if (!prop)
		return -ENOMEM;

	private->eotf_prop = prop;

	prop = drm_property_create_range(dev, DRM_MODE_PROP_ATOMIC,
					 "COLOR_SPACE", 0, 12);
	if (!prop)
		return -ENOMEM;
	private->color_space_prop = prop;

	prop = drm_property_create_range(dev, DRM_MODE_PROP_ATOMIC,
					 "GLOBAL_ALPHA", 0, 255);
	if (!prop)
		return -ENOMEM;
	private->global_alpha_prop = prop;

	prop = drm_property_create_range(dev, DRM_MODE_PROP_ATOMIC,
					 "BLEND_MODE", 0, 1);
	if (!prop)
		return -ENOMEM;
	private->blend_mode_prop = prop;

	prop = drm_property_create_range(dev, DRM_MODE_PROP_ATOMIC,
					 "ALPHA_SCALE", 0, 1);
	if (!prop)
		return -ENOMEM;
	private->alpha_scale_prop = prop;

	prop = drm_property_create_range(dev, DRM_MODE_PROP_ATOMIC,
					 "ASYNC_COMMIT", 0, 1);
	if (!prop)
		return -ENOMEM;
	private->async_commit_prop = prop;

	prop = drm_property_create_range(dev, DRM_MODE_PROP_ATOMIC,
					 "SHARE_ID", 0, UINT_MAX);
	if (!prop)
		return -ENOMEM;
	private->share_id_prop = prop;

	ret = drm_mode_create_tv_properties(dev, 0, NULL);

	return ret;
}

static void show_loader_logo(struct drm_device *drm_dev)
{
	printk("show_loader_logo start \n");
}


static bool is_support_hotplug(uint32_t output_type)
{
	switch (output_type) {
	case DRM_MODE_CONNECTOR_DVII:
	case DRM_MODE_CONNECTOR_DVID:
	case DRM_MODE_CONNECTOR_DVIA:
	case DRM_MODE_CONNECTOR_DisplayPort:
	case DRM_MODE_CONNECTOR_HDMIA:
	case DRM_MODE_CONNECTOR_HDMIB:
	case DRM_MODE_CONNECTOR_TV:
		return true;
	default:
		return false;
	}
}

static int kendryte_drm_bind(struct device *dev)
{
	struct drm_device *drm_dev;
	struct kendryte_drm_private *private;
	int ret;
	struct device_node *np = dev->of_node;
	struct device_node *parent_np;
	struct drm_crtc *crtc;

	drm_dev = drm_dev_alloc(&kendryte_drm_driver, dev);
	if (IS_ERR(drm_dev))
		return PTR_ERR(drm_dev);

	dev_set_drvdata(dev, drm_dev);

	private = devm_kzalloc(drm_dev->dev, sizeof(*private), GFP_KERNEL);
	if (!private) {
		ret = -ENOMEM;
		goto err_free;
	}

	drm_dev->dev_private = private;
	drm_mode_config_init(drm_dev);
	kendryte_framebuffer_init(drm_dev);	
	kendryte_drm_create_properties(drm_dev);

	ret = component_bind_all(dev, drm_dev);
	if (ret)
	{
		goto err_mode_config_cleanup;
	}
		
	ret = drm_vblank_init(drm_dev, drm_dev->mode_config.num_crtc);
	if (ret)
		goto err_unbind_all;

	drm_mode_config_reset(drm_dev);

//	kendryte_drm_set_property_default();

	/* init kms poll for handling hpd */
	drm_kms_helper_poll_init(drm_dev);

#ifndef MODULE
	show_loader_logo(drm_dev);
#endif
	ret = of_reserved_mem_device_init(drm_dev->dev);
	if (ret)
		DRM_DEBUG_KMS("No reserved memory region assign to drm\n");

#if 0 
	ret = kendryte_drm_fbdev_init(drm_dev);
	if (ret)
		goto err_kms_helper_poll_fini;
#endif

	drm_for_each_crtc(crtc, drm_dev) {
		struct drm_fb_helper *helper = private->fbdev_helper;
		struct kendryte_crtc_state *s = NULL;

		if (!helper)
			break;

		s = to_kendryte_crtc_state(crtc->state);
		if (is_support_hotplug(s->output_type))
			drm_framebuffer_get(helper->fb);
	}
	drm_dev->mode_config.allow_fb_modifiers = true;

	ret = drm_dev_register(drm_dev, 0);
	if (ret)
	{
		printk("ret is %d \n", ret);
		return ret ;
	}

	return 0;

err_kms_helper_poll_fini:
//	rockchip_gem_pool_destroy(drm_dev);
	drm_kms_helper_poll_fini(drm_dev);
err_unbind_all:
	component_unbind_all(dev, drm_dev);
err_mode_config_cleanup:
	drm_mode_config_cleanup(drm_dev);
err_free:
	drm_dev->dev_private = NULL;
	dev_set_drvdata(dev, NULL);
	drm_dev_put(drm_dev);
	return ret;

}

static void kendryte_drm_unbind(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	drm_dev_unregister(drm_dev);
	kendryte_drm_kms_cleanup(drm_dev);
	drm_dev_unref(drm_dev);
}


static const struct component_master_ops kendryte_drv_master_ops = {
	.bind	= kendryte_drm_bind,
	.unbind	= kendryte_drm_unbind,
};


static int compare_dev(struct device *dev, void *data)
{
	return dev == (struct device *)data;
}

static void kendryte_drm_match_remove(struct device *dev)
{
	struct device_link *link;

	list_for_each_entry(link, &dev->links.consumers, s_node)
		device_link_del(link);
}


static struct component_match *kendryte_drm_match_add(struct device *dev)
{
	struct component_match *match = NULL;
	int i;

	for (i = 0; i < num_kendryte_sub_drivers; i++) {
		struct platform_driver *drv = kendryte_sub_drivers[i];
		struct device *p = NULL, *d;

		do {
			d = bus_find_device(&platform_bus_type, p, &drv->driver,
					    (void *)platform_bus_type.match);
			put_device(p);
			p = d;

			if (!d)
				break;

			device_link_add(dev, d, DL_FLAG_STATELESS);
			component_match_add(dev, &match, compare_dev, d);
		} while (true);
	}

	if (IS_ERR(match))
		kendryte_drm_match_remove(dev);

	return match ?: ERR_PTR(-ENODEV);
}


static int kendryte_drm_platform_of_probe(struct device *dev)
{
	struct device_node *np = dev->of_node;
	struct device_node *port;
	bool found = false;
	int i;

	if (!np)
		return -ENODEV;

	for (i = 0;; i++) {

		port = of_parse_phandle(np, "ports", i);
		if (!port)
			break;

		if (!of_device_is_available(port->parent)) {
			of_node_put(port);
			continue;
		}

		found = true;

		of_node_put(port);
	}

	if (i == 0) {
		DRM_DEV_ERROR(dev, "missing 'ports' property\n");
		return -ENODEV;
	}

	if (!found) {
		DRM_DEV_ERROR(dev,
			      "No available vop found for display-subsystem.\n");
		return -ENODEV;
	}

	return 0;
}

static int kendryte_drv_probe(struct platform_device *pdev)
{
	struct component_match *match = NULL;
	struct device_node *np = pdev->dev.of_node;
    struct device_node *remote;
	struct device_node *port;
	struct device *dev = &pdev->dev;

	int i, ret, count = 0;

	ret = kendryte_drm_platform_of_probe(dev);
	if (ret)
	{
		printk("kendryte_drm_platform_of_probe failed \n");
		return ret;
	}
		

	match = kendryte_drm_match_add(dev);
	if (IS_ERR(match))
		return PTR_ERR(match);


	ret = component_master_add_with_match(&pdev->dev,
						       &kendryte_drv_master_ops,
						       match);

    printk("drm_debug, comp add match ret = %d\n", ret);
    
	
	for(i = 0; i < DISP_MAX;i++)
	{
		disp_reset_control[i] = devm_reset_control_get(&pdev->dev,k510disp_resets[i]);
	}
	printk("%s:disp_reset_control\n",__func__);

    return ret;
}

static int kendryte_drv_remove(struct platform_device *pdev)
{
	component_master_del(&pdev->dev, &kendryte_drv_master_ops);

	return 0;
}


#ifdef CONFIG_PM_SLEEP
static int kendryte_drv_drm_sys_suspend(struct device *dev)
{
        struct drm_device *drm = dev_get_drvdata(dev);

        return drm_mode_config_helper_suspend(drm);
}

static int kendryte_drv_drm_sys_resume(struct device *dev)
{
        struct drm_device *drm = dev_get_drvdata(dev);

        return drm_mode_config_helper_resume(drm);
}
#endif

static const struct dev_pm_ops kendryte_drv_drm_pm_ops = {
        SET_SYSTEM_SLEEP_PM_OPS(kendryte_drv_drm_sys_suspend,
                                kendryte_drv_drm_sys_resume)
};


static const struct of_device_id kendryte_drv_of_table[] = {
	{ .compatible = "kendryte,display-subsystem"},
	{ }
};
//MODULE_DEVICE_TABLE(of, kendryte_drv_of_table);


static struct platform_driver kendryte_drv_platform_driver = {
	.probe		= kendryte_drv_probe,
	.remove		= kendryte_drv_remove,
	.driver		= {
		.name		= "kendryte-drm",
		.of_match_table	= kendryte_drv_of_table,
		.pm = &kendryte_drv_drm_pm_ops,
	},
};
//module_platform_driver(kendryte_drv_platform_driver);



#define ADD_KENDRYTE_SUB_DRIVER(drv, cond) { \
	if (/*IS_ENABLED(cond) && */\
	    !WARN_ON(num_kendryte_sub_drivers >= MAX_KENDRYTE_SUB_DRIVERS)) \
		kendryte_sub_drivers[num_kendryte_sub_drivers++] = &drv; \
}

static int __init kendryte_drm_init(void)
{
	int ret;

	num_kendryte_sub_drivers = 0;
//	ADD_KENDRYTE_SUB_DRIVER(kendryte_drv_platform_driver, CONFIG_DRM_DRV);
	ADD_KENDRYTE_SUB_DRIVER(kendryte_dsi_driver, CONFIG_DRM_DSI);
	ADD_KENDRYTE_SUB_DRIVER(kendryte_layer_driver, CONFIG_DRM_LAYER);
				
	ret = platform_register_drivers(kendryte_sub_drivers,
					num_kendryte_sub_drivers);
	if (ret)
		return ret;

	ret = platform_driver_register(&kendryte_drv_platform_driver);
	if (ret)
		goto err_unreg_drivers;

	return 0;

err_unreg_drivers:
	platform_unregister_drivers(kendryte_sub_drivers,
				    num_kendryte_sub_drivers);
	return ret;
}


static void __exit kendryte_drm_fini(void)
{
	platform_driver_unregister(&kendryte_drv_platform_driver);

	platform_unregister_drivers(kendryte_sub_drivers,
				    num_kendryte_sub_drivers);
}

module_init(kendryte_drm_init);
module_exit(kendryte_drm_fini);


MODULE_AUTHOR("");
MODULE_DESCRIPTION("K510 Display Engine DRM/KMS Driver");
MODULE_LICENSE("GPL");


