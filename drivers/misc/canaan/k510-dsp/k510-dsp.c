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

#define DSP_CMD_BOOT             _IOWR('q', 1, unsigned long)

#define SYSCTL_RST_REG_OFFSET 0x2000
#define SYSCTL_AX25P_RST_CTL_OFF 0x14
#define SYSCTL_AX25P_CORE_RSTVEC_OFF 0x108

struct dsp_plat {
    // struct resource *res;
    void __iomem    *sysctl_base;
    unsigned long   dsp_reset_vec;
    int             major;
    int             minor;
    struct class    *class;
    struct cdev     cdev;
};

static struct dsp_plat *plat;


static int dsp_boot(unsigned long dsp_reset_vec)
{
    iowrite32(1 << 30 | 1, plat->sysctl_base + SYSCTL_RST_REG_OFFSET + SYSCTL_AX25P_RST_CTL_OFF);

    iowrite32(dsp_reset_vec, plat->sysctl_base + SYSCTL_AX25P_CORE_RSTVEC_OFF);

    iowrite32(0, plat->sysctl_base + SYSCTL_RST_REG_OFFSET + SYSCTL_AX25P_RST_CTL_OFF);

    // printk("k510-dsp: dsp_boot, ax25p_core_rstvec=%x , ax25p_rst_ctl=%x \n", ioread32(plat->sysctl_base + SYSCTL_AX25P_CORE_RSTVEC_OFF), ioread32(plat->sysctl_base + SYSCTL_RST_REG_OFFSET + SYSCTL_AX25P_RST_CTL_OFF));
    return 0;
}


static int setup_chrdev_region(void)
{
        dev_t dev = 0;
        int err;

        if (plat->major == 0) {
                err = alloc_chrdev_region(&dev, 0, 1, "k510-dsp");
                plat->major = MAJOR(dev);

                if (err) {
                        printk("k510-dsp: can't get major %d\n", plat->major);
                        return err;
                }
        }
        return 0;
}


static int create_module_class(void)
{
        plat->class = class_create(THIS_MODULE, "k510_dsp_class");

        if (IS_ERR(plat->class))
                return PTR_ERR(plat->class);

        return 0;
}

static void clean_up_dsp_cdev(void)
{
        cdev_del(&plat->cdev);
}

static int dsp_open(struct inode *inode, struct file *filp)
{
        return 0;
}

static int dsp_release(struct inode *inode, struct file *filp)
{
        return 0;
}

static long dsp_ioctl(struct file *filp, unsigned int cmd,
                             unsigned long arg)
{
    switch (cmd) {
    case DSP_CMD_BOOT:
            return dsp_boot(arg);
    default:
            printk("dsp_ioctl: Unknown ioctl: 0x%.8X\n", cmd);
            return -EINVAL;
    }

}

// static int dsp_mmap(struct file *file, struct vm_area_struct *vma)
// {

//     if(remap_pfn_range(vma,
//                        vma->vm_start,
//                        0x20000000 >> PAGE_SHIFT,
//                        vma->vm_end - vma->vm_start,
//                        vma->vm_page_prot))
//     {
//         printk("dsp_mmap: remap_pfn_range fail \n");
//         return -EAGAIN;
//     }

//     return 0;

// }

const struct file_operations dsp_fops = {
        .owner          = THIS_MODULE,
        .open           = dsp_open,
        .release        = dsp_release,
        .unlocked_ioctl = dsp_ioctl,
        // .mmap           = dsp_mmap,
};

int setup_dsp_cdev(const char *device_name)
{
        struct device *device;
        int err, devno =
                MKDEV(plat->major, plat->minor);

        cdev_init(&plat->cdev, &dsp_fops);
        plat->cdev.owner = THIS_MODULE;
        err = cdev_add(&plat->cdev, devno, 1);
        if (err) {
                printk("dsp: Error %d adding dsp device number %d \n", err, plat->minor);
                return err;
        }

        if (device_name != NULL) {
                device = device_create(plat->class, NULL, devno, NULL,
                                       device_name);
                if (IS_ERR(device)) {
                        printk("dsp: device not created\n");
                        clean_up_dsp_cdev();
                        return PTR_ERR(device);
                }
        }

        return 0;
}

static int dsp_probe(struct platform_device *pdev)
{
        // struct resource *res;
        u32 sysctl_phy_addr;

        // res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        // if (!res) {
        //         printk("dsp_probe: get resource failed \n");
        //         return -ENODEV;
        // }

    plat = kzalloc(sizeof(struct dsp_plat), GFP_KERNEL);
    if (!plat) {
        printk("dsp_probe: kzalloc failed \n");
        return -ENOMEM;
    }

        // plat->res = res;
    device_property_read_u32(&pdev->dev, "sysctl-phy-addr", &sysctl_phy_addr);
//        device_property_read_u32(pdev->dev, "dsp-reset-vec", &plat->dsp_reset_vec);
/*
    if (!request_mem_region(sysctl_phy_addr, 0xffff, "k510-dsp")){
            printk("dsp_probe: Could not get io memory region of sysctl\n");
            kfree(plat);
            return -ENOMEM;
    }
*/
	plat->sysctl_base = ioremap(sysctl_phy_addr, 0xffff);
        if(!plat->sysctl_base) {
            printk("dsp_probe: ioremap failed \n");
            kfree(plat);
            return -ENOMEM;
        }

    plat->major = 0;
    plat->minor = 0;

    setup_chrdev_region();
    create_module_class();

    setup_dsp_cdev("k510-dsp");

    printk("k510 dsp driver loaded, mapped sysctl base %x at %px \n", sysctl_phy_addr, plat->sysctl_base);

	return 0;
}

static int dsp_remove(struct platform_device *pdev)
{
        dev_t dev = MKDEV(plat->major, plat->minor);

        device_destroy(plat->class, dev);
        clean_up_dsp_cdev();
        kfree(plat);

        return 0;
}


static const struct of_device_id k510_dsp_ids[] = {
	{ .compatible = "k510-dsp" },
	{}
};

static struct platform_driver k510_dsp_driver = {
    .probe          = dsp_probe,
    .remove         = dsp_remove,
    .driver         = {
        .name           = "k510-dsp",
        .of_match_table = of_match_ptr(k510_dsp_ids),
    },
};

int dsp_module_init(void)
{
        int ret;

        ret = platform_driver_register(&k510_dsp_driver);

        return ret;
}

void dsp_module_deinit(void)
{
        platform_driver_unregister(&k510_dsp_driver);
}

module_init(dsp_module_init);
module_exit(dsp_module_deinit);
MODULE_LICENSE("GPL v2");
