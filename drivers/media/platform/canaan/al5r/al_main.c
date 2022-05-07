#include <linux/cdev.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
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
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>

#include "al_ioctl.h"
#include "al_alloc_ioctl.h"
#include "al_ip.h"

int al5r_codec_major;
int al5r_codec_nr_devs = AL5R_NR_DEVS;
module_param(al5r_codec_nr_devs, int, S_IRUGO);
static struct class *module_class;

int channel_is_ready(struct al5r_codec_chan *chan)
{
	unsigned long flags;
	int ret = chan->unblock;

	spin_lock_irqsave(&chan->codec->i_lock, flags);
	ret = ret || !list_empty(&chan->codec->irq_masks);
	spin_unlock_irqrestore(&chan->codec->i_lock, flags);
	return ret;
}

static int al5r_codec_open(struct inode *inode, struct file *filp)
{
	struct al5r_codec_chan *chan;
	int ret;

	/* initialize channel */
	chan = kzalloc(sizeof(struct al5r_codec_chan), GFP_KERNEL);
	if (!chan) {
		ret = -ENOMEM;
		goto fail;
	}

	INIT_LIST_HEAD(&chan->mem);
	spin_lock_init(&chan->lock);
	chan->num_bufs = 0;

	filp->private_data = chan;

	/* irq */
	init_waitqueue_head(&chan->irq_queue);

	ret = al5r_codec_bind_channel(chan, inode);
	if (ret)
		goto fail_codec_binding;

	return 0;

fail_codec_binding:
	kzfree(chan);
fail:
	return ret;
}

static int al5r_codec_release(struct inode *inode, struct file *filp)
{
	struct al5r_codec_chan *chan = filp->private_data;
	struct al5_dma_buf_mmap *tmp;
	struct list_head *pos, *n;

	al5r_codec_unbind_channel(chan);
	list_for_each_safe(pos, n, &chan->mem){
		tmp = list_entry(pos, struct al5_dma_buf_mmap, list);
		list_del(pos);
		kfree(tmp);
	}

	kfree(chan);
	return 0;
}

static long al5r_codec_compat_ioctl(struct file *file, unsigned int cmd,
				    unsigned long arg)
{
	long ret = -ENOIOCTLCMD;

	// al5_info("DRIVER: al5r_codec_compat_ioctl\n");

	if (file->f_op->unlocked_ioctl)
		ret = file->f_op->unlocked_ioctl(file, cmd, arg);

	return ret;
}

static struct al5_dma_buffer *find_buf_by_id(struct al5r_codec_chan *chan, int desc_id)
{
	struct al5_dma_buf_mmap *cur_buf_mmap;

	list_for_each_entry(cur_buf_mmap, &chan->mem, list){
		if (cur_buf_mmap->buf_id == desc_id)
			return cur_buf_mmap->buf;
	}

	return NULL;
}

static int al5r_dma_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct al5r_codec_chan *chan = filp->private_data;
	unsigned long start = vma->vm_start;
	unsigned long vsize = vma->vm_end - start;
	/* offset if already in page */
	int desc_id = vma->vm_pgoff;
	int ret = 0;
	struct al5_dma_buffer *buf = find_buf_by_id(chan, desc_id);

	if (!buf)
		return -EINVAL;

	vma->vm_pgoff = 0;

	ret = dma_mmap_coherent(chan->codec->device, vma, buf->cpu_handle,
				buf->dma_handle, vsize);
	if (ret < 0) {
		pr_err("Remapping memory failed, error: %d\n", ret);
		return ret;
	}

	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;

	return 0;
}


/* //cuiyan
static int al5r_mmap(struct file *file, struct vm_area_struct *vma)
{
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    if(remap_pfn_range(vma,
                        vma->vm_start,
                        (0x03000000 >> PAGE_SHIFT),
                        vma->vm_end - vma->vm_start,
                        vma->vm_page_prot))
    {          
        printk("h264_mmap: remap_pfn_range fail \n");
        return -EAGAIN;
    }
    return 0;
} */

static int unblock_channel(struct al5r_codec_chan *chan)
{
	chan->unblock = 1;
	wake_up_interruptible(&chan->irq_queue);
	return 0;
}

static int wait_irq(struct al5r_codec_chan *chan, unsigned long arg)
{
	struct al5r_codec_desc *codec = chan->codec;
	int callback;
	struct r_irq *i_callback;
	unsigned long flags;
	int ret;

	ret =wait_event_interruptible(chan->irq_queue,
				       channel_is_ready(chan));
	//al5_info("ret = %d\n",ret);
	if (ret == -ERESTARTSYS)
		return ret;
	if (chan->unblock) {
		al5_dbg("Unblocking channel\n");
		return -EINTR;
	}

	spin_lock_irqsave(&codec->i_lock, flags);
	i_callback = list_first_entry(&chan->codec->irq_masks,
				      struct r_irq, list);
	callback = i_callback->bitfield;
	list_del(&i_callback->list);
	kmem_cache_free(codec->cache, i_callback);
	spin_unlock_irqrestore(&codec->i_lock, flags);

	if (copy_to_user((void *)arg, &callback, sizeof(__u32)))
		return -EFAULT;
	//al5_info("arg = 0x%x\n",arg);
	//al5_info("callback = 0x%x\n", callback);
	// al5_info("*arg = 0x%x\n",*(uint32_t*)arg);
	return ret;
}

static int read_reg(struct al5r_codec_chan *chan, unsigned long arg)
{
	struct al5_reg reg;
	struct al5r_codec_desc *codec = chan->codec;
	int err;

	// al5_info("DRIVER: read_reg\n");

	if (copy_from_user(&reg, (struct al5_reg *)arg, sizeof(struct al5_reg)))
	{
		// al5_info("DRIVER:copy_from_user Error!\n");
		// al5_info("arg = 0x%x\n", arg);
		// al5_info("reg = 0x%x 0x%x\n", reg.id, reg.value);
		return -EFAULT;
	}
	// al5_info("copy_from_user\n");	

	// al5_info("arg = 0x%x\n", arg);
	// // al5_info("*arg = 0x%x 0x%x\n", ((struct al5_reg *)arg)->id, ((struct al5_reg *)arg)->value);
	// al5_info("reg = 0x%x\n", reg);

	if (reg.id % 4) {
		al5_err("Unaligned register access: 0x%.4X\n",
			reg.id);
		return -EINVAL;
	}
	if (reg.id < 0x8000 || reg.id > chan->codec->regs_size) {
		al5_err("Out-of-range register read: 0x%.4X\n",
			reg.id);
		al5_err("Out-of-range register read: 0x%.4X\n",
			reg.id);/* cuiyan */
		return -EINVAL;
	}

	err = al5r_codec_read_register(chan, &reg);
	if (err)
		return err;

	if (copy_to_user((struct al5_reg *)arg, &reg, sizeof(struct al5_reg)))
		return -EFAULT;
	
	// al5_info("copy_to_user done\n");
	// al5_info("arg = 0x%x\n", &arg);
	// al5_info("reg = 0x%x 0x%x\n", reg.id, reg.value);

	al5_dbg("Reg read: 0x%.4X: 0x%.8x\n", reg.id, reg.value);

	return 0;
}

static int write_reg(struct al5r_codec_chan *chan, unsigned long arg)
{
	struct al5_reg reg;
	struct al5r_codec_desc *codec = chan->codec;
	// al5_info("DRIVER: write_reg\n");

	if (copy_from_user(&reg, (struct al5_reg *)arg, sizeof(struct al5_reg)))
		return -EFAULT;
	
	// al5_info("arg = 0x%x\n", arg);

	if (reg.id == 0x8084 || reg.id == 0x8094)
		al5_dbg("Reg write: 0x%.4X: 0x%.8x\n", reg.id, reg.value);

	if (reg.id % 4) {
		al5_err("Unaligned register access: 0x%.4X\n",
			reg.id);
		return -EINVAL;
	}
	if (reg.id < 0x8000 || reg.id > chan->codec->regs_size) {
		al5_dbg("Out-of-range register write: 0x%.4X\n", reg.id);
		return -EINVAL;
	}

	al5r_codec_write_register(chan, &reg);

	if (copy_to_user((struct al5_reg *)arg, &reg, sizeof(struct al5_reg)))
		return -EFAULT;

	return 0;
}

//cuiyan
static int irq_cnt(struct al5r_codec_chan *chan, unsigned long arg)
{
	struct al5r_codec_desc *codec = chan->codec;
	unsigned int cnt;
	cnt = codec->irq_cnt;

	copy_to_user((unsigned int*)arg, &cnt, sizeof(cnt));

	return 0;
}

//cuiyan
static int clr_irq(struct al5r_codec_chan *chan, unsigned long arg)
{
	struct al5r_codec_desc *codec = chan->codec;
	// codec->irq_cnt -= arg;
	uint32_t clr_cnt;
	copy_from_user(&clr_cnt,(uint32_t*)arg,sizeof(uint32_t));
	codec->irq_cnt -= clr_cnt;

	// al5_info("DRIVER:clr_irq:%d\n",codec->irq_cnt);

	return 0;
}

static long al5r_codec_ioctl(struct file *filp, unsigned int cmd,
			     unsigned long arg)
{
	struct al5r_codec_chan *chan = filp->private_data;
	struct al5r_codec_desc *codec = chan->codec;

	// al5_info("DRIVER: al5r_codec_ioctl\n");

	switch (cmd) {
	case GET_DMA_MMAP:
		return al5_ioctl_get_dma_mmap(codec->device, chan, arg);
	case GET_DMA_FD:
		return al5_ioctl_get_dma_fd(codec->device, arg);
	case GET_DMA_PHY:
		return al5_ioctl_get_dmabuf_dma_addr(codec->device, arg);
	case AL_CMD_UNBLOCK_CHANNEL:
		return unblock_channel(chan);
	case AL_CMD_IP_WAIT_IRQ:
		return wait_irq(chan, arg);
	case AL_CMD_IP_READ_REG:
		return read_reg(chan, arg);
	case AL_CMD_IP_WRITE_REG:
		return write_reg(chan, arg);
	case AL_CMD_IP_IRQ_CNT:
		return irq_cnt(chan, arg);
	case AL_CMD_IP_CLR_IRQ:
		return clr_irq(chan, arg);
	default:
		al5_err("Unknown ioctl: 0x%.8X\n", cmd);
		return -EINVAL;
	}
}

const struct file_operations al5r_codec_fops = {
	.owner		= THIS_MODULE,
	.open		= al5r_codec_open,
	.release	= al5r_codec_release,
	.unlocked_ioctl = al5r_codec_ioctl,
	.compat_ioctl	= al5r_codec_compat_ioctl,
	.mmap		= al5r_dma_mmap,
	// .mmap		= al5r_mmap, //cuiyan
};

void clean_up_al5r_codec_cdev(struct al5r_codec_desc *dev)
{
	cdev_del(&dev->cdev);
}

int setup_chrdev_region(void)
{
	dev_t dev = 0;
	int err;

	if (al5r_codec_major == 0) {
		err = alloc_chrdev_region(&dev, 0, al5r_codec_nr_devs, "al5r");
		al5r_codec_major = MAJOR(dev);

		if (err) {
			pr_alert("Allegro codec: can't get major %d\n",
				 al5r_codec_major);
			return err;
		}
	}
	return 0;
}

int al5r_setup_codec_cdev(struct al5r_codec_desc *codec, int minor,
			  const char *device_name)
{
	struct device *device;
	int err, devno =
		MKDEV(al5r_codec_major, minor);

	cdev_init(&codec->cdev, &al5r_codec_fops);
	codec->cdev.owner = THIS_MODULE;
	err = cdev_add(&codec->cdev, devno, 1);
	if (err) {
		al5_err("Error %d adding allegro device number %d", err, minor);
		return err;
	}

	if (device_name != NULL) {
		device = device_create(module_class, NULL, devno, NULL,
				       device_name);
		if (IS_ERR(device)) {
			pr_err("device not created\n");
			clean_up_al5r_codec_cdev(codec);
			return PTR_ERR(device);
		}
	}

	return 0;
}

static int init_codec_desc(struct al5r_codec_desc *codec)
{
	INIT_LIST_HEAD(&codec->irq_masks);
	spin_lock_init(&codec->i_lock);
	/* make chan requirement explicit */
	codec->chan = NULL;
	codec->cache =  kmem_cache_create("al_codec_ram",
					  sizeof(struct r_irq),
					  0, SLAB_HWCACHE_ALIGN, NULL);
	if (!codec->cache)
		return -ENOMEM;

	return 0;
}

static void deinit_codec_desc(struct al5r_codec_desc *codec)
{
	kmem_cache_destroy(codec->cache);
}

int al5r_codec_probe(struct platform_device *pdev)
{
	int err, irq;
	static int current_minor;
	struct resource *res;
	struct al5r_codec_desc *codec
		= devm_kzalloc(&pdev->dev, sizeof(*codec), GFP_KERNEL);
	char *device_name;
	bool has_irq = false;

	struct clk *clk;

	al5_info("al5r_codec_probe\n");

	// clk = clk_get(&pdev->dev, "clk");
	// clk_enable(clk);

	if (!codec)
		return -ENOMEM;

	codec->device = &pdev->dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		al5_err("Can't get resource\n");
		err = -ENODEV;
		goto out_no_resource;
	}

	codec->regs = devm_ioremap_nocache(&pdev->dev,
					   res->start, resource_size(res));
	codec->regs_size = res->end - res->start;
	// al5_info("reg_size = 0x%x\n",codec->regs_size);
	codec->irq_cnt = 0;//cuiyan

	if (IS_ERR(codec->regs)) {
		al5_err("Can't map registers\n");
		err = PTR_ERR(codec->regs);
		goto out_map_register;
	}

	has_irq = true;
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		al5_info("No irq requested / Couldn't obtain request irq\n");
		has_irq = false;
	}

	err = init_codec_desc(codec);
	if (err)
		goto out_failed_request_irq;

	if (has_irq) {
		err = devm_request_irq(codec->device,
				       irq,
				       al5r_hardirq_handler,
				       IRQF_SHARED, dev_name(&pdev->dev), codec);
		if (err) {
			al5_err("Failed to request IRQ #%d -> :%d\n",
				irq, err);
			goto out_failed_request_irq;
		}
	}

	platform_set_drvdata(pdev, codec);
	
#if 0
	if (of_property_read_string(codec->device->of_node, "al,devicename",
				    (const char **)&device_name) != 0)
		device_name = NULL;

	err = al5r_setup_codec_cdev(codec, current_minor, device_name);
#else
	err = al5r_setup_codec_cdev(codec, current_minor, "h264-codec");
#endif
	if (err)
		return err;

	codec->minor = current_minor;
	++current_minor;

	return 0;

out_failed_request_irq:
out_map_register:
out_no_resource:
	return err;

}

int al5r_codec_remove(struct platform_device *pdev)
{
	struct al5r_codec_desc *codec = platform_get_drvdata(pdev);
	dev_t dev = MKDEV(al5r_codec_major, codec->minor);

	device_destroy(module_class, dev);
	clean_up_al5r_codec_cdev(codec);
	deinit_codec_desc(codec);

	return 0;
}

static int al5r_codec_pci_probe(struct pci_dev *pdev,
				const struct pci_device_id *id)
{
	struct al5r_codec_desc *codec =
		devm_kzalloc(&pdev->dev, sizeof(*codec), GFP_KERNEL);
	const char *device_name = dev_name(&pdev->dev);
	static int current_minor;
	int err;

	u8 irq;

	al5_info("al5r_codec_pci_probe\n");

	if (!codec)
		return -ENOMEM;

	codec->device = &pdev->dev;

	if (pci_enable_device(pdev))
		goto out_free;

	if (pci_request_regions(pdev, device_name))
		goto out_disable;

	codec->regs = pci_ioremap_bar(pdev, 0);
	if (!codec->regs)
		goto out_unrequest;
	codec->regs_size = (unsigned long)pci_resource_end(pdev, 0) -
			   pci_resource_start(pdev, 0);

#ifdef CONFIG_PCI_MSI_IRQ_DOMAIN
	if (pci_enable_msi(pdev) == 0)
		irq = pdev->irq;
	else
		goto out_iounmap;
#else
	goto out_iounmap;
#endif

	err = init_codec_desc(codec);
	if (err)
		goto out_disable_msi;

	if (request_irq(irq,
			al5r_hardirq_handler,
			IRQF_SHARED, device_name, codec))
		goto out_disable_msi;

	pci_set_drvdata(pdev, codec);

	err = al5r_setup_codec_cdev(codec, current_minor, NULL);
	if (err)
		return err;

	codec->minor = current_minor;
	++current_minor;

	return 0;

out_disable_msi:
#ifdef CONFIG_PCI_MSI_IRQ_DOMAIN
	pci_disable_msi(pdev);
#endif
out_iounmap:
	iounmap(codec->regs);
out_unrequest:
	pci_release_regions(pdev);
out_disable:
	pci_disable_device(pdev);
out_free:
	return -ENODEV;
}

static void al5r_codec_pci_remove(struct pci_dev *pdev)
{
	struct al5r_codec_desc *codec = pci_get_drvdata(pdev);

	free_irq(pdev->irq, codec);
#ifdef CONFIG_PCI_MSI_IRQ_DOMAIN
	pci_disable_msi(pdev);
#endif
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	iounmap(codec->regs);
	clean_up_al5r_codec_cdev(codec);
	deinit_codec_desc(codec);
}

#define PCI_TEST_VENDOR 0xcafe
#define PCI_TEST_DEVICE 0xdead

static struct pci_device_id al5r_pci_ids[] = {
	{ PCI_DEVICE(PCI_TEST_VENDOR, PCI_TEST_DEVICE) },
	{ 0 },
};

static struct pci_driver al5r_pci_driver = {
	.name		= "al5r",
	.id_table	= al5r_pci_ids,
	.probe		= al5r_codec_pci_probe,
	.remove		= al5r_codec_pci_remove,
};

static const struct of_device_id al5r_codec_of_match[] = {
	{ .compatible = "al,al5r" },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, al5r_codec_of_match);

static struct platform_driver al5r_platform_driver = {
	.probe			= al5r_codec_probe,
	.remove			= al5r_codec_remove,
	.driver			=       {
		.name		= "al5r",
		.of_match_table = of_match_ptr(al5r_codec_of_match),
	},
};

static int create_module_class(void)
{
	module_class = class_create(THIS_MODULE, "al5r_class");
	if (IS_ERR(module_class))
		return PTR_ERR(module_class);

	return 0;
}

static void destroy_module_class(void)
{
	class_destroy(module_class);
}

int al5r_module_init(void)
{
	int ret;

	ret = pci_register_driver(&al5r_pci_driver);
	if (ret)
		pr_alert("Init failed with %d for allegro pci driver", ret);

	ret = platform_driver_register(&al5r_platform_driver);
	if (ret)
		pci_unregister_driver(&al5r_pci_driver);

	return ret;
}

void al5r_module_deinit(void)
{
	platform_driver_unregister(&al5r_platform_driver);
	pci_unregister_driver(&al5r_pci_driver);
}

static int __init al5r_codec_init(void)
{
	dev_t devno;
	int err = setup_chrdev_region();

	// /* added by cuiyan */
	// /* config h264 clock */
    // uint32_t *sysctl_clk_base = ioremap(0x97000000+0x1000, 0x1000);
    // /* 
    // sysctl_clk->ax25m_clk_cfg = ((1 << 0) | (1 << 24));
    // sysctl_clk->h264_aclk_en = (enable == true) ? ((1 << 0) | (1 << 16)) : ((0 << 0) | (1 << 16));
    //  */
    // *(sysctl_clk_base + 0x0) = ((1 << 0) | (1 << 24));//ax25m_clk_cfg, sysctl_clk_set_leaf_parent(SYSCTL_CLK_AX25M_SRC, SYSCTL_CLK_ROOT_PLL0);
    // *(sysctl_clk_base + 0x150/4) = ((1 << 0) | (1 << 16));//h264_aclk_en,     sysctl_clk_set_leaf_en(SYSCTL_CLK_NOC_CLK_1_H264_AXI, 1);
	// //end of cuiyan's code

	if (err)
		return err;

	err = create_module_class();
	if (err)
		goto fail;

	err = al5r_module_init();
	if (err)
		goto fail;

	return 0;

fail:
	devno = MKDEV(al5r_codec_major, 0);
	unregister_chrdev_region(devno, al5r_codec_nr_devs);

	return err;
}

static void __exit al5r_codec_exit(void)
{
	dev_t devno = MKDEV(al5r_codec_major, 0);

	al5r_module_deinit();
	destroy_module_class();
	unregister_chrdev_region(devno, al5r_codec_nr_devs);
}

module_init(al5r_codec_init);
module_exit(al5r_codec_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kevin Grandemange");
MODULE_AUTHOR("Sebastien Alaiwan");
MODULE_DESCRIPTION("Allegro Codec Driver");
