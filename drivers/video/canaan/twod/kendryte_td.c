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
#include <linux/cpu.h>
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
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/clk.h>
#include <linux/poll.h>

// gpio 
#include "kendryte_td.h"


#define TWOD_MAJOR		230

static void kendryte_twod_init(struct kendryte_td *kendryte_td)
{
    
    spin_lock_init(&kendryte_td->reg_lock);
	spin_lock_init(&kendryte_td->irq_lock);

    // set flag 0
    kendryte_td->twod_int_flag = 0;

    // creat wait queue
    init_waitqueue_head(&kendryte_td->wait_que);

 //   kedryte_td_scaler_sw_config(kendryte_td);
    // set twod wrap
    kendryte_td_set_wrap(kendryte_td);

    kendryte_td_set_scaler_table(kendryte_td);

    kendryte_rot_default_config(kendryte_td);

    kedryte_td_scaler_init(kendryte_td);
}


static int kendryte_td_open(struct inode *inode, struct file *file)
{
    struct kendryte_td *kendryte_td = container_of(file->private_data,
                    struct kendryte_td, miscdev);

    file->private_data = kendryte_td;

    return 0;
}

static int kendryte_td_release(struct inode *inode, struct file *file)
{
    struct kendryte_td *kendryte_td = file->private_data;
    
    return 0;
}

static long kendryte_td_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)                      
{
    struct kendryte_td *kendryte_td = filp->private_data;
//    struct td_info *info;
    struct td_info info;
    struct twod_session *sesson;
    uint32_t ret = 0;
    uint32_t val = 0;
    size_t  size;

    //printk("kendryte_td_ioctl ----------  KENDRYTE_TWOD_SET_DATA cmd is %d ", cmd);
    switch (cmd) {
        case KENDRYTE_TWOD_SET_DATA :
           
            size = sizeof(struct td_info) + sizeof(struct twod_session);

            //info = kzalloc(sizeof(struct td_info), GFP_KERNEL);
            sesson = kzalloc(size, GFP_KERNEL);
            // get data
            if (unlikely(copy_from_user(&info, (void __user *)arg, sizeof(struct td_info))))           //(struct td_info*)arg
			{
				printk("copy_from_user failed\n");
               // kfree(info);
                kfree(sesson);
				ret = -EFAULT;
				break;
			}

            if(atomic_read(&kendryte_td->twod_num) == 0)
            {
                // first 
            //    kendryte_td->td_info = info;
                switch (info.mode)
                {
                    case TWOD_ROT_90:

                        kendryte_rot_reg_src_info(kendryte_td, &info.src, TWOD_ROT_90);
                        kendryte_rot_reg_dst_info(kendryte_td, &info.dsc);
                     //   kendrty_2d_get_reg_val(kendryte_td);
                        kendryte_rot_setframe_start(kendryte_td, 1);
                        kendryte_rot_setframe_start(kendryte_td, 0);
                        
                        break;
                    
                    case TWOD_ROT_270 :
                        kendryte_rot_reg_src_info(kendryte_td, &info.src, TWOD_ROT_270);
                        kendryte_rot_reg_dst_info(kendryte_td, &info.dsc);
                     //   kendrty_2d_get_reg_val(kendryte_td);
                        kendryte_rot_setframe_start(kendryte_td, 1);
                        kendryte_rot_setframe_start(kendryte_td, 0);
                        break;
                    
                    case TWOD_SCALE :
                    #if 1
                        kendryte_td_layer0_set_config(kendryte_td, &info.src);
                        kendryte_td_wb_set_config(kendryte_td, &info.dsc);
                        printk("td_info->v_step is %x  td_info->h_step is %x \n", info.src.v_step, info.src.h_step);
                        
                        kendryte_scaler_start_frame(kendryte_td, 1);
                        kendryte_scaler_start_frame(kendryte_td, 0);

                      //  kendrty_2d_get_reg_val(kendryte_td);
                      //  kendryte_scaler_get_reg_val(kendryte_td);
                    #endif
                        break;
                    
                }
            }else{
                // add list 

                sesson->td_info = &info;

                mutex_lock(&kendryte_td->mutex);
                list_add_tail(&sesson->session, &kendryte_td->waiting);
                mutex_unlock(&kendryte_td->mutex);
            }
            
            val = atomic_read(&kendryte_td->twod_num) + 1;
            atomic_set(&kendryte_td->twod_num, val);

            break;
    }

    return ret;
}

static long kendryte_td_compat_ioctl(struct file *file, unsigned int cmd,
                                    unsigned long arg)
{
    long ret = -ENOIOCTLCMD;

    if (file->f_op->unlocked_ioctl)
            ret = file->f_op->unlocked_ioctl(file, cmd, arg);

        return ret;
}

static unsigned int kendryte_td_poll(struct file *filp, poll_table *wait)
{
    unsigned int ret = 0;
    struct kendryte_td *kendryte_td = filp->private_data;

    poll_wait(filp, &kendryte_td->wait_que, wait);//将当前进程放到waitq列表

    if(kendryte_td->twod_int_flag) {
        ret |= POLLIN;                          //说明有数据被取到了
        kendryte_td->twod_int_flag = 0;
    }

    return ret;
}

static const struct file_operations kendryte_td_file_ops = {
    .owner          = THIS_MODULE,
    .open           = kendryte_td_open,
    .release        = kendryte_td_release,
    .unlocked_ioctl = kendryte_td_ioctl,
    .compat_ioctl   = kendryte_td_compat_ioctl,
    .poll           = kendryte_td_poll,
};


// Tdirq hander irq
static irqreturn_t kendryte_td_Tdirq(int irq, void *id)
{
	struct kendryte_td *kendryte_td = (struct kendryte_td *)id;
    int ret = 0;

    printk("kendryte_td_Tdirq **************** \n");
    // get rotation done
    if(kendryte_rot_get_intr_status(kendryte_td, TWOD_ROTATION_DONE) == 1)
    {
        // clear intr
        kendryte_rot_clear_intr(kendryte_td, TWOD_ROTATION_DONE);
        printk("TWOD_ROTATION_DONE --------------- \n");
        if(atomic_read(&kendryte_td->twod_num) == 1)
        {
            atomic_set(&kendryte_td->twod_num, 0);
            //kfree(kendryte_td->td_info);

        }else{
            // 处理链表
            
        }
        // poll 方式 上报 twod 完成 
        kendryte_td->twod_int_flag = 1;        
        wake_up_interruptible(&kendryte_td->wait_que);   /* 唤醒休眠的进程，即调用read函数的进程 */
    }

    if(kendryte_rot_get_intr_status(kendryte_td, TWOD_ROTATION_TIMEOUT) == 1)
    {
        kendryte_rot_clear_intr(kendryte_td, TWOD_ROTATION_TIMEOUT);

        printk("TWOD_ROTATION_TIMEOUT --------------- \n");
    }

    if(kendryte_rot_get_intr_status(kendryte_td, TWOD_MIX_DONE) == 1)
    {
        kendryte_rot_clear_intr(kendryte_td, TWOD_MIX_DONE);
        printk("TWOD_MIX_DONE --------------- \n");

        if(atomic_read(&kendryte_td->twod_num) == 1)
        {
            atomic_set(&kendryte_td->twod_num, 0);
            //kfree(kendryte_td->td_info);

        }else{
            // 处理链表
            
        }
        // poll 方式 上报 twod 完成 
        kendryte_td->twod_int_flag = 1;        
        wake_up_interruptible(&kendryte_td->wait_que);   /* 唤醒休眠的进程，即调用read函数的进程 */

//        printk("0x92720c98 is %x \n", readl(kendryte_td->base + 0xc98));
//        printk("0x92720c90 is %x \n", readl(kendryte_td->base + 0xc90));
    }
	return IRQ_HANDLED;
}

static const struct of_device_id kendryte_td_ids[] = {
	{ .compatible = "k510, kendrty_2d" },
	{}
};

#if 0 
static struct miscdevice kendryte_dev ={
	.minor = TWOD_MAJOR,
	.name  = "twod",
	.fops  = &kendryte_td_file_ops,
};
#endif

/*
    twod clk
    
    twod axi clk 0x97001000
        plck en 0x9700111c


    twod apb clk 
        pclk en 0x97001118

*/
static int kendryte_td_probe(struct platform_device *pdev)
{
    uint8_t ret;
    struct resource *res;
    struct kendryte_td *kendryte_td;

    printk("kendryte_td_probe   start \n");

    kendryte_td = devm_kzalloc(&pdev->dev, sizeof(*kendryte_td), GFP_KERNEL);

    platform_set_drvdata(pdev, kendryte_td);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    kendryte_td->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(kendryte_td->base))
		return PTR_ERR(kendryte_td->base);

    kendryte_td->twod_apb = devm_clk_get(&pdev->dev, "twod_apb");
    if (IS_ERR(kendryte_td->twod_apb)) {
        return PTR_ERR(kendryte_td->twod_apb);
    } 

    kendryte_td->twod_axi = devm_clk_get(&pdev->dev, "twod_axi");
    if (IS_ERR(kendryte_td->twod_axi)) {
        return PTR_ERR(kendryte_td->twod_axi);
    } 

    ret = clk_enable(kendryte_td->twod_axi);
    if(ret < 0)
    {
        printk("kendryte_td->twod_axi is %d \n", ret);
        return ret;
    }

    ret = clk_enable(kendryte_td->twod_apb);
    if(ret < 0)
    {
        printk("kendryte_td->twod_clk is %d \n", ret);
        return ret;
    }

    // get irq
    kendryte_td->irq_fram = platform_get_irq(pdev, 0);               //68
	if (kendryte_td->irq_fram < 0) {
			dev_err(&pdev->dev, "no irq for alarm\n");
			return kendryte_td->irq_fram;
	}

    ret = devm_request_irq(&pdev->dev, kendryte_td->irq_fram, kendryte_td_Tdirq,
								0, "kendryte_td TD", kendryte_td);
	if (ret) {
		dev_err(&pdev->dev, "IRQ%d error %d\n", kendryte_td->irq_fram, ret);
		return -1;
	}
    mutex_init(&kendryte_td->mutex);
    INIT_LIST_HEAD(&kendryte_td->waiting);
	INIT_LIST_HEAD(&kendryte_td->running);
    INIT_LIST_HEAD(&kendryte_td->done);

    atomic_set(&kendryte_td->twod_num, 0);
    atomic_set(&kendryte_td->rotation_num, 0);

    kendryte_twod_init(kendryte_td);

    kendryte_td->miscdev.minor = TWOD_MAJOR;
    kendryte_td->miscdev.name  = "twod";
    kendryte_td->miscdev.fops  = &kendryte_td_file_ops;


#if 0 
    // creat char device
    kendryte_td_class = class_create(THIS_MODULE, "kendryte_td");
    if (IS_ERR(kendryte_td_class)) {
        ret = PTR_ERR(kendryte_td_class);
        printk(KERN_ERR "phantom: can't register phantom class\n");
        return -1 ;
    }
    
    ret = alloc_chrdev_region(&dev, 0, 8, "kendryte_td");
    kendryte_td_major = MAJOR(dev);

    // cdev init 
    cdev_init(&kendryte_td->cdev, &kendryte_td_file_ops);
    kendryte_td->cdev.owner = THIS_MODULE;
    ret = cdev_add(&kendryte_td->cdev, MKDEV(kendryte_td_major, minor), 1);
    if (ret) {
        dev_err(&pdev->dev, "chardev registration failed\n");
        return -1 ;
    }

    // create char 
    if(IS_ERR(device_create(kendryte_td_class, &pdev->dev, MKDEV(kendryte_td_major, minor), NULL, "kendryte_td%u", minor)))
    {
        dev_err(&pdev->dev, "can't create device\n"); 
    }
#else


    ret = misc_register(&kendryte_td->miscdev);


#endif

    return 0;
              
}

static int kendryte_td_remove(struct platform_device *pdev)
{

    struct kendryte_td *kendryte_td = platform_get_drvdata(pdev);

#if 0
    unsigned int minor = MINOR(kendryte_td->cdev.dev);

    device_destroy(kendryte_td_class, MKDEV(kendryte_td_major, minor));
    cdev_del(&kendryte_td->cdev);

    unregister_chrdev_region(MKDEV(kendryte_td_major, 0), 8);
    class_destroy(kendryte_td_class);
#else

    misc_deregister(&kendryte_td->miscdev);
    
#endif
    return 0;
}

static struct platform_driver kendryte_td_driver = {
    .probe          = kendryte_td_probe,
    .remove         = kendryte_td_remove,
    .driver         = {
        .name   = " kendryte_td",
        .of_match_table = of_match_ptr(kendryte_td_ids),
    },
};


static int __init kendryte_td_init(void)
{
    int ret;
    printk("insmode kendryte_td_init \n");
    ret = platform_driver_register(&kendryte_td_driver);

    return 0;
}

static void __exit kendryte_td_exit(void)
{
    printk("insmode kendryte_td_init \n");
    platform_driver_unregister(&kendryte_td_driver);
}


module_init(kendryte_td_init);
module_exit(kendryte_td_exit);


MODULE_LICENSE("GPL");


