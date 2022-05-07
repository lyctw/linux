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


#include "vo_table.h"
#include "kendryte_vo.h"


void kendryte_vo_enable_vblank(struct kendryte_vo *vo, bool enable)
{


    printk("%sabling VBLANK interrupt\n", enable ? "En" : "Dis");

}

EXPORT_SYMBOL(kendryte_vo_enable_vblank);





void kendryte_vo_wrap_init(struct kendryte_vo *vo)
{
    uint8_t i = 0;

 
    writel(0x11, vo->base + VO_DMA_SW_CTL);
    writel(0x88, vo->base + VO_DMA_RD_CTL_OUT);
    writel(0x0, vo->base + VO_DMA_ARB_MODE);
    
    for(i = 0; i < 8; i++)
    {
        writel(0x1010101, vo->base + 0x14 + (4 * i));
    }

    writel(0x76543210, vo->base + 0x34);
    writel(0x76543210, vo->base + 0x3c);
    writel(0x76543210, vo->base + 0x44);
    writel(0x76543210, vo->base + VO_DMA_ID_RD_0);

    writel(0xfedcba98, vo->base + 0x38);
    writel(0xfedcba98, vo->base + 0x40);
    writel(0xfedcba98, vo->base + 0x48);
    writel(0xfedcba98, vo->base + 0x80);

    for(i = 0; i < 10; i++)
    {
        writel(0x701, vo->base + VO_LAYER0_LINE0_BD_CTL + (4 * i));
    }

   
}

void kendryte_vo_table_init(struct kendryte_vo *vo)
{
    int i = 0;

    // init VSCALE
    for(i = 0; i < 64; i++)
    {
        writel(V_Coef[i*2],(vo->base + VO_VSCALE_BASE + ((i*2)<<2)));
    	writel(V_Coef[i*2+1],(vo->base + VO_VSCALE_BASE + ((i*2+1)<<2)));
    }

    // init HSCALE
    for(i = 0; i < 64; i++)
    {
        writel(H_Coef[i*3], (vo->base + VO_HSCALE_BASE + ((i*4)<<2)));
		writel(H_Coef[i*3 + 1],(vo->base + VO_HSCALE_BASE + ((i*4+1)<<2)));
		writel(H_Coef[i*3 +2 ],(vo->base + VO_HSCALE_BASE + ((i*4+2)<<2)));
    }
/*
    // init GAMMA
    for(i = 0; i < 255; i++)
    {
        writel(GammaCoef[i],(vo->base + VO_GAMMA_BASE + 0x00 +((i)<<2)));
		writel(GammaCoef[i],(vo->base + VO_GAMMA_BASE + 0x400 +((i)<<2)));
		writel(GammaCoef[i],(vo->base + VO_GAMMA_BASE + 0x800 +((i)<<2)));
    }
*/

}

void kendryte_vo_set_yuv_background(struct kendryte_vo *vo, uint16_t y, uint32_t u, uint32_t v)
{
    uint32_t reg = 0;

    reg = y + (u << 8) + (v << 16);

    writel(reg, vo->base + VO_DISP_BACKGROUND);
    
}


void kendryte_vo_software_reset(struct kendryte_vo *vo)
{
    writel(0x0f, vo->base + VO_SOFT_RST_CTL);
}


void kendryte_vo_timing_init(struct kendryte_vo *vo, struct kendryte_vo_timing *vo_timing)
{
    uint32_t reg = 0;
    // default VO_SYSTEM_MODE_1080x1920x30P
    printk("kendryte_vo_timing_init start");

    // write hsa = hsa_end - hsa_start
    reg = vo_timing->hsa_start + (vo_timing->hsa_end << 16);
    writel(reg, vo->base + VO_DISP_HSYNC_CTL);

    //write vsa = vas_end - vas_start ( + 1)
    reg = vo_timing->vas_start + (vo_timing->vas_end << 16);
    writel(reg, vo->base + VO_DISP_HSYNC1_CTL);

    // 通过第一个vas 开始读取hsa
    reg = 0x01 + (0x01 << 16);
    writel(reg, vo->base + VO_DISP_VSYNC1_CTL);

    // write vo totle size
    reg = vo_timing->total_size.VT + (vo_timing->total_size.HT << 16);
    writel(reg, vo->base + VO_DISP_TOTAL_SIZE);

    /*
        rite x zone  
        HBP = zone_x_start - hsa
        HFP = total_size.HT - act.h - hbp - hsa 
    */
    reg = vo_timing->zone_x_start + ((vo_timing->zone_x_stop - 1) << 16);
    writel(reg, vo->base + VO_DISP_XZONE_CTL);  

    /*
        write y zone
        VBP = zone_y_start - 1
        VFP = total_size.VT - act.v - vbp
    */
    reg = vo_timing->zone_y_start + ((vo_timing->zone_y_stop -1) << 16) ;
    writel(reg, vo->base + VO_DISP_YZONE_CTL);


    writel(0x105d08cc, vo->base + VO_DISP_HSYNC2_CTL);
    writel(0xfdf1a55, vo->base + VO_DISP_VSYNC2_CTL);

    // config default layer location

    writel(0x1fff0000, vo->base + VO_DISP_LAYER0_XCTL);
    writel(0x1ce01ce, vo->base + VO_DISP_LAYER0_YCTL);

    writel(0x1fff0000, vo->base + VO_DISP_LAYER1_XCTL);
    writel(0xe020701, vo->base + VO_DISP_LAYER1_YCTL);

    writel(0x1fff0000, vo->base + VO_DISP_LAYER2_XCTL);
    writel(0x2081088, vo->base + VO_DISP_LAYER2_YCTL);

    writel(0x1fff0000, vo->base + VO_DISP_LAYER3_XCTL);
    writel(0x10001, vo->base + VO_DISP_LAYER3_YCTL);

    writel(0x50004, vo->base + VO_DISP_LAYER4_XCTL);
    writel(0x80005, vo->base + VO_DISP_LAYER4_YCTL);

    writel(0x11000a, vo->base + VO_DISP_LAYER5_XCTL);
    writel(0x14000b, vo->base + VO_DISP_LAYER5_YCTL);

    writel(0x20001, vo->base + VO_DISP_LAYER6_XCTL);
    writel(0xe0007, vo->base + VO_DISP_LAYER6_YCTL);


}

// uv addr form 
void kendryte_set_layer_addr(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t y_addr0, uint32_t y_addr1, uint32_t uv_addr0, uint32_t uv_addr1)
{
    switch (layer)
    {
        case LAYER0 :
            writel(y_addr0, vo->base + VO_DISP_LAYER0_Y_ADDR0);
            writel(y_addr1, vo->base + VO_DISP_LAYER0_Y_ADDR1);

            writel(uv_addr0, vo->base + VO_DISP_LAYER0_UV_ADDR0);
            writel(uv_addr1, vo->base + VO_DISP_LAYER0_UV_ADDR1);

            break;
        case LAYER1 :
            writel(y_addr0, vo->base + VO_DISP_LAYER1_Y_ADDR0);
            writel(y_addr1, vo->base + VO_DISP_LAYER1_Y_ADDR1);

            writel(uv_addr0, vo->base + VO_DISP_LAYER1_UV_ADDR0);
            writel(uv_addr1, vo->base + VO_DISP_LAYER1_UV_ADDR1);

            break;
        case LAYER2 :
            writel(y_addr0, vo->base + VO_DISP_LAYER2_Y_ADDR0);
            writel(y_addr1, vo->base + VO_DISP_LAYER2_Y_ADDR1);

            writel(uv_addr0, vo->base + VO_DISP_LAYER2_UV_ADDR0);
            writel(uv_addr1, vo->base + VO_DISP_LAYER2_UV_ADDR1);

            break;
        case LAYER3 :
            writel(y_addr0, vo->base + VO_DISP_LAYER3_Y_ADDR0);
            writel(y_addr1, vo->base + VO_DISP_LAYER3_Y_ADDR1);

            writel(uv_addr0, vo->base + VO_DISP_LAYER3_UV_ADDR0);
            writel(uv_addr1, vo->base + VO_DISP_LAYER3_UV_ADDR1);

            break;
    
        default:
            printk("The number of layers does not exist is %d", layer);
            break;
    }    
}

// uv addr form 
void kendryte_set_osd_addr(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t alpha_addr0, uint32_t alpha_addr1, uint32_t value0, uint32_t value1)
{
    switch (layer)
    {
        case OSD0 :

            writel(alpha_addr0, vo->base + VO_DISP_OSD0_ALP_ADDR0);
            writel(alpha_addr1, vo->base + VO_DISP_OSD0_ALP_ADDR1);

            writel(value0, vo->base + VO_DISP_OSD0_VLU_ADDR0);
            writel(value1, vo->base + VO_DISP_OSD0_VLU_ADDR1);
            break;
        case OSD1 :
        
            writel(alpha_addr0, vo->base + VO_DISP_OSD1_ALP_ADDR0);
            writel(alpha_addr1, vo->base + VO_DISP_OSD1_ALP_ADDR1);

            writel(value0, vo->base + VO_DISP_OSD1_VLU_ADDR0);
            writel(value1, vo->base + VO_DISP_OSD1_VLU_ADDR1);
            break;
        
        case OSD2 :
        
            writel(alpha_addr0, vo->base + VO_DISP_OSD2_ALP_ADDR0);
            writel(alpha_addr1, vo->base + VO_DISP_OSD2_ALP_ADDR1);

            writel(value0, vo->base + VO_DISP_OSD2_VLU_ADDR0);
            writel(value1, vo->base + VO_DISP_OSD2_VLU_ADDR1);
            break;
    
        default:
            printk("The number of osd does not exist is %d", layer);
            break;
    }    
}


void kendryte_set_layer_enable(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t status)
{
    uint32_t reg = 0;
    uint32_t val = 0;
    switch (layer)
    {
        case LAYER0 :

            writel(status, vo->base + VO_LAYER0_CTL);
            
            reg = readl(vo->base + VO_DISP_ENABLE);
            val = (reg & ~(BIT_MASK(0))) | (status << 0);
            writel(val, vo->base + VO_DISP_ENABLE);


            break;

        case LAYER1 :

            reg = readl(vo->base + VO_LAYER1_CTL);
            val = (reg & ~(BIT_MASK(1))) | (status << 0);
            writel(val, vo->base + VO_LAYER1_CTL);

            reg = readl(vo->base + VO_DISP_ENABLE);
            val = (reg & ~(BIT_MASK(1))) | (status << 1);
            writel(val, vo->base + VO_DISP_ENABLE);
            break;
        
        case LAYER2 :
            reg = readl(vo->base + VO_LAYER2_CTL);
            val = (reg & ~(BIT_MASK(1))) | (status << 0);
            writel(val, vo->base + VO_LAYER2_CTL);

            reg = readl(vo->base + VO_DISP_ENABLE);
            val = (reg & ~(BIT_MASK(2))) | (status << 2);
            writel(val, vo->base + VO_DISP_ENABLE);
            break;
        
        case LAYER3 :
            reg = readl(vo->base + VO_LAYER3_CTL);
            val = (reg & ~(BIT_MASK(1))) | (status << 0);
            writel(val, vo->base + VO_LAYER3_CTL);

            reg = readl(vo->base + VO_DISP_ENABLE);
            val = (reg & ~(BIT_MASK(3))) | (status << 3);
            writel(val, vo->base + VO_DISP_ENABLE);
            break;

        case OSD0 :
            reg = readl(vo->base + VO_DISP_ENABLE);
            val = (reg & ~(BIT_MASK(4))) | (status << 4);
            writel(val, vo->base + VO_DISP_ENABLE);
            break;

        case OSD1 :
            reg = readl(vo->base + VO_DISP_ENABLE);
            val = (reg & ~(BIT_MASK(5))) | (status << 5);
            writel(val, vo->base + VO_DISP_ENABLE);
            break;

        case OSD2 :
            reg = readl(vo->base + VO_DISP_ENABLE);
            val = (reg & ~(BIT_MASK(6))) | (status << 6);
            writel(val, vo->base + VO_DISP_ENABLE);
            break;
        
        default:
            printk("The number of osd does not exist is %d", layer);
            break;
    }   

}

void kendryte_set_layer_postion(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t vpos, uint32_t hpos, uint32_t v_offset, uint32_t h_offset)
{
    uint32_t val ;

    switch(layer)
    {
        case LAYER0 :
            // set layer0 in size 
            val = ((vpos -1 ) << 16) + (hpos - 1);
            writel(val, vo->base + VO_DISP_LAYER0_IN_ACT_SIZE);
            // set layer0 out size
            writel(val, vo->base + VO_DISP_LAYER0_OUT_ACT_SIZE);

            // set layer0 x start and stop
            val = (h_offset + 0x2e) + ((hpos + (h_offset + 0x2e) - 1) << 16);
            writel(val, vo->base + VO_DISP_LAYER0_XCTL);
            // set layer0 y start and stop
            val = (v_offset + 0x0e) + ((vpos + (v_offset + 0x0e) ) << 16);
            writel(val, vo->base + VO_DISP_LAYER0_YCTL);
            break;

        case LAYER1 :
            // set layer1 size 
            val = ((vpos -1 ) << 16) + (hpos - 1);
            writel(val, vo->base + VO_DISP_LAYER1_ACT_SIZE);
            // set layer1 x start and stop
            val = (h_offset + 0x2e) + ((hpos + (h_offset + 0x2e) - 1) << 16);
            writel(val, vo->base + VO_DISP_LAYER1_XCTL);
            // set layer1 y start and stop
            val = (v_offset + 0x0e) + ((vpos + (v_offset + 0x0e) ) << 16);
            writel(val, vo->base + VO_DISP_LAYER1_YCTL);
            break;
        
        case LAYER2 :
            // set layer2 size 
            val = ((vpos -1 ) << 16) + (hpos - 1);
            writel(val, vo->base + VO_DISP_LAYER2_ACT_SIZE);
            // set layer2 x start and stop
            val = (h_offset + 0x2e) + ((hpos + (h_offset + 0x2e) - 1) << 16);
            writel(val, vo->base + VO_DISP_LAYER2_XCTL);
            // set layer2 y start and stop
            val = (v_offset + 0x0e) + ((vpos + (v_offset + 0x0e) ) << 16);
            writel(val, vo->base + VO_DISP_LAYER2_YCTL);

            break;
        
        case LAYER3 :
            // set layer3 size 
            val = ((vpos -1 ) << 16) + (hpos - 1);
            writel(val, vo->base + VO_DISP_LAYER3_ACT_SIZE);
            // set layer3 x start and stop
            val = (h_offset + 0x2e) + ((hpos + (h_offset + 0x2e) - 1) << 16);
            writel(val, vo->base + VO_DISP_LAYER3_XCTL);
            // set layer3 y start and stop
            val = (v_offset + 0x0e) + ((vpos + (v_offset + 0x0e) ) << 16);
            writel(val, vo->base + VO_DISP_LAYER3_YCTL);
            break;

        case OSD0 :
            // set osd0 size 
            val = (vpos << 16) + hpos;
            writel(val, vo->base + VO_DISP_OSD0_ACT_SIZE);
            // set osd0 x start and stop
            val = (h_offset + 0x2e) + ((hpos + (h_offset + 0x2e) - 1) << 16);
            writel(val, vo->base + VO_DISP_LAYER4_XCTL);
            // set osd0 y start and stop
            val = (v_offset + 0x0e) + ((vpos + (v_offset + 0x0e) ) << 16);
            writel(val, vo->base + VO_DISP_LAYER4_YCTL);
            break;
        case OSD1 :
            // set osd1 size 
            val = (vpos << 16) + hpos;
            writel(val, vo->base + VO_DISP_OSD1_ACT_SIZE);
            // set osd1 x start and stop
            val = (h_offset + 0x2e) + ((hpos + (h_offset + 0x2e) - 1) << 16);
            writel(val, vo->base + VO_DISP_LAYER5_XCTL);
            // set osd1 y start and stop
            val = (v_offset + 0x0e) + ((vpos + (v_offset + 0x0e) ) << 16);
            writel(val, vo->base + VO_DISP_LAYER5_YCTL);

            break;
        case OSD2 :
            // set osd2 size 
            val = (vpos << 16) + hpos;
            writel(val, vo->base + VO_DISP_OSD2_ACT_SIZE);
            // set osd2 x start and stop
            val = (h_offset + 0x2e) + ((hpos + (h_offset + 0x2e) - 1) << 16);
            writel(val, vo->base + VO_DISP_LAYER6_XCTL);
            // set osd2 y start and stop
            val = (v_offset + 0x0e) + ((vpos + (v_offset + 0x0e) ) << 16);
            writel(val, vo->base + VO_DISP_LAYER6_YCTL);
            break;
        default:
            printk("The number of osd does not exist is %d", layer);
            break;
    }
}

void kendryte_set_layer_stride(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t stride)
{
    uint32_t val;

    switch(layer)
    {
        case LAYER0 :
            writel(stride, vo->base + VO_LAYER0_IMG_IN_STRIDE);
            break;

        case LAYER1 :
            
            writel(stride, vo->base + VO_LAYER1_IMG_IN_STRIDE);
            break;
        
        case LAYER2 :

            writel(stride, vo->base + VO_LAYER2_IMG_IN_STRIDE);
            break;
        
        case LAYER3 :
            writel(stride, vo->base + VO_LAYER3_IMG_IN_STRIDE);
            break;

        case OSD0 :
            val = (stride << 16 ) + stride;
            writel(val, vo->base + VO_DISP_OSD0_STRIDE);
            break;
        case OSD1 :
            val = (stride << 16 ) + stride;
            writel(val, vo->base + VO_DISP_OSD1_STRIDE);
            break;
        case OSD2 :
            val = (stride << 16 ) + stride;
            writel(val, vo->base + VO_DISP_OSD2_STRIDE);
            break;  
        default:
            printk("The number of osd does not exist is %d", layer);
            break;
    }
}

void kendryte_set_layer_y_endian(struct kendryte_vo *vo, enum VO_LAYER layer, enum VO_VIDEOLAYER_Y_ENDIAN_E ENDIAN)
{
    uint32_t reg = 0;

    
    switch(layer)
    {
        case LAYER0 :
            reg = readl(vo->base + VO_LAYER0_IMG_IN_DAT_MODE);
            reg = (reg & ~(GENMASK(9, 8))) | (ENDIAN << 8);
            writel(reg, vo->base + VO_LAYER0_IMG_IN_DAT_MODE);
            break;

        case LAYER1 :
            reg = readl(vo->base + VO_LAYER1_CTL);
            reg = (reg & ~(2 << 12)) | (ENDIAN << 12);
            writel(reg, vo->base + VO_LAYER1_CTL);
            break;
        
        case LAYER2 :
            reg = readl(vo->base + VO_LAYER2_CTL);
            reg = (reg & ~(2 << 12)) | (ENDIAN << 12);
            writel(reg, vo->base + VO_LAYER2_CTL);

            break;
        
        case LAYER3 :
            reg = readl(vo->base + VO_LAYER3_CTL);
            reg = (reg & ~(2 << 12)) | (ENDIAN << 12);
            writel(reg, vo->base + VO_LAYER3_CTL);
            break;

        default:
            printk("The number of osd does not exist is %d", layer);
            break;
    }
}

void kendryte_set_layer_uv_endian(struct kendryte_vo *vo, enum VO_LAYER layer, enum VO_VIDEOLAYER_UV_ENDIAN_E ENDIAN)
{
    uint32_t reg = 0;

    
    switch(layer)
    {
        case LAYER0 :
            reg = readl(vo->base + VO_LAYER0_IMG_IN_DAT_MODE);
            reg = (reg & ~(GENMASK(14, 12))) | (ENDIAN << 12);
            writel(reg, vo->base + VO_LAYER0_IMG_IN_DAT_MODE);
            break;

        case LAYER1 :
            reg = readl(vo->base + VO_LAYER1_CTL);
            reg = (reg & ~(GENMASK(18, 16))) | (ENDIAN << 16);
            writel(reg, vo->base + VO_LAYER1_CTL);
            break;
        
        case LAYER2 :
            reg = readl(vo->base + VO_LAYER2_CTL);
            reg = (reg & ~(GENMASK(18, 16))) | (ENDIAN << 16);
            writel(reg, vo->base + VO_LAYER2_CTL);
            break;
        
        case LAYER3 :
            reg = readl(vo->base + VO_LAYER3_CTL);
            reg = (reg & ~(GENMASK(18, 16))) | (ENDIAN << 16);
            writel(reg, vo->base + VO_LAYER3_CTL);
            break;

        default:
            printk("The number of osd does not exist is %d", layer);
            break;
    }
}

/*
    mode 0 : Hw switch the pingpang address automatically
    mode 1 : w switch the pingpang address according to register

*/
void kendryte_vo_set_pingpang_mode(struct kendryte_vo *vo, enum VO_LAYER layer, enum VO_LAYER_PINGPANG_MODE mode)
{
    uint32_t reg = 0x1100;

    switch(layer)
    {
        case LAYER0 :
            writel(reg, vo->base + VO_LAYER0_PINGPANG_SEL_MODE);
            break;

        case LAYER1 :
            writel(reg, vo->base + VO_LAYER1_PINGPANG_SEL_MODE);
            break;
        
        case LAYER2 :
            writel(reg, vo->base + VO_LAYER2_PINGPANG_SEL_MODE);
            break;
        
        case LAYER3 :
            writel(reg, vo->base + VO_LAYER3_PINGPANG_SEL_MODE);
            break;
        
        case OSD0 :
            writel(reg, vo->base + VO_LAYER4_PINGPANG_SEL_MODE);
            break;

        case OSD1 :
            writel(reg, vo->base + VO_LAYER5_PINGPANG_SEL_MODE);
            break;
        
        case OSD2 :
            writel(reg, vo->base + VO_LAYER6_PINGPANG_SEL_MODE);
            break;

        default:
            printk("The number of osd does not exist is %d", layer);
            break;
    }
}

void kendryte_vo_set_layer_format(struct kendryte_vo *vo, enum VO_LAYER layer, enum VO_LAYER_FORMATS formats)
{
    uint32_t reg = 0;

    switch(layer)
    {
        case LAYER0 :
            reg = readl(vo->base + VO_LAYER0_IMG_IN_DAT_MODE);
            reg = (reg & ~(BIT_MASK(0))) | (0x8 << 0);                // 默认了420
            writel(reg, vo->base + VO_LAYER0_IMG_IN_DAT_MODE);
            break;

        case LAYER1 :           
            reg = readl(vo->base + VO_LAYER1_CTL);
            reg = (reg & ~(BIT_MASK(4))) | (0 << 4);                //close 422
            writel(reg, vo->base + VO_LAYER1_CTL);

            reg = readl(vo->base + VO_LAYER1_CTL);
            reg = (reg & ~(BIT_MASK(8))) | (1 << 8);                //xuyaogai 默认了420
            writel(reg, vo->base + VO_LAYER1_CTL);
            break;
        
        case LAYER2 :
            reg = readl(vo->base + VO_LAYER2_CTL);
            reg = (reg & ~(BIT_MASK(4))) | (0 << 4);                //close 422
            writel(reg, vo->base + VO_LAYER2_CTL);

            reg = readl(vo->base + VO_LAYER2_CTL);
            reg = (reg & ~(BIT_MASK(8))) | (1 << 8);                //xuyaogai 默认了420
            writel(reg, vo->base + VO_LAYER2_CTL);
            break;
        
        case LAYER3 :
            reg = readl(vo->base + VO_LAYER3_CTL);
            reg = (reg & ~(BIT_MASK(4))) | (0 << 4);                //close 422
            writel(reg, vo->base + VO_LAYER3_CTL);

            reg = readl(vo->base + VO_LAYER3_CTL);
            reg = (reg & ~(BIT_MASK(8))) | ((1 << 8));                //xuyaogai 默认了420
            writel(reg, vo->base + VO_LAYER3_CTL);
            break;
        
        case OSD0 :
            writel(0x3, vo->base + VO_DISP_OSD0_INFO);             //默认   RGB-32-bit rgb888
            break;

        case OSD1 :
            writel(0x3, vo->base + VO_DISP_OSD1_INFO);
            break;
        
        case OSD2 :
            writel(0x3, vo->base + VO_DISP_OSD2_INFO);
            break;

        default:
            printk("The number of osd does not exist is %d", layer);
            break;
    }
}

void kendryte_vo_blenth_init(struct kendryte_vo *vo)
{
    uint32_t reg = 0;
    writel(0x07, vo->base + VO_LAYER0_SCALE_BLENTH);
    writel(0x0f, vo->base + VO_LAYER1_IMG_IN_BLENTH);
    writel(0x0f, vo->base + VO_LAYER2_IMG_IN_BLENTH);
    writel(0x0f, vo->base + VO_LAYER3_IMG_IN_BLENTH);

    reg = (0 << 6) + (0 << 4) + 0xf;   //6:mem[OSD_ADDR] is R  045: original order  0-3:dma _blenth
    writel(0x4f, vo->base + VO_DISP_OSD0_DMA_CTRL);
    writel(0x4f, vo->base + VO_DISP_OSD1_DMA_CTRL);
    writel(0x4f, vo->base + VO_DISP_OSD2_DMA_CTRL);

}


// layer set 1 uv swap  ,  osd set 1 mem(argb)
void kendryte_layer_set_uv_swap(struct kendryte_vo *vo, enum VO_LAYER layer, int swap)
{
    uint32_t reg = 0;

    switch(layer)
    {
        case LAYER0 :
            reg = readl(vo->base + VO_LAYER0_IMG_IN_DAT_MODE);
            reg = (reg & ~(BIT_MASK(24))) | (swap << 24); 

            writel(reg, vo->base + VO_LAYER0_IMG_IN_DAT_MODE);
            break;

        case LAYER1 :
            reg = readl(vo->base + VO_LAYER1_CTL);
            reg = (reg & ~(BIT_MASK(28))) | (swap << 28); 

            writel(reg, vo->base + VO_LAYER1_CTL);
            break;
        
        case LAYER2 :
            reg = readl(vo->base + VO_LAYER2_CTL);
            reg = (reg & ~(BIT_MASK(28))) | (swap << 28); 

            writel(reg, vo->base + VO_LAYER2_CTL);
            break;
        
        case LAYER3 :
            reg = readl(vo->base + VO_LAYER3_CTL);
            reg = (reg & ~(BIT_MASK(28))) | (swap << 28); 

            writel(reg, vo->base + VO_LAYER3_CTL);
            break;

        case OSD0 :
            reg = readl(vo->base + VO_DISP_OSD0_DMA_CTRL);
            reg = (reg & ~(BIT_MASK(6))) | (swap << 6); 

            writel(reg, vo->base + VO_DISP_OSD0_DMA_CTRL);             //默认   RGB-32-bit rgb888
            break;

        case OSD1 :
            reg = readl(vo->base + VO_DISP_OSD1_DMA_CTRL);
            reg = (reg & ~(BIT_MASK(6))) | (swap << 6); 

            writel(reg, vo->base + VO_DISP_OSD1_DMA_CTRL);
            break;
        
        case OSD2 :
            reg = readl(vo->base + VO_DISP_OSD2_DMA_CTRL);
            reg = (reg & ~(BIT_MASK(6))) | (swap << 6); 

            writel(reg, vo->base + VO_DISP_OSD2_DMA_CTRL);
            break;

        default:
            printk("The number of layer does not exist is %d", layer);
            break;
    }
}


void kendryte_layer_set_in_offset(struct kendryte_vo *vo, enum VO_LAYER layer, int offset)
{
    uint32_t reg = 0;

    switch(layer)
    {
        case LAYER0 :

            reg = offset;

            writel(reg, vo->base + VO_LAYER0_IMG_IN_OFFSET);
            break;

        case LAYER1 :

            reg = offset;

            writel(reg, vo->base + VO_LAYER1_IMG_IN_OFFSET);
            break;
        
        case LAYER2 :

            reg = offset;

            writel(reg, vo->base + VO_LAYER2_IMG_IN_OFFSET);
            break;
        
        case LAYER3 :

            reg = offset;

            writel(reg, vo->base + VO_LAYER3_IMG_IN_OFFSET);
            break;


        default:
            printk("The number of layer does not exist is %d", layer);
            break;
    }
}

void kendryte_display_crtl(struct kendryte_vo *vo, int status)
{
    if(status == 0)
    {
        writel(0x0, vo->base + VO_DISP_ENABLE);
    }
    else
    {
        // open display,close all plan
        writel(0x80, vo->base + VO_DISP_ENABLE);
    }
}


void kendryte_vo_set_config_done(struct kendryte_vo *vo)
{
    writel(0x11, vo->base + VO_REG_LOAD_CTL);
}


void kendryte_vo_set_config_mix(struct kendryte_vo *vo)
{

    
    writel(0x7f, vo->base + ADDR_VO_DISP_MIX_LAYER_GLB_EN);
    writel(0xffffffff, vo->base + ADDR_VO_DISP_MIX_LAYER_GLB_ALPHA0);
    writel(0xffffffff, vo->base + ADDR_VO_DISP_MIX_LAYER_GLB_ALPHA1);
    writel(0x76543210, vo->base + ADDR_VO_DISP_MIX_SEL);

    // config csc 
    writel(0x1, vo->base + ADDR_VO_DISP_YUV2RGB_CTL);
    // disp gamma enable
    writel(0x0, vo->base + ADDR_VO_DISP_CLUT_CTL);
    // disp dith enable
    writel(0x1, vo->base + ADDR_VO_DISP_DITH_CTL);

    writel(0x10001, vo->base + VO_OSD_RGB2YUV_CTL);

    writel(0xff, vo->base + ADDR_VO_DISP_MIX_LAYER_GLB_EN);
    
    //close vo irq
    writel(0x0, vo->base + ADDR_VO_DISP_IRQ0_CTL);
    writel(0x0, vo->base + ADDR_VO_DISP_IRQ1_CTL);
    writel(0x0, vo->base + ADDR_VO_DISP_IRQ2_CTL);

    writel(0x80300, vo->base + VO_DISP_CTL);

    kendryte_display_crtl(vo, 0);              // open 0  close  1

    // ????
    writel(0x077F8437, vo->base + 0x780);

}


void kendryte_vo_clear_intr(struct kendryte_vo *vop)
{
    writel(0xffffffff, vop->base + ADDR_VO_DISP_IRQ_STATUS);
}


void kendryte_vo_set_irq(struct kendryte_vo *vop, int status)
{
     // k510 irq only use vsync irq
    uint32_t reg = 0;
   
    reg = status << 20;
    writel(reg, vop->base + ADDR_VO_DISP_IRQ0_CTL);
    
}

void kendryte_vo_get_reg_val(struct kendryte_vo *vop)
{
    int i = 0;

    for(i = 0; i < 0x4A0; i = i + 4)  //0x49c
    {
    	printk("VO REG:Reg[0x%x]= 0x%x\n", i, readl(vop->base + i));
    }

}

int kendryte_vo_draw_frame(struct kendryte_vo *vop, struct vo_draw_frame *frame)
{
    uint32_t reg = 0;


    U_VO_REMAP_CTRL_FILL_INFO_AREA_0  info0_udata;
    U_VO_REMAP_CTRL_FILL_INFO_AREA_1  info1_udata;
    U_VO_REMAP_CTRL_FILL_DATA_AREA_0  data_udata;


    if(frame->frame_num > 31 || frame->frame_num < 0 )
    {
        return -1;
    }


    // set flil info area 0 0           base = vo->base + 0x600 + (frame->frame_num * 4 * 2 )
    reg = ADDR_VO_DISP_DRAW_FRAME + (frame->frame_num * 4 * 2);
    info0_udata.u32 = readl(vop->base + reg);

    info0_udata.bits.line_draw_en     = frame->draw_en;
    info0_udata.bits.v_line_u_en      = 0;
    info0_udata.bits.h_line_b_en      = 0;
    info0_udata.bits.v_line_l_en      = 0;
    info0_udata.bits.v_line_r_en      = 0;
    info0_udata.bits.line_start_pos_x = frame->line_x_start;
    info0_udata.bits.line_end_pos_x   = frame->line_x_end;//frame->line_y_start;
    writel(info0_udata.u32, vop->base + reg);


    // set fill info area 0 1           base = vo->base + 0x600 + (frame->frame_num * 4 * 2 + 0x4)
    reg = ADDR_VO_DISP_DRAW_FRAME + (frame->frame_num * 4 * 2 + 0x4);
    info1_udata.u32 = readl(vop->base + reg);

    info1_udata.bits.line_start_pos_y = frame->line_y_start;//frame->line_x_end;
    info1_udata.bits.line_end_pos_y   = frame->line_y_end;
    info1_udata.bits.line_width_h     = 0;
    info1_udata.bits.line_width_l     = 5;
	writel(info1_udata.u32, vop->base + reg);

    // set fill data area 0 0           base = vo->base + 0x600 + 0x100
    reg = ADDR_VO_DISP_DRAW_FRAME + 0x100 + (frame->frame_num * 4);
    data_udata.u32 = readl(vop->base + reg);

    data_udata.bits.fill_value_y      = 47;
    data_udata.bits.fill_value_cb     = 104;
    data_udata.bits.fill_value_cr     = 226;
    data_udata.bits.fill_alpha        = 0;
    writel(data_udata.u32, vop->base + reg);

    return 0;
}


void kendryte_vo_set_osd_alpha_tpye(struct kendryte_vo *vo, enum VO_LAYER layer, int type)
{
    uint32_t reg = 0;

    switch(layer)
    {
        case OSD0 :
            reg = readl(vo->base + VO_DISP_OSD0_INFO);
            reg = (reg & ~(GENMASK(6, 4))) | (type << 4);

            writel(reg, vo->base + VO_DISP_OSD0_INFO);             //默认   RGB-32-bit rgb888
            break;

        case OSD1 :
            reg = readl(vo->base + VO_DISP_OSD1_INFO);
            reg = (reg & ~(GENMASK(6, 4))) | (type << 4);

            writel(reg, vo->base + VO_DISP_OSD1_INFO);
            break;
        
        case OSD2 :
            reg = readl(vo->base + VO_DISP_OSD2_INFO);
            reg = (reg & ~(GENMASK(6, 4))) | (type << 4);

            writel(reg, vo->base + VO_DISP_OSD2_INFO);
            break;

        default:
            printk("The type of layer does not exist is %d", layer);
            break;
    }

  
}

int kendryte_vo_osd_set_rgb2yuv_enable(struct kendryte_vo *vo, enum VO_LAYER layer)
{
    uint32_t reg = 0;

    switch(layer)
    {
        case OSD0 :
            reg = readl(vo->base + VO_OSD_RGB2YUV_CTL);
            reg = (reg & ~(BIT_MASK(0))) | (1 << 0); 
            writel(reg, vo->base + VO_OSD_RGB2YUV_CTL);
            break;
        
        case OSD1:
            reg = readl(vo->base + VO_OSD_RGB2YUV_CTL);
            reg = (reg & ~(BIT_MASK(8))) | (1 << 8); 
            writel(reg, vo->base + VO_OSD_RGB2YUV_CTL);
            break;

        case OSD2:
            reg = readl(vo->base + VO_OSD_RGB2YUV_CTL);
            reg = (reg & ~(BIT_MASK(16))) | (1 << 16); 
            writel(reg, vo->base + VO_OSD_RGB2YUV_CTL);
            break;
        
        default:
            break;
    }
}

int kendryte_vo_osd_set_format(struct kendryte_vo *vo, enum VO_LAYER layer, int type)
{
    uint32_t reg = 0;

    switch(layer)
    {
        case OSD0 :
            reg = readl(vo->base + VO_DISP_OSD0_INFO);
            switch(type)
            {
                case VOP_FMT_ARGB8888:
                    reg = (reg & ~(GENMASK(6, 0))) | (0x53 << 0);
                    writel(reg, vo->base + VO_DISP_OSD0_INFO);             //默认   RGB-32-bit argb888

                    break;

                case VOP_FMT_RGB888:
                    reg = (reg & ~(GENMASK(3, 0))) | (0 << 0);
                    writel(reg, vo->base + VO_DISP_OSD0_INFO);             //默认   RGB-24-bit rgb888
                    break;

                case VOP_FMT_RGB565:
                    reg = (reg & ~(GENMASK(3, 0))) | (2 << 0);
                    writel(reg, vo->base + VO_DISP_OSD0_INFO);             //默认   RGB-16-bit rgb565
                    break;

                case VOP_FMT_ARGB4444:
                    reg = (reg & ~(GENMASK(3, 0))) | (4 << 0);
                    writel(reg, vo->base + VO_DISP_OSD0_INFO);             //默认   RGB-24-bit rgb4444
                    break;
                    
                case VOP_FMT_ARGB1555:
                    reg = (reg & ~(GENMASK(3, 0))) | (5 << 0);
                    writel(reg, vo->base + VO_DISP_OSD0_INFO);             //默认   RGB-25-bit rgb1555
                    break;

                default :
                    break;
            }
            
            break;

        case OSD1 :
            reg = readl(vo->base + VO_DISP_OSD1_INFO);
            switch(type)
            {
                case VOP_FMT_ARGB8888:
                    reg = (reg & ~(GENMASK(6, 0))) | (0x53 << 0);
                    writel(reg, vo->base + VO_DISP_OSD1_INFO);             //默认   RGB-32-bit argb888

                    break;

                case VOP_FMT_RGB888:
                    reg = (reg & ~(GENMASK(3, 0))) | (0 << 0);
                    writel(reg, vo->base + VO_DISP_OSD1_INFO);             //默认   RGB-24-bit rgb888
                    break;
                    
                case VOP_FMT_RGB565:
                    reg = (reg & ~(GENMASK(3, 0))) | (2 << 0);
                    writel(reg, vo->base + VO_DISP_OSD1_INFO);             //默认   RGB-16-bit rgb565
                    break;

                case VOP_FMT_ARGB4444:
                    reg = (reg & ~(GENMASK(3, 0))) | (4 << 0);
                    writel(reg, vo->base + VO_DISP_OSD1_INFO);             //默认   RGB-24-bit rgb4444
                    break;

                case VOP_FMT_ARGB1555:
                    reg = (reg & ~(GENMASK(3, 0))) | (5 << 0);
                    writel(reg, vo->base + VO_DISP_OSD1_INFO);             //默认   RGB-25-bit rgb1555
                    break;

                default :
                    break;
            }
            break;
        
        case OSD2 :
            reg = readl(vo->base + VO_DISP_OSD2_INFO);
            switch(type)
            {
                case VOP_FMT_ARGB8888:
                    reg = (reg & ~(GENMASK(6, 0))) | (0x53 << 0);
                    writel(reg, vo->base + VO_DISP_OSD2_INFO);             //默认   RGB-32-bit argb888
                    break;

                case VOP_FMT_RGB888:
                    reg = (reg & ~(GENMASK(3, 0))) | (0 << 0);
                    writel(reg, vo->base + VO_DISP_OSD2_INFO);             //默认   RGB-24-bit rgb888
                    break;

                case VOP_FMT_RGB565:
                    reg = (reg & ~(GENMASK(3, 0))) | (2 << 0);
                    writel(reg, vo->base + VO_DISP_OSD2_INFO);             //默认   RGB-16-bit rgb565
                    break;

                case VOP_FMT_ARGB4444:
                    reg = (reg & ~(GENMASK(3, 0))) | (4 << 0);
                    writel(reg, vo->base + VO_DISP_OSD2_INFO);             //默认   RGB-24-bit rgb4444
                    break;

                case VOP_FMT_ARGB1555:
                    reg = (reg & ~(GENMASK(3, 0))) | (5 << 0);
                    writel(reg, vo->base + VO_DISP_OSD2_INFO);             //默认   RGB-25-bit rgb1555
                    break;

                default :
                    break;
            }
            break;

        default:
            printk("The type of layer does not exist is %d", layer);
            break;
    }
}
