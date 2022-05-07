/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#ifndef _KENDRYTE_DRM_FBDEV_H
#define _KENDRYTE_DRM_FBDEV_H



struct kendryte_gem_object {
	struct drm_gem_object base;
    struct drm_fbdev_cma *fbdev_cma ;
	unsigned int flags;
    struct drm_gem_cma_object *obj ;
	void *kvaddr;
	void *cookie;
	dma_addr_t dma_addr;
	dma_addr_t dma_handle;

	size_t size;
};

//#define CONFIG_DRM_FBDEV_EMULATION

#ifdef CONFIG_DRM_FBDEV_EMULATION
int kendryte_drm_fbdev_init(struct drm_device *dev);
void kendryte_drm_fbdev_fini(struct drm_device *dev);
#else
static inline int kendryte_drm_fbdev_init(struct drm_device *dev)
{
	return 0;
}

static inline void kendryte_drm_fbdev_fini(struct drm_device *dev)
{
    
}



#endif

#endif 

