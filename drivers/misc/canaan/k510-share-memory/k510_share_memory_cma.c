#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/andesv5/proc.h>

#include <asm/dma-contiguous.h>
#include <linux/dma-contiguous.h>
#include <linux/cma.h>

extern void k510_sharemem_inval_range(unsigned long va_start, unsigned long pa_start, unsigned long size);
extern void k510_sharemem_wb_range(unsigned long va_start, unsigned long pa_start, unsigned long size);

#define SHARE_MEMORY_DEBUG

#ifdef SHARE_MEMORY_DEBUG
#define share_memory_log(format, ...) do {printk("k510-share-memory: " format, ##__VA_ARGS__);} while(0);
#else
#define share_memory_log(format, ...)
#endif

/* memory lib head */
typedef void *PART_ID;

typedef struct
{
    unsigned int numBytesFree;      /* Number of Free Bytes in Partition       */
    unsigned int numBlocksFree;     /* Number of Free Blocks in Partition      */
    unsigned int maxBlockSizeFree;  /* Maximum block size that is free.	      */
    unsigned int numBytesAlloc;     /* Number of Allocated Bytes in Partition  */
    unsigned int numBlocksAlloc;    /* Number of Allocated Blocks in Partition */
    unsigned int poolSize;          /* Number of Bytes in Partition (for R12C) */
}  MEM_PART_STATS;

/* memory lib c code */
#undef MEMLIB_DEBUG



#define SHARE_MEMORY_ALLOC          _IOWR('m', 1, unsigned long)
#define SHARE_MEMORY_ALIGN_ALLOC    _IOWR('m', 2, unsigned long)
#define SHARE_MEMORY_FREE           _IOWR('m', 3, unsigned long)
#define SHARE_MEMORY_SHOW           _IOWR('m', 4, unsigned long)
#define SHARE_MEMORY_INVAL_RANGE        _IOWR('m', 5, unsigned long)
#define SHARE_MEMORY_WB_RANGE           _IOWR('m', 6, unsigned long)

struct share_memory_plat {
    uint32_t        memory_phy_addr;    /* physical address */
    void __iomem    *memory_vir_addr;   /* virtual address */
    uint32_t        memory_sizes;
    PART_ID         part_id;
    int             major;
    int             minor;
    struct class    *class;
    struct device   *device;
    struct cdev     cdev;
    struct list_head buffer_list;
};

static struct share_memory_plat *plat;

static DEFINE_MUTEX(share_memory_mutex);

struct share_memory_alloc_args {
    uint32_t size;
    uint32_t phyAddr;
};

struct share_memory_alloc_align_args {
    uint32_t size;
    uint32_t alignment;
    uint32_t phyAddr;
};

struct memory_cache_sync_request {
    size_t size;
    uint64_t vaddr;
    uint64_t paddr;
};

struct memory_alloc_record {
    size_t size;
    uint64_t kvaddr;
    uint64_t paddr;
    struct list_head list;
};

static long share_memory_alloc(unsigned long arg)
{
    void *pBlockVirtual = NULL;
    uint32_t BlockPhysical = 0;
    struct share_memory_alloc_args alloc_args = {0,0};

    if(copy_from_user((void *)&alloc_args, (void *)arg, sizeof(struct share_memory_alloc_args))) {
        return -EFAULT;
    }

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_alloc share_memory_alloc size %d\r\n", (uint32_t)alloc_args.size);
#endif

    uint32_t count = PAGE_ALIGN(alloc_args.size) >> PAGE_SHIFT;
    uint32_t align = get_order(alloc_args.size);
    
    struct page *pages = cma_alloc(dev_get_cma_area(plat->device), count, align, GFP_KERNEL);

    if (!pages){
	share_memory_log("[ERROR] cma_alloc failure\r\n");
        return -ENOMEM;
    }

    struct memory_alloc_record *alloc_rec_ptr = kzalloc(sizeof(struct memory_alloc_record),GFP_NOWAIT);

    if(!alloc_rec_ptr){
        share_memory_log("[ERROR] kzalloc failure, can't alloc memory to record allocated cma buffer\r\n");
	return -ENOMEM;
    }


    alloc_rec_ptr->size = alloc_args.size;
    alloc_rec_ptr->kvaddr = page_address(pages);
    alloc_rec_ptr->paddr = page_to_phys(pages);

    /*Add allocated cma buffer record into the plat->buffer_list*/
    list_add_tail(&alloc_rec_ptr->list, &plat->buffer_list);
    
#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_alloc virtual address:0x%lx, physical address:0x%08x\r\n", alloc_rec_ptr->kvaddr, alloc_rec_ptr->paddr);
#endif
    alloc_args.phyAddr = alloc_rec_ptr->paddr;

    if(copy_to_user((void *)arg, (void *)&alloc_args, sizeof(struct share_memory_alloc_args)))
         return -EFAULT;

    if(0 == alloc_rec_ptr->paddr) {
        return -EFAULT;
    }
    else {
        return 0;
    }
}


static long share_memory_align_alloc(unsigned long arg)
{
    void *pBlockVirtual = NULL;
    uint32_t BlockPhysical = 0;
    struct share_memory_alloc_align_args alloc_args = {0,0,0};

    if(copy_from_user((void *)&alloc_args, (void *)arg, sizeof(struct share_memory_alloc_align_args)))
        return -EFAULT;

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_align_alloc size %d, alignment %d\r\n", alloc_args.size, alloc_args.alignment);
#endif

    uint32_t count = PAGE_ALIGN(alloc_args.size) >> PAGE_SHIFT;
    uint32_t align = alloc_args.alignment;

    struct page *pages = cma_alloc(dev_get_cma_area(plat->device), count, align, GFP_KERNEL);

    if (!pages){
        share_memory_log("[ERROR] cma_alloc failure\r\n");
        return -ENOMEM;
    }

    struct memory_alloc_record *alloc_rec_ptr = kzalloc(sizeof(struct memory_alloc_record),GFP_NOWAIT);

    if(!alloc_rec_ptr){
        share_memory_log("[ERROR] kzalloc failure, can't alloc memory to record allocated cma buffer\r\n");
        return -ENOMEM;
    }


    alloc_rec_ptr->size = alloc_args.size;
    alloc_rec_ptr->kvaddr = page_address(pages);
    alloc_rec_ptr->paddr = page_to_phys(pages);

    /*Add allocated cma buffer record into the plat->buffer_list*/
    list_add_tail(&alloc_rec_ptr->list, &plat->buffer_list);

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_align_alloc virtual address:0x%lx, physical address:0x%08x\r\n", alloc_rec_ptr->kvaddr, alloc_rec_ptr->paddr);
#endif

    alloc_args.phyAddr = alloc_rec_ptr->paddr;

    if(copy_to_user((void *)arg, (void *)&alloc_args, sizeof(struct share_memory_alloc_align_args)))
         return -EFAULT;

    if(0 == alloc_rec_ptr->paddr) {
        return -EFAULT;
    }
    else {
        return 0;
    }
}

static long share_memory_free(unsigned long arg)
{
    uint32_t size = NULL;
    uint32_t paddr = NULL;
    uint64_t kvaddr;
    struct memory_alloc_record *alloc_rec_ptr, *tmp;

    int ret;

    if(copy_from_user((void *)&paddr,(void *)arg, sizeof(uint32_t)))
        return -EFAULT;

    list_for_each_entry_safe(alloc_rec_ptr, tmp, &plat->buffer_list, list) {
        if(alloc_rec_ptr->paddr == paddr){
	    kvaddr = alloc_rec_ptr->kvaddr;
	    size = alloc_rec_ptr->size;
	    break;
	}
    }

    if(kvaddr == NULL && size == NULL){
        share_memory_log("[ERROR] share_memory_free: paddr(0x%08x) is not found in allocated buffer list \r\n", paddr);
        return 0;
    }

    struct page *pages = virt_to_page(kvaddr);
    uint32_t count = PAGE_ALIGN(size) >> PAGE_SHIFT;

    ret = cma_release(dev_get_cma_area(plat->device), pages, count);

    if(ret == false){
        share_memory_log("[ERROR] cma_release failure, cma buffer info: paddr=0x%08x, kvaddr=0x%lx, size=%d\r\n", alloc_rec_ptr->paddr, alloc_rec_ptr->kvaddr, alloc_rec_ptr->size);
        return -EFAULT;
    }

    list_del(&alloc_rec_ptr->list);
    kfree(alloc_rec_ptr);

#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_free: physical address:0x%08x virtual address 0x%lx\r\n",paddr,kvaddr);
#endif


    return 0;
}

static long share_memory_show(unsigned long arg)
{
    struct memory_alloc_record *alloc_rec_ptr, *tmp;

    printk("allocated cma buffer: \r\n");
    printk("paddr        kvaddr        size \r\n");
    list_for_each_entry_safe(alloc_rec_ptr, tmp, &plat->buffer_list, list) {
        printk("0x%08x    0x%lx    %d \r\n", alloc_rec_ptr->paddr, alloc_rec_ptr->kvaddr, alloc_rec_ptr->size);
    }

    printk("cma buffer show complete\r\n");

    return 0;
}

static long share_memory_inval_range(unsigned long arg)
{
    struct memory_cache_sync_request inval_range_args = {0, 0, 0};

    if(copy_from_user((void *)&inval_range_args, (void *)arg, sizeof(struct memory_cache_sync_request))) {
        return -EFAULT;
    }

    __asm__ __volatile__(
        "li t6, 0x00040000\t\n"
        "csrs sstatus, t6\t\n"
    );

    k510_sharemem_inval_range(inval_range_args.vaddr, inval_range_args.paddr, inval_range_args.size);

    __asm__ __volatile__(
        "li t6, 0x00040000\t\n"
        "csrc sstatus, t6\t\n"
    );
    return 0;

}

static long share_memory_wb_range(unsigned long arg)
{
    struct memory_cache_sync_request wb_range_args = {0, 0, 0};

    if(copy_from_user((void *)&wb_range_args, (void *)arg, sizeof(struct memory_cache_sync_request))) {
        return -EFAULT;
    }
    __asm__ __volatile__(
        "li t6, 0x00040000\t\n"
        "csrs sstatus, t6\t\n"
    );

    k510_sharemem_wb_range(wb_range_args.vaddr, wb_range_args.paddr, wb_range_args.size);

    __asm__ __volatile__(
        "li t6, 0x00040000\t\n"
        "csrc sstatus, t6\t\n"
    );
    return 0;
}

static long share_memory_ioctl(struct file *filp, unsigned int cmd,
                             unsigned long arg)
{
    switch (cmd) {
        case SHARE_MEMORY_ALLOC:
            return share_memory_alloc(arg);
        case SHARE_MEMORY_ALIGN_ALLOC:
            return share_memory_align_alloc(arg);
        case SHARE_MEMORY_FREE:
            return share_memory_free(arg);
        case SHARE_MEMORY_SHOW:
            return share_memory_show(arg);
	case SHARE_MEMORY_INVAL_RANGE:
	    return share_memory_inval_range(arg);
	case SHARE_MEMORY_WB_RANGE:
	    return share_memory_wb_range(arg);
        default:
            share_memory_log("[ERROR] share_memory_ioctl: Unknown ioctl: 0x%.8X\r\n", cmd);
            return -EINVAL;
    }
}

static long share_memory_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret;

    mutex_lock(&share_memory_mutex);
    ret = share_memory_ioctl(file, cmd, arg);
    mutex_unlock(&share_memory_mutex);

    return ret;
}

static int share_memory_open(struct inode *inode, struct file *filp)
{
#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_open success!\r\n");
#endif
    return 0;
}

static int share_memory_release(struct inode *inode, struct file *filp)
{
#ifdef MEMLIB_DEBUG
    share_memory_log("[INFO] share_memory_release success!\r\n");
#endif
    return 0;
}

const struct file_operations share_memory_fops = {
    .owner          = THIS_MODULE,
    .open           = share_memory_open,
    .release        = share_memory_release,
    .unlocked_ioctl = share_memory_unlocked_ioctl,
};

static int share_memory_probe(struct platform_device *pdev)
{
    struct resource *res;
    int err = 0;
    dev_t dev = 0;
    int devno;
/*
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        share_memory_log("[ERROR] share_memory_probe get resource failed \r\n");
        err = -ENODEV;
        goto error;
    }
*/
    plat = kzalloc(sizeof(struct share_memory_plat), GFP_KERNEL);
    if (!plat) {
        share_memory_log("[ERROR] share_memory_probe  kzalloc failed \r\n");
        err = -ENOMEM;
        goto error;
    }

    plat->major = 0;
    plat->minor = 0;

    INIT_LIST_HEAD(&plat->buffer_list);

    err = alloc_chrdev_region(&dev, 0, 1, "k510-share-memory");
    if (err) {
        share_memory_log("[ERROR] share_memory_probe  can't get major %d\r\n", plat->major);
        goto cleanup_kmem;
    }
    plat->major = MAJOR(dev);

    plat->class = class_create(THIS_MODULE, "k510_share_memory_class");
    if (IS_ERR(plat->class)) {
        err = PTR_ERR(plat->class);
        goto cleanup_cdev;
    }
    // create_module_class();
    devno = MKDEV(plat->major, plat->minor);

    cdev_init(&plat->cdev, &share_memory_fops);
    plat->cdev.owner = THIS_MODULE;
    err = cdev_add(&plat->cdev, devno, 1);
    if (err) {
        share_memory_log("[ERROR] share_memory_probe Error %d adding gnne device number %d \r\n", err, plat->minor);
        goto cleanup_class;
    }

    plat->device = device_create(plat->class, NULL, devno, NULL, "k510-share-memory");
    if (IS_ERR(plat->device)) {
        share_memory_log("[ERROR] share_memory_probe device NOT created\r\n");
        err = PTR_ERR(plat->device);
        goto cleanup_cdev;
    }

//    plat->memory_sizes          = (uint32_t)res->end - (uint32_t)res->start + 1;
//    plat->memory_phy_addr       = (uint32_t)res->start;
//    plat->memory_vir_addr       = ioremap(res->start,plat->memory_sizes);

//    if(NULL == plat->memory_vir_addr) {
//        share_memory_log("[ERROR] share_memory_probe ioremap error!\r\n");
//        goto cleanup_device;
//    }
#ifdef MEMLIB_DEBUG
//    share_memory_log("[INFO] share_memory_probe phy address 0x%08x virtual address 0x%lx size 0x%08x\r\n",plat->memory_phy_addr,(long)plat->memory_vir_addr, plat->memory_sizes);
#endif


#ifdef MEMLIB_DEBUG
    share_memory_log("share_memory_probe success!\r\n");
#endif

    return 0;

//cleanup_ioremap:
//    iounmap(plat->memory_vir_addr);
cleanup_device:
    device_destroy(plat->class, dev);
cleanup_class:
    class_destroy(plat->class);
cleanup_cdev:
    cdev_del(&plat->cdev);
cleanup_kmem:
    kfree(plat);
error:
	return err;
}

static int share_memory_remove(struct platform_device *pdev)
{
    dev_t dev = MKDEV(plat->major, plat->minor);

    device_destroy(plat->class, dev);
    cdev_del(&plat->cdev);
    class_destroy(plat->class);
    kfree(plat);

    return 0;
}

static const struct of_device_id k510_share_memory_ids[] = {
    { .compatible = "k510-share-memory-cma" },
    {}
};

static struct platform_driver k510_share_memory_driver = {
    .probe          = share_memory_probe,
    .remove         = share_memory_remove,
    .driver         = {
        .name           = "k510-share-memory",
        .of_match_table = of_match_ptr(k510_share_memory_ids),
    },
};

int share_memory_module_init(void)
{
    int ret;
    ret = platform_driver_register(&k510_share_memory_driver);
    return ret;
}

void share_memory_module_deinit(void)
{
    platform_driver_unregister(&k510_share_memory_driver);
}

module_init(share_memory_module_init);
module_exit(share_memory_module_deinit);
MODULE_LICENSE("GPL v2");
