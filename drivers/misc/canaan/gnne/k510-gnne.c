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
#include <linux/irq.h>    //含有IRQ_HANDLED\IRQ_TYPE_EDGE_RISING
// #include <asm-arm/irq.h>   //含有IRQT_BOTHEDGE触发类型
#include <linux/interrupt.h> //含有request_irq、free_irq函数
#include <linux/poll.h>

#define gnne_log(format, ...)       do{if (debug) {printk("GNNE: " format, ##__VA_ARGS__);}} while(0);

#define GNNE_ACLK_CFG       (0x97001020U)
#define GNNE_SYSCLK_CFG     (0x97001028U)

#define GNNE_ENABLE_ACLK    (0x08000200U)
#define GNNE_DISABLE_ACLK   (0x08000000U)
#define GNNE_ENABLE_SYSCLK  (0x04000100U)
#define GNNE_DISABLE_SYSCLK (0x04000000U)

// #define GNNE_REG_OFFSET     (0x00180000)
#define GNNE_ENABLE                         _IOWR('g', 1, unsigned long)
#define GNNE_RESET                          _IOWR('g', 2, unsigned long)
#define GNNE_DISABLE                        _IOWR('g', 3, unsigned long)
#define GNNE_SET_PC                         _IOWR('g', 4, unsigned long)
#define GNNE_SET_MEM_BASE                   _IOWR('g', 5, unsigned long)
#define GNNE_GET_STATUS                     _IOWR('g', 10, unsigned long)
#define GNNE_SET_PC_ENABLE                  _IOWR('g', 11, unsigned long)
#define GNNE_SET_MEM0                       _IOWR('g', 12, unsigned long)
#define GNNE_SET_MEM1                       _IOWR('g', 13, unsigned long)
#define GNNE_SET_MEM2                       _IOWR('g', 14, unsigned long)
#define GNNE_SET_MEM3                       _IOWR('g', 15, unsigned long)

#define GNNE_GET_PC                         _IOWR('g', 16, unsigned long)
#define GNNE_GET_CTRL                       _IOWR('g', 17, unsigned long)
#define GNNE_GET_DSP_INTR_MASK              _IOWR('g', 18, unsigned long)
#define GNNE_GET_MEM0                       _IOWR('g', 19, unsigned long)
#define GNNE_GET_MEM1                       _IOWR('g', 20, unsigned long)
#define GNNE_GET_MEM2                       _IOWR('g', 21, unsigned long)
#define GNNE_GET_MEM3                       _IOWR('g', 22, unsigned long)
#define GNNE_GET_LOAD_STROE_PC_ADDR         _IOWR('g', 23, unsigned long)
#define GNNE_GET_TCU_MFU_PC_ADDR            _IOWR('g', 24, unsigned long)
#define GNNE_GET_CCR_STATUS0                _IOWR('g', 25, unsigned long)
#define GNNE_GET_CCR_STATUS1                _IOWR('g', 26, unsigned long)
#define GNNE_GET_CCR_STATUS2                _IOWR('g', 27, unsigned long)
#define GNNE_GET_CCR_STATUS3                _IOWR('g', 28, unsigned long)

typedef struct {
    uint64_t gnne_enable: 1;
    uint64_t wait_dsp_intr_clr: 1;
    uint64_t wait_cpu_intr_clr: 1;
    uint64_t ctrl_reserved0: 1;
    uint64_t gnne_cg_off: 1;
    uint64_t change_pc_addr: 1;
    uint64_t dsp_resume: 1;
    uint64_t ctrl_reserved1: 25;
    uint64_t gnne_enable_wmask: 1;
    uint64_t wait_dsp_intr_clr_wmask: 1;
    uint64_t wait_cpu_intr_clr_wmask: 1;
    uint64_t enable_range_wmask: 1;
    uint64_t gnne_cg_off_wmask: 1;
    uint64_t change_pc_addr_mask: 1;
    uint64_t dsp_resume_mask: 1;
    uint64_t ctrl_reserved2: 25;
} gnne_ctrl;

typedef struct {
    uint64_t component_status: 8;
    uint64_t version_id: 6;
    uint64_t status: 2;
    uint64_t biu_status: 6;
    uint64_t exception: 2;
    uint64_t reset_status : 2;
    uint64_t axi_bresp_err : 1;
    uint64_t status_reserved2: 5;
    uint64_t intr_num: 32;
} gnne_status_t;

typedef struct {
    uint64_t ptrmask: 32;
    uint64_t regmask: 32;
} gnne_dsp_intr_mask;

typedef struct _gnne_reg {
    volatile uint64_t gpr[32];                      /* 0x000 - 0x0f8 */
    volatile uint64_t pc;                           /* 0x100 */
    volatile uint64_t host_mem_base_addr[4];        /* 0x108 - 0x120 */
    // ctrl reg
    volatile gnne_ctrl ctrl;                        /* 0x128 */
    // status reg
    volatile gnne_status_t status;                  /* 0x130 */
    volatile gnne_dsp_intr_mask dsp_intr_mask;      /* 0x138 */
    volatile uint64_t load_stroe_pc_addr;      /* 0x140 */
    volatile uint64_t tcu_mfu_pc_addr;         /* 0x148 */
    volatile uint64_t ccr_status0;             /* 0x150 */
    volatile uint64_t ccr_status1;             /* 0x158 */
    volatile uint64_t ccr_status2;             /* 0x160 */
    volatile uint64_t ccr_status3;             /* 0x158 */
} gnne_reg_t;

typedef enum _gnne_status
{
    GNNE_STATUS_IDLE,
    GNNE_STATUS_RUNNING,
    GNNE_STATUS_PENDING,
    GNNE_STATUS_ERROR
} gnne_status_e;

typedef enum _gnne_reset_status
{
    GNNE_RESET_STATUS_IDLE,
    GNNE_RESET_STATUS_BIU,
    GNNE_RESET_STATUS_ACT,
    GNNE_RESET_STATUS_HLD
} gnne_reset_status_t;

typedef enum {
    GNNE_CTRL_ENABLE_SET = (0x0001UL) << 32 | (0x0001),
    GNNE_CTRL_ENABLE_CLEAR = (0x0001UL) << 32 | (0x0000),
    GNNE_CTRL_DSP_RESUME_SET = (0x0040UL) << 32 | (0x0040),
    GNNE_CTRL_DSP_RESUME_CLEAR  = (0x0040UL) << 32,
    GNNE_CTRL_CHANGE_PC_ADDR_SET = (0x0020UL) << 32 | (0x0020),
    GNNE_CTRL_CHANGE_PC_ADDR_CLEAR = (0x0020UL) << 32,
    GNNE_CTRL_CG_OFF_SET = (0x0010UL) << 32 | (0x0010),
    GNNE_CTRL_CG_OFF_CLEAR = (0x0010UL) << 32,
    GNNE_CTRL_WAIT_CPU_INTR_CLR_SET = (0x0004UL) << 32 | (0x0004),
    GNNE_CTRL_WAIT_CPU_INTR_CLR_CLEAR = (0x0004UL) << 32,
    GNNE_CTRL_WAIT_DSP_INTR_CLR_SET = (0x0002UL) << 32 | (0x0002),
    GNNE_CTRL_WAIT_DSP_INTR_CLR_CLEAR = (0x0002UL) << 32
} gnne_ctrl_function_t;

typedef struct
{
    uint64_t        data;
    uint64_t        input;
    uint64_t        output;
    uint64_t        private;
} gnne_membase_t;


struct gnne_plat {
    struct resource *res;
    void __iomem    *regs;
    gnne_reg_t      *gnne_reg;
    int             major;
    int             minor;
    int             irq;
    struct class    *class;
    struct device   *device;
    struct cdev     cdev;
};

static int debug = 0;
static struct gnne_plat *plat;
static uint64_t gnne_membase[4];
static unsigned int gnne_int_flag;
static int gnne_fasync_flag = 0;
DECLARE_WAIT_QUEUE_HEAD(gnne_waitq);//注册一个等待队列gnne_waitq
struct fasync_struct *gnne_fasync;

static DEFINE_MUTEX(gnne_mutex);

static void gnne_ctrl_set(gnne_ctrl_function_t func)
{
    iowrite64(func, &(plat->gnne_reg->ctrl));
}

static gnne_status_t gnne_get_status(void)
{
    return plat->gnne_reg->status;
}

static void gnne_disable(void)
{
    gnne_ctrl_set(GNNE_CTRL_ENABLE_CLEAR);
    // while(gnne_get_status().reset_status != GNNE_RESET_STATUS_IDLE);
}

static int gnne_enable(void)
{
    if(gnne_get_status().status != GNNE_STATUS_IDLE) {
        return -EINVAL;
    }
    gnne_ctrl_set(GNNE_CTRL_ENABLE_SET);
    return 0;
}

static int gnne_set_pc(uint64_t pc)
{
    if(gnne_get_status().status != GNNE_STATUS_IDLE) {
        return -EINVAL;
    }
    plat->gnne_reg->pc = pc;
    return 0;
}

static int gnne_set_pc_enable(uint64_t pc)
{
    if(gnne_set_pc(pc)) {
        return -EINVAL;
    }
    if(gnne_enable()) {
        return -EINVAL;
    }
    return 0;
}

static void gnne_set_host_mem_base_addr(int idx, uint64_t addr)
{
    plat->gnne_reg->host_mem_base_addr[idx] = addr;
}

static void gnne_interrupt_clear(void)
{
    gnne_ctrl_set(GNNE_CTRL_WAIT_CPU_INTR_CLR_SET);
    // plat->gnne_reg->channel[0].eoi = plat->gnne_reg->channel[0].eoi;
}

static irqreturn_t gnne_irq(int irq, void *dev_id)
{
    gnne_status_t gnne_last_status = gnne_get_status();
    gnne_log("status reg            0x%llX\n", ioread64(&gnne_last_status));
    gnne_log("ctrl                  0x%llX\n", ioread64(&plat->gnne_reg->ctrl));
    gnne_log("load_stroe_pc_addr    0x%llX\n", ioread64(&plat->gnne_reg->load_stroe_pc_addr));
    gnne_log("tcu_mfu_pc_addr       0x%llX\n", ioread64(&plat->gnne_reg->tcu_mfu_pc_addr));
    gnne_log("ccr_status0           0x%llX\n", ioread64(&plat->gnne_reg->ccr_status0));
    gnne_log("ccr_status1           0x%llX\n", ioread64(&plat->gnne_reg->ccr_status1));
    gnne_log("ccr_status2           0x%llX\n", ioread64(&plat->gnne_reg->ccr_status2));
    gnne_log("ccr_status3           0x%llX\n", ioread64(&plat->gnne_reg->ccr_status3));

    gnne_log("status                %d\n", gnne_last_status.status);
    gnne_log("exception             %d\n", gnne_last_status.exception);

    gnne_interrupt_clear();
    gnne_log("clean interrupt ctrl  0x%llX\n", ioread64(&plat->gnne_reg->ctrl));
    gnne_log("Gnne interrupt\n");
    gnne_int_flag = 1;
    wake_up_interruptible(&gnne_waitq);   /* 唤醒休眠的进程，即调用read函数的进程 */

    if (gnne_fasync_flag)
        kill_fasync(&gnne_fasync, SIGIO, POLL_IN); // 发送信号
    return IRQ_HANDLED;
}

static int gnne_drv_fasync(int fd, struct file *file, int on)
{
    int err;
    gnne_log("fansync_helper %08X\n", on);
    err = fasync_helper(fd, file, on, &gnne_fasync);
    if(err < 0) {
        return err;
    }
    gnne_fasync_flag = on;

    return 0;
}

static unsigned int gnne_poll(struct file *file, poll_table *wait)
{
    unsigned int ret = 0;
    poll_wait(file, &gnne_waitq, wait);//将当前进程放到gnne_waitq列表
    if(gnne_int_flag) {
        ret |= POLLIN;//说明有数据被取到了
        gnne_int_flag = 0;
    }

    return ret;
}

static long gnne_ioctl(struct file *filp, unsigned int cmd,
                             unsigned long arg)
{
    // gnne_membase_t membase;
    gnne_status_t gnne_status;
    switch (cmd) {
        case GNNE_ENABLE :
            return gnne_enable();
            break;
        case GNNE_RESET :
            break;
        case GNNE_DISABLE :
            gnne_log("GNNE_DISABLE");
            gnne_disable();
            return 0;
            break;
        case GNNE_SET_PC :
            gnne_log("GNNE_SET_PC 0x%lX\n", arg);
            return gnne_set_pc(arg);
            break;
        case GNNE_SET_PC_ENABLE :
            gnne_log("GNNE_SET_PC_ENABLE 0x%lX\n", arg);
            return gnne_set_pc_enable(arg);
            break;
        case GNNE_SET_MEM_BASE :
            gnne_log("GNNE_SET_MEM_BASE\n");
            if(copy_from_user((void *)gnne_membase, (void *) arg, sizeof(gnne_membase)))
                return -EFAULT;
            gnne_set_host_mem_base_addr(0, gnne_membase[0]);
            gnne_set_host_mem_base_addr(1, gnne_membase[1]);
            gnne_set_host_mem_base_addr(2, gnne_membase[2]);
            gnne_set_host_mem_base_addr(3, gnne_membase[3]);
            return 0;
            break;
        case GNNE_SET_MEM0 :
            gnne_membase[0] = arg;
            gnne_set_host_mem_base_addr(0, gnne_membase[0]);
            break;
        case GNNE_SET_MEM1 :
            gnne_membase[1] = arg;
            gnne_set_host_mem_base_addr(1, gnne_membase[1]);
            break;
        case GNNE_SET_MEM2 :
            gnne_membase[2] = arg;
            gnne_set_host_mem_base_addr(2, gnne_membase[2]);
            break;
        case GNNE_SET_MEM3 :
            gnne_membase[3] = arg;
            gnne_set_host_mem_base_addr(3, gnne_membase[3]);
            break;
        case GNNE_GET_STATUS :
            gnne_status = gnne_get_status();
            if (copy_to_user((void *)arg, &gnne_status, 8)) {
                return -EFAULT;
            }
            break;
        default:
            gnne_log("gnne_ioctl: Unknown ioctl: 0x%.8X\n", cmd);
            return -EINVAL;
    }
    return 0;
}

static long gnne_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret;

    mutex_lock(&gnne_mutex);
    ret = gnne_ioctl(file, cmd, arg);
    mutex_unlock(&gnne_mutex);

    return ret;
}

// static int setup_chrdev_region(void)
// {
//     dev_t dev = 0;
//     int err;

//     if (plat->major == 0) {
//         err = alloc_chrdev_region(&dev, 0, 1, "k510-gnne");
//         plat->major = MAJOR(dev);

//         if (err) {
//                 gnne_log("k510-gnne: can't get major %d\n", plat->major);
//                 return err;
//         }
//     }
//     return 0;
// }

// static int create_module_class(void)
// {
//     plat->class = class_create(THIS_MODULE, "k510_gnne_class");

//     if (IS_ERR(plat->class))
//             return PTR_ERR(plat->class);

//     return 0;
// }

// static void clean_up_gnne_cdev(void)
// {
//     cdev_del(&plat->cdev);
// }

static int gnne_power_on(void)
{
    void __iomem *sysctl_reg;

    sysctl_reg = ioremap(GNNE_ACLK_CFG, 4);
    if (!sysctl_reg) {
        gnne_log("can't remap gnne sysctl 0x%08X\n", GNNE_ACLK_CFG);
        return -1;
    }
    iowrite32(GNNE_ENABLE_ACLK, sysctl_reg);
    iounmap(sysctl_reg);

    sysctl_reg = ioremap(GNNE_SYSCLK_CFG, 4);
    if (!sysctl_reg) {
        gnne_log("can't remap gnne sysctl 0x%08X\n", GNNE_SYSCLK_CFG);
        return -1;
    }
    iowrite32(GNNE_ENABLE_SYSCLK, sysctl_reg);
    iounmap(sysctl_reg);

    return 0;
}

static int gnne_power_off(void)
{
    void __iomem *sysctl_reg;

    sysctl_reg = ioremap(GNNE_ACLK_CFG, 4);
    if (!sysctl_reg) {
        gnne_log("can't remap gnne sysctl 0x%08X\n", GNNE_ACLK_CFG);
        return -1;
    }
    iowrite32(GNNE_DISABLE_ACLK, sysctl_reg);
    iounmap(sysctl_reg);

    sysctl_reg = ioremap(GNNE_SYSCLK_CFG, 4);
    if (!sysctl_reg) {
        gnne_log("can't remap gnne sysctl 0x%08X\n", GNNE_SYSCLK_CFG);
        return -1;
    }
    iowrite32(GNNE_DISABLE_SYSCLK, sysctl_reg);
    iounmap(sysctl_reg);

    return 0;
}

static int gnne_open(struct inode *inode, struct file *filp)
{
    gnne_int_flag = 0;
    gnne_power_on();
    gnne_disable();
    gnne_log("GNNE: open gnne success\n");
    return 0;
}

static int gnne_release(struct inode *inode, struct file *filp)
{
    gnne_int_flag = 0;
    gnne_disable();
    gnne_power_off();

    return 0;
}

const struct file_operations gnne_fops = {
    .owner          = THIS_MODULE,
    .open           = gnne_open,
    .release        = gnne_release,
    .unlocked_ioctl = gnne_unlocked_ioctl,
    .poll           = gnne_poll,
    .fasync         = gnne_drv_fasync,
};

// int setup_gnne_cdev(const char *device_name)
// {
//     struct device *device;
//     int err, devno =
//             MKDEV(plat->major, plat->minor);

//     cdev_init(&plat->cdev, &gnne_fops);
//     plat->cdev.owner = THIS_MODULE;
//     err = cdev_add(&plat->cdev, devno, 1);
//     if (err) {
//             gnne_log("gnne: Error %d adding gnne device number %d \n", err, plat->minor);
//             return err;
//     }

//     if (device_name != NULL) {
//             device = device_create(plat->class, NULL, devno, NULL,
//                                     device_name);
//             if (IS_ERR(device)) {
//                     gnne_log("gnne: device not created\n");
//                     clean_up_gnne_cdev();
//                     return PTR_ERR(device);
//             }
//     }

//     return 0;
// }

static ssize_t gnne_pc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->pc));
}

static ssize_t gnne_pc_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    int ret;
    uint64_t reg_val;
    ret = kstrtoull(buf, 0, &reg_val);
    if (ret) {
        gnne_log("%s: kstrtoll err return %d\n", __func__, ret);
        return -1;
    }
    iowrite64(reg_val, &plat->gnne_reg->pc);

    return count;
}

static ssize_t gnne_base_mem0_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->host_mem_base_addr[0]));
}

static ssize_t gnne_base_mem0_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    int ret;
    uint64_t reg_val;
    ret = kstrtoull(buf, 0, &reg_val);
    if (ret) {
        gnne_log("%s: kstrtoll err return %d\n", __func__, ret);
        return -1;
    }
    iowrite64(reg_val, &plat->gnne_reg->host_mem_base_addr[0]);

    return count;
}

static ssize_t gnne_base_mem1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->host_mem_base_addr[1]));
}

static ssize_t gnne_base_mem1_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    int ret;
    uint64_t reg_val;
    ret = kstrtoull(buf, 0, &reg_val);
    if (ret) {
        gnne_log("%s: kstrtoll err return %d\n", __func__, ret);
        return -1;
    }
    iowrite64(reg_val, &plat->gnne_reg->host_mem_base_addr[1]);

    return count;
}

static ssize_t gnne_base_mem2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->host_mem_base_addr[2]));
}

static ssize_t gnne_base_mem2_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    int ret;
    uint64_t reg_val;
    ret = kstrtoull(buf, 0, &reg_val);
    if (ret) {
        gnne_log("%s: kstrtoll err return %d\n", __func__, ret);
        return -1;
    }
    iowrite64(reg_val, &plat->gnne_reg->host_mem_base_addr[2]);

    return count;
}

static ssize_t gnne_base_mem3_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->host_mem_base_addr[3]));
}

static ssize_t gnne_base_mem3_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    int ret;
    uint64_t reg_val;
    ret = kstrtoull(buf, 3, &reg_val);
    if (ret) {
        gnne_log("%s: kstrtoll err return %d\n", __func__, ret);
        return -1;
    }
    iowrite64(reg_val, &plat->gnne_reg->host_mem_base_addr[3]);

    return count;
}

static ssize_t gnne_ctrl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->ctrl));
}

static ssize_t gnne_ctrl_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    int ret;
    uint64_t reg_val;
    ret = kstrtoull(buf, 0, &reg_val);
    if (ret) {
        gnne_log("%s: kstrtoll err return %d\n", __func__, ret);
        return -1;
    }
    iowrite64(reg_val, &plat->gnne_reg->ctrl);

    return count;
}

static ssize_t gnne_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->status));
}

static ssize_t gnne_status_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    return count;
}

static ssize_t gnne_dsp_intr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->dsp_intr_mask));
}

static ssize_t gnne_dsp_intr_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    return count;
}

static ssize_t gnne_load_stroe_pc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->load_stroe_pc_addr));
}

static ssize_t gnne_load_stroe_pc_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    return count;
}

static ssize_t gnne_tcu_mfu_pc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->tcu_mfu_pc_addr));
}

static ssize_t gnne_tcu_mfu_pc_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    return count;
}

static ssize_t gnne_ccr_status0_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->ccr_status0));
}

static ssize_t gnne_ccr_status0_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    return count;
}

static ssize_t gnne_ccr_status1_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->ccr_status1));
}

static ssize_t gnne_ccr_status1_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    return count;
}

static ssize_t gnne_ccr_status2_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->ccr_status2));
}

static ssize_t gnne_ccr_status2_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    return count;
}

static ssize_t gnne_ccr_status3_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "0x%016llX\n", ioread64(&plat->gnne_reg->ccr_status3));
}

static ssize_t gnne_ccr_status3_store(struct device *dev, struct device_attribute *attr, const  char *buf, size_t count)
{
    return count;
}

static DEVICE_ATTR(pc, 0664, gnne_pc_show, gnne_pc_store);
static DEVICE_ATTR(base_mem0, 0664, gnne_base_mem0_show, gnne_base_mem0_store);
static DEVICE_ATTR(base_mem1, 0664, gnne_base_mem1_show, gnne_base_mem1_store);
static DEVICE_ATTR(base_mem2, 0664, gnne_base_mem2_show, gnne_base_mem2_store);
static DEVICE_ATTR(base_mem3, 0664, gnne_base_mem3_show, gnne_base_mem3_store);
static DEVICE_ATTR(ctrl, 0664, gnne_ctrl_show, gnne_ctrl_store);
static DEVICE_ATTR(status, 0664, gnne_status_show, gnne_status_store);
static DEVICE_ATTR(dsp_intr, 0664, gnne_dsp_intr_show, gnne_dsp_intr_store);
static DEVICE_ATTR(load_stroe_pc, 0664, gnne_load_stroe_pc_show, gnne_load_stroe_pc_store);
static DEVICE_ATTR(tcu_mfu_pc, 0664, gnne_tcu_mfu_pc_show, gnne_tcu_mfu_pc_store);
static DEVICE_ATTR(ccr_status0, 0664, gnne_ccr_status0_show, gnne_ccr_status0_store);
static DEVICE_ATTR(ccr_status1, 0664, gnne_ccr_status1_show, gnne_ccr_status1_store);
static DEVICE_ATTR(ccr_status2, 0664, gnne_ccr_status2_show, gnne_ccr_status2_store);
static DEVICE_ATTR(ccr_status3, 0664, gnne_ccr_status3_show, gnne_ccr_status3_store);

static struct attribute *gnne_reg_als[] = {
	&dev_attr_pc.attr,
	&dev_attr_base_mem0.attr,
	&dev_attr_base_mem1.attr,
	&dev_attr_base_mem2.attr,
	&dev_attr_base_mem3.attr,
	&dev_attr_ctrl.attr,
	&dev_attr_status.attr,
	&dev_attr_dsp_intr.attr,
	&dev_attr_load_stroe_pc.attr,
	&dev_attr_tcu_mfu_pc.attr,
	&dev_attr_ccr_status0.attr,
	&dev_attr_ccr_status1.attr,
	&dev_attr_ccr_status2.attr,
	&dev_attr_ccr_status3.attr,
	NULL,
};

static struct attribute_group dev_attr_gnne_reg_group = {
	.name = "gnne_status",
	.attrs = gnne_reg_als,
};

static int gnne_probe(struct platform_device *pdev)
{
    struct resource *res;
    int err = 0;
    dev_t dev = 0;
    int devno;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        gnne_log("gnne_probe: get resource failed \n");
        err = -ENODEV;
        goto error;
    }

    plat = kzalloc(sizeof(struct gnne_plat), GFP_KERNEL);
    if (!plat) {
        gnne_log("gnne_probe: kzalloc failed \n");
        err = -ENOMEM;
        goto error;
    }

    plat->res = res;

    plat->irq = platform_get_irq(pdev, 0);
    gnne_log("Gnne irq number is %d\n", plat->irq);
    if(plat->irq < 0) {
        gnne_log("Gnne get irq err\n");
        err = -ENODEV;
        goto cleanup_kmem;
    }

    if((err = request_irq(plat->irq, gnne_irq, 0, "t0", NULL))) {
        gnne_log("Gnne request irq err %d\n", err);
        goto cleanup_kmem;
    }
    // if (!request_mem_region(res->start, resource_size(res), "k510-gnne")){
    //         gnne_log("gnne_probe: Could not get io memory region of sysctl\n");
    //         kfree(plat);
    //         return -ENOMEM;
    // }

    plat->regs = ioremap(res->start, resource_size(res));
    if(!plat->regs) {
        dev_dbg(&pdev->dev, "could not remap register memory\n");
        err = -ENOMEM;
        goto cleanup_irq;
    }
    plat->gnne_reg = (gnne_reg_t *)plat->regs;

    plat->major = 0;
    plat->minor = 0;

    err = alloc_chrdev_region(&dev, 0, 1, "k510-gnne");
    if (err) {
        gnne_log("k510-gnne: can't get major %d\n", plat->major);
        goto cleanup_ioremap;
    }
    plat->major = MAJOR(dev);

    // setup_chrdev_region();
    plat->class = class_create(THIS_MODULE, "k510_gnne_class");
    if (IS_ERR(plat->class)) {
        err = PTR_ERR(plat->class);
        goto cleanup_ioremap;
    }
    // create_module_class();
    devno = MKDEV(plat->major, plat->minor);

    cdev_init(&plat->cdev, &gnne_fops);
    plat->cdev.owner = THIS_MODULE;
    err = cdev_add(&plat->cdev, devno, 1);
    if (err) {
        gnne_log("Error %d adding gnne device number %d \n", err, plat->minor);
        goto cleanup_class;
    }

    plat->device = device_create(plat->class, NULL, devno, NULL, "k510-gnne");
    if (IS_ERR(plat->device)) {
        gnne_log("device not created\n");
        err = PTR_ERR(plat->device);
        goto cleanup_cdev;
    }
    // setup_gnne_cdev("k510-gnne");

    err = sysfs_create_group(&plat->device->kobj, &dev_attr_gnne_reg_group);
    if (err) {
        gnne_log("device group create file failed\n");
        goto cleanup_device;
    }

    gnne_log("k510 gnne driver loaded\n");

	return 0;

cleanup_device:
    device_destroy(plat->class, dev);
cleanup_cdev:
    cdev_del(&plat->cdev);
cleanup_class:
    class_destroy(plat->class);
cleanup_ioremap:
    iounmap(plat->regs);
cleanup_irq:
    free_irq(plat->irq, NULL);
cleanup_kmem:
    kfree(plat);
error:
	return err;
}

static int gnne_remove(struct platform_device *pdev)
{
    dev_t dev = MKDEV(plat->major, plat->minor);

    sysfs_remove_group(&plat->device->kobj, &dev_attr_gnne_reg_group);
    device_destroy(plat->class, dev);
    // clean_up_gnne_cdev();
    cdev_del(&plat->cdev);
    class_destroy(plat->class);
    iounmap(plat->regs);
    free_irq(plat->irq, NULL);
    kfree(plat);

    return 0;
}

static const struct of_device_id k510_gnne_ids[] = {
	{ .compatible = "k510-gnne" },
	{}
};

static struct platform_driver k510_gnne_driver = {
    .probe          = gnne_probe,
    .remove         = gnne_remove,
    .driver         = {
        .name           = "k510-gnne",
        .of_match_table = of_match_ptr(k510_gnne_ids),
    },
};

int gnne_module_init(void)
{
    int ret;
    ret = platform_driver_register(&k510_gnne_driver);
    return ret;
}

void gnne_module_deinit(void)
{
    platform_driver_unregister(&k510_gnne_driver);
}

module_init(gnne_module_init);
module_exit(gnne_module_deinit);
module_param(debug, int, S_IRUGO);
MODULE_LICENSE("GPL v2");
