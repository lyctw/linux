/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _KENDRYTE_VO_H_
#define _KENDRYTE_VO_H_

#include "kendryte_layer.h"
#include <drm/drm_flip_work.h>


#define VO_SOFT_RST_CTL                                             0x00
#define VO_REG_LOAD_CTL                                             0x04
#define VO_DMA_SW_CTL                                               0x08
#define VO_DMA_RD_CTL_OUT                                           0x0c
#define VO_DMA_ARB_MODE                                             0x10
#define VO_DMA_WEIGHT_RD0                                           0x24
#define VO_DMA_WEIGHT_RD1                                           0x28
#define VO_DMA_WEIGHT_RD2                                           0x2c
#define VO_DMA_WEIGHT_RD3                                           0x30

#define VO_DMA_PRIORITY_RD_1                                        0x40
#define VO_DMA_ID_RD_0                                              0x4c
#define VO_DMA_ID_RD_1                                              0x50

#define VO_CONFIG_LINE_BASE                                         0x0600
#define VO_HSCALE_BASE                                              0x8000
#define VO_VSCALE_BASE                                              0x8C00
#define VO_GAMMA_BASE                                               0xC000
  

#define VO_LAYER0_LINE0_BD_CTL                                      0x80
#define VO_LAYER0_LINE1_BD_CTL                                      0x84
#define VO_LAYER0_LINE2_BD_CTL                                      0x88
#define VO_LAYER0_LINE3_BD_CTL                                      0x8C
#define VO_LAYER1_BD_CTL                                            0x90
#define VO_LAYER2_BD_CTL                                            0x94
#define VO_LAYER3_BD_CTL                                            0x98
#define VO_LAYER4_BD_CTL                                            0x9c
#define VO_LAYER5_BD_CTL                                            0xa0
#define VO_LAYER6_BD_CTL                                            0xa4

//TIMING 
#define VO_DISP_HSYNC_CTL                                           0x100
#define VO_DISP_HSYNC1_CTL                                          0x104
#define VO_DISP_VSYNC1_CTL                                          0x108
#define VO_DISP_VSYNC2_CTL                                          0x110
#define VO_DISP_HSYNC2_CTL                                          0x10c
#define VO_DISP_TOTAL_SIZE                                          0x11C
#define VO_DISP_XZONE_CTL                                           0x0C0
#define VO_DISP_YZONE_CTL                                           0x0C4

// OFFSET + ACTIVE
#define VO_DISP_LAYER0_XCTL                                         0x0C8
#define VO_DISP_LAYER0_YCTL                                         0x0CC
#define VO_DISP_LAYER1_XCTL                                         0x0D0
#define VO_DISP_LAYER1_YCTL                                         0x0D4
#define VO_DISP_LAYER2_XCTL                                         0x0D8
#define VO_DISP_LAYER2_YCTL                                         0x0DC
#define VO_DISP_LAYER3_XCTL                                         0x0E0
#define VO_DISP_LAYER3_YCTL                                         0x0E4
#define VO_DISP_LAYER4_XCTL                                         0x0E8
#define VO_DISP_LAYER4_YCTL                                         0x0EC
#define VO_DISP_LAYER5_XCTL                                         0x0F0
#define VO_DISP_LAYER5_YCTL                                         0x0F4
#define VO_DISP_LAYER6_XCTL                                         0x0F8
#define VO_DISP_LAYER6_YCTL                                         0x0FC

// offset
#define VO_LAYER0_IMG_IN_OFFSET                                     0x16C
#define VO_LAYER1_IMG_IN_OFFSET                                     0x1D4
#define VO_LAYER2_IMG_IN_OFFSET                                     0x214
#define VO_LAYER3_IMG_IN_OFFSET                                     0x254

// VO ACTIVE
#define VO_DISP_OSD0_ACT_SIZE                                       0x284
#define VO_DISP_OSD1_ACT_SIZE                                       0x2C4
#define VO_DISP_OSD2_ACT_SIZE                                       0x304
#define VO_DISP_LAYER0_IN_ACT_SIZE                                  0x15C
#define VO_DISP_LAYER0_OUT_ACT_SIZE                                 0x160
#define VO_DISP_LAYER1_ACT_SIZE                                     0x1E0
#define VO_DISP_LAYER2_ACT_SIZE                                     0x220
#define VO_DISP_LAYER3_ACT_SIZE                                     0x260

//ADDR
#define VO_DISP_LAYER0_Y_ADDR0                                    0x14C
#define VO_DISP_LAYER0_Y_ADDR1                                    0x154
#define VO_DISP_LAYER0_UV_ADDR0                                   0x150
#define VO_DISP_LAYER0_UV_ADDR1                                   0x158

#define VO_DISP_LAYER1_Y_ADDR0                                    0x1C4
#define VO_DISP_LAYER1_Y_ADDR1                                    0x1CC
#define VO_DISP_LAYER1_UV_ADDR0                                   0x1C8
#define VO_DISP_LAYER1_UV_ADDR1                                   0x1D0

#define VO_DISP_LAYER2_Y_ADDR0                                    0x204
#define VO_DISP_LAYER2_Y_ADDR1                                    0x20c 
#define VO_DISP_LAYER2_UV_ADDR0                                   0x208  
#define VO_DISP_LAYER2_UV_ADDR1                                   0x210

#define VO_DISP_LAYER3_Y_ADDR0                                    0x244
#define VO_DISP_LAYER3_Y_ADDR1                                    0x24c
#define VO_DISP_LAYER3_UV_ADDR0                                   0x248
#define VO_DISP_LAYER3_UV_ADDR1                                   0x250

#define VO_DISP_OSD0_VLU_ADDR0                                    0x288
#define VO_DISP_OSD0_ALP_ADDR0                                    0x28C
#define VO_DISP_OSD0_VLU_ADDR1                                    0x290
#define VO_DISP_OSD0_ALP_ADDR1                                    0x294

#define VO_DISP_OSD1_VLU_ADDR0                                    0x2C8
#define VO_DISP_OSD1_ALP_ADDR0                                    0x2CC
#define VO_DISP_OSD1_VLU_ADDR1                                    0x2D0
#define VO_DISP_OSD1_ALP_ADDR1                                    0x2D4

#define VO_DISP_OSD2_VLU_ADDR0                                    0x308
#define VO_DISP_OSD2_ALP_ADDR0                                    0x30C
#define VO_DISP_OSD2_VLU_ADDR1                                    0x310
#define VO_DISP_OSD2_ALP_ADDR1                                    0x314

//VO LAYER CRTL
#define VO_LAYER0_CTL                                             0x140
#define VO_LAYER0_IMG_IN_DAT_MODE                                 0x148            
#define VO_LAYER1_CTL                                             0x1C0
#define VO_LAYER2_CTL                                             0x200
#define VO_LAYER3_CTL                                             0x240
#define VO_DISP_ENABLE                                            0x118
#define VO_DISP_CTL                                               0x114
#define VO_DISP_OSD0_INFO                                         0x280
#define VO_DISP_OSD1_INFO                                         0x2C0
#define VO_DISP_OSD2_INFO                                         0x300

//VO LAYER STRIDE
#define VO_LAYER0_IMG_IN_STRIDE                                   0x164
#define VO_LAYER1_IMG_IN_STRIDE                                   0x1DC
#define VO_LAYER2_IMG_IN_STRIDE                                   0x21C
#define VO_LAYER3_IMG_IN_STRIDE                                   0x25C
#define VO_DISP_OSD0_STRIDE                                       0x29C
#define VO_DISP_OSD1_STRIDE                                       0x2DC
#define VO_DISP_OSD2_STRIDE                                       0x31C

//dma burst length
#define VO_LAYER0_SCALE_BLENTH                                    0x144
#define VO_LAYER1_IMG_IN_BLENTH                                   0x1D8
#define VO_LAYER2_IMG_IN_BLENTH                                   0x21C
#define VO_LAYER3_IMG_IN_BLENTH                                   0x25C
#define VO_DISP_OSD0_DMA_CTRL                                     0x298
#define VO_DISP_OSD1_DMA_CTRL                                     0x2D8
#define VO_DISP_OSD2_DMA_CTRL                                     0x318

//VO PINPANG MODE SELSET
#define VO_LAYER0_PINGPANG_SEL_MODE                               0x184
#define VO_LAYER1_PINGPANG_SEL_MODE                               0x1E4
#define VO_LAYER2_PINGPANG_SEL_MODE                               0x224
#define VO_LAYER3_PINGPANG_SEL_MODE                               0x264
#define VO_LAYER4_PINGPANG_SEL_MODE                               0x2A0
#define VO_LAYER5_PINGPANG_SEL_MODE                               0x2E0
#define VO_LAYER6_PINGPANG_SEL_MODE                               0x320

#define VO_DISP_BACKGROUND                                        0x3D0
#define ADDR_VO_DISP_MIX_LAYER_GLB_EN                             0x3c0
#define ADDR_VO_DISP_MIX_LAYER_GLB_ALPHA0                         0x3c4
#define ADDR_VO_DISP_MIX_LAYER_GLB_ALPHA1                         0x3c8
#define ADDR_VO_DISP_MIX_SEL                                      0x3cc
          
#define ADDR_VO_DISP_YUV2RGB_CTL                                  0x380
#define ADDR_VO_DISP_CLUT_CTL                                     0x3d8
#define ADDR_VO_DISP_DITH_CTL                                     0x3d4
#define VO_OSD_RGB2YUV_CTL                                        0x340

#define ADDR_VO_DISP_IRQ0_CTL                                     0x3e0
#define ADDR_VO_DISP_IRQ1_CTL                                     0x3e4
#define ADDR_VO_DISP_IRQ2_CTL                                     0x3e8
#define ADDR_VO_DISP_IRQ_STATUS                                   0x3ec

#define ADDR_VO_DISP_DRAW_FRAME                                   0x600

#define KENDRYTE_VO_NUM_LAYERS		        7 


enum VO_LAYER {
    LAYER0 = 0x01,
    LAYER1 = 0x02,
    LAYER2 = 0x03,
    LAYER3 = 0x04,
    OSD0   = 0x05,
    OSD1   = 0x06,
    OSD2   = 0x07,
    BACkGROUD = 0X08
};

enum _VO_VIDEOLAYER_YUV_MODE_E
{
    VO_VIDEO_LAYER_YUV_MODE_420              = 0x08,
    VO_VIDEO_LAYER_YUV_MODE_422              = 0x09,

} ;

enum VO_VIDEOLAYER_Y_ENDIAN_E
{
    VO_VIDEO_LAYER_Y_ENDIAN_MODE0              = 0x0,//Y4 Y5 Y6 Y7 Y0 Y1 Y2 Y3
    VO_VIDEO_LAYER_Y_ENDIAN_MODE1              = 0x1,//Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
    VO_VIDEO_LAYER_Y_ENDIAN_MODE2              = 0x2,//Y0 Y1 Y2 Y3 Y4 Y5 Y6 Y7
    VO_VIDEO_LAYER_Y_ENDIAN_MODE3              = 0x3,//Y3 Y2 Y1 Y0 Y7 Y6 Y5 Y4
} ;


enum VO_VIDEOLAYER_UV_ENDIAN_E
{
    VO_VIDEO_LAYER_UV_ENDIAN_MODE0              = 0x0,//U2 V2 U3 V3 U0 V0 U1 V1
    VO_VIDEO_LAYER_UV_ENDIAN_MODE1              = 0x1,//V2 U2 V3 U3 V0 U0 V1 U1
    VO_VIDEO_LAYER_UV_ENDIAN_MODE2              = 0x2,//U3 V3 U2 V2 U1 V1 U0 V0
    VO_VIDEO_LAYER_UV_ENDIAN_MODE3              = 0x3,//V3 U3 V2 U2 V1 U1 V0 U0  
    VO_VIDEO_LAYER_UV_ENDIAN_MODE4              = 0x4,//U0 V0 U1 V1 U2 V2 U3 V3  
    VO_VIDEO_LAYER_UV_ENDIAN_MODE5              = 0x5,//V0 U0 V1 U1 V2 U2 V3 U3 
    VO_VIDEO_LAYER_UV_ENDIAN_MODE6              = 0x6,//U1 V1 U0 V0 U3 V3 U2 V2
    VO_VIDEO_LAYER_UV_ENDIAN_MODE7              = 0x7//V1 U1 V0 U0 V3 U3 V2 U2 

};

enum VO_LAYER_PINGPANG_MODE
{
    VO_LAYER_PINGPANG_AUTO = 0x01,
    VO_LAYER_PING,
    VO_LAYER_PANG,
    VO_LAYER_PINGPANG,
};

enum VO_LAYER_FORMATS {

	VO_FORMAT_YUV422 = 0X01,
	VO_FORMAT_YUV420,

	VO_FORMAT_ARGB1555,             //ARGB A 1BIT 
    VO_FORMAT_RGB888,
    VO_FORMAT_RGB565,
	VO_FORMAT_RGBA5551,
	VO_FORMAT_RGBA4444,	
    
	VO_FORMAT_UYVY,
	VO_FORMAT_VYUY,
	VO_FORMAT_XRGB8888,
	VO_FORMAT_YUYV,
	VO_FORMAT_YVYU,
};

enum vop_pending {
	VOP_PENDING_FB_UNREF,
};


struct vo_act_size {
    int  HA;
    int  VA;
};

struct vo_offset_size {
    int  h_offset;
    int  v_offset;
};

struct vo_total_size {
    int  HT;
    int  VT;
};
    
struct kendryte_vo_timing {

    // hsa
    int  hsa_start; 
    int  hsa_end;
    
    int  zone_x_start;
    int  zone_x_stop;


    //vsa
    int  vas_start;
    int  vas_end;

    int  zone_y_start;
    int  zone_y_stop;

    struct vo_total_size total_size;           //vo 的总共size
    struct vo_act_size act_size;               // vo 输出的 有效size（不是每层的act）,也就是屏幕的有效size

};

struct kendryte_layer {
    struct drm_plane        plane;
    enum drm_plane_type     type;
    struct kendryte_drv     *drv;
    int status;
    int id;
    int layer_id;
	int area_id;
	int zpos;
	uint32_t offset;
    uint16_t stride;
    struct kendryte_layer_state state;
    struct kendryte_vo      *vo;

};



typedef union { /* ctrl_fill_info_area_0_0 */

    struct 
    {
        unsigned int line_end_pos_x                    : 13;
        unsigned int line_draw_en                      : 1;
        unsigned int v_line_r_en                       : 1;
        unsigned int v_line_l_en                       : 1;
        unsigned int line_start_pos_x                  : 13;
        unsigned int h_line_b_en                       : 1;
        unsigned int v_line_u_en                       : 1;
        unsigned int reserved0                         : 1;
    } bits;    
    unsigned int u32; 
} U_VO_REMAP_CTRL_FILL_INFO_AREA_0;

typedef union { /* ctrl_fill_info_area_0_1 */

    struct 
    {
        unsigned int line_end_pos_y                    : 13;
        unsigned int line_width_l                      : 3;
        unsigned int line_start_pos_y                  : 13;
        unsigned int line_width_h                      : 3;
    } bits;    
    unsigned int u32; 
} U_VO_REMAP_CTRL_FILL_INFO_AREA_1;

typedef union { /* ctrl_fill_data_area_0_0 */

    struct 
    {
        unsigned int fill_value_cr                     : 8;
        unsigned int fill_value_cb                     : 8;
        unsigned int fill_value_y                      : 8;
        unsigned int fill_alpha                        : 8;
    } bits;    
    unsigned int u32; 
} U_VO_REMAP_CTRL_FILL_DATA_AREA_0;


struct vo_draw_frame {
    uint32_t draw_en;
    uint32_t line_x_start;
    uint32_t line_y_start;

    uint32_t line_x_end;
    uint32_t line_y_end;

    uint32_t frame_num;

    uint32_t crtc_id;
};

#define DRM_KENDRYTE_DRAW_FRAME         0x00
#define DRM_KENDRYTE_RESET              0x01

#define DRM_IOCTL_KENDRYTE_DRAW_FRAME   DRM_IOWR(DRM_COMMAND_BASE + \
                DRM_KENDRYTE_DRAW_FRAME, struct vo_draw_frame)
#define DRM_IOCTL_KENDRYTE_RESET   DRM_IOWR(DRM_COMMAND_BASE + \
                DRM_KENDRYTE_RESET, unsigned int)

struct kendryte_vo {

    struct drm_crtc                  crtc;
    struct device                   *dev;
    struct drm_device               *drm;
    
    // vo clk
    struct clk                      *vo_apb;
    struct clk                      *vo_axi;
    // vo base addr
    void __iomem  *base;

    /* lock vop irq reg */
	spinlock_t irq_lock;

    /* protected by dev->event_lock */
	struct drm_pending_vblank_event *event;

    struct drm_flip_work fb_unref_work;

	unsigned long pending;

    /* Reset control */
    struct reset_control *reset;

    // interrupt irq id
    int irq_vo;

    /* Platform adjustments */
    int     ch_num;     ;
    unsigned int layer_size;

    /* Associated crtc */
    int                             id;
    struct kendryte_vo_timing       *vo_timing;
    
    struct vo_act_size  act_size[KENDRYTE_VO_NUM_LAYERS];              //layer0 - layer6
    struct vo_offset_size offset_size[KENDRYTE_VO_NUM_LAYERS];         //layer0 - layer6

    struct kendryte_layer layer[];
};


static inline struct kendryte_vo *to_vop(struct drm_crtc *crtc)
{
    return container_of(crtc, struct kendryte_vo, crtc);
}

static inline struct kendryte_layer *to_vop_layer(struct drm_plane *plane)
{
    return container_of(plane, struct kendryte_layer, plane);
}


//#define to_vop(x) container_of(x, struct kendryte_vo, crtc)
//#define to_vop_layer(x) container_of(x, struct kendryte_layer, plane)

/*

struct vo_hw_ctx {
	void __iomem  *base;
	struct regmap *noc_regmap;
	struct clk *vo_core_clk;
	struct clk *media_noc_clk;
	struct clk *vo_pix_clk;
	struct reset_control *reset;
	bool power_on;
	int irq;
};

*/

void kendryte_vo_enable_vblank(struct kendryte_vo *vo, bool enable);
void kendryte_vo_wrap_init(struct kendryte_vo *vo);
void kendryte_vo_table_init(struct kendryte_vo *vo);
void kendryte_vo_set_yuv_background(struct kendryte_vo *vo, uint16_t y, uint32_t u, uint32_t v);
//void kendryte_vo_set_rgb_background(struct kendryte_vo *vo, uint16_t R, uint32_t G, uint32_t B);
void kendryte_vo_software_reset(struct kendryte_vo *vo);
void kendryte_vo_timing_init(struct kendryte_vo *vo, struct kendryte_vo_timing *vo_timing);
void kendryte_set_layer_addr(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t y_addr0, uint32_t y_addr1, uint32_t uv_addr0, uint32_t uv_addr1);
void kendryte_set_osd_addr(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t alpha_addr0, uint32_t alpha_addr1, uint32_t value0, uint32_t value1);
void kendryte_set_layer_enable(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t status);
//void kendryte_set_layer_act_size(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t vact, uint32_t hact);
//void kendryte_set_layer_offset_position(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t vpos, uint32_t hpos);
void kendryte_set_layer_y_endian(struct kendryte_vo *vo, enum VO_LAYER layer, enum VO_VIDEOLAYER_Y_ENDIAN_E ENDIAN);
void kendryte_set_layer_uv_endian(struct kendryte_vo *vo, enum VO_LAYER layer, enum VO_VIDEOLAYER_UV_ENDIAN_E ENDIAN);
void kendryte_vo_set_pingpang_mode(struct kendryte_vo *vo, enum VO_LAYER layer, enum VO_LAYER_PINGPANG_MODE);
void kendryte_vo_set_layer_format(struct kendryte_vo *vo, enum VO_LAYER layer, enum VO_LAYER_FORMATS formats);
void kendryte_vo_blenth_init(struct kendryte_vo *vo);
void kendryte_display_crtl(struct kendryte_vo *vo, int status);
void kendryte_vo_set_config_done(struct kendryte_vo *vo);
void kendryte_vo_set_config_mix(struct kendryte_vo *vo);
void kendryte_set_layer_postion(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t vpos, uint32_t hpos, uint32_t v_offset, uint32_t h_offset);
void kendryte_set_layer_stride(struct kendryte_vo *vo, enum VO_LAYER layer, uint32_t stride);
void kendryte_vo_get_reg_val(struct kendryte_vo *vop);
void kendryte_vo_clear_intr(struct kendryte_vo *vop);
void kendryte_vo_set_irq(struct kendryte_vo *vop, int status);
void kendryte_layer_set_uv_swap(struct kendryte_vo *vo, enum VO_LAYER layer, int swap);
void kendryte_layer_set_in_offset(struct kendryte_vo *vo, enum VO_LAYER layer, int offset);

int kendryte_vo_draw_frame(struct kendryte_vo *vop, struct vo_draw_frame *frame);
void kendryte_vo_set_osd_alpha_tpye(struct kendryte_vo *vop, enum VO_LAYER layer, int type);
int kendryte_vo_osd_set_format(struct kendryte_vo *vo, enum VO_LAYER layer, int type);
int kendryte_vo_osd_set_rgb2yuv_enable(struct kendryte_vo *vo, enum VO_LAYER layer);
#endif
