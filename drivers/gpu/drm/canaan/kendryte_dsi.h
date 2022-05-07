/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _KENDRYTE_DSI_H_
#define _KENDRYTE_DSI_H_

#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>
#include <drm/drm_mipi_dsi.h>

#include "kendryte_drv.h"



struct kendryte_dsi_timing {

    int  HACT;
    int  VACT;

    int  HSA;
    int  HBP;
    int  HFP;

    int  VSA;
    int  VBP;
    int  VFP;
};


struct kendryte_dsi {

    struct drm_connector                        connector;
    struct drm_encoder                          encoder;
    struct mipi_dsi_host                        host;
    struct drm_bridge                           *bridge;
    struct drm_panel                            *panel;

    int                                         id;
    int                                         irq;
    struct drm_display_mode                     mode;
    struct clk                                  *dsi_apb;
    struct clk                                  *dsi_system;
    struct clk                                  *dsi_pixel;
//    struct regmap           *regs;
//        struct reset_control    *reset;
    struct phy                                  *phy;
    struct kendryte_dsi_timing                  *timing;

    struct device                               *dev;
    struct device_node                          *client;
    void __iomem                                *base;
    struct kendryte_drv                         *drv;
    struct mipi_dsi_device                      *device;
    
    unsigned int                                lane_mbps; /* per lane */
	uint32_t                                    channel;
	uint32_t                                    lanes;
	uint32_t                                    format;
	unsigned long                               mode_flags;

    unsigned int                                frame_rate;

};


extern struct platform_driver kendryte_dsi_driver;

static inline struct kendryte_dsi *host_to_kendryte_dsi(struct mipi_dsi_host *host)
{
    return container_of(host, struct kendryte_dsi, host);
};

static inline struct kendryte_dsi *connector_to_kendryte_dsi(struct drm_connector *connector)
{
    return container_of(connector, struct kendryte_dsi, connector);
};

static inline struct kendryte_dsi *encoder_to_kendryte_dsi(const struct drm_encoder *encoder)
{
    return container_of(encoder, struct kendryte_dsi, encoder);
};


#endif
