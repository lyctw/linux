#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <asm/delay.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/scatterlist.h>
#include <linux/version.h>
#include <linux/debugfs.h>


#include "kendryte_td.h"

#include "kendryte_td_table.h"




static void kendryte_rot_set_format(struct kendryte_td *kendryte_td, enum TWOD_FORMAT format)
{
    uint32_t reg = 0;

    reg = readl(kendryte_td->base + TD_ROT_CTRL_MODE);

    //reg = (reg & ~(BIT_MASK(28))) | (swap << 28);
    reg = (reg & ~(GENMASK(5, 4))) | (format << 4);

    writel(reg, kendryte_td->base + TD_ROT_CTRL_MODE);
}


static void kendryte_rot_set_mode(struct kendryte_td *kendryte_td, int mode)
{
    uint32_t reg = 0;

    if(mode == TWOD_ROT_90)
    {
        reg = readl(kendryte_td->base + TD_ROT_CTRL_MODE);
        reg = (reg & ~(GENMASK(1, 0))) | (0 << 0);
        writel(reg, kendryte_td->base + TD_ROT_CTRL_MODE);
    }else{
        reg = readl(kendryte_td->base + TD_ROT_CTRL_MODE);
        reg = (reg & ~(GENMASK(1, 0))) | (1 << 0);
        writel(reg, kendryte_td->base + TD_ROT_CTRL_MODE);
    }
}


static void kendryte_rot_set_timeout(struct kendryte_td *kendryte_td, uint32_t val)
{
    writel(val, kendryte_td->base + TD_ROT_TIMEOUT);
}


static void kendryte_rot_set_dst_addr(struct kendryte_td *kendryte_td, uint32_t yrgb_addr, uint32_t uv_addr,uint32_t v_addr)
{
    writel(yrgb_addr, kendryte_td->base + TD_ROT_FRM_BASE_ADDR_Y_DES);

    writel(uv_addr, kendryte_td->base + TD_ROT_FRM_BASE_ADDR_U_DES);

    writel(v_addr, kendryte_td->base + TD_ROT_FRM_BASE_ADDR_V_DES);
}


static void kendryte_rot_set_src_addr(struct kendryte_td *kendryte_td, uint32_t yrgb_addr, uint32_t uv_addr,uint32_t v_addr)
{
    writel(yrgb_addr, kendryte_td->base + TD_ROT_FRM_BASE_ADDR_Y_SRC);

    writel(uv_addr, kendryte_td->base + TD_ROT_FRM_BASE_ADDR_U_SRC);

    writel(v_addr, kendryte_td->base + TD_ROT_FRM_BASE_ADDR_V_SRC);
}


static void kendryte_rot_set_src_stride(struct kendryte_td *kendryte_td, uint32_t y_stride, uint32_t u_stride, uint32_t v_stride)
{
    uint32_t reg = 0;

    reg = readl(kendryte_td->base + TD_ROT_STRIDE_Y);
    reg = (reg & ~(GENMASK(15, 0))) | (y_stride << 0);
    writel(reg, kendryte_td->base + TD_ROT_STRIDE_Y);

    reg = readl(kendryte_td->base + TD_ROT_STRIDE_U);
    reg = (reg & ~(GENMASK(15, 0))) | (u_stride << 0);
    writel(reg, kendryte_td->base + TD_ROT_STRIDE_U);


    reg = readl(kendryte_td->base + TD_ROT_STRIDE_V);
    reg = (reg & ~(GENMASK(15, 0))) | (v_stride << 0);
    writel(reg, kendryte_td->base + TD_ROT_STRIDE_V);
}


static void kendryte_rot_set_dst_stride(struct kendryte_td *kendryte_td, uint32_t y_stride, uint32_t u_stride, uint32_t v_stride)
{
    uint32_t reg = 0;

    reg = readl(kendryte_td->base + TD_ROT_STRIDE_Y);
    reg = (reg & ~(GENMASK(31, 16))) | (y_stride << 16);
    writel(reg, kendryte_td->base + TD_ROT_STRIDE_Y);

    reg = readl(kendryte_td->base + TD_ROT_STRIDE_U);
    reg = (reg & ~(GENMASK(31, 16))) | (u_stride << 16);
    writel(reg, kendryte_td->base + TD_ROT_STRIDE_U);


    reg = readl(kendryte_td->base + TD_ROT_STRIDE_V);
    reg = (reg & ~(GENMASK(31, 16))) | (v_stride << 16);
    writel(reg, kendryte_td->base + TD_ROT_STRIDE_V);
}


static void kendryte_rot_set_active_size(struct kendryte_td *kendryte_td, uint32_t act_w, uint32_t act_h)
{
    uint32_t reg = 0;

    reg = ((act_w - 1) << 0) + ((act_h - 1) << 16);
    writel(reg, kendryte_td->base + TD_ROT_IMAHE_SIZE_ACTIVE);
}


void kendryte_rot_reg_dst_info(struct kendryte_td *kendryte_td, struct td_image_info *info)
{

    uint32_t stride;

    printk("kendryte_rot_reg_dst_info  info->yrgb_addr is %x info->uv_addr is %x \n", info->yrgb_addr, info->uv_addr);
    // set dst addr
    kendryte_rot_set_dst_addr(kendryte_td, info->yrgb_addr, info->uv_addr, info->v_addr);

    // set stride
    stride = (info->act_w + info->bpp - 1) / info->bpp * info->bpp;     // 根据bpp，计算出对其方式 例如 act_w = 1080 bpp = 16 16位对齐，stride = （1080 + 15）/ 16 * 16 = 1088

    if(info->format == TWOD_FORMAT_YUV420SP)
    {
        kendryte_rot_set_dst_stride(kendryte_td, stride, stride, 0);
    }else{
        kendryte_rot_set_dst_stride(kendryte_td, stride, stride, stride);
    }

}


void kendryte_rot_reg_src_info(struct kendryte_td *kendryte_td, struct td_image_info *info, uint32_t rotation)
{
    uint32_t stride;
    uint32_t y_addr, u_addr, v_addr;

    printk("info->bpp is %d  info->act_w is %d info->act_h is %d info->yrgb_addr is %x  info->uv_addr is %x\n", info->bpp, info->act_w, info->act_h, info->yrgb_addr, info->uv_addr);

    // set fromat 
    kendryte_rot_set_format(kendryte_td, info->format);

    // set act size
    kendryte_rot_set_active_size(kendryte_td, info->act_w, info->act_h);

    // set stride
    stride = (info->act_w + info->bpp - 1) / info->bpp * info->bpp;     // 根据bpp，计算出对其方式 例如 act_w = 1080 bpp = 16 16位对齐，stride = （1080 + 15）/ 16 * 16 = 1088

    if(info->format == TWOD_FORMAT_YUV420SP)
    {
        kendryte_rot_set_src_stride(kendryte_td, stride, stride, 0);
    }else{
        kendryte_rot_set_src_stride(kendryte_td, stride, stride, stride);
    }

    printk("stride is %d \n", stride);
    switch(rotation)
    {
        case TWOD_ROT_90 :
            y_addr = info->yrgb_addr + (info->act_h - 1) * stride;
            u_addr = info->uv_addr  + (info->act_h / 2 - 1) * stride;
            if(info->format == TWOD_FORMAT_YUV420SP)
            {
                v_addr = 0;
            }else{
                v_addr = info->v_addr +  (info->act_h / 2) * stride;
            }

            kendryte_rot_set_src_addr(kendryte_td, y_addr, u_addr, v_addr);
            kendryte_rot_set_mode(kendryte_td, TWOD_ROT_90);

            break;

        case TWOD_ROT_270 :
            y_addr = info->yrgb_addr + info->act_w - 32;
            u_addr = info->uv_addr  + info->act_w - 32;
            if(info->format == TWOD_FORMAT_YUV420SP)
            {
                v_addr = 0;
            }else{
                v_addr = info->v_addr +  info->act_w - 32;
            }

            kendryte_rot_set_src_addr(kendryte_td, y_addr, u_addr, v_addr);
            kendryte_rot_set_mode(kendryte_td, TWOD_ROT_270);

            break;

        default:
            break;
    }
}

/*
 return 
        1 : Y done
        3 : Y and u/uv done
        7 : y/u/v done
*/
int kendryte_rot_get_intr_status(struct kendryte_td *kendryte_td, int status)
{
    uint32_t reg = 0;  
    int ret = 0;

    reg = readl(kendryte_td->base + TD_WRAP_INT_ST);

    switch(status)
    {
        case TWOD_ROTATION_DONE:
            ret = reg & 0x01;
            break;

        case TWOD_ROTATION_TIMEOUT:
            ret = (reg >> 1) & 0x01;
            break;
        
        case TWOD_MIX_DONE:
            ret = (reg >> 2) & 0x01;
            break;
   
        default:
            ret = -1;
        
    }
    return ret;
}


void kendryte_rot_set_intr(struct kendryte_td *kendryte_td, int status)
{
    uint32_t reg = 0; 

    reg = readl(kendryte_td->base + TD_WRAP_INT_ST);

    reg = (reg & ~(BIT_MASK(16))) | (status << 16);

    writel(reg, kendryte_td->base + TD_WRAP_INT_ST);
}

void kendryte_rot_clear_intr(struct kendryte_td *kendryte_td, int mode)
{
    uint32_t reg = 0;

    switch(mode)
    {
        case TWOD_ROTATION_DONE:
            reg = readl(kendryte_td->base + TD_WRAP_INT_ST);
            reg = (reg & ~(BIT_MASK(0))) | (1 << 0);
            writel(reg, kendryte_td->base + TD_WRAP_INT_ST);
            break;
        
        case TWOD_ROTATION_TIMEOUT :
            reg = readl(kendryte_td->base + TD_WRAP_INT_ST);
            reg = (reg & ~(BIT_MASK(1))) | (1 << 1);
            writel(reg, kendryte_td->base + TD_WRAP_INT_ST);
            break;

        case TWOD_MIX_DONE :
            reg = readl(kendryte_td->base + TD_WRAP_INT_ST);
            reg = (reg & ~(BIT_MASK(2))) | (1 << 2);
            writel(reg, kendryte_td->base + TD_WRAP_INT_ST);
            break;

        default :
            break;
    }
}

// set wrap
void kendryte_td_set_wrap(struct kendryte_td *kendryte_td)
{
    spin_lock(&kendryte_td->reg_lock);

    writel(0x3ff     , kendryte_td->base + 0x00);            
    writel(0xffffffff, kendryte_td->base + 0x0c);            
    writel(0         , kendryte_td->base + 0x20);            
    writel(0x01010101, kendryte_td->base + 0x24);            
    writel(0x01010101, kendryte_td->base + 0x28);            
    writel(0x01010101, kendryte_td->base + 0x2c);            
    writel(0x01010101, kendryte_td->base + 0x30);            
    writel(0x01234567, kendryte_td->base + 0x34);            
    writel(0x01234567, kendryte_td->base + 0x38);            
    writel(0x76543210, kendryte_td->base + 0x3c);            
    writel(0x76543210, kendryte_td->base + 0x40);            

//    reg = readl(kendryte_td->base + 0x78);
//    reg = reg | 0x01;
//    writel(1, kendryte_td->base + 0x78);                     
    writel(0x00020001, kendryte_td->base + 0x78);       

    writel(0xff0000, kendryte_td->base + 0x7c);  

    spin_unlock(&kendryte_td->reg_lock);            
}

// 0: enable , 1: disable
static void kendryte_rot_set_timeout_intr(struct kendryte_td *kendryte_td, uint32_t enable)
{
    uint32_t reg = 0; 

    reg = readl(kendryte_td->base + TD_WRAP_INT_ST);
    reg = (reg & ~(BIT_MASK(17))) | (enable << 17);
    writel(reg, kendryte_td->base + TD_WRAP_INT_ST);
}

void kendryte_rot_default_config(struct kendryte_td *kendryte_td)
{
    writel(0x1010f10, kendryte_td->base + 0x80 + 0x04) ;

    kendryte_rot_set_timeout(kendryte_td, 0xffff);

    // open rotation intr 
    kendryte_rot_set_intr(kendryte_td, TWOD_ROTATION_DONE);   

    // open timeout intr
    kendryte_rot_set_timeout_intr(kendryte_td, 0); 
}


void kendryte_rot_setframe_start(struct kendryte_td *kendryte_td, int val)
{
    writel(val, kendryte_td->base + TD_ROT_CMD);
    
}

void kendrty_2d_get_reg_val(struct kendryte_td *kendryte_td)
{
    uint32_t Iloop = 0; 
    uint32_t RegValue = 0; 

    while(1)
    {
        RegValue = readl(kendryte_td->base +  (Iloop << 2));
        printk("reg index = 0x%x, value = 0x%x !\n",Iloop,RegValue);
        Iloop++;
        if(Iloop >= 133)
        {
            break;
        }
    }
}


// scaler

void kendryte_td_set_rgb2yuv_csc(struct kendryte_td *kendryte_td)
{
    writel(0x132, kendryte_td->base + TD_SCALER_RGB2YUV_COEFF00);
    writel(0x259, kendryte_td->base + TD_SCALER_RGB2YUV_COEFF01);
    writel(0x75,  kendryte_td->base + TD_SCALER_RGB2YUV_COEFF02);
    writel(0x0,   kendryte_td->base + TD_SCALER_RGB2YUV_COEFF03);
    writel(0xf50, kendryte_td->base + TD_SCALER_RGB2YUV_COEFF10);
    writel(0xea5, kendryte_td->base + TD_SCALER_RGB2YUV_COEFF11);
    writel(0x20b, kendryte_td->base + TD_SCALER_RGB2YUV_COEFF12);
    writel(0x80,  kendryte_td->base + TD_SCALER_RGB2YUV_COEFF13);
    writel(0x20b, kendryte_td->base + TD_SCALER_RGB2YUV_COEFF20);
    writel(0xe4a, kendryte_td->base + TD_SCALER_RGB2YUV_COEFF21);
    writel(0xfab, kendryte_td->base + TD_SCALER_RGB2YUV_COEFF22);
    writel(0x80,  kendryte_td->base + TD_SCALER_RGB2YUV_COEFF23);
}


void kendryte_td_set_yuv2rgb_csc(struct kendryte_td *kendryte_td)
{
    writel(0x400, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF00);
    writel(0x0,   kendryte_td->base + TD_SCALER_YUV2RGB_COEFF01);
    writel(0x5a1, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF02);
    writel(0xf4c, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF03);
    writel(0x400, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF10);
    writel(0xe9e, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF11);
    writel(0xd22, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF12);
    writel(0x88,  kendryte_td->base + TD_SCALER_YUV2RGB_COEFF13);
    writel(0x400, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF20);
    writel(0x71e, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF21);
    writel(0x0,   kendryte_td->base + TD_SCALER_YUV2RGB_COEFF22);
    writel(0xf1c, kendryte_td->base + TD_SCALER_YUV2RGB_COEFF23);
}

// 0 enable , 1 disable
static void kendryte_td_scaler_set_intr(struct kendryte_td *kendryte_td, uint32_t enable)
{
    uint32_t reg = 0;

    reg = readl(kendryte_td->base + TD_WRAP_INT_ST);
    reg = (reg & ~(BIT_MASK(18))) | (enable << 18);
    writel(reg, kendryte_td->base + TD_WRAP_INT_ST);

}


void kendryte_td_set_scaler_table(struct kendryte_td *kendryte_td)
{
    uint32_t i = 0;

    for(i = 0; i < 64; i++)
    {
        writel(TD_V_Coef[i*2], kendryte_td->base + 0x9400 + ((i*2)<<2));
        writel(TD_V_Coef[i*2+1], kendryte_td->base + 0x9400 + ((i*2+1)<<2));
    }

    for(i = 0; i < 64; i++)
    {
        writel(TD_H_Coef[i*3], kendryte_td->base + 0x8800 + ((i*4)<<2) );
        writel(TD_H_Coef[i*3+1], kendryte_td->base + 0x8800 + ((i*4+1)<<2) );
        writel(TD_H_Coef[i*3+2], kendryte_td->base + 0x8800 + ((i*4+2)<<2) );
    }
}


void kendryte_td_get_scaler_table(struct kendryte_td *kendryte_td)
{
    uint32_t i = 0;

    for(i = 0; i < 64 * 2; i++)
    {
        printk("TD_V_Coef reg is %x , val is %x \n", 0x9400 + (i * 4), readl(kendryte_td->base + 0x9400 + (i * 4)));
    }
    
    for(i = 0; i < 64 * 3; i++)
    {
        printk("TD_H_Coef reg is %x , val is %x \n", 0x8800 + i, readl(kendryte_td->base + 0x8800 + (i * 4)));
    }
}


void kedryte_td_scaler_sw_config(struct kendryte_td *kendryte_td)
{
    writel(0x0000000f, kendryte_td->base + 0x800);
    writel(0x00000011, kendryte_td->base + 0x808);
}


void kedryte_td_scaler_init(struct kendryte_td *kendryte_td)
{
    uint32_t reg = 0;

//    kendryte_td_set_scaler_table(kendryte_td);

//    kedryte_td_scaler_sw_config(kendryte_td);
   
    //  set layer0 bd ctl
    writel(0x701, kendryte_td->base + TD_SCALER_LAYER0_LINE0_BD_CTL);
    writel(0x701, kendryte_td->base + TD_SCALER_LAYER0_LINE1_BD_CTL);
    writel(0x701, kendryte_td->base + TD_SCALER_LAYER0_LINE2_BD_CTL);
    writel(0x701, kendryte_td->base + TD_SCALER_LAYER0_LINE3_BD_CTL);
    // set layer4 bd ctl
    writel(0x701, kendryte_td->base + TD_SCALER_LAYER4_BD_CTL);
    // SET layer0 burst len ,uv endian mode (yuv420, y endian:1, uv endian:3, uv swap : 0)
   //writel(0x2108, kendryte_td->base + TD_SCALER_LAYER0_IMG_IN_DAT_MODE);
    writel(0x1002108, kendryte_td->base + TD_SCALER_LAYER0_IMG_IN_DAT_MODE);
   // set layer0 BLENTH
    writel(0xf, kendryte_td->base + TD_SCALER_LAYER0_BLENTH);
    // set display mix select 
    writel(0x10000, kendryte_td->base + TD_SCALER_MIX_SEL);
    // set display glb alpha
    writel(0xff, kendryte_td->base + TD_SCALER_LAYER0_GLB_ALPHA0);
    writel(0xff, kendryte_td->base + TD_SCALER_OSD_GLB_ALPHA1);
    //  set disp crtl
    writel(0x10002, kendryte_td->base + TD_SCALER_DISP_CTL);
    // set background
    writel(0xffffff, kendryte_td->base + TD_SCALER_LAYER0_BACKGROUND);

    // close osd
#if 0 
    reg = readl(kendryte_td->base + TD_SCALER_DISP_ENABLE);
    reg = (reg & ~(BIT_MASK(4))) | (0 << 4);
    writel(reg, kendryte_td->base + TD_SCALER_DISP_ENABLE);
#else
    reg = readl(kendryte_td->base + TD_SCALER_DISP_ENABLE);
    reg = (reg & ~(GENMASK(7, 4))) | (6 << 4);
    writel(reg, kendryte_td->base + TD_SCALER_DISP_ENABLE);
#endif
    // set csc
    kendryte_td_set_rgb2yuv_csc(kendryte_td);
    kendryte_td_set_yuv2rgb_csc(kendryte_td);

    // open scaler intr
    kendryte_td_scaler_set_intr(kendryte_td, 0);

    // set hsync ctl
    writel(0x320006, kendryte_td->base + TD_SCALER_LAYER0_HSYNC_CTL);
    

//    kendryte_td_get_scaler_table(kendryte_td);

}


static void kendryte_td_scaler_layer0_set_addr(struct kendryte_td *kendryte_td, uint32_t y_addr, uint32_t uv_addr)
{
    writel(y_addr, kendryte_td->base + TD_SCALER_LAYER0_BADDR0_Y);
    writel(uv_addr, kendryte_td->base + TD_SCALER_LAYER0_BADDR0_UV);
}

static void kendryte_td_scaler_layer0_set_stride(struct kendryte_td *kendryte_td, uint32_t stride)
{
    writel(stride, kendryte_td->base + TD_SCALER_LAYER0_IMG_IN_STRIDE);
}

static void kendryte_td_scaler_layer0_set_in_size(struct kendryte_td *kendryte_td, uint32_t h_act, uint32_t w_act)
{
    uint32_t reg = 0;

    reg = (w_act - 1) + ((h_act - 1) << 16);
    writel(reg, kendryte_td->base + TD_SCALER_LAYER0_IN_PIX_SIZE);
}

static void kendryte_td_scaler_layer0_set_out_size(struct kendryte_td *kendryte_td, uint32_t h_act, uint32_t w_act)
{
    uint32_t reg = 0;

    reg = (w_act - 1) + ((h_act - 1) << 16);
    writel(reg, kendryte_td->base + TD_SCALER_LAYER0_OUT_PIX_SIZE);
}

static void kendryte_td_scaler_layer0_set_status(struct kendryte_td *kendryte_td, uint32_t status)
{
    writel(status, kendryte_td->base + TD_SCALER_LAYER0_CRTL);
}

void kendryte_td_scaler_layer0_set_mode(struct kendryte_td *kendryte_td, uint32_t mode)
{
    uint32_t reg = 0;

    reg = readl(kendryte_td->base + TD_SCALER_LAYER0_IMG_IN_DAT_MODE);

    if(mode == TWOD_FORMAT_YUV420SP)
    {
        reg = (reg & ~(GENMASK(4, 0))) | (8 << 0);
        writel(reg, kendryte_td->base + TD_SCALER_LAYER0_IMG_IN_DAT_MODE);
    }

    if(mode == TWOD_FORMAT_YUV422SP)
    {
        reg = (reg & ~(GENMASK(4, 0))) | (9 << 0);
        writel(reg, kendryte_td->base + TD_SCALER_LAYER0_IMG_IN_DAT_MODE);
    }
}


void kendryte_td_layer0_set_config(struct kendryte_td *kendryte_td, struct td_image_info *info)
{
    uint32_t stride, offset ,val;
    uint32_t reg = 0;
    // set layer0 enable 
    kendryte_td_scaler_layer0_set_status(kendryte_td, 1);

    // start layer0
    reg = readl(kendryte_td->base + TD_SCALER_DISP_ENABLE);
    reg = (reg & ~(GENMASK(2, 0))) | (7 << 0);
    writel(reg, kendryte_td->base + TD_SCALER_DISP_ENABLE);

#if 0
    kedryte_td_scaler_init(kendryte_td);
#endif

    // set layer0 addr
    kendryte_td_scaler_layer0_set_addr(kendryte_td, info->yrgb_addr, info->uv_addr);
    // set layer0 in size
    kendryte_td_scaler_layer0_set_in_size(kendryte_td, info->act_h, info->act_w);
    // set in stride 
    
    if(info->bpp == 0 )
    {
        stride = ((info->act_w / 8) - 1) + ((info->act_h - 1) << 16);
    }
    else{
        stride = (info->act_w + info->bpp - 1) / info->bpp * info->bpp;     // 根据bpp，计算出对其方式 例如 act_w = 1080 bpp = 16 16位对齐，stride = （1080 + 15）/ 16 * 16 = 1088

    }
    kendryte_td_scaler_layer0_set_stride(kendryte_td, stride);
    // set offset
    offset = info->x_offset + (info->y_offset << 16);
    writel(offset, kendryte_td->base + TD_SCALER_LAYER0_IMG_IN_OFFSET);
    // set layer0 x start/end
#if 0
    val = info->x_offset + (info->y_offset << 16);
    writel(offset, kendryte_td->base + TD_SCALER_LAYER0_XCTL);
#else
    reg = readl(kendryte_td->base + TD_SCALER_XZONE_CTL);
    reg = (reg & ~(GENMASK(12, 0))) | (198 << 0);
    writel(reg, kendryte_td->base + TD_SCALER_XZONE_CTL);
#endif

#if 0 
    // set xzone 
    val = info->x_offset + (info->y_offset << 16);
    writel(offset, kendryte_td->base + TD_SCALER_XZONE_CTL);
#else
    // set xzone xstart
    reg = readl(kendryte_td->base + TD_SCALER_XZONE_CTL);
    reg = (reg & ~(GENMASK(12, 0))) | (198 << 0);
    writel(reg, kendryte_td->base + TD_SCALER_XZONE_CTL);
    writel(reg, kendryte_td->base + TD_SCALER_LAYER0_XCTL);

    // set yzone ystart
    reg = readl(kendryte_td->base + TD_SCALER_YZONE_CTL);
    reg = (reg & ~(GENMASK(12, 0))) | (42 << 0);
    writel(reg, kendryte_td->base + TD_SCALER_YZONE_CTL);
    writel(reg, kendryte_td->base + TD_SCALER_LAYER0_YCTL);

#endif
    // set step
    writel(info->v_step, kendryte_td->base + TD_SCALER_LAYER0_VSCALE_STEP);
    writel(info->v_st_pstep, kendryte_td->base + TD_SCALER_LAYER0_VSCALE_ST_PSTEP);
    writel(info->h_step, kendryte_td->base + TD_SCALER_LAYER0_HSCALE_STEP);
    writel(info->h_st_pstep, kendryte_td->base + TD_SCALER_LAYER0_HSCALE_ST_PSTEP);
    
    if(info->format == TWOD_FORMAT_YUV420SP)
    {
        // set wb yuv input mode  
        val = readl(kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);
        val = (val & ~(BIT_MASK(1))) | (0 << 1);
        writel(val, kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);

        // layer0 set mode
        kendryte_td_scaler_layer0_set_mode(kendryte_td, TWOD_FORMAT_YUV420SP);
    }
    if(info->format == TWOD_FORMAT_YUV422SP)
    {
        // set wb yuv input mode  
        val = readl(kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);
        val = (val & ~(BIT_MASK(1))) | (0 << 1);
        writel(val, kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);

        // layer0 set mode
        kendryte_td_scaler_layer0_set_mode(kendryte_td, TWOD_FORMAT_YUV422SP);
    }

    
}

// write back 
static void kendryte_td_wb_enable(struct kendryte_td *kendryte_td, uint32_t enable)
{
    if(enable == 1)
    {
        writel(0x03, kendryte_td->base + TD_WB_DMA_CH_WR_EN);
    }else{
        writel(0x00, kendryte_td->base + TD_WB_DMA_CH_WR_EN);
    }
}

void kendryte_td_wb_set_config(struct kendryte_td *kendryte_td, struct td_image_info *info)
{
    uint32_t reg = 0;
    uint32_t stride = 0;
    uint32_t in_v_act, in_h_act;
    uint32_t total_hsize, total_vsize;
    
    //wb enable 
    writel(0x01, kendryte_td->base + TD_WB_DMA_CH_WR_EN);

    // set wb mode
    writel(0x4, kendryte_td->base + TD_WB_MODE);

    if(info->format == TWOD_FORMAT_ARGB8888)
    {
        // set argb8888 format 
        reg = readl(kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);
        reg = (reg & ~(BIT_MASK(1))) | (0 << 0);
        writel(reg, kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);

        reg = readl(kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);
        reg = (reg & ~(BIT_MASK(1))) | (1 << 2);
        writel(reg, kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);

    }
    else if(info->format ==TWOD_FORMAT_YUV420SP)
    {
        reg = readl(kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);
        reg = (reg & ~(GENMASK(2, 0))) | (5 << 0);
        writel(reg, kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);

        // write stride
        if(info->bpp == 0)
        {
            stride = info->act_w;
        }else{
            stride = (info->act_w + info->bpp - 1) / info->bpp * info->bpp;
        }
        
        writel(stride, kendryte_td->base + TD_WB_HSTRIDE_OUT_Y);
        writel(stride, kendryte_td->base + TD_WB_HSTRIDE_OUT_UV);

    }
    else if(info->format ==TWOD_FORMAT_YUV422SP)
    {
        reg = readl(kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);
        reg = (reg & ~(GENMASK(4, 0))) | (1 << 0);
        writel(reg, kendryte_td->base + TD_WB_PIXEL_FORMAT_OUT);

        // write stride
        if(info->bpp == 0)
        {
            stride = info->act_w;

        }else{

            stride = (info->act_w + info->bpp - 1) / info->bpp * info->bpp;
        }

        writel(stride, kendryte_td->base + TD_WB_HSTRIDE_OUT_Y);
        writel(stride, kendryte_td->base + TD_WB_HSTRIDE_OUT_UV);

    }

    // set wb addr 
    writel(info->yrgb_addr, kendryte_td->base + TD_WB_OUT_ADDR0_Y);
    writel(info->yrgb_addr, kendryte_td->base + TD_WB_OUT_ADDR1_Y);
    writel(info->uv_addr, kendryte_td->base + TD_WB_OUT_ADDR0_UV);
    writel(info->uv_addr, kendryte_td->base + TD_WB_OUT_ADDR1_UV);

    // set size out
    writel(0xffff, kendryte_td->base + TD_WB_FRM_BUF_SIZE_OUT);

    //set act out
    writel(info->act_h - 1, kendryte_td->base + TD_WB_V_SIZE_ACTIVE_OUT);
    writel(info->act_w - 1, kendryte_td->base + TD_WB_H_SIZE_ACTIVE_OUT);


#if 0
    // set layer0 y start/end and xzone y 
    reg = readl(kendryte_td->base + TD_SCALER_LAYER0_XCTL);
    in_h_act = reg & 0xff;
    in_v_act = reg >> 16;
    reg = (in_h_act + info->act_w) + ((info->act_h + in_v_act) << 16);
    writel(reg, kendryte_td->base + TD_SCALER_LAYER0_YCTL);
    writel(reg, kendryte_td->base + TD_SCALER_YZONE_CTL);
#else
    reg = readl(kendryte_td->base + TD_SCALER_XZONE_CTL);
    reg = (reg & ~(GENMASK(28, 16))) | ((198 + info->act_w - 1) << 16);
    writel(reg, kendryte_td->base + TD_SCALER_XZONE_CTL);
    writel(reg, kendryte_td->base + TD_SCALER_LAYER0_XCTL);

    // set yzone ystart
    reg = readl(kendryte_td->base + TD_SCALER_YZONE_CTL);
    reg = (reg & ~(GENMASK(28, 16))) | ((42 + info->act_h - 1) << 16);
    writel(reg, kendryte_td->base + TD_SCALER_YZONE_CTL);
    writel(reg, kendryte_td->base + TD_SCALER_LAYER0_YCTL);
#endif

    // write totle szie 
    reg = readl(kendryte_td->base + TD_SCALER_YZONE_CTL) & 0xff;
    total_hsize = info->act_w * 110 / 100 + 192;
    total_vsize = info->act_h + 3 + reg;
    writel((total_vsize&0x1FFF)<<16|(total_hsize&0x1FFF), kendryte_td->base + TD_SCALER_LAYER0_DISP_SIZE);

    // set layer0 out size
    kendryte_td_scaler_layer0_set_out_size(kendryte_td, info->act_h, info->act_w);
    // write ch1 ch0 ch2 
    writel(0xc001, kendryte_td->base + TD_WB_CH1_DMA_MODE);
    //writel(0x1001, kendryte_td->base + TD_WB_CH1_DMA_MODE);
    writel(0x1, kendryte_td->base + TD_WB_CH1_ERROR_UNIT);
    writel(0x1, kendryte_td->base + TD_WB_CH0_ERROR_TH);
    writel(0x0, kendryte_td->base + TD_WB_CH0_INFO_CLR);
    writel(0x0, kendryte_td->base + TD_WB_CH1_RST_REQ);
    writel(0x1, kendryte_td->base + TD_WB_CH2_DMA_MODE);
    writel(0x0, kendryte_td->base + TD_WB_CH2_ERROR_UNIT);
    writel(0x0, kendryte_td->base + TD_WB_CH2_ERROR_TH);
   // writel(0x4001, kendryte_td->base + 0x460+ 0x800);
    writel(0xc001, kendryte_td->base + 0x460+ 0x800);
    writel(0x1, kendryte_td->base + TD_WB_CH2_INFO_CLR);
    writel(0x1, kendryte_td->base + TD_WB_CH2_RST_REQ);
/*
    // set idel ,int
    writel(0x1, kendryte_td->base + TD_WB_Y_WR_OUT_IDLE);
    writel(0x1, kendryte_td->base + TD_WB_UV_WR_OUT_IDLE);
    writel(0x1, kendryte_td->base + TD_WB_Y_WR_OUT_INT);
    writel(0x0, kendryte_td->base + TD_WB_UV_WR_OUT_INT);
*/
    kendryte_td_wb_enable(kendryte_td, 1);
}

void kendryte_scaler_start_frame(struct kendryte_td *kendryte_td, uint32_t enable)
{
    uint32_t reg = 0;

    if(enable == 1)
    {
        reg = readl(kendryte_td->base + TD_SCALER_DISP_ENABLE);
        reg = (reg & ~(BIT_MASK(7))) | (1 << 7);
        writel(reg, kendryte_td->base + TD_SCALER_DISP_ENABLE);
    }
    else
    {
        reg = readl(kendryte_td->base + TD_SCALER_DISP_ENABLE);
        reg = (reg & ~(BIT_MASK(7))) | (0 << 7);
        writel(reg, kendryte_td->base + TD_SCALER_DISP_ENABLE);
    }
}

void kendryte_scaler_get_reg_val(struct kendryte_td *kendryte_td)
{
    uint32_t Iloop = 0; 
    uint32_t RegValue = 0; 
    Iloop = 512;
    while(1)
    {
        RegValue = readl(kendryte_td->base + (Iloop << 2));
        printk("reg index = 0x%x, value = 0x%x !\n",Iloop*4,RegValue);
        Iloop++;
        if(Iloop >= 812)
        {
            break;
        }
    }
}


MODULE_LICENSE("GPL");

