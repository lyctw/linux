/* Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/pm_domain.h>
#include <linux/platform_device.h>

#include <dt-bindings/soc/canaan,k510_pm_domains.h>

#undef K510_PM_DEBUG

typedef enum {
    REG_PM_PWR_EN,
    REG_PM_PWR_STAT,
    REG_PM_REPAIR_EN,
    REG_PM_REPAIR_STAT,
    REG_PM_ARRAY_SIZE,
} K510_PM_REG_E;

struct k510_pm_domain {
    struct generic_pm_domain dm;
    u16 *reg_offset;
    u16 pwr_on_bit;
    u16 pwr_on_wen_bit;
    u16 pwr_off_bit;
    u16 pwr_off_wen_bit;
    bool repair_enable;
    u16 repair_bit;
    u16 repair_wen_bit;
    bool soft_control_enable;
};

#define PM_PWR_EN(pd) ((pd)->reg_offset[REG_PM_PWR_EN])
#define PM_PWR_STAT(pd) ((pd)->reg_offset[REG_PM_PWR_STAT])
#define PM_PWR_REPAIR_EN(pd) ((pd)->reg_offset[REG_PM_REPAIR_EN])
#define PM_PWR_REPAIR_STAT(pd) ((pd)->reg_offset[REG_PM_REPAIR_STAT])

static void __iomem *sysctl_power_base;

static int k510_power_on(struct generic_pm_domain *domain)
{
    struct k510_pm_domain *pd = (struct k510_pm_domain *)domain;
    unsigned long loop = 1000;
    u32 val;

#ifndef K510_PM_DEBUG
    if(false == pd->soft_control_enable) {
        pr_info("[K510_POWER]:skip power on %s\n", domain->name);
        return 0;
    }
    val = readl(sysctl_power_base + PM_PWR_EN(pd));
    val |= BIT (pd->pwr_on_bit);
    val |= BIT (pd->pwr_on_wen_bit);
    writel(val, sysctl_power_base + PM_PWR_EN(pd));

    if(true == pd->repair_enable) {
        val = readl(sysctl_power_base + PM_PWR_REPAIR_EN(pd));
        val |= BIT(pd->repair_bit);
        val |= BIT(pd->repair_wen_bit);
        writel(val, sysctl_power_base + PM_PWR_REPAIR_EN(pd));

        do {
            udelay(1);
            val = readl(sysctl_power_base + PM_PWR_REPAIR_STAT(pd)) & BIT(pd->repair_bit);
        } while (--loop && !val);

        if (!loop) {
            pr_err("[K510_POWER]:Error: %s %s repair fail\n", __func__, domain->name);
            return -EIO;
        }

        pr_debug("[K510_POWER]:poweron %s\n", domain->name);
    }

    loop = 1000;
    do {
        udelay(1);
        if(strcmp(pd->dm.name,"mctl_domain") == 0) {
            val = readl(sysctl_power_base + PM_PWR_STAT(pd)) & 0xf;  /*bit0-3   value 0--> work*/
            if(val == 0) {
                val = 1;    /*indicate power up success*/
            }
        } else {
            val = readl(sysctl_power_base + PM_PWR_STAT(pd)) & BIT(pd->pwr_on_bit);
        }
    } while (--loop && !val);

    if (!loop) {
        pr_err("[K510_POWER]:Error: %s %s power on fail\n", __func__, domain->name);
        return -EIO;
    }
#endif
    pr_info("[K510_POWER]:power on %s\n", domain->name);

    return 0;
}

static int k510_power_off(struct generic_pm_domain *domain)
{
    struct k510_pm_domain *pd = (struct k510_pm_domain *)domain;
    unsigned long loop = 1000;
    u32 val;

#ifndef K510_PM_DEBUG
    if(false == pd->soft_control_enable) {
        pr_info("[K510_POWER]:skip power off %s\n", domain->name);
        return 0;
    }

    val = readl(sysctl_power_base + PM_PWR_EN(pd));
    val |= BIT (pd->pwr_off_bit);
    val |= BIT (pd->pwr_off_wen_bit);
    writel(val, sysctl_power_base + PM_PWR_EN(pd));

    do {
        udelay(1);
        if(strcmp(pd->dm.name,"mctl_domain") == 0) {
            val = readl(sysctl_power_base + PM_PWR_STAT(pd)) & 0xf;  /*bit0-3   value 6--> power off */
            if(val == 6) {
                val = 1;    /*indicate power off success*/
            }
        } else {
            val = readl(sysctl_power_base + PM_PWR_STAT(pd)) & BIT(pd->pwr_off_bit);
        }
    } while (--loop && !val);

    if (!loop) {
        pr_err("[K510_POWER]:Error: %s %s power off fail\n", __func__, domain->name);
        return -EIO;
    }
#endif
    pr_info("[K510_POWER]:power off %s\n", domain->name);

    return 0;
}

int k510_pd_probe(struct platform_device *pdev,
    struct generic_pm_domain **k510_pm_domains,
    int domain_num)
{
    struct genpd_onecell_data *genpd_data;
    struct resource *res;
    int i;

    genpd_data = devm_kzalloc(&pdev->dev, sizeof(*genpd_data), GFP_KERNEL);
    if (!genpd_data)
        return -ENOMEM;

    genpd_data->domains = k510_pm_domains;
    genpd_data->num_domains = domain_num;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    sysctl_power_base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(sysctl_power_base))
        return PTR_ERR(sysctl_power_base);

    for (i = 0; i < domain_num; ++i) {
        k510_pm_domains[i]->power_on = k510_power_on;
        k510_pm_domains[i]->power_off = k510_power_off;

        pm_genpd_init(k510_pm_domains[i], NULL, false);
    }

    of_genpd_add_provider_onecell(pdev->dev.of_node, genpd_data);
    dev_info(&pdev->dev, "powerdomain init ok\n");
    return 0;
}

static u16 k510_offsets[K510_PM_DOMAIN_MAX][REG_PM_ARRAY_SIZE] = {
    {0xc,   0x14,   0x258,  0x25c},
    {0x2c,  0x34,   0x258,  0x25c},
    {0x4c,  0x54,   0x258,  0x25c},
    {0x6c,  0x74,   0x258,  0x25c},
    {0x8c,  0x94,   0x258,  0x25c},
    {0xac,  0xb4,   0x258,  0x25c},
    {0xcc,  0xd4,   0x258,  0x25c},
    {0xf0,  0xf8,   0x258,  0x25c},
    {0x10c, 0x114,  0x258,  0x25c},
    {0x12c, 0x134,  0x258,  0x25c},
    {0x16c, 0x174,  0x258,  0x25c},
    {0x18c, 0x194,  0x258,  0x25c},
    {0x1ac, 0x1b4,  0x258,  0x25c},
    {0x1cc, 0x1d4,  0x258,  0x25c},
};

static struct k510_pm_domain ax25m_domain = {
    .dm = {
        .name       = "ax25m_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = true,
    .repair_bit     = 0,
    .repair_wen_bit = 24,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_AX25M],
    .soft_control_enable = false, //alwalys on
};

static struct k510_pm_domain ax25p_domain = {
    .dm = {
        .name       = "ax25p_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = true,
    .repair_bit     = 1,
    .repair_wen_bit = 25,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_AX25P],
    .soft_control_enable  = true,
};

static struct k510_pm_domain ai_domain = {
    .dm = {
        .name       = "ai_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = false,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_AI],
    .soft_control_enable = false, // deleteld ai node
};

static struct k510_pm_domain gnne_domain = {
    .dm = {
        .name       = "gnne_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = true,
    .repair_bit     = 4,
    .repair_wen_bit = 28,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_GNNE],
    .soft_control_enable = true,
};

static struct k510_pm_domain sec_domain = {
    .dm = {
        .name       = "sec_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = false,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_SEC],
    .soft_control_enable = true,
};

static struct k510_pm_domain stor_domain = {
    .dm = {
        .name       = "stor_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = false,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_STOR],
    .soft_control_enable = true,
};

static struct k510_pm_domain peri_domain = {
    .dm = {
        .name       = "peri_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = false,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_PERI],
    .soft_control_enable = true,
};

static struct k510_pm_domain mctl_domain = {
    .dm = {
        .name       = "mctl_domain",
    },
    .pwr_on_bit     = 4,                /* power status is special */
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = false,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_MCTL],
    .soft_control_enable = false,
};

static struct k510_pm_domain sram0_domain = {
    .dm = {
        .name       = "sram0_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = true,
    .repair_bit     = 2,
    .repair_wen_bit = 26,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_SRAM0],
    .soft_control_enable = false,
};

static struct k510_pm_domain sram1_domain = {
    .dm = {
        .name       = "sram1_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = true,
    .repair_bit     = 3,
    .repair_wen_bit = 27,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_SRAM1],
    .soft_control_enable = false,
};

static struct k510_pm_domain disp_domain = {
    .dm = {
        .name       = "disp_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = false,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_DISP],
    .soft_control_enable = false,
};

static struct k510_pm_domain h264_domain = {
    .dm = {
        .name       = "h264_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = false,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_H264],
    .soft_control_enable = true,
};

static struct k510_pm_domain usb_domain = {
    .dm = {
        .name       = "usb_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = false,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_USB],
    .soft_control_enable = true,
};

static struct k510_pm_domain isp_domain = {
    .dm = {
        .name       = "isp_domain",
    },
    .pwr_on_bit     = 4,
    .pwr_on_wen_bit = 25,
    .pwr_off_bit    = 0,
    .pwr_off_wen_bit= 24,
    .repair_enable  = true,
    .repair_bit     = 5,
    .repair_wen_bit = 29,
    .reg_offset     = k510_offsets[K510_PM_DOMAIN_ISP],
    .soft_control_enable = true,
};


static struct generic_pm_domain *k510_pm_domains[] = {
    [K510_PM_DOMAIN_AX25M]  = &ax25m_domain.dm,
    [K510_PM_DOMAIN_AX25P]  = &ax25p_domain.dm,
    [K510_PM_DOMAIN_AI]     = &ai_domain.dm,
    [K510_PM_DOMAIN_GNNE]   = &gnne_domain.dm,
    [K510_PM_DOMAIN_SEC]    = &sec_domain.dm,
    [K510_PM_DOMAIN_STOR]   = &stor_domain.dm,
    [K510_PM_DOMAIN_PERI]   = &peri_domain.dm,
    [K510_PM_DOMAIN_MCTL]   = &mctl_domain.dm,
    [K510_PM_DOMAIN_SRAM0]  = &sram0_domain.dm,
    [K510_PM_DOMAIN_SRAM1]  = &sram1_domain.dm,
    [K510_PM_DOMAIN_DISP]   = &disp_domain.dm,
    [K510_PM_DOMAIN_H264]   = &h264_domain.dm,
    [K510_PM_DOMAIN_USB]    = &usb_domain.dm,
    [K510_PM_DOMAIN_ISP]    = &isp_domain.dm,
};

static int k510_power_domain_probe(struct platform_device *pdev)
{
    return k510_pd_probe(pdev, k510_pm_domains,ARRAY_SIZE(k510_pm_domains));
}

static const struct of_device_id k510_pm_domain_matches[] = {
    { .compatible = "canaan, k510-sysctl-power", },
    { },
};

static struct platform_driver k510_power_domain_driver = {
    .driver = {
        .name = "k510-powerdomain",
        .of_match_table = k510_pm_domain_matches,
    },
    .probe = k510_power_domain_probe,
};

static int __init k510_power_domain_init(void)
{
    return platform_driver_register(&k510_power_domain_driver);
}

subsys_initcall(k510_power_domain_init);
