/**********************************************************************
 * Copyright (C) 2014-2015 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 ***********************************************************************
 * cdn_of.c
 ***********************************************************************/
#define DEBUG
#include <linux/signal.h>

#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/consumer.h>

/**
 * Probe function for Cadence USB wrapper driver
 * @pdev: platform device object.
 */
static int cdn_of_probe(struct platform_device *pdev)
{
    struct device_node *dn = pdev->dev.of_node;
    struct resource memres;
    int usb_irq_nr, dma_irq_nr;
    int rv =0;
    void __iomem    *base;
    struct gpio_desc *otg_power_supply;

    dev_dbg(&pdev->dev, "initializing CADENCE-OF USB Controller\n");

    if (usb_disabled())
        return -ENODEV;

    /* Get the base address for this device */
    rv = of_address_to_resource(dn, 0, &memres);

    if (rv)
        return rv;

    /*Setting DMA mask for device*/
    pdev->dev.coherent_dma_mask  = DMA_BIT_MASK(32);
    pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
    base = devm_ioremap_resource(&pdev->dev, &memres);
    if (IS_ERR(base))
        return PTR_ERR(base);

    dev_dbg(&pdev->dev, "resource physical addr=0x%x length=%d\n", memres.start, resource_size(&memres));
    otg_power_supply = devm_gpiod_get(&pdev->dev, "otg_power_supply", GPIOD_OUT_HIGH);
    if(IS_ERR( otg_power_supply))
    {
        printk("get otg power supply gpio err \n");
    }
    gpiod_direction_output(otg_power_supply, 1);

    dma_irq_nr = platform_get_irq(pdev, 0); /*first irq*/
    usb_irq_nr = platform_get_irq(pdev, 1); /*first irq*/

    if(dma_irq_nr <= 0 && usb_irq_nr <= 0) {
        dev_err(&pdev->dev, "Getting DMA IRQ failed\n");
        goto probe_err;
    }

    if (dma_irq_nr <= 0) {
        dev_err(&pdev->dev, "Getting DMA IRQ failed\n");
        goto probe_err;
    }

    if (usb_irq_nr <= 0) {
        dev_err(&pdev->dev, "Getting USB IRQ failed\n");
        /*Only older version of Cadence USB Controller supports
         * two separate interrupts */

        /**/
        //goto probe_err;
    }


    rv = cdn_cusb_init_controller(&pdev->dev, usb_irq_nr, dma_irq_nr, base, &memres);
    if(rv != 0)
        goto probe_err;
    return 0;
probe_err:
    devm_iounmap(&pdev->dev, base);
    return rv;

}

static void cdn_of_shutdown(struct platform_device *pdev)
{
    struct device   *dev = &pdev->dev;

    cdn_cusb_free_controller(dev);
}

static int cdn_of_remove(struct platform_device *pdev)
{
    struct device   *dev = &pdev->dev;
    struct wr_cusb  *wr_cusb  = dev_get_drvdata(dev);

    cdn_of_shutdown(pdev);
    wr_cusb->cd_g_obj->destroy(wr_cusb->cd_g_priv);
    dma_free_coherent(dev, wr_cusb->cd_h_sys_req.trbMemSize, wr_cusb->cd_h_config.trbAddr, wr_cusb->cd_h_config.trbDmaAddr);
    dev_set_drvdata(dev, NULL);
    global_cusb= NULL;
    return 0;
}

/* DTS/OpenFirmware match table */
static struct of_device_id cdn_of_match[] = {
        { .compatible = "Cadence,usb-dev1.00", },
        {}
};

MODULE_DEVICE_TABLE(of, cdn_of_match);

static struct platform_driver cdn_of_driver = {
    .probe      = cdn_of_probe,
    .remove     = cdn_of_remove,
    .shutdown   = cdn_of_shutdown,
    .driver     = {
            .name       = DEV_NAME,
            .owner      = THIS_MODULE,
//          .pm     = &cusb_pm_ops,
            .of_match_table = cdn_of_match,
        },
};
