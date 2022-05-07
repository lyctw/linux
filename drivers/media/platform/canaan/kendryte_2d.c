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
#include <linux/clk.h>

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
#include <linux/string.h>  
#include <linux/jiffies.h>


#include "kendryte_2d.h"

//#include "sysctl_clk.h"

static const char kendrty_2d_name[] = "kendrty_2d";
static struct class * kendrty_2d_class;
static int kendrty_2d_major;


#define KENDRTY_2DROTATION_90    _IOWR('k', 0, unsigned long)
#define KENDRTY_2DROTATION_270    _IOWR('k', 1, unsigned long)

#define KENDRTY_2DROTATION_INPUT_ADDR    _IOWR('k', 2, unsigned long)
#define KENDRTY_2DROTATION_OUTPUT_ADDR    _IOWR('k', 3, unsigned long)
#define KENDRTY_2DROTATION_GET_REG_VAL    _IOWR('k', 4, unsigned long)




enum  Rotation_angle {
    ANGLE_90 = 0,
    ANGLE_270 = 1,
};

enum  WRAP_INT_ST {
    TD_INT_ST_ROT_DONE = 0,
    TD_INT_ST_ROT_TIMEOUT = 1,
    TD_INT_ST_MIX_DONE = 2,
    TD_INT_MASK_ROT_DONE = 3,
    TD_INT_MASK_ROT_TIMEOUT = 4,
    TD_INT_MASK_MIX_DONE = 5,
    TD_INT_MASK_DEBUG = 6,
};

enum  ROT_RADA_FUNC {
    TD_ROT_FRM_END = 0,
    TD_ROT_DONE_ST = 1,
    TD_ROT_FSM_ST_RD = 2,
    TD_ROT_FSM_ST_WR = 3,
    TD_ROT_CYCLE_CNT = 4,
};



static struct kendrty_2d {

    struct device   *dev;
    void __iomem    *base;
    int             irq_fram;
    struct cdev     cdev;
    struct clk      *twod_apb;
    struct clk      *twod_axi;

    uint32_t input_addr;
    uint32_t output_addr;

    uint32_t h_active;
    uint32_t v_active;

    uint32_t y_src_stride;
    uint32_t u_src_stride;

    uint32_t y_des_stride;
    uint32_t u_des_stride;

    enum Rotation_angle rotation;  

    struct twod_data data;  
    int vo_cut;    
};

struct kendrty_2d *twod_test = NULL;


unsigned long  timer1 = 0;
unsigned long  timer2 = 0;

EXPORT_SYMBOL(timer1);
EXPORT_SYMBOL(timer2);



static void kendrty_2d_set_wrap_intr_status(struct kendrty_2d *kendrty_2d, enum WRAP_INT_ST status)
{
    uint32_t reg = 0;
    
    reg = readl(kendrty_2d->base + 0x7c);

    switch(status)
    {
        case TD_INT_ST_ROT_DONE:
            
            reg = reg | 0x01;
            writel(reg, kendrty_2d->base + 0x7c);
            break;

        case TD_INT_ST_ROT_TIMEOUT:
            reg = reg | 0x02;
            writel(reg, kendrty_2d->base + 0x7c);
            break;
        
        case TD_INT_ST_MIX_DONE:
            reg = reg | 0x04;
            writel(reg, kendrty_2d->base + 0x7c);
            break;
   
        default:
            break;
    }
}

static void kendrty_2d_intr_ctl(struct kendrty_2d *kendrty_2d, enum WRAP_INT_ST intr, uint8_t status)
{
    int ret = -1;
    uint32_t reg = 0;

    reg = readl(kendrty_2d->base + 0x7c);

    switch(intr)
    {
        case TD_INT_MASK_ROT_DONE:
            
            reg = (reg & ~(BIT_MASK(16))) | (status << 16);
            writel(reg, kendrty_2d->base + 0x7c);
            break;

        case TD_INT_MASK_ROT_TIMEOUT:
            reg = (reg & ~(BIT_MASK(17))) | (status << 17);
            writel(reg, kendrty_2d->base + 0x7c);
            break;
        
        case TD_INT_MASK_MIX_DONE:
            reg = (reg & ~(BIT_MASK(18))) | (status << 18);
            writel(reg, kendrty_2d->base + 0x7c);
            break;
   
        default:
            ret = -1;
        
    }
    printk("kendrty_2d_intr_ctl ret is %x \n", reg);
}



// 1 start 0 stop
static void kendrty_2d_set_rot_frame_run(struct kendrty_2d *kendrty_2d, uint8_t status)
{
    writel(status, kendrty_2d->base + 0x80);
}

// 
static void kendrty_2d_set_rot_act_size(struct kendrty_2d *kendrty_2d, uint32_t h_act_size, uint32_t v_act_size)
{
    uint32_t reg = 0;
    reg = (h_act_size << 0) + (v_act_size << 16);

    writel(reg, kendrty_2d->base + 0x80 + 0x0c);
}

static void kendrty_2d_set_rot_frm_src_base(struct kendrty_2d *kendrty_2d, uint32_t y_base, uint32_t u_base, uint32_t v_base)
{

    writel(y_base, kendrty_2d->base + 0x80 + 0x10);  // y src base

    writel(u_base, kendrty_2d->base + 0x80 + 0x14);  // u src base

    writel(v_base, kendrty_2d->base + 0x80 + 0x18);  // u src base
}

static void kendrty_2d_set_rot_frm_des_base(struct kendrty_2d *kendrty_2d, uint32_t y_base, uint32_t u_base, uint32_t v_base)
{

    writel(y_base, kendrty_2d->base + 0x80 + 0x1c);  // y des base

    writel(u_base, kendrty_2d->base + 0x80 + 0x20);  // u des base

    writel(v_base, kendrty_2d->base + 0x80 + 0x24);  // u des base
}

static void kendrty_2d_set_rot_stride(struct kendrty_2d *kendrty_2d, uint32_t y_stride, uint32_t u_stride, uint32_t v_stride)
{
    writel(y_stride, kendrty_2d->base + 0x80 + 0x28);  // y stride

    writel(u_stride, kendrty_2d->base + 0x80 + 0x2c);  // u stride

    writel(v_stride, kendrty_2d->base + 0x80 + 0x30);  // u stride
}


void kendrty_set_rot_frame_run(int status)
{
    writel(status, twod_test->base + 0x80);
}

EXPORT_SYMBOL(kendrty_set_rot_frame_run);


  
void  kendrty_set_rot_frame_src_addr(uint32_t y_base, uint32_t u_base, uint32_t v_base)
{
    kendrty_2d_set_rot_frm_src_base(twod_test, y_base, u_base, v_base);
}
EXPORT_SYMBOL(kendrty_set_rot_frame_src_addr);


static int twod_cut = 0;

void  kendrty_set_rot_frame_des_addr(uint32_t y_base, uint32_t u_base, uint32_t v_base)
{
    kendrty_2d_set_rot_frm_des_base(twod_test, y_base, u_base, v_base);

}

EXPORT_SYMBOL(kendrty_set_rot_frame_des_addr);


int kendrty_get_frame_addr(struct twod_data dat)
{
//    dat = twod_test->data;         //twod_cut 会大1 ，vo记得减1
    memcpy(&dat, &twod_test->data, sizeof(dat));

//    printk("dat cut is %d \n", twod_test->data.val);
    return twod_test->vo_cut;

}
EXPORT_SYMBOL(kendrty_get_frame_addr);


void kendryte_set_fram_addr(uint32_t y_base, int cut)
{
    twod_test->data.addr[cut] = y_base;
    twod_test->data.val = cut;
    twod_test->vo_cut = cut;
}
EXPORT_SYMBOL(kendryte_set_fram_addr);



static int kendrty_2d_open(struct inode *inode, struct file *file)
{
    struct kendrty_2d *kendrty_2d = container_of(inode->i_cdev,
                    struct kendrty_2d, cdev);

    file->private_data = kendrty_2d;

    return 0;
}


static int kendrty_2d_release(struct inode *inode, struct file *file)
{
    struct kendrty_2d *kendrty_2d = file->private_data;

    return 0;
}


static long kendrty_2d_compat_ioctl(struct file *file, unsigned int cmd,
                                    unsigned long arg)
{
    long ret = -ENOIOCTLCMD;

    if (file->f_op->unlocked_ioctl)
            ret = file->f_op->unlocked_ioctl(file, cmd, arg);

        return ret;
}

static void kendrty_2d_get_reg_val(struct kendrty_2d *kendrty_2d)
{
    uint32_t Iloop = 0; 
    uint32_t RegValue = 0; 

    while(1)
    {
        RegValue = readl(kendrty_2d->base +  (Iloop << 2));
        printk("reg index = 0x%x, value = 0x%x !\n",Iloop,RegValue);
        Iloop++;
        if(Iloop >= 133)
        {
            break;
        }
    }
    RegValue = readl(kendrty_2d->base  + 0x3e0);
    printk("reg index = 0x%x, value = 0x%x !\n",Iloop,RegValue);

}

static long kendrty_2d_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)                      
{
    struct kendrty_2d *kendrty_2d = filp->private_data;
    uint32_t y_src, u_src, y_des, u_des;

    switch (cmd) {
        case KENDRTY_2DROTATION_90:
            kendrty_2d->rotation = ANGLE_90;

//            kendrty_2d_intr_ctl(kendrty_2d, TD_INT_MASK_ROT_DONE, 0);           // 0 open intr 

//            kendrty_2d_set_rot_frame_run(kendrty_2d, 1);            // start 
//            kendrty_2d_set_rot_frame_run(kendrty_2d, 0);            // start 

            kendrty_set_rot_frame_run(1);
            kendrty_set_rot_frame_run(0);

            break;
        
        case KENDRTY_2DROTATION_270:
            kendrty_2d->rotation = ANGLE_270;

            break;
        
        case KENDRTY_2DROTATION_INPUT_ADDR:
            kendrty_2d->input_addr = (uint32_t)arg;
            y_src = kendrty_2d->input_addr + (kendrty_2d->v_active - 1) * kendrty_2d->y_src_stride;
            u_src = (kendrty_2d->input_addr + (1080 * 1920) ) + (kendrty_2d->v_active / 2 - 1) * kendrty_2d->u_src_stride;

            kendrty_set_rot_frame_src_addr(y_src, u_src, 0);
//            kendrty_2d_set_rot_frm_src_base(kendrty_2d, y_src, u_src, 0);
            printk("kendrty_2d->input_addr is 0x%x u_src is %x \n", y_src, u_src);
            break;
        
        case KENDRTY_2DROTATION_OUTPUT_ADDR:
            kendrty_2d->output_addr = (uint32_t)arg;
            y_des = kendrty_2d->output_addr;
            u_des = kendrty_2d->output_addr + 1920 * 1080;

//            kendrty_2d_set_rot_frm_des_base(kendrty_2d, y_des, u_des, 0);
            kendrty_set_rot_frame_des_addr(y_des, u_des, 0);
            printk("kendrty_2d->output_addr is 0x%x \n", kendrty_2d->output_addr);
            break;

        case KENDRTY_2DROTATION_GET_REG_VAL :

            kendrty_2d_get_reg_val(kendrty_2d);
            break;
        
        default:
            printk("isp_ioctl: Unknown ioctl: 0x%.8X\n", cmd);
            return -EINVAL;
    }

    return 0;
}


static const struct file_operations kendrty_2d_file_ops = {
    .owner          = THIS_MODULE,
    .open           = kendrty_2d_open,
    .release        = kendrty_2d_release,
    .unlocked_ioctl = kendrty_2d_ioctl,
    .compat_ioctl   = kendrty_2d_compat_ioctl,

};

static int kendrty_2d_get_wrap_intr_status(struct kendrty_2d *kendrty_2d, enum WRAP_INT_ST status)
{
    int ret = -1;
    uint32_t reg = 0;

    reg = readl(kendrty_2d->base + 0x7c);

    switch(status)
    {
        case TD_INT_ST_ROT_DONE:
            ret = reg & 0x01;
            break;

        case TD_INT_ST_ROT_TIMEOUT:
            ret = (reg >> 1) & 0x01;
            break;
        
        case TD_INT_ST_MIX_DONE:
            ret = (reg >> 2) & 0x01;
            break;
   
        default:
            ret = -1;
        
    }
    return ret;
}  




// TD base 0x92720000
static void kendrty_2d_set_wrap(struct kendrty_2d *kendrty_2d)
{
    uint32_t reg = 0;

//    printk("-----------------------------1---------- \n");
    writel(0x3ff, kendrty_2d->base + 0x00);                 //MH_TD_set_WrapSwRst
//    printk("-----------------------------2---------- \n");
    writel(0xffffffff, kendrty_2d->base + 0x0c);            //MH_TD_set_WrapClkEn(0xffffffff);
    writel(0, kendrty_2d->base + 0x20);                     //MH_TD_set_WrapDmaArbMode(0);//w/r use priority mode
    writel(0x01010101, kendrty_2d->base + 0x24);            //MH_TD_set_WrapDmaWeightWR0(0x01010101);
//    printk("-----------------------------1---------- \n");
    writel(0x01010101, kendrty_2d->base + 0x28);            //MH_TD_set_WrapDmaWeightWR1(0x01010101);
    writel(0x01010101, kendrty_2d->base + 0x2c);            //MH_TD_set_WrapDmaWeightRD0(0x01010101);
    writel(0x01010101, kendrty_2d->base + 0x30);            //MH_TD_set_WrapDmaWeightRD1(0x01010101);
//    printk("-----------------------------3---------- \n");
    writel(0x01234567, kendrty_2d->base + 0x34);            //MH_TD_set_WrapDmaPriorityWR(0x01234567);
    writel(0x01234567, kendrty_2d->base + 0x38);            //MH_TD_set_WrapDmaPriorityRD(0x01234567);
    writel(0x76543210, kendrty_2d->base + 0x3c);            //MH_TD_set_WrapDmaIdWR(0x76543210);
    writel(0x76543210, kendrty_2d->base + 0x40);            //MH_TD_set_WrapDmaIdRD(0x76543210);
//    printk("-----------------------------4---------- \n");

//    reg = readl(kendrty_2d->base + 0x78);
//    reg = reg | 0x01;
//    writel(1, kendrty_2d->base + 0x78);                     //MH_TD_set_WrapCfg_Done(1);     MS_API_TD_WrapCfgDone
//    printk("-----------------------------7---------- \n");
    writel(0x00020001, kendrty_2d->base + 0x78);                                      //MH_TD_set_WrapCfg(0x00020001); 
//    printk("-----------------------------8---------- \n");
    writel(0xff0000, kendrty_2d->base + 0x7c);              //MH_TD_set_WrapInt(0xFE0000);
//    printk("-----------------------------5---------- \n");
}


static uint8_t  kendrty_2d_get_rot_rdate_status(struct kendrty_2d *kendrty_2d, enum ROT_RADA_FUNC func)
{
    int ret = -1;
    uint32_t reg = 0;

    reg = readl(kendrty_2d->base + 0x80 + 0x40);

    switch(func)
    {
        case TD_ROT_FRM_END:
            ret = reg & 0x01;
            break;

        case TD_ROT_DONE_ST:
            ret = (reg >> 1) & 0x07;
            break;
        
        case TD_ROT_FSM_ST_RD:
            ret = (reg >> 8) & 0x07;
            break;
        
        case TD_ROT_FSM_ST_WR:
            ret = (reg >> 12) & 0x07;
            break;
        
        case TD_ROT_CYCLE_CNT:
            ret = (reg >> 16) & 0xffff;
            break;
        
        default:
            ret = -1;
        
    }
    return ret; 
}

static void kendrty_2d_init_Rotation90DegreeMainYuv420(struct kendrty_2d *kendrty_2d)
{

//    printk("kendrty_2d_init_Rotation90DegreeMainYuv420 init \n");
    kendrty_2d_set_wrap(kendrty_2d);
//    printk("-----------------------------6---------- \n");
    writel(0x1010f10, kendrty_2d->base + 0x80 + 0x04) ;                 //TD_ROT_CTRL_MODE [1:0]:rotation mode   2'b00: 90 degree； 2'b01: 270 degree 
    writel(0xffff, kendrty_2d->base + 0x80 + 0x08) ;                     //TD_ROT_TIMEOUT

//    printk("-----------------------------6---------- \n");
    kendrty_2d_set_rot_act_size(kendrty_2d, 1920 - 1, 1080 - 1);                  //TD_ROT_IMAHE_SIZE_ACTIVE
//    printk("-----------------------------8---------- \n");
    kendrty_2d_set_rot_frm_src_base(kendrty_2d, 0x11f9c80, 0x12f6e80, 0x00);

//    printk("-----------------------------7---------- \n");
    kendrty_2d_set_rot_frm_des_base(kendrty_2d, 0x2000000, 0x21fa400, 0x00);
//    printk("-----------------------------9---------- \n");
    kendrty_2d_set_rot_stride(kendrty_2d, 0x4400780, 0x4400780, 0);  //0x4380780  0x4400780
//    printk("-----------------------------10---------- \n");

 //   printk("errrrr          ---------3----------- \n");
/*
    kendrty_2d_set_rot_frame_run(kendrty_2d, 1);       // start   
    kendrty_2d_set_rot_frame_run(kendrty_2d, 0);
 */

    // set 2d intr0 open 
//    writel(0x100000, kendrty_2d->base + 0x3e0) ;   //100000
    kendrty_2d_intr_ctl(kendrty_2d, TD_INT_MASK_ROT_DONE, 0);           // 0 open intr 

    kendrty_2d->h_active = 1920;
    kendrty_2d->v_active = 1080;

    kendrty_2d->y_src_stride = 1920;
    kendrty_2d->u_src_stride = 1920;

    kendrty_2d->y_des_stride = 1088;//1088;
    kendrty_2d->u_des_stride = 1088;//1088;

//    printk("errrrr          ------------4-------- \n");

}


static int twd_flag = 0;

// Tdirq hander irq
static irqreturn_t kendryte_2d_Tdirq(int irq, void *id)
{
	struct kendrty_2d *kendrty_2d = (struct kendrty_2d *)id;

//    printk("kendryte_2d_Tdirq ----------------\n");
    if(kendrty_2d_get_wrap_intr_status(kendrty_2d, TD_INT_ST_ROT_DONE) == 1)
    {
//        printk("kendryte_2d_Tdirq \n");

        kendrty_2d_set_wrap_intr_status(kendrty_2d, TD_INT_ST_ROT_DONE);
//        timer2 = jiffies;
        timer2 = sbi_get_cycles();
/*
        if(timer2 - timer1 > 25000)
        {
            printk("timer2 - timer1 is %d \n",  jiffies_to_msecs(timer2 - timer1));
        }
*/
        //printk("timer2 - timer1 is %d \n",  jiffies_to_msecs(timer2 - timer1));
//        printk("timer2 - timer1 is %d \n",  timer2 - timer1);
        kendrty_2d->data.flag[kendrty_2d->data.val] = 1;
#if 0
        twd_flag ++;
        if(twd_flag == 120)
        {
            twd_flag = 0;
            printk("twod intr \n");
        }
#endif
    }

    if(kendrty_2d_get_wrap_intr_status(kendrty_2d, TD_INT_ST_ROT_TIMEOUT) == 1)
    {
        kendrty_2d_set_wrap_intr_status(kendrty_2d, TD_INT_ST_ROT_TIMEOUT);
    }

    if(kendrty_2d_get_wrap_intr_status(kendrty_2d, TD_INT_ST_MIX_DONE) == 1)
    {
        kendrty_2d_set_wrap_intr_status(kendrty_2d, TD_INT_ST_MIX_DONE);
    }

	return IRQ_HANDLED;
}



static void kendryte_close_2d_bandwidth_limit(void)
{
    uint32_t addr = ioremap(0x9990070c, 0x08);        // dsi rst ctl

    writel(0, addr);        // *(volatile unsigned int *)0x9990070c = 0;

    iounmap(addr);
   
}

static int kendrty_2d_probe(struct platform_device *pdev)
{
    uint8_t ret;
    dev_t dev;
    unsigned int minor = 0;
    struct resource *res;
    struct kendrty_2d *kendrty_2d;

#if 0
    uint32_t *addr = ioremap(0x97001118, 0x04);                   //twod apb clk 0x100001
    uint32_t *addr2 = ioremap(0x9700111c, 0x04);                   //twod axi clk 0x10001
    uint32_t *addr3 = ioremap(0x97003170, 0x04);                   //disp domain
#endif
    uint32_t *addr4 = ioremap(0x9700316c, 0x04);                   //disp domain
    kendrty_2d = devm_kzalloc(&pdev->dev, sizeof(*kendrty_2d), GFP_KERNEL);

    twod_test = kendrty_2d;
    platform_set_drvdata(pdev, kendrty_2d);

    /* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    kendrty_2d->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(kendrty_2d->base))
		return PTR_ERR(kendrty_2d->base);
    
    printk("kendrty_2d->base is %ps \n", kendrty_2d->base);


    kendrty_2d->twod_apb = devm_clk_get(&pdev->dev, "twod_apb");
    if (IS_ERR(kendrty_2d->twod_apb)) {
        return PTR_ERR(kendrty_2d->twod_apb);
    } 

//    ret = clk_prepare_enable(kendrty_2d->twod_clk);
    ret = clk_enable(kendrty_2d->twod_apb);
    if(ret < 0)
    {
        printk("kendrty_2d->twod_clk is %d \n", ret);
        return ret;
    }

    kendrty_2d->twod_axi = devm_clk_get(&pdev->dev, "twod_axi");
    if (IS_ERR(kendrty_2d->twod_axi)) {
        return PTR_ERR(kendrty_2d->twod_axi);
    } 

//    ret = clk_prepare_enable(kendrty_2d->twod_clk);
    ret = clk_enable(kendrty_2d->twod_axi);
    if(ret < 0)
    {
        printk("kendrty_2d->twod_axi is %d \n", ret);
        return ret;
    }

    // clock 

//   writel(0xf001f, addr);
//    writel(0x30003, addr2);

    writel(0x2000010, addr4);
/*
    ret = readl(addr);
    printk("kendrty_2d->twod_apb is %x \n", ret);
    ret = readl(addr2);
    printk("kendrty_2d->twod_axi is %x \n", ret);

    ret = readl(addr3);
    printk("kendrty_2d->domain is %x \n", ret);
    
    */

    // get irq
    kendrty_2d->irq_fram = platform_get_irq(pdev, 0);               //68
	if (kendrty_2d->irq_fram < 0) {
			dev_err(&pdev->dev, "no irq for alarm\n");
			return kendrty_2d->irq_fram;
	}

    ret = devm_request_irq(&pdev->dev, kendrty_2d->irq_fram, kendryte_2d_Tdirq,
								0, "kendrty_2d TD", kendrty_2d);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", kendrty_2d->irq_fram, ret);
		return -1;
	}

    printk("start kendrty_2d_init_Rotation90DegreeMainYuv420 init \n");
    kendrty_2d_init_Rotation90DegreeMainYuv420(kendrty_2d);           // config 2d init default 

//    kendryte_close_2d_bandwidth_limit();
    printk("start kendrty_2d_init_Rotation90DegreeMainYuv420 end \n");

    // creat char device
    kendrty_2d_class = class_create(THIS_MODULE, "kendrty_2d");
    if (IS_ERR(kendrty_2d_class)) {
        ret = PTR_ERR(kendrty_2d_class);
        printk(KERN_ERR "phantom: can't register phantom class\n");
        return -1 ;
    }
    
    ret = alloc_chrdev_region(&dev, 0, 8, "kendrty_2d");
    kendrty_2d_major = MAJOR(dev);

    // cdev init 
    cdev_init(&kendrty_2d->cdev, &kendrty_2d_file_ops);
    kendrty_2d->cdev.owner = THIS_MODULE;
    ret = cdev_add(&kendrty_2d->cdev, MKDEV(kendrty_2d_major, minor), 1);
    if (ret) {
        dev_err(&pdev->dev, "chardev registration failed\n");
        return -1 ;
    }

    // create char 
    if(IS_ERR(device_create(kendrty_2d_class, &pdev->dev, MKDEV(kendrty_2d_major, minor), NULL, "kendrty_2d%u", minor)))
    {
        dev_err(&pdev->dev, "can't create device\n"); 
    }

    printk("kendrty_2d init success \n");               
	return 0;
}


static int kendrty_2d_remove(struct platform_device *pdev) 
{

    struct kendrty_2d *kendrty_2d = platform_get_drvdata(pdev);

    unsigned int minor = MINOR(kendrty_2d->cdev.dev);

    device_destroy(kendrty_2d_class, MKDEV(kendrty_2d_major, minor));
    cdev_del(&kendrty_2d->cdev);

    unregister_chrdev_region(MKDEV(kendrty_2d_major, 0), 8);
    class_destroy(kendrty_2d_class);

    return 0;
}


static const struct of_device_id kendrty_2d_ids[] = {
	{ .compatible = "k510, kendrty_2d" },
	{}
};


static struct platform_driver kendrty_2d_driver = {
    .probe          = kendrty_2d_probe,
    .remove         = kendrty_2d_remove,
    .driver         = {
        .name   = " kendrty_2d",
        .of_match_table = of_match_ptr(kendrty_2d_ids),
    },
};


static int __init kendrty_2d_init(void)
{
    int ret;
    printk("insmode kendrty_2d_init \n");
    ret = platform_driver_register(&kendrty_2d_driver);

    return 0;
}

static void __exit kendrty_2d_exit(void)
{
    printk("insmode kendrty_2d_init \n");
    platform_driver_unregister(&kendrty_2d_driver);
}

module_init(kendrty_2d_init);
module_exit(kendrty_2d_exit);


MODULE_LICENSE("GPL");