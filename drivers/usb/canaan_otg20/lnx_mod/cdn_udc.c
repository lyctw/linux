/**********************************************************************
 * Copyright (C) 2014-2016 Cadence Design Systems, Inc.
 * All rights reserved worldwide.
 ***********************************************************************
 * cdn_udc.c
 ***********************************************************************/
#include <linux/version.h>

#define DMA_ADDR_INVALID    (~(dma_addr_t)0)

struct wr_request {
    struct usb_request      request;
    CUSBD_Req               *cd_request;
    struct list_head        list;
    struct wr_ep            *ep;
    struct wr_cusb          *cusb;
    u8 is_tx;               /*endpoint direction */
    u8 epnum;
};

void cusb_g_reset(struct wr_cusb *cusb);

extern unsigned int irq_of_parse_and_map(struct device_node *dev, int index);

static inline struct wr_ep* ls_ep_to_cdn_hw_ep(struct usb_ep *ep)
{
    return ep ? container_of(ep, struct wr_ep, end_point) : NULL;
}

static inline struct wr_request* ls_request_to_wr_request(struct usb_request *req)
{
    return req ? container_of(req, struct wr_request, request) : NULL;
}

static inline struct wr_cusb* ls_gadget_to_wr_cusb(struct usb_gadget *g)
{
    return container_of(g, struct wr_cusb, ls_g);
}

void cdn_g_giveback(struct wr_ep *wr_ep, struct usb_request * ls_request, int cd_status)
{
    struct wr_request   * wr_request;
    struct wr_cusb      * wr_cusb;
    int busy = wr_ep->busy;

    wr_request = ls_request_to_wr_request(ls_request);
    wr_cusb = wr_request->cusb;

    list_del(&wr_request->list);

    if(ls_request->status == -EINPROGRESS) {
        if(cd_status == UHC_ESHUTDOWN) {
            ls_request->status = -ESHUTDOWN;
        } else if(cd_status == UDC_ECONNRESET) {
            ls_request->status = -ECONNRESET;
        } else
            ls_request->status = -wr_request->cd_request->status;
    }

    if (ls_request->status == 0) {
        dev_dbg(wr_ep->cusb->controller, "%s done request %p,  %d/%d\n",
                wr_ep->end_point.name, ls_request,
                ls_request->actual, ls_request->length);
    }
    else {
        dev_dbg(wr_ep->cusb->controller, "%s request %p, %d/%d fault %d\n",
                wr_ep->end_point.name, ls_request,
                ls_request->actual, ls_request->length,
                ls_request->status);
    }

    usb_gadget_unmap_request(&wr_cusb->ls_g, &wr_request->request,
            wr_request->is_tx);


    busy = wr_ep->busy;
    wr_ep->busy =1;
    spin_unlock(&global_cusb->lock);
    if(wr_request->request.complete) {
        wr_request->request.complete(&wr_request->ep->end_point, ls_request);
    }
    spin_lock(&global_cusb->lock);
    wr_ep->busy = busy;

}

/***********************************************************************
        Endpoint Management Functions
***********************************************************************/

static void cdn_g_cd_callback_complete(struct CUSBD_Ep *cd_ep,  CUSBD_Req * cd_request)
{
    struct wr_ep        *hw_ep;
    struct wr_request   *wr_request;
    struct usb_request * usb_request;    

    if(!global_cusb)
        return;

    if(!cd_ep || !cd_request) {
        return;
    }

    wr_request = cd_request->context;
    usb_request =  &wr_request->request;
    hw_ep = wr_request->ep;
    usb_request->actual = cd_request->actual;
    usb_request->length = cd_request->length;

    cdn_g_giveback(hw_ep, usb_request, cd_request->status);
}

/*CALLBACK*/
static void cdn_g_cd_callback_connect(void *pD)
{
    if(!global_cusb)
        return;

    global_cusb->ls_g.speed = global_cusb->cd_g->speed;
    if(global_cusb->cd_g->speed > CH9_USB_SPEED_HIGH)
        global_cusb->ls_g.ep0->maxpacket = 9;
    else
        global_cusb->ls_g.ep0->maxpacket =  global_cusb->cd_g->ep0->maxPacket;
}

static void cdn_g_cd_callback_disconnect(void *pD)
{
    if(!global_cusb)
        return;

    cusb_g_reset(global_cusb);
    mdelay(100);
}

static void cdn_g_cd_callback_resume(void *pD)
{
    return;
}

static void cdn_g_cd_callback_suspend(void *pD)
{
    return;
}

static u32 cdn_g_cd_callback_setup(void *pD, CH9_UsbSetup *ctrl)
{
    int handled =0;

    if(!global_cusb)
        return -1;

    if (!global_cusb->gadget_driver)
        return -EOPNOTSUPP;

    /*Firmware has to remembers information about direction of Data Stage*/
    if(ctrl->bmRequestType & USB_DIR_IN)
        global_cusb->ep0_data_stage_is_tx =1;
    else
        global_cusb->ep0_data_stage_is_tx =0;
    spin_unlock(&global_cusb->lock);
    handled = global_cusb->gadget_driver->setup(&global_cusb->ls_g, (struct usb_ctrlrequest*)ctrl);
    spin_lock(&global_cusb->lock);

    if(handled == 0x7FFF) {
        return handled;
    }

    if (handled < 0) {
        return 1;
    }

    return 0;
}


static void *cdn_g_cd_callback_request_alloc(void *pD, u32 size)
{
    CUSBD_Req * cusbd_req = NULL;

    cusbd_req = kzalloc(size, GFP_NOWAIT);
    if (!cusbd_req) {
        return NULL;
    }
    return cusbd_req;
}

static void cdn_g_cd_callback_request_free(void *pD,void *usbRequest)
{
    kfree(usbRequest);
}

/*---------------------------------------------------------------------
 * allocate a request object used by this endpoint
 * the main operation is to insert the req->queue to the eq->queue
 * Returns the request, or null if one could not be allocated
*---------------------------------------------------------------------*/

static int cdn_g_ep_enable(struct usb_ep * ls_ep,
        const struct usb_endpoint_descriptor *desc)
{
    struct wr_ep * wr_ep = NULL;
    struct wr_cusb * wr_cusb;
    u32 wr_status =0;
    unsigned long lockflags;
    if(!ls_ep || !desc)
        return -EINVAL;

    wr_ep = ls_ep_to_cdn_hw_ep(ls_ep);
    wr_cusb = wr_ep->cusb;

    if(wr_ep->desc) {
        return  -EBUSY;
    }
    spin_lock_irqsave(&wr_cusb->lock, lockflags);
    wr_ep->desc = desc;
    wr_ep->busy=0;
    wr_status  =  wr_cusb->cd_g_obj->epEnable(wr_cusb->cd_g_priv, wr_ep->cd_ep, (const CH9_UsbEndpointDescriptor*)desc);
    if(wr_status > 0) {
    }
    spin_unlock_irqrestore(&wr_cusb->lock, lockflags);
    return 0;
}

/*---------------------------------------------------------------------
 * @ep : the ep being unconfigured. May not be ep0
 * Any pending and uncomplete req will complete with status (-ESHUTDOWN)
*---------------------------------------------------------------------*/
static int cdn_g_ep_disable(struct usb_ep * ls_ep)
{
    struct wr_ep * wr_ep = NULL;
    struct wr_cusb * wr_cusb;
    u32 wr_status =0;
    unsigned long lockflags;

    wr_ep = ls_ep_to_cdn_hw_ep(ls_ep);
    wr_cusb = wr_ep->cusb;

    spin_lock_irqsave(&wr_cusb->lock, lockflags);
    wr_ep->desc = NULL;
    wr_ep->busy=0;
    wr_ep->end_point.desc = NULL;
    wr_status  =  wr_cusb->cd_g_obj->epDisable(wr_cusb->cd_g_priv, wr_ep->cd_ep);
    if(wr_status > 0) {
    }
    spin_unlock_irqrestore(&wr_cusb->lock, lockflags);
    return 0;
}

/**
 * Function queues (submits) an I/O request to an endpoint
 */
static int cdn_g_ep_queue(struct usb_ep * ls_ep, struct usb_request * ls_req, gfp_t gfp_flags)
{
    struct wr_ep    * wr_ep;
    struct wr_request   * wr_request;
    struct wr_cusb      * wr_cusb;
    int         status = 0;
    unsigned long       lockflags;

    if (!ls_ep || !ls_req)
        return -EINVAL;
    if (!ls_req->buf)
        return -ENODATA;

    wr_ep = ls_ep_to_cdn_hw_ep(ls_ep);
    wr_cusb = wr_ep->cusb;

    wr_request = ls_request_to_wr_request(ls_req);
    wr_request->cusb = wr_cusb;
    if (wr_request->ep !=wr_ep)
        return -EINVAL;

    dev_dbg(wr_cusb->controller, "<== to %s request=%p\n", ls_ep->name, ls_req);
    wr_request->request.actual = 0;
    wr_request->request.status = -EINPROGRESS;
    wr_request->epnum = wr_ep->ep_num;
    wr_request->is_tx = wr_ep->is_tx;

    wr_request->cd_request->length = ls_req->length;
    wr_request->cd_request->status =0;
    wr_request->cd_request->complete = cdn_g_cd_callback_complete;
    wr_request->cd_request->buf = ls_req->buf;
    wr_request->cd_request->context = ls_req;

    status = usb_gadget_map_request(&wr_cusb->ls_g, &wr_request->request,
                        wr_request->is_tx);

    /* don't queue if the ep is down */
    if (!wr_ep->desc) {
        dev_dbg(wr_cusb->controller, "req %p queued to %s while ep %s\n",
                wr_request, ls_ep->name, "disabled");
        status = -ESHUTDOWN;
        usb_gadget_unmap_request(&wr_cusb->ls_g, &wr_request->request,
                        wr_request->is_tx);
        return status;
    }

    spin_lock_irqsave(&wr_cusb->lock, lockflags);
    wr_request->cd_request->dma = wr_request->request.dma;

    list_add_tail(&wr_request->list, &wr_ep->req_list);

    dev_dbg(wr_cusb->controller, "queue to %s (%s), length=%d\n",
                wr_ep->name, wr_ep->is_tx ? "IN/TX" : "OUT/RX",
                wr_request->request.length);
    status = wr_cusb->cd_g_obj->reqQueue(wr_cusb->cd_g_priv, wr_ep->cd_ep, wr_request->cd_request);

    spin_unlock_irqrestore(&wr_cusb->lock, lockflags);
    return status;
}

/* dequeues (cancels, unlinks) an I/O request from an endpoint */
static int cdn_g_ep_dequeue(struct usb_ep * ep, struct usb_request * request)
{
    struct wr_ep    * wr_ep;
    struct wr_request   * wr_request;
    struct wr_cusb      * wr_cusb;
    int         status = 0;
    unsigned long       lockflags;
    struct wr_request   * wr_next_request;

    wr_ep = ls_ep_to_cdn_hw_ep(ep);
    wr_cusb = wr_ep->cusb;
    wr_request = ls_request_to_wr_request(request);

    if (!ep || !request || wr_request->ep != wr_ep)
        return -EINVAL;


    spin_lock_irqsave(&wr_cusb->lock, lockflags);

    list_for_each_entry(wr_next_request, &wr_ep->req_list, list) {
        if (wr_next_request == wr_request) {
            break;
        }
    }

    if (wr_next_request != wr_request) {
        dev_dbg(wr_cusb->controller, "request %p not queued to %s\n", request, ep->name);
        status = -EINVAL;
        goto done;
    }

    status = wr_cusb->cd_g_obj->reqDequeue(wr_cusb->cd_g_priv, wr_ep->cd_ep, wr_request->cd_request);

done:
    spin_unlock_irqrestore(&wr_cusb->lock, lockflags);
    return status;
}

/*-------------------------------------------------------------------------*/

static int cdn_g_ep_set_wedge(struct usb_ep * ls_ep){
    struct wr_ep *hw_ep = ls_ep_to_cdn_hw_ep(ls_ep);
    struct wr_cusb *wr_cusb = hw_ep->cusb;
    CUSBD_Ep *cd_ep = hw_ep->cd_ep;
    int cd_status = 0;

    if (!ls_ep)
        return -EINVAL;

    cd_status = wr_cusb->cd_g_obj->epSetWedge(wr_cusb->cd_g_priv, cd_ep);
    if(cd_status > 0) {
        return -cd_status;
    }
    return 0;
}

/**
 * modify the endpoint halt feature
 * @ep: the non-isochronous endpoint being stalled
 * @value: 1--set halt  0--clear halt
 * Returns zero, or a negative error code.
 */
static int cdn_g_ep_set_halt(struct usb_ep *ls_ep, int value)
{
    struct wr_ep *wr_ep = NULL;
    struct wr_cusb *wr_cusb = NULL;
    CUSBD_Ep *cd_ep = NULL;
    int cd_status = 0;
    unsigned long           flags;

    if (!ls_ep)
        return -EINVAL;

    wr_ep = ls_ep_to_cdn_hw_ep(ls_ep);
    wr_cusb = wr_ep->cusb;
    cd_ep = wr_ep->cd_ep;
    spin_lock_irqsave(&wr_cusb->lock, flags);
    cd_status = wr_cusb->cd_g_obj->epSetHalt(wr_cusb->cd_g_priv, cd_ep, value);
    spin_unlock_irqrestore(&wr_cusb->lock, flags);
    if(cd_status > 0) {
        return -cd_status;
    }
    return 0;

}

static int cdn_g_ep_fifo_status(struct usb_ep *ls_ep)
{
    return 0;
}

static void cdn_g_ep_fifo_flush(struct usb_ep *ls_ep)
{
    return;
}

struct usb_request *cdn_g_alloc_request(struct usb_ep *ls_ep, gfp_t gfp_flags)
{
    struct wr_ep *hw_ep = ls_ep_to_cdn_hw_ep(ls_ep);
    struct wr_cusb *wr_cusb = hw_ep->cusb;
    CUSBD_Ep *cd_ep = hw_ep->cd_ep;
    struct wr_request *wr_request = NULL;

    wr_request = kzalloc(sizeof(*wr_request), gfp_flags);
    if (!wr_request) {
        dev_dbg(wr_cusb->controller, "not enough memory\n");
        return NULL;
    }

    INIT_LIST_HEAD(&wr_request->list);
    wr_request->request.dma = DMA_ADDR_INVALID;
    wr_request->epnum = hw_ep->ep_num;
    wr_request->ep = hw_ep;
    wr_request->cusb = hw_ep->cusb;
    wr_cusb->cd_g_obj->reqAlloc(wr_cusb->cd_g_priv, cd_ep, &wr_request->cd_request);

    return &wr_request->request;
}

/**
 * Free a request
 * Reused by ep0 code.
 */
void cdn_g_free_request(struct usb_ep *ls_ep, struct usb_request *ls_req)
{
    struct wr_request *wr_request;
    struct wr_cusb *wr_cusb;
    struct wr_ep * wr_ep;

    wr_request = ls_request_to_wr_request(ls_req);
    wr_cusb = wr_request->cusb;
    wr_ep = ls_ep_to_cdn_hw_ep(ls_ep);

    wr_cusb->cd_g_obj->reqFree(wr_cusb->cd_g_priv, wr_ep->cd_ep, wr_request->cd_request);
    kfree(wr_request);

}

static struct usb_ep_ops cdn_g_ep_ops = {
    .enable = cdn_g_ep_enable,
    .disable = cdn_g_ep_disable,

    .alloc_request = cdn_g_alloc_request,
    .free_request = cdn_g_free_request,

    .queue = cdn_g_ep_queue,
    .dequeue = cdn_g_ep_dequeue,
    .set_wedge = cdn_g_ep_set_wedge,
    .set_halt = cdn_g_ep_set_halt,
    .fifo_status = cdn_g_ep_fifo_status,
    .fifo_flush = cdn_g_ep_fifo_flush,  /* flush fifo */
};

/*-------------------------------------------------------------------------
        Gadget Driver Layer Operations
-------------------------------------------------------------------------*/

/*----------------------------------------------------------------------
 * Get the current frame number (from DR frame_index Reg )
 *----------------------------------------------------------------------*/
static int cdn_g_get_frame(struct usb_gadget *gadget)
{
    return 0;
}

/*-----------------------------------------------------------------------
 * Tries to wake up the host connected to this gadget
 -----------------------------------------------------------------------*/
static int cdn_g_wakeup(struct usb_gadget *gadget)
{
    return 0;
}


/* Notify controller that VBUS is powered, Called by whatever
   detects VBUS sessions */
static int cdn_g_vbus_session(struct usb_gadget *gadget, int is_active)
{
    return 0;
}

/* constrain controller's VBUS power usage
 * This call is used by gadget drivers during SET_CONFIGURATION calls,
 * reporting how much power the device may consume.  For example, this
 * could affect how quickly batteries are recharged.
 *
 * Returns zero on success, else negative errno.
 */
static int cdn_g_vbus_draw(struct usb_gadget *gadget, unsigned mA)
{
    return 0;
}

/* Change Data+ pullup status
 * this func is used by usb_gadget_connect/disconnet
 */
static int cdn_g_pullup(struct usb_gadget *gadget, int is_on)
{
    return 0;
}

static int cdn_g_start(struct usb_gadget * gadget,
        struct usb_gadget_driver * driver);

static int cdn_g_stop(struct usb_gadget * gadget);

/* defined in gadget.h */
static struct usb_gadget_ops cdn_gadget_ops = {
    .get_frame = cdn_g_get_frame,
    .wakeup = cdn_g_wakeup,
    .vbus_session = cdn_g_vbus_session,
    .vbus_draw = cdn_g_vbus_draw,
    .pullup = cdn_g_pullup,
    .udc_start = cdn_g_start,
    .udc_stop = cdn_g_stop,
};

/*********************** Operations on  EP0  ********************************/
static int cdn_g_ep0_enable(struct usb_ep *ep, const struct usb_endpoint_descriptor *desc)
{
    /* always enabled */
    return -EINVAL;
}

static int cdn_g_ep0_disable(struct usb_ep * ls_ep)
{
    /* always enabled */
    return -EINVAL;
}


static int cdn_g_ep0_queue(struct usb_ep * ls_ep, struct usb_request * ls_request, gfp_t gfp_flags)
{
    struct wr_ep        *wr_ep;
    struct wr_request   *wr_request;
    struct wr_cusb      *wr_cusb;
    int         status;
    unsigned long       lockflags;

    if (!ls_ep || !ls_request)
        return -EINVAL;

    wr_ep = ls_ep_to_cdn_hw_ep(ls_ep);
    wr_cusb = wr_ep->cusb;

    wr_request = ls_request_to_wr_request(ls_request);

    spin_lock_irqsave(&wr_cusb->lock, lockflags);
    if (!list_empty(&wr_ep->req_list)) {
        status = -EBUSY;
        goto cleanup;
    }
    wr_request->cusb = wr_cusb;
    wr_request->request.actual = 0;
    wr_request->cd_request->actual = 0;
    wr_request->request.status = -EINPROGRESS;
    wr_request->is_tx = global_cusb->ep0_data_stage_is_tx;
    wr_request->cd_request->length = wr_request->request.length;
    wr_request->cd_request->status =0;
    wr_request->cd_request->complete = cdn_g_cd_callback_complete;
    wr_request->cd_request->buf = wr_request->request.buf;
    wr_request->cd_request->context = wr_request;

    status = usb_gadget_map_request(&wr_cusb->ls_g, &wr_request->request,
            wr_request->is_tx);
    if (status) {
        dev_dbg(wr_cusb->controller, "failed to map request\n");
        status =-EINVAL;
        goto cleanup;
    }

    wr_request->cd_request->dma = wr_request->request.dma;

    /* add request to the list */
    list_add_tail(&wr_request->list, &wr_ep->req_list);

    dev_dbg(wr_cusb->controller, "queue to %s (%s), length=%d\n",
            wr_ep->name, wr_ep->is_tx ? "IN/TX" : "OUT/RX",
                    wr_request->request.length);

    status = wr_cusb->cd_g_obj->reqQueue(wr_cusb->cd_g_priv, wr_ep->cd_ep, wr_request->cd_request);

    if(status >0) {
        status = -status;
        usb_gadget_unmap_request(&wr_cusb->ls_g, &wr_request->request,
                            wr_request->is_tx);
        list_del(&wr_request->list);
        goto cleanup;
    }
cleanup:
    spin_unlock_irqrestore(&wr_cusb->lock, lockflags);
    return status;

}

static int cdn_g_ep0_dequeue(struct usb_ep *ls_ep, struct usb_request * ls_request)
{
    /* we just won't support this */
    return -EOPNOTSUPP;
}

static int cdn_g_ep0_halt(struct usb_ep *ls_ep, int value)
{
    return 0;
}


const struct usb_ep_ops cdn_g_ep0_ops = {
    .enable         = cdn_g_ep0_enable,
    .disable        = cdn_g_ep0_disable,
    .alloc_request  = cdn_g_alloc_request,
    .free_request   = cdn_g_free_request,
    .queue          = cdn_g_ep0_queue,
    .dequeue        = cdn_g_ep0_dequeue,
    .set_halt       = cdn_g_ep0_halt,
};

/*************************************************************/
static void cdn_g_init_peripheral_ep(struct wr_cusb *wr_cusb, struct wr_ep *wr_ep,
    CUSBD_Ep * cd_ep, int is_tx)
{
    memset(wr_ep, 0, sizeof *wr_ep);
    wr_ep->cusb = wr_cusb;
    wr_ep->is_tx = is_tx;
    wr_ep->cd_ep = cd_ep;
    wr_ep->ep_num = cd_ep->address & 0x0F;
    wr_ep->end_point.maxpacket =  cd_ep->maxPacket;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0)
    wr_ep->end_point.maxpacket_limit = 1024;
    wr_ep->end_point.caps.dir_in = true;
	wr_ep->end_point.caps.dir_out = true;
    wr_ep->end_point.caps.type_iso = true;
    wr_ep->end_point.caps.type_bulk = true;
    wr_ep->end_point.caps.type_int = true;
#endif
    INIT_LIST_HEAD(&wr_ep->req_list);

    sprintf(wr_ep->name, "ep%d%s", wr_ep->ep_num , is_tx ? "in" : "out");
    wr_ep->end_point.name = wr_ep->name;
    INIT_LIST_HEAD(&wr_ep->end_point.ep_list);
    if (!wr_ep->ep_num ) {
        wr_ep->end_point.ops = &cdn_g_ep0_ops;
        wr_cusb->ls_g.ep0 = &wr_ep->end_point;
        if(wr_cusb->cd_g->maxSpeed > CH9_USB_SPEED_HIGH) wr_cusb->ls_g.ep0->maxpacket = 9;
    } else {
        wr_ep->end_point.ops = &cdn_g_ep_ops;
        list_add_tail(&wr_ep->end_point.ep_list, &wr_cusb->ls_g.ep_list);
    }
}

static inline void cdn_g_init_endpoints(struct wr_cusb *wr_cusb)
{
    unsigned        count = 0;
    CUSBD_ListHead *list;
    /* initialize endpoint list just once */
    INIT_LIST_HEAD(&(wr_cusb->ls_g.ep_list));


    /*initialize endpoint 0*/
    cdn_g_init_peripheral_ep(wr_cusb, &wr_cusb->endpoints_tx[0], wr_cusb->cd_g->ep0, 1);
    cdn_g_init_peripheral_ep(wr_cusb, &wr_cusb->endpoints_rx[0], wr_cusb->cd_g->ep0, 0);
    count++;

    for(list = wr_cusb->cd_g->epList.next; list != &wr_cusb->cd_g->epList; list=list->next) {
        CUSBD_Ep * cusbd_ep = (CUSBD_Ep*)list;
        if(cusbd_ep->address & USB_DIR_IN) {
            cdn_g_init_peripheral_ep(wr_cusb, &wr_cusb->endpoints_tx[cusbd_ep->address & 0x0f], cusbd_ep, 1);
            count++;
        }
        else {
            cdn_g_init_peripheral_ep(wr_cusb, &wr_cusb->endpoints_rx[cusbd_ep->address & 0x0f], cusbd_ep, 0);
            count++;
        }
    }
}

/* called once during driver setup to initialize and link into
 * the driver model; memory is zeroed.
 */
int cdn_g_setup(struct wr_cusb *wr_cusb)
{
    int status;
    wr_cusb->cd_g_obj->getDevInstance(wr_cusb->cd_g_priv, &wr_cusb->cd_g);
    wr_cusb->ls_g.ops = &cdn_gadget_ops;
    wr_cusb->ls_g.max_speed = wr_cusb->cd_g->maxSpeed;
    wr_cusb->ls_g.speed = wr_cusb->cd_g->speed;
    wr_cusb->is_host = 0;

    wr_cusb->ls_g.name = DRV_NAME;
    if(wr_cusb->cd_g_config.isOtg) {
        // wr_cusb->transceiver->otg->default_a = 0;
        // wr_cusb->transceiver->state= OTG_STATE_B_IDLE;
        wr_cusb->ls_g.is_otg = 1;
    }
    else
        wr_cusb->ls_g.is_otg = 0;

    cdn_g_init_endpoints(wr_cusb);

    status = usb_add_gadget_udc(wr_cusb->controller, &wr_cusb->ls_g);
    if (status)
        goto err;

    return 0;
err:
wr_cusb->ls_g.dev.parent = NULL;
    device_unregister(&wr_cusb->ls_g.dev);
    return status;
}


void cdn_g_cleanup(struct wr_cusb *wr_cusb)
{
    if (wr_cusb->cd_g_config.isEmbeddedHost)
        return;
    usb_del_gadget_udc(&wr_cusb->ls_g);
}

/**
 * Register the gadget driver. Used by gadget drivers when
 * registering themselves with the controller.
 *
 * -EINVAL something went wrong (not driver)
 * -EBUSY another gadget is already using the controller
 * -ENOMEM no memory to perform the operation
 *
 * @param driver the gadget driver
 * @return <0 if error, 0 if everything is fine
 */
static int cdn_g_start(struct usb_gadget *g,
        struct usb_gadget_driver *driver)
{
    struct wr_cusb      *wr_cusb;
    unsigned long       flags;
    int         retval = 0;

    wr_cusb = ls_gadget_to_wr_cusb(g);

    if (driver->max_speed < USB_SPEED_HIGH) {
        retval = -EINVAL;
        goto err;
    }

    dev_dbg(wr_cusb->controller, "registering driver %s\n", driver->function);

    wr_cusb->gadget_driver = driver;

    spin_lock_irqsave(&wr_cusb->lock, flags);

    spin_unlock_irqrestore(&wr_cusb->lock, flags);

    if(wr_cusb->cd_g_config.isDevice)
        wr_cusb->cd_g_obj->start(wr_cusb->cd_g_priv);
    return 0;

err:
    return retval;
}


/*
 * Unregister the gadget driver. Used by gadget drivers when
 * unregistering themselves from the controller.
 *
 * @param driver the gadget driver to unregister
 */
static int cdn_g_stop(struct usb_gadget *g)
{
    struct wr_cusb  *wr_cusb = ls_gadget_to_wr_cusb(g);
    unsigned long   flags;
    spin_lock_irqsave(&wr_cusb->lock, flags);

    // dev_dbg(wr_cusb->controller, "unregistering driver %s\n",
    //               driver ? driver->function : "(removed)");

    wr_cusb->gadget_driver = NULL;
    spin_unlock_irqrestore(&wr_cusb->lock, flags);
    return 0;
}

/* called when VBUS drops below session threshold, and in other cases */
void cusb_g_disconnect(struct wr_cusb  *wr_cusb)
{
    wr_cusb->ls_g.speed = USB_SPEED_UNKNOWN;
    if (wr_cusb->gadget_driver && wr_cusb->gadget_driver->disconnect) {
        spin_unlock(&wr_cusb->lock);
        wr_cusb->gadget_driver->disconnect(&wr_cusb->ls_g);
        spin_lock(&wr_cusb->lock);
    }
}

void cusb_g_reset(struct wr_cusb *wr_cusb)
__releases(wr_cusb->lock)
__acquires(wr_cusb->lock)
{

    dev_dbg(wr_cusb->controller, "<== driver '%s'\n",
            wr_cusb->gadget_driver
                ? wr_cusb->gadget_driver->driver.name
                : NULL
            );

    /* report disconnect, if we didn't already (flushing EP state) */
    if (wr_cusb->ls_g.speed != USB_SPEED_UNKNOWN)
        cusb_g_disconnect(wr_cusb);

    wr_cusb->ls_g.speed = wr_cusb->cd_g->speed;
    wr_cusb->is_host = 0;
}

