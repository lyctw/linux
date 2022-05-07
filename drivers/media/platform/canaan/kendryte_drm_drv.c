/* 
 * Copyright (c) 2022 Canaan Bright Sight Co., Ltd
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2014-2016 Hisilicon Limited.
 *
 * Author:
 *      Canaan Bright Sight Co., Ltd
 *      Xinliang Liu <z.liuxinliang@hisilicon.com>
 *      Xinliang Liu <xinliang.liu@linaro.org>
 *      Xinwei Kong <kong.kongxinwei@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/of_platform.h>
#include <linux/component.h>
#include <linux/of_graph.h>

#include <drm/drmP.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>

#include "kendryte_drm_drv.h"

static struct kendryte_dc_ops *dc_ops;

static int kendryte_drm_kms_cleanup(struct drm_device *dev)
{
	struct kendryte_drm_private *priv = dev->dev_private;

	if (priv->fbdev) {
		drm_fbdev_cma_fini(priv->fbdev);
		priv->fbdev = NULL;
	}

	drm_kms_helper_poll_fini(dev);
	dc_ops->cleanup(to_platform_device(dev->dev));
	drm_mode_config_cleanup(dev);
	devm_kfree(dev->dev, priv);
	dev->dev_private = NULL;

	return 0;
}

static void kendryte_fbdev_output_poll_changed(struct drm_device *dev)
{
	struct kendryte_drm_private *priv = dev->dev_private;

	drm_fbdev_cma_hotplug_event(priv->fbdev);
}

static const struct drm_mode_config_funcs kendryte_drm_mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
	.output_poll_changed = kendryte_fbdev_output_poll_changed,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static void kendryte_drm_mode_config_init(struct drm_device *dev)
{
	dev->mode_config.min_width = 0;
	dev->mode_config.min_height = 0;

	dev->mode_config.max_width = 2048;
	dev->mode_config.max_height = 2048;

	dev->mode_config.funcs = &kendryte_drm_mode_config_funcs;
}

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
	kendryte_drm_mode_config_init(dev);

	/* display controller init */
	ret = dc_ops->init(to_platform_device(dev->dev));
	if (ret)
		goto err_mode_config_cleanup;

    printk("vo drm init done\n");

	/* bind and init sub drivers */
	ret = component_bind_all(dev->dev, dev);
	if (ret) {
		DRM_ERROR("failed to bind all component.\n");
		goto err_dc_cleanup;
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

    printk("drm_kms_helper_poll_init done, num_connector %d \n",dev->mode_config.num_connector);

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
err_dc_cleanup:
	dc_ops->cleanup(to_platform_device(dev->dev));
err_mode_config_cleanup:
	drm_mode_config_cleanup(dev);
	devm_kfree(dev->dev, priv);
	dev->dev_private = NULL;

    printk("kendryte_drm_kms_init *ERR* exit\n");

	return ret;
}
phys_addr_t pa_msb = 0x100000;

static int kendryte_gem_mmap(struct file *filp, struct vm_area_struct *vma)
{
    printk("kendryte_gem_mmap enter\n");
    
	struct drm_gem_cma_object *cma_obj;
	struct drm_gem_object *gem_obj;
	int ret;

	ret = drm_gem_mmap(filp, vma);
	if (ret)
		return ret;

	gem_obj = vma->vm_private_data;
	cma_obj = to_drm_gem_cma_obj(gem_obj);

	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;

    unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long user_count = vma_pages(vma);
	unsigned long count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	unsigned long off = vma->vm_pgoff;
    void * cpu_addr = cma_obj->vaddr;

printk("mmap pa addr 0x%lx, vaddr 0x%lx, size %d\n", cma_obj->paddr, cpu_addr,size);
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (dma_mmap_from_dev_coherent(cma_obj->base.dev->dev, vma, cpu_addr, size, &ret))
		return ret;
	if (off < count && user_count <= (count - off))
    {   
        printk("remap pfn range, pa_msb 0x%lx, off %d, pfn %x, page %x, virt_to_pfn %x,vmalloc_to_pfn %x\n",pa_msb, 
            off, page_to_pfn(virt_to_page(cpu_addr)), virt_to_page(cpu_addr), virt_to_pfn(cpu_addr), vmalloc_to_pfn(cpu_addr));
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
    printk("kendryte_gem_mmap exit\n");
    
	return ret;
}

static int kendrtye_prime_mmap(struct drm_gem_object *obj,
			   struct vm_area_struct *vma)
{

    printk("kendrtye_prime_mmap enter\n");

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

    
    printk("kendrtye_prime_mmap exit\n");
	return ret;
}

//DEFINE_DRM_GEM_CMA_FOPS(kendryte_drm_fops);

static const struct file_operations kendryte_drm_driver_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.mmap = kendryte_gem_mmap,
	.poll = drm_poll,
	.read = drm_read,
	.unlocked_ioctl = drm_ioctl,
	.compat_ioctl = drm_compat_ioctl,
	.release = drm_release,
	.llseek		= noop_llseek,\	
};


static int kendryte_gem_cma_dumb_create(struct drm_file *file,
				     struct drm_device *dev,
				     struct drm_mode_create_dumb *args)
{
    printk("kendryte_gem_cma_dumb_create called\n");
	return drm_gem_cma_dumb_create_internal(file, dev, args);
}



static struct drm_driver kendryte_drm_driver = {
	.driver_features	= DRIVER_GEM | DRIVER_MODESET | DRIVER_PRIME |
				  DRIVER_ATOMIC,
	.fops			= &kendryte_drm_driver_fops, //&kendryte_drm_fops,
	.gem_free_object_unlocked = drm_gem_cma_free_object,
	.gem_vm_ops		= &drm_gem_cma_vm_ops,
	.dumb_create		= kendryte_gem_cma_dumb_create,

	.prime_handle_to_fd	= drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle	= drm_gem_prime_fd_to_handle,
	.gem_prime_export	= drm_gem_prime_export,
	.gem_prime_import	= drm_gem_prime_import,
	.gem_prime_get_sg_table = drm_gem_cma_prime_get_sg_table,
	.gem_prime_import_sg_table = drm_gem_cma_prime_import_sg_table,
	.gem_prime_vmap		= drm_gem_cma_prime_vmap,
	.gem_prime_vunmap	= drm_gem_cma_prime_vunmap,
	.gem_prime_mmap		= kendrtye_prime_mmap, // drm_gem_cma_prime_mmap,

	.name			= "kendryte-drm",
	.desc			= "Canaan Kendryte DRM Driver",
	.date			= "20201015",
	.major			= 1,
	.minor			= 0,
};

static int compare_of(struct device *dev, void *data)
{
	return dev->of_node == data;
}

static int kendryte_drm_bind(struct device *dev)
{
	struct drm_driver *driver = &kendryte_drm_driver;
	struct drm_device *drm_dev;
	int ret;

    printk("drm_debug, kendryte_drm_bind enter\n");
	drm_dev = drm_dev_alloc(driver, dev);
	if (IS_ERR(drm_dev))
		return PTR_ERR(drm_dev);

	ret = kendryte_drm_kms_init(drm_dev);
	if (ret)
		goto err_drm_dev_unref;

	ret = drm_dev_register(drm_dev, 0);
	if (ret)
		goto err_kms_cleanup;

    printk("kendryte_drm_bind normally exit");
	return 0;

err_kms_cleanup:
	kendryte_drm_kms_cleanup(drm_dev);
err_drm_dev_unref:
	drm_dev_unref(drm_dev);

    printk("kendryte_drm_bind error exit");

	return ret;
}

static void kendryte_drm_unbind(struct device *dev)
{
	struct drm_device *drm_dev = dev_get_drvdata(dev);

	drm_dev_unregister(drm_dev);
	kendryte_drm_kms_cleanup(drm_dev);
	drm_dev_unref(drm_dev);
}

static const struct component_master_ops kendryte_drm_ops = {
	.bind = kendryte_drm_bind,
	.unbind = kendryte_drm_unbind,
};

static int kendryte_drm_platform_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct component_match *match = NULL;
	struct device_node *remote;

    printk("drm_debug, %s enter\n", __func__);
	dc_ops = (struct kendryte_dc_ops *)of_device_get_match_data(dev);
    
    printk("drm_debug, dc_ops %lx\n", dc_ops);
	if (!dc_ops) {
        printk("drm_debug, failed to get dt id data\n");
        
		DRM_ERROR("failed to get dt id data\n");
		return -EINVAL;
	}

	remote = of_graph_get_remote_node(np, 0, 0);
    printk("drm_debug, remote %lx\n", remote);
    
	if (!remote)
		return -ENODEV;
    printk("drm_debug, got remote node\n");

	drm_of_component_match_add(dev, &match, compare_of, remote);
    printk("drm_debug, drm comp match %lx added\n", match);

	of_node_put(remote);
    printk("drm_debug, of node put\n");

    int ret = component_master_add_with_match(dev, &kendryte_drm_ops, match);
    printk("drm_debug, comp add match ret = %d\n", ret);
    return ret;

}

static int kendryte_drm_platform_remove(struct platform_device *pdev)
{
	component_master_del(&pdev->dev, &kendryte_drm_ops);
	dc_ops = NULL;
	return 0;
}

static const struct of_device_id kendryte_drm_dt_ids[] = {
	{ .compatible = "kendryte,k510-vo",
	  .data = &vo_dc_ops,
	},
	{ /* end node */ },
};
MODULE_DEVICE_TABLE(of, kendryte_drm_dt_ids);

static struct platform_driver kendryte_drm_platform_driver = {
	.probe = kendryte_drm_platform_probe,
	.remove = kendryte_drm_platform_remove,
	.driver = {
		.name = "kendrtye-drm",
		.of_match_table = kendryte_drm_dt_ids,
	},
};

module_platform_driver(kendryte_drm_platform_driver);

MODULE_DESCRIPTION("Canaan Kendryte DRM driver");
MODULE_LICENSE("GPL v2");

