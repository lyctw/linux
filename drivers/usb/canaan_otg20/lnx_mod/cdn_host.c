/**********************************************************************
 * Copyright (C) 2014-2017 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 ***********************************************************************
 * cdn_host.c
 * USB Embedded Host driver - implementation of  host side
 *
 ***********************************************************************/


#include <linux/usb/usb_phy_generic.h>

#define MAX_SUPPORTED_DEVICES 1
/*table for devices make possible to connect Core Driver USB device with Linux Driver USB Device*/
struct wr_usb_device *cdn_h_wr_devices_table[MAX_SUPPORTED_DEVICES] = {NULL};

static inline void listInit(CUSBH_ListHead *list)
{
    list->next = list;
    list->prev = list;
}


/**
 * Function used to updating  Core Drivers endpoint information
 */
static void cdn_h_endpoint_update(struct wr_cusb *wr_cusb, struct wr_usb_device * wr_udev,  struct usb_host_endpoint *ld_ep)
{
    CUSBH_Ep * cd_ep = NULL;
    int epnum = usb_endpoint_num(&ld_ep->desc);
    int is_out = usb_endpoint_dir_out(&ld_ep->desc);


    if(is_out ) {
        if(wr_udev->cd_out_h_ep[epnum]==NULL) {
            cd_ep = kzalloc(sizeof(CUSBH_Ep) + wr_cusb->cd_h_obj->epGetPirvateDataSize(wr_cusb->cd_h_priv), GFP_ATOMIC);
            wr_udev->cd_out_h_ep[epnum] =cd_ep;
        } else {
            cd_ep = wr_udev->cd_out_h_ep[epnum];
        }
    }
    else {
        if(wr_udev->cd_in_h_ep[epnum]==NULL) {
            cd_ep = kzalloc(sizeof(CUSBH_Ep) + wr_cusb->cd_h_obj->epGetPirvateDataSize(wr_cusb->cd_h_priv), GFP_ATOMIC);
            wr_udev->cd_in_h_ep[epnum] = cd_ep;
        } else {
            cd_ep = wr_udev->cd_in_h_ep[epnum];
        }
    }

    cd_ep->epDesc = *((CH9_UsbEndpointDescriptor *)&ld_ep->desc);
    cd_ep->userExt = (void*)ld_ep;
    listInit(&cd_ep->reqList);
    cd_ep->hcPriv = &((u8*)cd_ep)[sizeof *cd_ep];
}

/*CUSBH Callback function*/
void cdn_h_rh_port_status_change(void * pD)
{
    struct wr_cusb* wr_cusb = global_cusb;
    uint32_t statusHub =0;
    uint16_t *status = (uint16_t*)&statusHub;
    CH9_UsbSetup setup;
    uint32_t retval =0;

    if(!global_cusb)
        return;

    cdn_h_hub_status_data(wr_cusb->hcd, (char*)status);

    if(status) {
        setup.bRequest = CH9_USB_REQ_GET_STATUS;
        setup.bmRequestType = CH9_USB_REQ_TYPE_CLASS | CH9_USB_REQ_RECIPIENT_OTHER | CH9_USB_DIR_DEVICE_TO_HOST;
        setup.wIndex = cpu_to_le16(1);  //port number
        setup.wLength = cpu_to_le16(4);
        setup.wValue =0;

        retval = wr_cusb->cd_h_obj->vhubControl(wr_cusb->cd_h_priv, &setup, (uint8_t*)&statusHub);
        if(retval) {
            return;
        }

        if(status[1] & CH11_USB_PSC_CONNECTION) {
            if(status[0] & CH11_USB_PS_CONNECTION) {
                if (wr_cusb->hcd->status_urb) {
                    //inform HCD about changes
                    usb_hcd_poll_rh_status(wr_cusb->hcd);
                } else {
                    usb_hcd_resume_root_hub(wr_cusb->hcd);
                }
            } else {
                usb_hcd_resume_root_hub(wr_cusb->hcd);
                usb_hcd_poll_rh_status(wr_cusb->hcd);
            }
        }
    } else {
        usb_hcd_resume_root_hub(wr_cusb->hcd);
    }
}

u8 cdn_h_get_ep_toggle(void *pD, CUSBH_Device *cd_udev, u8 ep_num, u8 is_in)
{
    struct wr_usb_device *wr_udev;
    u8 toggle=0;
    if(!global_cusb)
        return 0;

    wr_udev = (struct wr_usb_device*) cd_udev->userExt;
    toggle =usb_gettoggle(wr_udev->ld_udev, ep_num, !is_in);
    return toggle;
}

void cdn_h_set_ep_toglle(void *pD, CUSBH_Device *cd_udev, u8 ep_num, u8 is_in, u8 toggle)
{
    struct wr_usb_device *wr_udev;

    if(!global_cusb)
        return;

    wr_udev = (struct wr_usb_device*) cd_udev->userExt;
    usb_settoggle(wr_udev->ld_udev, ep_num, !is_in, toggle);

}

void cdn_h_givback_request(void *pD, CUSBH_Req *cd_req, uint32_t status)
{
    struct urb * ld_req;
    struct wr_cusb * wr_cusb;
    int ld_status=0;
    if(!global_cusb)
        return;

    wr_cusb = global_cusb;

    ld_req = cd_req->userExt;

    ld_req->actual_length = cd_req->actualLength;

    switch(status) {
        case CUSBH_ESTALL:
            ld_status = -EPIPE;
            break;
        case CUSBH_EUNHANDLED:
            ld_status = -EPROTO;
            break;
        case CUSBH_ESHUTDOWN:
            ld_status = -ESHUTDOWN;
            break;
        default:
            ld_status = ld_req->status;
    }

    ld_status = (ld_req->status == -EINPROGRESS) ? 0 : ld_req->status;

    dev_dbg(wr_cusb->controller,
            "complete %p %pF (%d), dev%d ep%d%s, %d/%d\n",
            ld_req, ld_req->complete, status,
            usb_pipedevice(ld_req->pipe),
            usb_pipeendpoint(ld_req->pipe),
            usb_pipein(ld_req->pipe) ? "in" : "out",
            ld_req->actual_length, ld_req->transfer_buffer_length
            );

    if(usb_endpoint_xfer_isoc(&ld_req->ep->desc)) {
        u16 i=0;
        /*updates iso descriptor frame */
        for(i=0; i < ld_req->number_of_packets; i++ ) {
            ld_req->iso_frame_desc[i].status =0;
            ld_req->iso_frame_desc[i].actual_length =  cd_req->isoFramesDesc[i].length;
        }
    }
    usb_hcd_unlink_urb_from_ep(wr_cusb->hcd, ld_req);
    spin_unlock(&wr_cusb->lock);
    usb_hcd_giveback_urb(wr_cusb->hcd, ld_req, ld_status);
    spin_lock(&wr_cusb->lock);
}

void cdn_h_otg_state_change(void * pD, CUSBH_OtgState otgState){
    if(!global_cusb)
        return;
}

void cdn_h_cleanup(struct wr_cusb *cusb)
{
    if (cusb->cd_h_config.isDevice)
        return;
    usb_remove_hcd(cusb->hcd);
    cusb->hcd = NULL;
}

int cdn_h_setup(struct wr_cusb * wr_cusb, int power_budget)
{
    int ret;
    struct usb_hcd *hcd = wr_cusb->hcd;


    wr_cusb->is_host = 1;
    // wr_cusb->transceiver->otg->default_a = 1;
    // // wr_cusb->transceiver->state = OTG_STATE_A_IDLE;

    // otg_set_host(wr_cusb->transceiver->otg, &hcd->self);
    hcd->self.otg_port = 1;
    // wr_cusb->transceiver->otg->host = &hcd->self;
    hcd->power_budget = 2 * (power_budget ? : 250);

    ret = usb_add_hcd(hcd, 0, 0);
    if (ret < 0) {
        return ret;
    }
    return 0;
}


static int cdn_h_run (struct usb_hcd *hcd)
{
    /**
     * wr_cusb->cd_h_obj->start() is called when the hub driver
     * turns on port power
     */
    hcd->state = HC_STATE_RUNNING;
    return 0;
}

static void cdn_h_stop (struct usb_hcd *hcd)
{
    struct wr_cusb *wr_cusb = hcd_to_wr_cusb(hcd);

    wr_cusb->cd_h_obj->stop(wr_cusb->cd_h_priv);
    hcd->state = HC_STATE_HALT;
}

/**
 * cdn_cusb_reset-
 * @hcd:    Pointer to the usb_hcd device to which the host controller bound
 *
 * called during probe() after chip reset completes.
 */
static int cdn_h_reset(struct usb_hcd *hcd)
{
    return 0;
}

/**
 * cdn_cusb_shutdown kick in for silicon on any bus (not just pci, etc).
 * This forcibly disables dma and IRQs, helping kexec and other cases
 * where the next system software may expect clean state.
 */
static void cdn_h_shutdown(struct usb_hcd *hcd)
{
}

/**
 * non-error returns are a promise to giveback() the urb later
 * we drop ownership so next owner (or urb unlink) can get it
 *
 * urb + dev is in hcd.self.controller.urb_list
 * we're queueing URB onto software lists
 *
 * hcd-specific init for hcpriv hasn't been done yet
 */
static int cdn_h_urb_enqueue (struct usb_hcd  *hcd, struct urb  *urb, gfp_t mem_flags)
{
    struct wr_cusb *wr_cusb = hcd_to_wr_cusb(hcd);
    int ret, i;
    struct wr_usb_device * wr_udev;
    unsigned long           flags;
    struct usb_host_endpoint *ld_ep = urb->ep;
    struct usb_endpoint_descriptor  *epd = &ld_ep->desc;
    CUSBH_Req * cd_req;
    u32 isoFrameSize;

    if(!wr_cusb->cd_h_obj->isHostMode(wr_cusb->cd_h_priv)) {
        return -ENODEV;
    }

    wr_udev = cdn_h_wr_devices_table[urb->dev->slot_id];
    if(!wr_udev)
        return -ENODEV;

    if(usb_endpoint_dir_in(epd)) {
        if(!wr_udev->cd_in_h_ep[usb_endpoint_num(epd)]) {
            cdn_h_endpoint_update(wr_cusb, wr_udev, ld_ep);
        }
    } else {
        if(!wr_udev->cd_out_h_ep[usb_endpoint_num(epd)]) {
            cdn_h_endpoint_update(wr_cusb, wr_udev, ld_ep);
        }
    }

    spin_lock_irqsave(&wr_cusb->lock, flags);
    ret = usb_hcd_link_urb_to_ep(hcd, urb);
    spin_unlock_irqrestore(&wr_cusb->lock, flags);

    isoFrameSize = urb->number_of_packets * sizeof(CUSBH_IsoFramsDesc);
    cd_req = (CUSBH_Req*)kzalloc((sizeof *cd_req) + isoFrameSize, mem_flags);
    if(!cd_req) {
        spin_lock_irqsave(&wr_cusb->lock, flags);
        usb_hcd_unlink_urb_from_ep(hcd, urb);
        spin_unlock_irqrestore(&wr_cusb->lock, flags);
        return -ENOMEM;
    }

    cd_req->isoFramesDesc = NULL;
    cd_req->isoFramesNumber = urb->number_of_packets;
    if(usb_endpoint_xfer_isoc(epd)) {
        /*start address allocated for frame buffers is outside cd_req object*/
        cd_req->isoFramesDesc = (struct CUSBH_IsoFramsDesc*)(&cd_req[1]);

        for(i=0; i < urb->number_of_packets; i++) {
            cd_req->isoFramesDesc[i].length = urb->iso_frame_desc[i].length;
            cd_req->isoFramesDesc[i].offset = urb->iso_frame_desc[i].offset;
        }
    }

    spin_lock_irqsave(&wr_cusb->lock, flags);
    listInit(&cd_req->list);
    cd_req->userExt = (void*) urb;

    cd_req->actualLength = urb->actual_length;
    cd_req->bufAddress = urb->transfer_buffer;
    cd_req->buffDma = urb->transfer_dma;
    cd_req->buffLength = urb->transfer_buffer_length;
    cd_req->epIsIn = usb_endpoint_dir_in(epd);
    cd_req->epNum = usb_endpoint_num(epd);
    cd_req->epType = usb_endpoint_type(epd);
    cd_req->faddress =  usb_pipedevice(urb->pipe);
    cd_req->interval = urb->interval;
    cd_req->reqUnlinked =0;
    cd_req->setup = (CH9_UsbSetup*)urb->setup_packet;
    cd_req->setupDma = urb->setup_dma;
    cd_req->status = EINPROGRESS;
    cd_req->usbDev = &wr_udev->cd_udev;
    cd_req->usbEp = cd_req->epIsIn ? wr_udev->cd_in_h_ep[cd_req->epNum]:
        wr_udev->cd_out_h_ep[cd_req->epNum];

    if(!cd_req->epNum) {
        /* Update maxpacket size for ep0*/
        wr_udev->cd_ep0_hep.epDesc.wMaxPacketSize = urb->dev->ep0.desc.wMaxPacketSize;
    }

    urb->hcpriv = cd_req;
    ld_ep->hcpriv = (void*)wr_udev;

    ret = wr_cusb->cd_h_obj->reqQueue(wr_cusb->cd_h_priv, cd_req);
    if(ret) {
        usb_hcd_unlink_urb_from_ep(hcd, urb);
        urb->hcpriv = NULL;
        spin_unlock_irqrestore(&wr_cusb->lock, flags);
        kfree(cd_req);
        return -ret;
    }

    spin_unlock_irqrestore(&wr_cusb->lock, flags);
    return 0;
}


/**
 * Remove URB from hardware lists.
 * Completions normally happen asynchronously
 */
static int cdn_h_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
{
    struct wr_cusb *wr_cusb = hcd_to_wr_cusb(hcd);
    int ret;
    struct wr_usb_device * wr_udev;
    unsigned long           flags;
    CUSBH_Req * cd_req;

    dev_dbg(wr_cusb->controller, "urb=%p, dev%d ep%d%s\n", urb,
            usb_pipedevice(urb->pipe),
            usb_pipeendpoint(urb->pipe),
            usb_pipein(urb->pipe) ? "in" : "out");

    wr_udev = cdn_h_wr_devices_table[urb->dev->slot_id];
    if(!wr_udev)
        return -ENODEV;

    spin_lock_irqsave(&wr_cusb->lock, flags);
    ret = usb_hcd_check_unlink_urb(hcd, urb, status);
    if (ret)
        goto done;

    cd_req = urb->hcpriv;
    /*Core driver request not assigned so we dont't have to dequeue this request*/
    if(!urb->hcpriv)
        goto done;

    ret = wr_cusb->cd_h_obj->reqDequeue(wr_cusb->cd_h_priv, cd_req, status);
    if(ret) {
    }

    kfree(cd_req);
    urb->hcpriv = NULL;

done:
    spin_unlock_irqrestore(&wr_cusb->lock, flags);
    return ret;
}

static void cdn_h_endpoint_disable(struct usb_hcd *hcd, struct usb_host_endpoint *ld_ep)
{
    struct wr_usb_device * wr_udev = (struct wr_usb_device*)ld_ep->hcpriv;
    uint8_t ep_num = usb_endpoint_num(&ld_ep->desc);

    if(!wr_udev) {
        return;
    }

    if(ld_ep->desc.bEndpointAddress == 0) {
    } else {
        if(usb_endpoint_dir_in(&ld_ep->desc)) {
            if(!wr_udev->cd_in_h_ep[ep_num]) {
                wr_udev->cd_in_h_ep[ep_num]->userExt = NULL;
                kfree(wr_udev->cd_in_h_ep[ep_num]);
                wr_udev->cd_in_h_ep[ep_num] = NULL;
            }
        } else {
            if(!wr_udev->cd_out_h_ep[ep_num]) {
                listInit(&wr_udev->cd_out_h_ep[ep_num]->reqList);
                wr_udev->cd_out_h_ep[ep_num]->userExt = NULL;
                kfree(wr_udev->cd_out_h_ep[ep_num]);
                wr_udev->cd_out_h_ep[ep_num] = NULL;
            }
        }

    }

}

static int cdn_h_get_frame (struct usb_hcd *hcd)
{
    return 0;
}

void cdn_h_free(struct wr_cusb *cusb)
{
    usb_put_hcd(cusb->hcd);
}


/**
 * Function allocate object for connected Device
 */
int cdn_h_alloc_dev(struct usb_hcd *hcd, struct usb_device *udev)
{
    struct wr_usb_device * wr_udev;
    unsigned long flags =0;  
    struct wr_cusb * wr_cusb;
    int i=0;
    int slot=-EINVAL;

    if(!hcd || !udev)
        return -EINVAL;

    wr_cusb = hcd_to_wr_cusb(hcd);

    spin_lock_irqsave(&wr_cusb->lock, flags);
    for(i=0; i< MAX_SUPPORTED_DEVICES; i++) {
        if(cdn_h_wr_devices_table[i] == NULL) {
            slot=i;
            break;
        }
    }
    spin_unlock_irqrestore(&wr_cusb->lock, flags);

    /*Wrapper can't connect Core Driver Device with Linux USB Device*/
    if(slot < 0)
        return -ENOMEM;

    wr_udev = kzalloc((sizeof *wr_udev)+
            wr_cusb->cd_h_obj->epGetPirvateDataSize(wr_cusb->cd_h_priv),
            GFP_KERNEL);

    if(!wr_udev) {
        return -ENOMEM;
    }

    wr_udev->cd_ep0_hep.hcPriv = &((u8*)wr_udev)[sizeof *wr_udev];
    wr_udev->cd_udev.userExt =  (void*)wr_udev;

    wr_udev->ld_udev = udev;
    wr_udev->ld_udev->slot_id = slot;
    cdn_h_wr_devices_table[slot] = wr_udev;

    wr_udev->cd_ep0_hep.epDesc.bLength = CH9_USB_DS_ENDPOINT;
    wr_udev->cd_ep0_hep.epDesc.bDescriptorType = CH9_USB_DT_ENDPOINT;
    listInit(&wr_udev->cd_ep0_hep.reqList);
    wr_udev->cd_in_h_ep[0] = &wr_udev->cd_ep0_hep;
    wr_udev->cd_out_h_ep[0] = &wr_udev->cd_ep0_hep;

    return 1;
}

void cdn_h_free_dev(struct usb_hcd *hcd, struct usb_device *udev)
{
    struct wr_usb_device * wr_udev;
    struct wr_cusb * wr_cusb;
    int i=0;

    if(!hcd || !udev)
        return;

    wr_cusb = hcd_to_wr_cusb(hcd);

    if(!cdn_h_wr_devices_table[udev->slot_id]) {
        return;
    }

    wr_udev = cdn_h_wr_devices_table[udev->slot_id];

    /*object assigned to first element of array has been created together with struct wr_usb_device*/
    wr_udev->cd_in_h_ep[0] = NULL;
    wr_udev->cd_out_h_ep[0] = NULL;

    for(i=1; i<16; i++) {
        if(wr_udev->cd_in_h_ep[i]) {
            cdn_h_endpoint_disable(hcd, (struct usb_host_endpoint *)wr_udev->cd_in_h_ep[i]->userExt);
        }
        if(wr_udev->cd_out_h_ep[i]) {

            cdn_h_endpoint_disable(hcd, (struct usb_host_endpoint *)wr_udev->cd_out_h_ep[i]->userExt);
        }
    }

    cdn_h_wr_devices_table[udev->slot_id] = NULL;
    wr_udev->ld_udev->slot_id=0;


    wr_udev->cd_udev.userExt = (void*)NULL;
    kfree(wr_udev);
    return;
}

int cdn_h_address_device(struct usb_hcd *hcd, struct usb_device *udev)
{
    return 0;
}

int cdn_h_update_device(struct usb_hcd *hcd, struct usb_device *udev)
{
    struct wr_cusb *wr_cusb = hcd_to_wr_cusb(hcd);
    struct wr_usb_device * wr_udev;

    if(!wr_cusb->cd_h_obj->isHostMode(wr_cusb->cd_h_priv))
        return -ENODEV;
    wr_udev = cdn_h_wr_devices_table[udev->slot_id];
    if(!wr_udev)
        return -ENODEV;

    /*In this place firmware can update some fields. */
    wr_udev->cd_udev.devnum = udev->devnum;
    return 0;
}

int cdn_h_reset_device(struct usb_hcd *hcd, struct usb_device *udev)
{
    struct wr_usb_device * wr_udev;

    if(!hcd || !udev) {
        return -EINVAL;
    }

    wr_udev = cdn_h_wr_devices_table[udev->slot_id];

    if(!wr_udev) {
        return -ENODEV;
    }

    wr_udev->cd_udev.speed = wr_udev->ld_udev->speed;
    wr_udev->cd_udev.devnum = wr_udev->ld_udev->devnum;

    /*selecting Max Packet Size for EP0*/
    if(wr_udev->cd_udev.speed == CH9_USB_SPEED_HIGH || wr_udev->cd_udev.speed == CH9_USB_SPEED_FULL)
        wr_udev->cd_ep0_hep.epDesc.wMaxPacketSize = cpu_to_le16(64);
    else
        wr_udev->cd_ep0_hep.epDesc.wMaxPacketSize = cpu_to_le16(8);

    return 0;
}

/*Firmware use this function to connect Linux host endpoint with Core Driver Endpoint*/
int cdn_h_add_endpoint(struct usb_hcd *hcd, struct usb_device *udev, struct usb_host_endpoint *ep)
{
    return 0;
}

int cdn_h_drop_endpoint(struct usb_hcd *hcd, struct usb_device *udev, struct usb_host_endpoint *ep)
{
    return 0;
}

static const struct hc_driver cusb_hc_driver = {
    .description        = "cdn-hcd",
    .product_desc       = "Cadence Host USB Driver",
    .hcd_priv_size      = sizeof(struct wr_cusb *),

    /**
     * generic hardware linkage
     */
    .flags          = HCD_MEMORY | HCD_USB2,

    /**
     * basic lifecycle operations
     */
    .reset          = cdn_h_reset,
    .start          = cdn_h_run,
    .stop           = cdn_h_stop,
    .shutdown       = cdn_h_shutdown,

    /**
     * managing I/O requests and associated device resources
     */
    .urb_enqueue        = cdn_h_urb_enqueue,
    .urb_dequeue        = cdn_h_urb_dequeue,
    .endpoint_disable   = cdn_h_endpoint_disable,

    /*
     * scheduling support
     */
    .get_frame_number   = cdn_h_get_frame,
    .alloc_dev =        cdn_h_alloc_dev,
    .free_dev =     cdn_h_free_dev,

    /*below functions replace working of standard function from Linux USB CORE*/
    .reset_device =  cdn_h_reset_device,
    .update_device = cdn_h_update_device,
    .add_endpoint = cdn_h_add_endpoint,
    .drop_endpoint = cdn_h_drop_endpoint,

    /*
     * root hub support
     */
    .hub_status_data    = cdn_h_hub_status_data,
    .hub_control        = cdn_h_hub_control,
#ifdef  CONFIG_PM
    .bus_suspend        = cdn_h_bus_suspend,
    .bus_resume     = cdn_h_bus_resume,
#endif
};

int cdn_h_host_alloc(struct wr_cusb * wr_cusb)
{
    struct device   *dev = wr_cusb->controller;

    /* usbcore sets dev->driver_data to hcd, and sometimes uses that... */
    wr_cusb->hcd = usb_create_hcd(&cusb_hc_driver, dev, dev_name(dev));
    if (!wr_cusb->hcd)
        return -EINVAL;

   *wr_cusb->hcd->hcd_priv = (unsigned long) wr_cusb;
    wr_cusb->hcd->self.uses_pio_for_control = 0;
    wr_cusb->hcd->uses_new_polling = 1;
    wr_cusb->hcd->has_tt = 1;
    return 0;
}
