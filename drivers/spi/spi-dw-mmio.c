/*
 * Memory-mapped interface driver for DW SPI Core
 *
 * Copyright (c) 2010, Octasic semiconductor.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/regmap.h>

#include "spi-dw.h"

#define DRIVER_NAME "dw_spi_mmio"

struct dw_spi_mmio {
	struct dw_spi  dws;
	struct clk     *clk;
};

//yangguang: add dwc_ssi support start
static int dw_spi_dw_apb_init(struct platform_device *pdev,
                             struct dw_spi_mmio *dwsmmio)
{
       /* Register hook to configure CTRLR0 */
       dwsmmio->dws.update_cr0 = dw_spi_update_cr0;

        return 0;
}

static int dw_spi_dwc_ssi_init(struct platform_device *pdev,
                              struct dw_spi_mmio *dwsmmio)
{
	/* Register hook to configure CTRLR0 */
	dwsmmio->dws.update_cr0 = dw_spi_update_cr0_v1_01a;
	dw_spi_dma_setup_generic(&dwsmmio->dws);
	return 0;
}


static int dw_spi_mmio_probe(struct platform_device *pdev)
{
        int (*init_func)(struct platform_device *pdev,
                         struct dw_spi_mmio *dwsmmio);

	struct dw_spi_mmio *dwsmmio;
	struct dw_spi *dws;
	struct resource *mem;
	int ret;
	int num_cs;
	struct clk *clk_parent = NULL;
	u32 rate;

	dwsmmio = devm_kzalloc(&pdev->dev, sizeof(struct dw_spi_mmio),
			GFP_KERNEL);
	if (!dwsmmio)
		return -ENOMEM;

	dws = &dwsmmio->dws;

	/* Get basic io resource and map it */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dws->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(dws->regs)) {
		dev_err(&pdev->dev, "SPI region map failed\n");
		return PTR_ERR(dws->regs);
	}

	dws->paddr = mem->start;
	
	dws->irq = platform_get_irq(pdev, 0);
	if (dws->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return dws->irq; /* -ENXIO */
	}

	/*setup parent and frequency of spi clock*/
	dwsmmio->clk = devm_clk_get(&pdev->dev, "spi_sclk");
	if (IS_ERR(dwsmmio->clk))
	{
		dwsmmio->clk = devm_clk_get(&pdev->dev, NULL);
		if (IS_ERR(dwsmmio->clk))
			return PTR_ERR(dwsmmio->clk);
	}
	else
	{
		clk_parent = devm_clk_get(&pdev->dev, "pll0");
		if (!IS_ERR(clk_parent))
		{
			ret = clk_set_parent(dwsmmio->clk, clk_parent);
			if (ret) {
				pr_err("set %s clock parent to pll0 failed\n", pdev->name);
			}

			ret = device_property_read_u32(&pdev->dev, "clock-frequency", &rate);
			if (ret >= 0)
			{
				ret = clk_set_rate(dwsmmio->clk, rate);
				if (ret) {
					pr_err("set %s clock rate to %u failed\n", pdev->name, rate);
				}
			}
		}
	}
	
	ret = clk_prepare_enable(dwsmmio->clk);
	if (ret)
		return ret;

	dws->bus_num = pdev->id;

	dws->max_freq = clk_get_rate(dwsmmio->clk);

	device_property_read_u32(&pdev->dev, "reg-io-width", &dws->reg_io_width);

	num_cs = 4;

	device_property_read_u32(&pdev->dev, "num-cs", &num_cs);

	dws->num_cs = num_cs;

	if (pdev->dev.of_node) {
		int i;

		for (i = 0; i < dws->num_cs; i++) {
			int cs_gpio = of_get_named_gpio(pdev->dev.of_node,
					"cs-gpios", i);

			if (cs_gpio == -EPROBE_DEFER) {
				ret = cs_gpio;
				goto out;
			}

			if (gpio_is_valid(cs_gpio)) {
				ret = devm_gpio_request(&pdev->dev, cs_gpio,
						dev_name(&pdev->dev));
				if (ret)
					goto out;
			}
		}
	}
        
        //yangguang: add dwc_ssi support
        init_func = device_get_match_data(&pdev->dev);
        if (init_func) {
                ret = init_func(pdev, dwsmmio);
                if (ret)
                        goto out;
        }
 

	ret = dw_spi_add_host(&pdev->dev, dws);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, dwsmmio);
	return 0;

out:
	clk_disable_unprepare(dwsmmio->clk);
	return ret;
}

static int dw_spi_mmio_remove(struct platform_device *pdev)
{
	struct dw_spi_mmio *dwsmmio = platform_get_drvdata(pdev);

	dw_spi_remove_host(&dwsmmio->dws);
	clk_disable_unprepare(dwsmmio->clk);

	return 0;
}


static const struct of_device_id dw_spi_mmio_of_match[] = {
	{ .compatible = "snps,dw-apb-ssi", .data = dw_spi_dw_apb_init},
    { .compatible = "snps,dwc-ssi-1.01a", .data = dw_spi_dwc_ssi_init},
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, dw_spi_mmio_of_match);

static struct platform_driver dw_spi_mmio_driver = {
	.probe		= dw_spi_mmio_probe,
	.remove		= dw_spi_mmio_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = dw_spi_mmio_of_match,
	},
};
module_platform_driver(dw_spi_mmio_driver);

MODULE_AUTHOR("Jean-Hugues Deschenes <jean-hugues.deschenes@octasic.com>");
MODULE_DESCRIPTION("Memory-mapped I/O interface driver for DW SPI Core");
MODULE_LICENSE("GPL v2");
