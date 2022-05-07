/**********************************************************************
 * Copyright (C) 2014-2015 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 ***********************************************************************
 * cdn_hub.c
 ***********************************************************************/

/**
 * Function build "status change" packet (one or two bytes)
 */
static int cdn_h_hub_status_data (struct usb_hcd *hcd, char *buf)
{
    struct wr_cusb *wr_cusb = hcd_to_wr_cusb(hcd);
    int retval =0;

    retval = wr_cusb->cd_h_obj->vhubStatusData(wr_cusb->cd_h_priv, buf);
    printk(KERN_ERR "cdn_h_hub_status_data value %02x\n", *buf);
    if(retval !=0) {
        printk(KERN_ERR "hub ERROR: %d  \n", retval);
        return 0;
    }

    if(*buf == 0x02) {
        printk(KERN_ERR "Hub: change detection: %d  \n", retval);
        return 1;
    } else
        return 0;
}

static int cdn_h_hub_control ( struct usb_hcd  *hcd,  u16 typeReq, u16 wValue,
    u16 wIndex, char *buf, u16 wLength)
{
    struct wr_cusb  *wr_cusb = hcd_to_wr_cusb(hcd);
    CH9_UsbSetup setup;
    int     retval = 0;
    unsigned long   flags;

    setup.bmRequestType = typeReq >>8;
    setup.bRequest = typeReq & 0xFF;
    setup.wIndex = wIndex;
    setup.wLength =wLength;
    setup.wValue = wValue;

    spin_lock_irqsave(&wr_cusb->lock, flags);
    if (unlikely(!HCD_HW_ACCESSIBLE(hcd))) {
        spin_unlock_irqrestore(&wr_cusb->lock, flags);
        return -ESHUTDOWN;
    }

    retval = wr_cusb->cd_h_obj->vhubControl(wr_cusb->cd_h_priv, &setup, buf);
    if(retval != 0) {
    }

    spin_unlock_irqrestore(&wr_cusb->lock, flags);
    return retval;
}

#ifdef  CONFIG_PM
static int cdn_h_bus_suspend (struct usb_hcd *hcd)
{
    return 0;
}


/* caller has locked the root hub, and should reset/reinit on error */
static int cdn_h_bus_resume (struct usb_hcd *hcd)
{
    return 0;
}
#else

#define cdn_h_bus_suspend   NULL
#define cdn_h_bus_resume        NULL

#endif  /* CONFIG_PM */
