/*************************************************************\********
 * Copyright (C) 2014-2015 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 ***********************************************************************
 * cdn_core_wrapper.c
 *
 * Cadence USB linux Driver
 * This code implements the USB wrapper for the Cadence USB Core Driver.
 * This driver connects existing in kernel USB subsystem with Cadence
 * Core Driver.
 *
 * Core Driver is independent firmware that can be running without OS.
 *
 * The Cadence Core Driver architecture relies on an Object model to
 * provide access to "api" routines that can be used to perform different
 * functions required by the driver. For USB, this interface is
 * termed CUSBD (""Cadence USB Device Interface") or
 * CUSBH ("Cadence USB Host Interface") .
 *
 * The CUSBD_OBJ struct is used to contain a list of apis for Device only
 * USB Controller . Additional details about this interface and associated
 * routines may be found in another places in the documentation
 * set that ships with the Core Driver.
 *
 * The CUSBH_OBJ struct is used to contain a list of apis for USB Embedded
 * Host controller and USB OTG Controller  . Additional
 * details about this interface and associated routines may be found at
 * other places in the documentation set that ships with the Core Driver.
 *
 * The CUSBD_OBJ and CUSBH_OBJ struct and associated programming model includes the
 * use of a "probe" and an "init" routine (in addition to others) which
 * may be used by the current driver to get information about the amount
 * of memory required by the Core driver, etc.
 *
 * Naming convention:
 * To avoid misleading names of variables and function in wrapper are used
 * the following prefix
 * prefix wr - used for variables and structures related to wrapper
 * prefix cd - used for variables and structure related to Core Driver
 * prefix ls - used for variables and structures related to Linux Subsystem
 ***********************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/hcd.h>


#include "cusbh_if.h"
#include "cusb_ch11.h"

#include "cusbd_if.h"

#ifndef CONFIG_CDN_CUSB_PCI
#define CONFIG_CDN_CUSB_PLATFORM_OF 1
#endif
#define UHC_ESHUTDOWN 4
#define UDC_ECONNRESET 5

#define USB_CDN_CUSB_LOADED  3

#define DRIVER_DESC "Cadence USB Host Controller Driver"
static const char cdn_driver_name[] = "cdn_cusb";

/**
 * Driver should remember pointer to struct cdn_cusb because in some Core Driver callback function
 * firmware lost context.
 */
struct wr_cusb *global_cusb;

#include "cdn_core_wrapper.h"
#include "cdn_debug.c"
#include "cdn_hub.c"
#include "cdn_host.c"

/*For debug purpose*/
// unsigned int g_dbg_log_lvl = 0;
// unsigned int g_dbg_log_cnt = 0;
// unsigned int g_dbg_state = 0;
// unsigned int g_dbg_enable_log =1;

/**
 * Module information
 */
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

MODULE_AUTHOR("Cadence FW");
MODULE_ALIAS("platform:cdn_usb_hcd");

void cdn_cusb_start(struct wr_cusb *cusb);

#define DRIVER_VERSION  "2014-05-13"

#define DRV_NAME "cdn_cusb"
#define DEV_NAME "cdncusb"
#include "cdn_udc.c"

static const CUSBH_Config cd_config = {
    .epIN =
    {
        {.bufferingValue = 1, .maxPacketSize=64,  .startBuf=0 }, // ep0in
        {.bufferingValue = 4, .maxPacketSize=1024, .startBuf=64  }, // ep1in
        {.bufferingValue = 4, .maxPacketSize=1024, .startBuf=4160 }, // ep2in
        {.bufferingValue = 4, .maxPacketSize=1024, .startBuf=8256}, // ep3in
        {.bufferingValue = 4, .maxPacketSize=1024, .startBuf=12352}, // ep4in
        {.bufferingValue = 0, .maxPacketSize=0, .startBuf=16448}, // ep5in
        {.bufferingValue = 0, .maxPacketSize=0, .startBuf=20544}, // ep6in
        {.bufferingValue = 0, .maxPacketSize=0, .startBuf=24640}, // ep7in
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=28736}, // ep8in
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=32832}, // ep9in
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=36928}, // ep10in
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=41024}, // ep11in
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=45120}, // ep12in
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=49216}, // ep13in
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=53312}, // ep14in
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=57408} // ep15in
    },
    .epOUT =
    {
        {.bufferingValue = 1, .maxPacketSize=64,  .startBuf=0}, //ep0out1
        {.bufferingValue = 4, .maxPacketSize=1024, .startBuf=64}, //ep1out
        {.bufferingValue = 4, .maxPacketSize=1024, .startBuf=4160}, //ep2out
        {.bufferingValue = 4, .maxPacketSize=1024, .startBuf=8256}, //ep3out
        {.bufferingValue = 4, .maxPacketSize=1024,   .startBuf=12352}, //ep4out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=16448}, //ep5out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=20544}, //ep6out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=24640}, //ep7out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=28736}, //ep8out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=32832}, //ep9out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=36928}, //ep10out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=41024}, //ep11out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=45120}, //ep12out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=49216}, //ep13out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=53312}, //ep14out
        {.bufferingValue = 0, .maxPacketSize=0,   .startBuf=57408}  //ep15out
    },
    .dmultEnabled = 1, // set to 1 if scatter/gather DMA available
    .memoryAlignment = 8,
    .dmaSupport = 1,
    .isOtg = 1,     /*Support host and device and OTG*/
    .isEmbeddedHost = 0,/*Support only Host*/
    .isDevice = 0   /*Support device without OTG*/
};

static irqreturn_t cdn_dma_irq(int irq, void * dev_id)
{
    struct wr_cusb *wr_cusb = dev_id;
    unsigned long   flags;

    spin_lock_irqsave(&wr_cusb->lock, flags);

    wr_cusb->cd_h_obj->isr(wr_cusb->cd_h_priv);
    spin_unlock_irqrestore(&wr_cusb->lock, flags);
    return IRQ_HANDLED;
}

/**
 * USB device controller interrupt handler
 */
static irqreturn_t cdn_usb_irq(int irq, void * dev_id)
{
    struct wr_cusb *wr_cusb = dev_id;
    unsigned long   flags;

    spin_lock_irqsave(&wr_cusb->lock, flags);

    wr_cusb->cd_h_obj->isr(wr_cusb->cd_h_priv);
    spin_unlock_irqrestore(&wr_cusb->lock, flags);
    return IRQ_HANDLED;
}

static int cdn_cusb_free_controller(struct device *dev)
{
    struct wr_cusb  *wr_cusb  = dev_get_drvdata(dev);
    cdn_g_cleanup(wr_cusb);
    return 0;
}

/**
 * Perform generic per-controller initialization.
 *
 * @dev: the controller
 * @usb_irq_nr: USB IRQ number
 * @dma_irq_nr: DMA IRQ number
 * @base: virtual address of controller registers,
 * @res: resources
 */
static int cdn_cusb_init_controller(struct device *dev, int usb_irq_nr, int dma_irq_nr,
            void __iomem * base, struct resource * res)
{
    struct wr_cusb      *wr_cusb;
    int ret =0;
#ifdef CONFIG_CDN_CUSB_PCI
    int i;
#endif

    wr_cusb = devm_kzalloc(dev, sizeof(*wr_cusb), GFP_KERNEL);
    if (!wr_cusb)
        return -ENOMEM;

    wr_cusb->mregs = base;
    wr_cusb->resource = res;
    wr_cusb->usb_irq = 0;
    wr_cusb->usb_irq = 0;
    spin_lock_init(&wr_cusb->lock);

#ifdef CONFIG_CDN_CUSB_PCI
    /* Only support 1 endpoint for PCI HW, for now */
    for (i=1;i<16;i++) {
        cd_config.epOUT[i].bufferingValue = 0;
        cd_config.epIN[i].bufferingValue = 0;
    }
#endif

    wr_cusb->cd_h_config = cd_config;
    wr_cusb->cd_h_config.regBase = (uintptr_t)base;

    /*Configuration for device should be the same as for host*/
    wr_cusb->cd_g_config.isDevice = wr_cusb->cd_h_config.isDevice;
    wr_cusb->cd_g_config.isEmbeddedHost = wr_cusb->cd_h_config.isEmbeddedHost;
    wr_cusb->cd_g_config.isOtg = wr_cusb->cd_h_config.isOtg;
    wr_cusb->cd_g_config.dmaSupport = wr_cusb->cd_h_config.dmaSupport;
    wr_cusb->cd_g_config.dmultEnabled = wr_cusb->cd_h_config.dmultEnabled;

    wr_cusb->controller = dev;
    if(!wr_cusb->cd_g_config.isDevice && !wr_cusb->cd_g_config.isEmbeddedHost && !wr_cusb->cd_g_config.isOtg) {
        wr_cusb->cd_g_config.isDevice = 1; /*Default setting*/
        wr_cusb->cd_h_config.isDevice = 1; /*Default setting*/
    }

    wr_cusb->cd_h_obj = CUSBH_GetInstance();
    if(wr_cusb->cd_h_config.isDevice || wr_cusb->cd_h_config.isOtg) {
        wr_cusb->cd_g_obj = CUSBD_GetInstance();
    }
    ret = wr_cusb->cd_h_obj->probe(&wr_cusb->cd_h_config, &wr_cusb->cd_h_sys_req);
    if (ret) {
        ret = -ENODEV;
        goto err_probe;
    }

    wr_cusb->cd_g_callback.connect = cdn_g_cd_callback_connect;
    wr_cusb->cd_g_callback.disconnect= cdn_g_cd_callback_disconnect;
    wr_cusb->cd_g_callback.resume = cdn_g_cd_callback_resume;
    wr_cusb->cd_g_callback.setup = cdn_g_cd_callback_setup;
    wr_cusb->cd_g_callback.suspend = cdn_g_cd_callback_suspend;
    wr_cusb->cd_g_callback.usbRequestMemAlloc = cdn_g_cd_callback_request_alloc;
    wr_cusb->cd_g_callback.usbRequestMemFree = cdn_g_cd_callback_request_free;

    wr_cusb->cd_h_priv = devm_kzalloc(dev, wr_cusb->cd_h_sys_req.privDataSize, GFP_KERNEL);

    if(!wr_cusb->cd_h_priv) {
        ret = -ENOMEM;
        goto err_probe;
    }

    wr_cusb->cd_h_config.trbAddr = dma_alloc_coherent(dev, wr_cusb->cd_h_sys_req.trbMemSize, (dma_addr_t*)&wr_cusb->cd_h_config.trbDmaAddr, GFP_KERNEL);
    if (!wr_cusb->cd_h_config.trbAddr) {
        ret = -ENOMEM;
        goto err_dma_coherent;
    }

    wr_cusb->cd_h_callback.givbackRequest = cdn_h_givback_request;
    wr_cusb->cd_h_callback.otgStateChange = cdn_h_otg_state_change;
    wr_cusb->cd_h_callback.portStatusChange =cdn_h_rh_port_status_change;
    wr_cusb->cd_h_callback.setEpToggle = cdn_h_set_ep_toglle;
    wr_cusb->cd_h_callback.getEpToggle = cdn_h_get_ep_toggle;
    wr_cusb->cd_h_callback.usbDevCallbacks = &wr_cusb->cd_g_callback;
    ret = wr_cusb->cd_h_obj->init(wr_cusb->cd_h_priv, &wr_cusb->cd_h_config, &wr_cusb->cd_h_callback);

    if(ret) {
        ret = -ENODEV;
        goto err_init;
    }

    /*For CUSBH this should be called after CUSBH Core driver initialization*/
    wr_cusb->cd_g_priv = wr_cusb->cd_h_obj->getDevicePD(wr_cusb->cd_h_priv);
    if(!wr_cusb->cd_g_config.isDevice)  {
        ret = cdn_h_host_alloc(wr_cusb);

        if (ret < 0) {
            ret = -ENODEV;
            goto err_host;
        }
    }

    dev_set_drvdata(dev, wr_cusb);

    /*registering usbIrq*/
    if(usb_irq_nr > 0) {
        ret = devm_request_irq(wr_cusb->controller, usb_irq_nr, cdn_usb_irq,
                IRQF_SHARED, DRV_NAME, wr_cusb);

        if (ret != 0) {
            dev_err(wr_cusb->controller, "cannot request irq %d err %d\n",
                    wr_cusb->usb_irq, ret);
            ret = -ENODEV;
            goto err_irq1;
        }
        wr_cusb->usb_irq = usb_irq_nr;
    } else
    	wr_cusb->usb_irq = 0;
    /*registering dmaIrq*/
    if(dma_irq_nr > 0) {
        ret = devm_request_irq(wr_cusb->controller, dma_irq_nr, cdn_dma_irq,
                IRQF_SHARED, DRV_NAME, wr_cusb);

        if (ret != 0) {
            dev_err(wr_cusb->controller, "cannot request irq %d err %d\n",
                    wr_cusb->usb_irq, ret);
            ret = -ENODEV;
            goto err_irq2;
        }

        wr_cusb->dma_irq = dma_irq_nr;
    } else {
    	wr_cusb->dma_irq = dma_irq_nr;
    }
    global_cusb = wr_cusb;

    if(wr_cusb->cd_h_config.isEmbeddedHost || wr_cusb->cd_h_config.isOtg ) {
        // usb_phy_generic_register();
        // wr_cusb->transceiver = usb_get_phy(USB_PHY_TYPE_USB2);
        // if (IS_ERR_OR_NULL(wr_cusb->transceiver))
        //     return -EPROBE_DEFER;
    }

    if(wr_cusb->cd_h_config.isEmbeddedHost) {
        ret = cdn_h_setup(wr_cusb, 0);
    } else if (wr_cusb->cd_h_config.isDevice) {
        ret = cdn_g_setup(wr_cusb);
    } else if (wr_cusb->cd_h_config.isOtg) {
        ret = cdn_h_setup(wr_cusb,  0);
        if (ret < 0)
            goto err_setup;
        ret = cdn_g_setup(wr_cusb);
        if (ret)
            cdn_h_cleanup(wr_cusb);
    } else {
        dev_err(wr_cusb->controller, "unsupported mode\n");
    }



    if(ret < 0) {
        goto err_setup;
    }
    return ret;

err_setup:
err_irq2:
err_irq1:
    /*free host */
    cdn_h_free(wr_cusb);

err_host:
    if(!wr_cusb->cd_g_config.isDevice)
        wr_cusb->cd_h_obj->destroy(wr_cusb->cd_h_priv);
err_init:
    dma_free_coherent(dev, wr_cusb->cd_h_sys_req.trbMemSize, wr_cusb->cd_h_config.trbAddr, wr_cusb->cd_h_config.trbDmaAddr);
err_dma_coherent:
err_probe:
    dev_set_drvdata(dev, NULL);
    global_cusb= NULL;
    return ret;
}

#ifdef CONFIG_CDN_CUSB_PCI
#include "cdn_pci.c"
#define PCI_DRIVER cdn_cusb_pci_driver
#endif

#ifdef CONFIG_CDN_CUSB_PLATFORM_OF
#include "cdn_of.c"
#define OF_PLATFORM_DRIVER cdn_of_driver
#endif

#ifdef CONFIG_CDN_CUSB_PLATFORM
#include "cdn_of.c"
#define PLATFORM_DRIVER cdn_cusb_hcd_driver
#endif

static int cdn_init (void)
{
    int retval = 0;

    if (usb_disabled())
        return -ENODEV;

    set_bit(USB_CDN_CUSB_LOADED, &usb_hcds_loaded);

#ifdef PLATFORM_DRIVER
    retval = platform_driver_register(&PLATFORM_DRIVER);
    if (retval < 0)
        goto clean0;
#endif

#ifdef PCI_DRIVER
    retval = pci_register_driver(&PCI_DRIVER);
    if (retval < 0)
        goto clean1;
#endif

#ifdef OF_PLATFORM_DRIVER
    retval = platform_driver_register(&OF_PLATFORM_DRIVER);
    if (retval < 0)
        goto clean2;
#endif
    return retval;
#ifdef OF_PLATFORM_DRIVER
clean2:
    platform_driver_unregister(&OF_PLATFORM_DRIVER);
#endif
#ifdef PCI_DRIVER
clean1:
    pci_unregister_driver(&PCI_DRIVER);
#endif
#ifdef PLATFORM_DRIVER
clean0:
    platform_driver_unregister(&PLATFORM_DRIVER);
#endif

    clear_bit(USB_CDN_CUSB_LOADED, &usb_hcds_loaded);
    return retval;
}

static void cdn_exit (void)
{
#ifdef OF_PLATFORM_DRIVER
    platform_driver_unregister(&OF_PLATFORM_DRIVER);
#endif
#ifdef PLATFORM_DRIVER
    platform_driver_unregister(&PLATFORM_DRIVER);
#endif
#ifdef PCI_DRIVER
    pci_unregister_driver(&PCI_DRIVER);
#endif
    clear_bit(USB_CDN_CUSB_LOADED, &usb_hcds_loaded);
}

/**
 * Module init and exit
 */
module_init(cdn_init);
module_exit(cdn_exit);
