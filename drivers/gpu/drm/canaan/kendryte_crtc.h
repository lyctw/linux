/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
*/

#ifndef _KENDRYTE_CRTC_H_
#define _KENDRYTE_CRTC_H_


struct kendryte_crtc {
    struct drm_crtc                 crtc;
    struct drm_pending_vblank_event *event;

    struct kendryte_vo              *vo;
};



static inline struct kendryte_crtc *drm_crtc_to_kendryte_crtc(struct drm_crtc *crtc)
{
    return container_of(crtc, struct kendryte_crtc, crtc);
}


int kendryte_create_crtc(struct kendryte_vo *vop);

#endif 
