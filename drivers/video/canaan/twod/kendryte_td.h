/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __KENDRYTE_TD_H__
#define __KENDRYTE_TD_H__

#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <asm/ioctl.h>



// warp 
#define  TD_WRAP_INT_ST                            (0x00 + 0x7c)
#define  TD_WRAP_CONFIG_DONE                       (0x00 + 0x78)

// rotation reg 
#define  TD_ROT_CMD                                (0x00 + 0x80)
#define  TD_ROT_CTRL_MODE                          (0x04 + 0x80)
#define  TD_ROT_TIMEOUT                            (0x08 + 0x80)                            
#define  TD_ROT_IMAHE_SIZE_ACTIVE                  (0x0c + 0x80)                           
#define  TD_ROT_FRM_BASE_ADDR_Y_SRC                (0x10 + 0x80)                     
#define  TD_ROT_FRM_BASE_ADDR_U_SRC                (0x14 + 0x80)                            
#define  TD_ROT_FRM_BASE_ADDR_V_SRC                (0x18 + 0x80)                            
#define  TD_ROT_FRM_BASE_ADDR_Y_DES                (0x1c + 0x80)                            
#define  TD_ROT_FRM_BASE_ADDR_U_DES                (0x20 + 0x80)                            
#define  TD_ROT_FRM_BASE_ADDR_V_DES                (0x24 + 0x80)           
#define  TD_ROT_STRIDE_Y                           (0x28 + 0x80)                            
#define  TD_ROT_STRIDE_U                           (0x2c + 0x80)                            
#define  TD_ROT_STRIDE_V                           (0x30 + 0x80)                            
#define  TD_ROT_RDATA_FUNC                         (0x40 + 0x80)            

// scaler reg 
#define  TD_SCALER_SOFT_RESET                      (0x00 + 0x800)
#define  TD_SCALER_AXI_SW_CTL                      (0x08 + 0x800)
#define  TD_SCALER_LAYER0_LINE0_BD_CTL             (0x80 + 0x800)
#define  TD_SCALER_LAYER0_LINE1_BD_CTL             (0x84 + 0x800)
#define  TD_SCALER_LAYER0_LINE2_BD_CTL             (0x88 + 0x800)
#define  TD_SCALER_LAYER0_LINE3_BD_CTL             (0x8C + 0x800)
#define  TD_SCALER_LAYER4_BD_CTL                   (0x9C + 0x800)            
#define  TD_SCALER_LAYER0_BADDR0_Y                 (0x14c+ 0x800)   
#define  TD_SCALER_LAYER0_BADDR0_UV                (0x150+ 0x800)  
#define  TD_SCALER_LAYER0_IMG_IN_STRIDE            (0x164+ 0x800) 
#define  TD_SCALER_LAYER0_IN_PIX_SIZE              (0x15C+ 0x800) 
#define  TD_SCALER_LAYER0_OUT_PIX_SIZE             (0x160+ 0x800) 
#define  TD_SCALER_LAYER0_CRTL                     (0x140+ 0x800) 
#define  TD_SCALER_LAYER0_BLENTH                   (0x144+ 0x800) 
#define  TD_SCALER_LAYER0_IMG_IN_DAT_MODE          (0x148+ 0x800) 
#define  TD_SCALER_LAYER0_IMG_IN_OFFSET            (0x16c+ 0x800) 
#define  TD_SCALER_LAYER0_XCTL                     (0x0c8+ 0x800) 
#define  TD_SCALER_LAYER0_YCTL                     (0x0cc+ 0x800) 
#define  TD_SCALER_XZONE_CTL                       (0x0c0+ 0x800)
#define  TD_SCALER_YZONE_CTL                       (0x0c4+ 0x800)
#define  TD_SCALER_MIX_SEL                         (0x3cc+ 0x800) 
#define  TD_SCALER_LAYER0_GLB_ALPHA0               (0x3c4+ 0x800)  
#define  TD_SCALER_OSD_GLB_ALPHA1                  (0x3c8+ 0x800)  
#define  TD_SCALER_DISP_CTL                        (0x114+ 0x800)
#define  TD_SCALER_DISP_ENABLE                     (0x118+ 0x800)
#define  TD_SCALER_LAYER0_VSCALE_STEP              (0x174+ 0x800) 
#define  TD_SCALER_LAYER0_VSCALE_ST_PSTEP          (0x178+ 0x800) 
#define  TD_SCALER_LAYER0_HSCALE_STEP              (0x17c+ 0x800) 
#define  TD_SCALER_LAYER0_HSCALE_ST_PSTEP          (0x180+ 0x800) 
#define  TD_SCALER_LAYER0_HSYNC_CTL                (0x100+ 0x800) 
#define  TD_SCALER_LAYER0_DISP_SIZE                (0x11c+ 0x800)  
#define  TD_SCALER_LAYER0_BACKGROUND               (0x3d0+ 0x800)  

// write back
#define  TD_WB_DMA_CH_WR_EN                        (0x400+ 0x800)
#define  TD_WB_MODE                                (0x404+ 0x800)
#define  TD_WB_PIXEL_FORMAT_OUT                    (0x408+ 0x800)
#define  TD_WB_OUT_ADDR0_Y                         (0x40c+ 0x800)
#define  TD_WB_OUT_ADDR1_Y                         (0x410+ 0x800)
#define  TD_WB_OUT_ADDR0_UV                        (0x414+ 0x800)
#define  TD_WB_OUT_ADDR1_UV                        (0x418+ 0x800)
#define  TD_WB_HSTRIDE_OUT_Y                       (0x41c+ 0x800)
#define  TD_WB_HSTRIDE_OUT_UV                      (0x420+ 0x800)
#define  TD_WB_FRM_BUF_SIZE_OUT                    (0x424+ 0x800) 
#define  TD_WB_V_SIZE_ACTIVE_OUT                   (0x428+ 0x800) 
#define  TD_WB_H_SIZE_ACTIVE_OUT                   (0x42c+ 0x800) 
#define  TD_WB_CH1_DMA_MODE                        (0x440+ 0x800) 
#define  TD_WB_CH1_ERROR_UNIT                      (0x444+ 0x800)
#define  TD_WB_CH0_ERROR_TH                        (0x448+ 0x800)
#define  TD_WB_CH0_INFO_CLR                        (0x44c+ 0x800)
#define  TD_WB_CH1_RST_REQ                         (0x450+ 0x800)
#define  TD_WB_CH2_DMA_MODE                        (0x454+ 0x800)
#define  TD_WB_CH2_ERROR_UNIT                      (0x458+ 0x800)
#define  TD_WB_CH2_ERROR_TH                        (0x45c+ 0x800)
#define  TD_WB_CH2_INFO_CLR                        (0x464+ 0x800)
#define  TD_WB_CH2_RST_REQ                         (0x468+ 0x800)
#define  TD_WB_Y_WR_OUT_IDLE                       (0x480+ 0x800) 
#define  TD_WB_UV_WR_OUT_IDLE                      (0x484+ 0x800)  
#define  TD_WB_Y_WR_OUT_INT                        (0x488+ 0x800)  
#define  TD_WB_UV_WR_OUT_INT                       (0x48c+ 0x800)

// csc rgb2yuv
#define  TD_SCALER_RGB2YUV_COEFF00                 (0x344+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF01                 (0x348+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF02                 (0x34c+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF03                 (0x350+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF10                 (0x354+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF11                 (0x358+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF12                 (0x35c+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF13                 (0x360+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF20                 (0x364+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF21                 (0x368+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF22                 (0x36c+ 0x800)
#define  TD_SCALER_RGB2YUV_COEFF23                 (0x370+ 0x800)

// csc yuv2rgb
#define  TD_SCALER_YUV2RGB_COEFF00                 (0x384+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF01                 (0x388+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF02                 (0x38C+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF03                 (0x390+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF10                 (0x394+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF11                 (0x398+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF12                 (0x39C+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF13                 (0x3A0+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF20                 (0x3A4+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF21                 (0x3A8+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF22                 (0x3AC+ 0x800)
#define  TD_SCALER_YUV2RGB_COEFF23                 (0x3B0+ 0x800)

#define  KENDRYTE_TWOD_GET_VERSION                  _IOWR('T', 0, unsigned long)
#define  KENDRYTE_TWOD_ASYNC                        _IOWR('T', 1, unsigned long)
#define  KENDRYTE_TWOD_SYNC                         _IOWR('T', 2, unsigned long)
#define  KENDRYTE_TWOD_SET_DATA                     _IOWR('T', 3, unsigned long)


enum TWOD_FORMAT {
    TWOD_FORMAT_Y = 0,
    TWOD_FORMAT_YUV420SP = 1,
    TWOD_FORMAT_YUV422SP = 2,
    TWOD_FORMAT_RGB888 = 3,
    TWOD_FORMAT_ARGB8888 = 4,
};

enum TWOD_MODE {
    TWOD_ROT_90 = 1,
    TWOD_ROT_270 = 2,
    TWOD_SCALE = 3,
};

enum TWOD_INTR_STATUS {
    TWOD_ROTATION_DONE = 0,
    TWOD_ROTATION_TIMEOUT = 1,
    TWOD_MIX_DONE = 2,
};

struct td_image_info {

    uint32_t yrgb_addr;      /* yrgb    mem addr         */
    uint32_t uv_addr;        /* cb/cr   mem addr         */
    uint32_t v_addr;         /* cr      mem addr         */
    uint32_t format;         //definition by 
    uint32_t bpp;           /*  8bit  16bit   32bit */
    uint32_t pitch;
    uint32_t v_step;
    uint32_t v_st_pstep;
    uint32_t h_step;
    uint32_t h_st_pstep;
    unsigned short act_w;
    unsigned short act_h;
    unsigned short x_offset;
    unsigned short y_offset;
    unsigned short vir_w;
    unsigned short vir_h;
    unsigned int alpha_swap;
    
};


struct td_info {
    struct td_image_info src;      // if rotattion src , if scale layer0
    struct td_image_info dsc;      // if rotattion src,  if scale osd0
    uint32_t mode;   

};



struct twod_session {

    struct td_info *td_info;
    struct list_head session;
//    wait_queue_head_t   wait;
//    atomic_t num;                   // Unique ID 
};


struct kendryte_td {

    struct device   *dev;
    void __iomem    *base;
    int             irq_fram;
//    struct cdev     cdev;
    struct clk      *twod_apb;
    struct clk      *twod_axi;

    /* one time only one process allowed to config the register */
	spinlock_t reg_lock;
	/* lock  irq list */
	spinlock_t irq_lock;

    struct miscdevice miscdev;

    struct td_info *td_info;

    // wait queue
    uint32_t twod_int_flag;
    wait_queue_head_t  wait_que;

    // list 
    struct list_head	waiting;		
    struct list_head	running;		
    struct list_head	done;	

    atomic_t twod_num;  
    atomic_t rotation_num;

    struct mutex	mutex;	// mutex
};



void kendryte_td_set_wrap(struct kendryte_td *kendryte_td);
void kendryte_rot_reg_src_info(struct kendryte_td *kendryte_td, struct td_image_info *info, uint32_t rotation);
void kendryte_rot_reg_dst_info(struct kendryte_td *kendryte_td, struct td_image_info *info);
void kendryte_rot_set_intr(struct kendryte_td *kendryte_td, int status);
void kendryte_rot_clear_intr(struct kendryte_td *kendryte_td, int mode);
int kendryte_rot_get_intr_status(struct kendryte_td *kendryte_td, int status);
void kendrty_2d_get_reg_val(struct kendryte_td *kendryte_td);
void kendryte_rot_default_config(struct kendryte_td *kendryte_td);
void kendryte_rot_setframe_start(struct kendryte_td *kendryte_td, int val);

// scaler 
void kendryte_scaler_start_frame(struct kendryte_td *kendryte_td, uint32_t enable);
void kendryte_td_wb_set_config(struct kendryte_td *kendryte_td, struct td_image_info *info);
void kendryte_td_layer0_set_config(struct kendryte_td *kendryte_td, struct td_image_info *info);
void kedryte_td_scaler_init(struct kendryte_td *kendryte_td);
void kendryte_scaler_get_reg_val(struct kendryte_td *kendryte_td);

void kendryte_td_set_scaler_table(struct kendryte_td *kendryte_td);
void kedryte_td_scaler_sw_config(struct kendryte_td *kendryte_td);
#endif