#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>

#define OTP_BYPASS_ADDR_P           0x97000040

#define OTP_REG_ENABLE              0x8000
#define OTP_REG_MODE                0x8004
#define OTP_REG_TEST_ROW            0x8008
#define OTP_REG_MODE_STA            0x800c
#define OTP_REG_PROGRAM_READY       0x8010
#define OTP_REG_PROGRAM_ERROR       0x8014
#define OTP_REG_PROGRAM_ERROR_ADDR  0x8018
#define OTP_REG_PROGRAM_ERROR_DATA  0x801c
#define OTP_REG_INITIAL_DONE        0x8020
#define OTP_REG_KEY_SEL             0x8024

#define OTP_DISABLE             0x0
#define OTP_ENABLE              0x1

#define OTP_MODE_STANDBY        0x0
#define OTP_MODE_STANDBY_DEP    0x2
#define OTP_MODE_READ           0x4
#define OTP_MODE_PROGRAM        0x8
/* for test mode */
#define OTP_MODE_CHECK_INIT     0x10     /* check init data */
#define OTP_MODE_CHECK_PROMGRAM 0x20     /* check program data */
#define OTP_MODE_INIT_BUSY      0x80     /* init busy */

#define OTP_PROGRAM_SUCCESS     0x1
#define OTP_PROGRAM_PROCESS     0x0

#define OTP_NEED_NOTHING_INIT   0x0
#define OTP_INIT_PROCESS        0x2
#define OTP_INIT_SUCCESS        0x3

typedef enum
{
    OTP_BYPASS_STATUS                   = 0,
    OTP_MAX_STATUS                      = 1,
}OTP_STATUS_E;

struct otp_priv {
    struct device *dev;
    void __iomem *base;
    struct nvmem_config *config;
};

static bool sysctl_boot_get_otp_bypass(void)
{
     void __iomem *OTP_BYPASS_ADDR_V;
     OTP_BYPASS_ADDR_V = ioremap(OTP_BYPASS_ADDR_P, 4);
     if(readl(OTP_BYPASS_ADDR_V) & 0x4)
        return true;
    else
        return false;
}

static bool otp_get_status(OTP_STATUS_E eStatus)
{
    if(OTP_BYPASS_STATUS == eStatus)
        return sysctl_boot_get_otp_bypass();
    else
        return false;
}

static int k510_otp_read(void *context, unsigned int offset,
    void *val, size_t bytes)
{
    //printk("[%s,%d], k510_otp_read, offset:%x, bytes:%x", __FUNCTION__, __LINE__, offset, bytes);

    struct otp_priv *priv = context;
    uint32_t *buf;
    buf = (uint32_t *)val;
    uint32_t wait_us;
    int i;

    if(true == otp_get_status(OTP_BYPASS_STATUS))
    {
        //printk("[%s,%d], otp bypass", __FUNCTION__, __LINE__);
        return -1;
    }

    /* otp enable */
    writel(OTP_ENABLE, priv->base + OTP_REG_ENABLE);
    writel(0, priv->base + OTP_REG_TEST_ROW);
    wait_us = 0;
    while(1)
    {
        if(OTP_NEED_NOTHING_INIT == readl(priv->base + OTP_REG_INITIAL_DONE) ||
           OTP_INIT_SUCCESS == readl(priv->base + OTP_REG_INITIAL_DONE))
        {
            break;
        }
        else
        {
            if(wait_us++ > 2000)
            {
                writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
            }
            else
            {
                udelay(1);
            }
        }
    }

    /* set otp read */
    writel(OTP_MODE_READ, priv->base + OTP_REG_MODE);
    wait_us = 0;
    while(1)
    {
        if(OTP_MODE_READ == readl(priv->base + OTP_REG_MODE_STA))
        {
            break;
        }
        else
        {
            if(wait_us++ > 2000)
            {
                wait_us = 0;
                writel(OTP_MODE_STANDBY_DEP, priv->base + OTP_REG_MODE);
                while(1)
                {
                    if((OTP_MODE_STANDBY_DEP == readl(priv->base + OTP_REG_MODE_STA)) ||
                       (wait_us++ > 2000))
                    {
                        break;
                    }
                    else
                    {
                        udelay(1);
                    }
                }
                writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
                return -1;
            }
            else
            {
                udelay(1);
            }
        }
    }

    for(i = 0; i < bytes/4; i++)
    {
        buf[i] = readl(priv->base + offset + i*4);
    }

    writel(OTP_MODE_STANDBY_DEP, priv->base + OTP_REG_MODE);
    wait_us = 0;
    while(1)
    {
        if(OTP_MODE_STANDBY_DEP == readl(priv->base + OTP_REG_MODE_STA))
        {
            break;
        }
        else
        {
            if(wait_us++ > 2000)
            {
                writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
                return -1;
            }
            else
            {
                udelay(1);
            }
        }
    }

    writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
    
    // for(i = 0; i < bytes; i++)
    // {
    //     if(i%16 == 0)
    //         printk("");
    //     printk(KERN_CONT "%4x:%x ", i, ((uint8_t *)val)[i]);
    // }
    // printk("val:%x", val);

    return 0;
}

#if 1
static int k510_otp_write(void *context, unsigned int offset,
				 void *val, size_t bytes)
{
    //printk("[%s,%d], k510_otp_read, offset:%x, bytes:%x", __FUNCTION__, __LINE__, offset, bytes);

    struct otp_priv *priv = context;
    uint32_t *buf;
    buf = (uint32_t *)val;
    uint32_t wait_us;
    int i;

    if(true == otp_get_status(OTP_BYPASS_STATUS))
    {
        //printk("[%s,%d], otp bypass", __FUNCTION__, __LINE__);
        return -1;
    }

    /* otp enable */
    writel(OTP_ENABLE, priv->base + OTP_REG_ENABLE);
    writel(0, priv->base + OTP_REG_TEST_ROW);
    wait_us = 0;
    while(1)
    {
        if(OTP_NEED_NOTHING_INIT == readl(priv->base + OTP_REG_INITIAL_DONE) ||
           OTP_INIT_SUCCESS == readl(priv->base + OTP_REG_INITIAL_DONE))
        {
            break;
        }
        else
        {
            if(wait_us++ > 2000)
            {
                return -1;
            }
            else
            {
                udelay(1);
            }
        }
    }

    /* set otp program */
    writel(OTP_MODE_PROGRAM, priv->base + OTP_REG_MODE);
    wait_us = 0;
    while(1)
    {
        if(OTP_MODE_PROGRAM == readl(priv->base + OTP_REG_MODE_STA))
        {
            break;
        }
        else
        {
            if(wait_us++ > 2000)
            {
                wait_us = 0;
                writel(OTP_MODE_STANDBY_DEP, priv->base + OTP_REG_MODE);
                while(1)
                {
                    if(OTP_MODE_STANDBY_DEP == readl(priv->base + OTP_REG_MODE_STA) ||
                       wait_us++ > 2000)
                    {
                        break;
                    }
                    else
                    {
                        udelay(1);
                    }
                }
                writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
                return -1;
            }
            else
            {
                udelay(1);
            }
        }
    }

    /* wait for program success */
    for(i = 0; i < bytes/4; i++)
    {
        /* wait for ready */
        wait_us = 0;
        while(1)
        {
            if(OTP_PROGRAM_SUCCESS == readl(priv->base + OTP_REG_PROGRAM_READY))
            {
                break;
            }
            else
            {
                if(wait_us++ > 2000)
                {
                    wait_us = 0;
                    writel(OTP_MODE_STANDBY_DEP, priv->base + OTP_REG_MODE);
                    while(1)
                    {
                        if(OTP_MODE_STANDBY_DEP == readl(priv->base + OTP_REG_MODE_STA) ||
                           wait_us++ > 2000)
                        {
                            break;
                        }
                        else
                        {
                            udelay(1);
                        }
                    }
                    writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
                    return -1;
                }
                else
                {
                    udelay(1);
                }
            }
        }

        /* write data */
        #if 1
            writel(buf[i], priv->base + offset + i*4);
        #endif

        /* wait for program success */
        wait_us = 0;
        while(1)
        {
            if(OTP_PROGRAM_SUCCESS == readl(priv->base + OTP_REG_PROGRAM_READY))
            {
                break;
            }
            else
            {
                if(wait_us++ > 2000)
                {
                    wait_us = 0;
                    writel(OTP_MODE_STANDBY_DEP, priv->base + OTP_REG_MODE);
                    while(1)
                    {
                        if(OTP_MODE_STANDBY_DEP == readl(priv->base + OTP_REG_MODE_STA) ||
                           wait_us++ > 2000)
                        {
                            break;
                        }
                        else
                        {
                            udelay(1);
                        }
                    }
                    writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
                    return -1;
                }
                else
                {
                    udelay(1);
                }
            }
        }
    }

    writel(OTP_MODE_STANDBY_DEP, priv->base + OTP_REG_MODE);
    wait_us = 0;
    while(1)
    {
        if(OTP_MODE_STANDBY_DEP == readl(priv->base + OTP_REG_MODE_STA))
        {
            break;
        }
        else
        {
            if(wait_us++ > 2000)
            {
                writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
                return -1;
            }
            else
            {
                udelay(1);
            }
        }
    }

    writel(OTP_DISABLE, priv->base + OTP_REG_ENABLE);
    return 0;
}
#endif

static struct nvmem_config kendryte_otp_nvmem_config = {
    .name = "kendryte_otp",
    .read_only = false,
    .word_size = 4,
    .reg_read = k510_otp_read,
    .reg_write = k510_otp_write,
    .size = 0x8000,
};



static const struct of_device_id kendryte_otp_dt_ids[] = {
    { .compatible = "canaan,k510-otp" },
    {},
};
MODULE_DEVICE_TABLE(of, kendryte_otp_dt_ids);

static int kendryte_otp_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct resource *res;
    struct otp_priv *priv;
    struct nvmem_device *nvmem;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if(!priv)
        return -ENOMEM;
    
    priv->dev = dev;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    priv->base = devm_ioremap_resource(dev, res);
    if(IS_ERR(priv->base))
        return PTR_ERR(priv->base);

    kendryte_otp_nvmem_config.dev = dev;
    kendryte_otp_nvmem_config.priv = priv;
    priv->config = &kendryte_otp_nvmem_config;
    nvmem = devm_nvmem_register(dev, &kendryte_otp_nvmem_config);

    return PTR_ERR_OR_ZERO(nvmem);
}

static struct platform_driver kendryte_otp_driver = {
    .probe = kendryte_otp_probe,
    .driver = {
        .name = "kendryte_otp",
        .of_match_table = kendryte_otp_dt_ids,
    },
};

module_platform_driver(kendryte_otp_driver);

MODULE_DESCRIPTION("kendryte k510 otp driver");
MODULE_LICENSE("GPL v2");
