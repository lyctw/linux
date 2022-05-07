// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 Andes Technology Corporation
 * Rick Chen, Andes Technology Corporation <rick@andestech.com>
 */
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/irqflags.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/cacheinfo.h>
#include <linux/sizes.h>
#include <asm/csr.h>
#include <asm/andesv5/proc.h>
#include <asm/andesv5/csr.h>
#include <linux/slab.h>
#include <linux/device.h>

#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/dma-buf.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

// gpio 
#include <linux/gpio/driver.h>
#include <linux/gpio/consumer.h>

#include "sysctl_clk.h"

#include "kendryte_2d.h"


#define EVBLP3
#undef EVBLP3

#define ISP_CMD_READ_REG         _IOWR('q', 1, struct isp_reg)
#define ISP_CMD_WRITE_REG        _IOWR('q', 2, struct isp_reg)
#define ISP_CMD_ACT_SENSOR_RST   _IOWR('q', 3, unsigned long)
#define ISP_CMD_RLS_SENSOR_RST   _IOWR('q', 4, unsigned long)
#define ISP_CMD_SET_DMABUF_Y_FD    _IOWR('q', 5, unsigned long)
#define ISP_CMD_SET_DMABUF_UV_FD    _IOWR('q', 6, unsigned long)
#define ISP_CMD_MIPI_DSI_INIT    _IOWR('q', 7, unsigned long)

#define DVP_CMD_ACT_SENSOR_RST    _IOWR('q', 8, unsigned long)
#define DVP_CMD_RLS_SENSOR_RST    _IOWR('q', 9, unsigned long)
#define ISP_CMD_SET_DS2BUF        _IOWR('q', 10, unsigned long)
#define ISP_CMD_GET_DS2BUF        _IOWR('q', 11, unsigned long)
#define ISP_CMD_EN_DS2_INT        _IOWR('q', 12, unsigned long)
#define ISP_CMD_MIPI_DSI_ILI7807_INIT    _IOWR('q', 13, unsigned long)
#define ISP_CMD_MIPI_DSI_HX8399_INIT    _IOWR('q', 14, unsigned long)

#define ISP_CMD_DS1_ADDR                _IOWR('q', 15, unsigned long)
#define ISP_CMD_DS1_BUFF_COUNT          _IOWR('q', 16, unsigned long)

#define ISP_CMD_DS0_SRC_ADDR                _IOWR('q', 17, unsigned long)
#define ISP_CMD_DS1_DES_ADDR                _IOWR('q', 18, unsigned long)

#define ISP_CMD_TWOD_TEST_ADDR                _IOWR('q', 19, unsigned long)

#define ISP_CMD_ISP_RESET                _IOWR('q', 20, unsigned long)

#define ISP_CMD_SET_DS1_SIZE                _IOWR('q', 21, unsigned long)

#define ISP_CMD_DS1_USE_IRQ                _IOWR('q', 22, unsigned long)

#define ISP_CMD_DS0_USE_TWOD                _IOWR('q', 23, unsigned long)

#define ISP_CMD_CLEAR_FLAG                _IOWR('q', 24, unsigned long)

#define VIDEO_CLOSE_ALL_LAYER                _IOWR('q', 25, unsigned long)

#define VIDEO_SUBSYS_MIPI_CORNER_REG_BASE       0x92680000
#define VIDEO_TXDPHY_BASE_ADDR                  0x92718000
#define VIDEO_DSI_BASE_ADDR                     0x92710000
#define VIDEO_SUBSYSTEM_BASE_ADDR   0x92600000
#define VIDEO_RXDPHY_BASE           VIDEO_SUBSYSTEM_BASE_ADDR

#define TXDPHY_PLL_891M_F           3563
#define TXDPHY_PLL_891M_R           24
#define TXDPHY_PLL_891M_D           3

#define TXDPHY_PLL_1782M_F          1781
#define TXDPHY_PLL_1782M_R          24
#define TXDPHY_PLL_1782M_D          0

#define TXDPHY_PLL_445M_F           1781
#define TXDPHY_PLL_445M_R           24
#define TXDPHY_PLL_445M_D           3

#define TXDPHY_PLL_222M_F           1781
#define TXDPHY_PLL_222M_R           24
#define TXDPHY_PLL_222M_D           7


/*************************dsi********************************/
#define   MCTL_DPHY_CFG0_OFFSET             0x10  //0x00000600 DPHY Power and Reset Control
#define ISP_EXTCTRL_BASE_ADDR               0x92600000  //addr range 0x70000~0x7ffff
#define   CNDS_DSI_REG_START                0x92710000
#define   DPI_IRQ_EN_OFFSET                 0x1a0 //0x00000000 DPI interrupt enable
#define   MCTL_MAIN_PHY_CTL_OFFSET          0x8   //0x00000000 Main control setting for the physical lanes and drive the static signals for DPHY clock lane
#define   MCTL_DPHY_TIMEOUT1_OFFSET         0x14  //0x00000000 Main DPHY time-out settings. To better understand the way this register is
#define   MCTL_DPHY_TIMEOUT2_OFFSET         0x18  //0x00000000 To better understand the way this register is used, please refer to Section : DSI
#define   MCTL_MAIN_DATA_CTL_OFFSET         0x4   //0x00000000 Main Control - main control setting for datapath
#define   MCTL_MAIN_EN_OFFSET               0xc   //0x00000000 Control start/stop of the DSI link
#define   CMD_MODE_STS_CTL_OFFSET           0x134 //0x00000000 Controls the enabling and edge detection of command status bits EDGE = 0
#define   MCTL_ULPOUT_TIME_OFFSET           0x1c  //0x00000000 Specify time to leave ULP mode. The time-out is reached when the ulpout
#define   MCTL_MAIN_STS_OFFSET              0x24  //0x00000000 Status of the DSI link
#define   DIRECT_CMD_MAIN_SETTINGS_OFFSET    0x84  //0x00000000 Main control of the Direct Command function.
#define   DIRECT_CMD_WRDAT_OFFSET            0x90  //0x00000000 Write data to outgoing Direct Command FIFO, byte 0 to 3; applicable to either
#define   DIRECT_CMD_SEND_OFFSET             0x80  //0x00000000 Direct_cmd_send is not a real register. When this address is written (whatever
#define   DIRECT_CMD_STS_OFFSET              0x88  //0x00000000 Direct Command Status: To ensure that the observed status bits are coherent
#define   DIRECT_CMD_STS_CLR_OFFSET         0x158 //0x00000000 Direct command status register clear function. Write '1' to clear
#define   DIRECT_CMD_STS_CTL_OFFSET         0x138 //0x00000000 Controls the enabling and edge detection of Direct Command status bits
#define   TVG_IMG_SIZE_OFFSET              0x100  //0x00000000 TVG Generated image size
#define   TVG_COLOR1_OFFSET                0x104  //0x00000000 Color 1 of the dummy frame G, R
#define   TVG_COLOR1_BIS_OFFSET            0x108  //0x00000000 Color 1 of the dummy frame , B
#define   TVG_COLOR2_OFFSET                0x10c  //0x00000000 Color 2 of the dummy frame, G, R
#define   TVG_COLOR2_BIS_OFFSET            0x110  //0x00000000 Color 2 of the dummy frame, B
#define   VID_VSIZE1_OFFSET                0xb4   //0x00000000 Image vertical Sync and Blanking settings
#define   VID_MODE_STS_CTL_OFFSET           0x140 //0x00000000 Control the enabling and edge detection of VSG status bits
#define   VID_VSIZE2_OFFSET                0xb8   //0x00000000 Image vertical active line setting
#define   VID_HSIZE1_OFFSET                0xc0   //0x00000000 Image horizontal sync and Blanking setting
#define   VID_HSIZE2_OFFSET                0xc4   //0x00000000 Image horizontal byte size setting
#define   VID_BLKSIZE2_OFFSET              0xd0   //0x00000000 Pulse Mode blanking packet size
#define   VID_DPHY_TIME_OFFSET             0xdc   //0x00000000 Time of D-PHY behavior for wakeup time and Line duration for LP during
#define   VID_VCA_SETTING1_OFFSET          0xf4   //0x00000000 VCA control register 1
#define   VID_VCA_SETTING2_OFFSET          0xf8   //0x00000000 VCA control register 2
#define   VID_DPHY_TIME_OFFSET             0xdc   //0x00000000 Time of D-PHY behavior for wakeup time and Line duration for LP during
#define   VID_MAIN_CTL_OFFSET              0xb0   //0x80000000 Video mode main control
#define   DIRECT_CMD_RD_STS_CLR_OFFSET      0x15c //0x00000000 Direct command read status register clear function. Write '1' to clear
#define   DIRECT_CMD_RD_STS_OFFSET          0xa8  //0x00000000 Status of the read command received. It is recommended to clear
#define   DIRECT_CMD_RD_PROPERTY_OFFSET     0xa4  //0x00000000 read command characteristics
#define   DIRECT_CMD_RDDAT_OFFSET           0xa0  //0x00000000 Data from incoming Direct Command receive path, byte 0 to 3. NOTE: SW

/***************init *****************/
static void  mipi_coner_init(void);
static void mipi_txdphy_set_pll(uint32_t fbdiv, uint32_t refdiv, uint32_t outdiv);

struct isp_dmabuf {
    int fd;
    unsigned int dma_addr;
};

struct isp_reg {
    unsigned int id;
    unsigned int value;
};

typedef enum
{
    DS0_IRQ_ID = 1,
    DS1_IRQ_ID,
    DS2_IRQ_ID,

} ISP_IRQ;


struct ds_data {
    uint32_t addr;
    ISP_IRQ isp_irq_id;
};

struct isp_plat {
    struct resource *res;
    void __iomem    *regs;
    unsigned long   regs_size;
    void __iomem    *gpio;
    int             major;
    int             minor;
    struct class    *class;
    struct cdev     cdev;
	void __iomem    *iomux;
	int 		irq_ds2;
    int 		irq_vo;

    struct gpio_desc *dsi_en;
    struct gpio_desc *dsi_rst;
    struct gpio_desc *csi_en;

    // sensor 
    struct gpio_desc *imx385_powerdown;
    struct gpio_desc *imx385_reset;
    struct gpio_desc *irs3281_powerdown;
    struct gpio_desc *irs3281_reset;
    struct gpio_desc *dvp_powerdown;

    // ds1 for h264
    uint32_t ds1_addr;
    uint32_t ds1_buf_cut;
    uint32_t h264_addr;

    // ds0 for vo need totation 90
    uint32_t ds0_src_addr;
    uint32_t ds0_des_addr;
    uint32_t ds0_buf_cut;


    // report ds addr for app 
    struct ds_data *ds_data;
};

typedef enum
{
    RXDPHY_CHCFG_2X2,
    RXDPHY_CHCFG_1X4,
    RXDPHY_CHCFG_MAX,
} rxdphy_chcfg_t;


typedef enum
{
    LCD_ILI7807 = 0,
    LCD_HX8399,
    LCD_AML,
} LCD;

typedef enum
{
    RXDPHY_SPEED_MODE_2500M,
    RXDPHY_SPEED_MODE_1500M,
    RXDPHY_SPEED_MODE_MAX,
} rxdphy_speed_mode_t;


static struct isp_plat *plat;                                                                   
int dev_id = 0;
unsigned int int_cnt = 0;
ktime_t last_tm;
struct dma_buf *dmabuf_y = NULL;
struct dma_buf *dmabuf_uv = NULL;
struct device *device = NULL;
#define DS2_BUF_NUM_MAX  5
unsigned int ds2_buf_num = 0;
unsigned int ds2_buf[DS2_BUF_NUM_MAX];

DECLARE_WAIT_QUEUE_HEAD(isp_waitq);//注册一个等待队列gnne_waitq

static unsigned int isp_int_flag;
//uint32_t *sysctl_clk_base = ioremap(0x97000000+0x1000, 0x1000);					// sysclk base addr

static int ds0_flag = 0;
static int ds1_cut = 0;
static int ds0_cut = 0;
static int fram_val = 1080 * 1920 * 3 / 2;
static int fram_uv_addr = 640 * 480;//1080 * 1920 ;//1080 * 1920;
static int ds0_fram_uv_addr = 1080 * 1920 ;//1080 * 1920;
static int ds0_use_twod = 0;
static int ds1_use_irq = 0;
static int ds_set_addr_flag = 0;

static int ds0_twod_stride = 1088;

#define DMA_WR_INT_ST_OUT1_FRAM      0x100  
#define DMA_WR_INT_ST_OUT0_FRAM      0x01
// base addr   0x92600000 0x0 0x200000    isp addr = base + 0x50000;
static void isp_change_ds1_ping_addr(uint32_t val)
{

    iowrite32(val, plat->regs + 0x500c8);                           // y ping   
    iowrite32(val + fram_uv_addr, plat->regs + 0x500d0);            // uv ping 
}

static void isp_change_ds1_pang_addr(uint32_t val)
{
    iowrite32(val, plat->regs + 0x500cc);                           // y ping 
    iowrite32(val + fram_uv_addr, plat->regs + 0x500d4);            // uv ping 
}


static void isp_change_ds0_ping_addr(uint32_t val)
{

    iowrite32(val, plat->regs + 0x500b4);                           // y ping   
    iowrite32(val + ds0_fram_uv_addr, plat->regs + 0x500bc);            // uv ping 
}

static void isp_change_ds0_pang_addr(uint32_t val)
{
    iowrite32(val, plat->regs + 0x500b8);                           // y ping 
    iowrite32(val + ds0_fram_uv_addr, plat->regs + 0x500c0);            // uv ping 
}



static uint32_t vo_des_addr = 0;
unsigned long  ds_timer1 = 0;
unsigned long  ds_timer2 = 0;
int ds_timer_flag = 0;
int ds_timer_cut = 0;
static irqreturn_t isp_irq_handler(int irq, void *dev_id)
{
    uint32_t  reg;
    uint32_t  ds0_src_val;
    uint32_t y_src, u_src, y_des, u_des;
    unsigned int int_status =  ioread32(plat->regs + 0x501e8);              // read intr status 
    uint32_t intr_mask = ioread32(plat->regs + 0x501ec);                    //read intr mask bit

	if(ds_set_addr_flag == 0)
		return IRQ_HANDLED;
//    iowrite32(0xffffffff, plat->regs + 0x501e8);                // clear all ds1 intr

#if 0
    if(ds_timer_flag == 0)
    {
        ds_timer1 = sbi_get_cycles();
        ds_timer_flag = 1;
    }

    if(ds_timer_flag == 1)
    {
        if(ds_timer_cut == 120)
        {
            ds_timer2 = sbi_get_cycles() - ds_timer1;
            ds_timer_cut = 0;
            ds_timer_flag = 0;

            printk("ds_timer2 is %d int_status is %x intr_mask is %d \n", ds_timer2, int_status, intr_mask);
        }
    }

    ds_timer_cut ++;
#endif

    //ds1 intr
    if((int_status & DMA_WR_INT_ST_OUT1_FRAM) == DMA_WR_INT_ST_OUT1_FRAM)
    {     
        iowrite32(0xff00, plat->regs + 0x501e8);                // clear all ds1 intr

        plat->ds_data->isp_irq_id = DS1_IRQ_ID;
        if(plat->ds1_buf_cut != 1)
        {
            if(ds1_cut == plat->ds1_buf_cut)
                ds1_cut = 0;
        
            if(ds1_cut < plat->ds1_buf_cut - 2)
            {
                if((ds1_cut + 1) % 2 == 0)
                {
                    plat->ds_data->addr = plat->ds1_addr +(fram_val * (ds1_cut + 2));
                    isp_change_ds1_pang_addr(plat->ds1_addr +(fram_val * (ds1_cut + 2)));
                }                    
                else    
                {
                    plat->ds_data->addr = plat->ds1_addr +(fram_val * (ds1_cut + 2));
                    isp_change_ds1_ping_addr(plat->ds1_addr +(fram_val * (ds1_cut + 2)));
                }
                    
            }
            else
            {
                if(ds1_cut == plat->ds1_buf_cut - 1)  // 4 - 1= 3
                {
                    plat->ds_data->addr = plat->ds1_addr + fram_val;
                    isp_change_ds1_pang_addr(plat->ds_data->addr);  
            
                }else if(ds1_cut == plat->ds1_buf_cut - 2)  // 4 - 2 = 2
                {
                    plat->ds_data->addr = plat->ds1_addr ;                   
                    isp_change_ds1_ping_addr(plat->ds_data->addr);      
                }
            }
        }
        else
        {
            plat->ds_data->addr = plat->ds1_addr;
        }
           
        // report int for app    
        isp_int_flag = 1;        
        wake_up_interruptible(&isp_waitq);   /* 唤醒休眠的进程，即调用read函数的进程 */
        ds1_cut ++;
    }


    if(ds0_use_twod == 1)
    {
        // ds0 intr
        if((int_status & DMA_WR_INT_ST_OUT0_FRAM) == DMA_WR_INT_ST_OUT0_FRAM)
        { 
    #if 1
    //        plat->ds_data->isp_irq_id = DS0_IRQ_ID;
        
            iowrite32(0xff, plat->regs + 0x501e8);              // clear all ds0 intr

            if(plat->ds0_buf_cut != 1)
            {
                if(ds0_cut == plat->ds0_buf_cut)  //4
                    ds0_cut = 0;
            
                if(ds0_cut < plat->ds0_buf_cut - 2)  // 2 < 4 -2 
                {
                    if((ds0_cut + 1) % 2 == 0)
                    {
                        ds0_src_val = plat->ds0_src_addr + (fram_val * ds0_cut);//plat->ds0_src_addr + (fram_val * (ds0_cut + 2));   //get src base
                        reg = plat->ds0_des_addr + (fram_val * ds0_cut);                                               // get des base
    //                    vo_des_addr = reg - fram_val;

                        y_src = ds0_src_val + (ds0_twod_stride - 1) * 1920;
                        u_src = (ds0_src_val + (ds0_twod_stride * 1920) ) + (ds0_twod_stride / 2 - 1) * 1920;
    #if 0                      
                        kendrty_set_rot_frame_src_addr(y_src, u_src, 0);     // set src base
                        kendrty_set_rot_frame_des_addr(reg, reg + ds0_fram_uv_addr, 0);
                        kendryte_set_fram_addr(reg, ds0_cut);

                        kendrty_set_rot_frame_run(1);
                        kendrty_set_rot_frame_run(0);                    
    #endif
                        isp_change_ds0_pang_addr(plat->ds0_src_addr + (fram_val * (ds0_cut + 2)));
                    }                    
                    else    
                    {
                        ds0_src_val= plat->ds0_src_addr + (fram_val * ds0_cut);//plat->ds0_src_addr + (fram_val * (ds0_cut + 2));
                        reg = plat->ds0_des_addr + (fram_val * (ds0_cut));   
    //                    if(ds0_cut == 0)
    //                        vo_des_addr = plat->ds0_des_addr + (fram_val * (plat->ds0_buf_cut -1));
    //                    else
    //                        vo_des_addr = reg - fram_val;
                        
                        y_src = ds0_src_val + (ds0_twod_stride - 1) * 1920;
                        u_src = (ds0_src_val + (ds0_twod_stride * 1920) ) + (ds0_twod_stride / 2 - 1) * 1920;
    #if 0                    
                        kendrty_set_rot_frame_src_addr(y_src, u_src, 0);     // set src base
                        kendrty_set_rot_frame_des_addr(reg, reg + ds0_fram_uv_addr, 0);
                        kendryte_set_fram_addr(reg, ds0_cut);

                        kendrty_set_rot_frame_run(1);
                        kendrty_set_rot_frame_run(0);
    #endif
                        isp_change_ds0_ping_addr(plat->ds0_src_addr + (fram_val * (ds0_cut + 2)));
                    }
                        
                }
                else
                {
                    if(ds0_cut == plat->ds0_buf_cut - 1)  // 4 - 1= 3
                    {
                        ds0_src_val = plat->ds0_src_addr + (fram_val * ds0_cut);//plat->ds0_src_addr;
                        reg = plat->ds0_des_addr + (fram_val * ds0_cut);   
    //                    vo_des_addr = reg - fram_val;

                        y_src = ds0_src_val + (ds0_twod_stride - 1) * 1920;
                        u_src = (ds0_src_val + (ds0_twod_stride * 1920) ) + (ds0_twod_stride / 2 - 1) * 1920;
    #if 0                    
                        kendrty_set_rot_frame_src_addr(y_src, u_src, 0);     // set src base
                        kendrty_set_rot_frame_des_addr(reg, reg + ds0_fram_uv_addr, 0);
                        kendryte_set_fram_addr(reg, ds0_cut);

                        kendrty_set_rot_frame_run(1);
                        kendrty_set_rot_frame_run(0);
    #endif
                        isp_change_ds0_pang_addr(plat->ds0_src_addr + fram_val);      // buff[1]  
                
                    }else if(ds0_cut == plat->ds0_buf_cut - 2)  // 4 - 2 = 2
                    {
                        ds0_src_val = plat->ds0_src_addr + (fram_val * ds0_cut);//plat->ds0_src_addr + (fram_val * 3);
                        reg = plat->ds0_des_addr + (fram_val * ds0_cut);  
    //                    vo_des_addr = reg - fram_val; 

                        y_src = ds0_src_val + (ds0_twod_stride - 1) * 1920;
                        u_src = (ds0_src_val + (ds0_twod_stride * 1920) ) + (ds0_twod_stride / 2 - 1) * 1920;
    #if 0                     
                        kendrty_set_rot_frame_src_addr(y_src, u_src, 0);     // set src base
                        kendrty_set_rot_frame_des_addr(reg, reg + ds0_fram_uv_addr, 0);
                        kendryte_set_fram_addr(reg, ds0_cut);

                        kendrty_set_rot_frame_run(1);
                        kendrty_set_rot_frame_run(0);
    #endif
                        
                        isp_change_ds0_ping_addr(plat->ds0_src_addr);            //  buff[0]  
                    }
                }
                //timer1 = jiffies;
                //timer1 = sbi_get_cycles();
            }
            else
            {
                plat->ds_data->addr = plat->ds0_src_addr;
            }

            ds0_cut ++;
    #endif

    #if 0
            ds0_flag ++;
            if(ds0_flag == 120)
            {
                ds0_flag = 0;
                printk("ds0_flag intr ds0_cut is %d int_status is %x \n", ds0_cut, int_status);
            }
    #endif
        }
    }
    

    return IRQ_HANDLED;
}


#define VO_DISP_IRQ0                        0x01
static int vo_cut = 0;

static void vo_change_laye2_ping_addr(uint32_t y_addr, uint32_t uv_addr)
{

    iowrite32(y_addr, plat->regs + 0x100204);
    iowrite32(uv_addr, plat->regs + 0x100208);
    
}

static void vo_change_laye2_pang_addr(uint32_t y_addr, uint32_t uv_addr)
{
    iowrite32(y_addr, plat->regs + 0x10020c);
    iowrite32(uv_addr, plat->regs + 0x100210);

}


static void vo_change_laye1_ping_addr(uint32_t y_addr, uint32_t uv_addr)
{

    iowrite32(y_addr, plat->regs + 0x1001c4);
    iowrite32(uv_addr, plat->regs + 0x1001c8);
    
}

static void vo_change_laye1_pang_addr(uint32_t y_addr, uint32_t uv_addr)
{
    iowrite32(y_addr, plat->regs + 0x1001cc);
    iowrite32(uv_addr, plat->regs + 0x1001d0);
}


static void vo_change_laye0_ping_addr(uint32_t y_addr, uint32_t uv_addr)
{

    iowrite32(y_addr, plat->regs + 0x10014c);
    iowrite32(uv_addr, plat->regs + 0x100150);
    
}

static void vo_change_laye0_pang_addr(uint32_t y_addr, uint32_t uv_addr)
{
    iowrite32(y_addr, plat->regs + 0x100154);
    iowrite32(uv_addr, plat->regs + 0x100158);
}

static void video_close_all_layer(void)
{
	iowrite32(0x00, plat->regs + 0x100118);
	iowrite32(0x01, plat->regs + 0x100004);
}

static int vo_flag = 0;
static int vo_i = 0;
unsigned long  vo_timer1 = 0;
unsigned long  vo_timer2 = 0;
int vo_timer_flag = 0;
// vo irq   reg = <0x0 0x92600000 0x0 0x200000>;
static irqreturn_t vo_irq_handler(int irq, void *dev_id)
{
    uint32_t ret = 0;
    struct twod_data data = {0};
    uint32_t addr;
    unsigned int int_status = ioread32(plat->regs + 0x1003ec);    //10000001
    iowrite32(0xffffffff, plat->regs + 0x1003ec);

//    printk("vo start intr int_status si %x \n", int_status);
    // VO_DISP_IRQ0 
    if((int_status & VO_DISP_IRQ0) == VO_DISP_IRQ0)
    {
        
//        addr = plat->ds0_des_addr + (3 * (1080 * 1920 * 3 / 2));
//        vo_change_laye0_ping_addr(addr, addr + (1920 * 1080));
//        vo_change_laye0_pang_addr(addr, addr + (1920 * 1080));
//        kendrty_get_frame_addr(data);  // get twod addr 

/*
        if(vo_timer_flag == 0)
        {
            vo_timer1 = sbi_get_cycles();
            vo_timer_flag = 1;
        }

        if(vo_timer_flag == 1)
        {
            if(vo_flag == 120)
            {
                vo_timer2 = sbi_get_cycles() - vo_timer1;
                vo_flag = 0;
                vo_timer_flag = 0;

                printk("vo_timer2 is %d \n", vo_timer2);
            }
        }
        vo_flag ++ ;


*/
        if(vo_cut == 2)
            vo_cut = 0;

#if 0
        // ds0 for twod 
        ret = kendrty_get_frame_addr(data);  // get twod addr 
        addr = plat->ds0_des_addr + (ret * (fram_val));

        if(vo_cut == 0)
        {
//            vo_change_laye0_ping_addr(addr, addr + (ds0_fram_uv_addr));   // set ping
            vo_change_laye1_ping_addr(addr, addr + (ds0_fram_uv_addr));   // set ping
            
        }   
        else
        {
//            vo_change_laye0_pang_addr(addr, addr + (ds0_fram_uv_addr));   // set ping
            vo_change_laye1_pang_addr(addr, addr + (ds0_fram_uv_addr));   // set ping
        }
            
#endif

#if 0

        if(plat->ds1_buf_cut != 1)
        {
//            ret = ioread32(plat->regs + 0x100224);    //10000001
//            ret = (ret >> 12) & 0x01;

            ret = kendrty_get_frame_addr(data);  // get twod addr 
            data.val = ret;
            if(vo_cut == 0)
            {
                if(data.val - 1 == 0)
                {

                    addr = plat->ds0_des_addr + ((data.val - 1) * (1080 * 1920 * 3 / 2)); 
                    vo_change_laye0_ping_addr(addr, addr + (1920 * 1080));   // set ping
                    //vo_change_laye0_ping_addr(data.addr[3], data.addr[3] + (1920 * 1080));   // set ping
                    
                }
                else
                {
                    addr = plat->ds0_des_addr + ((plat->ds0_buf_cut - 1) * (1080 * 1920 * 3 / 2)); 
                    vo_change_laye0_ping_addr(addr, addr + (1920 * 1080));   // set ping
                   // vo_change_laye0_ping_addr(data->addr[data->val - 1], data.addr[data.val - 1] + (1080 * 1920));   // set ping  
                }
                
            }
            else
            {
                if(data.val - 1 == 0)
                {

                    addr = plat->ds0_des_addr + ((data.val - 1)  * (1080 * 1920 * 3 / 2)); 
                    vo_change_laye0_ping_addr(addr, addr + (1920 * 1080));   // set ping
                    //vo_change_laye0_pang_addr(data.addr[3], data.addr[3] + (1080 * 1920));   // set ping
                }
                else
                {
                    addr = plat->ds0_des_addr + ((plat->ds0_buf_cut - 1) * (1080 * 1920 * 3 / 2)); 
                    vo_change_laye0_ping_addr(addr, addr + (1920 * 1080));   // set ping
                    //vo_change_laye0_pang_addr(data->addr[data.val - 1], data.addr[data.val - 1] + (1080 * 1920));   // set ping  
                }
            }
//            printk("%x %d %d\n", data.addr, data.val, ret);
        }
#endif

#if 0
        // ds1 for h264 test 
        if(vo_cut == 0)
            vo_change_laye2_ping_addr(plat->ds_data->addr, plat->ds_data->addr + fram_uv_addr);   // set ping
        else
            vo_change_laye2_pang_addr(plat->ds_data->addr, plat->ds_data->addr + fram_uv_addr);   // set ping
#endif
                
        vo_cut ++ ;    

#if 0
        vo_flag ++ ;
        if(vo_flag + vo_i == 120)
        {
            vo_i ++;
            if(vo_i == 4)
                vo_i = 0;

            vo_flag = 0;
            printk("vo start intr data addr is %x  val is %d \n", data.addr, data.val);
        }

#endif           
    }
    return IRQ_HANDLED;
}


static int isp_read_reg(unsigned long arg)
{
        struct isp_reg reg;
        

        if (copy_from_user(&reg, (struct isp_reg *)arg, sizeof(struct isp_reg))) {
                printk("isp: read_reg: copy_from_user failed \n");                                                                
                return -EFAULT;
        }

        if (reg.id % 4) {
                printk("isp read_reg: Unaligned register access: 0x%.4X\n",
                        reg.id);
                return -EINVAL;
        }
        if (reg.id > plat->regs_size) {
                printk("isp read_reg: Out-of-range register read: 0x%.4X\n",
                        reg.id);
                return -EINVAL;
        }

        reg.value = ioread32(plat->regs + reg.id);

        if (copy_to_user((struct isp_reg *)arg, &reg, sizeof(struct isp_reg))) {
                printk("isp: read_reg: copy_to_user failed \n");                                                           
                return -EFAULT;
        }

        //printk("Reg read: 0x%.4X: 0x%.8x\n", reg.id, reg.value);

        return 0;
}

static int isp_write_reg(unsigned long arg)
{
        struct isp_reg reg;
        
//        printk("----------------------------------------------- \n");
        if (copy_from_user(&reg, (struct isp_reg *)arg, sizeof(struct isp_reg)))
                return -EFAULT;

//        printk("arg is id is 0x%x val is 0x%x\n", reg.id, reg.value);

        if (reg.id % 4) {
                printk("isp read_reg: Unaligned register access: 0x%.4X\n",
                        reg.id);
                return -EINVAL;
        }
        if (reg.id > plat->regs_size) {
                printk("isp read_reg: Out-of-range register read: 0x%.4X\n",
                        reg.id);
                return -EINVAL;
        }

        iowrite32(reg.value, plat->regs + reg.id);

        if (copy_to_user((struct isp_reg *)arg, &reg, sizeof(struct isp_reg)))
                return -EFAULT;

        return 0;
}

static int isp_act_sensor_rst(unsigned long arg)
{
#if 0
    unsigned int data = ioread32(plat->gpio);                           //     gpio0               active  csi0 imx385 powerdown    0
    printk("isp_act_sensor_rst read data is %x \n", data);
//    data &= ~1;
    data = (0 << 0) & data;
    printk("isp_act_sensor_rst write data is %x \n", data);
    iowrite32( data ,plat->gpio);

    printk("isp_act_sensor_rst success \n");
#else
//    gpiod_set_value(plat->imx385_powerdown, 0);
    printk("isp_act_sensor_rst success \n");

#endif
    return 0;
}

static int isp_rls_sensor_rst(unsigned long arg)
{ 
#if 0
    unsigned int data = ioread32(plat->gpio);                           //    csi0 imx385 reset     0
    printk("isp_rls_sensor_rst read data is %x \n", data);
    data |= 1;
//    data = (0 << 1) | data;
    printk("isp_rls_sensor_rst write data is %x \n", data);
    iowrite32( data ,plat->gpio);
    printk("isp_rls_sensor_rst success \n");
#else
//    gpiod_set_value(plat->imx385_powerdown, 1);
#endif
    return 0;
}

//dvp
static int dvp_act_sensor_rst(unsigned long arg)
{
#if 0
    unsigned int data = ioread32(plat->gpio);                           //     gpio4               active  csi0 imx385 powerdown    0
    printk("dvp_act_sensor_rst read data is %x \n", data);
//    data &= ~1;
    data = (0 << 4) & data;
    printk("dvp_act_sensor_rst write data is %x \n", data);
    iowrite32( data ,plat->gpio);

    printk("dvp_act_sensor_rst success \n");
#else
//    gpiod_set_value(plat->dvp_powerdown, 0);

    printk("dvp_act_sensor_rst success \n");
#endif
    return 0;
}

//dvp
static int dvp_rls_sensor_rst(unsigned long arg)
{
#if 0
    unsigned int data = ioread32(plat->gpio);                           //     gpio4               active  csi0 imx385 powerdown    0
    printk("dvp_rls_sensor_rst read data is %x \n", data);
//    data &= ~1;
    data = (1 << 4) | data;
    printk("dvp_rls_sensor_rst write data is %x \n", data);
    iowrite32( data ,plat->gpio);

    printk("dvp_rls_sensor_rst success \n");
#else
//    gpiod_set_value(plat->dvp_powerdown, 1);

    printk("dvp_rls_sensor_rst success \n");
#endif
    return 0;
}

static int ds2_buf_init(unsigned long arg)
{
    if (copy_from_user(&ds2_buf[0], (unsigned int *)arg, sizeof(ds2_buf)))
                return -EFAULT;
	int i = 0;
    for(i =0;i<DS2_BUF_NUM_MAX;i++)
        printk("ds2_buf 0x%x\n",ds2_buf[i]);
    return 0;
}

static int ds2_get_buf(unsigned long arg)
{
    unsigned int i  = 0;
    i = ds2_buf_num;
    if(ds2_buf_num >=DS2_BUF_NUM_MAX)
        i = ds2_buf_num -1;

    if (copy_to_user((unsigned int*)arg, &i, sizeof(unsigned int))) 
    {
          printk("isp: read_reg: copy_to_user failed \n");                                                           
          return -EFAULT;
    }
    return 0;
}
/**
 * @brief 
 * 
 * @param arg 
 * @return int 
 */
static int ds2_en_int(unsigned long arg)
{
    unsigned int int_enable =0;
    int_enable = 0xffff;

    return 0;
}
#include <drm/drm_gem_cma_helper.h>

static int isp_set_dmabuf_y_fd(unsigned long arg)
{
    int fd;
	struct dma_buf_attachment *attachment;
	struct sg_table *table;
    struct drm_gem_cma_object *cma_obj;
    struct isp_dmabuf dma;

	unsigned int reg_addr, reg_size;
    if (copy_from_user(&dma, (struct isp_dmabuf *)arg, sizeof(struct isp_dmabuf))) {
        
        printk("isp: set dmabuf: copy_from_user failed \n");                                                                
        return -EFAULT;
    }   

    printk("isp_set_dmabuf_y_fd fd %d\n", dma.fd);

	dmabuf_y = dma_buf_get(dma.fd);
    printk("get dmabuf_y 0x%lx\n", dmabuf_y);
    if(dmabuf_y == NULL)
        return -1;
	//dma_buf_put(dmabuf_y);
	
	cma_obj = (struct drm_gem_cma_object *)dmabuf_y->priv;
    dma.dma_addr = cma_obj->paddr;
	printk("dmabuf_y addr 0x%lx, size %d\n", cma_obj->paddr,dmabuf_y->size);

    if (copy_to_user((struct isp_dmabuf *)arg, &dma, sizeof(struct isp_dmabuf))) {
        printk("isp: set dmabuf: copy_to_user failed \n");                                                           
        return -EFAULT;
    }
#if 0
	attachment = dma_buf_attach(dmabuf_y, device);
    printk("dma buf attach dev 0x%lx, attachment 0x%lx\n", device, attachment);
	table = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);
    if(table < 0)
    {
        printk("dma_buf_map_attachment failed, err %d\n", table);
        return -1;
    }
    printk("dma_buf_map_attachment, table 0x%lx\n", table);

	reg_addr = sg_dma_address(table->sgl);
	reg_size = sg_dma_len(table->sgl);
	pr_info("dma y buf, fd %d, reg_addr = 0x%08x, reg_size = 0x%08x\n",fd, reg_addr, reg_size);
#endif

    return 0;
}

static int isp_set_dmabuf_uv_fd(unsigned long arg)
{
    int fd;
	struct dma_buf_attachment *attachment;
	struct sg_table *table;
    unsigned int reg_addr, reg_size;
        
    if (copy_from_user(&fd, (void __user *)arg, sizeof(int)))
            return -EFAULT;

	dmabuf_uv = dma_buf_get(fd);

	//dma_buf_put(dmabuf_y);
	attachment = dma_buf_attach(dmabuf_uv, device);
	table = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);

	reg_addr = sg_dma_address(table->sgl);
	reg_size = sg_dma_len(table->sgl);
	pr_info("dma uv buf,fd %d reg_addr = 0x%08x, reg_size = 0x%08x\n", fd, reg_addr, reg_size);

    return 0;
}


static int setup_chrdev_region(void)
{
        dev_t dev = 0;
        int err;

        if (plat->major == 0) {
                err = alloc_chrdev_region(&dev, 0, 1, "test_isp");
                plat->major = MAJOR(dev);

                if (err) {
                        printk("test-isp: can't get major %d\n", plat->major);
                        return err;
                }
        }
        return 0;
}


static int create_module_class(void)
{
        plat->class = class_create(THIS_MODULE, "isp_class");

        if (IS_ERR(plat->class))
                return PTR_ERR(plat->class);

        return 0;
}

void clean_up_isp_cdev(void)
{
        cdev_del(&plat->cdev);
}


static int isp_open(struct inode *inode, struct file *filp)
{
        return 0;
}

static int isp_release(struct inode *inode, struct file *filp)
{
		ds_set_addr_flag = 0;
        return 0;
}

/**
 * @brief :mipi csi
 * 
 * @param speed 
 * @param chcfg 
 */
static void mipi_rx_dphy_init(rxdphy_speed_mode_t speed,rxdphy_chcfg_t chcfg)
{
    unsigned int rdata, wdata = 0x0003a002;
    unsigned int dllrange = 208;
    uint32_t *addr = ioremap(VIDEO_RXDPHY_BASE, 0x08);

    //printk("------------------------------------------------------------------------ \n");
    //enable video apb clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_CSI_0_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_CSI_1_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_CSI_2_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_CSI_3_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_F2K_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_R2K_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_MFBC_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_VI_APB, 1);
    
    //enable video axi clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_VI_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_ISP_F2K_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_ISP_R2K_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_ISP_TOF_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_MFBC_AXI, 1);   

    //enable sys clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_CSI_0_SYSTEM, 1); 
    sysctl_clk_set_leaf_en(SYSCTL_CLK_CSI_1_SYSTEM, 1);

    //enable pixel clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_CSI_0_PIXEL, 1); 
    sysctl_clk_set_leaf_en(SYSCTL_CLK_CSI_1_PIXEL, 1); 
    sysctl_clk_set_leaf_en(SYSCTL_CLK_TPG_PIXEL, 1); 

    //enable rxdphy ref clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_MIPI_REF_RXPHY_REF, 1);

    //init mipi corner
//    mipi_coner_init();

    if(chcfg == RXDPHY_CHCFG_1X4)
        wdata |= 1; //one 4x1 mode

    if(speed == RXDPHY_SPEED_MODE_2500M)
    {
        wdata |= 1 << 2; //enable deskew
        dllrange = 124;
    }

    //wdata=0x0003a003;
    writel(wdata, addr);
    writel((dllrange << 7) | (dllrange << 15), addr + 4);

    iounmap(addr);

}
/********************
 * mipi dsi .........
 * 
 * ***************/
static void mipi_tx_dphy_5M_init(void)
{
    uint32_t rdata, wdata;
    uint32_t *addr = ioremap(VIDEO_TXDPHY_BASE_ADDR, 0x08);
    uint32_t *addr2 = ioremap(VIDEO_TXDPHY_BASE_ADDR + 0xc, 0x08);

    //enable display apb clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_DSI_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_VO_APB, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_TWOD_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_BT1120_APB, 1);

    //enable display axi clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_VO_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_MFBC_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);
    

    //enable display pix clk, default 148.5MHz
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISPLAY_PIXEL, 1);

    //enale dsi system clock
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_DSI_SYSTEM, 1);

    //init corner
    mipi_coner_init();

//    sysctl_boot_set_root_clk_freq(SYSCTL_CLK_ROOT_PLL2, 593, 24, 0);

    //enable txdphy rclk and ref clk            
    sysctl_clk_set_leaf_div(SYSCTL_CLK_MIPI_REF,1, 8);//16); // 148.5 

//    printk("\n\n SYSCTL_CLK_MIPI_REF out clk freq is %d \r\n\n",sysctl_clk_get_leaf_freq(SYSCTL_CLK_MIPI_REF));
    
    sysctl_clk_set_leaf_en(SYSCTL_CLK_MIPI_REF_TXPHY_REF, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_MIPI_REF_TXPHY_PLL, 1);
    
    //de-assert dphy standby
    wdata        = 1 << 14;
    writel(wdata, addr);


    //init pll, default 891M
    //mipi_txdphy_set_pll(TXDPHY_PLL_891M_F, TXDPHY_PLL_891M_R, TXDPHY_PLL_891M_D);
    mipi_txdphy_set_pll(TXDPHY_PLL_445M_F, TXDPHY_PLL_445M_R, TXDPHY_PLL_445M_D);       // p30

    //default timing settings
    wdata = 0x0f180080;                //0x0f180080       0x0f1be080    
    writel(wdata, addr2);

    iounmap(addr);
    iounmap(addr2);
}

static void mipi_dsi_reset_dphy(void)
{
        uint32_t wdata;

        uint32_t *addr = ioremap(CNDS_DSI_REG_START + VIDEO_TXDPHY_BASE_ADDR + 0x8, 0x08);
        uint32_t *addr2 = ioremap(VIDEO_TXDPHY_BASE_ADDR + 0x8, 0x08);
        uint32_t *addr3 = ioremap(VIDEO_TXDPHY_BASE_ADDR + 0x4, 0x08);

        mdelay(50);                            // 500
        writel(0xf0600, addr);                                   // dsi reset dphy

        wdata = 0xda036000;                                     // set d-phy
        writel(wdata, addr2);

        wdata = 0x3b01e;                                        // set d-phy
        writel(wdata, addr3);

        wdata = 0x9a036000;                                     // set d-phy
        writel(wdata, addr2);                                   //0x92718000

        mdelay(50);                            //500
        writel(0x1f0600, addr);                                 // dsi reset dphy 

        iounmap(addr);
        iounmap(addr2);   
}


static void DsiRegWr(int addr,int data)
{
      uint32_t *v_addr = ioremap(CNDS_DSI_REG_START + addr, 0x08);

      writel(data, v_addr);

      iounmap(v_addr);
}

static int  DsiRegRd(int addr)
{      
        int val ;
        uint32_t *v_addr = ioremap(CNDS_DSI_REG_START + addr, 0x08);
        val = readl(v_addr);
        iounmap(v_addr);

        return val;
}

static void wait_phy_pll_locked(void)
{
    while (1)
    {
        if ((DsiRegRd(MCTL_MAIN_STS_OFFSET) & 0x01) == 0x01)
        {     
                break;
        }
    }
}

static void wait_dat1_rdy(void)
{
    while (1)
    {
        if ((DsiRegRd(MCTL_MAIN_STS_OFFSET) & 0x04) == 0x04)
        {     
                break;
        }
    }
}

static void wait_dat2_rdy(void)
 {
    while (1)
    {
        if ((DsiRegRd(MCTL_MAIN_STS_OFFSET) & 0x08) == 0x08)
        {     
                break;
        }
    }
 }

 static void wait_dat3_rdy(void)
 {
    while (1)
    {
        if ((DsiRegRd(MCTL_MAIN_STS_OFFSET) & 0x10) == 0x10)
        {     
                break;
        }
    }
 }

 static void wait_dat4_rdy(void)
 {
     while (1)
	 {
         if ((DsiRegRd(MCTL_MAIN_STS_OFFSET) & 0x20) == 0x20)
          {     break;}
    }
 }

static void wait_cklane_rdy(void)
{
    while (1)
    {
        if ((DsiRegRd(MCTL_MAIN_STS_OFFSET) & 0x02) == 0x02)
        {     
                break;
        }
    }
 }


static void  dsc_cmd_send(int par, int data1, int data2)
{
        int cmd_sts =0;
        if(par == 0)
        {
//                DsiRegWr(DIRECT_CMD_STS_CTL_OFFSET,0x02) //[1]enable  write_completed_en
                DsiRegWr(DIRECT_CMD_MAIN_SETTINGS_OFFSET,0x01020500); //[24] cmd_lp_en  =1
                                                                                //[23:14] cmd_size  =cmd_size. 1 or 2           2
                                                                                //[13:8] cmd_head  =0x39
                                                                                //[3 ] cmd_longnotshort  =0x1
                                                                                //[2:0] cmd_nat : 3'b000:write command
                                                                                //                3'b001:read command
                                                                                //                3'b100:TE request
                                                                                //                3'b101:trigger request
                                                                                //                3'b110:BTA request
                DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,data1);// DATA
                DsiRegWr(DIRECT_CMD_SEND_OFFSET,0x00000); // cmd send
                while((cmd_sts & 0x02 )!= 0x02)
                {
                        cmd_sts = DsiRegRd(DIRECT_CMD_STS_OFFSET);
                }
                DsiRegWr(DIRECT_CMD_STS_CLR_OFFSET,0x02); // clear write_completed
         }
        else  if(par == 1)
        {
                //DsiRegWr(DIRECT_CMD_STS_CTL_OFFSET,0x02) //[1]enable  write_completed_en
                DsiRegWr(DIRECT_CMD_MAIN_SETTINGS_OFFSET,0x01011500); //[24] cmd_lp_en  =1
                                                                        //[23:14] cmd_size  =cmd_size. 1 or 2
                                                                        //[13:8] cmd_head  =0x39
                                                                        //[3 ] cmd_longnotshort  =0x1
                                                                        //[2:0] cmd_nat : 3'b000:write command
                                                                        //                3'b001:read command
                                                                        //                3'b100:TE request
                                                                        //                3'b101:trigger request
                                                                        //                3'b110:BTA request
                DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,((data2<<8)+data1));// DATA
                DsiRegWr(DIRECT_CMD_SEND_OFFSET,0x00000); // cmd send
                while((cmd_sts & 0x02) != 0x02)
                { 
                        cmd_sts = DsiRegRd(DIRECT_CMD_STS_OFFSET);
                }
                DsiRegWr(DIRECT_CMD_STS_CLR_OFFSET,0x02); // clear write_completed
        }
        else if(par == 2)
        {
                DsiRegWr(DIRECT_CMD_MAIN_SETTINGS_OFFSET,0x01023908); //[24] cmd_lp_en  =1              0x01023908
                                                                        //[23:14] cmd_size  =cmd_size. 1 or 2
                                                                        //[13:8] cmd_head  =0x39
                                                                        //[3 ] cmd_longnotshort  =0x1
                                                                        //[2:0] cmd_nat : 3'b000:write command
                                                                        //                3'b001:read command
                                                                        //                3'b100:TE request
                                                                        //                3'b101:trigger request
                                                                        //                3'b110:BTA request
                DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,((data2<<8)+data1));// DATA
                DsiRegWr(DIRECT_CMD_SEND_OFFSET,0x00000); // cmd send
                while((cmd_sts & 0x02 )!= 0x02)
                {  
                        cmd_sts = DsiRegRd(DIRECT_CMD_STS_OFFSET);              
                }
                DsiRegWr(DIRECT_CMD_STS_CLR_OFFSET,0x02); // clear write_completed

        }

}

void dsc_cmd_read(int addr)
{
        int cmd_sts =0;
        int data = 0;
        int size;
        int err;
        int reg = 0;

        DsiRegWr(DIRECT_CMD_STS_CLR_OFFSET,0xffffffff);
        DsiRegWr(DIRECT_CMD_RD_STS_CLR_OFFSET, 0xffffffff);

        DsiRegWr(DIRECT_CMD_MAIN_SETTINGS_OFFSET,0x01010601);           //[24] cmd_lp_en  =1            0x01010601        
                                                                        //[23:16] cmd_size  =cmd_size. 1 or 2                   
                                                                        //[14:15] cmd_type  0 write  1 read                     1           
                                                                        //[13:8] cmd_head  =0x5                                 6            
                                                                        //[3 ] cmd_longnotshort  =0x1                           1
                                                                        //[2:0] cmd_nat : 3'b000:write command                  01
                                                                        //                3'b001:read command
                                                                        //                3'b100:TE request
                                                                        //                3'b101:trigger request
                                                                        //                3'b110:BTA request   
        
        DsiRegWr(DIRECT_CMD_WRDAT_OFFSET, addr);

        DsiRegWr(DIRECT_CMD_SEND_OFFSET,0xffffffff); // cmd send   


        while((cmd_sts & 0x08 )!= 0x08)                                 //wait read success
        {  
                cmd_sts = DsiRegRd(DIRECT_CMD_STS_OFFSET);              
        }

        err = DsiRegRd(DIRECT_CMD_RD_STS_OFFSET);                   //read err
        size = DsiRegRd(DIRECT_CMD_RD_PROPERTY_OFFSET);             // read size [16 - 23]   
        data = DsiRegRd(DIRECT_CMD_RDDAT_OFFSET);                   //read data ;

        printk("err is 0x%8x, size is 0x%8x, data is 0x%8x \n", err, size, data);

}
/**
 * @brief 
 * 
 */
static void  aml_lcd_init(void)
{
        DsiRegWr(DIRECT_CMD_STS_CTL_OFFSET,0);//0x02); //[1]enable  write_completed_en
//        printk("aml_lcd_init 1\n");

        dsc_cmd_send(2,0xFF,0x01);     

//        dsc_cmd_read(0x0d);  

//        printk("lcd init 2\n");
        
        dsc_cmd_send(2,0xFB,0x01);
//        printk("lcd init 3\n");

        dsc_cmd_send(2,0xFF,0x02);
//        printk("lcd init 4\n");

        dsc_cmd_send(2,0xFB,0x01);
//        printk("lcd init 5\n");

        
        dsc_cmd_send(2,0xFF,0x03);

//        printk("lcd init 6\n");
        dsc_cmd_send(2,0xFB,0x01);
//        printk("lcd init 7\n");

        dsc_cmd_send(2,0xFF,0x04);
        dsc_cmd_send(2,0xFB,0x01);
        dsc_cmd_send(2,0xFF,0x05);
        dsc_cmd_send(2,0xFB,0x01);
        dsc_cmd_send(2,0xD7,0x31);
        dsc_cmd_send(2,0xD8,0x7E);
        //Delay(100);
#ifndef _SIMU        
        mdelay(100);
#endif
        dsc_cmd_send(2,0xFF,0x00);
        dsc_cmd_send(2,0xFB,0x01);
        dsc_cmd_send(2,0xBA,0x03);
        dsc_cmd_send(2,0x36,0x00);
        dsc_cmd_send(2,0xB0,0x00);
        dsc_cmd_send(2,0xD3,0x08);
        dsc_cmd_send(2,0xD4,0x0E);
        dsc_cmd_send(2,0xD5,0x0F);
        dsc_cmd_send(2,0xD6,0x48);
        dsc_cmd_send(2,0xD7,0x00);
        dsc_cmd_send(2,0xD9,0x00);
        dsc_cmd_send(2,0xFB,0x01);
        dsc_cmd_send(2,0xFF,0xEE);
        dsc_cmd_send(2,0x40,0x00);
        dsc_cmd_send(2,0x41,0x00);
        dsc_cmd_send(2,0x42,0x00);
        dsc_cmd_send(2,0xFB,0x01);
        dsc_cmd_send(2,0xFF,0x01);
        dsc_cmd_send(2,0xFB,0x01);
        dsc_cmd_send(2,0x01,0x55);
        dsc_cmd_send(2,0x04,0x0C);
        dsc_cmd_send(2,0x05,0x3A);
        dsc_cmd_send(2,0x06,0x50);
        dsc_cmd_send(2,0x07,0xD0);
        dsc_cmd_send(2,0x0A,0x0F);
        dsc_cmd_send(2,0x0C,0x06);
        dsc_cmd_send(2,0x0D,0x7F);
        dsc_cmd_send(2,0x0E,0x7F);
        dsc_cmd_send(2,0x0F,0x70);
        dsc_cmd_send(2,0x10,0x63);
        dsc_cmd_send(2,0x11,0x3C);
        dsc_cmd_send(2,0x12,0x5C);
        dsc_cmd_send(2,0x13,0x5A);
        dsc_cmd_send(2,0x14,0x5A);
        dsc_cmd_send(2,0x15,0x60);
        dsc_cmd_send(2,0x16,0x15);
        dsc_cmd_send(2,0x17,0x15);
        dsc_cmd_send(2,0x23,0x00);
        dsc_cmd_send(2,0x24,0x00);
        dsc_cmd_send(2,0x25,0x00);
        dsc_cmd_send(2,0x26,0x00);
        dsc_cmd_send(2,0x27,0x00);
        dsc_cmd_send(2,0x28,0x00);
        dsc_cmd_send(2,0x44,0x00);
        dsc_cmd_send(2,0x45,0x00);
        dsc_cmd_send(2,0x46,0x00);
        dsc_cmd_send(2,0x5B,0xCA);
        dsc_cmd_send(2,0x5C,0x00);
        dsc_cmd_send(2,0x5D,0x00);
        dsc_cmd_send(2,0x5E,0x2D);
        dsc_cmd_send(2,0x5F,0x1B);
        dsc_cmd_send(2,0x60,0xD5);
        dsc_cmd_send(2,0x61,0xF7);
        dsc_cmd_send(2,0x6C,0xAB);
        dsc_cmd_send(2,0x6D,0x44);
        //dsc_cmd_send(2,0x6E,0x80);
        dsc_cmd_send(2,0xFF,0x05);
        dsc_cmd_send(2,0xFB,0x01);
        dsc_cmd_send(2,0x00,0x3F);
        dsc_cmd_send(2,0x01,0x3F);
        dsc_cmd_send(2,0x02,0x3F);
        dsc_cmd_send(2,0x03,0x3F);
        dsc_cmd_send(2,0x04,0x38);
        dsc_cmd_send(2,0x05,0x3F);
        dsc_cmd_send(2,0x06,0x3F);
        dsc_cmd_send(2,0x07,0x19);
        dsc_cmd_send(2,0x08,0x1D);
        dsc_cmd_send(2,0x09,0x3F);
        dsc_cmd_send(2,0x0A,0x3F);
        dsc_cmd_send(2,0x0B,0x1B);
        dsc_cmd_send(2,0x0C,0x17);
        dsc_cmd_send(2,0x0D,0x3F);
        dsc_cmd_send(2,0x0E,0x3F);
        dsc_cmd_send(2,0x0F,0x08);
        dsc_cmd_send(2,0x10,0x3F);
        dsc_cmd_send(2,0x11,0x10);
        dsc_cmd_send(2,0x12,0x3F);
        dsc_cmd_send(2,0x13,0x3F);
        dsc_cmd_send(2,0x14,0x3F);
        dsc_cmd_send(2,0x15,0x3F);
        dsc_cmd_send(2,0x16,0x3F);
        dsc_cmd_send(2,0x17,0x3F);
        dsc_cmd_send(2,0x18,0x38);
        dsc_cmd_send(2,0x19,0x18);
        dsc_cmd_send(2,0x1A,0x1C);
        dsc_cmd_send(2,0x1B,0x3F);
        dsc_cmd_send(2,0x1C,0x3F);
        dsc_cmd_send(2,0x1D,0x1A);
        dsc_cmd_send(2,0x1E,0x16);
        dsc_cmd_send(2,0x1F,0x3F);
        dsc_cmd_send(2,0x20,0x3F);
        dsc_cmd_send(2,0x21,0x3F);
        dsc_cmd_send(2,0x22,0x3F);
        dsc_cmd_send(2,0x23,0x06);
        dsc_cmd_send(2,0x24,0x3F);
        dsc_cmd_send(2,0x25,0x0E);
        dsc_cmd_send(2,0x26,0x3F);
        dsc_cmd_send(2,0x27,0x3F);
        dsc_cmd_send(2,0x54,0x06);
        dsc_cmd_send(2,0x55,0x05);
        dsc_cmd_send(2,0x56,0x04);
        dsc_cmd_send(2,0x58,0x03);
        dsc_cmd_send(2,0x59,0x1B);
        dsc_cmd_send(2,0x5A,0x1B);
        dsc_cmd_send(2,0x5B,0x01);
        dsc_cmd_send(2,0x5C,0x32);
        dsc_cmd_send(2,0x5E,0x18);
        dsc_cmd_send(2,0x5F,0x20);
        dsc_cmd_send(2,0x60,0x2B);
        dsc_cmd_send(2,0x61,0x2C);
        dsc_cmd_send(2,0x62,0x18);
        dsc_cmd_send(2,0x63,0x01);
        dsc_cmd_send(2,0x64,0x32);
        dsc_cmd_send(2,0x65,0x00);
        dsc_cmd_send(2,0x66,0x44);
        dsc_cmd_send(2,0x67,0x11);
        dsc_cmd_send(2,0x68,0x01);
        dsc_cmd_send(2,0x69,0x01);
        dsc_cmd_send(2,0x6A,0x04);
        dsc_cmd_send(2,0x6B,0x2C);
        dsc_cmd_send(2,0x6C,0x08);
        dsc_cmd_send(2,0x6D,0x08);
        dsc_cmd_send(2,0x78,0x00);
        dsc_cmd_send(2,0x79,0x00);
        dsc_cmd_send(2,0x7E,0x00);
        dsc_cmd_send(2,0x7F,0x00);
        dsc_cmd_send(2,0x80,0x00);
        dsc_cmd_send(2,0x81,0x00);
        dsc_cmd_send(2,0x8D,0x00);
        dsc_cmd_send(2,0x8E,0x00);
        dsc_cmd_send(2,0x8F,0xC0);
        dsc_cmd_send(2,0x90,0x73);
        dsc_cmd_send(2,0x91,0x10);
        dsc_cmd_send(2,0x92,0x07);
        dsc_cmd_send(2,0x96,0x11);
        dsc_cmd_send(2,0x97,0x14);
        dsc_cmd_send(2,0x98,0x00);
        dsc_cmd_send(2,0x99,0x00);
        dsc_cmd_send(2,0x9A,0x00);
        dsc_cmd_send(2,0x9B,0x61);
        dsc_cmd_send(2,0x9C,0x15);
        dsc_cmd_send(2,0x9D,0x30);
        dsc_cmd_send(2,0x9F,0x0F);
        dsc_cmd_send(2,0xA2,0xB0);
        dsc_cmd_send(2,0xA7,0x0A);
        dsc_cmd_send(2,0xA9,0x00);
        dsc_cmd_send(2,0xAA,0x70);
        dsc_cmd_send(2,0xAB,0xDA);
        dsc_cmd_send(2,0xAC,0xFF);
        dsc_cmd_send(2,0xAE,0xF4);
        dsc_cmd_send(2,0xAF,0x40);
        dsc_cmd_send(2,0xB0,0x7F);
        dsc_cmd_send(2,0xB1,0x16);
        dsc_cmd_send(2,0xB2,0x53);
        dsc_cmd_send(2,0xB3,0x00);
        dsc_cmd_send(2,0xB4,0x2A);
        dsc_cmd_send(2,0xB5,0x3A);
        dsc_cmd_send(2,0xB6,0xF0);
        dsc_cmd_send(2,0xBC,0x85);
        dsc_cmd_send(2,0xBD,0xF4);
        dsc_cmd_send(2,0xBE,0x33);
        dsc_cmd_send(2,0xBF,0x13);
        dsc_cmd_send(2,0xC0,0x77);
        dsc_cmd_send(2,0xC1,0x77);
        dsc_cmd_send(2,0xC2,0x77);
        dsc_cmd_send(2,0xC3,0x77);
        dsc_cmd_send(2,0xC4,0x77);
        dsc_cmd_send(2,0xC5,0x77);
        dsc_cmd_send(2,0xC6,0x77);
        dsc_cmd_send(2,0xC7,0x77);
        dsc_cmd_send(2,0xC8,0xAA);
        dsc_cmd_send(2,0xC9,0x2A);
        dsc_cmd_send(2,0xCA,0x00);
        dsc_cmd_send(2,0xCB,0xAA);
        dsc_cmd_send(2,0xCC,0x92);
        dsc_cmd_send(2,0xCD,0x00);
        dsc_cmd_send(2,0xCE,0x18);
        dsc_cmd_send(2,0xCF,0x88);
        dsc_cmd_send(2,0xD0,0xAA);
        dsc_cmd_send(2,0xD1,0x00);
        dsc_cmd_send(2,0xD2,0x00);
        dsc_cmd_send(2,0xD3,0x00);
        dsc_cmd_send(2,0xD6,0x02);
        dsc_cmd_send(2,0xED,0x00);
        dsc_cmd_send(2,0xEE,0x00);
        dsc_cmd_send(2,0xEF,0x70);
        dsc_cmd_send(2,0xFA,0x03);
        dsc_cmd_send(2,0xFF,0x00);

        ////page selection cmd start
        dsc_cmd_send(2,0xFF,0x01);
        dsc_cmd_send(2,0xFB,0x01);
        ////page selection cmd end
        ////R(+) MCR cmd
        dsc_cmd_send(2,0x75,0x00);
        dsc_cmd_send(2,0x76,0x00);
        dsc_cmd_send(2,0x77,0x00);
        dsc_cmd_send(2,0x78,0x2C);
        dsc_cmd_send(2,0x79,0x00);
        dsc_cmd_send(2,0x7A,0x4F);
        dsc_cmd_send(2,0x7B,0x00);
        dsc_cmd_send(2,0x7C,0x69);
        dsc_cmd_send(2,0x7D,0x00);
        dsc_cmd_send(2,0x7E,0x7F);
        dsc_cmd_send(2,0x7F,0x00);
        dsc_cmd_send(2,0x80,0x92);
        dsc_cmd_send(2,0x81,0x00);
        dsc_cmd_send(2,0x82,0xA3);
        dsc_cmd_send(2,0x83,0x00);
        dsc_cmd_send(2,0x84,0xB3);
        dsc_cmd_send(2,0x85,0x00);
        dsc_cmd_send(2,0x86,0xC1);
        dsc_cmd_send(2,0x87,0x00);
        dsc_cmd_send(2,0x88,0xF3);
        dsc_cmd_send(2,0x89,0x01);
        dsc_cmd_send(2,0x8A,0x1B);
        dsc_cmd_send(2,0x8B,0x01);
        dsc_cmd_send(2,0x8C,0x5A);
        dsc_cmd_send(2,0x8D,0x01);
        dsc_cmd_send(2,0x8E,0x8B);
        dsc_cmd_send(2,0x8F,0x01);
        dsc_cmd_send(2,0x90,0xD9);
        dsc_cmd_send(2,0x91,0x02);
        dsc_cmd_send(2,0x92,0x16);
        dsc_cmd_send(2,0x93,0x02);
        dsc_cmd_send(2,0x94,0x18);
        dsc_cmd_send(2,0x95,0x02);
        dsc_cmd_send(2,0x96,0x4E);
        dsc_cmd_send(2,0x97,0x02);
        dsc_cmd_send(2,0x98,0x88);
        dsc_cmd_send(2,0x99,0x02);
        dsc_cmd_send(2,0x9A,0xAC);
        dsc_cmd_send(2,0x9B,0x02);
        dsc_cmd_send(2,0x9C,0xDD);
        dsc_cmd_send(2,0x9D,0x03);
        dsc_cmd_send(2,0x9E,0x01);
        dsc_cmd_send(2,0x9F,0x03);
        dsc_cmd_send(2,0xA0,0x2E);
        dsc_cmd_send(2,0xA2,0x03);
        dsc_cmd_send(2,0xA3,0x3C);
        dsc_cmd_send(2,0xA4,0x03);
        dsc_cmd_send(2,0xA5,0x4C);
        dsc_cmd_send(2,0xA6,0x03);
        dsc_cmd_send(2,0xA7,0x5D);
        dsc_cmd_send(2,0xA9,0x03);
        dsc_cmd_send(2,0xAA,0x70);
        dsc_cmd_send(2,0xAB,0x03);
        dsc_cmd_send(2,0xAC,0x88);
        dsc_cmd_send(2,0xAD,0x03);
        dsc_cmd_send(2,0xAE,0xA8);
        dsc_cmd_send(2,0xAF,0x03);
        dsc_cmd_send(2,0xB0,0xC8);
        dsc_cmd_send(2,0xB1,0x03);
        dsc_cmd_send(2,0xB2,0xFF);
        ////R(-) MCR cmd
        dsc_cmd_send(2,0xB3,0x00);
        dsc_cmd_send(2,0xB4,0x00);
        dsc_cmd_send(2,0xB5,0x00);
        dsc_cmd_send(2,0xB6,0x2C);
        dsc_cmd_send(2,0xB7,0x00);
        dsc_cmd_send(2,0xB8,0x4F);
        dsc_cmd_send(2,0xB9,0x00);
        dsc_cmd_send(2,0xBA,0x69);
        dsc_cmd_send(2,0xBB,0x00);
        dsc_cmd_send(2,0xBC,0x7F);
        dsc_cmd_send(2,0xBD,0x00);
        dsc_cmd_send(2,0xBE,0x92);
        dsc_cmd_send(2,0xBF,0x00);
        dsc_cmd_send(2,0xC0,0xA3);
        dsc_cmd_send(2,0xC1,0x00);
        dsc_cmd_send(2,0xC2,0xB3);
        dsc_cmd_send(2,0xC3,0x00);
        dsc_cmd_send(2,0xC4,0xC1);
        dsc_cmd_send(2,0xC5,0x00);
        dsc_cmd_send(2,0xC6,0xF3);
        dsc_cmd_send(2,0xC7,0x01);
        dsc_cmd_send(2,0xC8,0x1B);
        dsc_cmd_send(2,0xC9,0x01);
        dsc_cmd_send(2,0xCA,0x5A);
        dsc_cmd_send(2,0xCB,0x01);
        dsc_cmd_send(2,0xCC,0x8B);
        dsc_cmd_send(2,0xCD,0x01);
        dsc_cmd_send(2,0xCE,0xD9);
        dsc_cmd_send(2,0xCF,0x02);
        dsc_cmd_send(2,0xD0,0x16);
        dsc_cmd_send(2,0xD1,0x02);
        dsc_cmd_send(2,0xD2,0x18);
        dsc_cmd_send(2,0xD3,0x02);
        dsc_cmd_send(2,0xD4,0x4E);
        dsc_cmd_send(2,0xD5,0x02);
        dsc_cmd_send(2,0xD6,0x88);
        dsc_cmd_send(2,0xD7,0x02);
        dsc_cmd_send(2,0xD8,0xAC);
        dsc_cmd_send(2,0xD9,0x02);
        dsc_cmd_send(2,0xDA,0xDD);
        dsc_cmd_send(2,0xDB,0x03);
        dsc_cmd_send(2,0xDC,0x01);
        dsc_cmd_send(2,0xDD,0x03);
        dsc_cmd_send(2,0xDE,0x2E);
        dsc_cmd_send(2,0xDF,0x03);
        dsc_cmd_send(2,0xE0,0x3C);
        dsc_cmd_send(2,0xE1,0x03);
        dsc_cmd_send(2,0xE2,0x4C);
        dsc_cmd_send(2,0xE3,0x03);
        dsc_cmd_send(2,0xE4,0x5D);
        dsc_cmd_send(2,0xE5,0x03);
        dsc_cmd_send(2,0xE6,0x70);
        dsc_cmd_send(2,0xE7,0x03);
        dsc_cmd_send(2,0xE8,0x88);
        dsc_cmd_send(2,0xE9,0x03);
        dsc_cmd_send(2,0xEA,0xA8);
        dsc_cmd_send(2,0xEB,0x03);
        dsc_cmd_send(2,0xEC,0xC8);
        dsc_cmd_send(2,0xED,0x03);
        dsc_cmd_send(2,0xEE,0xFF);
        ////G(+) MCR cmd
        dsc_cmd_send(2,0xEF,0x00);
        dsc_cmd_send(2,0xF0,0x00);
        dsc_cmd_send(2,0xF1,0x00);
        dsc_cmd_send(2,0xF2,0x2C);
        dsc_cmd_send(2,0xF3,0x00);
        dsc_cmd_send(2,0xF4,0x4F);
        dsc_cmd_send(2,0xF5,0x00);
        dsc_cmd_send(2,0xF6,0x69);
        dsc_cmd_send(2,0xF7,0x00);
        dsc_cmd_send(2,0xF8,0x7F);
        dsc_cmd_send(2,0xF9,0x00);
        dsc_cmd_send(2,0xFA,0x92);
        ////page selection cmd start
        dsc_cmd_send(2,0xFF,0x02);
        dsc_cmd_send(2,0xFB,0x01);
        ////page selection cmd end
        dsc_cmd_send(2,0x00,0x00);
        dsc_cmd_send(2,0x01,0xA3);
        dsc_cmd_send(2,0x02,0x00);
        dsc_cmd_send(2,0x03,0xB3);
        dsc_cmd_send(2,0x04,0x00);
        dsc_cmd_send(2,0x05,0xC1);
        dsc_cmd_send(2,0x06,0x00);
        dsc_cmd_send(2,0x07,0xF3);
        dsc_cmd_send(2,0x08,0x01);
        dsc_cmd_send(2,0x09,0x1B);
        dsc_cmd_send(2,0x0A,0x01);
        dsc_cmd_send(2,0x0B,0x5A);
        dsc_cmd_send(2,0x0C,0x01);
        dsc_cmd_send(2,0x0D,0x8B);
        dsc_cmd_send(2,0x0E,0x01);
        dsc_cmd_send(2,0x0F,0xD9);
        dsc_cmd_send(2,0x10,0x02);
        dsc_cmd_send(2,0x11,0x16);
        dsc_cmd_send(2,0x12,0x02);
        dsc_cmd_send(2,0x13,0x18);
        dsc_cmd_send(2,0x14,0x02);
        dsc_cmd_send(2,0x15,0x4E);
        dsc_cmd_send(2,0x16,0x02);
        dsc_cmd_send(2,0x17,0x88);
        dsc_cmd_send(2,0x18,0x02);
        dsc_cmd_send(2,0x19,0xAC);
        dsc_cmd_send(2,0x1A,0x02);
        dsc_cmd_send(2,0x1B,0xDD);
        dsc_cmd_send(2,0x1C,0x03);
        dsc_cmd_send(2,0x1D,0x01);
        dsc_cmd_send(2,0x1E,0x03);
        dsc_cmd_send(2,0x1F,0x2E);
        dsc_cmd_send(2,0x20,0x03);
        dsc_cmd_send(2,0x21,0x3C);
        dsc_cmd_send(2,0x22,0x03);
        dsc_cmd_send(2,0x23,0x4C);
        dsc_cmd_send(2,0x24,0x03);
        dsc_cmd_send(2,0x25,0x5D);
        dsc_cmd_send(2,0x26,0x03);
        dsc_cmd_send(2,0x27,0x70);
        dsc_cmd_send(2,0x28,0x03);
        dsc_cmd_send(2,0x29,0x88);
        dsc_cmd_send(2,0x2A,0x03);
        dsc_cmd_send(2,0x2B,0xA8);
        dsc_cmd_send(2,0x2D,0x03);
        dsc_cmd_send(2,0x2F,0xC8);
        dsc_cmd_send(2,0x30,0x03);
        dsc_cmd_send(2,0x31,0xFF);
        ////G(-) MCR cmd
        dsc_cmd_send(2,0x32,0x00);
        dsc_cmd_send(2,0x33,0x00);
        dsc_cmd_send(2,0x34,0x00);
        dsc_cmd_send(2,0x35,0x2C);
        dsc_cmd_send(2,0x36,0x00);
        dsc_cmd_send(2,0x37,0x4F);
        dsc_cmd_send(2,0x38,0x00);
        dsc_cmd_send(2,0x39,0x69);
        dsc_cmd_send(2,0x3A,0x00);
        dsc_cmd_send(2,0x3B,0x7F);
        dsc_cmd_send(2,0x3D,0x00);
        dsc_cmd_send(2,0x3F,0x92);
        dsc_cmd_send(2,0x40,0x00);
        dsc_cmd_send(2,0x41,0xA3);
        dsc_cmd_send(2,0x42,0x00);
        dsc_cmd_send(2,0x43,0xB3);
        dsc_cmd_send(2,0x44,0x00);
        dsc_cmd_send(2,0x45,0xC1);
        dsc_cmd_send(2,0x46,0x00);
        dsc_cmd_send(2,0x47,0xF3);
        dsc_cmd_send(2,0x48,0x01);
        dsc_cmd_send(2,0x49,0x1B);
        dsc_cmd_send(2,0x4A,0x01);
        dsc_cmd_send(2,0x4B,0x5A);
        dsc_cmd_send(2,0x4C,0x01);
        dsc_cmd_send(2,0x4D,0x8B);
        dsc_cmd_send(2,0x4E,0x01);
        dsc_cmd_send(2,0x4F,0xD9);
        dsc_cmd_send(2,0x50,0x02);
        dsc_cmd_send(2,0x51,0x16);
        dsc_cmd_send(2,0x52,0x02);
        dsc_cmd_send(2,0x53,0x18);
        dsc_cmd_send(2,0x54,0x02);
        dsc_cmd_send(2,0x55,0x4E);
        dsc_cmd_send(2,0x56,0x02);
        dsc_cmd_send(2,0x58,0x88);
        dsc_cmd_send(2,0x59,0x02);
        dsc_cmd_send(2,0x5A,0xAC);
        dsc_cmd_send(2,0x5B,0x02);
        dsc_cmd_send(2,0x5C,0xDD);
        dsc_cmd_send(2,0x5D,0x03);
        dsc_cmd_send(2,0x5E,0x01);
        dsc_cmd_send(2,0x5F,0x03);
        dsc_cmd_send(2,0x60,0x2E);
        dsc_cmd_send(2,0x61,0x03);
        dsc_cmd_send(2,0x62,0x3C);
        dsc_cmd_send(2,0x63,0x03);
        dsc_cmd_send(2,0x64,0x4C);
        dsc_cmd_send(2,0x65,0x03);
        dsc_cmd_send(2,0x66,0x5D);
        dsc_cmd_send(2,0x67,0x03);
        dsc_cmd_send(2,0x68,0x70);
        dsc_cmd_send(2,0x69,0x03);
        dsc_cmd_send(2,0x6A,0x88);
        dsc_cmd_send(2,0x6B,0x03);
        dsc_cmd_send(2,0x6C,0xA8);
        dsc_cmd_send(2,0x6D,0x03);
        dsc_cmd_send(2,0x6E,0xC8);
        dsc_cmd_send(2,0x6F,0x03);
        dsc_cmd_send(2,0x70,0xFF);
        ////B(+) MCR cmd
        dsc_cmd_send(2,0x71,0x00);
        dsc_cmd_send(2,0x72,0x00);
        dsc_cmd_send(2,0x73,0x00);
        dsc_cmd_send(2,0x74,0x2C);
        dsc_cmd_send(2,0x75,0x00);
        dsc_cmd_send(2,0x76,0x4F);
        dsc_cmd_send(2,0x77,0x00);
        dsc_cmd_send(2,0x78,0x69);
        dsc_cmd_send(2,0x79,0x00);
        dsc_cmd_send(2,0x7A,0x7F);
        dsc_cmd_send(2,0x7B,0x00);
        dsc_cmd_send(2,0x7C,0x92);
        dsc_cmd_send(2,0x7D,0x00);
        dsc_cmd_send(2,0x7E,0xA3);
        dsc_cmd_send(2,0x7F,0x00);
        dsc_cmd_send(2,0x80,0xB3);
        dsc_cmd_send(2,0x81,0x00);
        dsc_cmd_send(2,0x82,0xC1);
        dsc_cmd_send(2,0x83,0x00);
        dsc_cmd_send(2,0x84,0xF3);
        dsc_cmd_send(2,0x85,0x01);
        dsc_cmd_send(2,0x86,0x1B);
        dsc_cmd_send(2,0x87,0x01);
        dsc_cmd_send(2,0x88,0x5A);
        dsc_cmd_send(2,0x89,0x01);
        dsc_cmd_send(2,0x8A,0x8B);
        dsc_cmd_send(2,0x8B,0x01);
        dsc_cmd_send(2,0x8C,0xD9);
        dsc_cmd_send(2,0x8D,0x02);
        dsc_cmd_send(2,0x8E,0x16);
        dsc_cmd_send(2,0x8F,0x02);
        dsc_cmd_send(2,0x90,0x18);
        dsc_cmd_send(2,0x91,0x02);
        dsc_cmd_send(2,0x92,0x4E);
        dsc_cmd_send(2,0x93,0x02);
        dsc_cmd_send(2,0x94,0x88);
        dsc_cmd_send(2,0x95,0x02);
        dsc_cmd_send(2,0x96,0xAC);
        dsc_cmd_send(2,0x97,0x02);
        dsc_cmd_send(2,0x98,0xDD);
        dsc_cmd_send(2,0x99,0x03);
        dsc_cmd_send(2,0x9A,0x01);
        dsc_cmd_send(2,0x9B,0x03);
        dsc_cmd_send(2,0x9C,0x2E);
        dsc_cmd_send(2,0x9D,0x03);
        dsc_cmd_send(2,0x9E,0x3C);
        dsc_cmd_send(2,0x9F,0x03);
        dsc_cmd_send(2,0xA0,0x4C);
        dsc_cmd_send(2,0xA2,0x03);
        dsc_cmd_send(2,0xA3,0x5D);
        dsc_cmd_send(2,0xA4,0x03);
        dsc_cmd_send(2,0xA5,0x70);
        dsc_cmd_send(2,0xA6,0x03);
        dsc_cmd_send(2,0xA7,0x88);
        dsc_cmd_send(2,0xA9,0x03);
        dsc_cmd_send(2,0xAA,0xA8);
        dsc_cmd_send(2,0xAB,0x03);
        dsc_cmd_send(2,0xAC,0xC8);
        dsc_cmd_send(2,0xAD,0x03);
        dsc_cmd_send(2,0xAE,0xFF);
        ////B(-) MCR cmd
        dsc_cmd_send(2,0xAF,0x00);
        dsc_cmd_send(2,0xB0,0x00);
        dsc_cmd_send(2,0xB1,0x00);
        dsc_cmd_send(2,0xB2,0x2C);
        dsc_cmd_send(2,0xB3,0x00);
        dsc_cmd_send(2,0xB4,0x4F);
        dsc_cmd_send(2,0xB5,0x00);
        dsc_cmd_send(2,0xB6,0x69);
        dsc_cmd_send(2,0xB7,0x00);
        dsc_cmd_send(2,0xB8,0x7F);
        dsc_cmd_send(2,0xB9,0x00);
        dsc_cmd_send(2,0xBA,0x92);
        dsc_cmd_send(2,0xBB,0x00);
        dsc_cmd_send(2,0xBC,0xA3);
        dsc_cmd_send(2,0xBD,0x00);
        dsc_cmd_send(2,0xBE,0xB3);
        dsc_cmd_send(2,0xBF,0x00);
        dsc_cmd_send(2,0xC0,0xC1);
        dsc_cmd_send(2,0xC1,0x00);
        dsc_cmd_send(2,0xC2,0xF3);
        dsc_cmd_send(2,0xC3,0x01);
        dsc_cmd_send(2,0xC4,0x1B);
        dsc_cmd_send(2,0xC5,0x01);
        dsc_cmd_send(2,0xC6,0x5A);
        dsc_cmd_send(2,0xC7,0x01);
        dsc_cmd_send(2,0xC8,0x8B);
        dsc_cmd_send(2,0xC9,0x01);
        dsc_cmd_send(2,0xCA,0xD9);
        dsc_cmd_send(2,0xCB,0x02);
        dsc_cmd_send(2,0xCC,0x16);
        dsc_cmd_send(2,0xCD,0x02);
        dsc_cmd_send(2,0xCE,0x18);
        dsc_cmd_send(2,0xCF,0x02);
        dsc_cmd_send(2,0xD0,0x4E);
        dsc_cmd_send(2,0xD1,0x02);
        dsc_cmd_send(2,0xD2,0x88);
        dsc_cmd_send(2,0xD3,0x02);
        dsc_cmd_send(2,0xD4,0xAC);
        dsc_cmd_send(2,0xD5,0x02);
        dsc_cmd_send(2,0xD6,0xDD);
        dsc_cmd_send(2,0xD7,0x03);
        dsc_cmd_send(2,0xD8,0x01);
        dsc_cmd_send(2,0xD9,0x03);
        dsc_cmd_send(2,0xDA,0x2E);
        dsc_cmd_send(2,0xDB,0x03);
        dsc_cmd_send(2,0xDC,0x3C);
        dsc_cmd_send(2,0xDD,0x03);
        dsc_cmd_send(2,0xDE,0x4C);
        dsc_cmd_send(2,0xDF,0x03);
        dsc_cmd_send(2,0xE0,0x5D);
        dsc_cmd_send(2,0xE1,0x03);
        dsc_cmd_send(2,0xE2,0x70);
        dsc_cmd_send(2,0xE3,0x03);
        dsc_cmd_send(2,0xE4,0x88);
        dsc_cmd_send(2,0xE5,0x03);
        dsc_cmd_send(2,0xE6,0xA8);
        dsc_cmd_send(2,0xE7,0x03);
        dsc_cmd_send(2,0xE8,0xC8);
        dsc_cmd_send(2,0xE9,0x03);
        dsc_cmd_send(2,0xEA,0xFF);

        dsc_cmd_send(2,0xFF,0x00);
        dsc_cmd_send(2,0xFB,0x01);

        dsc_cmd_send(2,0x11,0x00);

#ifndef _SIMU        
        mdelay(100);
#endif
        dsc_cmd_send(2,0x29,0x00);
#ifndef _SIMU
        mdelay(100);
#endif
//        printk("lcd init 8\n");
}
/**
 * @brief 
 * 
 * @param data1 
 * @param data2 
 * @param data3 
 * @param data4 
 */
void  dsc_cmd_send_4(int data1, int data2, int data3,int data4)
{
	int cmd_sts =0;
	DsiRegWr(DIRECT_CMD_MAIN_SETTINGS_OFFSET,0x01043908); //[24] cmd_lp_en  =1    0x01043908
															//[23:14] cmd_size  =cmd_size. 1 or 2   //write 4
															//[13:8] cmd_head  =0x39
															//[3 ] cmd_longnotshort  =0x1
															//[2:0] cmd_nat : 3'b000:write command
															//                3'b001:read command
															//                3'b100:TE request
															//                3'b101:trigger request
															//                3'b110:BTA request
															
	DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,((data4<<24) + (data3<<16) + (data2<<8) + data1));// DATA
	DsiRegWr(DIRECT_CMD_SEND_OFFSET,0x00000); // cmd send
	while((cmd_sts & 0x02 )!= 0x02)
	{  
		cmd_sts = DsiRegRd(DIRECT_CMD_STS_OFFSET);
	}
	DsiRegWr(DIRECT_CMD_STS_CLR_OFFSET,0x02); // clear write_completed
}
/**
 * @brief 
 * 
 */
void ILI7807_lcd_init(void)
{
//        printk("ILI7807_lcd_init\n");
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x01);			// write page 1	
	dsc_cmd_send(2, 0x42, 0x11);
	dsc_cmd_send(2, 0x43, 0xB2);		//VGH_CLP = 11.0V
//        dsc_cmd_read(0x43);
	dsc_cmd_send(2, 0x44, 0x94);		//VGL_CLP = -7.0V
	dsc_cmd_send(2, 0x4A, 0x15);
	dsc_cmd_send(2, 0x4B, 0x15);
	dsc_cmd_send(2, 0x50, 0x73);		//GVDDP = 5.3V
	dsc_cmd_send(2, 0x51, 0x73);		//GVDDN = - 5.3V
	dsc_cmd_send(2, 0x5A, 0x33);		//LVD
	dsc_cmd_send(2, 0xA0, 0x10);
	dsc_cmd_send(2, 0xA2, 0x01);		//VCOM1 = - 0.35V
	dsc_cmd_send(2, 0xA3, 0x22);		//VCOM1 = - 0.35V
	dsc_cmd_send(2, 0xB3, 0x60);
	dsc_cmd_send(2, 0xB4, 0x60);
	
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x01);			// write page 1
	dsc_cmd_send(2, 0x22, 0x06);
	dsc_cmd_send(2, 0x36, 0x00);
	dsc_cmd_send(2, 0x60, 0x0A);
	dsc_cmd_send(2, 0x64, 0x08);
	dsc_cmd_send(2, 0x6C, 0x45);		//PRC & PRCB
	dsc_cmd_send(2, 0x35, 0x22);
	
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x07);			// write page 7
	dsc_cmd_send(2, 0x37, 0x00);
	dsc_cmd_send(2, 0x12, 0x22);
	
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x08);			// write page 8 for GIP

	dsc_cmd_send(2, 0x09, 0x0e);
	
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x06);			// write page 8 for GIP
	dsc_cmd_send(2, 0x00, 0x42);
	dsc_cmd_send(2, 0x01, 0x04);
	dsc_cmd_send(2, 0x02, 0x0B);
	dsc_cmd_send(2, 0x03, 0x03);
	dsc_cmd_send(2, 0x04, 0x00);
	dsc_cmd_send(2, 0x05, 0x00);
	dsc_cmd_send(2, 0x06, 0x00);
	dsc_cmd_send(2, 0x07, 0x00);
	dsc_cmd_send(2, 0x08, 0x81);
	dsc_cmd_send(2, 0x09, 0x05);
	dsc_cmd_send(2, 0x0a, 0x70);
	dsc_cmd_send(2, 0x0b, 0x00);
	dsc_cmd_send(2, 0x0c, 0x0B);
	dsc_cmd_send(2, 0x0d, 0x0B);
	dsc_cmd_send(2, 0x0e, 0x03);
	dsc_cmd_send(2, 0x0f, 0x03);
	dsc_cmd_send(2, 0x10, 0x00);
	dsc_cmd_send(2, 0x10, 0x00);
	dsc_cmd_send(2, 0x12, 0x00);
	dsc_cmd_send(2, 0x13, 0x00);
	dsc_cmd_send(2, 0x14, 0x00);
	dsc_cmd_send(2, 0x15, 0x00);
	dsc_cmd_send(2, 0x31, 0x22);
	dsc_cmd_send(2, 0x32, 0x27);
	dsc_cmd_send(2, 0x33, 0x07);
	dsc_cmd_send(2, 0x34, 0x08);
	dsc_cmd_send(2, 0x35, 0x10);
	dsc_cmd_send(2, 0x36, 0x16);
	dsc_cmd_send(2, 0x37, 0x14);
	dsc_cmd_send(2, 0x38, 0x12);
	dsc_cmd_send(2, 0x39, 0x07);
	dsc_cmd_send(2, 0x3a, 0x07);
	dsc_cmd_send(2, 0x3b, 0x07);
	dsc_cmd_send(2, 0x3c, 0x28);
	dsc_cmd_send(2, 0x3d, 0x29);
	dsc_cmd_send(2, 0x3e, 0x2a);
	dsc_cmd_send(2, 0x3f, 0x07);
	dsc_cmd_send(2, 0x40, 0x07);
	dsc_cmd_send(2, 0x41, 0x22);
	dsc_cmd_send(2, 0x42, 0x27);
	dsc_cmd_send(2, 0x43, 0x07);
	dsc_cmd_send(2, 0x44, 0x09);
	dsc_cmd_send(2, 0x45, 0x11);
	dsc_cmd_send(2, 0x46, 0x17);
	dsc_cmd_send(2, 0x47, 0x15);
	dsc_cmd_send(2, 0x48, 0x13);
	dsc_cmd_send(2, 0x49, 0x07);
	dsc_cmd_send(2, 0x4a, 0x07);
	dsc_cmd_send(2, 0x4b, 0x07);
	dsc_cmd_send(2, 0x4c, 0x28);
	dsc_cmd_send(2, 0x4d, 0x29);
	dsc_cmd_send(2, 0x4e, 0x2a);
	dsc_cmd_send(2, 0x4f, 0x07);
	dsc_cmd_send(2, 0x50, 0x07);
	dsc_cmd_send(2, 0x61, 0x22);
	dsc_cmd_send(2, 0x62, 0x27);
	dsc_cmd_send(2, 0x63, 0x07);
	dsc_cmd_send(2, 0x64, 0x09);
	dsc_cmd_send(2, 0x65, 0x13);
	dsc_cmd_send(2, 0x66, 0x15);
	dsc_cmd_send(2, 0x67, 0x17);
	dsc_cmd_send(2, 0x68, 0x11);
	dsc_cmd_send(2, 0x69, 0x07);
	dsc_cmd_send(2, 0x6a, 0x07);
	dsc_cmd_send(2, 0x6b, 0x07);
	dsc_cmd_send(2, 0x6c, 0x28);
	dsc_cmd_send(2, 0x6d, 0x29);
	dsc_cmd_send(2, 0x6e, 0x2a);
	dsc_cmd_send(2, 0x6f, 0x07);
	
	dsc_cmd_send(2, 0x70, 0x07);
	dsc_cmd_send(2, 0x71, 0x22);
	dsc_cmd_send(2, 0x72, 0x27);
	dsc_cmd_send(2, 0x73, 0x07);
	dsc_cmd_send(2, 0x74, 0x08);
	dsc_cmd_send(2, 0x75, 0x12);
	dsc_cmd_send(2, 0x76, 0x14);
	dsc_cmd_send(2, 0x77, 0x16);
	dsc_cmd_send(2, 0x78, 0x10);
	dsc_cmd_send(2, 0x79, 0x07);
	dsc_cmd_send(2, 0x7a, 0x07);
	dsc_cmd_send(2, 0x7b, 0x07);
	dsc_cmd_send(2, 0x7c, 0x28);
	dsc_cmd_send(2, 0x7d, 0x29);
	dsc_cmd_send(2, 0x7e, 0x2a);
	dsc_cmd_send(2, 0x7f, 0x07);
	dsc_cmd_send(2, 0x80, 0x07);
	
	dsc_cmd_send(2, 0xd0, 0x01);
	
	dsc_cmd_send(2, 0xdb, 0x40);
	dsc_cmd_send(2, 0xdc, 0x04);
	dsc_cmd_send(2, 0xdd, 0x00);
	
	dsc_cmd_send(2, 0xa0, 0x13);
	dsc_cmd_send(2, 0xa1, 0x00);
	dsc_cmd_send(2, 0xa2, 0x05);
	dsc_cmd_send(2, 0xa3, 0x20);
	dsc_cmd_send(2, 0xa6, 0x00);
	dsc_cmd_send(2, 0xa7, 0x00);
	
	dsc_cmd_send(2, 0x97, 0x22);
	
	dsc_cmd_send(2, 0xD1, 0x01);
	dsc_cmd_send(2, 0xD2, 0x00);
	dsc_cmd_send(2, 0xDF, 0x00);
	dsc_cmd_send(2, 0xE5, 0x10);
	dsc_cmd_send(2, 0xE6, 0x00);
	dsc_cmd_send(2, 0xD6, 0x10);
	dsc_cmd_send(2, 0xD7, 0x20);
	dsc_cmd_send(2, 0xD8, 0x14);
	dsc_cmd_send(2, 0xD9, 0x00);
	dsc_cmd_send(2, 0xDA, 0x00);
	
	dsc_cmd_send(2, 0xB2, 0x00);
	dsc_cmd_send(2, 0xB3, 0x00);
	
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x02);	 //page 02
	dsc_cmd_send(2, 0x00, 0x00);
	dsc_cmd_send(2, 0x01, 0x30);
	dsc_cmd_send(2, 0x02, 0x00);
	dsc_cmd_send(2, 0x03, 0x4A);
	dsc_cmd_send(2, 0x04, 0x00);
	dsc_cmd_send(2, 0x05, 0x6F);
	dsc_cmd_send(2, 0x06, 0x00);
	dsc_cmd_send(2, 0x07, 0x8B);
	dsc_cmd_send(2, 0x08, 0x00);
	dsc_cmd_send(2, 0x09, 0xA1);
	dsc_cmd_send(2, 0x0A, 0x00);
	dsc_cmd_send(2, 0x0B, 0xB4);
	dsc_cmd_send(2, 0x0C, 0x0C);
	dsc_cmd_send(2, 0x0D, 0xC5);
	dsc_cmd_send(2, 0x0E, 0x00);
	dsc_cmd_send(2, 0x0F, 0xD5);
	dsc_cmd_send(2, 0x10, 0x00);
	dsc_cmd_send(2, 0x11, 0xE2);
	dsc_cmd_send(2, 0x12, 0x01);
	dsc_cmd_send(2, 0x13, 0x10);
	dsc_cmd_send(2, 0x14, 0x01);
	dsc_cmd_send(2, 0x15, 0x34);
	dsc_cmd_send(2, 0x16, 0x01);
	dsc_cmd_send(2, 0x17, 0x6D);
	dsc_cmd_send(2, 0x18, 0x01);
	dsc_cmd_send(2, 0x19, 0x9A);
	dsc_cmd_send(2, 0x1A, 0x01);
	dsc_cmd_send(2, 0x1B, 0xE2);
	dsc_cmd_send(2, 0x1C, 0x02);
	dsc_cmd_send(2, 0x1D, 0x1B);
	dsc_cmd_send(2, 0x1E, 0x02);
	dsc_cmd_send(2, 0x1F, 0x1C);
	dsc_cmd_send(2, 0x20, 0x02);
	dsc_cmd_send(2, 0x21, 0x52);
	dsc_cmd_send(2, 0x22, 0x02);
	dsc_cmd_send(2, 0x23, 0x8F);
	dsc_cmd_send(2, 0x24, 0x02);
	dsc_cmd_send(2, 0x25, 0xB7);
	dsc_cmd_send(2, 0x26, 0x02);
	dsc_cmd_send(2, 0x27, 0xEC);
	dsc_cmd_send(2, 0x28, 0x03);
	dsc_cmd_send(2, 0x29, 0x0E);
	dsc_cmd_send(2, 0x2A, 0x03);
	dsc_cmd_send(2, 0x2B, 0x3A);
	dsc_cmd_send(2, 0x2C, 0x03);
	dsc_cmd_send(2, 0x2D, 0x47);
	dsc_cmd_send(2, 0x2E, 0x03);
	dsc_cmd_send(2, 0x2F, 0x55);
	dsc_cmd_send(2, 0x30, 0x03);
	dsc_cmd_send(2, 0x31, 0x65);
	dsc_cmd_send(2, 0x32, 0x03);
	dsc_cmd_send(2, 0x33, 0x76);
	dsc_cmd_send(2, 0x34, 0x03);
	dsc_cmd_send(2, 0x35, 0x8A);
	dsc_cmd_send(2, 0x36, 0x03);
	dsc_cmd_send(2, 0x37, 0xA3);
	dsc_cmd_send(2, 0x38, 0x03);
	dsc_cmd_send(2, 0x39, 0xC4);
	dsc_cmd_send(2, 0x3A, 0x03);
	dsc_cmd_send(2, 0x3B, 0xD1);
	
	dsc_cmd_send(2, 0x3C, 0x00);
	dsc_cmd_send(2, 0x3D, 0x30);
	dsc_cmd_send(2, 0x3E, 0x00);
	dsc_cmd_send(2, 0x3F, 0x4A);
	dsc_cmd_send(2, 0x40, 0x00);
	dsc_cmd_send(2, 0x41, 0x6F);
	dsc_cmd_send(2, 0x42, 0x00);
	dsc_cmd_send(2, 0x43, 0x8B);
	dsc_cmd_send(2, 0x44, 0x00);
	dsc_cmd_send(2, 0x45, 0xA1);
	dsc_cmd_send(2, 0x46, 0x00);
	dsc_cmd_send(2, 0x47, 0xB4);
	dsc_cmd_send(2, 0x48, 0x00);
	dsc_cmd_send(2, 0x49, 0xC5);
	dsc_cmd_send(2, 0x4A, 0x00);
	dsc_cmd_send(2, 0x4B, 0xD5);
	dsc_cmd_send(2, 0x4C, 0x00);
	dsc_cmd_send(2, 0x4D, 0xE2);
	dsc_cmd_send(2, 0x4E, 0x01);
	dsc_cmd_send(2, 0x4F, 0x10);
	dsc_cmd_send(2, 0x50, 0x01);
	dsc_cmd_send(2, 0x51, 0x34);
	dsc_cmd_send(2, 0x52, 0x01);
	dsc_cmd_send(2, 0x53, 0x6D);
	dsc_cmd_send(2, 0x54, 0x01);
	dsc_cmd_send(2, 0x55, 0x9A);
	dsc_cmd_send(2, 0x56, 0x01);
	dsc_cmd_send(2, 0x57, 0xE2);
	dsc_cmd_send(2, 0x58, 0x02);
	dsc_cmd_send(2, 0x59, 0x1B);
	dsc_cmd_send(2, 0x5A, 0x02);
	dsc_cmd_send(2, 0x5B, 0x1C);
	dsc_cmd_send(2, 0x5C, 0x02);
	dsc_cmd_send(2, 0x5D, 0x52);
	dsc_cmd_send(2, 0x5E, 0x02);
	dsc_cmd_send(2, 0x5F, 0x8F);
	dsc_cmd_send(2, 0x60, 0x02);
	dsc_cmd_send(2, 0x61, 0xB7);
	dsc_cmd_send(2, 0x62, 0x02);
	dsc_cmd_send(2, 0x63, 0xEC);
	dsc_cmd_send(2, 0x64, 0x03);
	dsc_cmd_send(2, 0x65, 0x0E);
	dsc_cmd_send(2, 0x66, 0x03);
	dsc_cmd_send(2, 0x67, 0x3A);
	dsc_cmd_send(2, 0x68, 0x03);
	dsc_cmd_send(2, 0x69, 0x47);
	dsc_cmd_send(2, 0x6A, 0x03);
	dsc_cmd_send(2, 0x6B, 0x55);
	dsc_cmd_send(2, 0x6C, 0x03);
	dsc_cmd_send(2, 0x6D, 0x65);
	dsc_cmd_send(2, 0x6E, 0x03);
	dsc_cmd_send(2, 0x6F, 0x76);
	dsc_cmd_send(2, 0x70, 0x03);
	dsc_cmd_send(2, 0x71, 0x8A);
	dsc_cmd_send(2, 0x72, 0x03);
	dsc_cmd_send(2, 0x73, 0xA3);
	dsc_cmd_send(2, 0x74, 0x03);
	dsc_cmd_send(2, 0x75, 0xC4);
	dsc_cmd_send(2, 0x76, 0x03);
	dsc_cmd_send(2, 0x77, 0xD1);
	
	dsc_cmd_send(2, 0x78, 0x01);
	dsc_cmd_send(2, 0x79, 0x01);
	
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x03);	// page3 
	dsc_cmd_send(2, 0x00, 0x00);	
	dsc_cmd_send(2, 0x01, 0x30);
	dsc_cmd_send(2, 0x02, 0x00);
	dsc_cmd_send(2, 0x03, 0x4A);
	dsc_cmd_send(2, 0x04, 0x00);
	dsc_cmd_send(2, 0x05, 0x6F);
	dsc_cmd_send(2, 0x06, 0x00);
	dsc_cmd_send(2, 0x07, 0x8B);
	dsc_cmd_send(2, 0x08, 0x00);
	dsc_cmd_send(2, 0x09, 0xA1);
	dsc_cmd_send(2, 0x0A, 0x00);
	dsc_cmd_send(2, 0x0B, 0xB4);
	dsc_cmd_send(2, 0x0C, 0x00);
	dsc_cmd_send(2, 0x0D, 0xC5);
	dsc_cmd_send(2, 0x0E, 0x00);
	dsc_cmd_send(2, 0x0F, 0xD5);
	dsc_cmd_send(2, 0x10, 0x00);
	dsc_cmd_send(2, 0x11, 0xE2);
	dsc_cmd_send(2, 0x12, 0x01);
	dsc_cmd_send(2, 0x13, 0x10);
	dsc_cmd_send(2, 0x14, 0x01);
	dsc_cmd_send(2, 0x15, 0x34);
	dsc_cmd_send(2, 0x16, 0x01);
	dsc_cmd_send(2, 0x17, 0x6D);
	dsc_cmd_send(2, 0x18, 0x01);
	dsc_cmd_send(2, 0x19, 0x9A);
	dsc_cmd_send(2, 0x1A, 0x01);
	dsc_cmd_send(2, 0x1B, 0xE2);
	dsc_cmd_send(2, 0x1C, 0x02);
	dsc_cmd_send(2, 0x1D, 0x1B);
	dsc_cmd_send(2, 0x1E, 0x02);
	dsc_cmd_send(2, 0x1F, 0x1C);
	dsc_cmd_send(2, 0x20, 0x02);
	dsc_cmd_send(2, 0x21, 0x52);
	dsc_cmd_send(2, 0x22, 0x02);
	dsc_cmd_send(2, 0x23, 0x8F);
	dsc_cmd_send(2, 0x24, 0x02);
	dsc_cmd_send(2, 0x25, 0xB7);
	dsc_cmd_send(2, 0x26, 0x02);
	dsc_cmd_send(2, 0x27, 0xEC);
	dsc_cmd_send(2, 0x28, 0x03);
	dsc_cmd_send(2, 0x29, 0x0E);
	dsc_cmd_send(2, 0x2A, 0x03);
	dsc_cmd_send(2, 0x2B, 0x3A);
	dsc_cmd_send(2, 0x2C, 0x03);
	dsc_cmd_send(2, 0x2D, 0x47);
	dsc_cmd_send(2, 0x2E, 0x03);
	dsc_cmd_send(2, 0x2F, 0x55);
	dsc_cmd_send(2, 0x30, 0x03);
	dsc_cmd_send(2, 0x31, 0x65);
	dsc_cmd_send(2, 0x32, 0x03);
	dsc_cmd_send(2, 0x33, 0x76);
	dsc_cmd_send(2, 0x34, 0x03);
	dsc_cmd_send(2, 0x35, 0x8A);
	dsc_cmd_send(2, 0x36, 0x03);
	dsc_cmd_send(2, 0x37, 0xA3);
	dsc_cmd_send(2, 0x38, 0x03);
	dsc_cmd_send(2, 0x39, 0xC4);
	dsc_cmd_send(2, 0x3A, 0x03);
	dsc_cmd_send(2, 0x3B, 0xD1);
	
	dsc_cmd_send(2, 0x3C, 0x00);
	dsc_cmd_send(2, 0x3D, 0x30);
	dsc_cmd_send(2, 0x3E, 0x00);
	dsc_cmd_send(2, 0x3F, 0x4A);
	dsc_cmd_send(2, 0x40, 0x00);
	dsc_cmd_send(2, 0x41, 0x6F);
	dsc_cmd_send(2, 0x42, 0x00);
	dsc_cmd_send(2, 0x43, 0x8B);
	dsc_cmd_send(2, 0x44, 0x00);
	dsc_cmd_send(2, 0x45, 0xA1);
	dsc_cmd_send(2, 0x46, 0x00);
	dsc_cmd_send(2, 0x47, 0xB4);
	dsc_cmd_send(2, 0x48, 0x00);
	dsc_cmd_send(2, 0x49, 0xC5);
	dsc_cmd_send(2, 0x4A, 0x00);
	dsc_cmd_send(2, 0x4B, 0xD5);
	dsc_cmd_send(2, 0x4C, 0x00);
	dsc_cmd_send(2, 0x4D, 0xE2);
	dsc_cmd_send(2, 0x4E, 0x01);
	dsc_cmd_send(2, 0x4F, 0x10);
	dsc_cmd_send(2, 0x50, 0x01);
	dsc_cmd_send(2, 0x51, 0x34);
	dsc_cmd_send(2, 0x52, 0x01);
	dsc_cmd_send(2, 0x53, 0x6D);
	dsc_cmd_send(2, 0x54, 0x01);
	dsc_cmd_send(2, 0x55, 0x9A);
	dsc_cmd_send(2, 0x56, 0x01);
	dsc_cmd_send(2, 0x57, 0xE2);
	dsc_cmd_send(2, 0x58, 0x02);
	dsc_cmd_send(2, 0x59, 0x1B);
	dsc_cmd_send(2, 0x5A, 0x02);
	dsc_cmd_send(2, 0x5B, 0x1C);
	dsc_cmd_send(2, 0x5C, 0x02);
	dsc_cmd_send(2, 0x5D, 0x52);
	dsc_cmd_send(2, 0x5E, 0x02);
	dsc_cmd_send(2, 0x5F, 0x8F);
	dsc_cmd_send(2, 0x60, 0x02);
	dsc_cmd_send(2, 0x61, 0xB7);
	dsc_cmd_send(2, 0x62, 0x02);
	dsc_cmd_send(2, 0x63, 0xEC);
	dsc_cmd_send(2, 0x64, 0x03);
	dsc_cmd_send(2, 0x65, 0x0E);
	dsc_cmd_send(2, 0x66, 0x03);
	dsc_cmd_send(2, 0x67, 0x3A);
	dsc_cmd_send(2, 0x68, 0x03);
	dsc_cmd_send(2, 0x69, 0x47);
	dsc_cmd_send(2, 0x6A, 0x03);
	dsc_cmd_send(2, 0x6B, 0x55);
	dsc_cmd_send(2, 0x6C, 0x03);
	dsc_cmd_send(2, 0x6D, 0x65);
	dsc_cmd_send(2, 0x6E, 0x03);
	dsc_cmd_send(2, 0x6F, 0x76);
	dsc_cmd_send(2, 0x70, 0x03);
	dsc_cmd_send(2, 0x71, 0x8A);
	dsc_cmd_send(2, 0x72, 0x03);
	dsc_cmd_send(2, 0x73, 0xA3);
	dsc_cmd_send(2, 0x74, 0x03);
	dsc_cmd_send(2, 0x75, 0xC4);
	dsc_cmd_send(2, 0x76, 0x03);
	dsc_cmd_send(2, 0x77, 0xD1);
	
	dsc_cmd_send(2, 0x78, 0x01);
	dsc_cmd_send(2, 0x79, 0x01);
	
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x04); // pagetr for 04
	dsc_cmd_send(2, 0x00, 0x00);
	dsc_cmd_send(2, 0x01, 0x30);
	dsc_cmd_send(2, 0x02, 0x00);
	dsc_cmd_send(2, 0x03, 0x4A);
	dsc_cmd_send(2, 0x04, 0x00);
	dsc_cmd_send(2, 0x05, 0x6F);
	dsc_cmd_send(2, 0x06, 0x00);
	dsc_cmd_send(2, 0x07, 0x8B);
	dsc_cmd_send(2, 0x08, 0x00);
	dsc_cmd_send(2, 0x09, 0xA1);
	dsc_cmd_send(2, 0x0A, 0x00);
	dsc_cmd_send(2, 0x0B, 0xB4);
	dsc_cmd_send(2, 0x0C, 0x00);
	dsc_cmd_send(2, 0x0D, 0xC5);
	dsc_cmd_send(2, 0x0E, 0x00);
	dsc_cmd_send(2, 0x0F, 0xD5);
	dsc_cmd_send(2, 0x10, 0x00);
	dsc_cmd_send(2, 0x11, 0xE2);
	dsc_cmd_send(2, 0x12, 0x01);
	dsc_cmd_send(2, 0x13, 0x10);
	dsc_cmd_send(2, 0x14, 0x01);
	dsc_cmd_send(2, 0x15, 0x34);
	dsc_cmd_send(2, 0x16, 0x01);
	dsc_cmd_send(2, 0x17, 0x6D);
	dsc_cmd_send(2, 0x18, 0x01);
	dsc_cmd_send(2, 0x19, 0x9A);
	dsc_cmd_send(2, 0x1A, 0x01);
	dsc_cmd_send(2, 0x1B, 0xE2);
	dsc_cmd_send(2, 0x1C, 0x02);
	dsc_cmd_send(2, 0x1D, 0x1B);
	dsc_cmd_send(2, 0x1E, 0x02);
	dsc_cmd_send(2, 0x1F, 0x1C);
	dsc_cmd_send(2, 0x20, 0x02);
	dsc_cmd_send(2, 0x21, 0x52);
	dsc_cmd_send(2, 0x22, 0x02);
	dsc_cmd_send(2, 0x23, 0x8F);
	dsc_cmd_send(2, 0x24, 0x02);
	dsc_cmd_send(2, 0x25, 0xB7);
	dsc_cmd_send(2, 0x26, 0x02);
	dsc_cmd_send(2, 0x27, 0xEC);
	dsc_cmd_send(2, 0x28, 0x03);
	dsc_cmd_send(2, 0x29, 0x0E);
	dsc_cmd_send(2, 0x2A, 0x03);
	dsc_cmd_send(2, 0x2B, 0x3A);
	dsc_cmd_send(2, 0x2C, 0x03);
	dsc_cmd_send(2, 0x2D, 0x47);
	dsc_cmd_send(2, 0x2E, 0x03);
	dsc_cmd_send(2, 0x2F, 0x55);
	dsc_cmd_send(2, 0x30, 0x03);
	dsc_cmd_send(2, 0x31, 0x65);
	dsc_cmd_send(2, 0x32, 0x03);
	dsc_cmd_send(2, 0x33, 0x76);
	dsc_cmd_send(2, 0x34, 0x03);
	dsc_cmd_send(2, 0x35, 0x8A);
	dsc_cmd_send(2, 0x36, 0x03);
	dsc_cmd_send(2, 0x37, 0xA3);
	dsc_cmd_send(2, 0x38, 0x03);
	dsc_cmd_send(2, 0x39, 0xC4);
	dsc_cmd_send(2, 0x3A, 0x03);
	dsc_cmd_send(2, 0x3B, 0xD1);
	
	dsc_cmd_send(2, 0x3C, 0x00);
	dsc_cmd_send(2, 0x3D, 0x30);
	dsc_cmd_send(2, 0x3E, 0x00);
	dsc_cmd_send(2, 0x3F, 0x4A);
	dsc_cmd_send(2, 0x40, 0x00);
	dsc_cmd_send(2, 0x41, 0x6F);
	dsc_cmd_send(2, 0x42, 0x00);
	dsc_cmd_send(2, 0x43, 0x8B);
	dsc_cmd_send(2, 0x44, 0x00);
	dsc_cmd_send(2, 0x45, 0xA1);
	dsc_cmd_send(2, 0x46, 0x00);
	dsc_cmd_send(2, 0x47, 0xB4);
	dsc_cmd_send(2, 0x48, 0x00);
	dsc_cmd_send(2, 0x49, 0xC5);
	dsc_cmd_send(2, 0x4A, 0x00);
	dsc_cmd_send(2, 0x4B, 0xD5);
	dsc_cmd_send(2, 0x4C, 0x00);
	dsc_cmd_send(2, 0x4D, 0xE2);
	dsc_cmd_send(2, 0x4E, 0x01);
	dsc_cmd_send(2, 0x4F, 0x10);
	dsc_cmd_send(2, 0x50, 0x01);
	dsc_cmd_send(2, 0x51, 0x34);
	dsc_cmd_send(2, 0x52, 0x01);
	dsc_cmd_send(2, 0x53, 0x6D);
	dsc_cmd_send(2, 0x54, 0x01);
	dsc_cmd_send(2, 0x55, 0x9A);
	dsc_cmd_send(2, 0x56, 0x01);
	dsc_cmd_send(2, 0x57, 0xE2);
	dsc_cmd_send(2, 0x58, 0x02);
	dsc_cmd_send(2, 0x59, 0x1B);
	dsc_cmd_send(2, 0x5A, 0x02);
	dsc_cmd_send(2, 0x5B, 0x1C);
	dsc_cmd_send(2, 0x5C, 0x02);
	dsc_cmd_send(2, 0x5D, 0x52);
	dsc_cmd_send(2, 0x5E, 0x02);
	dsc_cmd_send(2, 0x5F, 0x8F);
	dsc_cmd_send(2, 0x60, 0x02);
	dsc_cmd_send(2, 0x61, 0xB7);
	dsc_cmd_send(2, 0x62, 0x02);
	dsc_cmd_send(2, 0x63, 0xEC);
	dsc_cmd_send(2, 0x64, 0x03);
	dsc_cmd_send(2, 0x65, 0x0E);
	dsc_cmd_send(2, 0x66, 0x03);
	dsc_cmd_send(2, 0x67, 0x3A);
	dsc_cmd_send(2, 0x68, 0x03);
	dsc_cmd_send(2, 0x69, 0x47);
	dsc_cmd_send(2, 0x6A, 0x03);
	dsc_cmd_send(2, 0x6B, 0x55);
	dsc_cmd_send(2, 0x6C, 0x03);
	dsc_cmd_send(2, 0x6D, 0x65);
	dsc_cmd_send(2, 0x6E, 0x03);
	dsc_cmd_send(2, 0x6F, 0x76);
	dsc_cmd_send(2, 0x70, 0x03);
	dsc_cmd_send(2, 0x71, 0x8A);
	dsc_cmd_send(2, 0x72, 0x03);
	dsc_cmd_send(2, 0x73, 0xA3);
	dsc_cmd_send(2, 0x74, 0x03);
	dsc_cmd_send(2, 0x75, 0xC4);
	dsc_cmd_send(2, 0x76, 0x03);
	dsc_cmd_send(2, 0x77, 0xD1);
	
	dsc_cmd_send(2, 0x78, 0x01);
	dsc_cmd_send(2, 0x79, 0x01);
	
	dsc_cmd_send_4(0xFF, 0x78, 0x07, 0x00);
	
	dsc_cmd_send(2, 0x11, 0x00);
	//msleep(150);
        mdelay(150);
	dsc_cmd_send(2, 0x29, 0x00);
	//msleep(50);
        mdelay(50);		
}
unsigned int g_mipi_corner_initialized = 0;

static void  mipi_coner_init(void)
{
    unsigned int rdata, wdata;
    uint32_t *addr = ioremap(VIDEO_SUBSYS_MIPI_CORNER_REG_BASE, 0x08);
    uint32_t *addr2 = ioremap(VIDEO_SUBSYS_MIPI_CORNER_REG_BASE + 0x4, 0x08);
    if(g_mipi_corner_initialized == 0)
    {
        sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_MIPI_CORNER_APB, 1);
        rdata =readl(addr);

        wdata=rdata |0x8;       
        writel(wdata, addr);
       
        rdata =readl(addr);

        wdata=rdata |0x4;
        writel(wdata, addr);
        
//        rdata =readl(addr+0x4);
        rdata =readl(addr2);

        while((rdata & 0x10000) != 0x10000)
        {
//          rdata =readl(addr+0x4);
             rdata =readl(addr2);
        }
        
        printk("mipi_coner_init done, pvt code 0x%x\n", rdata);
        //need a sleep?

        g_mipi_corner_initialized = 1;

        iounmap(addr);
        iounmap(addr2);
    }
}
/**
 * @brief 
 * 
 */
static void mipi_txdphy_init(void)
{
    uint32_t rdata, wdata;
    uint32_t *addr = ioremap(VIDEO_TXDPHY_BASE_ADDR, 0x08);
    uint32_t *addr2 = ioremap(VIDEO_TXDPHY_BASE_ADDR + 0xc, 0x08);

    //enable display apb clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_DSI_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_VO_APB, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_TWOD_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_BT1120_APB, 1);

    //enable display axi clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_VO_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_MFBC_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);
    

    //enable display pix clk, default 148.5MHz
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISPLAY_PIXEL, 1);

    //enale dsi system clock
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_DSI_SYSTEM, 1);

    //init corner
    mipi_coner_init();

//    sysctl_boot_set_root_clk_freq(SYSCTL_CLK_ROOT_PLL2, 593, 24, 0);

    //enable txdphy rclk and ref clk            
//    sysctl_clk_set_leaf_div(SYSCTL_CLK_MIPI_REF,1, 9);

//    printk("\n\n SYSCTL_CLK_MIPI_REF out clk freq is %d \r\n\n",sysctl_clk_get_leaf_freq(SYSCTL_CLK_MIPI_REF));
    
    sysctl_clk_set_leaf_en(SYSCTL_CLK_MIPI_REF_TXPHY_REF, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_MIPI_REF_TXPHY_PLL, 1);
    
    //de-assert dphy standby
    wdata        = 1 << 14;
    writel(wdata, addr);


    //init pll, default 891M
 //   mipi_txdphy_set_pll(TXDPHY_PLL_891M_F, TXDPHY_PLL_891M_R, TXDPHY_PLL_891M_D);
    mipi_txdphy_set_pll(TXDPHY_PLL_445M_F, TXDPHY_PLL_445M_R, TXDPHY_PLL_445M_D);       // p30

    //default timing settings
    wdata = 0x0f180080;                //0x0f180080       0x0f1be080    
    writel(wdata, addr2);

    iounmap(addr);
    iounmap(addr2);
}
/**
 * @brief 
 * 
 */
void dcs_rest_dphy(void)
{

    mipi_dsi_reset_dphy();
    mdelay(50);   // 1000
   mipi_txdphy_init();
    mdelay(50);   // 5000
//    printk("test end\n");
}

static void dsc_cmd_send_buff(uint8_t *buff, uint32_t len)
{
        uint32_t reg, val, val2;
        int cmd_sts;
        uint8_t i = 0;

        reg = 0x01003908 | (len << 16);                 //0x01003908
        DsiRegWr(DIRECT_CMD_MAIN_SETTINGS_OFFSET,reg); //[24] cmd_lp_en  =1    0x01043908
                                                        //[23:14] cmd_size  cmd_size = len
                                                        //[13:8] cmd_head  =0x39
                                                        //[3 ] cmd_longnotshort  =0x1
                                                        //[2:0] cmd_nat : 3'b000:write command
                                                        //                3'b001:read command
                                                        //                3'b100:TE request
                                                        //                3'b101:trigger request
                                                        //                3'b110:BTA request
        val = len / 4;       
        for(i = 0; i < val; i++)
        {
                DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,((buff[(4 * i) + 3]<<24) + (buff[(4 * i) + 2]<<16) + (buff[(4 * i) + 1]<<8) + buff[(4 * i)]));// DATA  
        }
        val2 = len % 4;
        if(val2 == 1)
                DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,buff[(4 * val)]);// DATA
        else if(val2 == 2)
                DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,((buff[(4 * val) + 1]<<8) + buff[(4 * val)]));// DATA
        else if(val2 == 3)
                DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,((buff[(4 * val) + 2]<<16) + (buff[(4 * val) + 1]<<8) + buff[(4 * val)]));// DATA

        DsiRegWr(DIRECT_CMD_SEND_OFFSET,0x00000); // cmd send
	while((cmd_sts & 0x02 )!= 0x02)
	{  
		cmd_sts = DsiRegRd(DIRECT_CMD_STS_OFFSET);
	}
	DsiRegWr(DIRECT_CMD_STS_CLR_OFFSET,0x02); // clear write_completed
}

static void dsc_cmd_send_short_pag(uint8_t val)
{
	uint8_t reg = 0;
	int cmd_sts;

   // reg = 0x01000500 | (val << 16);
	DsiRegWr(DIRECT_CMD_MAIN_SETTINGS_OFFSET,0x01010500); //[24] cmd_lp_en  =1    0x01010500
														//[23:14] cmd_size  =cmd_size. 1 or 2   //write 4
														//[13:8] cmd_head  =0x39
														//[3 ] cmd_longnotshort  =0x0  short pkg
														//[2:0] cmd_nat : 3'b000:write command
														//                3'b001:read command
														//                3'b100:TE request
														//                3'b101:trigger request
														//                3'b110:BTA request
	DsiRegWr(DIRECT_CMD_WRDAT_OFFSET,val);                         // DATA            ???                                    
	
	DsiRegWr(DIRECT_CMD_SEND_OFFSET,0x00000); // cmd send

	while((cmd_sts & 0x02 )!= 0x02)
	{  
		cmd_sts = DsiRegRd(DIRECT_CMD_STS_OFFSET);
	}
	DsiRegWr(DIRECT_CMD_STS_CLR_OFFSET,0x02); // clear write_completed
}

static void hx8399_c_init(void)
{
	uint8_t pag1[5] = {0xB9, 0xFF, 0x83, 0x99};                    
	uint8_t pag2[20] = {0xB1, 0x02, 0x04, 0x70, 0x90, 0x01, 0x32, 0x33, 0x11, 0x11, 0x4D, 0x57, 0x56, 0x73, 0x02, 0x02};           // 0x10
	uint8_t pag3[20] = {0xB2, 0x00, 0x80, 0x80, 0xAE, 0x05, 0x07, 0x5A, 0x11, 0x10, 0x10, 0x00};                     // 0c
//      uint8_t pag3[20] = {0xB2, 0x00, 0x80, 0x80, 0xAE, 0x05, 0x00, 0x5A, 0x11, 0x10, 0x10, 0x00};                     // 0c  0x3b- > 0b  03

	uint8_t pag4[50] = {0xB4, 0x00, 0xFF, 0x04, 0x08, 0x0C, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x02, 0x00, 0x24, 0x02, 0x04, 0x09, 0x21, 0x03, 0x00, 0x00, 0x0A, 0x90, 0x88, 0x04, 0x08, 0x0C, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x02, 0x00, 0x24, 0x02, 0x04, 0x08, 0x00, 0x00, 0x02, 0x88, 0x00, 0x08};
	uint8_t pag5[50] = {0xD3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x32, 0x10, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x05, 0x05, 0x13, 0x00, 0x00, 0x00, 0x05, 0x40, 0x00, 0x00, 0x00, 0x05, 0x20, 0x80};
	uint8_t pag6[50] = {0xD5, 0x00, 0x00, 0x21, 0x20, 0x19, 0x19, 0x18, 0x18, 0x00, 0x00, 0x01, 0x00, 0x18, 0x18, 0x03, 0x02, 0x19, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x31, 0x30, 0x30, 0x2F, 0x2F};
	uint8_t pag7[50] = {0xD6, 0x40, 0x40, 0x20, 0x21, 0x18, 0x18, 0x19, 0x19, 0x40, 0x40, 0x02, 0x03, 0x18, 0x18, 0x00, 0x01, 0x19, 0x19, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x31, 0x31, 0x30, 0x30, 0x2F, 0x2F};
	uint8_t pag8[20] = {0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t pag9[5] = {0xBD, 0x01};
	uint8_t pag10[20] = {0xD8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t pag11[5] = {0xBD, 0x02};
	uint8_t pag12[20] = {0xD8, 0xAA, 0xAE, 0xEA, 0xAA, 0xAA, 0xAE, 0xEA, 0xAA};
	uint8_t pag13[5] = {0xBD, 0x00};
	uint8_t pag14[60] = {0xE0, 0x01, 0x15, 0x22, 0x1E, 0x46, 0x51, 0x5E, 0x5A, 0x63, 0x6A, 0x71, 0x76, 0x7B, 0x82, 0x88, 0x8D, 0x92, 0x9C, 0xA0, 0xAB, 0xA2, 0xB5, 0xBD, 0x63, 0x61, 0x6E, 0x7A, 0x01, 0x15, 0x22, 0x1E, 0x46, 0x51, 0x5E, 0x5A, 0x63, 0x6A, 0x71, 0x76, 0x7B, 0x82, 0x89, 0x8E, 0x92, 0x9C, 0xA0, 0xAB, 0xA2, 0xB5, 0xBD, 0x63, 0x61, 0x6E, 0x7A};
	uint8_t pag15[5] = {0xC0, 0x25, 0x5A};
	uint8_t pag16[5] = {0xB6, 0x91, 0x91};
	uint8_t pag17[50] = {0xD2, 0x66};
	uint8_t pag18[50] = {0xCC, 0x08};
	uint8_t pag19[50] = {0x36, 0x08};

	uint8_t pag20[50] = {0xB2, 0x0b, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22};                // red


	dsc_cmd_send_buff(pag1, 4);                             // access the extension commands.
	mdelay(2);
	dsc_cmd_send_buff(pag2, 16);          //Set power related register
	mdelay(2);
	dsc_cmd_send_buff(pag3, 12);          // Set display related register
	mdelay(2);
	dsc_cmd_send_buff(pag4, 46);
	mdelay(2);
	dsc_cmd_send_buff(pag5, 40);     
	mdelay(2);                            
	dsc_cmd_send_buff(pag6, 33);
	mdelay(2);
	dsc_cmd_send_buff(pag7, 33);
	mdelay(2);
	dsc_cmd_send_buff(pag8, 17);        
	mdelay(2);            
	dsc_cmd_send(2, 0xBD, 0x01);

	dsc_cmd_send_buff(pag10, 17);
	mdelay(2);
	dsc_cmd_send(2, 0xBD, 0x02);
	mdelay(2);
	dsc_cmd_send_buff(pag12, 9);
	mdelay(2);
	dsc_cmd_send(2, 0xBD, 0x00);
	mdelay(2);
	dsc_cmd_send_buff(pag14, 55);
	mdelay(2);
//        dsc_cmd_send_3(0xC0, 0x25, 0x5A);
	dsc_cmd_send_buff(pag15, 3);
	mdelay(2);
	dsc_cmd_send_buff(pag16, 3);
//        dsc_cmd_send_3(0xB6, 0x91, 0x91);
	mdelay(2);
//        dsc_cmd_send_3(0xB6, 0x61, 0x03);                       // set 2 lan

	dsc_cmd_send(2, 0xD2, 0x66);
	mdelay(2);
	dsc_cmd_send(2, 0xCC, 0x08);   //0xCC, 0x08  
	mdelay(2);            
	dsc_cmd_send(2, 0x36, 0x03);  //0x36, 0x08
	mdelay(2);

	dsc_cmd_send_short_pag(0x11);
	mdelay(300);
	dsc_cmd_send_short_pag(0x29);
	mdelay(100);

}

static void mipi_dsi_rst(void)
{
    uint32_t ret;
    uint32_t *addr = ioremap(0x97002000 + 0x0164, 0x08);        // dsi rst ctl
    uint32_t *addr1= ioremap(0x97002000 + 0x017c, 0x08);        // vo rst ctl

    writel(0x80000000, addr);
    writel(0x80000000, addr1);
    mdelay(100);

    // rst dsi 
    writel(0x01, addr);
//    printk("++++++++++++++++++++++++++++++++++++++++++++++++++++ \n");
    writel(0x01, addr1);

    mdelay(100);

    ret = readl(addr);
//    printk("ret is %x \n", ret);

    ret = readl(addr1);
//    printk("ret is %x \n", ret);

    iounmap(addr);
    iounmap(addr1);

} 

/**
 * @brief 
 * 
 */
static void mipi_dsi_init(LCD lcd)
{
    int  HACT=1080;
    int  VACT=1920;
    int  HSA=20;
    int  HBP=20;
    int  HFP=220;
    int  VSA=5;
    int  VBP=8;
    int  VFP=5;
    int i = 0;
    uint32_t reg = 0;

    mipi_dsi_reset_dphy(); 
    mipi_dsi_rst();

    mipi_tx_dphy_5M_init();
    

    DsiRegWr(DPI_IRQ_EN_OFFSET, 0); //enable dpi overflow int

    DsiRegWr(MCTL_MAIN_PHY_CTL_OFFSET,0x3C17);    //[6]  lane1_ulp_en = 1
                                                            //[4]  clk_continuous  =1
                                                            //[2]  lane4_en =1
                                                            //[1]  lane3_en =1
                                                            //[0]  lane2_en =1
//        printk("MCTL_MAIN_PHY_CTL_OFFSET is %x \n",DsiRegRd(MCTL_MAIN_PHY_CTL_OFFSET));

    DsiRegWr(MCTL_DPHY_TIMEOUT1_OFFSET,0xed8afffb);
//        printk("MCTL_DPHY_TIMEOUT1_OFFSET is %x \n",DsiRegRd(MCTL_DPHY_TIMEOUT1_OFFSET));
    DsiRegWr(MCTL_DPHY_TIMEOUT2_OFFSET,0xf30fffff);
//        printk("MCTL_DPHY_TIMEOUT2_OFFSET is %x \n",DsiRegRd(MCTL_DPHY_TIMEOUT2_OFFSET));
    DsiRegWr(MCTL_MAIN_DATA_CTL_OFFSET,0x2e027); //[6] tvg_sel = 1  test video generator enabled   //default 0x27
                                                    //[5] vid_en = 1   enable the video stream generator
                                                    //[3:2] vid_if_select =2'b00  00:sdi;01:dpi��10:DSC
                                                    //[1] sdi_if_vid_mode = 1  select video mode
                                                    //[0] link_en = 1; // enable link
//        printk("MCTL_MAIN_DATA_CTL_OFFSET is %x \n",DsiRegRd(MCTL_MAIN_DATA_CTL_OFFSET));

    DsiRegWr(MCTL_MAIN_EN_OFFSET,0x40f9);
                                                    //[15]   if3_en: enable dsc interface
                                                    //[14]   if2_en: enable dpi interface   =1
                                                    //[13]   if1_en: enable sdi interface
                                                    //[9]   lane1_ulp_req =1
                                                    //[7]   dat4_en
                                            //[6]   dat3_en
                                            //[5]   dat2_en
                                            //[4]   dat1_en
                                            //[3]   cklane_en

//        printk("MCTL_MAIN_EN_OFFSET is %x \n",DsiRegRd(MCTL_MAIN_EN_OFFSET));
    //disable status detection
    unsigned int data = DsiRegRd(CMD_MODE_STS_CTL_OFFSET);
 //   printk("CMD_MODE_STS_CTL_OFFSET data 0x%x\n",data);
    data &= ~ (1 << 0); 
    DsiRegWr(CMD_MODE_STS_CTL_OFFSET,data );
                                                        

    wait_phy_pll_locked();
//    printk("phy pll locked ");

    wait_cklane_rdy();
//    printk("cklane  is ready!!! ");
    wait_dat1_rdy();
//    printk("dat1  is ready!!! ");
    wait_dat2_rdy();
//   printk("dat2  is ready!!! ");
    wait_dat3_rdy();
//    printk("dat3  is ready!!! ");
    wait_dat4_rdy();
//    printk("dat4  is ready!!! ");
    // will init lcd
    DsiRegWr(MCTL_ULPOUT_TIME_OFFSET,0x0003ab05);


    switch(lcd)
    {
        case LCD_ILI7807:
            ILI7807_lcd_init();
            break;
        case LCD_HX8399:
            hx8399_c_init();
            break;
        
        case LCD_AML:
            aml_lcd_init();                                         // lcd init 
            break;
        
        default:

            break;
    }

    if(lcd != LCD_HX8399)
    {
        dcs_rest_dphy();
    }
    

    printk("lcd init  ok  lcd is %d  ---------------------------------!!! \n", lcd); 

    DsiRegWr(TVG_IMG_SIZE_OFFSET,(VACT <<16) +HACT*3);
    DsiRegWr(TVG_COLOR1_OFFSET,(0<<12) +255)  ;  //[23:12] col1_green
                                                            //[11:0]  col1_red
    DsiRegWr(TVG_COLOR1_BIS_OFFSET,0)  ; //[11:0]  col1_blue
    DsiRegWr(TVG_COLOR2_OFFSET,(0<<12) +0) ;   //[23:12] col2_green
                                                            //[11:0]  col2_red
    DsiRegWr(TVG_COLOR2_BIS_OFFSET,255) ;  //[11:0]  col2_blue


    DsiRegWr(VID_VSIZE1_OFFSET,(VFP<<12)+(VBP<<6)+VSA);
                                    //[19:12] vfp_length
                                    //[11:6]  vbp_length
                                    //[5:0]   vsa_length
    DsiRegWr(VID_MODE_STS_CTL_OFFSET,0);//0xffffffff);
    DsiRegWr(VID_VSIZE2_OFFSET,VACT); //[12:0] vact_length
    DsiRegWr(VID_HSIZE1_OFFSET,((HBP*3-12)<<16)+(HSA*3-14));
                                            //[31:16] hbp_length  =(dpi_hbp*24/8-12)
                                            //[9:0]   hsa_length  =(dpi_hsa*24/8-14)

    DsiRegWr(VID_HSIZE2_OFFSET,((HFP*3-6)<<16)+(HACT*3));
                                            //[26:16] hfp_length  =(dpi_hfp*24/8-6) >hss+hsa+hse+hbp
                                            //[14:0] rgb_size
    DsiRegWr(VID_BLKSIZE2_OFFSET,(HSA+HBP+HFP+HACT)*3-20-(HSA*3-14));
                                            //[14:0] blkline_pulse_pck = (dpi_hsa+dpi_hbp+dpi_hact+dpi_hfp)*24/8-20-((dpi_hsa*24/8)-14)=3342
                                            //  (1080+20+15+72)*3-20= 3541
    DsiRegWr(VID_DPHY_TIME_OFFSET,(0x38<<17)+((HSA+HBP+HFP+HACT)*3/4)-((HSA*3-14)/4));
                                            //[16:0] reg_line_duration =(line_total_)
                                            //[27:17] reg_wakeup_time
    DsiRegWr(VID_VCA_SETTING1_OFFSET,(0x000000));
    DsiRegWr(VID_VCA_SETTING2_OFFSET,((HSA+HBP+HFP+HACT)*3-20-(HSA*3-14)-6)<<16);
                                            //[31:16] max_line_limit = gen_blkline_pulse_pck-6;
    DsiRegWr(VID_MAIN_CTL_OFFSET,(0x1<<31)+(0x1<<25)+(0x0<<21)+(1<<20)+(1<<19)+(3<<14)+(0x3e<<8)+0); //0x1 << 21

}

static void csi_rst(void)
{
    uint32_t ret;
    uint32_t *addr = ioremap(0x97002000 + 0x0134, 0x08);        // isp f-4k rst ctl
    uint32_t *addr1= ioremap(0x97002000 + 0x0138, 0x08);        // isp f-2k rst ctl
//    uint32_t *addr2= ioremap(0x97002000 + 0x013c, 0x08);        // isp r-2k rst ctl
//    uint32_t *addr3= ioremap(0x97002000 + 0x0140, 0x08);        // isp tof rst ctl

    writel(0x80000000, addr);
    writel(0x80000000, addr1);
//    writel(0x80000000, addr2);
//    writel(0x80000000, addr3);
    mdelay(100);

    // rst dsi 
    writel(0x01, addr);
    writel(0x01, addr1);
//    writel(0x01, addr2);
//    writel(0x01, addr3);

    mdelay(100);

    ret = readl(addr);
//    printk("ret is %x \n", ret);

    ret = readl(addr1);
//    printk("ret is %x \n", ret);

    iounmap(addr);
    iounmap(addr1);
//    iounmap(addr2);
//    iounmap(addr3);
}
static void isp_rst(void)
{
    uint32_t ret;
    uint32_t *addr = ioremap(0x97002000 + 0x0120, 0x08);        // isp f-4k rst ctl
    uint32_t *addr1= ioremap(0x97002000 + 0x0124, 0x08);        // isp f-2k rst ctl
    uint32_t *addr2= ioremap(0x97002000 + 0x0128, 0x08);        // isp r-2k rst ctl
    uint32_t *addr3= ioremap(0x97002000 + 0x012c, 0x08);        // isp tof rst ctl

    writel(0x80000000, addr);
    writel(0x80000000, addr1);
    writel(0x80000000, addr2);
    writel(0x80000000, addr3);
    mdelay(100);

    // rst dsi 
    writel(0x01, addr);
    writel(0x01, addr1);
    writel(0x01, addr2);
    writel(0x01, addr3);

    mdelay(100);

    ret = readl(addr);
//    printk("ret is %x \n", ret);

    ret = readl(addr1);
//    printk("ret is %x \n", ret);

    iounmap(addr);
    iounmap(addr1);
    iounmap(addr2);
    iounmap(addr3);
}

static long isp_ioctl(struct file *filp, unsigned int cmd,
                             unsigned long arg)
{
    uint32_t y_src, u_src;

    switch (cmd) {

        case ISP_CMD_DS1_ADDR:
            plat->ds1_addr = arg;
            ds_set_addr_flag = 1;
            printk("plat->ds1_addr is%x \n", plat->ds1_addr);            
            return 0;

        case ISP_CMD_DS1_BUFF_COUNT:  
            plat->ds1_buf_cut = arg;
            printk("plat->ds1_buf_cut is %x \n", plat->ds1_buf_cut);
            return 0;
        
        case ISP_CMD_DS0_SRC_ADDR:  
            plat->ds0_src_addr = arg;
            plat->ds0_buf_cut = 2;
            ds_set_addr_flag = 1;
            printk("plat->ds0_src_addr is %x \n", plat->ds0_src_addr);
            return 0;
        
        case ISP_CMD_SET_DS1_SIZE :
            fram_uv_addr = arg;
            printk(" ISP_CMD_SET_DS1_SIZE fram_uv_addr is %x \n", fram_uv_addr);
            return 0;
        
        case ISP_CMD_DS1_USE_IRQ :
            ds1_use_irq = arg;
            printk(" ISP_CMD_DS1_USE_IRQ ds1_use_irq is %d \n", ds1_use_irq);
            return 0;
        
        case ISP_CMD_DS0_USE_TWOD:
            ds0_use_twod = arg;
            printk(" ISP_CMD_DS0_USE_TWOD ds0_use_twod is %d \n", ds0_use_twod);
            return 0;
        
        case ISP_CMD_CLEAR_FLAG:
            ds1_cut = 0;
            ds0_cut = 0;
            vo_cut = 0;
            return 0;
        
		case VIDEO_CLOSE_ALL_LAYER:
			video_close_all_layer();
			return 0;
			
        case ISP_CMD_DS1_DES_ADDR:  
            plat->ds0_des_addr = arg;

            printk("plat->ds0_des_addr is %x \n", plat->ds0_des_addr);
            return 0;
        
        case ISP_CMD_TWOD_TEST_ADDR:
#if 0
            y_src = plat->ds0_src_addr + (1080 - 1) * 1920;
            u_src = (plat->ds0_src_addr + (1080 * 1920) ) + (1080 / 2 - 1) * 1920;
                
            kendrty_set_rot_frame_src_addr(y_src, u_src, 0);     // set src base
            kendrty_set_rot_frame_des_addr(plat->ds0_des_addr, plat->ds0_des_addr + fram_uv_addr, 0);

            kendrty_set_rot_frame_run(1);
            kendrty_set_rot_frame_run(0);
#endif
            return 0;
        
        case ISP_CMD_ISP_RESET:
            isp_rst();
            csi_rst();
            return 0;

        case ISP_CMD_READ_REG:
                return isp_read_reg(arg);
        case ISP_CMD_WRITE_REG:
                return isp_write_reg(arg);
        case ISP_CMD_ACT_SENSOR_RST:
                return isp_act_sensor_rst(arg);                         //open sensor
        case ISP_CMD_RLS_SENSOR_RST:
                return isp_rls_sensor_rst(arg);                         //close sensor
        //dvp 
        case DVP_CMD_ACT_SENSOR_RST:   
                return dvp_act_sensor_rst(arg);                               
        //dvp 
        case  DVP_CMD_RLS_SENSOR_RST:
                return dvp_rls_sensor_rst(arg);

        case ISP_CMD_SET_DMABUF_Y_FD:
                return isp_set_dmabuf_y_fd(arg);
        case ISP_CMD_SET_DMABUF_UV_FD:
                return isp_set_dmabuf_uv_fd(arg);

        case ISP_CMD_MIPI_DSI_INIT:
                mipi_dsi_init(LCD_AML);
                printk("mipi_dsi_init done\n");
                //mipi_rx_dphy_init(RXDPHY_SPEED_MODE_1500M, RXDPHY_CHCFG_1X4);
				mipi_rx_dphy_init(RXDPHY_SPEED_MODE_1500M, RXDPHY_CHCFG_2X2);
                printk("mipi_rx_dphy_init done!\n");
                return 0;
                
        case ISP_CMD_MIPI_DSI_ILI7807_INIT:
//                mipi_dsi_iliI7807_init();
                mipi_dsi_init(LCD_ILI7807);
                printk("mipi_dsi_init done\n");
                //mipi_rx_dphy_init(RXDPHY_SPEED_MODE_1500M, RXDPHY_CHCFG_1X4);
				mipi_rx_dphy_init(RXDPHY_SPEED_MODE_1500M, RXDPHY_CHCFG_2X2);
                printk("mipi_rx_dphy_init done!\n");
                return 0;
        
        case ISP_CMD_MIPI_DSI_HX8399_INIT:
//                mipi_dsi_hx8399_init();
                mipi_dsi_init(LCD_HX8399);
                printk("mipi_dsi_init done\n");
                //mipi_rx_dphy_init(RXDPHY_SPEED_MODE_1500M, RXDPHY_CHCFG_1X4);
				mipi_rx_dphy_init(RXDPHY_SPEED_MODE_1500M, RXDPHY_CHCFG_2X2);
                printk("mipi_rx_dphy_init done!\n");
                return 0;
        
        case ISP_CMD_SET_DS2BUF:
        		ds_set_addr_flag = 1;
                return ds2_buf_init(arg);
        case ISP_CMD_GET_DS2BUF:
                return ds2_get_buf(arg);
        case ISP_CMD_EN_DS2_INT:
                return ds2_en_int(arg);             

        default:
                printk("isp_ioctl: Unknown ioctl: 0x%.8X\n", cmd);
                return -EINVAL;
    }

}

static long isp_compat_ioctl(struct file *file, unsigned int cmd,
                                    unsigned long arg)
{
        long ret = -ENOIOCTLCMD;

        if (file->f_op->unlocked_ioctl)
                ret = file->f_op->unlocked_ioctl(file, cmd, arg);

        return ret;
}


static int isp_mmap(struct file *file, struct vm_area_struct *vma)
{    
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    if(remap_pfn_range(vma,
                        vma->vm_start,
                        (0x50000000 >> PAGE_SHIFT) | 0x100000,
                        vma->vm_end - vma->vm_start,
                        vma->vm_page_prot))
    {          
        printk("isp_mmap: remap_pfn_range fail \n");
        return -EAGAIN;
    }
    return 0;
}

static unsigned int isp_poll(struct file *file, poll_table *wait)
{
    unsigned int ret = 0;
//    printk("wait isp_poll  start \n ");
    poll_wait(file, &isp_waitq, wait);//将当前进程放到gnne_waitq列表
    if(isp_int_flag) {
        ret |= POLLIN;//说明有数据被取到了
        isp_int_flag = 0;
    }
//    printk("wait isp_poll ret is %d end\n ", ret);
    return ret;
}

static ssize_t isp_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{

//    printk("start isp_read \n ");
    if(copy_to_user(buf, plat->ds_data, size))
    {
        return -EFAULT;
    }

//    printk("start isp_read success \n ");

    return 1;
}

const struct file_operations isp_fops = {
    .owner          = THIS_MODULE,
    .open           = isp_open,
    .read           = isp_read,
    .release        = isp_release,
    .unlocked_ioctl = isp_ioctl,
    .compat_ioctl   = isp_compat_ioctl,
    .poll           = isp_poll,
    .mmap           = isp_mmap,
};


int setup_isp_cdev(const char *device_name)
{
        int err, devno =
                MKDEV(plat->major, plat->minor);

        cdev_init(&plat->cdev, &isp_fops);
        plat->cdev.owner = THIS_MODULE;
        err = cdev_add(&plat->cdev, devno, 1);
        if (err) {
                printk("isp: Error %d adding isp device number %d \n", err, plat->minor);
                return err;
        }

        if (device_name != NULL) {
                device = device_create(plat->class, NULL, devno, NULL,
                                       device_name);
                if (IS_ERR(device)) {
                        printk("isp: device not created\n");
                        clean_up_isp_cdev();
                        return PTR_ERR(device);
                }
        }

        return 0;
}



static void mipi_txdphy_set_pll(uint32_t fbdiv, uint32_t refdiv, uint32_t outdiv)
{
    uint32_t wdata, rdata;
    uint32_t bwadj = fbdiv/2;

    //judge if vco output is beyond range.
    uint32_t vco = 25*(fbdiv+1)/(refdiv+1);

    uint32_t *addr = ioremap(VIDEO_TXDPHY_BASE_ADDR + 0x8, 0x08);
    uint32_t *addr2 = ioremap(VIDEO_TXDPHY_BASE_ADDR + 0x4, 0x08);
    uint32_t *addr3 = ioremap(VIDEO_TXDPHY_BASE_ADDR + 0x14, 0x08);
    uint32_t *addr4 = ioremap(VIDEO_DSI_BASE_ADDR + 0x10, 0x08);


    if(vco < 720 || vco > 3600)
    {
        printk("Error: mipi txdphy pll vco is out of range!\n");
        return;    
    }

    //assert dphy reset
    wdata = readl(addr4); 
    wdata &= (~(1 << 20));       
    writel(wdata, addr4);

    //power-down mipi pll and reset pll
    wdata = (1 << 31) /*pll reset */
                | (1 << 30) /*pwr down */
                | (1 << 28) /*PLL fast-lock */
                | (1 << 27) /* ENSAT*/
                | (1 << 25) /* PLL Override*/
                | (outdiv << 21) /*PLL D */
                | (0 << 19) /*lane0_sel */
                | (1 << 17) /*lane1_sel */
                | (2 << 15) /*lane2_sel */
                | (3 << 13) /*lane3_sel */;
                    
    writel(wdata, addr);

    //set pll para
    wdata = (refdiv << 25) /* CLKR */
                | (fbdiv << 12) /* CLKF*/
                | (bwadj) /*BWADJ*/; 

    
    writel(wdata, addr2);
                
    //power-on mipi pll   
    wdata = readl(addr);
    wdata &= (~(1 << 30));
    writel(wdata, addr);

    //sleep 5us
    udelay(5);
    
    
    //de-assert pll reset 
    wdata = readl(addr);
    wdata &= (~(1 << 31));
    writel(wdata, addr);

    //wait for pll lock
    rdata = 0;
    while((rdata & 0x4) != 0x4)
    {
        rdata =readl(addr3);
    }

    //de-assert dphy reset
    wdata = readl(addr4); 
    wdata |= (1 << 20);       
    writel(wdata, addr4);
    
    iounmap(addr);
    iounmap(addr2);
    iounmap(addr3);
    iounmap(addr4);
}
/**
 * @brief 
 * 
 */
void SYSCTL_DRV_Init(void)
{
    //1. init csi pix clk
    //a. enable clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_CSI_0_PIXEL, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_CSI_1_PIXEL, 1);
    
    //b. set pix clk to 74.25M by default
    sysctl_clk_set_leaf_div(SYSCTL_CLK_CSI_0_PIXEL,1, 4);//8);
    sysctl_clk_set_leaf_div(SYSCTL_CLK_CSI_1_PIXEL,1, 4);//8);

    //2. init csi system clk    
    //a. enable clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_CSI_0_SYSTEM, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_CSI_1_SYSTEM, 1);
    
    //b. set system clk to 166M by default
    sysctl_clk_set_leaf_div(SYSCTL_CLK_CSI_0_SYSTEM,1, 1);//2);
    sysctl_clk_set_leaf_div(SYSCTL_CLK_CSI_1_SYSTEM,1, 1);//2);

    //3. init dsi pix clk    
    //a. enable clk
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISPLAY_PIXEL, 1);
    
    //b. set pix clk to 74.25M by default
//    sysctl_clk_set_leaf_div(SYSCTL_CLK_DISPLAY_PIXEL,1, 8);   ///8    4
    
    //4. init dsi system clk    
    //a. enable clk
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_DSI_SYSTEM, 1);
    
    //b. set system clk to 166M by default, all display subsystem apb clk will be set to 166M together.
//    sysctl_clk_set_leaf_div(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV,1, 2);

    //5. init tpg pix clk
    //a. enable clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_TPG_PIXEL, 1);
    
    //b. set tpg pix clk to 594/5=118.8M by default
    sysctl_clk_set_leaf_div(SYSCTL_CLK_TPG_PIXEL,1, 7);//5);

    //6. enable video subsystem apb and axi clk
    //enable video apb clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_CSI_0_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_CSI_1_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_CSI_2_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_CSI_3_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_F2K_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_R2K_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TOF_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_MFBC_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_VI_APB, 1);
    

    //enable video axi clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_VI_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_ISP_F2K_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_ISP_R2K_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_ISP_TOF_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_MFBC_AXI, 1); 

    //7. enable display subsystem apb and axi clk
    //enable display apb clk
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_DSI_APB, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_VO_APB, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_TWOD_APB, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_DISP_SYS_AND_APB_CLK_DIV_BT1120_APB, 1);

    //enable display axi clk
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_VO_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);
    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_MFBC_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);
//    sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_TWOD_AXI, 1);

}


static void mipi_init(void)
{

//    printk("before SYSCTL_DRV_Init\n");

    SYSCTL_DRV_Init();

//    printk("SYSCTL_DRV_Init done\n");
    //mipi_rx_dphy_init(RXDPHY_SPEED_MODE_1500M, RXDPHY_CHCFG_1X4);
	mipi_rx_dphy_init(RXDPHY_SPEED_MODE_1500M, RXDPHY_CHCFG_2X2);
    printk("mipi_rxdphy_init done\n");

//    mipi_txdphy_init();


}

static const char isp_name[] = "test-isp";

int isp_probe(struct platform_device *pdev)
{
    struct resource *res;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
            printk("isp_probe: get resource failed \n");
            return -ENODEV;
    }

    plat = kzalloc(sizeof(struct isp_plat), GFP_KERNEL);
    if (!plat) {
        printk("isp_probe: kzalloc failed \n");  
        return -ENOMEM;
    }

    plat->ds_data = kzalloc(sizeof(struct ds_data), GFP_KERNEL);
    if (!plat->ds_data) {
        printk("plat->ds_data: kzalloc failed \n");  
        return -ENOMEM;
    }

    plat->regs = ioremap(res->start, resource_size(res));
    if(!plat->regs) {
        printk("isp_probe: ioremap failed \n");
        kfree(plat);
        return -ENOMEM;
    }
    
    plat->regs_size = res->end - res->start;
    plat->major = 0;
    plat->minor = 0;

    plat->gpio = ioremap(0x97050000, 0x1000);
    if(!plat->gpio) {
        printk("isp_probe: gpio ioremap failed \n");
        kfree(plat);
        return -ENOMEM;
    }

    plat->iomux = ioremap(0x97040000, 0x1000);
    if(!plat->iomux) {
        printk("isp_probe: iomux ioremap failed \n");
        kfree(plat);
        return -ENOMEM;
    }               
    printk("isp probe ok, mapped %llx at %px, mapped size %lld \n", res->start, plat->regs, resource_size(res));
    
    //get mipi csi dsi gpio
    plat->dsi_en = devm_gpiod_get(&pdev->dev, "dsi_en", GPIOD_OUT_HIGH);
    if(IS_ERR( plat->dsi_en))
    {
        printk("get dsi_en err \n");
        return -EINVAL;
    }

    plat->dsi_rst = devm_gpiod_get(&pdev->dev, "dsi_rest", GPIOD_OUT_LOW);
    if(IS_ERR( plat->dsi_rst))
    {
        printk("get dsi_rst err \n");
        return -EINVAL;
    }

    plat->csi_en = devm_gpiod_get(&pdev->dev, "csi_en", GPIOD_OUT_HIGH);
    if(IS_ERR( plat->csi_en))
    {
        printk("get csi_en err \n");
//        return -EINVAL;
    }

#if 0
    plat->imx385_powerdown = devm_gpiod_get(&pdev->dev, "imx385_powerdown", GPIOD_OUT_HIGH);
    if(IS_ERR( plat->imx385_powerdown))
    {
        printk("get imx385_powerdown err \n");
//        return -EINVAL;
    }

    plat->imx385_reset = devm_gpiod_get(&pdev->dev, "imx385_reset", GPIOD_OUT_HIGH);
    if(IS_ERR( plat->imx385_reset))
    {
        printk("get imx385_reset err \n");
        // return -EINVAL;
    }

    plat->irs3281_powerdown = devm_gpiod_get(&pdev->dev, "irs3281_powerdown", GPIOD_OUT_HIGH);
    if(IS_ERR( plat->irs3281_powerdown))
    {
        printk("get irs3281_powerdown err \n");
        // return -EINVAL;
    }

    plat->irs3281_reset = devm_gpiod_get(&pdev->dev, "irs3281_reset", GPIOD_OUT_HIGH);
    if(IS_ERR( plat->irs3281_reset))
    {
        printk("get irs3281_reset err \n");
        // return -EINVAL;
    }

    plat->dvp_powerdown = devm_gpiod_get(&pdev->dev, "dvp_powerdown", GPIOD_OUT_HIGH);
    if(IS_ERR( plat->dvp_powerdown))
    {
        printk("get dvp_powerdown err \n");
        // return -EINVAL;
    }
#endif
    

    printk("isp gpio get succes \n");
    // rst dsi sensor

#if 0
    mdelay(50);
//    gpiod_set_value(plat->dsi_rst, 1);
    gpiod_set_value_cansleep(plat->dsi_rst, 1);
    mdelay(50);
//    gpiod_set_value(plat->dsi_rst, 0);
    gpiod_set_value_cansleep(plat->dsi_rst, 0);
#endif

    printk("mipi dsi rst succcess \n");

    #ifdef CANAAN_440
    printk("iomux 86, 0x%x\n", ioread32(plat->iomux + 86*4));
    printk("iomux 87, 0x%x\n", ioread32(plat->iomux + 87*4));

    printk("iomux 88, 0x%x\n", ioread32(plat->iomux + 88*4));
    printk("iomux 89, 0x%x\n", ioread32(plat->iomux + 89*4));
    printk("iomux 90, 0x%x\n", ioread32(plat->iomux + 90*4));
    printk("iomux 91, 0x%x\n", ioread32(plat->iomux + 91*4));

    iowrite32(0x800a0cce, plat->iomux + 86*4);
    iowrite32(0x80430eef, plat->iomux + 87*4);

    iowrite32(0x80420eef, plat->iomux + 88*4);
    iowrite32(0x80420eef, plat->iomux + 89*4); //?
    
    iowrite32(0x80440eef, plat->iomux + 90*4);
    iowrite32(0x80450eef, plat->iomux + 91*4);

    iowrite32(0x3f, plat->gpio + 4);

    iowrite32(0x3f, plat->gpio + 4);
    #endif

    #ifdef EVBLP3
    // iomux   
    printk("iomux 125, 0x%x\n", ioread32(plat->iomux + 125*4));
    printk("iomux 123, 0x%x\n", ioread32(plat->iomux + 123*4));

    printk("iomux 105, 0x%x\n", ioread32(plat->iomux + 105*4));
    printk("iomux 107, 0x%x\n", ioread32(plat->iomux + 107*4));
    //DVP
    //iowrite32(0x80030094, plat->iomux + 47*4);/*DVP0*/                                
    //iowrite32(0x80030094, plat->iomux + 48*4);/*DVP1*/
    //iowrite32(0x80030094, plat->iomux + 49*4);/*DVP2*/
    //iowrite32(0x80030094, plat->iomux + 50*4);/*DVP3*/
    //iowrite32(0x80030094, plat->iomux + 51*4);/*DVP4*/
    //iowrite32(0x80030094, plat->iomux + 52*4);/*DVP5*/
    //iowrite32(0x80030094, plat->iomux + 53*4);/*DVP6*/
    //iowrite32(0x80030094, plat->iomux + 54*4);/*DVP7*/
    //iowrite32(0x80030094, plat->iomux + 55*4);/*DVP8*/
    //iowrite32(0x80030094, plat->iomux + 56*4);/*DVP9*/
    //iowrite32(0x80030094, plat->iomux + 57*4);/*DVP10*/
    //iowrite32(0x80030094, plat->iomux + 58*4);/*DVP11*/
    //iowrite32(0x80030094, plat->iomux + 59*4);/*DVP12*/
    //iowrite32(0x80030094, plat->iomux + 60*4);/*DVP13*/
    //iowrite32(0x80030094, plat->iomux + 61*4);/*DVP14*/    
    //iowrite32(0x80030094, plat->iomux + 62*4);/*DVP15*/
    //iowrite32(0x80030094, plat->iomux + 63*4);/*DVP VSYNC*/
    //iowrite32(0x80030094, plat->iomux + 64*4);/*DVP HREF*/
    //iowrite32(0x80030094, plat->iomux + 65*4);/*DVP DEN*/
    //iowrite32(0x80030094, plat->iomux + 66*4);/*DVP PCLK*/
    // bt1120
    iowrite32(0x8000014e, plat->iomux + 67*4);/*bt1120_out_data_y[0]*/
    iowrite32(0x8000014e, plat->iomux + 68*4);/*bt1120_out_data_y[1]*/
    iowrite32(0x8000014e, plat->iomux + 69*4);/*bt1120_out_data_y[2]*/
    iowrite32(0x8000014e, plat->iomux + 70*4);/*bt1120_out_data_y[3]*/
    iowrite32(0x8000014e, plat->iomux + 71*4);/*bt1120_out_data_y[4]*/
    iowrite32(0x8000014e, plat->iomux + 72*4);/*bt1120_out_data_y[5]*/
    iowrite32(0x8000014e, plat->iomux + 73*4);/*bt1120_out_data_y[6]*/
    iowrite32(0x8000014e, plat->iomux + 74*4);/*bt1120_out_data_y[7]*/
    iowrite32(0x8000014e, plat->iomux + 75*4);/*bt1120_out_data_c[0]*/
    iowrite32(0x8000014e, plat->iomux + 76*4);/*bt1120_out_data_c[1]*/
    iowrite32(0x8000014e, plat->iomux + 77*4);/*bt1120_out_data_c[2]*/
    iowrite32(0x8000014e, plat->iomux + 78*4);/*bt1120_out_data_c[3]*/
    iowrite32(0x8000014e, plat->iomux + 79*4);/*bt1120_out_data_c[4]*/
    iowrite32(0x8000014e, plat->iomux + 80*4);/*bt1120_out_data_c[5]*/
    iowrite32(0x8000014e, plat->iomux + 81*4);/*bt1120_out_data_c[6]*/
    iowrite32(0x8000014e, plat->iomux + 82*4);/*bt1120_out_data_c[7]*/
    iowrite32(0x8000014e, plat->iomux + 83*4);/*bt1120_out_clk*/
    iowrite32(0x8000014e, plat->iomux + 84*4);/*bt1120_out_vsync*/
    iowrite32(0x8000014e, plat->iomux + 85*4);/*bt1120_out_hsync*/

//        printk("iomux 65, 0x%x\n", ioread32(plat->iomux +  65*4));

//        printk("iomux 28, 0x%x\n", ioread32(plat->iomux + 28*4));
//       printk("iomux 27, 0x%x\n", ioread32(plat->iomux + 27*4));

/*
    iowrite32(0x800a0cce, plat->iomux + 125*4);                     //gpio0                 csi0 imx385 powerdown          
    iowrite32(0x800b0cce, plat->iomux + 123*4);                     //gpio1                 csi0 imx385 reset

    iowrite32(0x800c0cce, plat->iomux + 105*4);                     //gpio2                 csi1 opn8008/irs3281 powerdown
    iowrite32(0x800d0cce, plat->iomux + 107*4);                      //gpio3 

    iowrite32(0x800e0cce, plat->iomux + 65*4);                      //gpio4                 csi1 opn8008/irs3281 reset
    
//        iowrite32(0x80440eef, plat->iomux + 28*4);
//        iowrite32(0x80450eef, plat->iomux + 27*4);

    iowrite32(0x1f, plat->gpio + 4);                                 // 5 gpio  set output
*/

    #endif
    ////////////
    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res == NULL) {
            printk("isp no IRQ resource found\n");
    }
    else
            printk("isp has IRQ resource, irq %d\n", res->start);
        
    int irq = res->start;

    int ret = 0;
    
#if 1 
    // enable isp intr
    plat->irq_ds2 = platform_get_irq(pdev, 0);               
    if (plat->irq_ds2 < 0) {
            dev_err(&pdev->dev, "no irq for alarm\n");
            return plat->irq_ds2;
    }

    ret = devm_request_irq(&pdev->dev, plat->irq_ds2, isp_irq_handler,
                                IRQF_SHARED, "kendryte isp-f2k", plat);
    if (ret) {
        dev_err(&pdev->dev, "IRQ%d error %d\n", plat->irq_ds2, ret);
        return -1;
    }

    // enable vo intr
    plat->irq_vo = platform_get_irq(pdev, 1);  

    ret = devm_request_irq(&pdev->dev, plat->irq_vo, vo_irq_handler,
                                IRQF_SHARED, "kendryte vo", plat);
    if (ret) {
        dev_err(&pdev->dev, "IRQ%d error %d\n", plat->irq_vo, ret);
        return -1;
    }

    printk("lat->irq_vo is %d ,plat->irq_ds2 is %d \n", plat->irq_vo, plat->irq_ds2);

#endif 
//        ret = request_irq(irq, isp_irq_handler, IRQF_SHARED, isp_name, &dev_id);

    //ret = request_irq(irq, isp_irq_handler, 0, isp_name, NULL);
//    printk("request irq %d  ret %d\n", plat->irq_ds2, ret);


    setup_chrdev_region();
    create_module_class();

    setup_isp_cdev("test-isp");

    sysctl_clk = ioremap(0x97000000+0x1000, 0x1000);					// sysclk base addr
    mipi_init();
        
	return 0;
}

int isp_remove(struct platform_device *pdev)
{
	struct resource *res;
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res == NULL) {
        printk("isp no IRQ resource found\n");
    }
    else
        printk("isp has IRQ resource, irq %d\n", res->start);
    
    int irq = res->start;
    dev_t dev = MKDEV(plat->major, plat->minor);

    device_destroy(plat->class, dev);
    clean_up_isp_cdev();
    kfree(plat);
    free_irq(irq, &dev_id);

    return 0;
}


static const struct of_device_id test_isp_ids[] = {
	{ .compatible = "canaan,k510-isp" },
	{}
};

static struct platform_driver test_isp_driver = {
    .probe          = isp_probe,
    .remove         = isp_remove,
    .driver         = {
            .name           = "test-isp",
            .of_match_table = of_match_ptr(test_isp_ids),
    },
};

#if 0
static int __init my_isp_module_init(void)
{
        int ret;
		
        return ret;
}

static void __exit my_isp_module_deinit(void)
{
        
}

#endif 

static int __init hello_init(void){
    int ret;

    printk("this is hello word\n");

    ret = platform_driver_register(&test_isp_driver);
    return 0;
}


static void __exit hello_exit(void){
    printk("remove hello word \n");

    platform_driver_unregister(&test_isp_driver);
}

// late_initcall(hello_init);
module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");


