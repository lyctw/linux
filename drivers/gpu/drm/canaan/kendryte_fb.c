#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>

#include <drm/drmP.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>

#include "kendryte_drv.h"
#include "kendryte_fb.h"
#include "kendryte_layer.h"


static int kendryte_atomic_check(struct drm_device *dev,
                                 struct drm_atomic_state *state)
{
    int ret;

    ret = drm_atomic_helper_check_modeset(dev, state);
    if (ret)
            return ret;
    ret = drm_atomic_normalize_zpos(dev, state);
    if (ret)
            return ret;
	ret = drm_atomic_helper_check_planes(dev, state);		

    return ret;
}


static const struct drm_mode_config_funcs kendryte_de_mode_config_funcs = {
    .atomic_check           = kendryte_atomic_check,
    .atomic_commit          = drm_atomic_helper_commit,//kendryte_drm_atomic_commit,//drm_atomic_helper_commit,
    .fb_create              = drm_gem_fb_create,
};


static struct drm_mode_config_helper_funcs kendryte_de_mode_config_helpers = {
    .atomic_commit_tail     = drm_atomic_helper_commit_tail_rpm,
};

void kendryte_framebuffer_init(struct drm_device *drm)
{
 //   drm_mode_config_reset(drm);           // reset all drm

    drm->mode_config.max_width = 8192;
    drm->mode_config.max_height = 8192;
    drm->mode_config.funcs = &kendryte_de_mode_config_funcs;
    drm->mode_config.helper_private = &kendryte_de_mode_config_helpers;

}




MODULE_LICENSE("GPL");
