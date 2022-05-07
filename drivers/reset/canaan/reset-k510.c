/*
 * Copyright (c) 2016-2017 Linaro Ltd.
 * Copyright (c) 2022, Canaan Bright Sight Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <dt-bindings/reset/canaan-k510-reset.h>

#define K510_RESET_DEBUG

struct k510_reset_controller {
    spinlock_t                  lock;
    void __iomem                *membase;
    struct reset_controller_dev rst;
};

#define to_k510_reset_controller(_rst) \
    container_of(_rst, struct k510_reset_controller, rst)

static int k510_reset_of_xlate(struct reset_controller_dev *rcdev, const struct of_phandle_args *reset_spec)
{
    u32 offset;
    u32 type;
    u32 done;
    u32 reset;

    offset  = (reset_spec->args[0] << K510_RESET_REG_OFFSET_SHIFT) & K510_RESET_REG_OFFSET_MASK;
    type    = (reset_spec->args[1] << K510_RESET_TYPE_SHIFT)       & K510_RESET_TYPE_MASK;
    done    = (reset_spec->args[2] << K510_RESET_DONE_BIT_SHIFT)   & K510_RESET_DONE_BIT_MASK;
    reset   = (reset_spec->args[3] << K510_RESET_ASSERT_BIT_SHIFT) & K510_RESET_ASSERT_BIT_MASK;

    return (offset | type | done | reset);
}

static int k510_reset(struct reset_controller_dev *rcdev, unsigned long id) 
{
    struct k510_reset_controller *rstc = to_k510_reset_controller(rcdev);
    unsigned long uninitialized_var(flags);
    u32 offset  = (id & K510_RESET_REG_OFFSET_MASK) >> K510_RESET_REG_OFFSET_SHIFT;
    u32 type    = (id & K510_RESET_TYPE_MASK)       >> K510_RESET_TYPE_SHIFT;
    u32 done    = (id & K510_RESET_DONE_BIT_MASK)   >> K510_RESET_DONE_BIT_SHIFT;
    u32 reset   = (id & K510_RESET_ASSERT_BIT_MASK) >> K510_RESET_ASSERT_BIT_SHIFT;
    u32 reg;

#ifdef K510_RESET_DEBUG
    pr_info("[K510_RESET]:k510_reset id = 0x%08x, offset = 0x%08x type = %d done = %d reset =%d",(int)id,offset,type,done,reset);
#endif

    spin_lock_irqsave(&rstc->lock, flags);
    switch(type) {
        case K510_RESET_TYPE_DSP: {
            /* clear done bit */
            reg = readl(rstc->membase+offset);
            reg |= (1 << done);
            writel(reg, rstc->membase+offset);

            /* set reset bit */
            reg |= (1 << reset);
            writel(reg, rstc->membase+offset);

            udelay(10);

            /* clear reset bit */
            reg &= ~(1 << reset);
            writel(reg, rstc->membase+offset);

            /* wait done bit set */
            while(1) {
                reg = readl(rstc->membase+offset);
                if(reg & (1 << done)) {
                    /* clear done and break */
                    writel(reg, rstc->membase+offset);
                    break;
                }
            }
            break;
        }
        case K510_RESET_TYPE_HW_AUTO_DONE: {
            /* clear done bit */
            reg = readl(rstc->membase+offset);
            reg |= (1 << done);
            writel(reg, rstc->membase+offset);

            /* set reset bit */
            reg |= (1 << reset);
            writel(reg, rstc->membase+offset);

            /* wait done bit set */
            while(1) {
                reg = readl(rstc->membase+offset);
                if(reg & (1 << done)) {
                    /* clear done and break */
                    writel(reg, rstc->membase+offset);
                    break;
                }
            }
            break;
        }
        case K510_RESET_TYPE_SW_SET_DONE: {
            /* set reset bit */
            reg = readl(rstc->membase+offset);
            reg |= (1 << reset);
            writel(reg, rstc->membase+offset);

            udelay(10);

            /* clear reset bit */
            reg &= ~(1 << reset);
            writel(reg, rstc->membase+offset);

            break;
        }
        default:{
            break;
        }
    }

    spin_unlock_irqrestore(&rstc->lock, flags);
    return 0;
}

static int k510_reset_assert(struct reset_controller_dev *rcdev, unsigned long id)
{
    struct k510_reset_controller *rstc = to_k510_reset_controller(rcdev);
    unsigned long uninitialized_var(flags);
    u32 offset  = (id & K510_RESET_REG_OFFSET_MASK) >> K510_RESET_REG_OFFSET_SHIFT;
    u32 type    = (id & K510_RESET_TYPE_MASK)       >> K510_RESET_TYPE_SHIFT;
    /*u32 done    = (id & K510_RESET_DONE_BIT_MASK) >> K510_RESET_DONE_BIT_SHIFT;*/
    u32 reset   = (id & K510_RESET_ASSERT_BIT_MASK) >> K510_RESET_ASSERT_BIT_SHIFT;
    u32 reg;

#ifdef K510_RESET_DEBUG
    pr_info("[K510_RESET]:k510_reset_assert id = 0x%08x",(int)id);
#endif

    if(type == K510_RESET_TYPE_HW_AUTO_DONE) {
        pr_err("hardware auto done reset DOESNOT support reset assert!");
    } else {
        spin_lock_irqsave(&rstc->lock, flags);
        reg = readl(rstc->membase+offset);
        /* set reset bit */
        reg |= (1 << reset);
        writel(reg, rstc->membase+offset);
        spin_unlock_irqrestore(&rstc->lock, flags);
    }
    return 0;
}

static int k510_reset_deassert(struct reset_controller_dev *rcdev, unsigned long id)
{
    struct k510_reset_controller *rstc = to_k510_reset_controller(rcdev);
    unsigned long uninitialized_var(flags);
    u32 offset  = (id & K510_RESET_REG_OFFSET_MASK) >> K510_RESET_REG_OFFSET_SHIFT;
    u32 type    = (id & K510_RESET_TYPE_MASK)       >> K510_RESET_TYPE_SHIFT;
    u32 done    = (id & K510_RESET_DONE_BIT_MASK)   >> K510_RESET_DONE_BIT_SHIFT;
    u32 reset   = (id & K510_RESET_ASSERT_BIT_MASK) >> K510_RESET_ASSERT_BIT_SHIFT;
    u32 reg;

#ifdef K510_RESET_DEBUG
        pr_info("[K510_RESET]:k510_reset_deassert id = 0x%08x",(int)id);
#endif

    if(type == K510_RESET_TYPE_HW_AUTO_DONE) {
        pr_err("hardware auto done reset DOESNOT support reset assert!");
    } else {
        spin_lock_irqsave(&rstc->lock, flags);
        reg = readl(rstc->membase+offset);
        /* clear reset bit */
        reg &= ~(1 << reset);
        writel(reg, rstc->membase+offset);
        if(type == K510_RESET_TYPE_DSP) {
            /* check bit done */
            while(1) {
                reg = readl(rstc->membase+offset);
                if(reg & (1 << done)) {
                    /* clear done and break */
                    writel(reg, rstc->membase+offset);
                    break;
                }
            }
        }
        spin_unlock_irqrestore(&rstc->lock, flags);
    }
    return 0;
}

static const struct reset_control_ops k510_reset_ops = {
    .reset          = k510_reset,
    .assert         = k510_reset_assert,
    .deassert       = k510_reset_deassert,
};

static int k510_reset_probe(struct platform_device *pdev)
{
    struct k510_reset_controller *rstc;
    struct resource *res;

    rstc = devm_kmalloc(&pdev->dev, sizeof(*rstc), GFP_KERNEL);
    if (!rstc) {
        pr_err("k510_reset_init dev_kmalloc error!");
        return -1;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    rstc->membase = devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!rstc->membase) {
        pr_err("k510_reset_init devm_ioremap error!");
        return -1;
    }
    #ifdef K510_RESET_DEBUG
        pr_info("[K510_RESET]:sysctl reset phy addr 0x%08x",(int)res->start);
    #endif

    spin_lock_init(&rstc->lock);
    rstc->rst.owner = THIS_MODULE;
    rstc->rst.ops = &k510_reset_ops;
    rstc->rst.of_node = pdev->dev.of_node;
    rstc->rst.of_reset_n_cells = 4;
    rstc->rst.of_xlate = k510_reset_of_xlate;
    if(0 == reset_controller_register(&rstc->rst)) {
    #ifdef K510_RESET_DEBUG
        pr_info("[K510_RESET]:k510_reset_probe ok!");
    #endif
    } else {
        pr_info("[K510_RESET]:k510_reset_probe error!");
    }
    return 0;
}

void k510_reset_exit(struct k510_reset_controller *rstc)
{
    reset_controller_unregister(&rstc->rst);
}

static const struct of_device_id k510_reset_match[] = {
    { .compatible = "canaan,k510-sysctl-reset",},
    {},
};

MODULE_DEVICE_TABLE(of, k510_reset_match);

static struct platform_driver k510_reset_driver = {
    .probe = k510_reset_probe,
    .driver = {
            .name = "k510-sysctl-reset",
            .of_match_table = k510_reset_match,
    },
};

static int __init k510_reset_init(void)
{
    return platform_driver_register(&k510_reset_driver);
}
arch_initcall(k510_reset_init);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:k510-sysctl-reset");
MODULE_DESCRIPTION("Canaan K510 Reset Driver");

/* how to reset device in the device driver:
    1. write reset attribute in reset_consumer.dtsi
    2. use device_reset(&platform_device->device) api to reset device.
    for example: please see emac driver and dtsi
 */
