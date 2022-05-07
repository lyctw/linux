/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>


#include "ws2812b.h"

#define dbgmsg(fmt, ...) \
    printk("%s:%s:(%d): " fmt, __FILE__, __func__, __LINE__, ##__VA_ARGS__)


#define LED_SYS_NAME    "led"
#define LED_DEV_NAME    "led"

#define BITS_PERWORD	(32)

/*
static struct class *led_dev_class = NULL;
static struct device *led_dev_device = NULL;
*/

struct led_dev
{
    struct spi_device *spi;
    struct miscdevice *misc;
    led_attr_t attr;	/*初始化参数*/
    spinlock_t		spi_lock;
    struct mutex mutex;
	u32  ismmap;	  	/*使能mmap标记*/
	u32  count;		  	/*LED数量*/
    led_rgb_u *led;		/*存储LED原始颜色数据*/
    u32  pre_len;     	/*预处理之后的数据长度*/
    u8   *pre_buf;    	/*存储预处理之后的数据*/
    u32  bytes_align; 	/*存储rgb+reset 4bytes对齐长度*/
};

struct led_dev *led_devp = NULL;

static int led_dev_init(struct led_dev *devp, led_attr_t *attr)
{
	int ret = 0;
	u64 count;
	u64 bits_org, bytes_align;
	u64 bits_rst_org, bytes_rst_align;

	devp->attr.led_count = attr->led_count;
	devp->attr.speed_hz = attr->speed_hz;
	devp->attr.rst_us = attr->rst_us;
	devp->attr.t0h_ns = attr->t0h_ns;
	devp->attr.t0l_ns = attr->t0l_ns;
	devp->attr.t1h_ns = attr->t1h_ns;
	devp->attr.t1l_ns = attr->t1l_ns;

	count = attr->led_count * sizeof(led_rgb_u);
	devp->led = (led_rgb_u *)kzalloc(count, GFP_KERNEL);
    if (!devp->led)
    {
        dbgmsg("request led-buf mem failed\n");
        ret = -ENOMEM;
        goto EXIT;
    }
    devp->count = attr->led_count;

	bits_rst_org = attr->rst_us;
	bits_rst_org *= devp->attr.speed_hz;
	bits_rst_org /= 1000000;
	bytes_rst_align = (bits_rst_org + 7) / 8;
	bytes_rst_align = (bytes_rst_align + 3) / 4;
	bytes_rst_align *= 4;

	bits_org = attr->t1h_ns + attr->t1l_ns;
	bits_org *= attr->speed_hz;
	bits_org /= 1000000000;
	bits_org *= 24;
	
	bits_org += bits_rst_org; 			/*rgb + reset*/
	bytes_align = (bits_org + 7) / 8;
	bytes_align = (bytes_align + 3) / 4;
	bytes_align *= 4;
	devp->bytes_align = bytes_align;
	
	bytes_align *= attr->led_count;		/*(rgb + reset) X num*/
	bytes_align += bytes_rst_align;
	devp->pre_buf = kzalloc (bytes_align, GFP_DMA);
    if (!devp->pre_buf)
    {
        dbgmsg("request led-buf mem failed\n");
        ret = -ENOMEM;
        goto EXIT;
    }
    devp->pre_len = bytes_align;

EXIT:
	return ret;
}

static int led_dev_deinit(struct led_dev *devp)
{
	if (!devp)
		return 0;

	if (devp->led)
	{
		kfree(devp->led);
		devp->led = NULL;
	}

	if (devp->pre_buf)
	{
		kfree(devp->pre_buf);
		devp->pre_buf = NULL;
	}

	return 0;
}

#if (BITS_PERWORD == 8)

static void led_dev_preprocess(struct led_dev *devp)
{
    int i = 0, j = 0, k = 0, v = 0;
    int index, shift;

    u64 count;
	u64 bits_org, bytes_align;
	u64 bits_rst_org, bytes_rst_align;
	u64 bits_t1h, bits_t1l, bits_t0h, bits_t0l;

	memset(devp->pre_buf, 0x00, devp->pre_len);
	
	bits_t1h = devp->attr.t1h_ns;
	bits_t1h *= devp->attr.speed_hz;
	bits_t1h /= 1000000000;
	
	bits_t1l = devp->attr.t1l_ns;
	bits_t1l *= devp->attr.speed_hz;
	bits_t1l /= 1000000000;

	bits_t0h = devp->attr.t0h_ns;
	bits_t0h *= devp->attr.speed_hz;
	bits_t0h /= 1000000000;
	
	bits_t0l = devp->attr.t0l_ns;
	bits_t0l *= devp->attr.speed_hz;
	bits_t0l /= 1000000000;
	

	/*在开始传输之前复位*/
	bits_rst_org = devp->attr.rst_us;
	bits_rst_org *= devp->attr.speed_hz;
	bits_rst_org /= 1000000;
	bytes_rst_align = (bits_rst_org + 7) / 8;
	bytes_rst_align = (bytes_rst_align + 3) / 4;
	bytes_rst_align *= 4;

	led_rgb_u *ledp = devp->led;
	unsigned char *p = NULL;

	for (i = 0; i < devp->count; i++)
	{
		p = (unsigned char *)(ledp + i);
		k = (bytes_rst_align + i*devp->bytes_align) * 8;
		for (v = 3; v > 0; v--)
		{
		    for (j = 1; j <= 8; j++)
		    {
		        if ((p[v] >> (8-j)) & 0x1)
		        {
					count = bits_t1h;
					while (count--)
					{
						index = k / 8;
						shift = 7 - k % 8;
						k++;
						devp->pre_buf[index] |= (1 << shift);
						//printk(KERN_CONT "1");
					}

					count = bits_t1l;
					while (count--)
					{
						index = k / 8;
						shift = 7 - k % 8;
						k++;
						devp->pre_buf[index] &= ~(1 << shift);
						//printk(KERN_CONT "0");
					}
		        }
		        else
		        {
					count = bits_t0h;
					while (count--)
					{
						index = k / 8;
						shift = 7 - k % 8;
						k++;
						devp->pre_buf[index] |= (1 << shift);
						//printk(KERN_CONT "1");
					}

					count = bits_t0l;
					while (count--)
					{
						index = k / 8;
						shift = 7 - k % 8;
						k++;
						devp->pre_buf[index] &= ~(1 << shift);
						//printk(KERN_CONT "0");
					}
		        }
		    }
	    }
	    //printk(KERN_CONT "\n");
    }
    
    return;
}

#else

static void led_dev_preprocess(struct led_dev *devp)
{
    int i = 0;
    int shift;
    u64 count;
	u64 bits_rst_org, bytes_rst_align;
	u64 bits_t1h, bits_t1l, bits_t0h, bits_t0l;
	u32 *pos;
	u32 mask = 0x80000000;
	led_rgb_u *ledp = (led_rgb_u *)devp->led;
	led_rgb_u *_ledp = NULL;

	bits_t1h = devp->attr.t1h_ns;
	bits_t1h *= devp->attr.speed_hz;
	bits_t1h /= 1000000000;
	
	bits_t1l = devp->attr.t1l_ns;
	bits_t1l *= devp->attr.speed_hz;
	bits_t1l /= 1000000000;

	bits_t0h = devp->attr.t0h_ns;
	bits_t0h *= devp->attr.speed_hz;
	bits_t0h /= 1000000000;
	
	bits_t0l = devp->attr.t0l_ns;
	bits_t0l *= devp->attr.speed_hz;
	bits_t0l /= 1000000000;
	

	/*在开始传输之前复位*/
	bits_rst_org = devp->attr.rst_us;
	bits_rst_org *= devp->attr.speed_hz;
	bits_rst_org /= 1000000;
	bytes_rst_align = (bits_rst_org + 7) / 8;
	bytes_rst_align = (bytes_rst_align + 3) / 4;
	bytes_rst_align *= 4;

	memset(devp->pre_buf, 0x00, devp->pre_len);
	
	for (i = 0; i < devp->count; i++)
	{
		_ledp = ledp + i;
		pos = (unsigned int *)(devp->pre_buf + bytes_rst_align + i*devp->bytes_align);
		/*
		printk(KERN_CONT "value: 0x%08X, green: %02X, red: %02X, blue: %02X\n", 
			_ledp->value, _ledp->rgb.green, _ledp->rgb.red, _ledp->rgb.blue);
		*/
		for (mask = 0x80000000, shift = 31; mask > 0x80; mask >>= 1)
		{
			if (_ledp->value & mask)
			{
				count = bits_t1h;
				while (count--)
				{
					*pos |= (1 << shift--);
					if (shift < 0)
					{
						pos++;
						shift = 31;
					}
					//printk(KERN_CONT "1");
				}

				count = bits_t1l;
				while (count--)
				{
					*pos &= ~(1 << shift--);
					if (shift < 0)
					{
						pos++;
						shift = 31;
					}
					//printk(KERN_CONT "0");
				}
					
			}
			else
			{
				count = bits_t0h;
				while (count--)
				{
					*pos |= (1 << shift--);
					if (shift < 0)
					{
						pos++;
						shift = 31;
					}
					//printk(KERN_CONT "1");
				}

				count = bits_t0l;
				while (count--)
				{
					*pos &= ~(1 << shift--);
					if (shift < 0)
					{
						pos++;
						shift = 31;
					}
					//printk(KERN_CONT "0");
				}
			}
		}
		//printk(KERN_CONT "\n");
	}

    return;
}

#endif

static int led_dev_open(struct inode *inode, struct file *filp)
{
    int ret = -1;
    filp->private_data = led_devp;

    ret = nonseekable_open (inode, filp);
    if (0 != ret)
    {
        dbgmsg ("disable seek function failed\n");
        goto EXIT;
    }

    ret = 0;
EXIT:
    return ret;
}

static int led_dev_release(struct inode *inode, struct file *filp)
{
	struct led_dev *devp = filp->private_data;
	devp->ismmap = 0;
    return 0;
}

static int spi_led_write(struct led_dev *devp)
{
	int ret = -1;
    struct spi_transfer t;
    struct spi_message  m;
    struct spi_device *spi;

	spin_lock_irq(&devp->spi_lock);
	spi = devp->spi;
	spin_unlock_irq(&devp->spi_lock);

	t.tx_buf 	= devp->pre_buf;
	t.rx_buf	= NULL;
	t.len		= devp->pre_len;
	t.speed_hz	= devp->attr.speed_hz;
	t.tx_nbits = 1;
	t.rx_nbits = 1;
	t.bits_per_word = BITS_PERWORD;
	
	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	
	//spi->mode |= SPI_LSB_FIRST | SPI_MODE_0;

	if (spi == NULL)
		ret = -ESHUTDOWN;
	else
		ret = spi_sync(spi, &m);

	if (ret == 0)
		ret = m.actual_length;
	
	return ret;
}

static ssize_t led_dev_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = -1;
    struct led_dev *devp = filp->private_data;

	mutex_lock(&devp->mutex);
	if (!devp->ismmap)
	{
		ret = copy_from_user((void *)devp->led, buf, count);
		if (ret)
		{
			goto EXIT;
		}
	}

	led_dev_preprocess(devp);

	ret = spi_led_write(devp);
	mutex_unlock(&devp->mutex);

	if (ret == devp->pre_len)
		ret = count;

EXIT:
    return ret;
}

static int led_dev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = -1;
	struct led_dev *devp = filp->private_data;
	u64 pfn = virt_to_phys((void *)devp->led) >> PAGE_SHIFT;
	u64 size = vma->vm_end - vma->vm_start;
	printk ("led_dev_mmap pfn: 0x%llx, size: %llu\n", pfn, size);

	//vma->vm_flags |= (VM_IO | VM_LOCKED | (VM_DONTEXPAND | VM_DONTDUMP));

	ret = remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot);
	if(ret != 0)
	{
		dbgmsg ("map 0x%llx to 0x%lx, size: 0x%llx faild\n", pfn, vma->vm_start, size);
		ret = -EAGAIN;
		goto EXIT;
	}
	devp->ismmap = 1;
	printk ("led_dev_mmap susccess\n");

EXIT:
	return ret;
}

static long led_dev_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = -1;
    struct led_dev *devp = filp->private_data;

    switch (cmd)
    {
    	case CMD_IOCTL_LED_INIT:
    	{
    		led_attr_t attr;
    		memset(&attr, 0x00, sizeof(led_attr_t));
    		ret = copy_from_user(&attr, (void *)arg, sizeof(led_attr_t));
            if (0 != ret)
            {
                dbgmsg ("remaining %d bytes copy failed\n", ret);
                goto EXIT;
            }
            ret = led_dev_init(devp, &attr);
			break;
    	}
    	case CMD_IOCTL_LED_DEINIT:
    	{
            ret = led_dev_deinit(devp);
			break;
    	}
        default:
            dbgmsg ("command %d is invalid\n", cmd);
            ret = -1;
            break;
    }

EXIT:
    return ret;
}

struct file_operations led_dev_fops =
{
	.owner		= THIS_MODULE,
	.open		= led_dev_open,
	.release	= led_dev_release,
	.write		= led_dev_write,
	.mmap		= led_dev_mmap,
	.unlocked_ioctl = led_dev_ioctl,
	.compat_ioctl	= led_dev_ioctl,
	.llseek 		= no_llseek,
};


static int led_drv_probe(struct spi_device *spi)
{
	int ret = -1;

    led_devp = kzalloc(sizeof(struct led_dev), GFP_KERNEL);
    if (!led_devp)
    {
        dbgmsg("request led-dev mem failed\n");
        ret = -ENOMEM;
        goto EXIT;
    }
    //dbgmsg("request led-dev mem success !!!\n");

    led_devp->misc = kzalloc(sizeof(struct miscdevice), GFP_KERNEL);
    if (!led_devp->misc)
    {
        dbgmsg("request misc device failed\n");
        ret = -ENOMEM;
        goto EXIT;
    }
    //dbgmsg("request misc device success !!!\n");

    led_devp->misc->name = "ws2812b";
    led_devp->misc->minor = MISC_DYNAMIC_MINOR;
    led_devp->misc->fops = &led_dev_fops;

    spin_lock_init(&led_devp->spi_lock);
    mutex_init(&led_devp->mutex);

    ret = misc_register (led_devp->misc);
	if (ret) {
		dbgmsg ("Failed to create misc device, err = %d\n", ret);
		ret = -EPERM;
		goto EXIT;
	}
	//dbgmsg ("create misc devic success !!!\n");

    led_devp->spi = spi;
	led_devp->attr.speed_hz = spi->max_speed_hz;
    spi_set_drvdata (spi, led_devp);

    return ret;

EXIT:
    if (led_devp && led_devp->misc)
    {
        misc_deregister(led_devp->misc);

        kfree(led_devp->misc);
        led_devp->misc = NULL;
    }
        
    if (led_devp)
    {
        kfree (led_devp);
        led_devp = NULL;
    }
        
    return ret;
}

static int led_drv_remove(struct spi_device *spi)
{
    struct led_dev *devp = spi_get_drvdata (spi);
    
    spin_lock_irq(&devp->spi_lock);
	devp->spi = NULL;
	spin_unlock_irq(&devp->spi_lock);
	
    if (devp && devp->misc)
    {
        misc_deregister (devp->misc);
        kfree(devp->misc);
        devp->misc = NULL;
    }

    if (devp)
    {
        kfree (devp);
        devp = NULL;
    }

	return 0;
}

static struct spi_device_id led_dev_id_table[] = 
{
    {"ws2812b", 0},
    {}
};
MODULE_DEVICE_TABLE(spi, led_dev_id_table);


static const struct of_device_id spiled_of_match[] = {
	{ .compatible = "ws2812b" },
	{},
};
MODULE_DEVICE_TABLE(of, spiled_of_match);


static struct spi_driver led_spi_drv =
{
    .driver = {
        .name = "ws2812b",
        .owner = THIS_MODULE,
        .of_match_table = spiled_of_match,
    },
    .probe = led_drv_probe,
    .remove = led_drv_remove,
    .id_table = led_dev_id_table,
};

/************************************ module init and exit ***************************************************/
static int __init led_init(void)
{
    int ret = -1;
	/*
    struct spi_board_info led_spi_info[] = {
        {
            .modalias = LED_DEV_NAME,
            .max_speed_hz = 25000000,
            .bus_num = 1,
            .chip_select = 1,
            .mode = SPI_MODE_0 | SPI_NO_CS,
        },
    };

    ret = spi_register_board_info (led_spi_info, ARRAY_SIZE(led_spi_info));
    if (ret != 0)
    {
        dbgmsg ("spi_register_board_info failed\n");
        ret = -ENODEV;
        goto EXIT;
    }
	*/
    ret = spi_register_driver (&led_spi_drv);
    if (ret != 0)
    {
        dbgmsg ("spi_register_driver failed\n");
        ret = -ENODEV;
        goto EXIT;
    }
    dbgmsg ("spi_register_driver success!!!\n");

EXIT:
    return ret;
}
module_init (led_init);

static void __exit led_exit (void)
{
    spi_unregister_driver (&led_spi_drv);
}
module_exit (led_exit);

MODULE_AUTHOR ("Baikun Xu <xubaikun@canaan-creative.com>");
MODULE_LICENSE ("GPL v2");