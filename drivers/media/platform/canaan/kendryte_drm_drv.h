/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __KIRIN_DRM_DRV_H__
#define __KIRIN_DRM_DRV_H__

#define MAX_CRTC	2

/* display controller init/cleanup ops */
struct kendryte_dc_ops {
	int (*init)(struct platform_device *pdev);
	void (*cleanup)(struct platform_device *pdev);
};

struct kendryte_drm_private {
	struct drm_fbdev_cma *fbdev;
};

extern const struct kendryte_dc_ops vo_dc_ops;

#endif /* __KIRIN_DRM_DRV_H__ */

