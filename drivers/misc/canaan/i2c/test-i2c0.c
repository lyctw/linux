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


#define I2C0_CMD_READ_REG         _IOWR('q', 1, struct i2c0_reg)
#define I2C0_CMD_WRITE_REG        _IOWR('q', 2, struct i2c0_reg)
#define I2C0_CMD_TEST              _IOWR('q', 3, unsigned long)


struct i2c0_reg {
        unsigned int id;
        unsigned int value;
};

struct i2c0_plat {
        struct resource *res;
	    void __iomem    *regs;
	    void __iomem    *iomux;
	    void __iomem    *gpio;
    
        unsigned long   regs_size;
        int             major;
        int             minor;
        struct class    *class;
        struct cdev     cdev;
};

static struct i2c0_plat *plat;
static int dev_id = 0;
static unsigned int int_cnt = 0;
static ktime_t last_tm;
static irqreturn_t i2c0_irq_handler(int irq, void *dev_id)
{
/*
    unsigned int int_status =  ioread32(plat->regs + 0x201DC);

    iowrite32(int_status | 0x00000008, plat->regs + 0x201DC);
    if(int_cnt == 500)
        last_tm = ktime_get();

    
    if((int_cnt > 1000) && (int_cnt < 1020))
    {
        ktime_t tm = ktime_get();
        ktime_t delta = ktime_sub(tm, last_tm);
        last_tm = tm;
        printk("i2c0 int come!!!!, delta  %llu\n", ktime_to_us(delta));
    }
    int_cnt ++;

    //if(int_cnt % 300 == 0)
    //    printk("300 int\n");
  */  
    return IRQ_HANDLED;
}

static int i2c0_read_reg(unsigned long arg)
{
        struct i2c0_reg reg;
        

        if (copy_from_user(&reg, (struct i2c0_reg *)arg, sizeof(struct i2c0_reg))) {
                printk("i2c0: read_reg: copy_from_user failed \n");                                                                
                return -EFAULT;
        }

        if (reg.id % 4) {
                printk("i2c0 read_reg: Unaligned register access: 0x%.4X\n",
                        reg.id);
                return -EINVAL;
        }
        if (reg.id > plat->regs_size) {
                printk("i2c0 read_reg: Out-of-range register read: 0x%.4X\n",
                        reg.id);
                return -EINVAL;
        }

        reg.value = ioread32(plat->regs + reg.id);

        if (copy_to_user((struct i2c0_reg *)arg, &reg, sizeof(struct i2c0_reg))) {
                printk("i2c0: read_reg: copy_to_user failed \n");                                                           
                return -EFAULT;
        }

        //printk("Reg read: 0x%.4X: 0x%.8x\n", reg.id, reg.value);

        return 0;
}

static int i2c0_write_reg(unsigned long arg)
{
        struct i2c0_reg reg;

        if (copy_from_user(&reg, (struct i2c0_reg *)arg, sizeof(struct i2c0_reg)))
                return -EFAULT;

        if (reg.id % 4) {
                printk("i2c0 read_reg: Unaligned register access: 0x%.4X\n",
                        reg.id);
                return -EINVAL;
        }
        if (reg.id > plat->regs_size) {
                printk("i2c0 read_reg: Out-of-range register read: 0x%.4X\n",
                        reg.id);
                return -EINVAL;
        }

        iowrite32(reg.value, plat->regs + reg.id);

        if (copy_to_user((struct i2c0_reg *)arg, &reg, sizeof(struct i2c0_reg)))
                return -EFAULT;

        return 0;
}


static  int setup_chrdev_region(void)
{
        dev_t dev = 0;
        int err;

        if (plat->major == 0) {
                err = alloc_chrdev_region(&dev, 0, 1, "test_i2c0");
                plat->major = MAJOR(dev);

                if (err) {
                        printk("test-i2c0: can't get major %d\n", plat->major);
                        return err;
                }
        }
        return 0;
}


static int create_module_class(void)
{
        plat->class = class_create(THIS_MODULE, "i2c0_class");

        if (IS_ERR(plat->class))
                return PTR_ERR(plat->class);

        return 0;
}

void clean_up_i2c0_cdev(void)
{
        cdev_del(&plat->cdev);
}


static int i2c0_open(struct inode *inode, struct file *filp)
{
        return 0;
}

static int i2c0_release(struct inode *inode, struct file *filp)
{
        return 0;
}

static int i2c0_cmd_test(unsigned long arg)
{
        unsigned int data = ioread32(plat->regs + 0x4);

        printk("i2c test, base addr 0x%lx\n", plat->regs);

        printk("i2c test, arg 0x%x, data = 0x%x\n", arg, data);

        //printk("Reg read: 0x%.4X: 0x%.8x\n", reg.id, reg.value);

        return 0;
}

static long i2c0_ioctl(struct file *filp, unsigned int cmd,
                             unsigned long arg)
{

        switch (cmd) {
        case I2C0_CMD_READ_REG:
                return i2c0_read_reg(arg);
        case I2C0_CMD_WRITE_REG:
                return i2c0_write_reg(arg);
        case I2C0_CMD_TEST:
                return i2c0_cmd_test(arg);
        default:
                printk("i2c0_ioctl: Unknown ioctl: 0x%.8X\n", cmd);
                return -EINVAL;
        }

}

static long i2c0_compat_ioctl(struct file *file, unsigned int cmd,
                                    unsigned long arg)
{
        long ret = -ENOIOCTLCMD;

        if (file->f_op->unlocked_ioctl)
                ret = file->f_op->unlocked_ioctl(file, cmd, arg);

        return ret;
}

static int i2c0_mmap(struct file *file, struct vm_area_struct *vma)
{    
    if(remap_pfn_range(vma,
                        vma->vm_start,
                        0x97060000 >> PAGE_SHIFT,
                        vma->vm_end - vma->vm_start,
                        vma->vm_page_prot))
    {          
        printk("dsp_mmap: remap_pfn_range fail \n");
        return -EAGAIN;
    }
    return 0;
}

const struct file_operations i2c0_fops = {
        .owner          = THIS_MODULE,
        .open           = i2c0_open,
        .release        = i2c0_release,
        .unlocked_ioctl = i2c0_ioctl,
        .compat_ioctl   = i2c0_compat_ioctl,
        .mmap           = i2c0_mmap,
};


int setup_i2c0_cdev(const char *device_name)
{
        struct device *device;
        int err, devno =
                MKDEV(plat->major, plat->minor);

        cdev_init(&plat->cdev, &i2c0_fops);
        plat->cdev.owner = THIS_MODULE;
        err = cdev_add(&plat->cdev, devno, 1);
        if (err) {
                printk("i2c0: Error %d adding isc0 device number %d \n", err, plat->minor);
                return err;
        }

        if (device_name != NULL) {
                device = device_create(plat->class, NULL, devno, NULL,
                                       device_name);
                if (IS_ERR(device)) {
                        printk("i2c0: device not created\n");
                        clean_up_i2c0_cdev();
                        return PTR_ERR(device);
                }
        }

        return 0;
}

static const char i2c0_name[] = "test-i2c0";

int i2c0_probe(struct platform_device *pdev)
{
        struct resource *res;
        unsigned int reg = 0;
        unsigned int  *addr = ioremap(0x97001000 + 0xb8, 0x08);
        //unsigned int  *addr1 = ioremap(0x97001000 + 0xbc, 0x08);
        // set iic0 freq    
        writel(0x1000000,addr);
        // read iic0 freq
        reg = readl(addr);
        printk("----------------- reg0 is %x \n", reg);
        // set iic1 freq    
        //writel(0x1000000,addr1);
        // read iic1 freq
        //reg = readl(addr1);
        //printk("----------------- reg1 is %x \n", reg);
        
        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!res) {
                printk("i2c0_probe: get resource failed \n");
                return -ENODEV;
        }

        plat = kzalloc(sizeof(struct i2c0_plat), GFP_KERNEL);
        if (!plat) {
           printk("i2c0_probe: kzalloc failed \n");  
           return -ENOMEM;
        }

        plat->res = request_mem_region(res->start, resource_size(res),
                                       dev_name(&pdev->dev));
        if (!plat->res) {
                printk("i2c0_probe: Could not reserve memory region\n");
                kfree(plat);
                return -ENOMEM;
        }

	plat->regs = ioremap(res->start, resource_size(res));
        if(!plat->regs) {
            printk("i2c0_probe: ioremap failed \n");
            kfree(plat);
            return -ENOMEM;
        }
        
        plat->regs_size = res->end - res->start;
        plat->major = 0;
        plat->minor = 0;
                
        printk("i2c0 probe ok, mapped %llx at %px, mapped size %lld \n", res->start, plat->regs, resource_size(res));

        plat->gpio = ioremap(0x97050000, 0x1000);
        if(!plat->gpio) {
            printk("i2c_probe: gpio ioremap failed \n");
            kfree(plat);
            return -ENOMEM;
        }
    
        plat->iomux = ioremap(0x97040000, 0x1000);
        if(!plat->iomux) {
            printk("i2c_probe: iomux ioremap failed \n");
            kfree(plat);
            return -ENOMEM;
        }               
	
	
//    printk("iomux 85, 0x%x\n", ioread32(plat->iomux + 85*4));
//    printk("iomux 90, 0x%x\n", ioread32(plat->iomux + 90*4));
//    printk("iomux 91, 0x%x\n", ioread32(plat->iomux + 91*4));
    
    iowrite32(0x80420eef, plat->iomux + 117*4);											//scl0
    iowrite32(0x80430eef, plat->iomux + 116*4);											//sda0
    iowrite32(0x80440eef, plat->iomux + 119*4);											//scl0
    iowrite32(0x80450eef, plat->iomux + 118*4);	

    printk("iomux 116, 0x%x\n", ioread32(plat->iomux + 116*4));							//sda       87
    printk("iomux 117, 0x%x\n", ioread32(plat->iomux + 117*4));							//scl     86
    printk("iomux 118, 0x%x\n", ioread32(plat->iomux + 118*4));							//sda       87
    printk("iomux 119, 0x%x\n", ioread32(plat->iomux + 119*4));	   
	
//    iowrite32(0x80420eef, plat->iomux + 85*4);
//    iowrite32(0x80440eef, plat->iomux + 90*4);
//    iowrite32(0x80450eef, plat->iomux + 91*4);
    
//    iowrite32(0x3f, plat->gpio + 4);


        ////////////
        res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
            if (res == NULL) {
                printk("i2c0 no IRQ resource found\n");
            }
            else
                printk("i2c0 has IRQ resource, irq %d\n", res->start);
            
        int irq = res->start;
    
        int ret = 0;
        
        ret = request_irq(irq, i2c0_irq_handler, IRQF_SHARED, i2c0_name, &dev_id);
        printk("request irq %d  ret %d\n", irq, ret);


        setup_chrdev_region();
        create_module_class();

        setup_i2c0_cdev("test-i2c0");


        iounmap(addr);
        //iounmap(addr1);
	return 0;
}

int i2c0_remove(struct platform_device *pdev)
{
        dev_t dev = MKDEV(plat->major, plat->minor);

        device_destroy(plat->class, dev);
        clean_up_i2c0_cdev();
        kfree(plat);
        free_irq(128, &dev_id);

        return 0;
}

static const struct of_device_id test_i2c0_ids[] = {
	{ .compatible = "i2c0" },
	{}
};

static struct platform_driver test_i2c0_driver = {
        .probe          = i2c0_probe,
        .remove         = i2c0_remove,
        .driver         = {
                .name           = "test-i2c0",
                .of_match_table = of_match_ptr(test_i2c0_ids),
        },
};

int i2c0_module_init(void)
{
        int ret;

        ret = platform_driver_register(&test_i2c0_driver);

        return ret;
}

void i2c0_module_deinit(void)
{
        platform_driver_unregister(&test_i2c0_driver);
}

module_init(i2c0_module_init);
module_exit(i2c0_module_deinit);
MODULE_LICENSE("GPL v2");
